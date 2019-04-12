// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/logging.h>

#include <azure_c_shared_utility/envvariable.h>
#include <azure_c_shared_utility/refcount.h>
#include <azure_c_shared_utility/threadapi.h>

// System dependencies
#include <stdlib.h>
#include <string.h>

// External dependencies

// SPDLOG has an ASSERT in an unreachable part of code, which results in a warning
// and we are treating warnings as errors.
#ifdef _MSC_VER
#pragma warning(disable : 4702)
#endif
#include <spdlog/spdlog.h>
#ifdef _MSC_VER
#pragma warning(default : 4702)
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// This logger implements two different loggers, that have different lifespans.
// 1) A streaming callback function can deliver debug messages to a registered callback function.
// 2) This module and therefore k4a.dll is capable of independently logging to a file or STDOUT through the use of
// environment variables defined in k4atypes.h
//

// This is the context for the users logger. Otherwise known as the registered callback function.
typedef struct _logger_user_cb_info_t
{
    k4a_logging_message_cb_t *callback;
    void *callback_context;
} logger_user_cb_info_t;

// This is the context for built in logger that uses env variable to the users logger.
typedef struct _logger_context_t
{
    std::shared_ptr<spdlog::logger> logger;
} logger_context_t;

K4A_DECLARE_CONTEXT(logger_t, logger_context_t);

// Logger data for forwarding debug messages to registered callback.
static volatile logger_user_cb_info_t *g_user_logger_cb_info = nullptr;
static volatile long g_user_logger_cb_info_ref = 0;
static k4a_log_level_t g_user_log_level = K4A_LOG_LEVEL_OFF;

// Logger data used for forwarding debug message to stdout or a dedicated k4a file.
static std::shared_ptr<spdlog::logger> g_env_logger = nullptr;
static volatile long g_env_logger_count = 0;
static bool g_env_logger_is_file_based = false;
static k4a_log_level_t g_env_log_level = K4A_LOG_LEVEL_OFF;

#define K4A_LOGGER "k4a_logger"

// NOTE, if a sub directory for the log is used, then it needs to be created prior to attempting to create the file
#define LOG_FILE_MAX_FILES (3)
#define LOG_FILE_EXTENSION ".log"

k4a_result_t logger_register_message_callback(k4a_logging_message_cb_t *message_cb,
                                              void *message_cb_context,
                                              k4a_log_level_t min_level)
{
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    if (message_cb)
    {
        result = K4A_RESULT_FROM_BOOL(min_level >= K4A_LOG_LEVEL_CRITICAL && min_level <= K4A_LOG_LEVEL_OFF);
        if (K4A_SUCCEEDED(result))
        {
            long count_of_registered_callbacks = INC_REF_VAR(g_user_logger_cb_info_ref);
            if (count_of_registered_callbacks == 1)
            {
                // We won the right to register and create logger_cb_t
                g_user_logger_cb_info = new logger_user_cb_info_t();
                g_user_logger_cb_info->callback = message_cb;
                g_user_logger_cb_info->callback_context = message_cb_context;
                g_user_log_level = min_level;
            }
            else if (g_user_logger_cb_info->callback == message_cb)
            {
                // User is calling to update the min_level
                g_user_log_level = min_level;
                DEC_REF_VAR(g_user_logger_cb_info_ref);
            }
            else
            {
                // Disallow the caller from registering a 2nd callback. A new callback can be established by clearing
                // the existing callback function.
                result = K4A_RESULT_FROM_BOOL(count_of_registered_callbacks == 1);
                DEC_REF_VAR(g_user_logger_cb_info_ref);
                result = K4A_RESULT_FAILED;
            }
        }
    }
    else if (g_user_logger_cb_info)
    {
        volatile logger_user_cb_info_t *logger_info = g_user_logger_cb_info;
        g_user_logger_cb_info = nullptr;
        DEC_REF_VAR(g_user_logger_cb_info_ref);

        // NOTE: this loop could loop forever if user calls start and a new g_user_logger_cb_info is allocated, which
        // would add a ref to g_user_logger_cb_info_ref. We don't expect there to be races between 1 thread trying to
        // release the callback function and another thread to add one. To protect against this we would need a more
        // elaborate locking scheme and the logging functions would having to use it, which would impact performance.
        while (g_user_logger_cb_info_ref != 0)
        {
            // Wait for the parallel calls to logger_log to end
            ThreadAPI_Sleep(10);
        }

        delete logger_info;
        g_user_log_level = K4A_LOG_LEVEL_OFF;
    }
    return result;
}

