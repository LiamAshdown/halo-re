// control_profile_fill_default_gamepad_slots  (Ghidra: FUN_0053b7f0, renamed)
// address 0x53b7f0, size 436 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: out/phase4/saved_games_functions.md summary "Fills any empty controller/profile
// slots (up to 4) with default profile templates from a global table until either all slots are
// used or the template list is exhausted."; out/phase4/saved_games_functions.md lists
// player_profile_initialize (0x53a1c0) as a caller (out/phase2/saved_games/00.md callees of
// 0x53a1c0 include 0053b7f0:FUN_0053b7f0). The connected-device table at 0x006b1868 (stride
// 0x240, count at 0x006b1844) and its control_profile_find_or_create_gamepad_slot (0x53b470) consumer are the same ones
// src/interface/controls_gamepad_lists_load.c already documents; that file's `input_devices` /
// `input_device_count` externs and its have-entry-flag idiom (for the same
// uninitialised-stack-byte pattern seen here) are reused verbatim.
// Phase 4 review: matched objdump 0x53b7f0..0x53b9a3, including the uninitialised have-entry byte
// ([esp+0x17]) shared by both passes.
// register convention: profile pointer as the single stack argument (confirmed by objdump
// 0x53b7f0..0x53b820: `mov edx,[ebp+0x8]` reads the one and only caller-supplied value).
// UNSURE: local_2231 / local_2008's first dword are genuinely uninitialised stack in the
// binary; both are modeled as starting 0, matching the convention already used in
// src/interface/controls_gamepad_lists_load.c for the identical pattern.
// reconciled: R78 0x006b1844 input_gamepad_count(_dword) -> input.h int32_t input_device_count; the WORD readers keep their int16 width through an (int16_t) cast
// reconciled: R20 controls_gamepad_record.device_key[5] -> input_guid product_guid (+0x20c, device_key[0..3]) and int32_t product_instance (+0x21c, device_key[4])

#include <string.h>
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

extern int32_t input_device_count; // 0x006b1844, input.h (0..8 connected input devices)
extern uint8_t input_devices[]; // 0x006b1868, stride 0x240; UNSURE name

extern int32_t input_device_default_profile_tag_find(input_guid guid, uint8_t *out_profile); // 0x490110
extern uint8_t control_profile_find_or_create_gamepad_slot(controls_gamepad_record *entry, saved_player_profile *profile); // 0x53b470, saved_games (below this batch)

