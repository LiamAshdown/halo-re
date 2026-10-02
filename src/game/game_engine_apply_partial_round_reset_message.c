// game_engine_apply_partial_round_reset_message  (Ghidra: FUN_00468320; named per this rewrite)
// address 0x468320, size 62 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Validates an incoming network message and, if valid,
//   triggers the object-cleanup/reset pass and an engine callback"); identical validate/reject
//   shape to the already-committed game_engine_dispatch_end_game_notification.c (0x467230):
//   `*(int *)*event == 0` gates a call to message_delta_decode_compound_field(event, out_values) (0x4ec590, blam-cc
//   EAX -> event, ECX -> out_values), and the rejection path forwards to message_delta_decode_compound_field_staged(event)
//   (0x4ec670, blam-cc EAX -> event). On success this runs a subset of
//   game_engine_reset_round_objects.c's (0x468260, this batch) sequence -- bipeds, items,
//   projectiles, then the loaded gametype's own reset_objects hook.
// register convention: `event` in EAX (in_EAX).
//   // blam-cc: EAX -> event
// UNSURE: out_values is passed but never read back here (unlike the stage-dispatch sibling),
//   so its value is a don't-care local scratch slot; message_delta_decode_compound_field/message_delta_decode_compound_field_staged's true purpose
//   is inherited unresolved from that sibling.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_engine_definition *current_game_engine; // 0x006f1d20

extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590
extern void message_delta_decode_compound_field_staged(void *event);                       // 0x4ec670

extern void game_engine_reset_respawns_and_cleanup_bipeds(void); // 0x467e60
extern void game_engine_cleanup_stray_items(void);                // 0x468010, this batch
extern void game_engine_cleanup_stray_projectiles(void);          // 0x467f70

// blam-cc: EAX -> event
// If `event` validates (via message_delta_decode_compound_field), resets respawns/bipeds, deletes stray items and
// stray projectiles, then runs the loaded gametype's own reset_objects hook (if any). Otherwise
// forwards `event` to message_delta_decode_compound_field_staged.
void game_engine_apply_partial_round_reset_message(void *event)
{
    int32_t scratch;

    if (*(int32_t *)*(void **)event == 0) {
        if (message_delta_decode_compound_field(event, &scratch) != 0) {
            game_engine_reset_respawns_and_cleanup_bipeds();
            game_engine_cleanup_stray_items();
            game_engine_cleanup_stray_projectiles();
            if (current_game_engine->reset_objects != 0) {
                ((void (*)(void))current_game_engine->reset_objects)();
            }
        }
    } else {
        message_delta_decode_compound_field_staged(event);
    }
}

#if 0
Original Ghidra decompilation (0x468320), from tools/pack.py 0x468320:

void FUN_00468320(void)

{
  char cVar1;
  undefined4 *in_EAX;

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if (cVar1 != '\0') {
      FUN_00467e60();
      FUN_00468010();
      FUN_00467f70();
      if (*(code **)(DAT_006f1d20 + 0xac) != (code *)0x0) {
        (**(code **)(DAT_006f1d20 + 0xac))();
        return;
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
