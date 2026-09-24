// control_profile_reestablish_device_slot_mappings  (Ghidra: FUN_0053b620, renamed)
// address 0x53b620, size 131 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/saved_games_functions.md summary "Re-establishes device-to-control-
// profile-slot mappings for all connected joysticks/devices." Clears any existing mappings for
// this profile first (control_profile_clear_device_slot_mappings, 0x53b5a0, this session's
// sibling file), then, for each used gamepad slot (0..3, safely bounded here unlike its
// sibling), establishes a fresh device<->slot mapping if both directions are currently free.
// register convention: profile in EAX.

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
extern void control_profile_clear_device_slot_mappings(saved_player_profile *profile); // 0x53b5a0, this module

// blam-cc: profile in EAX
void control_profile_reestablish_device_slot_mappings(saved_player_profile *profile)
{
    int32_t slot;
    int16_t device_index;

    if (profile == 0) {
        return;
    }
    control_profile_clear_device_slot_mappings(profile);

    for (slot = 0; slot < k_control_gamepad_count; slot = slot + 1) {
        if (profile->gamepads[slot].name[0] != 0) {
            device_index = input_device_find_index_by_guid(&profile->gamepads[slot]);
            if (device_index != -1 && device_index < input_gamepad_count &&
                input_device_to_slot[device_index * 0x90] == -1 &&
                input_slot_to_device[slot] == -1) {
                input_device_to_slot[device_index * 0x90] = slot;
                input_slot_to_device[slot] = device_index;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x53b620):

void FUN_0053b620(void)

{
  int in_EAX;
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  short *psVar5;

  if (in_EAX != 0) {
    FUN_0053b5a0();
    iVar4 = 0;
    psVar5 = (short *)(in_EAX + 0x1108);
    do {
      if (((-1 < iVar4) && (iVar4 < 4)) && (*psVar5 != 0)) {
        uVar1 = input_device_find_index_by_guid((int)psVar5);
        if ((((short)uVar1 != -1) && (iVar2 = (int)(short)uVar1, iVar2 < DAT_006b1844)) &&
           (((&DAT_006b1a98)[iVar2 * 0x90] == -1 &&
            (iVar3 = (int)(short)iVar4, (&DAT_006b2ce8)[iVar3] == -1)))) {
          (&DAT_006b1a98)[iVar2 * 0x90] = iVar3;
          (&DAT_006b2ce8)[iVar3] = iVar2;
        }
      }
      iVar4 = iVar4 + 1;
      psVar5 = psVar5 + 0x110;
    } while (iVar4 < 4);
  }
  return;
}
#endif
