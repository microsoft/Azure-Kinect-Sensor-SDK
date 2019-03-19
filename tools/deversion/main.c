// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>

#include <k4ainternal/k4aplugin.h>
#include <k4ainternal/dynlib.h>

static void print_usage()
{
    printf("deversion\n");
    printf("\tPrints out the version of the depth engine in the path");
}

int main(const int argc, const char *argv[])
{
    (void)argv;

    if (argc != 1)
    {
        print_usage();
        return 0;
    }

    uint32_t num_found = 0;

    for (uint32_t major_ver = 0; major_ver <= K4A_PLUGIN_MAJOR_VERSION; major_ver++)
    {
        for (uint32_t minor_ver = 0; minor_ver <= K4A_PLUGIN_MINOR_VERSION; minor_ver++)
        {
            k4a_plugin_t plugin;
            memset(&plugin, 0, sizeof(plugin));
            dynlib_t handle;
            memset(&handle, 0, sizeof(handle));

            k4a_register_plugin_fn registerFn = NULL;

            k4a_result_t result = dynlib_create(K4A_PLUGIN_DYNAMIC_LIBRARY_NAME, major_ver, minor_ver, &handle);

            if (K4A_SUCCEEDED(result))
            {
                result = dynlib_find_symbol(handle, K4A_PLUGIN_EXPORTED_FUNCTION, (void **)&registerFn);
            }
            else
            {
                continue;
            }

            if (K4A_SUCCEEDED(result))
            {
                bool succeeded = registerFn(&plugin);
                result = succeeded ? K4A_RESULT_SUCCEEDED : K4A_RESULT_FAILED;
            }

            if (K4A_SUCCEEDED(result))
            {
                bool current = (major_ver == K4A_PLUGIN_MAJOR_VERSION) && (minor_ver == K4A_PLUGIN_MINOR_VERSION);
                printf("Found Depth Engine Pluging Version: %u.%u.%u%s\n",
                       plugin.version.major,
                       plugin.version.minor,
                       plugin.version.patch,
                       (current) ? " (current)" : "");

                num_found++;
            }

            dynlib_destroy(handle);
        }
    }

    if (num_found == 0)
    {
        printf("No depth engine plugins found\n");
    }

    return 0;
}