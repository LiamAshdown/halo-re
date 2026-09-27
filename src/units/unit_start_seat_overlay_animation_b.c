// unit_start_seat_overlay_animation_b  (Ghidra: unit_start_seat_overlay_animation_b)
// address 0x566410, size 273 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.35
// evidence: types/units.h unit_data.overlays[1] (0x2ae), .unknown_2a5 (0x2a5),
//   .animation_state (0x2a3), .animation_definition_index/.animation_weapon_index/
//   .animation_weapon_type_index (0x2a0/0x2a1/0x2a2); types/tags.h
//   ModelAnimationsAnimationGraphWeaponType (animations TagReflexive at 0x30) -- same chain as
//   unit_start_seat_overlay_animation_a (0x565e00), but this one only ever resolves through the
//   weapon-type table.
// register convention: unit index in EAX, overlay command in an unresolved register
//   (unaff_BX -- no caller in this batch, modelled as an explicit parameter).
//   // blam-cc: in_EAX -> unit_index, unaff_BX -> command

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, EAX graph, DX animation, stack

void unit_start_seat_overlay_animation_b(uint32_t unit_index, int16_t command) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (unit->unknown_2a5 > command) {
        return;
    }

    switch (unit->animation_state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1d: case 0x1e: case 0x1f:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        return; // uninterruptible/special-move states; same set as unit_state_is_scripted_animation
    default:
        break;
    }

    Object *obj_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    void *graph = tag_instances[obj_tag->animation_graph.tag_id.index].data;
    uint8_t *unit_block = *(uint8_t **)((uint8_t *)graph + 0x10);
    ModelAnimationsAnimationGraphUnitSeat *unit_seat =
        (ModelAnimationsAnimationGraphUnitSeat *)(unit_block + unit->animation_definition_index * 100);
    ModelAnimationsAnimationGraphWeapon *weapon_anim =
        (ModelAnimationsAnimationGraphWeapon *)((uint8_t *)unit_seat->weapons.pointer +
                                                 unit->animation_weapon_index * 0xbc);
    ModelAnimationsAnimationGraphWeaponType *weapon_type =
        (ModelAnimationsAnimationGraphWeaponType *)((uint8_t *)weapon_anim->weapon_types.pointer +
                                                      unit->animation_weapon_type_index * 0x3c);

    int16_t raw_index;
    switch (command) {
    case 1: raw_index = 4; break;
    case 2: raw_index = 5; break;
    case 3: raw_index = 6; break;
    case 4: raw_index = 7; break;
    case 5: raw_index = 2; break;
    case 6: raw_index = 3; break;
    default: return;
    }

    if (raw_index < (int32_t)weapon_type->animations.count &&
        *(int16_t *)((uint8_t *)weapon_type->animations.pointer + raw_index * 2) != -1) {
        unit->overlays[1].animation_index = animation_choose_random_permutation(                // 0x5664fd
            *(datum_index *)&obj_tag->animation_graph.tag_id,
            *(int16_t *)((uint8_t *)weapon_type->animations.pointer + raw_index * 2), 1);
        unit->overlays[1].frame = 0;
        unit->unknown_2a5 = (int8_t)command;
    }
}

#if 0
Original Ghidra decompilation (0x566410):

void FUN_00566410(void)

{
  uint *puVar1;
  undefined2 uVar2;
  uint in_EAX;
  int iVar3;
  short sVar4;
  short unaff_BX;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (*(char *)((int)puVar1 + 0x2a5) <= unaff_BX) {
    switch(*(undefined1 *)((int)puVar1 + 0x2a3)) {
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1d:
    case 0x1e:
    case 0x1f:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x27:
    case 0x29:
      break;
    default:
      iVar3 = *(char *)((int)puVar1 + 0x2a2) * 0x3c +
              *(int *)(*(int *)(*(int *)(*(int *)((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 +
                                                                      0x14 + DAT_0087bc14) + 0x44) &
                                                  0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x10) +
                                0x5c + (char)puVar1[0xa8] * 100) + 0xb4 +
                      *(char *)((int)puVar1 + 0x2a1) * 0xbc);
      switch(unaff_BX) {
      case 1:
        sVar4 = 4;
        break;
      case 2:
        sVar4 = 5;
        break;
      case 3:
        sVar4 = 6;
        break;
      case 4:
        sVar4 = 7;
        break;
      case 5:
        sVar4 = 2;
        break;
      case 6:
        sVar4 = 3;
        break;
      default:
        goto switchD_0056644d_caseD_17;
      }
      if (((int)sVar4 < *(int *)(iVar3 + 0x30)) &&
         (*(short *)(*(int *)(iVar3 + 0x34) + sVar4 * 2) != -1)) {
        uVar2 = FUN_004d6280(1);
        *(undefined2 *)((int)puVar1 + 0x2ae) = uVar2;
        *(undefined2 *)(puVar1 + 0xac) = 0;
        *(char *)((int)puVar1 + 0x2a5) = (char)unaff_BX;
      }
    }
  }
switchD_0056644d_caseD_17:
  return;
}
#endif
