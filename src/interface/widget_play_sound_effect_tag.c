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

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index,
    sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint); // 0x549af0

// blam-cc: EAX -> sound_tag
void widget_play_sound_effect_tag(datum_index sound_tag)
{
    if (sound_tag != (datum_index)-1) {
        // 0x49bde2: a local sound_location with type 0 (unspatialized), scale 1.0, gain 1.0; the original leaves
        // the rest of the record uninitialized (never read for type 0), zeroed here
        sound_location location;
        memset(&location, 0, sizeof(location));
        location.type = 0;
        location.scale = 1.0f;
        location.gain = 1.0f;
        sound_play_new(sound_tag, &location, -1, 0, 0, 0, 0);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
