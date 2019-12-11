// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>
#include <iostream>

// Module being tested
#include <k4ainternal/matroska_write.h>
#include <k4arecord/record.h>
#include <k4a/k4a.h>

#include <ebml/MemIOCallback.h>
#include <matroska/KaxSegment.h>

using namespace testing;
using namespace k4arecord;

class record_ut : public ::testing::Test
{
protected:
    k4a_record_context_t *context;
    k4a_record_t recording_handle;

    void SetUp() override
    {
        context = k4a_record_t_create(&recording_handle);
        context->ebml_file = make_unique<libebml::MemIOCallback>();
        context->timecode_scale = MATROSKA_TIMESCALE_NS;
        context->file_segment = make_unique<libmatroska::KaxSegment>();
    }

    void TearDown() override
    {
        for (cluster_t *cluster : context->pending_clusters)
        {
            delete cluster;
        }
        context->pending_clusters.clear();

        k4a_record_t_destroy(recording_handle);
    }
};

static_assert(MAX_CLUSTER_LENGTH_NS > 10, "Tests need to run with clusters > 10ns");

TEST_F(record_ut, new_clusters_in_order)
{
    ASSERT_EQ(context->pending_clusters.size(), 0u);
    ASSERT_EQ(context->last_written_timestamp, 0);

    // Create 3 clusters in order
    cluster_t *cluster1 = get_cluster_for_timestamp(context, 0);
    ASSERT_NE(cluster1, nullptr);
    ASSERT_EQ(cluster1->time_start_ns, 0);
    ASSERT_EQ(cluster1->time_end_ns, MAX_CLUSTER_LENGTH_NS);

    cluster_t *cluster2 = get_cluster_for_timestamp(context, MAX_CLUSTER_LENGTH_NS);
    ASSERT_NE(cluster2, nullptr);
    ASSERT_EQ(cluster2->time_start_ns, MAX_CLUSTER_LENGTH_NS);
    ASSERT_EQ(cluster2->time_end_ns, MAX_CLUSTER_LENGTH_NS * 2);
    ASSERT_NE(cluster2, cluster1);

    ASSERT_EQ(context->pending_clusters.size(), 2u);

    // Try looking up each cluster by timestamp
    for (uint64 timestamp = 0; timestamp < 10; timestamp++)
    {
        cluster_t *cluster = get_cluster_for_timestamp(context, timestamp);
        ASSERT_NE(cluster, nullptr);
        ASSERT_EQ(cluster->time_start_ns, 0);
        ASSERT_EQ(cluster->time_end_ns, MAX_CLUSTER_LENGTH_NS);
        ASSERT_EQ(cluster, cluster1);

        cluster = get_cluster_for_timestamp(context, MAX_CLUSTER_LENGTH_NS + timestamp);
        ASSERT_NE(cluster, nullptr);
        ASSERT_EQ(cluster->time_start_ns, MAX_CLUSTER_LENGTH_NS);
        ASSERT_EQ(cluster->time_end_ns, MAX_CLUSTER_LENGTH_NS * 2);
        ASSERT_EQ(cluster, cluster2);
    }

    ASSERT_EQ(context->pending_clusters.size(), 2u);
}

