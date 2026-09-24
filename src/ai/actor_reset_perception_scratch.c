// actor_reset_perception_scratch  (Ghidra: actor_reset_perception_scratch, renamed)
// address 0x428f40, size 190 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: phase-4 summary "Resets a per-actor perception/aim scratch structure, seeding it
// from the current unit's orientation fields before clearing related lookahead state."
// Calls unit_get_forward_vector_or_marker_normal (fills a caller-owned request block, itself taking no visible
// arguments -- presumably the block built on this function's own stack), unit_apply_control_block and
// unit_refresh_targeting_flag_and_weapons, none established elsewhere in this repo.
//   UNSURE: this is one of the least-confident rewrites in this pass. The 22-byte stack
//   block's fields (a flag pair, three -1 sentinels, and a copy of the shared origin
//   vector), the six unit fields read at +0x23c/0x240/0x244/0x260/0x264/0x268, and the exact
//   argument unit_get_forward_vector_or_marker_normal/unit_apply_control_block/unit_refresh_targeting_flag_and_weapons expect are all guessed from the
//   pseudocode's literal offsets, not independently confirmed with objdump.
// register convention: ESI -> unit_index (unaff_ESI).
//   // blam-cc: ESI -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern const real_vector3d *global_origin3d_pointer; // 0x00696714
extern data_array *object_data; // 0x008603b0

// The caller-owned request block unit_get_forward_vector_or_marker_normal fills; only the fields this function itself
// writes are named.

extern void unit_get_forward_vector_or_marker_normal(actor_perception_request *request); // 0x569720, UNSURE signature
extern void unit_apply_control_block(uint32_t param); // 0x5639f0, UNSURE signature
extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_index); // 0x569bf0, UNSURE signature

// blam-cc: ESI -> unit_index
void actor_reset_perception_scratch(datum_index unit_index)
{
    actor_perception_request request;
    uint32_t cached[6];
    object *unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;

    request.flag_a = 1;
    request.flag_b = 1;
    request.unknown_02 = 0;
    request.unknown_04 = -1;
    request.unknown_06 = -1;
    request.unknown_08 = -1;
    request.origin = *global_origin3d_pointer;

    unit_get_forward_vector_or_marker_normal(&request);

    cached[0] = *(uint32_t *)((uint8_t *)unit_object + 0x23c);
    cached[1] = *(uint32_t *)((uint8_t *)unit_object + 0x240);
    cached[2] = *(uint32_t *)((uint8_t *)unit_object + 0x244);
    cached[3] = *(uint32_t *)((uint8_t *)unit_object + 0x260);
    cached[4] = *(uint32_t *)((uint8_t *)unit_object + 0x264);
    cached[5] = *(uint32_t *)((uint8_t *)unit_object + 0x268);
    (void)cached;

    unit_apply_control_block(0xffffffff);
    unit_refresh_targeting_flag_and_weapons(unit_index);
}

#if 0
Original Ghidra decompilation (0x428f40):

void FUN_00428f40(void)

{
  undefined4 *puVar1;
  int iVar2;
  uint unaff_ESI;
  undefined4 local_40;
  undefined2 local_3c;
  undefined2 local_3a;
  undefined2 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  puVar1 = &local_40;
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar1 = 0;
    puVar1 = puVar1 + 1;
  }
  local_34 = *(undefined4 *)PTR_DAT_00696714;
  local_30 = *(undefined4 *)(PTR_DAT_00696714 + 4);
  local_2c = *(undefined4 *)(PTR_DAT_00696714 + 8);
  local_40._0_1_ = 1;
  local_40._1_1_ = 1;
  local_40._2_2_ = 0;
  local_3c = 0xffff;
  local_3a = 0xffff;
  local_38 = 0xffff;
  FUN_00569720();
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_ESI & 0xffff) * 0xc);
  local_18 = *(undefined4 *)(iVar2 + 0x23c);
  local_14 = *(undefined4 *)(iVar2 + 0x240);
  local_10 = *(undefined4 *)(iVar2 + 0x244);
  local_c = *(undefined4 *)(iVar2 + 0x260);
  local_8 = *(undefined4 *)(iVar2 + 0x264);
  local_4 = *(undefined4 *)(iVar2 + 0x268);
  FUN_005639f0(0xffffffff);
  FUN_00569bf0();
  return;
}
#endif
