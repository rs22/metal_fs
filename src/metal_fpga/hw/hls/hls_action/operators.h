#ifndef __MTL_OPERATORS_H__
#define __MTL_OPERATORS_H__

#include <hls_stream.h>
#include <hls_snap.H>
#include "mtl_definitions.h"
#include "mtl_op_mem.h"

void clear_operator_interrupts(hls::stream<snapu8_t> &interrupt_reg, snapu32_t *metal_ctrl);

void action_run_operators(
    snap_membus_t * mem_in,
    snap_membus_t * mem_out,
    axi_datamover_command_stream_t &mm2s_cmd,
    axi_datamover_status_stream_t &mm2s_sts,
    axi_datamover_command_stream_t &s2mm_cmd,
    axi_datamover_status_ibtt_stream_t &s2mm_sts,
    snapu32_t *metal_ctrl,
    hls::stream<snapu8_t> &interrupt_reg,
    snapu64_t enable_mask,
    snapu64_t *bytes_written
);

#endif // __MTL_OPERATORS_H__
