// control_profile_clear_device_slot_mappings  (Ghidra: FUN_0053b5a0, renamed)
// address 0x53b5a0, size 122 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/saved_games_functions.md summary "Clears all joystick/device-to-control-
// profile-slot mappings that reference a given profile array." out/phase4/saved_games_types_notes.md's
// cross-module globals note: "0x006b1844/0x006b1868/0x006b1a98/0x006b2ce8 input device count,
// table (stride 0x240...), device -> slot and slot -> device"; the 0x90-dword (0x240-byte)
// index into input_device_to_slot here matches that stride.
// register convention: profile in ECX.
// UNSURE: the loop iterates the connected-device count (0x006b1844) while advancing through
// profile->gamepads at the gamepad stride (0x220 bytes) -- reproduced exactly; if more than 4
// devices are connected this walks past the 4-entry gamepads array into the profile's own
// trailing bytes, matching the original binary's own behaviour (not "fixed" here).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int16_t input_gamepad_count; // 0x006b1844
extern int32_t input_device_to_slot[]; // 0x006b1a98, stride 0x90 dwords (0x240 bytes) per device
extern int32_t input_slot_to_device[]; // 0x006b2ce8

extern int16_t input_device_find_index_by_guid(controls_gamepad_record *gamepad); // 0x4916e0, not in this module

// blam-cc: profile in ECX
void control_profile_clear_device_slot_mappings(saved_player_profile *profile)
{
    int32_t count;
    uint8_t *entry;
    int16_t device_index;
    int32_t slot;

    if (profile == 0) {
        return;
    }
    count = input_gamepad_count;
    entry = (uint8_t *)profile + 0x1108;
    while (0 < count) {
        device_index = input_device_find_index_by_guid((controls_gamepad_record *)entry);
        if (device_index != -1 && device_index < input_gamepad_count) {
            slot = input_device_to_slot[device_index * 0x90];
            if (slot != -1) {
                input_device_to_slot[device_index * 0x90] = -1;
                input_slot_to_device[slot] = -1;
            }
        }
        entry = entry + 0x220;
        count = count - 1;
    }
}

#if 0
Original Ghidra decompilation (0x53b5a0):

void FUN_0053b5a0(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int in_ECX;
  int iVar5;

  if (in_ECX != 0) {
    iVar2 = (int)(short)DAT_006b1844;
    if (0 < iVar2) {
      iVar5 = in_ECX + 0x1108;
      do {
        uVar3 = input_device_find_index_by_guid(iVar5);
        if (((short)uVar3 != -1) && (iVar4 = (int)(short)uVar3, iVar4 < DAT_006b1844)) {
          iVar1 = (&DAT_006b1a98)[iVar4 * 0x90];
          if ((iVar1 != -1) && (iVar1 != -1)) {
            (&DAT_006b1a98)[iVar4 * 0x90] = 0xffffffff;
            (&DAT_006b2ce8)[iVar1] = 0xffffffff;
          }
        }
        iVar5 = iVar5 + 0x220;
        iVar2 = iVar2 + -1;
      } while (iVar2 != 0);
    }
  }
  return;
}
#endif
