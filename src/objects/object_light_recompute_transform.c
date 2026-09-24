// object_light_recompute_transform
// address 0x4f2a00, size 674 bytes
// name confidence: 0.6 (still FUN_004f2a00 in Ghidra; functions.md's summary matches: "Recomputes
//   an attached light's world-space transform (from its marker or a default basis) and, when
//   flagged, its spotlight cone parameters")
// rewrite confidence: 0.6
// evidence: types/objects.h light (stride 0x7c, flags 0x02, definition_tag 0x04, next_light 0x10,
//   owner_object 0x2c, position 0x30, direction 0x3c, up 0x48, marker_link 0x58, marker_index
//   0x5c); types/objects.h object_marker (node_transform at 0x38, i.e. forward 0x3c, left 0x48,
//   up 0x54, position 0x60); types/tags.h Light tag fields read through light_tag (cutoff angle
//   at 0x14, compared against pi/2 and pi/4); globals 0x00860b14 light_data, 0x008603b0
//   object_data, 0x0087bc14 tag_instances.
// register convention: light index as the single stack argument ([esp+0x8c] after the prologue's
//   sub esp,0x84 and three pushes).
// resolved from disassembly (objdump -d -M intel bin/halo.exe, 0x4f2a00..0x4f2ca1); these were
// all UNSURE in the previous draft:
//   - The 60-byte "local_6c" buffer plus Ghidra's separate local_30..local_4 are ONE
//     0x6c-byte object_marker at [esp+0x24]: Ghidra split it because the call to 0x4f6080
//     writes past the part it modelled. The nine floats copied out land exactly on
//     node_transform.position (marker+0x60), node_transform.forward (marker+0x3c) and
//     node_transform.up (marker+0x54), which is why they go to light+0x30 (position),
//     light+0x3c (direction) and light+0x48.
//   - 0x4f2a2f mov dx,[esi+0x5c] then 0x4f2a3a mov eax,edi then call 0x4f6030: FUN_004f6030
//     takes the object index in EAX and the ATTACHMENT index in DX, and returns
//     &Object.attachments[i].marker -- a marker NAME string, not a "tag entry". The "1"
//     Ghidra shows as its second argument is really 0x4f6080's fourth stack argument: MSVC
//     shares one 4-slot argument area for both calls and cleans it once (0x4f2a9c add esp,0x10).
//   - 0x4f2aa4 push -1 then mov ecx,edi then call 0x4f6ec0: object_try_and_get takes the object
//     index in ECX and the type mask as its stack argument, so this is
//     object_try_and_get(owner_object, _object_mask_all).
//   - 0x4f2c4f mov ecx,ds:0x746f9c then mov edx,[ecx+0xe4]: the leaf table pointer is TWO
//     dereferences deep (global, then +0xe4, then the table), and the leaf index is masked with
//     0x7fffffff before the 0x10 stride (0x4f2c5b and eax,0x7fffffff). The previous draft
//     dropped both the inner dereference and the mask.
//   - 0x4f2c83 lea eax,[esp+0x2c] and mov edi,0x860b20 before call 0x551f00: the leaf/cluster
//     pair computed just above is passed to 0x551f00 in EAX (Ghidra showed it as dead), and
//     EDI carries the address of the global at 0x00860b20.
// UNSURE: FUN_005013a0 (a leaf/visibility probe, ECX = the global at 0x00746f90, EDX = a stack
//   scratch pointer, EAX = 0) and object_get_root_location are not examined here; FUN_00551f00 is the same
//   placement helper object_set_cluster_and_parent uses and is left unnamed.
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *light_data; // 0x00860b14
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *global_structure_bsp; // 0x00746f9c, the render-side BSP globals block
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90, passed to FUN_005013a0 in ECX
extern datum_index *light_cluster_first; // 0x00860b20, the per-cluster light list head table;
    // FUN_00551f00 takes the ADDRESS of this descriptor in EDI (0x4f2c87 mov edi,0x860b20)

extern char *object_get_attachment_marker_name(uint32_t object_index, int16_t attachment_index); // 0x4f6030, EAX and DX
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080, all four on the stack
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, index in ECX, mask on the stack
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *in, real_matrix4x3 *m);
    // 0x4cbde0; out in EAX, in in EDX, matrix on the stack (0x4f2adb lea eax,[esi+0x30] /
    // lea edx,[esi+0x60] / push edi)
extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal,
                                                  real_matrix4x3 *m); // 0x4cbec0, same shape,
    // and it returns its output pointer in EAX (0x4f2af9 mov edx,eax feeds the next call)
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir);
    // 0x4cd670; out in ECX, dir in EDX (verified against the body, which reads only in_ECX
    // and in_EDX)
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern int32_t FUN_005013a0(void *globals, real_point3d *point, int32_t index);
    // 0x5013a0; globals in ECX, point in EDX, index in EAX
