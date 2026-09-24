// objects_update_player_visibility_masks
// address 0x4f4880, size 588 bytes
// name confidence: 0.75 (still FUN_004f4880 in Ghidra; types/objects.h's
//   _object_mask_scenery_and_light_fixture comment names this function explicitly, and its
//   object 0x020 field comment used to attribute player_visibility_mask writes to it; see R26 below)
// rewrite confidence: 0.3
// evidence: types/objects.h object_type_definition (the same per-type slot_config indexing
//   scheme as objects_update_control_bindings 0x4f3ba0: unknown_0a at 0x0a, plus two more int16
//   fields at 0x0c/0x0e not folded into named fields); global 0x0069bfdc
//   object_type_definitions[12]; global 0x0069e8d8 "the current local player index" and
//   0x00746f8c "one of the player and local-player globals", both from types/objects.h's
//   globals-read-not-owned list; global 0x006b8cb8 object_name_list (0x200 datum_index entries,
//   matches the 0x1ff bound check here).
// register convention: prune flag in EAX (param_1, a char/bool -- functions.md: "optionally
//   pruning invalid ones").
// UNSURE: the per-type slot table's element layout (validity int16 at +0x00, a name-list index
//   at +0x02, a flag byte at +0x04, a position triple at +0x08/0xc/0x10, an euler-angle triple
//   at +0x14/0x18/0x1c, a per-player visibility uint16 at +0x20) is inferred purely from this
//   function's own arithmetic; no header struct is claimed for it, all offsets are raw. UNSURE:
//   matrix4x3_from_euler_angles/matrix4x3_transform_point and the two bsp3d_node_find_leaf calls
//   clearly pass a matrix/position through some channel Ghidra did not resolve as arguments
//   (the intervening local_44/local_40/local_3c writes); the calls are preserved exactly as
//   shown, in the order shown, without guessing that channel. UNSURE: 0x006b8cb0 (the per-player
//   "already computed" bitmask) is not documented in types/objects.h; it sits immediately before
//   the documented object_memory_pool/object_name_list globals.
// reconciled: R38 object_type_definition +0x0a/+0x0c/+0x0e/+0x10 -> scenario_placement_offset/scenario_palette_offset/scenario_placement_size/network_delta_message_type (int32, -1 = none)
// reconciled: R26/R79 the bit mask at WORD +0x20 lives in the scenario placement entries, not in the object (object 0x020 is network_position.y); 0x0069e8d8 is the structure BSP index (current_local_player_index -> global_structure_bsp_index), so each bit is 'placement lies in BSP n'
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern int16_t global_structure_bsp_index; // 0x0069e8d8, types/physics.h (was current_local_player_index)
extern Scenario *global_scenario;  // 0x00746f8c, the loaded scenario tag's data. Named and
    // typed as in the 25 src/hs files that use it; object_type_definition's +0x0a and +0x0c
    // are byte offsets into that block, which is why it reads as a bare base address here.
extern uint16_t object_visibility_computed_mask; // 0x006b8cb0, UNSURE: name and width guessed
extern datum_index *object_name_list; // 0x006b8cb8, k_maximum_object_names entries
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

extern void matrix4x3_from_euler_angles(real_matrix4x3 *out, real yaw, real pitch, real roll);
    // 0x4cba10; destination in EAX (0x4f4956 lea eax,[ebp-0x68]), the three angles on the stack
extern tag_instance *tag_instances; // 0x0087bc14
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *in, real_matrix4x3 *m); // 0x4cbde0
extern void block_list_compact(); // memory module, 0x4d1eb0. src/memory/block_list_compact.c
    // takes `memory_pool *arena`; Ghidra models no argument at the call sites in this file,
    // so no prototype is asserted here rather than inventing an arena pointer.
extern uint32_t object_get_or_build_render_permutation(); // 0x4f9b70 = object_get_or_build_render_permutation in this
    // module, whose definition is (int16_t *pair, uint8_t *table_owner) with the pair in EDI.
    // No prototype is asserted: Ghidra models fewer or differently-typed arguments here than
    // the other call site(s) of the same address, because the missing operands travel in
    // registers it could not source. The empty parameter list is the convention this module
    // already uses for FUN_00450870 -- one declaration per symbol, no invented signature.
