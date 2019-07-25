/** \file allocator.h
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK.
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <k4a/k4atypes.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Allocation Source.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4atypes.h (include k4a/k4a.h)</requirement>
 * </requirements>
 * \endxmlonly
 */
typedef enum
{
    ALLOCATION_SOURCE_USER = 0,  /**< Memory was allocated by the user */
    ALLOCATION_SOURCE_DEPTH,     /**< Memory was allocated by the depth reader */
    ALLOCATION_SOURCE_COLOR,     /**< Memory was allocated by the Color reader */
    ALLOCATION_SOURCE_IMU,       /**< Memory was allocated by the IMU reader */
    ALLOCATION_SOURCE_USB_DEPTH, /**< Memory was allocated by the USB reader */
    ALLOCATION_SOURCE_USB_IMU,   /**< Memory was allocated by the USB reader */
} allocation_source_t;

/** Initializes the globals used by the allocator
 *
 */
void allocator_initialize(void);

/** Deinitializes the globals used by the allocator
 *
 */
void allocator_deinitialize(void);

/** Sets the callback functions fot the SDK allocator
 *
 * \param allocate
 * The callback function to allocate memory. When the SDK requires memory allocation this callback will be
 * called and the application can provide a buffer and a context.
 *
 * \param free
 * The callback function to free memory. The SDK will call this function when memory allocated b by \p allocate
 * is no longer needed.
 *
 * \return ::K4A_RESULT_SUCCEEDED if the callback function was set or cleared successfully. ::K4A_RESULT_FAILED if an
 * error is encountered or the callback function has already been set.
 *
 * \remarks
 * Call this function to hook memory allocation by the SDK. Calling with both \p allocate and \p free as NULL will
 * clear the hook and reset to the default allocator.
 *
 * \remarks
 * If this function is called after memory has been allocated, the previous version of \p free function may still be
 * called in the future. The SDK will always call the \p free function that was set at the time that the memory
 * was allocated.
 *
 * \xmlonly
 * <requirements>
 *   <requirement name="Header">k4a.h (include k4a/k4a.h)</requirement>
 *   <requirement name="Library">k4a.lib</requirement>
 *   <requirement name="DLL">k4a.dll</requirement>
 * </requirements>
 * \endxmlonly
 */
k4a_result_t allocator_set_allocator(k4a_memory_allocate_cb_t allocate, k4a_memory_destroy_cb_t free);

/** Allocates memory from the allocator
 *
 * \param source
 * the source of code allocating the memory
 *
 * \param alloc_size
 * size of the memory to allocate
 *
 * This call only cleans up the allocator handle.
 * This function should not be called until all outstanding \ref k4a_capture_t objects are freed.
 *
 * This function should return 0 to indicate the number of outstanding allocations. Consider this the
 * number of leaked allocations.
 */
uint8_t *allocator_alloc(allocation_source_t source, size_t alloc_size);
void allocator_free(void *buffer);
long allocator_test_for_leaks(void);

#ifdef __cplusplus
}
#endif

#endif /* ALLOCATOR_H */
