// player_profile_apply_audio_options  (Ghidra: FUN_004957d0, unnamed)
// address 0x4957d0, size 409 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/interface_functions.md "Applies the current profile's audio settings
// (music/effects/voice volume and related toggles) to the sound system."; sibling function
// player_profile_apply_video_options.c's precedent for profile_write_back_enabled (0x007196f4)
// and the settings-block-in-register convention.
// register convention: profile settings block in ESI (unaff_ESI). // blam-cc: ESI -> settings
// UNSURE: sound_set_master_gain (the first of three 0..10 gain setters, called with settings+0xb78) is
// not otherwise named or attested; the sound module is well outside this batch. sound_driver_set_quality's
// three boolean/byte arguments (an "enabled" flag gated on three other conditions, a
// settings+0xb7b == 1 flag, and the raw settings+0xb7d byte) are likewise unresolved beyond what
// this one call site shows. DAT_007252e0, DAT_00746120 and DAT_007252b8 are foreign sound-module
// globals named only by role.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t profile_write_back_enabled; // 0x007196f4

extern uint8_t sound_environment_available; // 0x007252e0, UNSURE
extern uint8_t sound_hardware_supports_eax; // 0x00746120, UNSURE
extern uint16_t sound_environment_id;       // 0x007252b8, UNSURE

extern void sound_driver_set_quality(int32_t enabled, uint8_t flag_b7b, uint8_t value_b7d); // 0x5480f0, UNSURE
extern void sound_set_master_gain(float gain);          // 0x548590, UNSURE: first of three gain setters
extern void sound_set_music_gain(float gain);   // 0x548680
extern void sound_set_effects_gain(float gain); // 0x5487b0

// blam-cc: ESI -> settings
// Applies the profile's audio settings: three 0..10 volume sliders (settings+0xb78..0xb7a,
// scaled to 0.0..1.0 and clamped) for the master/effects/music gains, an environment/EAX id
// (settings+0xb7f), and an environment-enable call gated on hardware support plus
// settings+0xb7c, carrying settings+0xb7b and settings+0xb7d along.
void player_profile_apply_audio_options(uint8_t *settings)
{
    float gain;
    int32_t environment_enabled;

    if (profile_write_back_enabled != 0) {
        settings[0xb78] = 10;
        settings[0xb79] = 10;
        settings[0xb7a] = 6;
        settings[0xb7b] = 0;
        settings[0xb7c] = 0;
        settings[0xb7d] = 0;
        settings[0xb7e] = 0;
        settings[0xb7f] = 0;
    }

    gain = (float)settings[0xb78] * 0.1f;
    if (gain < 0.0f) {
        gain = 0.0f;
    } else if (gain > 1.0f) {
        gain = 1.0f;
    }
    sound_set_master_gain(gain);

    gain = (float)settings[0xb79] * 0.1f;
    if (gain < 0.0f) {
        gain = 0.0f;
    } else if (gain > 1.0f) {
        gain = 1.0f;
    }
    sound_set_effects_gain(gain); // 0x5487b0

    gain = (float)settings[0xb7a] * 0.1f;
    if (gain < 0.0f) {
        gain = 0.0f;
    } else if (gain > 1.0f) {
        gain = 1.0f;
    }
    sound_set_music_gain(gain); // 0x548680

    sound_environment_id = settings[0xb7f];
    if (sound_environment_available == 0 || sound_hardware_supports_eax == 0 ||
        settings[0xb7c] == 0) {
        environment_enabled = 0;
    } else {
        environment_enabled = 1;
    }
    sound_driver_set_quality(environment_enabled, settings[0xb7b] == 1, settings[0xb7d]);
}

#if 0
Original Ghidra decompilation (0x4957d0):

void FUN_004957d0(void)

{
  undefined4 uVar1;
  int unaff_ESI;
  float local_4;

  if (DAT_007196f4 != 0) {
    *(undefined1 *)(unaff_ESI + 0xb78) = 10;
    *(undefined1 *)(unaff_ESI + 0xb79) = 10;
    *(undefined1 *)(unaff_ESI + 0xb7a) = 6;
    *(undefined1 *)(unaff_ESI + 0xb7b) = 0;
    *(undefined1 *)(unaff_ESI + 0xb7c) = 0;
    *(undefined1 *)(unaff_ESI + 0xb7d) = 0;
    *(undefined1 *)(unaff_ESI + 0xb7e) = 0;
    *(undefined1 *)(unaff_ESI + 0xb7f) = 0;
  }
  local_4 = (float)*(byte *)(unaff_ESI + 0xb78) * 0.1;
  if (0.0 <= local_4) {
    if (1.0 < local_4) {
      local_4 = 1.0;
    }
  }
  else {
    local_4 = 0.0;
  }
  FUN_00548590(local_4);
  local_4 = (float)*(byte *)(unaff_ESI + 0xb79) * 0.1;
  if (0.0 <= local_4) {
    if (1.0 < local_4) {
      local_4 = 1.0;
    }
  }
  else {
    local_4 = 0.0;
  }
  sound_set_effects_gain(local_4);
  local_4 = (float)*(byte *)(unaff_ESI + 0xb7a) * 0.1;
  if (0.0 <= local_4) {
    if (1.0 < local_4) {
      local_4 = 1.0;
    }
  }
  else {
    local_4 = 0.0;
  }
  sound_set_music_gain(local_4);
  DAT_007252b8 = (ushort)*(byte *)(unaff_ESI + 0xb7f);
  if (((DAT_007252e0 == '\0') || (DAT_00746120 == '\0')) || (*(char *)(unaff_ESI + 0xb7c) == '\0'))
  {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  FUN_005480f0(uVar1,*(char *)(unaff_ESI + 0xb7b) == '\x01',*(undefined1 *)(unaff_ESI + 0xb7d));
  return;
}
#endif