// blam-cc: profile pointer as the one stack argument
// Counts the profile's already-used gamepad slots (0..4, by the first word of each of the four
// controls_gamepad_record entries at +0x1108). While fewer than 4 are used, walks the connected
// device table once trying to add each device whose GUID resolves to a device_defaults tag
// (input_device_default_profile_tag_find), then, if slots are still free, walks the table again
// unconditionally. Both passes hand the copied device record to control_profile_find_or_create_gamepad_slot, which finds or
// reuses a free profile slot and stores the device's binding data into it.
void control_profile_fill_default_gamepad_slots(saved_player_profile *profile)
{
    uint32_t used_count;
    int32_t device_count;
    int32_t i;
    controls_gamepad_record entry;
    uint8_t have_entry;
    int32_t tag_index;
    uint8_t added;
    input_guid key;
    uint8_t tag_scratch[0x1ffc]; // discarded output of input_device_default_profile_tag_find;
                                 // in the binary this overflows into the tail of a large stack
                                 // buffer that is never read back (see the address evidence above)

    used_count = 0;
    if (0 < (int16_t)input_device_count) {
        if (profile != 0) {
            used_count = (profile->gamepads[0].name[0] != 0);
            if (profile->gamepads[1].name[0] != 0) {
                used_count = used_count + 1;
            }
            if (profile->gamepads[2].name[0] != 0) {
                used_count = used_count + 1;
            }
            if (profile->gamepads[3].name[0] != 0) {
                used_count = used_count + 1;
            }
        }
        device_count = (int32_t)(int16_t)input_device_count;

        // pass 1: only devices with a matching device_defaults tag
        i = 0;
        have_entry = 0; // UNSURE: uninitialised stack byte in the binary
        if (0 < device_count) {
            do {
                if (3 < (int32_t)used_count) {
                    break;
                }
                if ((int16_t)i < input_device_count) {
                    memcpy(&entry, input_devices + (int16_t)i * 0x240, sizeof(entry));
                    have_entry = 1;
pass1_try_add:
                    key = entry.product_guid;
                    tag_index = input_device_default_profile_tag_find(key, tag_scratch);
                    if (tag_index != -1) {
                        added = control_profile_find_or_create_gamepad_slot(&entry, profile);
                        if (added != 0) {
                            used_count = used_count + 1;
                        }
                    }
                } else if (have_entry != 0) {
                    goto pass1_try_add;
                }
                i = i + 1;
            } while (i < device_count);
        }

        // pass 2: every remaining device, unconditionally
        i = 0;
        have_entry = 0; // UNSURE: uninitialised stack byte in the binary
        if (0 < device_count) {
            do {
                if (3 < (int32_t)used_count) {
                    return;
                }
                if ((int16_t)i < input_device_count) {
                    memcpy(&entry, input_devices + (int16_t)i * 0x240, sizeof(entry));
                    have_entry = 1;
pass2_try_add:
                    added = control_profile_find_or_create_gamepad_slot(&entry, profile);
                    if (added != 0) {
                        used_count = used_count + 1;
                    }
                } else if (have_entry != 0) {
                    goto pass2_try_add;
                }
                i = i + 1;
            } while (i < device_count);
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x53b7f0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_0053b7f0(int param_1)

{
  char cVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  uint local_2238;
  char local_2231;
  int local_2230;
  undefined4 local_2228 [131];
  undefined4 local_201c;
  undefined4 local_2018;
  undefined4 local_2014;
  undefined4 local_2010;
  undefined1 local_2008 [8188];
  undefined4 uStack_c;

  uStack_c = 0x53b800;
  local_2238 = 0;
  if (0 < (short)DAT_006b1844) {
    if (param_1 != 0) {
      local_2238 = (uint)(*(short *)(param_1 + 0x1108) != 0);
      if (*(short *)(param_1 + 0x1328) != 0) {
        local_2238 = local_2238 + 1;
      }
      if (*(short *)(param_1 + 0x1548) != 0) {
        local_2238 = local_2238 + 1;
      }
      if (*(short *)(param_1 + 0x1768) != 0) {
        local_2238 = local_2238 + 1;
      }
    }
    iVar3 = (int)(short)DAT_006b1844;
    local_2230 = 0;
    cVar2 = local_2231;
    if (0 < iVar3) {
      do {
        if (3 < (int)local_2238) break;
        if ((short)local_2230 < DAT_006b1844) {
          puVar6 = &DAT_006b1868 + (short)local_2230 * 0x90;
          puVar7 = local_2228;
          for (iVar4 = 0x88; iVar4 != 0; iVar4 = iVar4 + -1) {
            *puVar7 = *puVar6;
            puVar6 = puVar6 + 1;
            puVar7 = puVar7 + 1;
          }
          cVar2 = '\x01';
LAB_0053b8c7:
          iVar4 = input_device_default_profile_tag_find
                            (local_201c,local_2018,local_2014,local_2010,local_2008);
          if ((iVar4 != -1) && (cVar1 = FUN_0053b470(local_2228,param_1), cVar1 != '\0')) {
            local_2238 = local_2238 + 1;
          }
        }
        else if (cVar2 != '\0') goto LAB_0053b8c7;
        local_2230 = local_2230 + 1;
      } while (local_2230 < iVar3);
    }
    iVar4 = 0;
    if (0 < iVar3) {
      do {
        if (3 < (int)local_2238) {
          return;
        }
        if ((short)iVar4 < DAT_006b1844) {
          puVar6 = &DAT_006b1868 + (short)iVar4 * 0x90;
          puVar7 = local_2228;
          for (iVar5 = 0x88; iVar5 != 0; iVar5 = iVar5 + -1) {
            *puVar7 = *puVar6;
            puVar6 = puVar6 + 1;
            puVar7 = puVar7 + 1;
          }
          local_2231 = '\x01';
LAB_0053b97e:
          cVar2 = FUN_0053b470(local_2228,param_1);
          if (cVar2 != '\0') {
            local_2238 = local_2238 + 1;
          }
        }
        else if (local_2231 != '\0') goto LAB_0053b97e;
        iVar4 = iVar4 + 1;
      } while (iVar4 < iVar3);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
