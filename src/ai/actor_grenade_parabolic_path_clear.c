// actor_grenade_parabolic_path_clear  (Ghidra: actor_grenade_parabolic_path_clear; named for this rewrite)
// address 0x42b5d0, size 565 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: phase-4 summary ("steps a projected parabolic trajectory forward in short
// segments, checking line-of-sight and proximity to nearby actors along each segment").
// Ghidra dropped every register argument to actor_gather_nearby_grenade_targets (recovered
// in actor_gather_nearby_grenade_targets.c) and to segment3d_within_radius_of_segment;
// EAX/ECX/EDX confirmed via objdump -d -M intel --start-address=0x42b5d0
// --stop-address=0x42b660 bin/halo.exe (source registers cached into ESI/EDI at entry and
// used unchanged thereafter). The segment3d_within_radius_of_segment argument mapping
// (b_start/b_direction from the gathered entry) mirrors actor_grenade_trajectory_blocked.c's
// already-derived mapping for the same callee and entry layout.
// register convention: EAX -> initial_velocity, ECX -> source_actor_index, EDX ->
// start_position; stack -> total_time, vertical_acceleration, exclude_object_index, wide_mask.
// blam-cc: EAX -> initial_velocity, ECX -> source_actor_index, EDX -> start_position, stack
// -> total_time, vertical_acceleration, exclude_object_index, wide_mask

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t actor_gather_nearby_grenade_targets(datum_index source_actor_index, int16_t maximum_count,
                                                     ai_grenade_avoidance_entry *out_entries); // 0x0042afc0
extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta,
                             uint32_t exclude_object, void *scratch); // 0x505880
extern int segment3d_within_radius_of_segment(real_point3d *a_start, real_point3d *b_start, real_vector3d *a_direction, real_vector3d *b_direction, real radius); // 0x4ceae0

// blam-cc: EAX -> initial_velocity, ECX -> source_actor_index, EDX -> start_position, stack
// -> total_time, vertical_acceleration, exclude_object_index, wide_mask
// Walks the grenade's parabolic trajectory (constant initial_velocity, vertical_acceleration
// applied to z) forward in up-to-6-second segments out to total_time. Each segment is tested
// for a world collision and, for every nearby gathered target, for coming within that
// target's avoidance radius of the segment. Returns 0 as soon as either check fails, 1 if
// the whole trajectory stays clear.
uint8_t actor_grenade_parabolic_path_clear(real_vector3d *initial_velocity, datum_index source_actor_index,
                                            real_point3d *start_position, real total_time,
                                            real vertical_acceleration, datum_index exclude_object_index,
                                            uint8_t wide_mask)
{
    ai_grenade_avoidance_entry entries[32];
    int16_t nearby_count;
    uint32_t collision_mask;
    real_point3d position;
    real_point3d next_position;
    real_vector3d segment_delta;
    real t;
    real previous_t;
    uint8_t clear;
    uint8_t scratch[0x50]; // collision_result (0x50 bytes; [esp+0x60] in 0x42b67b), was 64 and overflowed
    int16_t i;

    nearby_count = actor_gather_nearby_grenade_targets(source_actor_index, 32, entries);
    collision_mask = wide_mask ? 0xc0b3u : 0xc2b3u;

    position = *start_position;
    previous_t = 0.0f;
    t = (6.0f <= total_time) ? 6.0f : total_time;
    clear = 0;

    do {
        real dt = t - previous_t;
        next_position.x = initial_velocity->i * dt + position.x;
        next_position.y = initial_velocity->j * dt + position.y;
        next_position.z = position.z + initial_velocity->k * dt + dt * dt * vertical_acceleration * 0.5f;

        segment_delta.i = next_position.x - position.x;
        segment_delta.j = next_position.y - position.y;
        segment_delta.k = next_position.z - position.z;

        clear = (collision_test_movement_segment(collision_mask, &position, &segment_delta, exclude_object_index, scratch) == 0);
        if (!clear) {
            return 0;
        }

        for (i = 0; i < nearby_count; i++) {
            real_vector3d target_offset;
            int hit;
            target_offset.i = 0.0f;
            target_offset.j = 0.0f;
            target_offset.k = entries[i].crouch_offset;
            hit = segment3d_within_radius_of_segment(&position, &entries[i].target_position, &segment_delta,
                                                       &target_offset, entries[i].avoid_until);
            if (hit) {
                return 0;
            }
        }

        position = next_position;
        previous_t = t;
        t = t + 6.0f;
        if (total_time < t) {
            t = total_time;
        }
    } while (previous_t < total_time);

    return clear;
}

