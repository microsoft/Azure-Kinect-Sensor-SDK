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

/** Allocates memory from the allocator
 *
 * \param source
 * the source of code allocating the memory
 *
 * \param alloc_size
 * size of the memory to allocate
 *
 * \param context [OUT]
 * context to go with the allocation be used when freed
 *
 * This call only cleans up the allocator handle.
 * This function should not be called until all outstanding \ref k4a_capture_t objects are freed.
 *
 * This function should return 0 to indicate the number of outstanding allocations. Consider this the
 * number of leaked allocations.
 */
uint8_t *allocator_alloc(allocation_source_t source, size_t alloc_size, void **context);
void allocator_free(void *buffer, void *context);

long allocator_test_for_leaks(void);

#ifdef __cplusplus
}
#endif

#endif /* ALLOCATOR_H */
