// actor_reset_perception_scratch  (Ghidra: actor_reset_perception_scratch, renamed)
// address 0x428f40, size 190 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (REWRITTEN from objdump)
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

#include <string.h>
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

extern void unit_get_forward_vector_or_marker_normal(uint32_t unit_index, real_vector3d *out); // 0x569720, ECX, EAX
extern void unit_apply_control_block(uint32_t unit_index, const void *control, int32_t source_id); // 0x5639f0, EAX, EDX, stack
extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_index, uint8_t initial_targeting_flag); // 0x569bf0, stack, CL

// blam-cc: ESI -> unit_index
// REWRITTEN from objdump 0x428f40..0x428ffd. Builds a neutral 0x40-byte unit control block: bytes 0/1 = 1, word 2 = 0,
//   words 4/6/8 = -1, +0x0c = the global origin (zero throttle), +0x1c = the unit's forward (or marker normal),
//   +0x28 = unit +0x23c (aim), +0x34 = unit +0x260 (look). It then applies the block to the unit
//   (unit_apply_control_block: EAX unit, EDX block, stack -1) and refreshes targeting (CL = 0). The draft
//   passed no unit or block to the helpers, so the unit received garbage controls.
// blam-cc: ESI -> unit_index
void actor_reset_perception_scratch(datum_index unit_index)
{
    uint8_t block[0x40];
    uint8_t *unit_object = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;

    memset(block, 0, sizeof(block));
    block[0] = 1;
    block[1] = 1;
    *(int16_t *)(block + 0x2) = 0;
    *(int16_t *)(block + 0x4) = -1;
    *(int16_t *)(block + 0x6) = -1;
    *(int16_t *)(block + 0x8) = -1;
    *(real_vector3d *)(block + 0xc) = *(const real_vector3d *)global_origin3d_pointer;
    unit_get_forward_vector_or_marker_normal(unit_index, (real_vector3d *)(block + 0x1c));
    *(real_vector3d *)(block + 0x28) = *(real_vector3d *)(unit_object + 0x23c);
    *(real_vector3d *)(block + 0x34) = *(real_vector3d *)(unit_object + 0x260);
    unit_apply_control_block(unit_index, block, -1);
    unit_refresh_targeting_flag_and_weapons(unit_index, 0); // CL = 0
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
