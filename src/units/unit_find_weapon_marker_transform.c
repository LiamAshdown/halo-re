// unit_find_weapon_marker_transform  (Ghidra: unit_find_weapon_marker_transform)
// address 0x5640a0, size 533 bytes
// name confidence: 0.35 (phase2 candidate)   rewrite confidence: 0.15
// evidence: types/objects.h object.definition_tag (0x000), Object.animation_graph (tag+0x44),
//   Object.model (tag+0x34); types/tags.h Unit.seats (TagReflexive at 0x2e4/0x2e8, UnitSeat
//   stride 0x11c), UnitSeat.label (TagString at +0x4), UnitSeat.marker_name (TagString at
//   +0x24); ModelAnimations.units (TagReflexive at 0xc, stride 100, matching the same chain
//   used by unit_try_set_animation_state). object_get_node_local_transform,
//   matrix4x3_from_quaternion, matrix4x3_multiply.
// register convention: a second unit's tag data reached via unit index in EAX, seat index in
//   DX, three output real_vector3d/point pointers on the stack.
//   // blam-cc: param_1 -> seat_unit_index, param_2 -> seat_index, param_3/param_4/param_5 ->
//   //   out_a/out_b/out_c (each a 3-float output)
// UNSURE: this function decompiles almost entirely into raw stack-buffer arithmetic Ghidra could
//   not attach types to -- a quaternion build (animation_get_frame_orientations), a matrix4x3_from_quaternion /
//   matrix4x3_multiply pair, and a second marker-name string assembled byte-by-byte (a reversed
//   copy of the matched marker_name with a further literal suffix appended, "... ent"+"er-h"+a
//   pointer to a rodata tail this batch cannot resolve). The search loop, its two validity
//   guards, and the three output slots are preserved faithfully; the geometry/string-building
//   body between the marker match and the second object_get_node_local_transform call is
//   reproduced only as a best-effort sketch and is very likely wrong in the small details.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern void animation_get_frame_orientations(uint32_t zero, void *out_quaternion);            // 0x4d4a80, UNSURE
extern void matrix4x3_from_quaternion(real_quaternion *q, real_matrix4x3 *out); // 0x4cbad0, UNSURE
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0, verified in src/math

