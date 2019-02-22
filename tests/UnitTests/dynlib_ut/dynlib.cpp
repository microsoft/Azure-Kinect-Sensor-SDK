// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>

#include <k4ainternal/dynlib.h>

int main(int argc, char **argv)
{
    return k4a_test_commmon_main(argc, argv);
}

TEST(dynlib_ut, loadk4a)
{
    dynlib_t dynlib_handle = NULL;

    ASSERT_EQ(K4A_RESULT_SUCCEEDED,
              dynlib_create(TEST_LIBRARY_NAME, TEST_LIBRARY_MAJOR_VER, TEST_LIBRARY_MINOR_VER, &dynlib_handle));

    void *address = NULL;
    ASSERT_EQ(K4A_BUFFER_RESULT_SUCCEEDED, dynlib_find_symbol(dynlib_handle, "say_hello", &address));
    ASSERT_NE(nullptr, address);

    dynlib_destroy(dynlib_handle);
}
