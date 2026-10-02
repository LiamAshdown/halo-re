// encounters_update  (Ghidra: encounters_update; named for this rewrite)
// address 0x435e00, size 246 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: phase-4 summary ("per-tick top-level dispatcher that iterates live squads and,
//   on a staggered schedule, invokes each squad's morale, timer-decay, reinforcement, and
//   target-selection update routines"). Every callee is an already-rewritten per-encounter
//   update in this directory (encounter_advance_grenade_timers,
//   encounter_process_squad_reinforcements, encounter_decay_squad_spawn_delays,
//   encounter_update_platoon_defending_flag, encounter_redistribute_squads_toward_targets,
//   encounter_propagate_platoon_state_to_actors), all taking the encounter index.
// register convention: no arguments. Its encounter_iterator runs with active_only set, so
//   only encounters whose units_active flag is set are visited.
//
// UNSURE: encounter_advance_grenade_timers (0x438db0) is shown by Ghidra with no argument at
// all; it is written here as taking the same encounter index, which is the signature its own
// rewrite in this directory declares.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_time_globals *game_time; // 0x006f1d6c
extern ai_globals *ai_globals_ptr;   // 0x00880354
extern data_array *encounter_data;   // 0x008802c8

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern void encounters_recompute_dirty(void);                                      // 0x435f00
extern void encounters_update_activation(void);                                    // 0x437e20
extern void encounter_recompute_morale(datum_index encounter_index);               // 0x437940
extern void encounter_advance_grenade_timers(datum_index encounter_index);         // 0x438db0
extern void encounter_process_squad_reinforcements(datum_index encounter_index);   // 0x4390a0
extern void encounter_decay_squad_spawn_delays(datum_index encounter_index);        // 0x4392f0
extern void encounter_update_platoon_defending_flag(datum_index encounter_index);  // 0x4393b0
extern void encounter_redistribute_squads_toward_targets(datum_index encounter_index); // 0x4394a0
extern void encounter_propagate_platoon_state_to_actors(datum_index encounter_index);  // 0x439d80

// The per-tick encounter pass. Every 30 ticks it flushes the dirty-encounter recompute and
// re-evaluates which encounters should be active; every tick it runs the full per-encounter
// update for the one fifteenth of the live encounters whose index matches this tick.
void encounters_update(void)
{
    int32_t tick;
    encounter_iterator iterator;
    encounter *enc;

    tick = game_time->game_time;
    if (tick % 0x1e == 0) {
        encounters_recompute_dirty();
        encounters_update_activation();
    }

    if (ai_globals_ptr->actors_valid != 0) {
        iterator.data = encounter_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)encounter_data ^ 0x69746572;
        iterator.active_only = 1;
    }

    for (;;) {
        if (ai_globals_ptr->actors_valid == 0) {
            return;
        }
        do {
            enc = (encounter *)data_iterator_next((data_iterator *)&iterator);
            if (enc == 0 || iterator.active_only == 0) {
                break;
            }
        } while (enc->units_active == 0);
        iterator.encounter_index = iterator.index;
        if (enc == 0) {
            return;
        }
        if ((int16_t)((uint32_t)(iterator.encounter_index & 0xffff) % 0xf) ==
            (int16_t)(tick % 0xf)) {
            encounter_recompute_morale(iterator.encounter_index);
            encounter_advance_grenade_timers(iterator.encounter_index);
            encounter_process_squad_reinforcements(iterator.encounter_index);
            encounter_decay_squad_spawn_delays(iterator.encounter_index);
            encounter_update_platoon_defending_flag(iterator.encounter_index);
            encounter_redistribute_squads_toward_targets(iterator.encounter_index);
            encounter_propagate_platoon_state_to_actors(iterator.encounter_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x435e00):

void FUN_00435e00(void)

{
  int iVar1;
  int iVar2;
  uint local_14;
  char local_8;

  iVar1 = *(int *)(DAT_006f1d6c + 0xc);
  if (iVar1 % 0x1e == 0) {
    FUN_00435f00();
    FUN_00437e20();
  }
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    local_14 = 0xffffffff;
    local_8 = '\x01';
  }
  do {
    if (*(char *)(DAT_00880354 + 1) == '\0') {
      return;
    }
    do {
      iVar2 = data_iterator_next();
      if ((iVar2 == 0) || (local_8 == '\0')) break;
    } while (*(char *)(iVar2 + 0xd) == '\0');
    if (iVar2 == 0) {
      return;
    }
    if ((short)((ulonglong)(local_14 & 0xffff) % 0xf) == (short)(iVar1 % 0xf)) {
      FUN_00437940(local_14);
      FUN_00438db0();
      FUN_004390a0(local_14);
      FUN_004392f0(local_14);
      FUN_004393b0(local_14);
      FUN_004394a0(local_14);
      FUN_00439d80(local_14);
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