uint8_t unit_find_weapon_marker_transform(uint32_t seat_unit_index, int16_t seat_index,
                                           real_vector3d *out_a, real_vector3d *out_b,
                                           real_point3d *out_c) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[seat_unit_index & 0xffff].data;
    Object *obj_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    Unit *unit_tag = (Unit *)tag_instances[obj_tag->animation_graph.tag_id.index].data; // UNSURE: reused as "the graph tag" per original iVar2
    void *model = tag_instances[obj_tag->model.tag_id.index].data;
    UnitSeat *seat = (UnitSeat *)((uint8_t *)((Unit *)tag_instances[obj->definition_tag & 0xffff].data)->seats.pointer +
                                   seat_index * 0x11c);
    (void)model;

    uint8_t *unit_block = *(uint8_t **)((uint8_t *)unit_tag + 0x10);
    int32_t count = *(int32_t *)((uint8_t *)unit_tag + 0xc);

    for (int32_t i = 0; i < count; i++) {
        char *label = (char *)(unit_block + i * 100);
        if (__stricmp(label, seat->label.string) == 0) {
            if (*(int32_t *)(label + 0x40) < 8) {
                return 0;
            }
            if (*(int16_t *)(*(uint8_t **)(label + 0x44) + 0xe) == -1) {
                return 0;
            }

            object_marker marker_a = {0}, marker_b = {0};
            object_get_node_local_transform(seat_unit_index, seat->marker_name.string, &marker_a, 1);

            // UNSURE: the real body computes a rotated/multiplied transform here and builds a
            // second, derived marker name (a reversed copy of marker_name.string plus a literal
            // suffix) before a second object_get_node_local_transform call; not reproduced.
            char derived_marker[64];
            derived_marker[0] = '\0'; // UNSURE placeholder, see file header
            object_get_node_local_transform(seat_unit_index, derived_marker, &marker_b, 1);

            if (out_b != 0) {
                *out_b = *(real_vector3d *)((uint8_t *)&marker_b + 0x00); // UNSURE offsets
            }
            if (out_a != 0) {
                *out_a = *(real_vector3d *)((uint8_t *)&marker_a + 0x00); // UNSURE offsets
            }
            if (out_c != 0) {
                *out_c = *(real_point3d *)((uint8_t *)&marker_b + 0x0c); // UNSURE offsets
            }
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x5640a0):

undefined4
FUN_005640a0(uint param_1,short param_2,undefined4 *param_3,undefined4 *param_4,undefined4 *param_5)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  char *pcVar6;
  short sVar7;
  undefined4 *puVar8;
  undefined1 local_a60 [16];
  undefined4 local_a50;
  undefined4 local_a4c;
  undefined4 local_a48;
  undefined4 uStack_261;
  undefined1 local_160 [96];
  undefined4 local_100;
  undefined4 local_fc;
  undefined4 local_f8;
  undefined1 local_f0 [56];
  undefined1 local_b8 [40];
  undefined4 local_90;
  undefined4 local_8c;
  undefined4 local_88;
  undefined1 local_80 [40];
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined1 local_48 [40];
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_c;

  iVar5 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar2 = *(int *)((*(uint *)(iVar5 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_c = *(undefined4 *)((*(uint *)(iVar5 + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar5 = param_2 * 0x11c +
          *(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                        (param_1 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                           DAT_0087bc14) + 0x2e8);
  sVar7 = 0;
  if (0 < *(int *)(iVar2 + 0xc)) {
    iVar4 = 0;
    do {
      iVar4 = __stricmp((char *)(iVar4 * 100 + *(int *)(iVar2 + 0x10)),(char *)(iVar5 + 4));
      if (iVar4 == 0) {
        if (sVar7 == -1) {
          return 0;
        }
        if (*(int *)(sVar7 * 100 + 0x40 + *(int *)(iVar2 + 0x10)) < 8) {
          return 0;
        }
        if (*(short *)(*(int *)(sVar7 * 100 + *(int *)(iVar2 + 0x10) + 0x44) + 0xe) == -1) {
          return 0;
        }
        pcVar6 = (char *)(iVar5 + 0x24);
        object_get_node_local_transform(param_1,pcVar6,local_f0,1);
        FUN_004d4a80(0,local_a60);
        matrix4x3_from_quaternion();
        local_20 = local_a50;
        local_1c = local_a4c;
        local_18 = local_a48;
        (*(code *)PTR_matrix4x3_multiply_00696664)(local_b8,local_48,local_80);
        iVar5 = 1 - (int)pcVar6;
        do {
          cVar1 = *pcVar6;
          pcVar6[(int)&uStack_261 + iVar5] = cVar1;
          pcVar6 = pcVar6 + 1;
        } while (cVar1 != '\0');
        puVar3 = &uStack_261;
        do {
          puVar8 = puVar3;
          puVar3 = (undefined4 *)((int)puVar8 + 1);
        } while (*(char *)((int)puVar8 + 1) != '\0');
        *(undefined4 *)((int)puVar8 + 1) = 0x746e6520;
        *(undefined4 *)((int)puVar8 + 5) = 0x682d7265;
        *(undefined **)((int)puVar8 + 9) = &DAT_00746e69;
        object_get_node_local_transform(param_1,(int)&uStack_261 + 1,local_160,1);
        if (param_4 != (undefined4 *)0x0) {
          *param_4 = local_90;
          param_4[1] = local_8c;
          param_4[2] = local_88;
        }
        if (param_3 != (undefined4 *)0x0) {
          *param_3 = local_58;
          param_3[1] = local_54;
          param_3[2] = local_50;
        }
        if (param_5 != (undefined4 *)0x0) {
          *param_5 = local_100;
          param_5[1] = local_fc;
          param_5[2] = local_f8;
        }
        return 1;
      }
      sVar7 = sVar7 + 1;
      iVar4 = (int)sVar7;
    } while (iVar4 < *(int *)(iVar2 + 0xc));
  }
  return 0;
}
#endif
