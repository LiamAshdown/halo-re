// game_engine_collect_matching_waypoints  (Ghidra: FUN_00462190; renamed per its summary)
// address 0x462190, size 150 bytes
// VERIFIED against disassembly 0x462190..0x462226 (2026-09-30)
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("For a given player/object filter, walks the custom
// waypoint table and copies the positions and slot indices of all matching entries into the
// caller-supplied output arrays"); types/game.h custom_waypoint (position +0x00),
// k_maximum_custom_waypoints (32), game_variant::unknown_3c (+0x3c, aliased 0x006f1cc4); this
// batch's custom_waypoint_matches_filter (0x4620c0).
// register convention: four stack parameters. FIXED 2026-09-30: the earlier fifth "forwarded reference_team" parameter
// does not exist -- the function itself resolves the candidate player handle to its record
// (player_data->data + (candidate & 0xffff) * 0x200, 0x4621c2..0x4621c8) and passes that in EAX to
// custom_waypoint_matches_filter, with the slot index in EDI.
//   // blam-cc: stack -> candidate, out_positions, out_slots, max_count
// Only position.x and position.y (8 bytes per entry) are copied out (0x462200..0x46220c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88 (unknown_3c aliased 0x006f1cc4)
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints]; // 0x006f1888

extern data_array *player_data; // 0x0087a480

extern uint8_t custom_waypoint_matches_filter(int32_t candidate, player *reference_player,
    int32_t slot_index); // 0x4620c0, EAX reference_player, EDI slot_index, stack candidate

// blam-cc: stack -> candidate, out_positions, out_slots, max_count
int16_t game_engine_collect_matching_waypoints(int32_t candidate, float *out_positions,
    uint8_t *out_slots, int16_t max_count)
{
    int16_t written = 0;
    int16_t slot;

    if (current_game_engine != 0 && game_engine_variant.objective_indicator == 0 && candidate != -1) {
        player *reference_player = (player *)((uint8_t *)player_data->data + (candidate & 0xffff) * 0x200);

        for (slot = 0; slot < k_maximum_custom_waypoints; slot++) {
            if (custom_waypoint_matches_filter(candidate, reference_player, slot) != 0 &&
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
