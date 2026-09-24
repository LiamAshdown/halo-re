// widget_play_sound_effect_tag  (Ghidra: FUN_0049bdd0, renamed)
// renamed from FUN_0049bdd0 in the naming pass
// address 0x49bdd0, size 59 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: functions.md: "Plays a sound effect via sound_play_new provided the caller-supplied
// sound-tag index is valid." widget_play_sound_effect.c (already rewritten, outside this range)
// tail-calls into this address with the sound tag in a register and already fixed its signature.
// register convention: fixed by widget_play_sound_effect.c: EAX -> sound_tag.
// blam-cc: EAX -> sound_tag
// UNSURE: sound_play_new's call here shows zero visible arguments in Ghidra; modeled on the
// 7-argument "play a positioned sound" signature already declared in ui_widget_list_item_activate.c
// (sound_tag, position, then five UNSURE trailing arguments), passing NULL for position and -1
// for the first trailing argument to match that file's other no-position call sites.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern void sound_play_new(datum_index sound_tag, void *position, int32_t unknown1, int32_t unknown2,
                          int32_t unknown3, int32_t unknown4, int32_t unknown5); // 0x549af0, UNSURE signature

// blam-cc: EAX -> sound_tag
void widget_play_sound_effect_tag(datum_index sound_tag)
{
    if (sound_tag != (datum_index)-1) {
        sound_play_new(sound_tag, (void *)0, -1, 0, 0, 0, 0);
    }
}

#if 0
Original Ghidra decompilation (0x49bdd0):

void FUN_0049bdd0(void)

{
  int in_EAX;

  if (in_EAX != -1) {
    FUN_00549af0();
  }
  return;
}
#endif
