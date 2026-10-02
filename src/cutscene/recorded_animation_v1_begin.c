// recorded_animation_v1_begin  (Ghidra: missed_44a890; renamed per types/cutscene.h's codec table
// comment: "begin: ... 0x44a890 v1 (unpack control data only)" and the owned global
// "0x00686fe0: recorded_animation_codec recorded_animation_v1_codec")
// address 0x44a890, size 25 bytes
// name confidence: 0.75 (types/cutscene.h names the codec and describes this entry point's job,
// but does not spell a function name)   rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "The codec table 0x686fe8 is {0x686fe0, 0x686fe0,
// 0x686fe0, 0x686fd8}, with 0x686fd8 = {0x44a550, 0x44a590} and 0x686fe0 = {0x44a890, 0x44a8b0}."
// Compare with recorded_animation_compressed_begin (0x44a550, this batch): the same
// unit_control_data_unpack call, but this v1 begin never touches state, matching
// types/cutscene.h's "the v1 codec never touches [the decoder state]".
// register convention: same recorded_animation_begin_proc shape and call site as
// recorded_animation_compressed_begin (state, control, cursor, unit_control_data_version), all
// cdecl stack arguments. As in the compressed begin, control is loaded into EBX before the call
// to unit_control_data_unpack, matching that callee's own register convention; state (the first
// argument) is loaded but never used.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void unit_control_data_unpack(unit_control_data *control, uint8_t **cursor,
    uint8_t version); // 0x449fd0, this module

// blam-cc: stack -> (state, control, cursor, version)
// Uncompressed (versions 1..3) codec begin. Unpacks the initial unit_control_data from *cursor
// (per unit_control_data_version) and advances *cursor past it. Unlike the compressed begin, the
// v1 codec has no decoder state to seed, so state is unused.
void recorded_animation_v1_begin(recorded_animation_decoder_state *state, unit_control_data *control,
    uint8_t **cursor, uint8_t version)
{
    (void)state;
    unit_control_data_unpack(control, cursor, version);
}

#if 0
Original Ghidra decompilation (0x44a890):

void FUN_0044a890(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  unit_control_data_unpack(param_3,param_4);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
