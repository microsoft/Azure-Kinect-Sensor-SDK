// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/logging.h>

#include <k4ainternal/global.h>
#include <k4ainternal/rwlock.h>

#include <azure_c_shared_utility/envvariable.h>
#include <azure_c_shared_utility/refcount.h>
#include <azure_c_shared_utility/threadapi.h>

// System dependencies
#include <stdlib.h>
#include <stdio.h>
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
// This logger implements two different loggers
// 1) A streaming callback function can deliver debug messages to a registered callback function.
// 2) This module and therefore consumers are capable of independently logging to a file or STDOUT through the use of
// environment variables defined in k4atypes.h
//

#define K4A_LOGGER "k4a_logger"

// NOTE, if a sub directory for the log is used, then it needs to be created prior to attempting to create the file
#define LOG_FILE_MAX_FILES (3)
#define LOG_FILE_EXTENSION ".log"

static const char K4A_ENABLE_LOG_TO_STDOUT[] = "K4A_ENABLE_LOG_TO_STDOUT";
static const char K4A_LOG_LEVEL[] = "K4A_LOG_LEVEL";
static const char K4A_LOG_FILE_NAME[] = "k4a.log";
static size_t K4A_LOG_FILE_50MB_MAX_SIZE = (1048576 * 50);

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
} logger_global_context_t;

static void logger_init_once(logger_global_context_t *global);
static void logger_deinit();

// Creates a function called logger_global_context_t_get() which returns the initialized
// singleton global
K4A_DECLARE_GLOBAL(logger_global_context_t, logger_init_once);

// To deal with destroying the singleton we create a static class and use the destructor to tear down the object.
// For now this is separate from K4A_DECLARE_GLOBAL as it is C-based and can be reused in the SDK.
class logger_global_destroy
{
public:
    logger_global_destroy() {}
    ~logger_global_destroy()
    {
        logger_deinit();
    }
};
static logger_global_destroy destroy_loggger_on_binary_unload;

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

    // The user may set the callback,  clear the callback, or set the callback to the
    // same value it was previously. It is a failure to change the callback
    // from an existing registration.
    if (g_context->user_callback == nullptr || message_cb == nullptr || g_context->user_callback == message_cb)
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

static void logger_init_once(logger_global_context_t *global)
{
    // All other members are initialized to zero
    rwlock_init(&global->lock);

    global->env_log_level = K4A_LOG_LEVEL_OFF;
    global->user_log_level = K4A_LOG_LEVEL_OFF;

    const char *enable_file_logging = nullptr;
    const char *enable_stdout_logging = nullptr;
    const char *logging_level = nullptr;
    k4a_result_t result = K4A_RESULT_SUCCEEDED;

    // environment_get_variable will return null or "\0" if the env var is not set - depends on the OS.
    enable_file_logging = environment_get_variable(K4A_ENV_VAR_LOG_TO_A_FILE);
    enable_stdout_logging = environment_get_variable(K4A_ENABLE_LOG_TO_STDOUT);
    logging_level = environment_get_variable(K4A_LOG_LEVEL);

    if (enable_file_logging && enable_file_logging[0] != '\0')
    {
        // enable_file_logging is valid see if its a file path
        const char *log_file = K4A_LOG_FILE_NAME;
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

        if (log_file)
        {
            try
            {
                // Create a file rotating logger with 50mb size max and 3 rotated files
                global->env_logger =
                    spdlog::rotating_logger_mt(K4A_LOGGER, log_file, K4A_LOG_FILE_50MB_MAX_SIZE, LOG_FILE_MAX_FILES);

                spdlog::set_pattern("%v");
                global->env_logger->info("\n\nNew logging session started\n");
                global->env_logger_is_file_based = true;
            }
            catch (...)
            {
                // Probably trying to use a file that is already opened by another instance.
                global->env_logger = nullptr;
                printf("ERROR: Logger unable to open log file \"%s\".\n", log_file);
            }
        }
    }

    // log to stdout if enabled via ENV var AND if file logging is not enabled.
    if (K4A_SUCCEEDED(result) && (global->env_logger == nullptr))
    {
        bool enable_stdout_logger = false;

        // Default with user_callback enabled is no stdout logging unless specifically enabled
        if (enable_stdout_logging && enable_stdout_logging[0] != '0')
        {
            enable_stdout_logger = true;
        }
        else if (global->user_callback == nullptr)
        {
            // Default with user_callback disabled is use stdout logging unless specifically disabled
            if (global->user_callback == nullptr &&
                (enable_stdout_logging == nullptr || enable_stdout_logging[0] == '\0' ||
                 (enable_stdout_logging && enable_stdout_logging[0] != '0')))
            {
                enable_stdout_logger = true;
            }
        }

        if (enable_stdout_logger)
        {
            // Unable to turn on color logging due to bug with CTest https://gitlab.kitware.com/cmake/cmake/issues/17620
            global->env_logger = spdlog::stdout_logger_mt(K4A_LOGGER);
        }
    }

    if (K4A_SUCCEEDED(result) && (global->env_logger))
    {
        global->env_log_level = K4A_LOG_LEVEL_ERROR;

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
                global->env_log_level = K4A_LOG_LEVEL_TRACE;
            }
            else if (logging_level[0] == 'i' || logging_level[0] == 'I')
            {
                // capture a severity of info or higher
                global->env_log_level = K4A_LOG_LEVEL_INFO;
            }
            else if (logging_level[0] == 'w' || logging_level[0] == 'W')
            {
                // capture a severity of warning or higher
                global->env_log_level = K4A_LOG_LEVEL_WARNING;
            }
            else if (logging_level[0] == 'e' || logging_level[0] == 'E')
            {
                // capture a severity of error or higher
                global->env_log_level = K4A_LOG_LEVEL_ERROR;
            }
            else if (logging_level[0] == 'c' || logging_level[0] == 'C')
            {
                // capture a severity of error or higher
                global->env_log_level = K4A_LOG_LEVEL_CRITICAL;
            }
        }

        global->env_logger->flush_on(spdlog::level::warn);
    }
}

void logger_deinit(void)
{
    logger_global_context_t *g_context = logger_global_context_t_get();

    rwlock_acquire_write(&g_context->lock);

    g_context->env_logger = nullptr;

    rwlock_release_write(&g_context->lock);
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
