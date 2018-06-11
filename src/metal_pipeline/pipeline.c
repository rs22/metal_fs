#include "pipeline.h"

#include <stdio.h>
#include <string.h>
#include <endian.h>
#include <errno.h>
#include <pthread.h>

#include <snap_tools.h>
#include <libsnap.h>
#include <snap_hls_if.h>

#include "../metal_fpga/include/action_metalfpga.h"
#include "../metal_storage/storage.h"

#include "../metal/metal.h"

pthread_mutex_t snap_mutex = PTHREAD_MUTEX_INITIALIZER;

struct snap_action *_action = NULL;
struct snap_card *_card = NULL;

int mtl_pipeline_initialize() {
    char device[128];
    snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", 0);

    if (_card == NULL) {
        _card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM, SNAP_DEVICE_ID_SNAP);
        if (_card == NULL) {
            fprintf(stderr, "err: failed to open card %u: %s\n", 0, strerror(errno));
            return MTL_ERROR_INVALID_ARGUMENT;
        }
    }

    if (_action == NULL) {
        snap_action_flag_t action_irq = (SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
        _action = snap_attach_action(_card, METALFPGA_ACTION_TYPE, action_irq, 60);
        if (_action == NULL) {
            fprintf(stderr, "err: failed to attach action %u: %s\n", 0, strerror(errno));
            snap_card_free(_card);
            return MTL_ERROR_INVALID_ARGUMENT;
        }
    }

    return MTL_SUCCESS;
}

int mtl_pipeline_deinitialize() {
    snap_detach_action(_action);
    snap_card_free(_card);

    _action = NULL;
    _card = NULL;

    return MTL_SUCCESS;
}

void mtl_configure_operator(mtl_operator_specification *op_spec) {
    pthread_mutex_lock(&snap_mutex);

    op_spec->apply_config(_action);

    pthread_mutex_unlock(&snap_mutex);
}

void mtl_finalize_operator(mtl_operator_specification *op_spec) {
    if (op_spec->finalize) {
        pthread_mutex_lock(&snap_mutex);

        op_spec->finalize(_action);

        pthread_mutex_unlock(&snap_mutex);
    }
}

void mtl_configure_pipeline(mtl_operator_execution_plan execution_plan) {
    uint64_t enable_mask = 0;
    for (uint64_t i = 0; i < execution_plan.length; ++i)
        enable_mask |= (1 << execution_plan.operators[i].enable_id);

    uint64_t *job_struct_enable = (uint64_t*)snap_malloc(sizeof(uint32_t) * 10);
    *job_struct_enable = htobe64(enable_mask);

    uint32_t *job_struct = (uint32_t*)(job_struct_enable + 1);
    const uint32_t disable = 0x80000000;
    for (int i = 0; i < 8; ++i)
        job_struct[i] = htobe32(disable);

    uint8_t previous_op_stream = execution_plan.length ? execution_plan.operators[0].stream_id : 0;
    for (uint64_t i = 1; i < execution_plan.length; ++i) {
        // From the perspective of the Stream Switch:
        // Which Master port (output) should be
        // sourced from which Slave port (input)
        job_struct[execution_plan.operators[i].stream_id] = htobe32(previous_op_stream);
        previous_op_stream = execution_plan.operators[i].stream_id;
    }

    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_CONFIGURE_STREAMS;
    mjob.job_address = (uint64_t)job_struct_enable;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    free(job_struct_enable);

    if (rc != 0)
        // Some error occurred
        return;

    if (cjob.retc != SNAP_RETC_SUCCESS)
        // Some error occurred
        return;

    return;
}

void mtl_run_pipeline() {
    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_RUN_OPERATORS;
    mjob.job_address = 0;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    if (rc != 0)
        // Some error occurred
        return;

    if (cjob.retc != SNAP_RETC_SUCCESS)
        // Some error occurred
        return;

    return;
}

void mtl_configure_perfmon(uint64_t stream_id) {
    uint64_t *job_struct = (uint64_t*)snap_malloc(sizeof(uint64_t));
    *job_struct = htobe64(stream_id);

    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_CONFIGURE_PERFMON;
    mjob.job_address = (uint64_t)job_struct;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    free(job_struct);

    if (rc != 0)
        // Some error occurred
        return;

    if (cjob.retc != SNAP_RETC_SUCCESS)
        // Some error occurred
        return;

    return;
}

