// unit_find_weapon_marker_transform  (Ghidra: FUN_005640a0; it finds a vehicle seat's entry points)
// address 0x5640a0, size 533 bytes
// name confidence: 0.2   rewrite confidence: 0.85
// REWRITTEN from objdump 0x5640a0..0x5642b4 (the draft swapped the unit and the vehicle, dropped the entering
//   unit and returned placeholder positions). EAX: the unit about to enter; stack (vehicle, seat, out_entry,
//   out_seat, out_hint). The unit's animation graph (tag +0x44) must have a units block whose label matches the
//   seat's label (_stricmp, 0x64 each) with more than 7 animations and an "enter" animation (slot 7); otherwise 0.
//   The seat marker (seat +0x24) of the vehicle gives the seat's world transform; frame 0 of the enter animation
//   (0x4d4a80 on the unit's model) placed on it gives the world point the unit starts entering from. The optional
//   outputs are that entry point, the seat marker's world position and the world position of the marker named
//   "<seat marker> enter-hint".
// blam-cc: EAX -> unit_index, stack -> (vehicle_index, seat_index, out_entry, out_seat, out_hint)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "models.h"
#include <string.h>

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern void animation_get_frame_orientations(ModelAnimationsAnimation *animation, GBXModel *model,
    int16_t frame, real_orientation *out_orientations); // 0x4d4a80, EAX model, EDI animation, stack
extern void matrix4x3_from_quaternion(real_quaternion *q, real_matrix4x3 *out); // 0x4cbad0, ECX q, EDX out
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0 (via 0x696664)

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

uint8_t unit_find_weapon_marker_transform(uint32_t unit_index, uint32_t vehicle_index, int16_t seat_index,
    real_point3d *out_entry, real_point3d *out_seat, real_point3d *out_hint)
{
    uint8_t *unit_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(unit_index));
    uint8_t *model = TAG_DATA(*(datum_index *)(unit_tag + 0x34));
    uint8_t *graph = TAG_DATA(*(datum_index *)(unit_tag + 0x44));
    uint8_t *seat = *(uint8_t **)(TAG_DATA(*(datum_index *)OBJECT_DATA(vehicle_index)) + 0x2e8) + seat_index * 0x11c;
    uint8_t *block = 0;
    int16_t i;
    int16_t enter_animation;
    ModelAnimationsAnimation *animation;
    object_marker seat_marker;
    object_marker hint_marker;
    real_orientation orientations[k_maximum_nodes_per_model];
    real_matrix4x3 root;
    real_matrix4x3 entry;
    char hint_name[0x100];

    for (i = 0; i < *(int32_t *)(graph + 0xc); i++) {
        if (__stricmp((char *)(*(uint8_t **)(graph + 0x10) + i * 0x64), (char *)(seat + 0x4)) == 0) {
            block = *(uint8_t **)(graph + 0x10) + i * 0x64;
            break;
        }
    }
    if (block == 0 || *(int32_t *)(block + 0x40) <= 7) {
        return 0;
    }
    enter_animation = (*(int16_t **)(block + 0x44))[7];
    if (enter_animation == -1) {
        return 0;
    }
    animation = (ModelAnimationsAnimation *)(*(uint8_t **)(graph + 0x78) + enter_animation * 0xb4);
    object_get_node_local_transform(vehicle_index, (char *)(seat + 0x24), &seat_marker, 1);
    animation_get_frame_orientations(animation, (GBXModel *)model, 0, orientations);
    matrix4x3_from_quaternion(&orientations[0].rotation, &root);
    root.position = orientations[0].translation;
    matrix4x3_multiply(&seat_marker.node_transform, &root, &entry);
    strcpy(hint_name, (char *)(seat + 0x24));
    strcat(hint_name, " enter-hint");
    object_get_node_local_transform(vehicle_index, hint_name, &hint_marker, 1);
    if (out_seat != 0) {
        *out_seat = seat_marker.node_transform.position;
    }
    if (out_entry != 0) {
        *out_entry = entry.position;
    }
    if (out_hint != 0) {
        *out_hint = hint_marker.node_transform.position;
    }
    return 1;
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
