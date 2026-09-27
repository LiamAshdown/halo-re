// unit_update_ik_detail_nodes  (Ghidra: no function created; the phase-4 types agent carved a
//   placeholder "missed_5643f0" from the object_type_definition vtable evidence)
// address 0x5643f0, size 379 bytes
// name confidence 0.3, rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x5643f0..0x564573 (parent and weapon IK chains; ECX object for 0x4f6d60).) (see the UNSURE notes)
// evidence: out/phase4/units_types_notes.md: "The unit row's other columns are ... 0x5643f0
//   (+0x4c) ...". Both loops in this function call object_solve_two_bone_ik_to_marker (0x4f6d60,
//   already rewritten this pass), which is exactly a "walk a marker's node up two levels and
//   solve a two-bone IK chain to it" helper, so this column runs a unit's per-tick IK chains: one
//   set driven by the unit's own current animation-graph "unit block" record (gated on the
//   Unit tag not being `simple_creature`, bit 0x800 of UnitFlags at tag+0x17c, and on
//   unit_data.animation_definition_index being valid), and a second set driven by the current
//   weapon's animation-graph "weapon block" record (gated on unit_data.current_weapon_index being
//   valid), each iterating a table of IK-chain records and clearing unit_data.animation_state_
//   flags bit 0 once the weapon set runs.
// Cleanup-pass review (objdump 0x5643f0..0x564570): FUN_00565d60 / FUN_00565d00 both receive
//   ECX = &unit_data.animation_state_flags (object+0x298, 0x564480 / 0x5644e8), which the draft
//   dropped; passed now.
// UNSURE: the "unit block" and "weapon block" record layouts inside the ModelAnimations tag
//   (reached via tag+0x44's animation_graph tag_id, matching Object.animation_graph in
//   types/tags.h, then +0x10 / +0x5c per unit_update_scale_function_inputs.c's own header note
//   on animation_definition_index / animation_weapon_index) are not defined anywhere in
//   types/tags.h. The IK-chain record table at unit_record+0x50 (count at +0x4c) and the weapon
//   detail-node table at weapon_record+0xa8 (count at +0xa4) are read as raw 0x40-byte-stride
//   arrays of {char marker_a_name[0x20]; char marker_b_name[0x20];} records, matching
//   object_solve_two_bone_ik_to_marker's own (marker_a_name, ..., marker_b_name, ...) call shape
//   and TagString's 0x20-byte size; not cross-checked against a models/cache types pass.
// UNSURE: object_solve_two_bone_ik_to_marker takes its object index in ECX, a register argument
//   Ghidra's decompile of THIS function drops from both call sites; passed as this function's own
//   object_index here (the only object index in scope), not invented further.
// UNSURE: FUN_00565d60 / FUN_00565d00 are outside this pass's range; called with zero visible
//   arguments exactly as Ghidra decompiled them, and their boolean-looking return is tested the
//   same way both places.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t unit_animation_state_allows_parent_ik(uint8_t *animation_block); // 0x565d60, blam-cc: ECX -> animation_block;
extern uint8_t unit_animation_state_allows_weapon_ik(uint8_t *animation_block); // 0x565d00, blam-cc: ECX -> animation_block;
extern void object_solve_two_bone_ik_to_marker(uint32_t object_index, char *marker_a_name,
    uint32_t param_2, char *param_3, uint8_t *node_base); // 0x4f6d60,

