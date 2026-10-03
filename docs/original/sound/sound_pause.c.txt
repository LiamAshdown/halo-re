// sound_pause  (Ghidra: FUN_00548170)
// address 0x548170, size 46 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Notifies a registered interface once of a
//   state change (likely device focus/pause) alongside two external update calls."; sound_paused
//   (0x00725202) and sound_driver* (0x00725208, types/sound.h); vtable+0x28 is
//   sound_driver.set_paused(uint8_t); reuses sound_stop_all (0x54adb0, already named by the
//   sibling agent covering that address range) and sound_cache_release_unused (0x443fd0, already
//   named).
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t sound_paused;    // 0x00725202
extern sound_driver *current_sound_driver; // 0x00725208

extern void sound_stop_all(void); // 0x54adb0
extern void sound_cache_release_unused(void); // 0x443fd0

// Stops every playing sound, marks the sound engine paused (notifying the driver once), then
// releases any now-unused sound cache entries.
void sound_pause(void)
{
    sound_stop_all();

    if (sound_paused != 1) {
        sound_paused = 1;
        if (current_sound_driver != 0) {
            current_sound_driver->set_paused(1);
        }
    }

    sound_cache_release_unused();
}

#if 0
Original Ghidra decompilation (0x548170):

void FUN_00548170(void)

{
  FUN_0054adb0();
  if (DAT_00725202 != '\x01') {
    DAT_00725202 = '\x01';
    if (DAT_00725208 != 0) {
      (**(code **)(DAT_00725208 + 0x28))(1);
    }
  }
  sound_cache_release_unused();
  return;
}

Disassembly (0x548170..0x54819e, capstone; phase-4 review):

0x548170: call 0x54adb0
0x548175: mov al, byte ptr [0x725202]
0x54817a: mov ecx, 1
0x54817f: cmp al, cl
0x548181: je 0x548199
0x548183: mov eax, dword ptr [0x725208]
0x548188: test eax, eax
0x54818a: mov byte ptr [0x725202], cl
0x548190: je 0x548199
0x548192: push ecx
0x548193: call dword ptr [eax + 0x28]
0x548196: add esp, 4
0x548199: jmp 0x443fd0
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
