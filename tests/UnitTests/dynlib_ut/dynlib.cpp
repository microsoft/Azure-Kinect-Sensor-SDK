#include <utcommon.h>

#include <k4ainternal/dynlib.h>

int main(int argc, char **argv)
{
    return k4a_test_commmon_main(argc, argv);
}

TEST(dynlib_ut, loadk4a)
{
    dynlib_t dynlib_handle = NULL;

    ASSERT_EQ(K4A_RESULT_SUCCEEDED, dynlib_create("k4a", &dynlib_handle));

    void *address = NULL;
    ASSERT_EQ(K4A_BUFFER_RESULT_SUCCEEDED,
              dynlib_find_symbol(dynlib_handle, "k4a_device_get_installed_count", &address));
    ASSERT_NE(nullptr, address);

    dynlib_destroy(dynlib_handle);
}
