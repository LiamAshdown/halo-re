// ai_tick_dispatcher  (Ghidra: ai_tick_dispatcher, renamed)
// address 0x42a900, size 74 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: types/ai.h ai_globals.actors_valid (0x01)/initialized (0x00)/unknown_02 (0x02).
//   Calls ai_process_vehicle_entry_queue (0x42bf90), ai_release_actors_and_swarms (0x428ea0)
//   and ai_reset_all_actors_perception (0x429080), all already established/rewritten, plus
//   ai_conversation_update and encounters_update (outside this rewrite's range, UNSURE signatures).
// register convention: plain __cdecl, no parameters (reads the ai_globals global directly).
//   // blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ai_globals *ai_globals_ptr; // 0x00880354

extern void ai_process_vehicle_entry_queue(void); // 0x42bf90
extern void ai_conversation_update(void); // 0x430a70, UNSURE signature, not in this rewrite range
extern void encounters_update(void); // 0x435e00, UNSURE signature, not in this rewrite range
extern void ai_release_actors_and_swarms(void); // 0x428ea0
extern void ai_reset_all_actors_perception(void); // 0x429080

// Top-level AI subsystem tick dispatcher: runs the main per-frame update pipeline (vehicle
// entry queue, two unestablished per-tick passes, then swarm/actor release bookkeeping) the
// first time it sees `initialized`, latching unknown_02 so the next tick instead just
// re-initializes actor perception once.
void ai_tick_dispatcher(void)
{
    if (ai_globals_ptr->actors_valid != 0) {
        ai_process_vehicle_entry_queue();
        if (ai_globals_ptr->ai_active != 0) {
            ai_conversation_update();
            encounters_update();
            ai_release_actors_and_swarms();
            ai_globals_ptr->ai_was_active = 1;
            return;
        }
        if (ai_globals_ptr->ai_was_active != 0) {
            ai_reset_all_actors_perception();
            ai_globals_ptr->ai_was_active = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x42a900):

void FUN_0042a900(void)

{
  char *pcVar1;

  if (DAT_00880354[1] != '\0') {
    ai_process_vehicle_entry_queue();
    pcVar1 = DAT_00880354;
    if (*DAT_00880354 != '\0') {
      FUN_00430a70();
      FUN_00435e00();
      ai_release_actors_and_swarms();
      DAT_00880354[2] = '\x01';
      return;
    }
    if (DAT_00880354[2] != '\0') {
      ai_reset_all_actors_perception();
      pcVar1[2] = '\0';
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
