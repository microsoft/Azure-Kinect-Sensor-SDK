// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/logging.h>

#include <k4ainternal/global.h>

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

// This is the context for built in logger that uses env variable to the users logger.
typedef struct _logger_context_t
{
    std::shared_ptr<spdlog::logger> logger;
} logger_context_t;

K4A_DECLARE_CONTEXT(logger_t, logger_context_t);

typedef struct
{
    k4a_rwlock_t lock;

    // Logger data for forwarding debug messages to registered callback.
    k4a_logging_message_cb_t *user_callback;
    void *user_callback_context;
    k4a_log_level_t user_log_level;

    // Logger data used for forwarding debug message to stdout or a dedicated k4a file.
    std::shared_ptr<spdlog::logger> env_logger;
    bool env_logger_is_file_based;
    k4a_log_level_t env_log_level;
    int env_logger_count;
} logger_global_context_t;

// This function runs once to initialize the global context for the logger
static void logger_init_function(logger_global_context_t *global)
{
    rwlock_init(&global->lock);

    global->env_log_level = K4A_LOG_LEVEL_OFF;
    global->user_log_level = K4A_LOG_LEVEL_OFF;
}

K4A_DECLARE_GLOBAL(logger_global_context_t, logger_init_function);

#define K4A_LOGGER "k4a_logger"

// NOTE, if a sub directory for the log is used, then it needs to be created prior to attempting to create the file
#define LOG_FILE_MAX_FILES (3)
#define LOG_FILE_EXTENSION ".log"

k4a_result_t logger_register_message_callback(k4a_logging_message_cb_t *message_cb,
                                              void *message_cb_context,
                                              k4a_log_level_t min_level)
{
    logger_global_context_t *g_context = logger_global_context_t_get();
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    // Validate parameters
    if (message_cb)
    {
        RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, min_level < K4A_LOG_LEVEL_CRITICAL);
        RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, min_level > K4A_LOG_LEVEL_OFF);
    }

    rwlock_acquire_write(&g_context->lock);

    // The user may set or clear the callback, or set the callback to the
    // same value it was previously. It is a failure to change the callback
    // from an existing registration.
    if (g_context->user_callback == NULL || message_cb == NULL || g_context->user_callback == message_cb)
    {
        // Store the user log level
        g_context->user_log_level = min_level;
        g_context->user_callback = message_cb;
        g_context->user_callback_context = message_cb_context;
    }
    else
    {
        // Don't call any functions/macros that may log while we hold the lock
        result = K4A_RESULT_FAILED;
    }

    rwlock_release_write(&g_context->lock);

    return result;
}

k4a_result_t logger_create(logger_config_t *config, logger_t *logger_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, logger_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config->max_log_size == 0);

    logger_context_t *context = logger_t_create(logger_handle);
    logger_global_context_t *g_context = logger_global_context_t_get();

    rwlock_acquire_write(&g_context->lock);

    const char *enable_file_logging = nullptr;
    const char *enable_stdout_logging = nullptr;
    const char *logging_level = nullptr;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

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

    if (g_context->env_logger)
    {
        // Reuse the configured logger settings in the event we have more than 1 active handle at a time
        context->logger = g_context->env_logger;
        g_context->env_logger_count++;

        rwlock_release_write(&g_context->lock);
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
            try
            {
                // Create a file rotating logger with 50mb size max and 3 rotated files
                g_context->env_logger =
                    spdlog::rotating_logger_mt(K4A_LOGGER, log_file, config->max_log_size, LOG_FILE_MAX_FILES);

                spdlog::set_pattern("%v");
                g_context->env_logger->info("\n\nNew logging session started\n");
                g_context->env_logger_is_file_based = true;
            }
            catch (...)
            {
                // Probably trying to use a file that is already opened by another instance.
                g_context->env_logger = nullptr;
                printf("ERROR: Unable to open log file \"%s\".\n", log_file);
                result = K4A_RESULT_FAILED;
            }
        }
    }

    // log to stdout if enabled via ENV var AND if file logging is not enabled.
    if (K4A_SUCCEEDED(result) && (g_context->env_logger == NULL))
    {
        bool enable_stdout_logger = false;

        // Default with user_callback enabled is no stdout logging unless specifically enabled
        if (enable_stdout_logging && enable_stdout_logging[0] != '0')
        {
            enable_stdout_logger = true;
        }
        else if (g_context->user_callback == NULL)
        {
            // Default with user_callback disabled is use stdout logging unless specifically disabled
            if (g_context->user_callback == NULL &&
                (enable_stdout_logging == NULL || enable_stdout_logging[0] == '\0' ||
                 (enable_stdout_logging && enable_stdout_logging[0] != '0')))
            {
                enable_stdout_logger = true;
            }
        }

        if (enable_stdout_logger)
        {
            // Unable to turn on color logging due to bug with CTest https://gitlab.kitware.com/cmake/cmake/issues/17620
            g_context->env_logger = spdlog::stdout_logger_mt(K4A_LOGGER);
        }
    }

    if (K4A_SUCCEEDED(result) && (g_context->env_logger))
    {
        context->logger = g_context->env_logger;
        g_context->env_logger_count++;
        g_context->env_log_level = K4A_LOG_LEVEL_ERROR;

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
                g_context->env_log_level = K4A_LOG_LEVEL_TRACE;
            }
            else if (logging_level[0] == 'i' || logging_level[0] == 'I')
            {
                // capture a severity of info or higher
                g_context->env_log_level = K4A_LOG_LEVEL_INFO;
            }
            else if (logging_level[0] == 'w' || logging_level[0] == 'W')
            {
                // capture a severity of warning or higher
                g_context->env_log_level = K4A_LOG_LEVEL_WARNING;
            }
            else if (logging_level[0] == 'e' || logging_level[0] == 'E')
            {
                // capture a severity of error or higher
                g_context->env_log_level = K4A_LOG_LEVEL_ERROR;
            }
            else if (logging_level[0] == 'c' || logging_level[0] == 'C')
            {
                // capture a severity of error or higher
                g_context->env_log_level = K4A_LOG_LEVEL_CRITICAL;
            }
        }

        g_context->env_logger->flush_on(spdlog::level::warn);
    }

    rwlock_release_write(&g_context->lock);

    return result;
}

