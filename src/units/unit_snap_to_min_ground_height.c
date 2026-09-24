// unit_snap_to_min_ground_height  (Ghidra: unit_snap_to_min_ground_height, renamed)
// address 0x55ecf0, size 469 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: Biped.jump_velocity (0x3b4, types/tags.h) used as the tag-defined minimum height;
//   unit_data.unknown_424 ("0..1 stun meter ... matg_stun_scale * this", types/units.h) matches
//   the controlling_player-gated scale here; unit_data.swarm_actor_index/actor_index
//   (0x1f8/0x1f4) and biped_data.unknown_504/unknown_4f0 (0x504/0x4f0) all match by offset.
// UNSURE: FUN_00417fa0's exact contract (an actor-notification call whose return doubles as this
//   function's early-exit value on failure); the globals_tag_data+0x174 player-info field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *globals_tag_data;   // 0x00746fa0, +0x174 player info (types/units.h)
extern uint8_t DAT_0087abc4;        // UNSURE global (cheat/debug toggle)
extern uint8_t unit_updates_suppressed; // 0x0071c419

extern uint32_t FUN_00417fa0(uint32_t object_index, float min_height); // UNSURE module
extern void unit_fire_animation_sound_trigger(uint32_t unit_index, uint32_t trigger_kind, int16_t contact_point_index);           // 0x560590, next batch

// Snaps a unit's velocity up to its tag-defined minimum ground height (Biped.jump_velocity,
// scaled down by the controlling player's stun meter and quadrupled under a debug toggle) if its
// upward velocity component has sunk below it, then marks the biped grounded and resets its
// target-lock and cached-look-at state.
uint32_t unit_snap_to_min_ground_height(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    uint32_t fallback = (object_index & 0xffff) * 3 & 0xffffff00;

    if ((biped->flags & 1) != 0 || biped->unknown_508 == 1) {
        return fallback;
    }

    {
        Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
        float min_height = tag->jump_velocity;
        real_vector3d velocity = obj->velocity;
        float upward_speed;
        datum_index actor_ref;

        if (unit->controlling_player != k_datum_index_none) {
            min_height = (1.0f - *(float *)(globals_tag_data + 0x174 + 0x84) * unit->unknown_424) * min_height;
        }
        if (DAT_0087abc4 != 0 && unit->controlling_player != k_datum_index_none) {
            min_height = min_height * 4.0f;
        }

        upward_speed = velocity.i * obj->up.i + velocity.k * obj->up.k + velocity.j * obj->up.j;
        if (upward_speed < min_height) {
            float delta = min_height - upward_speed;
            velocity.i += delta * obj->up.i;
            velocity.j += delta * obj->up.j;
            velocity.k += delta * obj->up.k;
        }

        actor_ref = unit->swarm_actor_index;
        if (actor_ref == k_datum_index_none) {
            actor_ref = unit->actor_index;
        }
        if (actor_ref != k_datum_index_none) {
            uint32_t result = FUN_00417fa0(object_index, min_height);
            if ((uint8_t)result == 0) {
                return result;
            }
        }

        obj->velocity = velocity;
        biped->flags |= 1;
        biped->unknown_504 = 0;
        biped->unknown_4f0 = k_datum_index_none;
        if (unit_updates_suppressed == 0) {
            // unit_index rides in EBX; Ghidra bound only the two stack arguments
            unit_fire_animation_sound_trigger(object_index, 4, 0);
            unit_fire_animation_sound_trigger(object_index, 4, 1);
        }
    }
    return fallback;
}

#if 0
Original Ghidra decompilation (0x55ecf0):

uint FUN_0055ecf0(uint param_1)

{
  uint *puVar1;
  float fVar2;
  char cVar3;
  uint uVar4;
  uint uVar5;
  undefined3 uVar6;
  undefined3 extraout_var;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  uVar5 = param_1;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  uVar4 = (param_1 & 0xffff) * 3 & 0xffffff00;
  if (((puVar1[0x133] & 1) == 0) && ((short)puVar1[0x142] != 1)) {
    local_10 = *(float *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x3b4);
    param_1._0_1_ = '\x01';
    if (puVar1[0x86] != 0xffffffff) {
      local_10 = (1.0 - *(float *)(*(int *)(DAT_00746fa0 + 0x174) + 0x84) * (float)puVar1[0x109]) *
                 local_10;
    }
    if ((DAT_0087abc4 != '\0') && (puVar1[0x86] != 0xffffffff)) {
      local_10 = local_10 * 4.0;
    }
    local_c = (float)puVar1[0x1a];
    local_8 = (float)puVar1[0x1b];
    local_4 = (float)puVar1[0x1c];
    fVar2 = local_c * (float)puVar1[0x20] +
            local_4 * (float)puVar1[0x22] + local_8 * (float)puVar1[0x21];
    if (fVar2 < local_10) {
      fVar2 = local_10 - fVar2;
      local_c = fVar2 * (float)puVar1[0x20] + local_c;
      local_8 = fVar2 * (float)puVar1[0x21] + local_8;
      local_4 = fVar2 * (float)puVar1[0x22] + local_4;
    }
    uVar4 = puVar1[0x7e];
    if (uVar4 == 0xffffffff) {
      uVar4 = puVar1[0x7d];
    }
    if (uVar4 != 0xffffffff) {
      uVar5 = FUN_00417fa0(uVar5,local_10);
      param_1._0_1_ = (char)uVar5;
      if ((char)param_1 == '\0') {
        return uVar5;
      }
    }
    puVar1[0x1a] = (uint)local_c;
    cVar3 = DAT_0071c419;
    uVar6 = (undefined3)((uint)local_c >> 8);
    puVar1[0x1b] = (uint)local_8;
    puVar1[0x1c] = (uint)local_4;
    puVar1[0x133] = puVar1[0x133] | 1;
    *(undefined1 *)(puVar1 + 0x141) = 0;
    puVar1[0x136] = 0xffffffff;
    if (cVar3 == '\0') {
      FUN_00560590(4,0);
      FUN_00560590(4,1);
      uVar6 = extraout_var;
    }
    uVar4 = CONCAT31(uVar6,(char)param_1);
  }
  return uVar4;
}
#endif
