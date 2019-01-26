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
        context->pending_clusters = make_unique<std::list<cluster_t *>>();
    }

    void TearDown() override
    {
        k4a_record_t_destroy(recording_handle);
    }
};

static_assert(MAX_CLUSTER_LENGTH_NS > 10, "Tests need to run with clusters > 10ns");

TEST_F(record_ut, new_clusters_in_order)
{
    ASSERT_EQ(context->pending_clusters->size(), 0u);
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

    ASSERT_EQ(context->pending_clusters->size(), 2u);

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

    ASSERT_EQ(context->pending_clusters->size(), 2u);
}

TEST_F(record_ut, new_cluster_out_of_order)
{
    ASSERT_EQ(context->pending_clusters->size(), 0u);
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

    ASSERT_EQ(context->pending_clusters->size(), 3u);

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

    ASSERT_EQ(context->pending_clusters->size(), 3u);
}

TEST_F(record_ut, read_write_bytes)
{
    uint8_t buffer[32] = { 0 };
    uint8_t *data_ptr = &buffer[0];

    { // write uint32
        size_t offset = write_bytes<uint32_t>(data_ptr, 0xFFEEDDCC);
        ASSERT_EQ(offset, 4u);
        ASSERT_EQ(buffer[0], 0xCC);
        ASSERT_EQ(buffer[1], 0xDD);
        ASSERT_EQ(buffer[2], 0xEE);
        ASSERT_EQ(buffer[3], 0xFF);
        data_ptr += offset;
    }

    { // write float
        size_t offset = write_bytes<float>(data_ptr, -2.5009765625);
        ASSERT_EQ(offset, 4u);
        ASSERT_EQ(buffer[4], 0x00); // -2.5009765625 == 0xC0201000
        ASSERT_EQ(buffer[5], 0x10);
        ASSERT_EQ(buffer[6], 0x20);
        ASSERT_EQ(buffer[7], 0xC0);
        data_ptr += offset;
    }

    { // write uint64
        size_t offset = write_bytes<uint64_t>(data_ptr, 0xFFEEDDCCBBAA9988);
        ASSERT_EQ(offset, 8u);
        ASSERT_EQ(buffer[8], 0x88);
        ASSERT_EQ(buffer[9], 0x99);
        ASSERT_EQ(buffer[10], 0xAA);
        ASSERT_EQ(buffer[11], 0xBB);
        ASSERT_EQ(buffer[12], 0xCC);
        ASSERT_EQ(buffer[13], 0xDD);
        ASSERT_EQ(buffer[14], 0xEE);
        ASSERT_EQ(buffer[15], 0xFF);
        data_ptr += offset;
    }

    data_ptr = &buffer[0];

    { // read uint32
        uint32_t value = 0;
        size_t offset = read_bytes<uint32_t>(data_ptr, &value);
        ASSERT_EQ(offset, 4u);
        ASSERT_EQ(value, 0xFFEEDDCC);
        data_ptr += offset;
    }

    { // read float
        float value = 0;
        size_t offset = read_bytes<float>(data_ptr, &value);
        ASSERT_EQ(offset, 4u);
        ASSERT_EQ(value, -2.5009765625); // 0xC0201000 == -2.5009765625
        data_ptr += offset;
    }

    { // read uint64
        uint64_t value = 0;
        size_t offset = read_bytes<uint64_t>(data_ptr, &value);
        ASSERT_EQ(offset, 8u);
        ASSERT_EQ(value, 0xFFEEDDCCBBAA9988);
        data_ptr += offset;
    }
}

int main(int argc, char **argv)
{
    return k4a_test_commmon_main(argc, argv);
}
