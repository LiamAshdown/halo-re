// unit_snap_to_min_ground_height  (Ghidra: unit_snap_to_min_ground_height, renamed)
// address 0x55ecf0, size 469 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// evidence: Biped.jump_velocity (0x3b4, types/tags.h) used as the tag-defined minimum height;
//   unit_data.unknown_424 ("0..1 stun meter ... matg_stun_scale * this", types/units.h) matches
//   the controlling_player-gated scale here; unit_data.swarm_actor_index/actor_index
//   (0x1f8/0x1f4) and biped_data.unknown_504/unknown_4f0 (0x504/0x4f0) all match by offset.
// UNSURE: FUN_00417fa0's exact contract (an actor-notification call whose return doubles as this
//   function's early-exit value on failure); the global_globals+0x174 player-info field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern Globals *global_globals;
extern uint8_t cheat_super_jump;    // 0x0087abc4
extern uint8_t unit_updates_suppressed; // 0x0071c419

extern uint8_t actor_get_requested_velocity(uint8_t skip_clamp, datum_index actor_index, real_vector3d *out_velocity,
    uint32_t object_index, float speed_limit); // 0x417fa0, AL, ECX, EDX, stack
extern void unit_fire_animation_sound_trigger(uint32_t unit_index, uint32_t trigger_kind, int16_t contact_point_index); // 0x560590, EBX, stack

// REWRITTEN from objdump 0x55ecf0..0x55eec4 (really the biped jump launch). Unless already airborne (+0x4cc bit 0)
//   or +0x508 == 1: the Biped tag's jump speed (+0x3b4; a player's scaled by 1 - globals player info +0x84 * stun
//   +0x424, times 4 with the super-jump cheat) is the least speed along the unit's up; an AI unit (+0x1f8, else
//   +0x1f4) then takes its actor's requested velocity capped at that speed (0x417fa0, unclamped in states 0x27 /
//   0x28) and may refuse. The velocity is committed, the unit goes airborne (+0x504 = 0, +0x4d8 = -1) and both
//   jump sound triggers fire. The draft called the velocity request with 2 of 5 operands.
uint32_t unit_snap_to_min_ground_height(uint32_t object_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;   // esi
    float jump_speed;                                                                           // [esp+0x10]
    real_vector3d velocity;                                                                     // [esp+0x14]
    real_vector3d *up = (real_vector3d *)(obj + 0x80);
    float up_speed;
    datum_index actor_index;
    uint8_t result = 1;                                                                         // the argument slot

    if ((obj[0x4cc] & 1) || *(int16_t *)(obj + 0x508) == 1) {
        return 0;
    }
    jump_speed = *(float *)((uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data + 0x3b4);
    if (((unit_object *)obj)->unit.controlling_player != k_datum_index_none) {
        jump_speed = (1.0f - *(float *)((uint8_t *)global_globals->player_information.pointer + 0x84) * ((struct unit_object *)obj)->unit.stun) *
            jump_speed;
    }
    if (cheat_super_jump && ((unit_object *)obj)->unit.controlling_player != k_datum_index_none) {
        jump_speed = jump_speed * 4.0f;
    }
    velocity = *(real_vector3d *)&((unit_object *)obj)->base.velocity.i;
    up_speed = velocity.j * up->j + velocity.k * up->k + velocity.i * up->i;
    if (!(up_speed >= jump_speed)) {
        float delta = jump_speed - up_speed;

        velocity.i = delta * up->i + velocity.i;
        velocity.j = delta * up->j + velocity.j;
        velocity.k = delta * up->k + velocity.k;
    }
    actor_index = ((unit_object *)obj)->unit.swarm_actor_index;
    if (actor_index == k_datum_index_none) {
        actor_index = ((unit_object *)obj)->unit.actor_index;
    }
    if (actor_index != k_datum_index_none) {
        uint8_t skip_clamp = (obj[0x2a3] == 0x27 || obj[0x2a3] == 0x28) ? 1 : 0;

        result = actor_get_requested_velocity(skip_clamp, actor_index, &velocity, object_index, jump_speed);
        if (!result) {
            return 0;
        }
    }
    *(real_vector3d *)&((unit_object *)obj)->base.velocity.i = velocity;
    *(uint32_t *)(obj + 0x4cc) |= 1;
    obj[0x504] = 0;
    *(int32_t *)(obj + 0x4d8) = -1;
    if (!unit_updates_suppressed) {
        unit_fire_animation_sound_trigger(object_index, 4, 0);
        unit_fire_animation_sound_trigger(object_index, 4, 1);
    }
    return result;
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
