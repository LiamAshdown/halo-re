// recorded_animation_compressed_begin  (Ghidra: missed_44a550; renamed per types/cutscene.h's
// codec table comment: "begin: 0x44a550 compressed (unpack control data, then copy the 0x0c byte
// decoder state)" and the owned global "0x00686fd8: recorded_animation_codec
// recorded_animation_compressed_codec")
// address 0x44a550, size 56 bytes
// name confidence: 0.75 (types/cutscene.h names the codec and describes this entry point's job,
// but does not spell a function name)   rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md "The codec table 0x686fe8 is {0x686fe0, 0x686fe0,
// 0x686fe0, 0x686fd8}, with 0x686fd8 = {0x44a550, 0x44a590} and 0x686fe0 = {0x44a890, 0x44a8b0}."
// types/cutscene.h recorded_animation_begin_proc gives the 4-argument shape (state, control,
// cursor, unit_control_data_version); recorded_animation_decoder_state (0x0c, three
// recorded_animation_angles) matches the three raw dwords copied here.
// register convention: objdump-traced call site 0x44a930 (recorded_animation_start) pushes
// (state = record + 0x54, control = record + 0x14, cursor = record + 0x10, version byte) in that
// order for a plain cdecl call through the codec table -- all four are ordinary stack arguments,
// no register-passed values. Internally this function loads control into EBX before calling
// unit_control_data_unpack, matching that callee's own "EBX = control" register convention
// (out/phase4/cutscene_types_notes.md / src/cutscene/unit_control_data_unpack.c); that register
// hand-off is implicit in the C call below. blam-cc: stack -> (state, control, cursor, version).

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
// Compressed (version 4) codec begin. Unpacks the initial unit_control_data from *cursor (per
// unit_control_data_version), then copies the 0x0c byte decoder state (the compressed codec's
// starting facing/aiming/looking angle triple) raw from the now-advanced *cursor into state, and
// advances *cursor past it.
void recorded_animation_compressed_begin(recorded_animation_decoder_state *state,
    unit_control_data *control, uint8_t **cursor, uint8_t version)
{
    uint32_t *source;
    uint32_t *destination;

    unit_control_data_unpack(control, cursor, version);

    source = (uint32_t *)*cursor;
    destination = (uint32_t *)state;
    destination[0] = source[0];
    destination[1] = source[1];
    destination[2] = source[2];
    *cursor += 0x0c;
}

#if 0
Original Ghidra decompilation (0x44a550):

void FUN_0044a550(undefined4 *param_1,undefined4 param_2,int *param_3,undefined4 param_4)

{
  undefined4 *puVar1;

  unit_control_data_unpack(param_3,param_4);
  puVar1 = (undefined4 *)*param_3;
  *param_1 = *puVar1;
  param_1[1] = puVar1[1];
  param_1[2] = puVar1[2];
  *param_3 = *param_3 + 0xc;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