#if 0
Original Ghidra decompilation (0x42b5d0):

char FUN_0042b5d0(float param_1,float param_2,undefined4 param_3,char param_4)

{
  float fVar1;
  char cVar2;
  char cVar3;
  float *in_EAX;
  float *in_EDX;
  short sVar4;
  float local_59c;
  float local_598;
  float local_594;
  float local_590;
  float local_58c;
  float local_588;
  float local_584;
  float local_580;
  undefined4 local_57c;
  undefined4 local_578;
  float local_574;
  float local_570;
  float local_56c;
  float local_568;
  float local_564;
  float local_560;
  float local_55c;
  float local_558;
  float local_554;
  undefined1 local_550 [116];
  undefined4 auStack_4dc [311];

  local_578 = actor_gather_nearby_grenade_targets();
  local_57c = 0xc2b3;
  if (param_4 != '\0') {
    local_57c = 0xc0b3;
  }
  local_594 = in_EDX[1];
  local_598 = *in_EDX;
  local_580 = 0.0;
  local_590 = in_EDX[2];
  local_570 = in_EAX[1];
  local_574 = *in_EAX;
  local_56c = in_EAX[2];
  if (6.0 <= param_1) {
    local_59c = 6.0;
  }
  else {
    local_59c = param_1;
  }
  do {
    fVar1 = local_59c;
    local_580 = local_59c - local_580;
    local_58c = local_574 * local_580 + local_598;
    local_588 = local_570 * local_580 + local_594;
    local_584 = local_580 * local_56c + local_590 + local_580 * local_580 * param_2 * 0.5;
    local_55c = local_58c - local_598;
    local_558 = local_588 - local_594;
    local_554 = local_584 - local_590;
    cVar2 = FUN_00505880(local_57c,&local_598,&local_55c,param_3,local_550);
    cVar2 = '\x01' - (cVar2 != '\0');
    if (cVar2 == '\0') {
      return '\0';
    }
    sVar4 = 0;
    local_568 = local_58c - local_598;
    local_564 = local_588 - local_594;
    local_560 = local_584 - local_590;
    if (0 < (short)local_578) {
      do {
        cVar3 = segment3d_within_radius_of_segment(&local_598,auStack_4dc[sVar4 * 10]);
        if (cVar3 != '\0') {
          return '\0';
        }
        sVar4 = sVar4 + 1;
      } while (sVar4 < (short)local_578);
    }
    local_594 = local_588;
    local_598 = local_58c;
    local_56c = local_580 * param_2 + local_56c;
    local_590 = local_584;
    local_59c = local_59c + 6.0;
    if (param_1 < local_59c) {
      local_59c = param_1;
    }
    local_580 = fVar1;
  } while (fVar1 < param_1);
  return cVar2;
}

Real disassembly confirming the register mapping (0x42b5d0-0x42b5e9):

0042b5d0: sub    esp,0x5a0
0042b5d6: push   ebx
0042b5d7: push   ebp
0042b5d8: push   esi
0042b5d9: push   edi
0042b5da: mov    esi,eax             ; esi = initial_velocity
0042b5dc: lea    eax,[esp+0xb0]      ; eax = &entries[0]
0042b5e3: push   eax
0042b5e4: push   0x20
0042b5e6: push   ecx                 ; ecx = source_actor_index
0042b5e7: mov    edi,edx             ; edi = start_position
0042b5e9: call   0x42afc0
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
