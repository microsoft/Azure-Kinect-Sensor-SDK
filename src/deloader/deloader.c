#include <k4ainternal/deloader.h>

#include <k4a/k4atypes.h>
#include <k4a/k4aplugin.h>
#include <k4ainternal/logging.h>
#include <k4ainternal/dynlib.h>

typedef struct
{
    k4a_plugin_t plugin;
    dynlib_t handle;
    k4a_register_plugin_fn registerFn;
    volatile bool loaded;
} destub_context_t;

static destub_context_t g_destub = { .registerFn = NULL, .loaded = false };

static bool is_plugin_loaded()
{
    return g_destub.loaded;
}

static bool verify_plugin(const k4a_plugin_t *plugin)
{
    RETURN_VALUE_IF_ARG(false, plugin == NULL);

    // Major versions must match
    if (plugin->version.major != K4A_PLUGIN_MAJOR_VERSION)
    {
        LOG_ERROR("Plugin Major Version Mismatch. Expected %u. Found %u",
                  K4A_PLUGIN_MAJOR_VERSION,
                  plugin->version.major);
        return false;
    }

    // All function pointers must be non NULL
    RETURN_VALUE_IF_ARG(false, plugin->depth_engine_create_and_initialize == NULL);
    RETURN_VALUE_IF_ARG(false, plugin->depth_engine_process_frame == NULL);
    RETURN_VALUE_IF_ARG(false, plugin->depth_engine_get_output_frame_size == NULL);
    RETURN_VALUE_IF_ARG(false, plugin->depth_engine_destroy == NULL);

    return true;
}

static k4a_result_t load_depth_engine()
{
    if (g_destub.loaded)
    {
        return K4A_RESULT_SUCCEEDED;
    }

    k4a_result_t result = dynlib_create(K4A_PLUGIN_DYNAMIC_LIBRARY_NAME, &g_destub.handle);
    if (K4A_FAILED(result))
    {
        LOG_ERROR("Failed to Load Depth Engine Plugin (%s). Depth functionality will not work",
                  K4A_PLUGIN_DYNAMIC_LIBRARY_NAME);
        LOG_ERROR("Make sure the depth engine plugin is in your loaders path", 0);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = dynlib_find_symbol(g_destub.handle, K4A_PLUGIN_EXPORTED_FUNCTION, (void **)&g_destub.registerFn);
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(g_destub.registerFn(&g_destub.plugin));
    }

    if (K4A_SUCCEEDED(result))
    {
        result = K4A_RESULT_FROM_BOOL(verify_plugin(&g_destub.plugin));
    }

    if (K4A_SUCCEEDED(result))
    {
        g_destub.loaded = true;
    }

    return result;
}

k4a_depth_engine_result_code_t destub_depth_engine_create_and_initialize(k4a_depth_engine_context_t **context,
                                                                         size_t cal_block_size_in_bytes,
                                                                         void *cal_block,
                                                                         k4a_depth_engine_mode_t mode,
                                                                         k4a_depth_engine_input_type_t input_format,
                                                                         void *camera_calibration,
                                                                         k4a_processing_complete_cb_t *callback,
                                                                         void *callback_context)
{
    k4a_result_t result = load_depth_engine();

    if (K4A_FAILED(result))
    {
        LOG_ERROR("Failed to load depth engine plugin", 0);
        return K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_ENGINE_NOT_LOADED;
    }

    return g_destub.plugin.depth_engine_create_and_initialize(context,
                                                              cal_block_size_in_bytes,
                                                              cal_block,
                                                              mode,
                                                              input_format,
                                                              camera_calibration,
                                                              callback,
                                                              callback_context);
}

k4a_depth_engine_result_code_t
destub_depth_engine_process_frame(k4a_depth_engine_context_t *context,
                                  void *input_frame,
                                  size_t input_frame_size,
                                  k4a_depth_engine_output_type_t output_type,
                                  void *output_frame,
                                  size_t output_frame_size,
                                  k4a_depth_engine_output_frame_info_t *output_frame_info,
                                  k4a_depth_engine_input_frame_info_t *input_frame_info)
{
    if (!is_plugin_loaded())
    {
        return K4A_DEPTH_ENGINE_RESULT_FATAL_ERROR_ENGINE_NOT_LOADED;
    }

    return g_destub.plugin.depth_engine_process_frame(context,
                                                      input_frame,
                                                      input_frame_size,
                                                      output_type,
                                                      output_frame,
                                                      output_frame_size,
                                                      output_frame_info,
                                                      input_frame_info);
}

size_t destub_depth_engine_get_output_frame_size(k4a_depth_engine_context_t *context)
{
    if (!is_plugin_loaded())
    {
        return 0;
    }

    return g_destub.plugin.depth_engine_get_output_frame_size(context);
}

void destub_depth_engine_destroy(k4a_depth_engine_context_t **context)
{
    if (!is_plugin_loaded())
    {
        return;
    }

    g_destub.plugin.depth_engine_destroy(context);
}