extern void objects_garbage_collection(void); // 0x4f9c60
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern int32_t bsp3d_node_find_leaf(void *globals, real_point3d *point, int32_t index); // 0x5013a0, UNSURE: unexamined; a leaf/visibility probe

void objects_update_player_visibility_masks(uint8_t prune) // blam-cc: EAX -> prune
{
    uint8_t *scenario_base;
    int16_t type_index;
    object_type_definition **def_slot;
    int16_t remaining;

    if (global_structure_bsp_index == -1) {
        return;
    }

    // 32-bit target: the base is used as a plain integer address that 16-bit field offsets
    // out of object_type_definition are added to.
    scenario_base = (uint8_t *)global_scenario;
    type_index = 0;
    def_slot = object_type_definitions;
    remaining = k_maximum_object_types;
    do {
        object_type_definition *def = *def_slot;
        if (((1 << (type_index & 0x1f)) & _object_mask_scenery_and_light_fixture) != 0 &&
            def->scenario_placement_offset != -1 && def->scenario_palette_offset != -1) {
            int16_t stride = def->scenario_placement_size;
            uint8_t *entries_base = scenario_base + def->scenario_palette_offset;
            int32_t player_bit = global_structure_bsp_index;
            int32_t *slot = (int32_t *)(scenario_base + def->scenario_placement_offset);

            if ((object_visibility_computed_mask & (1 << (global_structure_bsp_index & 0x1f))) == 0 &&
                *slot > 0) {
                int32_t i = 0;
                int16_t counter = 0;
                do {
                    uint8_t *entry = (uint8_t *)(i * stride + slot[1]);
                    if (*(int16_t *)entry != -1) {
                        // Resolved from the disassembly at 0x4f4950..0x4f49d1: [ebp-0x68] is a
                        // real_matrix4x3 built from the instance's three Euler angles, and
                        // matrix4x3_transform_point maps the DEFINITION's origin
                        // (tag_data + 8, EDX) through it into [ebp-0x2c]. The two visibility
                        // probes then test the instance's own position (EBX = entry + 8) and
                        // that transformed origin, in that order.
                        real_matrix4x3 instance_basis;  // [ebp-0x68]
                        real_point3d transformed_origin; // [ebp-0x2c]
                        real_point3d *instance_position = (real_point3d *)(entry + 8);
                        uint8_t *definition_data =
                            (uint8_t *)tag_instances[*(uint32_t *)(entry + 0) & 0xffff].data;
                            // UNSURE: the tag handle is reached through def + 0x0c in the original;
                            // kept here as the nearest readable equivalent

                        matrix4x3_from_euler_angles(&instance_basis, *(float *)(entry + 0x14),
                                                     *(float *)(entry + 0x18), *(float *)(entry + 0x1c));
                        matrix4x3_transform_point(&transformed_origin,
                                                  (real_point3d *)(definition_data + 8),
                                                  &instance_basis);
                        if (bsp3d_node_find_leaf(global_collision_bsp, instance_position, 0) == -1 &&
                            bsp3d_node_find_leaf(global_collision_bsp, &transformed_origin, 0) == -1) {
                            *(uint16_t *)(entry + 0x20) &= (uint16_t)~(1 << (player_bit & 0x1f));
                        } else {
                            *(uint16_t *)(entry + 0x20) |= (uint16_t)(1 << (player_bit & 0x1f));
                        }
                    }
                    counter = counter + 1;
                    i = counter;
                } while (i < *slot);
            }

            if (prune != 0) {
                uint8_t *reset_base = entries_base;
                int16_t j;
                objects_garbage_collection();
                block_list_compact(); // the arena pointer is not visible at this call site
                j = 0;
                if (*slot > 0) {
                    int32_t k = 0;
                    do {
                        uint8_t *entry = (uint8_t *)(k * stride + slot[1]);
                        int16_t name_index = *(int16_t *)(entry + 2);
                        if ((name_index == -1 || name_index < 0 || name_index > 0x1ff ||
                             object_name_list[name_index] == k_datum_index_none) &&
                            (entry[4] & 1) == 0 &&
                            (*(uint16_t *)(entry + 0x20) & (1 << (global_structure_bsp_index & 0x1f))) != 0) {
                            object_get_or_build_render_permutation(reset_base);
                            objects_garbage_collection();
                            reset_base = entries_base;
                        }
                        j = j + 1;
                        k = j;
                    } while (k < *slot);
                }
            }
        }
        type_index = type_index + 1;
        def_slot = def_slot + 1;
        remaining = remaining - 1;
    } while (remaining != 0);

    object_visibility_computed_mask |= (uint16_t)(1 << (global_structure_bsp_index & 0x1f));
}

