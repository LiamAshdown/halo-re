// actor_resolve_look_target  (Ghidra: actor_resolve_look_target, renamed)
// address 0x414d00, size 580 bytes
// name confidence: 0.35  rewrite confidence: 0.9 (VERIFIED against objdump 0x414d00..0x414f43; record target and horizontal aim FIXED)
// evidence: the only caller (0x415480, the module's central look/aim update, actor_update_look_target) passes &actor.position_cache_a or &actor.position_cache_b as the register
// vector argument; this function either accepts the recognized prop actor_select_facing_
// target_prop finds, or falls back to that caller-supplied direction (normalized, or the
// global forward vector if it is degenerate), and in both cases hands the result to
// actor_look_pick_random_point_in_cone to pick an actual look point, then arms the wait
// timer via actor_look_get_wait_ticks.
// register convention: reconstructed from objdump -d -M intel over 0x414d00..0x414f3f and
// its one call site at 0x415c89..0x415cc1. preferred_direction in EAX; actor_index,
// deviation_table, require_trust, use_aiming_deviation and force_fallback are five genuine
// stack parameters (Ghidra recognized all five correctly here).
// blam-cc: EAX -> preferred_direction, stack -> actor_index, stack -> deviation_table,
//   stack -> require_trust, stack -> use_aiming_deviation, stack -> force_fallback
// RESOLVED (was UNSURE "param_2 is never read"): 0x414ef5 does mov edi,[esp+0x30], which with
// this frame (sub esp,0x18 plus four pushes) is the SECOND stack parameter, and EDI is exactly
// the implicit deviation-table argument of actor_look_get_wait_ticks. So param_2 is the float
// pair table, forwarded on; the one caller already passes its idle_range table for it.
// VERIFIED against disassembly 0x414d00..0x414f43 (2026-09-30): all field offsets, both min() selections, the pi
// fallback (require_trust), the horizontal aim direction and the 6-slot push sequence for the cone call agree with the C.
// actor_look_get_wait_ticks's second argument is a uint32 whose only live bit is its low byte (out_in_front).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

// actor_recognition_scan_result now lives in types/ai.h (folded from this file).

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern const real_vector3d *global_forward3d_pointer; // 0x00696718

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern uint8_t actor_select_facing_target_prop(datum_index actor_index, uint8_t require_trust, uint8_t skip_lane_test,
                                                actor_recognition_scan_result *out_result, uint8_t *out_in_front); // 0x414a90, this module
extern uint8_t actor_look_pick_random_point_in_cone(void *origin, float yaw_min, float yaw_max, float pitch_min, float pitch_max, real_vector3d *base_direction, uint8_t check_obstruction, real_point3d *out); // 0x415260, this module,
                                                     // blam-cc: stack x6, ESI -> base_direction, BL -> check_obstruction
extern int32_t actor_look_get_wait_ticks(datum_index actor_index, int16_t mode, uint32_t flags, float *deviation_table); // 0x415150, EAX, stack, EDI
                                          // blam-cc: stack -> mode, stack -> flags, EDI -> deviation_table

// blam-cc: EAX -> preferred_direction, stack -> actor_index, stack -> deviation_table,
//   stack -> require_trust, stack -> use_aiming_deviation, stack -> force_fallback
uint8_t actor_resolve_look_target(real_point3d *preferred_direction, datum_index actor_index, float *deviation_table,
                                   uint8_t require_trust, uint8_t use_aiming_deviation, uint8_t force_fallback)
{
    actor *self;
    Actor *definition;
    uint32_t out_in_front;
    real_vector3d direction;
    float yaw_half;
    float pitch_half;
    float pitch_center;
    int32_t wait_ticks;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    out_in_front = 0;
    self->idle_major_active = 0;

    if (force_fallback ||
        actor_select_facing_target_prop(actor_index, require_trust, use_aiming_deviation,
            (actor_recognition_scan_result *)((uint8_t *)self + 0x56c), (uint8_t *)&out_in_front) == 0) {
        // FIXED (0x414d4f..0x414d67): the facing target is written into the actor's look record +0x56c
        direction.i = preferred_direction->x;
        direction.j = preferred_direction->y;
        direction.k = preferred_direction->z;

        if (use_aiming_deviation == 0) {
            yaw_half = (definition->maximum_looking_deviation.yaw <= definition->idle_looking_range.yaw)
                           ? definition->maximum_looking_deviation.yaw
                           : definition->idle_looking_range.yaw;
            pitch_half = (definition->maximum_looking_deviation.pitch <= definition->idle_looking_range.pitch)
                             ? definition->maximum_looking_deviation.pitch
                             : definition->idle_looking_range.pitch;
        } else {
            if (require_trust == 0) {
                yaw_half = (definition->maximum_aiming_deviation.yaw <= definition->idle_aiming_range.yaw)
                               ? definition->maximum_aiming_deviation.yaw
                               : definition->idle_aiming_range.yaw;
            } else {
                yaw_half = 3.1415927f;
            }
            pitch_half = (definition->maximum_aiming_deviation.pitch <= definition->idle_aiming_range.pitch)
                              ? definition->maximum_aiming_deviation.pitch
                              : definition->idle_aiming_range.pitch;

            direction.k = 0.0f; // FIXED (0x414dff): the aim search is centred on the HORIZONTAL direction
            if (vector3d_normalize_with_length(&direction) == 0.0f) {
                direction = *global_forward3d_pointer;
            }
        }

        pitch_center = -pitch_half;
        if (self->vehicle_gunner != 0) {
            pitch_center = pitch_center * 0.5f;
        }

        self->idle_major_direction_type = 4;
        if (!actor_look_pick_random_point_in_cone(&self->aim_origin, -yaw_half, yaw_half, pitch_center, pitch_half,
                                                    &direction, 1, &self->idle_major_point)) {
            return (uint8_t)out_in_front;
        }
    }

    wait_ticks = actor_look_get_wait_ticks(actor_index, (use_aiming_deviation == 0) + 1, out_in_front, deviation_table);
    self->idle_major_timer = wait_ticks;
    if (wait_ticks == 0) {
        return (uint8_t)out_in_front;
    }
    self->idle_major_active = 1;
    self->idle_major_is_aiming = (int8_t)use_aiming_deviation;
    return (uint8_t)out_in_front;
}

