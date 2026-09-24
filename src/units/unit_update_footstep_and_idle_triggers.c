// unit_update_footstep_and_idle_triggers  (Ghidra: unit_update_footstep_and_idle_triggers)
// address 0x560410, size 359 bytes
// name confidence: 0.3 (renamed from the phase2 candidate "unit_update_vehicle_seat_reaction";
//   see UNSURE below)   rewrite confidence: 0.35
// evidence: types/objects.h object.animation_graph/animation_index/animation_frame (0xcc/0xd0/
//   0xd2); types/tags.h ModelAnimationsAnimation (stride 0xb4, left_foot_frame_index at +0x40,
//   right_foot_frame_index at +0x41); types/units.h unit_data.animation_state (0x2a3),
//   .throttle (0x278), .base_animation_state (0x2a7); biped_data.movement_state (0x4d2) and
//   unknown_503 (0x503).
// register convention: object index in EAX.
//   // blam-cc: in_EAX -> unit_index
// UNSURE: out/phase4/units_types_notes.md documents biped_data.unknown_503 as "the latch 0x560410
//   toggles at the seat angle limit", but the code read here never touches a seat or turret
//   field -- it counts ticks of biped_data.movement_state == 0 (standing) up to 4 and then fires
//   unit_fire_animation_sound_trigger(3, 0) and (3, 1). The functions.md summary ("seat/turret
//   angle reaches its limits") looks equally unsupported by this decompilation; the two trigger
//   calls with a foot index (0/1) that gate on the Biped tag's left/right foot frame numbers are
//   unambiguous footstep events, so that part of the name is solid. The "idle" half (movement_state
//   == 0, counting to 4 ticks) is kept under its literal behaviour rather than either prior guess.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void unit_fire_animation_sound_trigger(uint32_t unit_index, uint32_t trigger_kind, int16_t contact_point_index); // 0x560590

void unit_update_footstep_and_idle_triggers(uint32_t unit_index) // blam-cc: in_EAX -> unit_index
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    int32_t is_turning = 0;
    int32_t is_moving_fast = 0;

    switch (unit->animation_state) {
    case 2:
    case 3:
        is_turning = 1;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        if (0.25f < unit->throttle.k * unit->throttle.k + unit->throttle.j * unit->throttle.j +
                        unit->throttle.i * unit->throttle.i) {
            is_moving_fast = 1;
        }
        break;
    default:
        break;
    }

    if (obj->animation_index != -1) {
        void *graph = tag_instances[obj->animation_graph & 0xffff].data;
        uint8_t *animations = *(uint8_t **)((uint8_t *)graph + 0x78);
        ModelAnimationsAnimation *anim = (ModelAnimationsAnimation *)(animations + obj->animation_index * 0xb4);

        if (is_turning) {
            if (obj->animation_frame == 0) {
                unit_fire_animation_sound_trigger(unit_index, 3, 0);
                unit_fire_animation_sound_trigger(unit_index, 3, 1);
            }
        } else if (is_moving_fast &&
                   (anim->left_foot_frame_index != 0 || anim->right_foot_frame_index != 0)) {
            int16_t foot;
            if ((uint16_t)obj->animation_frame == (uint16_t)(uint8_t)anim->left_foot_frame_index) {
                foot = 0;
            } else if ((uint16_t)obj->animation_frame != (uint16_t)(uint8_t)anim->right_foot_frame_index) {
                goto idle_timeout;
            } else {
                foot = 1;
            }
            unit_fire_animation_sound_trigger(unit_index, unit->base_animation_state == 2, foot);
        }
    }

idle_timeout:
    if (biped->movement_state == 0) {
        if (biped->unknown_503 < 1) {
            return;
        }
        biped->unknown_503 = biped->unknown_503 + 1;
        if (biped->unknown_503 < 4) {
            return;
        }
        unit_fire_animation_sound_trigger(unit_index, 3, 0);
        unit_fire_animation_sound_trigger(unit_index, 3, 1);
    } else if (biped->movement_state == 1) {
        biped->unknown_503 = 1;
        return;
    } else {
        biped->unknown_503 = 0;
    }
}

#if 0
Original Ghidra decompilation (0x560410):

void FUN_00560410(void)

{
  int iVar1;
  bool bVar2;
  bool bVar3;
  undefined1 uVar4;
  char cVar5;
  uint in_EAX;
  int iVar6;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  bVar3 = false;
  bVar2 = false;
  switch(*(undefined1 *)(iVar1 + 0x2a3)) {
  case 2:
  case 3:
    bVar2 = true;
    break;
  case 4:
  case 5:
  case 6:
  case 7:
    if (0.25 < *(float *)(iVar1 + 0x280) * *(float *)(iVar1 + 0x280) +
               *(float *)(iVar1 + 0x27c) * *(float *)(iVar1 + 0x27c) +
               *(float *)(iVar1 + 0x278) * *(float *)(iVar1 + 0x278)) {
      bVar3 = true;
    }
  }
  if (*(short *)(iVar1 + 0xd0) != -1) {
    iVar6 = *(short *)(iVar1 + 0xd0) * 0xb4 +
            *(int *)(*(int *)((*(uint *)(iVar1 + 0xcc) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                    0x78);
    if (bVar2) {
      if (*(short *)(iVar1 + 0xd2) == 0) {
        FUN_00560590(3,0);
        FUN_00560590(3,1);
      }
    }
    else if ((bVar3) && ((*(byte *)(iVar6 + 0x40) != 0 || (*(char *)(iVar6 + 0x41) != '\0')))) {
      if (*(ushort *)(iVar1 + 0xd2) == (ushort)*(byte *)(iVar6 + 0x40)) {
        uVar4 = 0;
      }
      else {
        if (*(ushort *)(iVar1 + 0xd2) != (ushort)*(byte *)(iVar6 + 0x41)) goto LAB_00560529;
        uVar4 = 1;
      }
      FUN_00560590(*(char *)(iVar1 + 0x2a7) == '\x02',uVar4);
    }
  }
LAB_00560529:
  if (*(char *)(iVar1 + 0x4d2) == '\0') {
    if (*(char *)(iVar1 + 0x503) < '\x01') {
      return;
    }
    cVar5 = *(char *)(iVar1 + 0x503) + '\x01';
    *(char *)(iVar1 + 0x503) = cVar5;
    if (cVar5 < '\x04') {
      return;
    }
    FUN_00560590(3,0);
    FUN_00560590(3,1);
  }
  else if (*(char *)(iVar1 + 0x4d2) == '\x01') {
    *(undefined1 *)(iVar1 + 0x503) = 1;
    return;
  }
  *(undefined1 *)(iVar1 + 0x503) = 0;
  return;
}
#endif