void mtl_print_perfmon() {
    uint32_t *job_struct = (uint32_t*)snap_malloc(sizeof(uint32_t) * 7);

    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_READ_PERFMON_COUNTERS;
    mjob.job_address = (uint64_t)job_struct;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    if (rc != 0) {
        // Some error occurred
        free(job_struct);
        return;
    }

    if (cjob.retc != SNAP_RETC_SUCCESS) {
        // Some error occurred
        free(job_struct);
        return;
    }

    printf("Performance counters\n");

    printf("  Transfer Cycle Count: %u\n", be32toh(job_struct[0]));
    printf("    Gives the total number of cycles the data is transferred.\n");
    printf("\n");

    printf("  Packet Count: %u\n", be32toh(job_struct[1]));
    printf("    Gives the total number of packets transferred.\n");
    printf("\n");

    printf("  Data Byte Count: %u\n", be32toh(job_struct[2]));
    printf("    Gives the total number of data bytes transferred.\n");
    printf("\n");

    printf("  Position Byte Count: %u\n", be32toh(job_struct[3]));
    printf("    Gives the total number of position bytes transferred.\n");
    printf("\n");

    printf("  Null Byte Count: %u\n", be32toh(job_struct[4]));
    printf("    Gives the total number of null bytes transferred.\n");
    printf("\n");

    printf("  Slv_Idle_Cnt: %u\n", be32toh(job_struct[5]));
    printf("    Gives the number of idle cycles caused by the slave.\n");
    printf("\n");

    printf("  Mst_Idle_Cnt: %u\n", be32toh(job_struct[6]));
    printf("    Gives the number of idle cycles caused by the master.\n");

    return;
}

void mtl_pipeline_set_file_read_extent_list(const mtl_file_extent *extents, uint64_t length) {

    // Allocate job struct memory aligned on a page boundary
    uint64_t *job_words = (uint64_t*)snap_malloc(
        sizeof(uint64_t) * (
            8 // words for the prefix
            + (2 * length) // two words for each extent
        )
    );

    // printf("Mapping %lu extents for reading\n", length);
    job_words[0] = htobe64(0);  // slot number
    job_words[1] = htobe64(true);  // map (vs unmap)
    job_words[2] = htobe64(length);  // extent count

    for (uint64_t i = 0; i < length; ++i) {
        // printf("   Extent %lu has length %lu and starts at %lu\n", i, extents[i].length, extents[i].offset);
        job_words[8 + 2*i + 0] = htobe64(extents[i].offset);
        job_words[8 + 2*i + 1] = htobe64(extents[i].length);
    }

    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_MAP;
    mjob.job_address = (uint64_t)job_words;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    free(job_words);

    if (rc != 0) {
        return;
    }

    if (cjob.retc != SNAP_RETC_SUCCESS) {
        return;
    }
}

void mtl_pipeline_set_file_write_extent_list(const mtl_file_extent *extents, uint64_t length) {

    // Allocate job struct memory aligned on a page boundary
    uint64_t *job_words = (uint64_t*)snap_malloc(
        sizeof(uint64_t) * (
            8 // words for the prefix
            + (2 * length) // two words for each extent
        )
    );

    // printf("Mapping %lu extents for writing\n", length);
    job_words[0] = htobe64(1);  // slot number
    job_words[1] = htobe64(true);  // map (vs unmap)
    job_words[2] = htobe64(length);  // extent count

    for (uint64_t i = 0; i < length; ++i) {
        // printf("   Extent %lu has length %lu and starts at %lu\n", i, extents[i].length, extents[i].offset);
        job_words[8 + 2*i + 0] = htobe64(extents[i].offset);
        job_words[8 + 2*i + 1] = htobe64(extents[i].length);
    }

    metalfpga_job_t mjob;
    mjob.job_type = MTL_JOB_MAP;
    mjob.job_address = (uint64_t)job_words;

    struct snap_job cjob;
    snap_job_set(&cjob, &mjob, sizeof(mjob), NULL, 0);

    pthread_mutex_lock(&snap_mutex);
    const unsigned long timeout = 10;
    int rc = snap_action_sync_execute_job(_action, &cjob, timeout);
    pthread_mutex_unlock(&snap_mutex);

    free(job_words);

    if (rc != 0) {
        return;
    }

    if (cjob.retc != SNAP_RETC_SUCCESS) {
        return;
    }
}