#if 0
Original Ghidra decompilation (0x4f4880):

void FUN_004f4880(char param_1)

{
  short sVar1;
  undefined *puVar2;
  short sVar3;
  int *piVar4;
  int iVar5;
  int iVar6;
  undefined1 local_6c [40];
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  int local_24;
  int local_20;
  int local_1c;
  int local_18;
  int local_14;
  int local_10;
  undefined **local_c;
  int local_8;

  if (DAT_0069e8d8 != -1) {
    local_24 = DAT_00746f8c;
    local_1c = 0;
    local_c = &PTR_PTR_0069bfdc;
    local_20 = 0xc;
    do {
      if ((((1 << ((byte)local_1c & 0x1f) & 0x240U) != 0) &&
          (puVar2 = *local_c, *(short *)(puVar2 + 10) != -1)) && (*(short *)(puVar2 + 0xc) != -1)) {
        local_18 = (int)*(short *)(puVar2 + 0xe);
        local_8 = *(short *)(puVar2 + 0xc) + local_24;
        local_10 = (int)DAT_0069e8d8;
        piVar4 = (int *)(*(short *)(puVar2 + 10) + local_24);
        if ((((uint)DAT_006b8cb0 & 1 << ((byte)DAT_0069e8d8 & 0x1f)) == 0) &&
           (local_14 = 0, 0 < *piVar4)) {
          iVar5 = 0;
          do {
            iVar6 = iVar5 * local_18 + piVar4[1];
            if (*(short *)(iVar5 * local_18 + piVar4[1]) != -1) {
              matrix4x3_from_euler_angles
                        (*(undefined4 *)(iVar6 + 0x14),*(undefined4 *)(iVar6 + 0x18),
                         *(undefined4 *)(iVar6 + 0x1c));
              local_44 = *(undefined4 *)(iVar6 + 8);
              local_40 = *(undefined4 *)(iVar6 + 0xc);
              local_3c = *(undefined4 *)(iVar6 + 0x10);
              matrix4x3_transform_point(local_6c);
              iVar5 = FUN_005013a0();
              if ((iVar5 == -1) && (iVar5 = FUN_005013a0(), iVar5 == -1)) {
                *(ushort *)(iVar6 + 0x20) =
                     *(ushort *)(iVar6 + 0x20) & ~(ushort)(1 << ((byte)local_10 & 0x1f));
              }
              else {
                *(ushort *)(iVar6 + 0x20) =
                     *(ushort *)(iVar6 + 0x20) | (ushort)(1 << ((byte)local_10 & 0x1f));
              }
            }
            local_14 = local_14 + 1;
            iVar5 = (int)(short)local_14;
          } while (iVar5 < *piVar4);
        }
        iVar5 = local_8;
        if (param_1 != '\0') {
          objects_garbage_collection();
          block_list_compact();
          sVar3 = 0;
          if (0 < *piVar4) {
            iVar6 = 0;
            do {
              iVar6 = iVar6 * local_18 + piVar4[1];
              sVar1 = *(short *)(iVar6 + 2);
              if (((((sVar1 == -1) || (sVar1 < 0)) || (0x1ff < sVar1)) ||
                  (*(int *)(DAT_006b8cb8 + sVar1 * 4) == -1)) &&
                 (((*(byte *)(iVar6 + 4) & 1) == 0 &&
                  (((uint)*(ushort *)(iVar6 + 0x20) & 1 << ((byte)DAT_0069e8d8 & 0x1f)) != 0)))) {
                FUN_004f9b70(iVar5);
                objects_garbage_collection();
                iVar5 = local_8;
              }
              sVar3 = sVar3 + 1;
              iVar6 = (int)sVar3;
            } while (iVar6 < *piVar4);
          }
        }
      }
      local_1c = local_1c + 1;
      local_c = local_c + 1;
      local_20 = local_20 + -1;
    } while (local_20 != 0);
    DAT_006b8cb0 = DAT_006b8cb0 | (ushort)(1 << ((byte)DAT_0069e8d8 & 0x1f));
  }
  return;
}
#endif
