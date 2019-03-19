/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef K4ADYNLIB_H
#define K4ADYNLIB_H

#include <k4a/k4atypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Handle to the dynamic library.
 *
 * Handles are created with \ref dynlib_create and are destroyed with
 * \ref dynlib_destroy
 *
 * Invalid handles are set to 0.
 */
K4A_DECLARE_HANDLE(dynlib_t);

/**
 * The maximum major version we support for loading a dynamic library
 */
#define DYNLIB_MAX_MAJOR_VERSION 99ul

/**
 * The maximum minor version we support for loading a dynamic library
 */
#define DYNLIB_MAX_MINOR_VERSION 99ul

/** Loads a versioned dynamic library (shared library) by name and version.
 *  The version information is encoded in the filename.
 *
 * \param name [IN]
 * Name of the dynamic library or shared library
 *
 * \param major_ver
 * Expected major version of the dynamic library. This number must be less than
 * or equal to \ref DYNLIB_MAX_MAJOR_VERSION.
 *
 * \param minor_ver
 * Expected minor version of the dynamic library. This number must be less than
 * or equal to \ref DYNLIB_MIN_MAJOR_VERSION.
 *
 * \param dynlib_handle [OUT]
 * A handle to store dynlib in. Only valid if function returns
 * K4A_RESULT_SUCCEEDED
 *
 * \remarks
 * The version information should be encoded in the filename of the dynamic
 * library. This encoding will be different for Windows versus Linux. For
 * Windows the dynamic library name is "<name>_<major_ver>_<minor_ver>.dll".
 * For Linux, the dynamic library name is
 * "lib<name>.so.<major_ver>.<minor_ver>".
 *
 * \remarks
 * This function only supports relative path libraries. If you try to load an
 * explicit path this will fail.
 *
 * For example: dynlib_create("depthengine", 1, 0, &handle) will search for
 * depthengine_1_0.dll on Windows and libdepthengine.so.1.0 on Linux
 *
 * \return K4A_RESULT_SUCCEEDED if the library was loaded, K4A_RESULT_FAILED
 * otherwise
 */
k4a_result_t dynlib_create(const char *name, uint32_t major_ver, uint32_t minor_ver, dynlib_t *dynlib_handle);

/** Finds the address of an exported symbol in a loaded dynamic library
 *
 * \param dynlib_handle [IN]
 * Handle to the loaded dynamic library
 *
 * \param symbol [IN]
 * Name of the exported symbol to find
 *
 * \param address [OUT]
 * void* pointer passed by reference which is either set to the address of the
 * symbol or NULL if the symbol was not found. Only valid if function
 * returns K4A_RESULT_SUCCEEDED.
 *
 * \return K4A_RESULT_SUCCEEDED if the symbol was successfully searched for in
 * the loaded dynamic library. Returns K4A_RESULT_FAILED on any failure.
 */
k4a_result_t dynlib_find_symbol(dynlib_t dynlib_handle, const char *symbol, void **address);

/** Unload the dynamic library.
 *
 * \param dynlib_handle [IN]
 * Dynamic library to unload. This handle is no longer valid after this function
 * call
 */
void dynlib_destroy(dynlib_t dynlib_handle);

#ifdef __cplusplus
}
#endif

#endif