k4a_result_t logger_create(logger_config_t *config, logger_t *logger_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, logger_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config->max_log_size == 0);
    logger_context_t *context = logger_t_create(logger_handle);
    const char *enable_file_logging = nullptr;
    const char *enable_stdout_logging = nullptr;
    const char *logging_level = nullptr;

    // environment_get_variable will return null or "\0" if the env var is not set - depends on the OS.
    if (config->env_var_log_to_a_file)
    {
        enable_file_logging = environment_get_variable(config->env_var_log_to_a_file);
    }
    if (config->env_var_log_to_stdout)
    {
        enable_stdout_logging = environment_get_variable(config->env_var_log_to_stdout);
    }
    if (config->env_var_log_level)
    {
        logging_level = environment_get_variable(config->env_var_log_level);
    }

#if defined(REFCOUNT_USE_STD_ATOMIC)
    // Validate implementation of INC_REF_VAR. Documentation for the implementation in this mode indicates the API
    // behaves differently than the others. REFCOUNT_USE_STD_ATOMIC returns the pre-opperation value while other return
    // the post opperation value
    static_assert(0, "This configuration needs to be confirmed");
#endif
    if ((INC_REF_VAR(g_env_logger_count) > 1) || (g_env_logger))
    {
        // Reuse the configured logger settings in the event we have more than 1 active handle at a time
        context->logger = g_env_logger;
        return K4A_RESULT_SUCCEEDED;
    }

    if (enable_file_logging && enable_file_logging[0] != '\0')
    {
        const char *log_file = nullptr;
        if (enable_file_logging == NULL || enable_file_logging[0] == '\0')
        {
            log_file = config->log_file;
        }
        else if (enable_file_logging[0] != '0')
        {
            // enable_file_logging is valid see if its a file path
            log_file = config->log_file;
            uint32_t extension_sz = sizeof(LOG_FILE_EXTENSION);

            // If enable_file_logging ends in .log then we use it as a path and or file name, else we use the default
            uint32_t path_length = (uint32_t)strlen(enable_file_logging) + 1;
            if (path_length > extension_sz)
            {
                const char *extension = enable_file_logging + (char)(path_length - extension_sz);
                if (strncmp(extension, LOG_FILE_EXTENSION, extension_sz) == 0)
                {
                    log_file = enable_file_logging;
                }
            }
        }

        if (log_file)
        {
            // Create a file rotating logger with 50mb size max and 3 rotated files
            g_env_logger = spdlog::rotating_logger_mt(K4A_LOGGER, log_file, config->max_log_size, LOG_FILE_MAX_FILES);

            spdlog::set_pattern("%v");
            g_env_logger->info("\n\nNew logging session started\n");
            g_env_logger_is_file_based = true;
        }
    }

    // log to stdout if enabled via ENV var AND if file logging is not enabled.
    if (g_env_logger == NULL)
    {
        bool enable_stdout_logger = false;

        // Default with g_user_logger_cb_info enabled is no stdout logging unless specifically enabled
        if (enable_stdout_logging && enable_stdout_logging[0] != '0')
        {
            enable_stdout_logger = true;
        }
        else if (g_user_logger_cb_info == NULL)
        {
            // Default with g_user_logger_cb_info disabled is use stdout logging unless specifically disabled
            if (g_user_logger_cb_info == NULL && (enable_stdout_logging == NULL || enable_stdout_logging[0] == '\0' ||
                                                  (enable_stdout_logging && enable_stdout_logging[0] != '0')))
            {
                enable_stdout_logger = true;
            }
        }

        if (enable_stdout_logger)
        {
            // Unable to turn on color logging due to bug with CTest https://gitlab.kitware.com/cmake/cmake/issues/17620
            g_env_logger = spdlog::stdout_logger_mt(K4A_LOGGER);
        }
    }

    if (g_env_logger)
    {
        context->logger = g_env_logger;
        g_env_log_level = K4A_LOG_LEVEL_ERROR;

        //[2018-08-27 10:44:23.218] [level] [threadID] <message>
        // https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [t=%t] %v");

        // Set the default logging level SPD will allow. g_env_log_level will furthar refine this.
        spdlog::set_level(spdlog::level::trace);

        // override the default logging level
        if (logging_level && logging_level[0] != '\0')
        {
            if (logging_level[0] == 't' || logging_level[0] == 'T')
            {
                // capture a severity of trace or higher
                g_env_log_level = K4A_LOG_LEVEL_TRACE;
            }
            else if (logging_level[0] == 'i' || logging_level[0] == 'I')
            {
                // capture a severity of info or higher
                g_env_log_level = K4A_LOG_LEVEL_INFO;
            }
            else if (logging_level[0] == 'w' || logging_level[0] == 'W')
            {
                // capture a severity of warning or higher
                g_env_log_level = K4A_LOG_LEVEL_WARNING;
            }
            else if (logging_level[0] == 'e' || logging_level[0] == 'E')
            {
                // capture a severity of error or higher
                g_env_log_level = K4A_LOG_LEVEL_ERROR;
            }
            else if (logging_level[0] == 'c' || logging_level[0] == 'C')
            {
                // capture a severity of error or higher
                g_env_log_level = K4A_LOG_LEVEL_CRITICAL;
            }
        }

        g_env_logger->flush_on(spdlog::level::warn);
    }
    return K4A_RESULT_SUCCEEDED;
}

