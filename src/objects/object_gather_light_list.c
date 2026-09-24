// object_gather_light_list
// address 0x4f2430, size 280 bytes
// name confidence: 0.3 (still FUN_004f2430 in Ghidra; functions.md's summary: "Gathers the
//   visible/nearby light-datum indices for an object into its light list and resolves each to a
//   light-record pointer")
// rewrite confidence: 0.25
// evidence: types/objects.h globals list (0x008607c4 light_frame_counter, 0x008607c0
//   light_render_unknown_7c0, 0x00860b14 light_data); callees object_get_root_parent_placement
//   (0x4f5f70, this batch) and FUN_004f2df0 (0x4f2df0, this batch).
// register convention: destination struct pointer in EAX (param_1); a count at +0x40 and an
//   array of up to some N light handles at +0x44, per the raw offsets used here. types/objects.h
//   does not attribute a struct to this layout (it is not the object struct: object+0x40 falls
//   inside the module's own "unresolved, nothing touches it" range), so it is kept as a raw
//   caller-owned buffer rather than a named type.
// UNSURE: object_get_root_parent_placement and FUN_004f2df0 are both called here with no visible
//   arguments; the real object index / gather parameters they need are not visible in this
//   function's decompilation. The loop that walks the collideable-reference chain via
//   local_2c/local_28 mirrors the same chain-walk seen in object_get_root_parent_placement and
//   object_resolve_collideable_reference elsewhere in this batch, but the two output locals here
//   are kept raw rather than aliased onto those functions' types, since the call arguments that
//   would confirm the aliasing are not visible.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern int32_t light_frame_counter; // 0x008607c4
extern int32_t light_render_unknown_7c0; // 0x008607c0
extern data_array *light_data; // 0x00860b14

extern int16_t object_get_root_parent_placement(uint32_t object_index,
    object_placement_cursor *out_cursor); // out block typed per types/objects.h
    // this module, 0x4f5f70. Takes the object index in EAX and the out pointer in ESI, neither
    // of which is visible at this call site. UNSURE: passed as 0/NULL here.
extern void object_lights_gather_nearest(int16_t cluster_index, uint32_t self_object_index,
    real_point3d *probe_point, float search_margin, uint32_t *out_indices, float *out_intensities,
    uint32_t out_falloffs, int16_t *count, int16_t max_count); // this module, 0x4f2df0.
    // This call site pushes six stack slots but cleans eight (0x4f24cf call / add esp,0x20),
    // so its visible argument list does not line up with the nine-parameter form the
    // definition recovers. Declared to match the definition and called with placeholders.
    // UNSURE: none of the nine arguments is recoverable here -- see file header.

void object_gather_light_list(uint8_t *param_1) // blam-cc: EAX -> param_1
{
    int16_t *count = (int16_t *)(param_1 + 0x40);
    int32_t *local_2c; // the "family" pointer, same convention as object_get_root_parent_placement
    uint32_t local_28;
    int16_t next;
    data_array *table;
    int16_t i;

    *count = 0;
    light_frame_counter = light_frame_counter + 1;
    light_render_unknown_7c0 = 1;

    next = object_get_root_parent_placement(0, 0); // UNSURE: see file header
    table = light_data;
    while (next != -1) {
        light_data = table;
        object_lights_gather_nearest(0, 0, 0, 0.0f, 0, 0, 0, 0, 0); // UNSURE: see file header
        table = light_data;
        if (local_28 == 0xffffffff) {
            next = -1;
            local_28 = 0xffffffff;
        } else {
            uint32_t index = local_28 & 0xffff;
            data_array *reference_table = (data_array *)local_2c[2];
            object_cluster_reference *refs = (object_cluster_reference *)reference_table->data;
            local_28 = refs[index].next_reference;
            next = (int16_t)refs[index].object_index;
        }
    }

    light_render_unknown_7c0 = 0;
    if (*count > 0) {
        for (i = 0; i < *count; i++) {
            uint32_t *slot = (uint32_t *)(param_1 + 0x44 + i * 4);
            *slot = *(uint32_t *)((uint8_t *)table->data + (*slot & 0xffff) * 0x7c + 8);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f2430):

void FUN_004f2430(int param_1)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  uint uVar5;
  short *psVar6;
  int local_2c;
  uint local_28;

  psVar6 = (short *)(param_1 + 0x40);
  *psVar6 = 0;
  DAT_008607c4 = DAT_008607c4 + 1;
  DAT_008607c0 = 1;
  sVar4 = FUN_004f5f70();
  iVar3 = DAT_00860b14;
  while (DAT_00860b14 = iVar3, sVar4 != -1) {
    FUN_004f2df0();
    iVar3 = DAT_00860b14;
    if (local_28 == 0xffffffff) {
      sVar4 = -1;
      local_28 = 0xffffffff;
    }
    else {
      uVar5 = local_28 & 0xffff;
      iVar2 = *(int *)(*(int *)(local_2c + 8) + 0x34);
      local_28 = *(uint *)(iVar2 + 8 + uVar5 * 0xc);
      sVar4 = (short)*(undefined4 *)(iVar2 + uVar5 * 0xc + 4);
    }
  }
  sVar4 = 0;
  DAT_008607c0 = 0;
  if (0 < *psVar6) {
    do {
      puVar1 = (uint *)(param_1 + 0x44 + sVar4 * 4);
      sVar4 = sVar4 + 1;
      *puVar1 = *(uint *)((*puVar1 & 0xffff) * 0x7c + 8 + *(int *)(iVar3 + 0x34));
    } while (sVar4 < *psVar6);
  }
  return;
}
#endif
