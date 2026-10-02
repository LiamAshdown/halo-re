// actor_look_randomize_direction  (Ghidra: actor_look_randomize_direction, renamed)
// address 0x414f50, size 401 bytes
// name confidence: 0.35  rewrite confidence: 0.9 (VERIFIED against objdump 0x414f50..0x4150e0; record target and BL FIXED)
// evidence: phase-4 summary "picks a new random gaze/look direction constrained to the
// unit's aim cone and arms the look timer for the actor"; the only caller (0x415480) only
// reaches this function when actor.unknown_568 (the wait-tick timer this function itself
// arms) is already zero, gated behind a small flag check this rewrite does not attempt to
// interpret. Always uses actor_select_facing_target_prop's full lane test (require_trust=0,
// skip_lane_test=0) into a separate output slot (actor+0x57c) from actor_resolve_look_target
// (actor+0x56c), and always calls actor_look_pick_random_point_in_cone with obstruction
// checking on (unlike actor_resolve_look_target's caller-selected on/off), matching a
// "reactive" look versus that function's more general one.
// register convention: reconstructed from objdump -d -M intel over 0x414f50..0x4150e0 and
// the one call site inside 0x415480 (0x415ef0..0x415eff, which pushes 3 stack dwords,
// add esp,0xc after the call). actor_index, deviation_table and base_direction are all
// genuine stack parameters; Ghidra's own decompilation of this function only recognized the
// first of the three (as "param_1"), reading the other two as raw, unnamed stack offsets
// past its own frame instead of formal parameters.
// blam-cc: stack -> actor_index, stack -> deviation_table, stack -> base_direction
// UNSURE: deviation_table's contents/meaning are not established beyond being forwarded to
// actor_look_get_wait_ticks with mode=2 (its third pair, offsets 0x10/0x14); see that file.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

// actor_recognition_scan_result now lives in types/ai.h (folded from this file).

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

extern double cos(double x); // FCOS

extern uint8_t actor_select_facing_target_prop(datum_index actor_index, uint8_t require_trust, uint8_t skip_lane_test,
                                                actor_recognition_scan_result *out_result, uint8_t *out_in_front); // 0x414a90, this module
extern uint8_t actor_look_pick_random_point_in_cone(void *origin, float yaw_min, float yaw_max, float pitch_min,
                                                     float pitch_max, real_vector3d *base_direction,
                                                     uint8_t check_obstruction, real_point3d *out); // 0x415260, this module
extern int32_t actor_look_get_wait_ticks(datum_index actor_index, int16_t mode, uint32_t flags, float *deviation_table); // 0x415150, EAX, stack, EDI

// blam-cc: stack -> actor_index, stack -> deviation_table, stack -> base_direction
void actor_look_randomize_direction(datum_index actor_index, float *deviation_table, real_vector3d *base_direction)
{
    actor *self;
    Actor *definition;
    uint32_t out_in_front;
    float yaw_max, pitch_max;
    float delta_l, delta_r;
    float yaw_min;
    int32_t wait_ticks;
    real_point3d look_point;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    out_in_front = 0;
    self->idle_look_state[1] = 0; // self+0x55f

    // FIXED (0x414f7d..0x414fa8): the facing target lands in the actor's own look record (+0x57c), which
    //   actor_update_look_target resolves next; the draft wrote it into a local
    if (!actor_select_facing_target_prop(actor_index, 0, 0, (actor_recognition_scan_result *)((uint8_t *)self + 0x57c),
            (uint8_t *)&out_in_front)) {
        yaw_max = (definition->maximum_looking_deviation.yaw <= definition->idle_looking_range.yaw)
                      ? definition->maximum_looking_deviation.yaw
                      : definition->idle_looking_range.yaw;
        pitch_max = (definition->maximum_looking_deviation.pitch <= definition->idle_looking_range.pitch)
                        ? definition->maximum_looking_deviation.pitch
                        : definition->idle_looking_range.pitch;

        if (self->awareness_level == 3) { // 0x6a, exactly as Ghidra reads it
            delta_l = definition->combat_look_delta_l;
            delta_r = definition->combat_look_delta_r;
        } else {
            delta_l = definition->noncombat_look_delta_l;
            delta_r = definition->noncombat_look_delta_r;
        }

        yaw_min = -delta_l;
        if (yaw_min < -yaw_max) {
            yaw_min = -yaw_max;
        }
        if (delta_r < yaw_max) {
            yaw_max = delta_r;
        }

        if (!actor_look_pick_random_point_in_cone(&self->aim_origin, yaw_min, yaw_max, -pitch_max, pitch_max,
                                                    base_direction, 0, &look_point)) { // BL = 0 (xor bl,bl at 0x41507e)
            return;
        }
        self->idle_look_point = look_point; // self+0x580
        out_in_front = 0;
        self->idle_look_direction_type = 4; // self+0x57c
    }

    wait_ticks = actor_look_get_wait_ticks(actor_index, 2, out_in_front, deviation_table);
    *(int32_t *)self->idle_minor_timer = wait_ticks;
    if (wait_ticks != 0) {
        self->idle_look_state[1] = 1; // self+0x55f
    }
}