TEST_F(record_ut, new_cluster_out_of_order)
{
    ASSERT_EQ(context->pending_clusters.size(), 0u);
    ASSERT_EQ(context->last_written_timestamp, 0);

    // Create 3 clusters out of order
    cluster_t *cluster3 = get_cluster_for_timestamp(context, MAX_CLUSTER_LENGTH_NS * 2);
    ASSERT_NE(cluster3, nullptr);
    ASSERT_EQ(cluster3->time_start_ns, MAX_CLUSTER_LENGTH_NS * 2);
    ASSERT_EQ(cluster3->time_end_ns, MAX_CLUSTER_LENGTH_NS * 3);

    cluster_t *cluster1 = get_cluster_for_timestamp(context, MAX_CLUSTER_LENGTH_NS - 10);
    ASSERT_NE(cluster1, nullptr);
    ASSERT_EQ(cluster1->time_start_ns, 0);
    ASSERT_EQ(cluster1->time_end_ns, MAX_CLUSTER_LENGTH_NS);
    ASSERT_NE(cluster1, cluster3);

    cluster_t *cluster2 = get_cluster_for_timestamp(context, MAX_CLUSTER_LENGTH_NS + 10);
    ASSERT_NE(cluster2, nullptr);
    ASSERT_EQ(cluster2->time_start_ns, MAX_CLUSTER_LENGTH_NS);
    ASSERT_EQ(cluster2->time_end_ns, MAX_CLUSTER_LENGTH_NS * 2);
    ASSERT_NE(cluster2, cluster1);
    ASSERT_NE(cluster2, cluster3);

    ASSERT_EQ(context->pending_clusters.size(), 3u);

    // Try looking up each cluster by timestamp
    for (uint64 timestamp = 0; timestamp < 10; timestamp++)
    {
        cluster_t *cluster = get_cluster_for_timestamp(context, timestamp);
        ASSERT_NE(cluster, nullptr);
        ASSERT_EQ(cluster->time_start_ns, 0);
        ASSERT_EQ(cluster->time_end_ns, MAX_CLUSTER_LENGTH_NS);
        ASSERT_EQ(cluster, cluster1);

        cluster = get_cluster_for_timestamp(context, MAX_CLUSTER_LENGTH_NS + timestamp);
        ASSERT_NE(cluster, nullptr);
        ASSERT_EQ(cluster->time_start_ns, MAX_CLUSTER_LENGTH_NS);
        ASSERT_EQ(cluster->time_end_ns, MAX_CLUSTER_LENGTH_NS * 2);
        ASSERT_EQ(cluster, cluster2);

        cluster = get_cluster_for_timestamp(context, MAX_CLUSTER_LENGTH_NS * 2 + timestamp);
        ASSERT_NE(cluster, nullptr);
        ASSERT_EQ(cluster->time_start_ns, MAX_CLUSTER_LENGTH_NS * 2);
        ASSERT_EQ(cluster->time_end_ns, MAX_CLUSTER_LENGTH_NS * 3);
        ASSERT_EQ(cluster, cluster3);
    }

    ASSERT_EQ(context->pending_clusters.size(), 3u);
}

// This test's goal is to fill up the write queue by saturating disk write.
// It should trigger the write speed warning message in the logs.
// Since this test is unlikely to complete, and needs to be manually run, it is disabled.
TEST_F(record_ut, DISABLED_bgra_color_max_disk_write)
{
    k4a_device_configuration_t record_config = {};
    record_config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    record_config.color_resolution = K4A_COLOR_RESOLUTION_2160P;
    record_config.depth_mode = K4A_DEPTH_MODE_OFF;
    record_config.camera_fps = K4A_FRAMES_PER_SECOND_30;

    std::cout
        << "A 'Disk write speed is too low, write queue is filling up.' log message is expected after about 4 seconds."
        << std::endl;
    std::cout
        << "If the test completes without this log message, the check may be broken, or the test disk may be too fast."
        << std::endl;
    std::cout
        << "If the test crashes due to an out-of-memory condition without logging a disk warning, the check is broken."
        << std::endl;

    k4a_record_t handle = NULL;
    k4a_result_t result = k4a_record_create("record_test_bgra_color.mkv", NULL, record_config, &handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    result = k4a_record_write_header(handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    uint64_t timestamp_ns = 0;
    for (int i = 0; i < 1000; i++)
    {
        k4a_capture_t capture = NULL;
        result = k4a_capture_create(&capture);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        uint32_t width = 3840;
        uint32_t height = 2160;
        uint32_t stride = width * 4;
        size_t buffer_size = height * stride;
        uint8_t *buffer = new uint8_t[height * stride];
        memset(buffer, 0xFF, height * stride);

        k4a_image_t color_image = NULL;
        result = k4a_image_create_from_buffer(record_config.color_format,
                                              (int)width,
                                              (int)height,
                                              (int)stride,
                                              buffer,
                                              buffer_size,
                                              [](void *_buffer, void *ctx) {
                                                  delete[](uint8_t *) _buffer;
                                                  (void)ctx;
                                              },
                                              NULL,
                                              &color_image);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

        k4a_image_set_device_timestamp_usec(color_image, timestamp_ns / 1000);
        k4a_capture_set_color_image(capture, color_image);
        k4a_image_release(color_image);

        result = k4a_record_write_capture(handle, capture);
        ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);
        k4a_capture_release(capture);
        timestamp_ns += 1_s / 30;
    }

    result = k4a_record_flush(handle);
    ASSERT_EQ(result, K4A_RESULT_SUCCEEDED);

    k4a_record_close(handle);

    ASSERT_EQ(std::remove("record_test_bgra_color.mkv"), 0);
}

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
