// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <k4ainternal/deloader.h>

#include <k4a/k4atypes.h>
#include <k4ainternal/global.h>
#include <k4ainternal/logging.h>
#include <k4ainternal/dynlib.h>

typedef struct
{
    k4a_plugin_t plugin;
    dynlib_t handle;
    k4a_register_plugin_fn registerFn;
    volatile bool loaded;
} deloader_global_context_t;

static void deloader_init_once(deloader_global_context_t *global);
static void deloader_deinit(void);

// Creates a function called deloader_global_context_t_get() which returns the initialized
// singleton global
K4A_DECLARE_GLOBAL(deloader_global_context_t, deloader_init_once);

// To deal with destroying the singleton we create a static class and use the destructor to tear down the object.
// For now this is separate from K4A_DECLARE_GLOBAL as it is C-based and can be reused in the SDK.
class deloader_global_destroy
{
public:
    deloader_global_destroy() {}
    ~deloader_global_destroy()
    {
        deloader_deinit();
    }
};
static deloader_global_destroy destroy_deloader_on_binary_unload;

static bool is_plugin_loaded(deloader_global_context_t *global)
{
    return global->loaded;
}

static bool verify_plugin(const k4a_plugin_t *plugin)
{
    RETURN_VALUE_IF_ARG(false, plugin == NULL);

    LOG_INFO("Loaded Depth Engine version: %u.%u.%u",
             plugin->version.major,
             plugin->version.minor,
             plugin->version.patch);

    // All function pointers must be non NULL
    RETURN_VALUE_IF_ARG(false, plugin->depth_engine_create_and_initialize == NULL);
    RETURN_VALUE_IF_ARG(false, plugin->depth_engine_process_frame == NULL);
    RETURN_VALUE_IF_ARG(false, plugin->depth_engine_get_output_frame_size == NULL);
    RETURN_VALUE_IF_ARG(false, plugin->depth_engine_destroy == NULL);
    RETURN_VALUE_IF_ARG(false, plugin->transform_engine_create_and_initialize == NULL);
    RETURN_VALUE_IF_ARG(false, plugin->transform_engine_process_frame == NULL);
    RETURN_VALUE_IF_ARG(false, plugin->transform_engine_get_output_frame_size == NULL);
    RETURN_VALUE_IF_ARG(false, plugin->transform_engine_destroy == NULL);

    return true;
}

// Load Depth Engine
static void deloader_init_once(deloader_global_context_t *global)
{
    // All members are initialized to zero

    k4a_result_t result = dynlib_create(K4A_PLUGIN_DYNAMIC_LIBRARY_NAME, K4A_PLUGIN_VERSION, &global->handle);
    if (K4A_FAILED(result))
    {
        LOG_ERROR("Failed to Load Depth Engine Plugin (%s). Depth functionality will not work",
                  K4A_PLUGIN_DYNAMIC_LIBRARY_NAME);
        LOG_ERROR("Make sure the depth engine plugin is in your loaders path", 0);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = dynlib_find_symbol(global->handle, K4A_PLUGIN_EXPORTED_FUNCTION, (void **)&global->registerFn);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(global->registerFn(&global->plugin));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(verify_plugin(&global->plugin));
    }

    if (K4A_SUCCEEDED(result))
    {
        global->loaded = true;
    }
}

k4a_depth_engine_result_code_t deloader_depth_engine_create_and_initialize(k4a_depth_engine_context_t **context,
                                                                           size_t cal_block_size_in_bytes,
                                                                           void *cal_block,
                                                                           k4a_depth_engine_mode_t mode,
                                                                           k4a_depth_engine_input_type_t input_format,
                                                                           void *camera_calibration,
                                                                           k4a_processing_complete_cb_t *callback,
                                                                           void *callback_context)
{
    deloader_global_context_t *global = deloader_global_context_t_get();

    if (!is_plugin_loaded(global))
    {
        LOG_ERROR("Failed to load depth engine plugin", 0);
        return K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_ENGINE_NOT_LOADED;
    }

    return global->plugin.depth_engine_create_and_initialize(context,
                                                             cal_block_size_in_bytes,
                                                             cal_block,
                                                             mode,
                                                             input_format,
                                                             camera_calibration,
                                                             callback,
                                                             callback_context);
}

