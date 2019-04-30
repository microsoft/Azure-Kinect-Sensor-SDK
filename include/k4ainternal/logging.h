/** \file logging.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <k4a/k4atypes.h>
#include <k4ainternal/handle.h>

// System dependencies
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FORCEINLINE
#ifdef _MSC_VER
#define FORCEINLINE inline __forceinline
#else
#define FORCEINLINE inline __attribute__((always_inline))
#endif
#endif
/** Handle to the logger device.
 *
 * Handles are created with \ref logger_create and closed
 * with \ref logger_destroy.
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(logger_t);

/** Default logging settings
 */
#define K4A_ENABLE_LOG_TO_A_FILE "K4A_ENABLE_LOG_TO_A_FILE"
#define K4A_ENABLE_LOG_TO_STDOUT "K4A_ENABLE_LOG_TO_STDOUT"
#define K4A_LOG_LEVEL "K4A_LOG_LEVEL"
#define K4A_LOG_FILE_NAME "k4a.log"
#define K4A_LOG_FILE_50MB_MAX_SIZE (1048576 * 50)

/** Logger configuration - allows logger to be used in seperate DLL's and
    provide different ENV vars for processes that need to load both instances.
    For example Azure Kinect SDK and potentially Azure Kinect playback.
 */
typedef struct
{
    const char *env_var_log_to_a_file; // env var name for logging to a file
    const char *env_var_log_to_stdout; // env var name for logging to stdout
    const char *env_var_log_level;     // env var name for setting the logging level
    const char *log_file;              // default log file name
    size_t max_log_size;               // max log size before rolling over to a new file.
} logger_config_t;

/** Initialize logger_config_t to the default settings for Azure Kinect SDK
 */
static inline void logger_config_init_default(logger_config_t *config)
{
    memset(config, 0, sizeof(logger_config_t));
    config->env_var_log_to_a_file = K4A_ENABLE_LOG_TO_A_FILE;
    config->env_var_log_to_stdout = K4A_ENABLE_LOG_TO_STDOUT;
    config->env_var_log_level = K4A_LOG_LEVEL;
    config->log_file = NULL;
    config->max_log_size = K4A_LOG_FILE_50MB_MAX_SIZE;
}

/** Open a handle to the logger device.
 *
 * \param config [IN]
 *    A pointer to the logger configuration
 *
 * \param logger_handle [OUT]
 *    A pointer to write the opened logger device handle to
 *
 * \return K4A_RESULT_SUCCEEDED if the device was opened, otherwise an error code
 *
 * If successful, \ref logger_create will return a logger device handle in the logger
 * parameter.
 *
 * When done with the device, close the handle with \ref logger_destroy
 */
k4a_result_t logger_create(logger_config_t *config, logger_t *logger_handle);

/** Closes the logger module and free's it resources
 */
void logger_destroy(logger_t logger_handle);

/** true if the logger is initialized and going to a file.
 */
bool logger_is_file_based(void);

/** Registers a callback function to deliver messages to.
 *
 * \param message_cb [IN]
 * callback function for delivering message to. Set to NULL to unregister a callback function.
 *
 * \param message_cb_context [IN]
 * The callback functions context.
 *
 * \param min_message_level [IN]
 * The least critical error the user wants to be notified about.
 *
 * \remarks
 * See \ref k4a_set_debug_message_handler for more detailed documentation.
 */
k4a_result_t logger_register_message_callback(k4a_logging_message_cb_t *message_cb,
                                              void *message_cb_context,
                                              k4a_log_level_t min_message_level);

void logger_log(k4a_log_level_t level, const char *file, const int line, const char *format, ...);

FORCEINLINE k4a_result_t
TraceError(k4a_result_t result, const char *szCall, const char *szFile, int line, const char *szFunction)
{
    if (K4A_FAILED(result))
    {
        logger_log(K4A_LOG_LEVEL_ERROR, szFile, line, "%s returned failure in %s()", szCall, szFunction);
    }
    return result;
}
FORCEINLINE k4a_buffer_result_t
TraceBufferError(k4a_buffer_result_t result, const char *szCall, const char *szFile, int line, const char *szFunction)
{
    if (result == K4A_BUFFER_RESULT_FAILED)
    {
        logger_log(K4A_LOG_LEVEL_ERROR, szFile, line, "%s returned failure in %s()", szCall, szFunction);
    }
    return result;
}
FORCEINLINE k4a_wait_result_t
TraceWaitError(k4a_wait_result_t result, const char *szCall, const char *szFile, int line, const char *szFunction)
{
    if (result == K4A_WAIT_RESULT_FAILED)
    {
        logger_log(K4A_LOG_LEVEL_ERROR, szFile, line, "%s returned failure in %s()", szCall, szFunction);
    }
    return result;
}

FORCEINLINE k4a_result_t TraceReturn(k4a_result_t result, const char *szFile, int line, const char *szFunction)
{
    if (K4A_FAILED(result))
    {
        logger_log(K4A_LOG_LEVEL_ERROR, szFile, line, "%s() returned failure.", szFunction);
    }
    else
    {
        logger_log(K4A_LOG_LEVEL_TRACE, szFile, line, "%s() returned success.", szFunction);
    }
    return result;
}

FORCEINLINE void TraceArg(int result, const char *szFile, int line, const char *szFunction, const char *szExpression)
{
    if (!result)
    {
        logger_log(K4A_LOG_LEVEL_ERROR, szFile, line, "Invalid argument to %s(). %s", szFunction, szExpression);
    }
}