#if 0
Original Ghidra decompilation (0x414d00):

uint FUN_00414d00(uint param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,char param_5
                 )

{
  char cVar1;
  undefined4 *in_EAX;
  int iVar2;
  int iVar3;
  float10 fVar4;
  float local_18;
  uint local_14;
  float local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar3 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar2 = *(int *)((*(uint *)(iVar3 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_14 = local_14 & 0xffffff00;
  *(undefined1 *)(iVar3 + 0x55c) = 0;
  if ((param_5 != '\0') ||
     (cVar1 = FUN_00414a90(param_3,param_4,iVar3 + 0x56c,&local_14), cVar1 == '\0')) {
    local_4 = in_EAX[2];
    local_c = *in_EAX;
    local_8 = in_EAX[1];
    if ((char)param_4 == '\0') {
      if (*(float *)(iVar2 + 0xac) <= *(float *)(iVar2 + 0xcc)) {
        _param_5 = *(float *)(iVar2 + 0xac);
      }
      else {
        _param_5 = *(float *)(iVar2 + 0xcc);
      }
      if (*(float *)(iVar2 + 0xb0) <= *(float *)(iVar2 + 0xd0)) {
        local_18 = *(float *)(iVar2 + 0xb0);
      }
      else {
        local_18 = *(float *)(iVar2 + 0xd0);
      }
    }
    else {
      if ((char)param_3 == '\0') {
        if (*(float *)(iVar2 + 0xa4) <= *(float *)(iVar2 + 0xc4)) {
          _param_5 = *(float *)(iVar2 + 0xa4);
        }
        else {
          _param_5 = *(float *)(iVar2 + 0xc4);
        }
      }
      else {
        _param_5 = 3.1415927;
      }
      if (*(float *)(iVar2 + 0xa8) <= *(float *)(iVar2 + 200)) {
        local_18 = *(float *)(iVar2 + 0xa8);
      }
      else {
        local_18 = *(float *)(iVar2 + 200);
      }
      local_4 = 0;
      fVar4 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 == fVar4) {
        local_c = *(undefined4 *)PTR_DAT_00696718;
        local_8 = *(undefined4 *)(PTR_DAT_00696718 + 4);
        local_4 = *(undefined4 *)(PTR_DAT_00696718 + 8);
      }
    }
    local_10 = -local_18;
    if (*(char *)(iVar3 + 0x161) != '\0') {
      local_10 = local_10 * 0.5;
    }
    *(undefined2 *)(iVar3 + 0x56c) = 4;
    cVar1 = actor_look_pick_random_point_in_cone
                      (iVar3 + 0x120,-_param_5,_param_5,local_10,local_18,iVar3 + 0x570);
    if (cVar1 == '\0') {
      return local_14 & 0xff;
    }
  }
  iVar2 = actor_look_get_wait_ticks(((char)param_4 == '\0') + '\x01',local_14);
  *(int *)(iVar3 + 0x564) = iVar2;
  if (iVar2 == 0) {
    return local_14 & 0xff;
  }
  *(undefined1 *)(iVar3 + 0x55c) = 1;
  *(char *)(iVar3 + 0x55d) = (char)param_4;
  return local_14 & 0xff;
}

Disassembly cross-check (objdump -d -M intel bin/halo.exe):

Call site inside 0x415480 (0x415c89..0x415cc1): EAX set to lea eax,[esi+0x5a4] or [esi+0x5b0]
(actor.position_cache_a / position_cache_b) depending on a flag pair, then 5 stack pushes for
param_1..param_5 (add esp,0x14 after the call confirms 5 dwords).

Call site inside this function to actor_look_pick_random_point_in_cone (0x414eb9..0x414ee1):
6 stack pushes (add esp,0x18 confirms 6), matching Ghidra's 6-arg call exactly; `lea esi,
[esp+0x34]` loads ESI with a stack copy of local_c/local_8/local_4 just before the call, and
`mov bl,1` sets BL, both consumed as implicit register arguments by actor_look_pick_random_
point_in_cone's own decompilation (unaff_ESI, unaff_BL).
#endif