extern void object_get_root_location(uint32_t object_index); // 0x4f6b10, object index in ECX; UNSURE: unexamined
extern void FUN_00551f00(uint32_t light_or_object_handle, datum_index *placement_slot, real_point3d *position,
                          float radius, void *leaf_and_cluster, void *cluster_list);
    // 0x551f00; first four on the stack, leaf_and_cluster in EAX, placement_globals in EDI

void object_light_recompute_transform(uint32_t light_index) // blam-cc: stack -> light_index
{
    light *entry = (light *)((uint8_t *)light_data->data + (light_index & 0xffff) * 0x7c);
    datum_index owner_object = entry->owner_object;

    if (entry->marker_link == -1) {
        object_marker marker;
        char *marker_name = object_get_attachment_marker_name(owner_object, entry->marker_index);

        object_get_node_local_transform(owner_object, marker_name, &marker, 1);

        entry->position = marker.node_transform.position;
        entry->direction = marker.node_transform.forward;
        *(real_vector3d *)((uint8_t *)entry + 0x48) = marker.node_transform.up;
    } else if (object_try_and_get(owner_object, _object_mask_all) != 0) {
        object *owner = ((object_header *)object_data->data)[owner_object & 0xffff].data;
        uint8_t *node = (uint8_t *)owner + owner->nodes.offset + entry->marker_index * 0x34;

        // The positioned form keeps its local placement at light+0x60 (point) and light+0x6c
        // (direction); transform both by the owner's node matrix into light->position and
        // light->direction, then derive the perpendicular "up" at light+0x48 and normalize it.
        real_point3d *local_position = (real_point3d *)((uint8_t *)entry + 0x60);
        real_vector3d *local_direction = (real_vector3d *)((uint8_t *)entry + 0x6c);
        real_vector3d *up = (real_vector3d *)((uint8_t *)entry + 0x48);
        real_vector3d *world_direction;

        matrix4x3_transform_point(&entry->position, local_position, (real_matrix4x3 *)node);
        matrix4x3_transform_normal(&entry->direction, local_direction, (real_matrix4x3 *)node);
        world_direction = &entry->direction; // 0x4cbec0 returns its out-parameter in EAX
        vector3d_build_perpendicular(up, world_direction);
        vector3d_normalize_with_length(up);
    }

    if ((entry->flags & _light_attached_bit) != 0) {
        uint8_t *light_tag = (uint8_t *)tag_instances[entry->definition_tag & 0xffff].data;
        float attenuation = *(float *)(light_tag + 0xc) * *(float *)(light_tag + 4);
        bsp_leaf_reference leaf_reference;
        real_point3d position;
        float radius;

        if ((*light_tag & 2) == 0) {
            attenuation = attenuation * *(float *)(light_tag + 0x24);
        }

        if (*(float *)(light_tag + 0x18) <= attenuation) {
            if (*(float *)(light_tag + 0x14) >= 1.5707964f) {          // cutoff >= pi/2: omni
                position = entry->position;
            } else if (*(float *)(light_tag + 0x14) >= 0.7853982f) {   // pi/4 <= cutoff < pi/2
                float offset = attenuation * *(float *)(light_tag + 0x20);
                position.x = offset * entry->direction.i + entry->position.x;
                position.y = offset * entry->direction.j + entry->position.y;
                position.z = offset * entry->direction.k + entry->position.z;
                attenuation = attenuation * *(float *)(light_tag + 0x28);
            } else {                                                   // narrow cone
                attenuation = attenuation / *(float *)(light_tag + 0x20);
                position.x = attenuation * entry->direction.i + entry->position.x;
                position.y = attenuation * entry->direction.j + entry->position.y;
                position.z = attenuation * entry->direction.k + entry->position.z;
            }
        } else {
            position = entry->position;
            attenuation = *(float *)(light_tag + 0x18);
        }
        radius = attenuation;

        if (entry->owner_object == k_datum_index_none ||
            object_try_and_get(entry->owner_object, _object_mask_all) == 0) {
            leaf_reference.leaf_index = FUN_005013a0(global_collision_bsp, &position, 0);
            if (leaf_reference.leaf_index == -1) {
                leaf_reference.cluster_index = -1;
            } else {
                uint8_t *leaves = *(uint8_t **)(global_structure_bsp + 0xe4);
                leaf_reference.cluster_index =
                    *(int16_t *)(leaves + (leaf_reference.leaf_index & 0x7fffffff) * 0x10 + 8);
            }
        } else {
            object_get_root_location(entry->owner_object); // UNSURE: see file header
        }

        FUN_00551f00(light_index, &entry->next_light, &position, radius, &leaf_reference,
                     &light_cluster_first);
        entry->flags |= _light_transform_dirty_bit;
    }
}

