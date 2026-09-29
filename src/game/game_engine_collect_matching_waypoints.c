// game_engine_collect_matching_waypoints  (Ghidra: FUN_00462190; renamed per its summary)
// address 0x462190, size 150 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("For a given player/object filter, walks the custom
// waypoint table and copies the positions and slot indices of all matching entries into the
// caller-supplied output arrays"); types/game.h custom_waypoint (position +0x00),
// k_maximum_custom_waypoints (32), game_variant::unknown_3c (+0x3c, aliased 0x006f1cc4); this
// batch's custom_waypoint_matches_filter (0x4620c0).
// register convention: no register-passed arguments Ghidra recovers for this function's own
// four stack parameters; custom_waypoint_matches_filter's own EAX/EDI inputs (reference,
// slot index) are forwarded from values this function's caller must supply, since neither is
// set anywhere in this body.
//   // blam-cc: stack -> candidate, out_positions, out_slots, max_count
// UNSURE: only position.x and position.y (not .z) are copied out, matching Ghidra's own
// rendering exactly; `reference_team` is a forwarded parameter standing in for
// custom_waypoint_matches_filter's unrecovered EAX input.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88 (unknown_3c aliased 0x006f1cc4)
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints]; // 0x006f1888

extern uint8_t custom_waypoint_matches_filter(int32_t candidate, custom_waypoint *slot,
    int32_t reference_team); // 0x4620c0, this batch

// blam-cc: stack -> candidate, out_positions, out_slots, max_count
int16_t game_engine_collect_matching_waypoints(int32_t candidate, float *out_positions,
    uint8_t *out_slots, int16_t max_count, int32_t reference_team) // UNSURE: extra forwarded param
{
    int16_t written = 0;
    int16_t slot;

    if (current_game_engine != 0 && game_engine_variant.objective_indicator == 0 && candidate != -1) {
        for (slot = 0; slot < k_maximum_custom_waypoints; slot++) {
            if (custom_waypoint_matches_filter(candidate, &custom_waypoints[slot], reference_team) != 0 &&
                written < max_count) {
                out_slots[written] = (uint8_t)slot;
                out_positions[written * 2] = custom_waypoints[slot].position.x;
                out_positions[written * 2 + 1] = custom_waypoints[slot].position.y;
                written = written + 1;
            }
        }
    }
    return written;
}

#if 0
Original Ghidra decompilation (0x462190), from tools/pack.py 0x462190:

short FUN_00462190(int param_1,int param_2,int param_3,short param_4)

{
  char cVar1;
  int iVar2;
  char cVar3;
  short sVar4;
  undefined4 *puVar5;

  sVar4 = 0;
  if (((DAT_006f1d20 != 0) && (DAT_006f1cc4 == 0)) && (param_1 != -1)) {
    cVar3 = '\0';
    puVar5 = &DAT_006f1888;
    do {
      cVar1 = FUN_004620c0(param_1);
      if ((cVar1 != '\0') && (sVar4 < param_4)) {
        iVar2 = (int)sVar4;
        *(char *)(iVar2 + param_3) = cVar3;
        *(undefined4 *)(param_2 + iVar2 * 8) = *puVar5;
        *(undefined4 *)(param_2 + 4 + iVar2 * 8) = puVar5[1];
        sVar4 = sVar4 + 1;
      }
      puVar5 = puVar5 + 8;
      cVar3 = cVar3 + '\x01';
    } while ((int)puVar5 < 0x6f1c88);
  }
  return sVar4;
}
#endif
