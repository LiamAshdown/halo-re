// actor_movement_advance_waypoint  (Ghidra: actor_movement_advance_waypoint, renamed)
// address 0x4163e0, size 796 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (checked against objdump 0x4163e0..0x4166fb)
// evidence: re-resolves the movement action when needed (actor_movement_action_resolve),
// checks arrival (actor_movement_check_arrival), then walks a short list of 16-byte waypoint
// records starting at actor+0x4a8 (self->movement_action_complete's own address, reused here
// as a byte-array base rather than a single flag -- see TYPES-GAP below), advancing a cursor
// (unknown_4c0[2]) past waypoints that are either close enough (within a fixed radius) or
// behind the actor's current heading, and commits the resulting target into unknown_50c /
// unknown_518 (delta from body_position). Falls back to a fixed-offset point along the
// actor's facing, or clears the path state entirely, depending on unknown_15e.
// register convention: actor_index in EAX (Ghidra's in_EAX).
// blam-cc: EAX -> actor_index
// TYPES-GAP: the byte region actor+0x4a8..0x4c8 that this function walks as an array of
// 16-byte {real_point2d point; uint8_t pad[8];} waypoint records overlaps several fields
// types/ai.h already names individually in that range (movement_action_complete at 0x4a8,
// unknown_4a9[19], unknown_4bc, unknown_4c0[12]); not reconciled with the header here, only
// accessed through raw offsets from a local byte pointer, matching what this function does.
// UNSURE: POPCOUNT(bVar12)&1 (Ghidra's parity-bit rendering of a single boolean OR of a
// less-than compare and a NaN check) is simplified here to a plain floating compare, which is
// equivalent for the non-NaN case and returns false (not "true", as POPCOUNT's odd/even flip
// might otherwise suggest) when the compared value is NaN; not independently re-derived from
// the flags.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context); // 0x41a460, this module
extern uint8_t actor_movement_check_arrival(datum_index actor_index); // 0x416700, this module

// blam-cc: EAX -> actor_index
void actor_movement_advance_waypoint(datum_index actor_index)
{
    actor *self;
    uint8_t *waypoints; // &self->movement_action_complete, reinterpreted as the record array base

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->needs_new_path != 0 && self->unknown_4a4 == 0 && self->keep_unit_alive == 0) {
        actor_movement_action_resolve(actor_index, 0, 0);
    }
    actor_movement_check_arrival(actor_index);

    waypoints = &self->movement_action_complete; // 0x4a8
    if (self->movement_action_complete != 0) {
        for (;;) {
            int cursor = (int)(int8_t)self->unknown_4c0[2];
            float *cur;
            float *next;
            float dx, dy, ex, ey;
            uint8_t reject;

            if ((int)(int8_t)self->unknown_4c0[1] <= cursor + 1) break;

            cur = (float *)(waypoints + (cursor + 2) * 0x10);
            next = (float *)(waypoints + (cursor + 3) * 0x10);
            dx = cur[0] - self->body_position.x;
            dy = cur[1] - self->body_position.y;
            ex = next[0] - cur[0];
            ey = next[1] - cur[1];

            // When unknown_506 is already set, Ghidra skips the whole reject test below and
            // falls straight through to advancing the cursor (i.e. behaves as if rejected).
            if (self->waypoint_reached == 0) {
                if (self->moving == 0 || self->movement_thwarted == 0) {
                    float dist2 = dx * dx + dy * dy;
                    reject = dist2 < 0.0225f;
                } else {
                    float along = ex * dx + ey * dy;
                    if (ex * self->facing.i + ey * self->facing.j <= 0.0f || 0.0f <= along) break;
                    along = -along;
                    dx = ex * along + dx;
                    dy = ey * along + dy;
                    {
                        float dist2 = dx * dx + dy * dy;
                        reject = dist2 < 0.0625f;
                    }
                }
                if (!reject) break;
            }

            self->unknown_4c0[2] = self->unknown_4c0[2] + 1;
            self->waypoint_reached = 0;
        }

        if (self->waypoint_reached != 0 && self->unknown_4c0[0] != 0) {
            self->movement_action_complete = 0;
            self->movement_completed = 1;
            self->movement_timer = 0;
        }

        if (*waypoints != 0 && (self->moving != 0 || self->movement_completed == 0)) {
            float *cur;
            real_point3d *target;
            self->moving = 1;
            cur = (float *)((uint8_t *)self + 0x4c8 + (int8_t)self->unknown_4c0[2] * 0x10);
            target = (real_point3d *)&self->current_waypoint;
            target->x = cur[0];
            target->y = cur[1];
            target->z = cur[2];
            self->desired_movement_vector.x = target->x - self->body_position.x;
            self->desired_movement_vector.y = target->y - self->body_position.y;
            self->desired_movement_vector.z = target->z - self->body_position.z;
            return;
        }
    }

    if (self->vehicle_driving_type == 4) {
        float sign = (self->avoidance_emergency <= 0.9f) ? 1.0f : -1.0f;
        float scale = sign * 3.0f;
        self->moving = 1;
        self->waypoint_reached = 0;
        self->desired_movement_vector.x = scale * self->facing.i;
        self->desired_movement_vector.y = scale * self->facing.j;
        self->desired_movement_vector.z = scale * self->facing.k;
        *(real_point3d *)&self->current_waypoint = self->body_position;
        ((real_point3d *)&self->current_waypoint)->x += self->desired_movement_vector.x;
        ((real_point3d *)&self->current_waypoint)->y += self->desired_movement_vector.y;
        ((real_point3d *)&self->current_waypoint)->z += self->desired_movement_vector.z;
        return;
    }

    self->moving = 0;
    self->waypoint_reached = 0;
    self->movement_completed = 1;
    self->movement_action_complete = 0;
    self->movement_completed = 1;
    self->movement_timer = 0;
}

