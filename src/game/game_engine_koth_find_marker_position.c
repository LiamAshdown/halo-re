// game_engine_koth_find_marker_position  (Ghidra: FUN_0046beb0; named per its summary)
// address 0x46beb0, size 296 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Finds a valid (or, failing that, a random) type-2
//   scenario starting location and returns its position, used to place the King-of-the-Hill
//   marker"); types/tags.h ScenarioNetgameFlags (0x94 stride, type at +0x10, position at +0x00);
//   game_engine_find_valid_starting_locations (0x461080, already committed); game_variant::
//   ctf_option_7c aliased at 0x006f1d04; random_seed_global's LCG already used identically in
//   game_engine_pick_random_recent_location.c (this batch).
// register convention: output position pointer in param_1 (Ghidra's own stack parameter); type
//   filter in CX.
//   // blam-cc: stack -> out_position, CX -> type_filter

// CORRECTED (phase 4 review): the Blam random-index idiom is
//   movsx ecx,<count> ; shr eax,0x10 ; imul eax,ecx ; shr eax,0x10 ; movsx <idx>,ax
// so the seed's high half is used ZERO-extended (shr, no movsx) and the product is shifted
// down logically. Casting (seed >> 16) to int16_t first, as this file did, makes the index
// negative for half of all seeds, which silently disables the pick.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Scenario *global_scenario;      // 0x00746f8c
extern random_seed random_seed_global;    // 0x00719cd0
extern game_variant game_engine_variant; // 0x006f1c88 (ctf_option_7c aliased 0x006f1d04)

extern int32_t game_engine_find_valid_starting_locations(real_point3d *origin,
    float max_horizontal_dist, float max_height_delta, int16_t team, int16_t type,
    int32_t max_results, int32_t *results); // 0x461080

// blam-cc: stack -> out_position, CX -> type_filter
// Tries game_engine_find_valid_starting_locations first (unless ctf_option_7c is set); if that
// finds nothing, falls back to picking a uniformly random type-2 ScenarioNetgameFlags entry via
// the shared LCG. Writes the chosen entry's position into *out_position.
void game_engine_koth_find_marker_position(real_point3d *out_position, int16_t type_filter)
{
    int32_t index = -1;
    // CORRECTED (phase 4 review): every failure path in the original falls through to the same
    // three-dword copy at 0x46bfb6, so out_position is written even when no entry matched -- with
    // whatever was on the stack. This staging copy is deliberately left uninitialized to keep that
    // behaviour visible instead of silently skipping the write.
    real_point3d found; /* uninitialized on purpose, see above */

    if (game_engine_variant.engine.oddball.random_start == 0) {
        game_engine_find_valid_starting_locations((real_point3d *)0, 0.0f, 0.0f, 2, type_filter, 1, &index);
    }

    if (index == -1) {
        int32_t flag_count = (int32_t)global_scenario->netgame_flags.count;
        ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;
        int32_t matching = 0;
        int32_t i;

        for (i = 0; i < flag_count; i++) {
            if (flags[i].type == 2) {
                matching++;
            }
        }

        if (matching != 0) {
            int32_t pick;
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            pick = (int16_t)(((random_seed_global >> 0x10) *
                              (uint32_t)(int32_t)(int16_t)matching) >> 0x10);

            for (i = 0; i < flag_count; i++) {
                if (flags[i].type == 2) {
                    if (pick == 0) {
                        index = i;
                        break;
                    }
                    pick--;
                }
            }
        }
    }

    if (index != -1) {
        ScenarioNetgameFlags *flags = (ScenarioNetgameFlags *)global_scenario->netgame_flags.pointer;

        found.x = flags[index].position.x;
        found.y = flags[index].position.y;
        found.z = flags[index].position.z;
    }
    out_position->x = found.x;
    out_position->y = found.y;
    out_position->z = found.z;
}

#if 0
Original Ghidra decompilation (0x46beb0), from tools/pack.py 0x46beb0:

void FUN_0046beb0(undefined4 *param_1)

{
  int iVar1;
  int iVar2;
  short in_CX;
  short sVar3;
  undefined4 *puVar4;
  int iVar5;
  int local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar1 = global_scenario;
  if (DAT_006f1d04 == '\0') {
    local_10 = -1;
    game_engine_find_valid_starting_locations(0.0,0.0,2,in_CX,1,&local_10);
    if (local_10 != -1) {
LAB_0046bf94:
      puVar4 = (undefined4 *)(*(int *)(iVar1 + 0x37c) + local_10 * 0x94);
      local_c = *puVar4;
      local_8 = puVar4[1];
      local_4 = puVar4[2];
      goto LAB_0046bfb6;
    }
  }
  iVar5 = 0;
  sVar3 = 0;
  if (0 < *(int *)(iVar1 + 0x378)) {
    iVar2 = 0;
    do {
      if (*(short *)(iVar2 * 0x94 + 0x10 + *(int *)(iVar1 + 0x37c)) == 2) {
        iVar5 = iVar5 + 1;
      }
      sVar3 = sVar3 + 1;
      iVar2 = (int)sVar3;
    } while (iVar2 < *(int *)(iVar1 + 0x378));
    if (iVar5 != 0) {
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      sVar3 = 0;
      iVar5 = (int)(short)((random_seed_global >> 0x10) * (int)(short)iVar5 >> 0x10);
      if (0 < *(int *)(iVar1 + 0x378)) {
        iVar2 = 0;
        do {
          if (*(short *)(iVar2 * 0x94 + 0x10 + *(int *)(iVar1 + 0x37c)) == 2) {
            if (iVar5 == 0) {
              local_10 = (int)sVar3;
              if (local_10 != -1) goto LAB_0046bf94;
              break;
            }
            iVar5 = iVar5 + -1;
          }
          sVar3 = sVar3 + 1;
          iVar2 = (int)sVar3;
        } while (iVar2 < *(int *)(iVar1 + 0x378));
      }
    }
  }
LAB_0046bfb6:
  *param_1 = local_c;
  param_1[1] = local_8;
  param_1[2] = local_4;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
