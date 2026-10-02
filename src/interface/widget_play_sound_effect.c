// widget_play_sound_effect  (Ghidra: widget_play_sound_effect, already named)
// address 0x498e90, size 114 bytes
// name confidence: 0.65   rewrite confidence: 0.6
// evidence: matches the given name exactly; looks up one of the four ui sound effect tags
// (group 'snd!', register EDI, per tag_lookup's real signature in src/cache/tag_lookup.c) by a
// caller-supplied selector and tail-jumps into widget_play_sound_effect_tag to actually queue it.
// register convention: selector in AX (in_AX, unresolved register read).
// blam-cc: AX -> effect_id
// The switch is one-based (0x498e90..0x498e97, dec eax; cmp eax,3; ja default): 1 cursor,
// 2 forward, 3 back, 4 flag_failure, anything else is a no-op. types/interface.h ui_sound_effect
// now carries these one-based values.
// UNSURE: widget_play_sound_effect_tag (0x49bdd0) is reached by `jmp`, not `call` (a tail call): the sound tag
// id is handed to it in EAX exactly as tag_lookup returned it, with no other visible argument.
// Declared here as an ordinary call since C has no manual tail-call syntax; behaviourally
// identical.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern void widget_play_sound_effect_tag(datum_index sound_tag); // 0x49bdd0, UNSURE signature
// blam-cc: EAX -> sound_tag

// blam-cc: AX -> effect_id
// Plays one of the standard UI sound effects (cursor move, forward, back, or failure), selected
// by a one-based id; any other id is a silent no-op.
void widget_play_sound_effect(int16_t effect_id)
{
    datum_index sound_tag;

    switch (effect_id) {
    case 1:
        sound_tag = tag_lookup(0x736e6421 /* 'snd!' */, (char *)"sound\\sfx\\ui\\cursor");
        widget_play_sound_effect_tag(sound_tag);
        return;
    case 2:
        sound_tag = tag_lookup(0x736e6421 /* 'snd!' */, (char *)"sound\\sfx\\ui\\forward");
        widget_play_sound_effect_tag(sound_tag);
        return;
    case 3:
        sound_tag = tag_lookup(0x736e6421 /* 'snd!' */, (char *)"sound\\sfx\\ui\\back");
        widget_play_sound_effect_tag(sound_tag);
        return;
    case 4:
        sound_tag = tag_lookup(0x736e6421 /* 'snd!' */, (char *)"sound\\sfx\\ui\\flag_failure");
        widget_play_sound_effect_tag(sound_tag);
        return;
    default:
        return;
    }
}

#if 0
Original Ghidra decompilation (0x498e90):

void widget_play_sound_effect(void)

{
  undefined2 in_AX;

  switch(in_AX) {
  case 1:
    tag_lookup("sound\\sfx\\ui\\cursor");
    FUN_0049bdd0();
    return;
  case 2:
    tag_lookup("sound\\sfx\\ui\\forward");
    FUN_0049bdd0();
    return;
  case 3:
    tag_lookup("sound\\sfx\\ui\\back");
    FUN_0049bdd0();
    return;
  case 4:
    tag_lookup("sound\\sfx\\ui\\flag_failure");
    FUN_0049bdd0();
    return;
  default:
    return;
  }
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
