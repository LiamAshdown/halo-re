// actor_grenade_trace_from_source  (Ghidra: actor_grenade_trace_from_source, renamed)
// address 0x4029e0, size 184 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4029e0..0x402a97)
// evidence: types/ai.h actor.movement_completed (0x484)/movement_action_complete (0x4a8);
//   phase-4 summary ("issues a trace request from a cached or newly fetched source point
//   toward a target point, used when evaluating a grenade throw").
// register convention: actor index in EAX; the target point is a real_point3d in ESI
//   (unaff_ESI).
//   // blam-cc: EAX -> actor_index, ESI -> target_point
// UNSURE: actor+0x120/0x124/0x128 fall inside actor.mode_data (a per-mode union, see
//   types/ai.h); read here as a cached source point but not independently confirmed.
//   UNSURE: when movement_completed is clear and movement_action_complete is set, the
//   original calls FUN_00569190 to refresh actor+0x4ac (inside the unattributed run at
//   actor.unknown_4a9) but then goes on to read the *mode_data* source point (offset
//   0x120/0x124/0x128) without ever having written it on this path -- i.e. the decompiled
//   function reads that point uninitialized here. Preserved exactly rather than "fixed",
//   per the no-invented-behaviour rule; FUN_00569190 and FUN_00505880 are both outside this
//   session's range and unreviewed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

// 0x569190, not yet rewritten (a different module): refreshes some per-actor cached point.
extern void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t mode, float *world_point,
    uint32_t reference_direction, uint32_t offsets, real_point3d *accumulator); // 0x569190, stack, EAX accumulator
// 0x505880, not yet rewritten (a different module): a generic trace/raycast request.
extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object, void *scratch);

// If the actor's queued movement has not completed, and its last movement action has not
// completed either, does nothing (no trace to issue). Otherwise resolves a source point --
// either refreshed via FUN_00569190 (if the movement completed) or the actor's cached
// mode_data point (if it did not, see UNSURE) -- and issues a trace (kind 0x33) from it
// toward target_point, always reporting success once a trace is issued.
int32_t actor_grenade_trace_from_source(uint32_t actor_index, real_point3d *target_point)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    real_point3d source;
    real_vector3d delta;
    uint8_t trace_result[80];

    if (a->movement_completed == 0) {
        if (a->movement_action_complete == 0) {
            return 0;
        }
        unit_add_marker_relative_offset(a->unit_index, 1, (float *)((uint8_t *)a + 0x4ac), 0, 0, &source); // 0x402a83: EAX = source
    } else {
        source.x = a->aim_origin.x;
        source.y = a->aim_origin.y;
        source.z = a->aim_origin.z;
    }

    delta.i = target_point->x - source.x;
    delta.j = target_point->y - source.y;
    delta.k = target_point->z - source.z;
    collision_test_movement_segment(0x33, &source, &delta, 0xffffffff, trace_result);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4029e0):

undefined4 FUN_004029e0(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  float *unaff_ESI;
  float local_68;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  undefined1 local_50 [80];

  iVar1 = (in_EAX & 0xffff) * 0x724;
  iVar2 = iVar1 + *(int *)(DAT_00880360 + 0x34);
  if (*(char *)(iVar1 + 0x484 + *(int *)(DAT_00880360 + 0x34)) == '\0') {
    if (*(char *)(iVar2 + 0x4a8) == '\0') {
      return 0;
    }
    FUN_00569190(*(undefined4 *)(iVar2 + 0x18),1,iVar2 + 0x4ac,0,0);
  }
  else {
    local_68 = *(float *)(iVar2 + 0x120);
    local_64 = *(float *)(iVar2 + 0x124);
    local_60 = *(float *)(iVar2 + 0x128);
  }
  local_5c = *unaff_ESI - local_68;
  local_58 = unaff_ESI[1] - local_64;
  local_54 = unaff_ESI[2] - local_60;
  FUN_00505880(0x33,&local_68,&local_5c,0xffffffff,local_50);
  return 1;
}
#endif