#if 0
Original Ghidra decompilation (0x4f2a00):

void FUN_004f2a00(uint param_1)

{
  uint uVar1;
  byte *pbVar2;
  float fVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  int local_74;
  undefined2 local_70;
  undefined1 local_6c [60];
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar6 = (param_1 & 0xffff) * 0x7c;
  iVar7 = *(int *)(DAT_00860b14 + 0x34) + iVar6;
  uVar1 = *(uint *)(iVar7 + 0x2c);
  if (*(int *)(*(int *)(DAT_00860b14 + 0x34) + 0x58 + iVar6) == -1) {
    uVar4 = FUN_004f6030(local_6c,1);
    FUN_004f6080(uVar1,uVar4);
    *(undefined4 *)(iVar7 + 0x30) = local_c;
    *(undefined4 *)(iVar7 + 0x34) = local_8;
    *(undefined4 *)(iVar7 + 0x38) = local_4;
    *(undefined4 *)(iVar7 + 0x3c) = local_30;
    *(undefined4 *)(iVar7 + 0x40) = local_2c;
    *(undefined4 *)(iVar7 + 0x44) = local_28;
    *(undefined4 *)(iVar7 + 0x48) = local_18;
    *(undefined4 *)(iVar7 + 0x4c) = local_14;
    *(undefined4 *)(iVar7 + 0x50) = local_10;
  }
  else {
    iVar5 = object_try_and_get(0xffffffff);
    if (iVar5 != 0) {
      iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
      iVar5 = (int)*(short *)(iVar5 + 0x1f2) + *(short *)(iVar7 + 0x5c) * 0x34 + iVar5;
      matrix4x3_transform_point(iVar5);
      matrix4x3_transform_normal(iVar5);
      vector3d_build_perpendicular();
      vector3d_normalize_with_length();
    }
  }
  if ((*(byte *)(iVar7 + 2) & 2) != 0) {
    iVar5 = *(int *)(DAT_00860b14 + 0x34) + iVar6;
    pbVar2 = *(byte **)((*(uint *)(*(int *)(DAT_00860b14 + 0x34) + 4 + iVar6) & 0xffff) * 0x20 +
                        0x14 + DAT_0087bc14);
    local_84 = *(float *)(pbVar2 + 0xc) * *(float *)(pbVar2 + 4);
    if ((*pbVar2 & 2) == 0) {
      local_84 = local_84 * *(float *)(pbVar2 + 0x24);
    }
    if (*(float *)(pbVar2 + 0x18) <= local_84) {
      if (1.5707964 <= *(float *)(pbVar2 + 0x14)) {
        local_80 = *(float *)(iVar5 + 0x30);
        local_7c = *(float *)(iVar5 + 0x34);
        local_78 = *(float *)(iVar5 + 0x38);
      }
      else if (0.7853982 <= *(float *)(pbVar2 + 0x14)) {
        fVar3 = local_84 * *(float *)(pbVar2 + 0x20);
        local_80 = fVar3 * *(float *)(iVar5 + 0x3c) + *(float *)(iVar5 + 0x30);
        local_7c = fVar3 * *(float *)(iVar5 + 0x40) + *(float *)(iVar5 + 0x34);
        local_78 = fVar3 * *(float *)(iVar5 + 0x44) + *(float *)(iVar5 + 0x38);
        local_84 = local_84 * *(float *)(pbVar2 + 0x28);
      }
      else {
        local_84 = local_84 / *(float *)(pbVar2 + 0x20);
        local_80 = local_84 * *(float *)(iVar5 + 0x3c) + *(float *)(iVar5 + 0x30);
        local_7c = local_84 * *(float *)(iVar5 + 0x40) + *(float *)(iVar5 + 0x34);
        local_78 = local_84 * *(float *)(iVar5 + 0x44) + *(float *)(iVar5 + 0x38);
      }
    }
    else {
      local_80 = *(float *)(iVar5 + 0x30);
      local_7c = *(float *)(iVar5 + 0x34);
      local_78 = *(float *)(iVar5 + 0x38);
      local_84 = *(float *)(pbVar2 + 0x18);
    }
    if ((*(int *)(iVar7 + 0x2c) == -1) || (iVar6 = object_try_and_get(0xffffffff), iVar6 == 0)) {
      local_74 = FUN_005013a0();
      if (local_74 == -1) {
        local_70 = 0xffff;
      }
      else {
        local_70 = *(undefined2 *)(local_74 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
      }
    }
    else {
      FUN_004f6b10();
    }
    FUN_00551f00(param_1,iVar7 + 0x10,&local_80,local_84);
    *(byte *)(iVar7 + 2) = *(byte *)(iVar7 + 2) | 4;
    return;
  }
  return;
}
#endif