#if 0
Original Ghidra decompilation (0x4163e0):

void FUN_004163e0(void)

{
  char *pcVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  uint in_EAX;
  int iVar7;
  int iVar8;
  float *pfVar9;
  int iVar10;
  int iVar11;
  byte bVar12;

  iVar11 = (in_EAX & 0xffff) * 0x724;
  iVar10 = *(int *)(DAT_00880360 + 0x34) + iVar11;
  if (((*(char *)(*(int *)(DAT_00880360 + 0x34) + 0x4c + iVar11) != '\0') &&
      (*(char *)(iVar10 + 0x4a4) == '\0')) && (*(char *)(iVar10 + 0x13) == '\0')) {
    actor_movement_action_resolve();
  }
  FUN_00416700();
  pcVar1 = (char *)(iVar10 + 0x4a8);
  if (*(char *)(iVar10 + 0x4a8) != '\0') {
    while( true ) {
      iVar7 = (int)*(char *)(iVar10 + 0x4c2);
      if ((int)*(char *)(iVar10 + 0x4c1) <= iVar7 + 1) break;
      iVar8 = (iVar7 + 2) * 0x10;
      pfVar9 = (float *)(pcVar1 + iVar8);
      fVar2 = *(float *)(pcVar1 + iVar8) - *(float *)(iVar10 + 300);
      fVar3 = pfVar9[1] - *(float *)(iVar10 + 0x130);
      fVar4 = *(float *)(pcVar1 + (iVar7 + 3) * 0x10) - *pfVar9;
      fVar5 = *(float *)((int)(pcVar1 + (iVar7 + 3) * 0x10) + 4) - pfVar9[1];
      if (*(char *)(iVar10 + 0x506) == '\0') {
        if ((*(char *)(iVar10 + 0x504) == '\0') || (*(char *)(iVar10 + 0x507) == '\0')) {
          fVar2 = fVar2 * fVar2 + fVar3 * fVar3;
          bVar12 = fVar2 < 0.0225 | (byte)((ushort)((ushort)NAN(fVar2) << 10) >> 8);
        }
        else {
          fVar6 = fVar4 * fVar2 + fVar5 * fVar3;
          if ((fVar4 * *(float *)(iVar10 + 0x174) + fVar5 * *(float *)(iVar10 + 0x178) <= 0.0) ||
             (0.0 <= fVar6)) break;
          fVar6 = -fVar6;
          fVar2 = fVar4 * fVar6 + fVar2;
          fVar3 = fVar5 * fVar6 + fVar3;
          fVar2 = fVar2 * fVar2 + fVar3 * fVar3;
          bVar12 = fVar2 < 0.0625 | (byte)((ushort)((ushort)NAN(fVar2) << 10) >> 8);
        }
        if ((POPCOUNT(bVar12) & 1U) == 0) break;
      }
      *(char *)(iVar10 + 0x4c2) = *(char *)(iVar10 + 0x4c2) + '\x01';
      *(undefined1 *)(iVar10 + 0x506) = 0;
    }
    if ((*(char *)(iVar10 + 0x506) != '\0') && (*(char *)(iVar10 + 0x4c0) != '\0')) {
      iVar7 = *(int *)(DAT_00880360 + 0x34) + iVar11;
      *(undefined1 *)(iVar7 + 0x4a8) = 0;
      *(undefined1 *)(iVar7 + 0x484) = 1;
      *(undefined4 *)(iVar7 + 0x4a0) = 0;
    }
    if ((*pcVar1 != '\0') &&
       ((*(char *)(iVar10 + 0x504) != '\0' || (*(char *)(iVar10 + 0x484) == '\0')))) {
      *(undefined1 *)(iVar10 + 0x504) = 1;
      pfVar9 = (float *)(*(char *)(iVar10 + 0x4c2) * 0x10 + 0x4c8 + iVar10);
      *(float *)(iVar10 + 0x50c) = *pfVar9;
      *(float *)(iVar10 + 0x510) = pfVar9[1];
      *(float *)(iVar10 + 0x514) = pfVar9[2];
      *(float *)(iVar10 + 0x518) = *(float *)(iVar10 + 0x50c) - *(float *)(iVar10 + 300);
      *(float *)(iVar10 + 0x51c) = *(float *)(iVar10 + 0x510) - *(float *)(iVar10 + 0x130);
      *(float *)(iVar10 + 0x520) = *(float *)(iVar10 + 0x514) - *(float *)(iVar10 + 0x134);
      return;
    }
  }
  iVar7 = DAT_00880360;
  if (*(short *)(iVar10 + 0x15e) == 4) {
    *(undefined1 *)(iVar10 + 0x504) = 1;
    *(undefined1 *)(iVar10 + 0x506) = 0;
    fVar2 = (float)(int)((uint)(*(float *)(iVar10 + 0x5ec) <= 0.9) * 2 + -1) * 3.0;
    *(float *)(iVar10 + 0x518) = fVar2 * *(float *)(iVar10 + 0x174);
    *(float *)(iVar10 + 0x51c) = fVar2 * *(float *)(iVar10 + 0x178);
    *(float *)(iVar10 + 0x520) = fVar2 * *(float *)(iVar10 + 0x17c);
    *(float *)(iVar10 + 0x50c) = *(float *)(iVar10 + 300) + *(float *)(iVar10 + 0x518);
    *(float *)(iVar10 + 0x510) = *(float *)(iVar10 + 0x130) + *(float *)(iVar10 + 0x51c);
    *(float *)(iVar10 + 0x514) = *(float *)(iVar10 + 0x134) + *(float *)(iVar10 + 0x520);
    return;
  }
  *(undefined1 *)(iVar10 + 0x504) = 0;
  *(undefined1 *)(iVar10 + 0x506) = 0;
  *(undefined1 *)(iVar10 + 0x484) = 1;
  iVar11 = *(int *)(iVar7 + 0x34) + iVar11;
  *(undefined1 *)(iVar11 + 0x4a8) = 0;
  *(undefined1 *)(iVar11 + 0x484) = 1;
  *(undefined4 *)(iVar11 + 0x4a0) = 0;
  return;
}
#endif