void logger_destroy(logger_t logger_handle)
{
    RETURN_VALUE_IF_HANDLE_INVALID(VOID_VALUE, logger_t, logger_handle);

    logger_context_t *context = logger_t_get_context(logger_handle);
    logger_global_context_t *g_context = logger_global_context_t_get();

    rwlock_acquire_write(&g_context->lock);

    g_context->env_logger_count--;

    // Destroy the logger
    if (g_context->env_logger_count == 0)
    {
        bool drop_logger = g_context->env_logger != NULL;
        g_context->env_logger = NULL;
        if (drop_logger)
        {
            spdlog::drop(K4A_LOGGER);
        }
        g_context->env_logger_is_file_based = false;
        g_context->env_log_level = K4A_LOG_LEVEL_OFF;
    }
    context->logger = nullptr;

    rwlock_release_write(&g_context->lock);

    logger_t_destroy(logger_handle);
}

#if defined(__GNUC__) || defined(__clang__)
// Enable printf type checking in clang and gcc
__attribute__((__format__ (__printf__, 2, 0)))
#endif
void logger_log(k4a_log_level_t level, const char * file, const int line, const char *format, ...)
{
    logger_global_context_t *g_context = logger_global_context_t_get();

    rwlock_acquire_read(&g_context->lock);

    // Quick exit if we are not logging the message
    if ((level > g_context->env_log_level && level > g_context->user_log_level) ||
        (!g_context->env_logger && !g_context->user_callback))
    {
        rwlock_release_read(&g_context->lock);
        return;
    }

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

    if ((level <= g_context->user_log_level) && (g_context->user_log_level != K4A_LOG_LEVEL_OFF))
    {
        if (g_context->user_callback)
        {
            g_context->user_callback(g_context->user_callback_context, level, file, line, buffer);
        }
    }

    if ((level <= g_context->env_log_level) && (g_context->env_log_level != K4A_LOG_LEVEL_OFF))
    {
        if (g_context->env_logger)
        {
            switch (level)
            {
            case K4A_LOG_LEVEL_CRITICAL:
                g_context->env_logger->critical("{0} ({1}): {2}", file, line, buffer);
                break;
            case K4A_LOG_LEVEL_ERROR:
                g_context->env_logger->error("{0} ({1}): {2}", file, line, buffer);
                break;
            case K4A_LOG_LEVEL_WARNING:
                g_context->env_logger->warn("{0} ({1}): {2}", file, line, buffer);
                break;
            case K4A_LOG_LEVEL_INFO:
                g_context->env_logger->info("{0} ({1}): {2}", file, line, buffer);
                break;
            case K4A_LOG_LEVEL_TRACE:
            default:
                g_context->env_logger->trace("{0} ({1}): {2}", file, line, buffer);
                break;
            }
        }
    }

    rwlock_release_read(&g_context->lock);
}

bool logger_is_file_based()
{
    logger_global_context_t *g_context = logger_global_context_t_get();
    return g_context->env_logger_is_file_based;
}

#ifdef __cplusplus
}
#endif
