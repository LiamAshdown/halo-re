// control_profile_gamepad_slot_find  (Ghidra: FUN_0053b6b0, renamed)
// address 0x53b6b0, size 62 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/saved_games_types_notes.md "0x53b6b0 (+0x21c then +0x20c..+0x218)":
// walks profile->gamepads[0..3], matching product_instance (the extra dword at controls_gamepad_record
// +0x21c) and then the 16-byte device_key[0..3] (+0x20c..+0x21c) against a caller-supplied key
// record. Confirmed against objdump 0x53b6b0..0x53b6f0: `add edx,0x1314` is exactly
// offsetof(saved_player_profile, gamepads) + offsetof(controls_gamepad_record, device_key)
// (0x1108 + 0x20c), the `repz cmps` compares 4 dwords (16 bytes) at device_key[0..3], and the
// stride `add edx,0x220` is sizeof(controls_gamepad_record).
// register convention: profile in EDX, key record in EBX (confirmed by objdump: no stack args,
// EAX/ECX are locals, EDX and EBX are live-in / never assigned before first use).
// reconciled: R20 controls_gamepad_record.device_key[5] -> input_guid product_guid (+0x20c, device_key[0..3]) and int32_t product_instance (+0x21c, device_key[4])

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

// blam-cc: profile in EDX, key in EBX
// Searches the profile's four gamepad slots for one whose device_key matches key's device_key
// (the extra dword at [4] first, then the 16-byte guid at [0..3]). Returns the matching slot
// index (0..3), or -1 if none match.
int32_t control_profile_gamepad_slot_find(saved_player_profile *profile, controls_gamepad_record *key)
{
    int32_t i;
    controls_gamepad_record *slot;

    for (i = 0; i < k_control_gamepad_count; i++) {
        slot = &profile->gamepads[i];
        if (slot->product_instance == key->product_instance &&
            slot->product_guid.words[0] == key->product_guid.words[0] &&
            slot->product_guid.words[1] == key->product_guid.words[1] &&
            slot->product_guid.words[2] == key->product_guid.words[2] &&
            slot->product_guid.words[3] == key->product_guid.words[3]) {
            return i;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x53b6b0):

int FUN_0053b6b0(void)

{
  int iVar1;
  int iVar2;
  int in_EDX;
  int *piVar3;
  int unaff_EBX;
  int *piVar4;
  int *piVar5;
  bool bVar6;

  iVar1 = 0;
  piVar3 = (int *)(in_EDX + 0x1314);
  do {
    if (piVar3[4] == *(int *)(unaff_EBX + 0x21c)) {
      iVar2 = 4;
      bVar6 = true;
      piVar4 = piVar3;
      piVar5 = (int *)(unaff_EBX + 0x20c);
      do {
        if (iVar2 == 0) break;
        iVar2 = iVar2 + -1;
        bVar6 = *piVar4 == *piVar5;
        piVar4 = piVar4 + 1;
        piVar5 = piVar5 + 1;
      } while (bVar6);
      if (bVar6) {
        return iVar1;
      }
    }
    iVar1 = iVar1 + 1;
    piVar3 = piVar3 + 0x88;
    if (3 < iVar1) {
      return -1;
    }
  } while( true );
}
#endif
