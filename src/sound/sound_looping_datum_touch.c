// sound_looping_datum_touch  (Ghidra: FUN_00549f50)
// address 0x549f50, size 76 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Finds a looping-sound datum by reference and
//   stamps it with the current double-buffer flag, when the sound system is active."; matches
//   looping_sound.update_toggle (0x4c, types/sound.h) written from sound_update_toggle
//   (0x00725214); called from src/sound/game_looping_sound_touch_if_valid.c (0x544290).
// register convention: plain __cdecl, reference value as the recognized stack parameter
//   (param_1).
// blam-cc: stack -> reference
// sound_looping_find_by_owner (0x54e5d0) takes the owner key on the stack (checked).
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t sound_initialized;   // 0x00725200
extern uint8_t sound_enabled;       // 0x00725201
extern uint8_t sound_disabled;      // 0x007252b6
extern data_array *looping_sound_data; // 0x00724a50
extern uint8_t sound_update_toggle; // 0x00725214

extern datum_index sound_looping_find_by_owner(int32_t owner); // 0x54e5d0 (stack argument, confirmed)

// blam-cc: stack -> reference
// While the sound system is active, finds the looping_sound datum whose stored reference matches
// `reference` and stamps its update_toggle with the current frame's double-buffer flag, keeping
// it alive for this frame.
void sound_looping_datum_touch(int32_t reference)
{
    if (sound_initialized != 0 && sound_enabled != 0 && sound_disabled == 0) {
        datum_index found = sound_looping_find_by_owner(reference);

        if (found != (datum_index)0xffffffff) {
            ((looping_sound *)looping_sound_data->data)[(uint16_t)found].update_toggle = sound_update_toggle;
        }
    }
}

#if 0
Original Ghidra decompilation (0x549f50):

void FUN_00549f50(undefined4 param_1)

{
  uint uVar1;

  if (((DAT_00725200 != '\0') && (DAT_00725201 != '\0')) && (DAT_007252b6 == '\0')) {
    uVar1 = FUN_0054e5d0(param_1);
    if (uVar1 != 0xffffffff) {
      *(undefined1 *)((uVar1 & 0xffff) * 0xe4 + 0x4c + *(int *)(DAT_00724a50 + 0x34)) = DAT_00725214
      ;
    }
  }
  return;
}

Disassembly (0x549f50..0x549f9c, capstone; phase-4 review):

0x549f50: mov al, byte ptr [0x725200]
0x549f55: test al, al
0x549f57: je 0x549f9b
0x549f59: mov al, byte ptr [0x725201]
0x549f5e: test al, al
0x549f60: je 0x549f9b
0x549f62: mov al, byte ptr [0x7252b6]
0x549f67: test al, al
0x549f69: jne 0x549f9b
0x549f6b: mov eax, dword ptr [esp + 4]
0x549f6f: push eax
0x549f70: call 0x54e5d0
0x549f75: add esp, 4
0x549f78: cmp eax, -1
0x549f7b: je 0x549f9b
0x549f7d: mov ecx, dword ptr [0x724a50]
0x549f83: mov edx, dword ptr [ecx + 0x34]
0x549f86: mov cl, byte ptr [0x725214]
0x549f8c: and eax, 0xffff
0x549f91: imul eax, eax, 0xe4
0x549f97: mov byte ptr [eax + edx + 0x4c], cl
0x549f9b: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
