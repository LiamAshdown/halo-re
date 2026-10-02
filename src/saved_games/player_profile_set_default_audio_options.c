// player_profile_set_default_audio_options  (Ghidra: FUN_0053b240, renamed)
// address 0x53b240, size 107 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase4/saved_games_functions.md summary calls this "network/connection-related
// option fields", but every offset touched (0xb78..0xb7f) is the audio block per
// out/phase4/saved_games_types_notes.md (master_volume 0xb78, effects_volume 0xb79,
// music_volume 0xb7a, unknown_b7b..unknown_b7f) -- the same fields and the same machine-class
// gated values player_profile_initialize (0x53a1c0) sets inline; trusting the header's field
// table over the summary's guess.
// register convention: profile in EAX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t safe_mode; // 0x007196f4, read as a dword (src/math/math_initialize.c name); nonzero selects the low-end defaults
extern uint32_t cpu_speed; // machine class threshold
extern uint32_t physical_memory; // machine class threshold

// blam-cc: profile in EAX
uint8_t player_profile_set_default_audio_options(saved_player_profile *profile)
{
    if (safe_mode == 0 && 1000 < cpu_speed && 0x80 < physical_memory) {
        profile->sound_quality = 1;
        profile->sound_variety = 2;
    } else {
        profile->sound_quality = 0;
        profile->sound_variety = 1;
    }
    profile->unknown_b7e = 0;
    profile->eax_enabled = 0;
    profile->hardware_acceleration = 0;
    profile->music_volume = 6;
    profile->effects_volume = 10;
    profile->master_volume = 10;
    return 1;
}

#if 0
Original Ghidra decompilation (0x53b240):

undefined4 FUN_0053b240(void)

{
  int in_EAX;

  if (((DAT_007196f4 == 0) && (1000 < DAT_00722bac)) && (0x80 < DAT_00722ba8)) {
    *(undefined1 *)(in_EAX + 0xb7d) = 1;
    *(undefined1 *)(in_EAX + 0xb7f) = 2;
  }
  else {
    *(undefined1 *)(in_EAX + 0xb7d) = 0;
    *(undefined1 *)(in_EAX + 0xb7f) = 1;
  }
  *(undefined1 *)(in_EAX + 0xb7e) = 0;
  *(undefined1 *)(in_EAX + 0xb7c) = 0;
  *(undefined1 *)(in_EAX + 0xb7b) = 0;
  *(undefined1 *)(in_EAX + 0xb7a) = 6;
  *(undefined1 *)(in_EAX + 0xb79) = 10;
  *(undefined1 *)(in_EAX + 0xb78) = 10;
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
