// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// This library
#include <k4ainternal/logging.h>

#include <azure_c_shared_utility/envvariable.h>
#include <azure_c_shared_utility/refcount.h>

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

// Logger used for capturing errors from this module
static std::shared_ptr<spdlog::logger> g_logger = NULL;
static volatile long g_logger_count = 0;
static bool g_logger_is_file_based = false;

typedef struct logger_context_t
{
    std::shared_ptr<spdlog::logger> logger;
} logger_context_t;

K4A_DECLARE_CONTEXT(logger_t, logger_context_t);

#define K4A_LOGGER "k4a_logger"

// NOTE, if a sub directory for the log is used, then it needs to be created prior to attempting to create the file
#define LOG_FILE_MAX_FILES (3)
#define LOG_FILE_EXTENSION ".log"

k4a_result_t logger_create(logger_config_t *config, logger_t *logger_handle)
{
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, logger_handle == NULL);
    RETURN_VALUE_IF_ARG(K4A_RESULT_FAILED, config->max_log_size == 0);
    logger_context_t *context = logger_t_create(logger_handle);
    const char *enable_file_logging = NULL;
    const char *enable_stdout_logging = NULL;
    const char *logging_level = NULL;

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
    if ((INC_REF_VAR(g_logger_count) > 1) || (g_logger))
    {
        // Reuse the configured logger settings in the event we have more than 1 active handle at a time
        return K4A_RESULT_SUCCEEDED;
    }

    if (enable_file_logging && enable_file_logging[0] != '\0')
    {
        const char *log_file = NULL;
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
            // Create a file rotating logger with 5mb size max and 3 rotated files
            g_logger = spdlog::rotating_logger_mt(K4A_LOGGER, log_file, config->max_log_size, LOG_FILE_MAX_FILES);

            spdlog::set_pattern("%v");
            g_logger->info("\n\nNew logging session started\n");
            g_logger_is_file_based = true;
        }
    }

    // log to STD out if file logger not created and STDOUT logging is not disabled.
    if (g_logger == NULL)
    {
        if (enable_stdout_logging == NULL || enable_stdout_logging[0] == '\0' ||
            (enable_stdout_logging && enable_stdout_logging[0] != '0'))
        {
            // Unable to turn on color logging due to bug with CTest
            // https://gitlab.kitware.com/cmake/cmake/issues/17620
            g_logger = spdlog::stdout_logger_mt(K4A_LOGGER);
        }
    }
    context->logger = g_logger;

    if (g_logger)
    {

        //[2018-08-27 10:44:23.218] [level] [threadID] <message>
        // https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [t=%t] %v");

        // Set the default logging level
        spdlog::set_level(spdlog::level::err);

        // override the default logging level
        if (logging_level && logging_level[0] != '\0')
        {
            if (logging_level[0] == 't' || logging_level[0] == 'T')
            {
                // capture a severity of trace or higher
                spdlog::set_level(spdlog::level::trace);
            }
            else if (logging_level[0] == 'i' || logging_level[0] == 'I')
            {
                // capture a severity of info or higher
                spdlog::set_level(spdlog::level::info);
            }
            else if (logging_level[0] == 'w' || logging_level[0] == 'W')
            {
                // capture a severity of warning or higher
                spdlog::set_level(spdlog::level::warn);
            }
            else if (logging_level[0] == 'e' || logging_level[0] == 'E')
            {
                // capture a severity of error or higher
                spdlog::set_level(spdlog::level::err);
            }
            else if (logging_level[0] == 'c' || logging_level[0] == 'C')
            {
                // capture a severity of error or higher
                spdlog::set_level(spdlog::level::critical);
            }
        }

        g_logger->flush_on(spdlog::level::warn);
    }
    return K4A_RESULT_SUCCEEDED;
}

void logger_destroy(logger_t logger_handle)
{
    logger_context_t *context = logger_t_get_context(logger_handle);

    // destroy the logger
    if (DEC_REF_VAR(g_logger_count) == 0)
    {
        bool drop_logger = g_logger != NULL;
        g_logger = NULL;
        if (drop_logger)
        {
            spdlog::drop(K4A_LOGGER);
        }
        g_logger_is_file_based = false;
    }
    context->logger = NULL;

    logger_t_destroy(logger_handle);
}

#if defined(__GNUC__) || defined(__clang__)
// Enable printf type checking in clang and gcc
__attribute__((__format__ (__printf__, 2, 0)))
#endif
void logger_trace(
    const char * zone,
    const char * file,
    const int line,
    const char * format,
    ...)
{
    char buffer[1024];

    if (g_logger == NULL)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);

    g_logger->trace("[{0}] {1} ({2}): {3}", zone, file, line, buffer);
    va_end(args);
}

#if defined(__GNUC__) || defined(__clang__)
// Enable printf type checking in clang and gcc
__attribute__((__format__ (__printf__, 2, 0)))
#endif
void logger_info(
    const char * zone,
    const char * file,
    const int line,
    const char * format,
    ...)
{
    char buffer[1024];

    if (g_logger == NULL)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);

    g_logger->info("[{0}] {1} ({2}): {3}", zone, file, line, buffer);
    va_end(args);
}

#if defined(__GNUC__) || defined(__clang__)
// Enable printf type checking in clang and gcc
__attribute__((__format__ (__printf__, 2, 0)))
#endif
void logger_warn(
    const char * zone,
    const char * file,
    const int line,
    const char * format,
    ...)
{
    char buffer[1024];

    if (g_logger == NULL)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);

    g_logger->warn("[{0}] {1} ({2}): {3}", zone, file, line, buffer);
    va_end(args);
}

#if defined(__GNUC__) || defined(__clang__)
// Enable printf type checking in clang and gcc
__attribute__((__format__ (__printf__, 2, 0)))
#endif
void logger_error(
    const char * zone,
    const char * file,
    const int line,
    const char * format,
    ...)
{
    char buffer[1024];

    if (g_logger == NULL)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);

    g_logger->error("[{0}] {1} ({2}): {3}", zone, file, line, buffer);
    va_end(args);
}

#if defined(__GNUC__) || defined(__clang__)
// Enable printf type checking in clang and gcc
__attribute__((__format__ (__printf__, 2, 0)))
#endif
void logger_critical(
    const char * zone,
    const char * file,
    const int line,
    const char * format,
    ...)
{
    char buffer[1024];

    if (g_logger == NULL)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);

    g_logger->critical("[{0}] {1} ({2}): {3}", zone, file, line, buffer);
    va_end(args);
}

bool logger_is_file_based()
{
    return g_logger_is_file_based;
}

#ifdef __cplusplus
}
#endif