#if 0
Original Ghidra decompilation (0x414f50):

void FUN_00414f50(uint param_1)

{
  char cVar1;
  float *pfVar2;
  int iVar3;
  int iVar4;
  float local_1c;
  uint local_18;
  float local_14;
  float local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar3 = (param_1 & 0xffff) * 0x724;
  iVar4 = iVar3 + *(int *)(DAT_00880360 + 0x34);
  iVar3 = *(int *)((*(uint *)(iVar3 + 0x58 + *(int *)(DAT_00880360 + 0x34)) & 0xffff) * 0x20 + 0x14
                  + DAT_0087bc14);
  local_18 = local_18 & 0xffffff00;
  *(undefined1 *)(iVar4 + 0x55f) = 0;
  cVar1 = FUN_00414a90(0,0,(undefined2 *)(iVar4 + 0x57c),&local_18);
  if (cVar1 == '\0') {
    if (*(float *)(iVar3 + 0xac) <= *(float *)(iVar3 + 0xcc)) {
      local_14 = *(float *)(iVar3 + 0xac);
    }
    else {
      local_14 = *(float *)(iVar3 + 0xcc);
    }
    if (*(float *)(iVar3 + 0xb0) <= *(float *)(iVar3 + 0xd0)) {
      local_1c = *(float *)(iVar3 + 0xb0);
    }
    else {
      local_1c = *(float *)(iVar3 + 0xd0);
    }
    pfVar2 = (float *)(iVar3 + 0xbc);
    if (*(short *)(iVar4 + 0x6a) != 3) {
      pfVar2 = (float *)(iVar3 + 0xb4);
    }
    local_10 = -*pfVar2;
    if (local_10 < -local_14) {
      local_10 = -local_14;
    }
    if (pfVar2[1] < local_14) {
      local_14 = pfVar2[1];
    }
    cVar1 = actor_look_pick_random_point_in_cone
                      (iVar4 + 0x120,local_10,local_14,-local_1c,local_1c,&local_c);
    if (cVar1 == '\0') {
      return;
    }
    *(undefined4 *)(iVar4 + 0x580) = local_c;
    *(undefined4 *)(iVar4 + 0x584) = local_8;
    local_18 = local_18 & 0xffffff00;
    *(undefined2 *)(iVar4 + 0x57c) = 4;
    *(undefined4 *)(iVar4 + 0x588) = local_4;
  }
  iVar3 = actor_look_get_wait_ticks(2,local_18);
  *(int *)(iVar4 + 0x568) = iVar3;
  if (iVar3 != 0) {
    *(undefined1 *)(iVar4 + 0x55f) = 1;
  }
  return;
}

Disassembly cross-check (objdump -d -M intel bin/halo.exe): the call site inside 0x415480
(0x415ef0..0x415eff) pushes 3 stack dwords (ecx = [esp+0x20], edx = [esp+0x60], eax =
lea eax,[esp+0x2c]) and cleans up with add esp,0xc, one more than this function's own
decompilation shows a parameter for; 0x414f50's own body reads the extra two at
[esp+0x34]/[esp+0x38] as unaff_EDI (forwarded into actor_look_get_wait_ticks) / unaff_ESI
(forwarded into actor_look_pick_random_point_in_cone), confirming they are param_2/param_3.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
