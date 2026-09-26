// scenario_objects_place_for_structure_bsp  (Ghidra: FUN_004f4880; formerly objects_update_player_visibility_masks)
// address 0x4f4880, size 588 bytes
// name confidence: 0.75  rewrite confidence: 0.85
// evidence: scenario_objects_place 0x4f3ba0 tail-jumps here with place = 1; the structure BSP switch calls it too.
// REWRITTEN (first-boot track, objdump 0x4f4880..0x4f4acb): nothing without a structure BSP. For scenery and light
//   fixtures only (mask 0x240), whose placements depend on the BSP:
//   - the first time this BSP is seen (bit not yet in 0x006b8cb0), each placement's bit for this BSP in its word
//     at +0x20 is set when its position, or its definition's origin (tag data +8) moved by the placement's
//     Euler-angle matrix, lies in a leaf of the collision BSP (bsp3d_node_find_leaf(0, bsp, point) != -1), and
//     cleared otherwise
//   - with place set: objects_garbage_collection, block_list_compact(object_memory_pool), then every placement not
//     already named in use, not network-only (+4 bit 0) and marked for this BSP is created
//     (object_new_from_scenario_placement) followed by objects_garbage_collection
//   Then this BSP's bit is added to 0x006b8cb0. The old version called bsp3d_node_find_leaf with its arguments
//   in the wrong order, looked the definition up through the placement instead of the palette, and dropped the
//   arguments of the create and compact calls.
// blam-cc: stack -> place
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern int16_t global_structure_bsp_index; // 0x0069e8d8
extern uint8_t *global_scenario; // 0x00746f8c
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc
extern uint16_t object_visibility_computed_mask; // 0x006b8cb0
extern datum_index *object_name_list; // 0x006b8cb8
extern tag_instance *tag_instances; // 0x0087bc14
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern memory_pool *object_memory_pool; // 0x006b8cb4

extern void matrix4x3_from_euler_angles(real_matrix4x3 *out, real yaw, real pitch, real roll); // 0x4cba10, EAX out
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0,
    // blam-cc: EAX -> out, EDX -> point, stack -> m
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0,
    // blam-cc: EAX -> node_index, ECX -> bsp, EDX -> point
extern void objects_garbage_collection(void); // 0x4f9c60
extern void block_list_compact(memory_pool *arena); // 0x4d1eb0, blam-cc: EBX -> arena
extern datum_index object_new_from_scenario_placement(uint8_t *placement, TagReflexive *palette); // 0x4f9b70

void scenario_objects_place_for_structure_bsp(uint8_t place)
{
    int16_t type;
    uint16_t bsp_bit;

    if (global_structure_bsp_index == -1) {
        return;
    }
    bsp_bit = (uint16_t)(1 << global_structure_bsp_index);
    for (type = 0; type < k_maximum_object_types; type++) {
        object_type_definition *definition = object_type_definitions[type];
        TagReflexive *placements;
        TagReflexive *palette;
        int32_t size;
        int16_t i;

        if (((1 << type) & 0x240) == 0 || definition->scenario_placement_offset == -1 ||
            definition->scenario_palette_offset == -1) {
            continue;
        }
        size = definition->scenario_placement_size;
        placements = (TagReflexive *)(global_scenario + definition->scenario_placement_offset);
        palette = (TagReflexive *)(global_scenario + definition->scenario_palette_offset);

        if ((object_visibility_computed_mask & bsp_bit) == 0) {
            for (i = 0; i < (int32_t)placements->count; i++) {
                uint8_t *placement = (uint8_t *)placements->pointer + i * size;
                int16_t kind = *(int16_t *)placement;
                real_matrix4x3 basis;
                real_point3d origin;
                datum_index tag;
                uint8_t *definition_data;

                if (kind == -1) {
                    continue;
                }
                matrix4x3_from_euler_angles(&basis, *(float *)(placement + 0x14), *(float *)(placement + 0x18),
                                            *(float *)(placement + 0x1c));
                tag = *(datum_index *)((uint8_t *)palette->pointer + kind * 0x30 + 0xc);
                definition_data = (uint8_t *)tag_instances[tag & 0xffff].data;
                matrix4x3_transform_point(&origin, (real_point3d *)(definition_data + 8), &basis);
                if (bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)(placement + 8)) == 0xffffffff &&
                    bsp3d_node_find_leaf(0, global_collision_bsp, &origin) == 0xffffffff) {
                    *(uint16_t *)(placement + 0x20) &= (uint16_t)~bsp_bit;
                } else {
                    *(uint16_t *)(placement + 0x20) |= bsp_bit;
                }
            }
        }
        if (place) {
            objects_garbage_collection();
            block_list_compact(object_memory_pool);
            for (i = 0; i < (int32_t)placements->count; i++) {
                uint8_t *placement = (uint8_t *)placements->pointer + i * size;
                int16_t name = *(int16_t *)(placement + 2);

                if (name != -1 && name >= 0 && name < 0x200 && object_name_list[name] != k_datum_index_none) {
                    continue;
                }
                if ((placement[4] & 1) != 0 || (*(uint16_t *)(placement + 0x20) & bsp_bit) == 0) {
                    continue;
                }
                object_new_from_scenario_placement(placement, palette);
                objects_garbage_collection();
            }
        }
    }
    object_visibility_computed_mask |= bsp_bit;
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
