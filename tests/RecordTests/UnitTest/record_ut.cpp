// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <utcommon.h>

// Module being tested
#include <k4ainternal/matroska_write.h>

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

int main(int argc, char **argv)
{
    return k4a_test_common_main(argc, argv);
}