void logger_destroy(logger_t logger_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, logger_t, logger_handle);

    logger_context_t *context = logger_t_get_context(logger_handle);

    // Destroy the logger
    if (DEC_REF_VAR(g_env_logger_count) == 0)
    {
        bool drop_logger = g_env_logger != NULL;
        g_env_logger = NULL;
        if (drop_logger)
        {
            spdlog::drop(K4A_LOGGER);
        }
        g_env_logger_is_file_based = false;
        g_env_log_level = K4A_LOG_LEVEL_OFF;
    }
    context->logger = nullptr;

    logger_t_destroy(logger_handle);
}

#if defined(__GNUC__) || defined(__clang__)
// Enable printf type checking in clang and gcc
__attribute__((__format__ (__printf__, 2, 0)))
#endif
void logger_log(k4a_log_level_t level, const char * file, const int line, const char *format, ...)
{
    // Quick exit if we are not logging the message
    if (level > g_env_log_level && level > g_user_log_level)
    {
        return;
    }

    if (g_env_logger || g_user_logger_cb_info)
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
        vsnprintf(buffer, sizeof(buffer), format, args);
#ifndef _WIN32
#pragma GCC diagnostic pop
#endif
        va_end(args);

        if ((level <= g_user_log_level) && (g_user_log_level != K4A_LOG_LEVEL_OFF))
        {
            // must ++ before getting the shared pointer, or the wait in releasing the callback function can return
            // without waiting for all pending instances using this context to complete.
            INC_REF_VAR(g_user_logger_cb_info_ref);
            volatile logger_user_cb_info_t *logger_cb = g_user_logger_cb_info;
            if (logger_cb)
            {
                logger_cb->callback(logger_cb->callback_context, level, file, line, buffer);
                logger_cb = nullptr;
            }
            DEC_REF_VAR(g_user_logger_cb_info_ref);
        }
        if ((level <= g_env_log_level) && (g_env_log_level != K4A_LOG_LEVEL_OFF))
        {
            // Keep a copy of the logger around while we add this entry
            std::shared_ptr<spdlog::logger> logger = g_env_logger;
            if (logger)
            {
                switch (level)
                {
                case K4A_LOG_LEVEL_CRITICAL:
                    logger->critical("{0} ({1}): {2}", file, line, buffer);
                    break;
                case K4A_LOG_LEVEL_ERROR:
                    logger->error("{0} ({1}): {2}", file, line, buffer);
                    break;
                case K4A_LOG_LEVEL_WARNING:
                    logger->warn("{0} ({1}): {2}", file, line, buffer);
                    break;
                case K4A_LOG_LEVEL_INFO:
                    logger->info("{0} ({1}): {2}", file, line, buffer);
                    break;
                case K4A_LOG_LEVEL_TRACE:
                default:
                    logger->trace("{0} ({1}): {2}", file, line, buffer);
                    break;
                }
            }
            logger = nullptr;
        }
    }
}

bool logger_is_file_based()
{
    return g_env_logger_is_file_based;
}

#ifdef __cplusplus
}
#endif