// object_type_definition "unit" row, +0x4c column. Solves the unit's own IK chains (from its
// animation-graph "unit block" record) and, when it is holding a weapon, that weapon's IK chains
// (from the graph's "weapon block" record), then clears the "weapon IK dirty" flag.
// FIXED (register inputs, objdump): the original never reads ECX as an input (it overwrites or only saves it); those parameters arrive on the stack (2 stack argument(s) read).
// blam-cc: stack -> object_index, node_base
void unit_update_ik_detail_nodes(uint32_t object_index, void *node_base)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Unit *tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if ((tag->unit_flags & 0x800) != 0 || unit->animation_definition_index == -1) { // 0x800 = simple_creature
        return;
    }

    {
        // UNSURE: raw "unit block" / "weapon block" walk; see the header note above
        tag_instance *graph = &tag_instances[*(int32_t *)&tag->base.animation_graph.tag_id & 0xffff];
        uint8_t *unit_block = *(uint8_t **)((uint8_t *)graph->data + 0x10);
        uint8_t *unit_record = unit_block + (int32_t)unit->animation_definition_index * 100;
        // the weapons sub-block pointer lives inside unit_record itself, at +0x5c
        uint8_t *weapon_block = *(uint8_t **)(unit_record + 0x5c);
        uint8_t *weapon_record = weapon_block + (int32_t)unit->animation_weapon_index * 0xbc;

        if (obj->parent_object != (datum_index)-1 && unit_animation_state_allows_parent_ik((uint8_t *)&unit->animation_state_flags) != 0) {
            int32_t count = *(int32_t *)(unit_record + 0x4c);
            uint8_t *table = *(uint8_t **)(unit_record + 0x50);
            int32_t i;
            for (i = 0; i < count; i++) {
                uint8_t *entry = table + i * 0x40;
                object_solve_two_bone_ik_to_marker(object_index, (char *)entry,
                    obj->parent_object, (char *)(entry + 0x20), node_base);
            }
        }

        if (unit->current_weapon_index != -1 && unit_animation_state_allows_weapon_ik((uint8_t *)&unit->animation_state_flags) != 0) {
            int32_t count = *(int32_t *)(weapon_record + 0xa4);
            uint8_t *table = *(uint8_t **)(weapon_record + 0xa8);
            int32_t i;
            for (i = 0; i < count; i++) {
                object *fresh_obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
                unit_data *fresh_unit = (unit_data *)((uint8_t *)fresh_obj + k_unit_data_offset);
                uint8_t *entry = table + i * 0x40;
                int16_t current_weapon = fresh_unit->current_weapon_index;
                datum_index weapon_handle = (datum_index)-1;
                if (current_weapon != -1) {
                    weapon_handle = fresh_unit->weapons[current_weapon];
                }
                object_solve_two_bone_ik_to_marker(object_index, (char *)entry,
                    weapon_handle, (char *)(entry + 0x20), node_base);
            }
            unit->animation_state_flags &= 0xfffe;
        }
    }
}

#if 0
Original Ghidra decompilation (0x5643f0):

void missed_5643f0(uint param_1,undefined4 param_2)

{
  short sVar1;
  uint *puVar2;
  char cVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined4 uVar8;
  int iVar9;

  iVar5 = (param_1 & 0xffff) * 0xc;
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar5);
  iVar9 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((*(uint *)(iVar9 + 0x17c) & 0x800) == 0) && ((char)puVar2[0xa8] != -1)) {
    iVar6 = (char)puVar2[0xa8] * 100;
    iVar9 = *(int *)(*(int *)((*(uint *)(iVar9 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                    0x10);
    iVar7 = iVar6 + iVar9;
    iVar9 = *(char *)((int)puVar2 + 0x2a1) * 0xbc + *(int *)(iVar6 + 0x5c + iVar9);
    if ((puVar2[0x47] != 0xffffffff) &&
       ((cVar3 = FUN_00565d60(), cVar3 != '\0' && (0 < *(int *)(iVar7 + 0x4c))))) {
      iVar6 = 0;
      sVar4 = 0;
      do {
        iVar6 = iVar6 * 0x40 + *(int *)(iVar7 + 0x50);
        FUN_004f6d60(iVar6,puVar2[0x47],iVar6 + 0x20,param_2);
        sVar4 = sVar4 + 1;
        iVar6 = (int)sVar4;
      } while (iVar6 < *(int *)(iVar7 + 0x4c));
    }
    if (*(short *)((int)puVar2 + 0x2f2) != -1) {
      cVar3 = FUN_00565d00();
      if (cVar3 != '\0') {
        sVar4 = 0;
        if (0 < *(int *)(iVar9 + 0xa4)) {
          iVar6 = 0;
          do {
            iVar7 = iVar6 * 0x40 + *(int *)(iVar9 + 0xa8);
            iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar5);
            sVar1 = *(short *)(iVar6 + 0x2f2);
            uVar8 = 0xffffffff;
            if (sVar1 != -1) {
              uVar8 = *(undefined4 *)(iVar6 + 0x2f8 + sVar1 * 4);
            }
            FUN_004f6d60(iVar7,uVar8,iVar7 + 0x20,param_2);
            sVar4 = sVar4 + 1;
            iVar6 = (int)sVar4;
          } while (iVar6 < *(int *)(iVar9 + 0xa4));
        }
        *(byte *)(puVar2 + 0xa6) = (byte)puVar2[0xa6] & 0xfe;
      }
    }
  }
  return;
}
#endif
