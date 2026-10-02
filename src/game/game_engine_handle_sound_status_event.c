// game_engine_handle_sound_status_event  (Ghidra: FUN_0046bca0; named per this rewrite)
// address 0x46bca0, size 84 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: out/phase4/game_functions.md ("Checks whether a pending flag is unset and a valid
//   multiplayer-sound table entry exists for the given index, and if so triggers local sound
//   playback; otherwise clears/defers"); types/tags.h GlobalsMultiplayerInformation::sounds
//   (TagReflexive, count/pointer at +0x5c/+0x60, GlobalsSound stride 0x10, TagDependency at
//   +0x0c per types/game.h's own evidence note); identical validate/reject event shape to
//   game_engine_apply_partial_round_reset_message.c (0x468320, this batch) and the already-
//   committed game_engine_dispatch_end_game_notification.c (message_delta_decode_compound_field EAX->event,
//   message_delta_decode_compound_field_staged EAX->event).
// register convention: `event` in EAX; sound index in ECX.
//   // blam-cc: EAX -> event, ECX -> sound_index
// UNSURE: sound_start_unspatialized's exact identity (a local sound-playback trigger, argument 1.0f looks
//   like a volume/gain).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Globals *global_globals; // 0x00746fa0

extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590
extern void message_delta_decode_compound_field_staged(void *event);                       // 0x4ec670
extern datum_index sound_start_unspatialized(datum_index definition_index, float scale); // 0x543dd0, EDX definition, stack scale

// blam-cc: EAX -> event, ECX -> sound_index
// If `event` validates, and the map has multiplayer sound information, and `sound_index` names
// a GlobalsSound entry with a real tag reference, plays that sound locally at full volume.
// Otherwise forwards `event` to message_delta_decode_compound_field_staged.
// FIXED 2026-09-28 (networking call audit, from the disassembly 0x46bca0..0x46bcf4): the sound index is the decoded
// value itself (there is no ECX argument), and the sound's tag (+0xc of its 0x10-byte entry) goes to
// sound_start_unspatialized in EDX.
void game_engine_handle_sound_status_event(void *event)
{
    if (*(int32_t *)*(void **)event == 0) {
        int32_t sound_index;
        if (message_delta_decode_compound_field(event, &sound_index) != 0) {
            GlobalsMultiplayerInformation *mp_info =
                (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
            if (mp_info != (GlobalsMultiplayerInformation *)0 &&
                sound_index < (int32_t)mp_info->sounds.count) {
                uint8_t *sound = (uint8_t *)mp_info->sounds.pointer + sound_index * 0x10;
                if (sound != (uint8_t *)0 && *(int32_t *)(sound + 0xc) != -1) {
                    sound_start_unspatialized(*(datum_index *)(sound + 0xc), 1.0f);
                }
            }
        }
    } else {
        message_delta_decode_compound_field_staged(event);
    }
}

#if 0
Original Ghidra decompilation (0x46bca0), from tools/pack.py 0x46bca0:

void FUN_0046bca0(void)

{
  char cVar1;
  undefined4 *in_EAX;
  int iVar2;
  int in_ECX;

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if ((((cVar1 != '\0') && (iVar2 = *(int *)(DAT_00746fa0 + 0x168), iVar2 != 0)) &&
        (in_ECX < *(int *)(iVar2 + 0x5c))) &&
       ((iVar2 = in_ECX * 0x10 + *(int *)(iVar2 + 0x60), iVar2 != 0 && (*(int *)(iVar2 + 0xc) != -1)
        ))) {
      FUN_00543dd0(0x3f800000);
      return;
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
