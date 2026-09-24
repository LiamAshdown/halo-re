// control_profile_reestablish_device_slot_mappings  (Ghidra: FUN_0053b620, renamed)
// address 0x53b620, size 131 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/saved_games_functions.md summary "Re-establishes device-to-control-
// profile-slot mappings for all connected joysticks/devices." Clears any existing mappings for
// this profile first (control_profile_clear_device_slot_mappings, 0x53b5a0, this session's
// sibling file), then, for each used gamepad slot (0..3, safely bounded here unlike its
// sibling), establishes a fresh device<->slot mapping if both directions are currently free.
// register convention: profile in EAX.
// reconciled: R02 0x006b2ce8 input_slot_to_device -> input.h joystick_slot_devices[4]
// reconciled: R78 0x006b1844 input_gamepad_count(_dword) -> input.h int32_t input_device_count; the WORD readers keep their int16 width through an (int16_t) cast
// fixed (reconciliation check): the bounds test against input_device_count is a full DWORD
// compare (mov ecx,ds:0x6b1844; movsx eax,ax; cmp eax,ecx), so it no longer truncates to int16.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t input_device_count; // 0x006b1844, input.h (0..8 connected input devices)
extern int32_t input_device_to_slot[]; // 0x006b1a98, stride 0x90 dwords (0x240 bytes) per device
extern int32_t joystick_slot_devices[4]; // 0x006b2ce8, input.h

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
            if (device_index != -1 && device_index < input_device_count /* DWORD compare */ &&
                input_device_to_slot[device_index * 0x90] == -1 &&
                joystick_slot_devices[slot] == -1) {
                input_device_to_slot[device_index * 0x90] = slot;
                joystick_slot_devices[slot] = device_index;
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