k4a_depth_engine_result_code_t
deloader_depth_engine_process_frame(k4a_depth_engine_context_t *context,
                                    void *input_frame,
                                    size_t input_frame_size,
                                    k4a_depth_engine_output_type_t output_type,
                                    void *output_frame,
                                    size_t output_frame_size,
                                    k4a_depth_engine_output_frame_info_t *output_frame_info,
                                    k4a_depth_engine_input_frame_info_t *input_frame_info)
{
    deloader_global_context_t *global = deloader_global_context_t_get();

    if (!is_plugin_loaded(global))
    {
        return K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_ENGINE_NOT_LOADED;
    }

    return global->plugin.depth_engine_process_frame(context,
                                                     input_frame,
                                                     input_frame_size,
                                                     output_type,
                                                     output_frame,
                                                     output_frame_size,
                                                     output_frame_info,
                                                     input_frame_info);
}

size_t deloader_depth_engine_get_output_frame_size(k4a_depth_engine_context_t *context)
{
    deloader_global_context_t *global = deloader_global_context_t_get();

    if (!is_plugin_loaded(global))
    {
        return 0;
    }

    return global->plugin.depth_engine_get_output_frame_size(context);
}

void deloader_depth_engine_destroy(k4a_depth_engine_context_t **context)
{
    deloader_global_context_t *global = deloader_global_context_t_get();
    if (!is_plugin_loaded(global))
    {
        return;
    }

    global->plugin.depth_engine_destroy(context);
}

k4a_depth_engine_result_code_t deloader_transform_engine_create_and_initialize(k4a_transform_engine_context_t **context,
                                                                               void *camera_calibration,
                                                                               k4a_processing_complete_cb_t *callback,
                                                                               void *callback_context)
{
    deloader_global_context_t *global = deloader_global_context_t_get();

    if (!is_plugin_loaded(global))
    {
        LOG_ERROR("Failed to load depth engine plugin", 0);
        return K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_ENGINE_NOT_LOADED;
    }

    return global->plugin.transform_engine_create_and_initialize(context,
                                                                 camera_calibration,
                                                                 callback,
                                                                 callback_context);
}

k4a_depth_engine_result_code_t
deloader_transform_engine_process_frame(k4a_transform_engine_context_t *context,
                                        k4a_transform_engine_type_t type,
                                        const void *depth_frame,
                                        size_t depth_frame_size,
                                        const void *frame2,
                                        size_t frame2_size,
                                        void *output_frame,
                                        size_t output_frame_size,
                                        void *output_frame2,
                                        size_t output_frame2_size,
                                        k4a_transform_engine_interpolation_t interpolation,
                                        uint32_t invalid_value)
{
    deloader_global_context_t *global = deloader_global_context_t_get();

    if (!is_plugin_loaded(global))
    {
        return K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_ENGINE_NOT_LOADED;
    }

    return global->plugin.transform_engine_process_frame(context,
                                                         type,
                                                         interpolation,
                                                         invalid_value,
                                                         depth_frame,
                                                         depth_frame_size,
                                                         frame2,
                                                         frame2_size,
                                                         output_frame,
                                                         output_frame_size,
                                                         output_frame2,
                                                         output_frame2_size);
}

size_t deloader_transform_engine_get_output_frame_size(k4a_transform_engine_context_t *context,
                                                       k4a_transform_engine_type_t type)
{
    deloader_global_context_t *global = deloader_global_context_t_get();

    if (!is_plugin_loaded(global))
    {
        return 0;
    }

    return global->plugin.transform_engine_get_output_frame_size(context, type);
}

void deloader_transform_engine_destroy(k4a_transform_engine_context_t **context)
{
    deloader_global_context_t *global = deloader_global_context_t_get();

    if (!is_plugin_loaded(global))
    {
        return;
    }

    global->plugin.transform_engine_destroy(context);
}

void deloader_deinit(void)
{
    deloader_global_context_t *global = deloader_global_context_t_get();

    if (global->handle)
    {
        dynlib_destroy(global->handle);
    }
}