FORCEINLINE void TraceInvalidHandle(int result,
                                    const char *szFile,
                                    int line,
                                    const char *szFunction,
                                    const char *szHandleType,
                                    const char *szExpression,
                                    void *pHandleValue)
{
    if (!result)
    {
        logger_log(K4A_LOG_LEVEL_ERROR,
                   szFile,
                   line,
                   "Invalid argument to %s(). %s (%p) is not a valid handle of type %s",
                   szFunction,
                   szExpression,
                   pHandleValue,
                   szHandleType);
    }
}

/*
    Standards for using the tracing Macros:

    Parameter validation:
        All parameters that requires validation should be checked at the start of a function.
        Parameters should be checked in the order they are declared.
        One check should be used per parameter.
        Use RETURN*_IF_HANDLE_INVALID for handles created with handle.h
        Use RETURN*_IF_ARG for other types

    Calling other functions:
        If the callee function returns a k4a_result_t, wrap the call in TRACE_CALL.
            TRACE_CALL will pass through the result, and trace the error information if one occurs
        If the callee function returns another type, K4A_RESULT_FROM_BOOL will convert a boolean success expression to a
            a k4a_result_t, and trace the error information if one occurs

    Checking an error condition expression:
        When checking a condition that indicates a success or failure, wrap the expression in TRACE_BOOL and use the
        k4a_result_t result.

    Checking for failures:
        The preferred pattern is to initialize a result variable as K4A_RESULT_SUCCEEDED, then update that variable with
        each call or operation in the function. If a subsequent operation shouldn't be performed after a failure,
        wrap it in an "if (K4A_SUCCEEDED(result))" block

    Cleaning up from errors:
        Generally, if a function returns K4A_RESULT_FAILED, it should not have any side effects on persistent state.
   This means cleaning up any memory that was allocated before the failure, or undo-ing any operations that were
   partially completed.

        Functions should check for failure with a K4A_FAILED(result) check, and perform all the needed cleanup. If there
   the failing function is a "create" function, you may wish to call the respective "destroy" at this point to avoid
   duplicating cleanup logic.
*/

#define VOID_VALUE

// Trace the output of a call to another function that returns a k4a_result_t
#define TRACE_CALL(_call_) TraceError((_call_), #_call_, __FILE__, __LINE__, __func__)
#define TRACE_BUFFER_CALL(_call_) TraceBufferError((_call_), #_call_, __FILE__, __LINE__, __func__)
#define TRACE_WAIT_CALL(_call_) TraceWaitError((_call_), #_call_, __FILE__, __LINE__, __func__)

// Convert a bool success epxression to a k4a_result_t. Trace the result of the conversion on failures.
#define K4A_RESULT_FROM_BOOL(_call_)                                                                                   \
    TraceError((_call_) ? K4A_RESULT_SUCCEEDED : K4A_RESULT_FAILED, #_call_, __FILE__, __LINE__, __func__)

//
// RETURN_* macros should be used only for parameter validation at the beginning of a function
//

// Returns early from a function if the _expression_ is true, and traces that an invalid argument was used
#define RETURN_VALUE_IF_ARG(_fail_value_, _expression_)                                                                \
    if ((_expression_))                                                                                                \
    {                                                                                                                  \
        TraceArg(0, __FILE__, __LINE__, __func__, #_expression_);                                                      \
        TraceReturn(K4A_RESULT_FAILED, __FILE__, __LINE__, __func__);                                                  \
        return _fail_value_;                                                                                           \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        TraceArg(1, __FILE__, __LINE__, __func__, #_expression_);                                                      \
    }

#define RETURN_VALUE_IF_HANDLE_INVALID(_fail_value_, _type_, _handle_)                                                 \
    if ((NULL == _type_##_get_context(_handle_)))                                                                      \
    {                                                                                                                  \
        TraceInvalidHandle(0, __FILE__, __LINE__, __func__, #_type_, #_handle_, _handle_);                             \
        return _fail_value_;                                                                                           \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        TraceInvalidHandle(1, __FILE__, __LINE__, __func__, #_type_, #_handle_, _handle_);                             \
    }

// Logs a message
#define LOG_TRACE(message, ...)                                                                                        \
    logger_log(K4A_LOG_LEVEL_TRACE, __FILE__, __LINE__, "%s(). " message, __func__, __VA_ARGS__)
#define LOG_INFO(message, ...)                                                                                         \
    logger_log(K4A_LOG_LEVEL_INFO, __FILE__, __LINE__, "%s(). " message, __func__, __VA_ARGS__)
#define LOG_WARNING(message, ...)                                                                                      \
    logger_log(K4A_LOG_LEVEL_WARNING, __FILE__, __LINE__, "%s(). " message, __func__, __VA_ARGS__)
#define LOG_ERROR(message, ...)                                                                                        \
    logger_log(K4A_LOG_LEVEL_ERROR, __FILE__, __LINE__, "%s(). " message, __func__, __VA_ARGS__)
#define LOG_CRITICAL(message, ...)                                                                                     \
    logger_log(K4A_LOG_LEVEL_CRITICAL, __FILE__, __LINE__, "%s(). " message, __func__, __VA_ARGS__)
#define LOG_HANDLE(message, ...)                                                                                       \
    logger_log(K4A_LOG_LEVEL_TRACE, __FILE__, __LINE__, "%s(). " message, __func__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_H */
