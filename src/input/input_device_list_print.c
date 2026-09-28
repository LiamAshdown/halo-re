// input_device_list_print  (Ghidra: already named)
// address 0x491750, size 335 bytes
// name confidence: 0.65   rewrite confidence: 0.55
// evidence: out/phase4/input_functions.md summary "Enumerates all registered input devices and
// logs each one's index, GUID string, and device name."; confirmed unreachable in this build (0
// callers per out/phase4/input_batch/491750.md). types/interface.h controls_gamepad_record
// (name[0x106], product_guid.words[5]) is exactly the 0x88-dword block copied out of each input_device.
// UNSURE: the decompiled `else if (local_3d5 != '\0') goto LAB_004917aa;` branch is guarded by
// `iVar5 < DAT_006b1844`, which is identical to the enclosing loop's own `sVar1 <
// DAT_006b1844` condition (iVar5 is always sVar1), so that branch is unreachable; omitted here
// as dead code, not behaviour.
// UNSURE: `local_1c` (the GUID StringFromGUID2 reads) is never explicitly written in the
// decompile; from the stack-offset gap between it and `local_228` (0x20c bytes) it appears to
// land inside the very controls_gamepad_record the loop just copied to local_228, at record
// offset 0x20c -- exactly where device_key[0..3] (the product GUID) starts. Reproduced as
// printing device_key[0..3] as a GUID, not instance_guid; not independently confirmed.
// register convention: no parameters, no return value.
// reconciled: R20 controls_gamepad_record.device_key[5] -> input_guid product_guid (+0x20c, device_key[0..3]) and int32_t product_instance (+0x21c, device_key[4])

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <wchar.h>
#include <string.h>

extern int32_t input_device_count;    // 0x006b1844
extern input_device input_devices[8]; // 0x006b1868

extern int32_t __stdcall StringFromGUID2(const input_guid *guid, uint16_t *out, int32_t max_chars); // OLE32 IAT
extern void console_printf_verbose(ColorARGB *color, char *format, ...); // interface module, 0x496a80

// Debug/test routine: logs "<index>) deviceid <guid> - <name>" for every registered input
// device. Dead code in this build: nothing calls it.
void input_device_list_print(void)
{
    int32_t index;
    controls_gamepad_record record;
    uint16_t guid_wide[0x27];
    char guid_ascii[0x27];
    char guid_ascii_trimmed[0x26];
    char name_ascii[0x105];
    uint32_t length;
    uint32_t i;
    int32_t hr;

    for (index = 0; index < input_device_count; index++) {
        record = input_devices[index].record;

        hr = StringFromGUID2(&record.product_guid, guid_wide, 0x27);
        if (hr < 0) {
            continue;
        }

        length = (uint32_t)wcslen(guid_wide);
        if (length >= 0x27) {
            continue;
        }
        for (i = 0; i < length; i++) {
            guid_ascii[i] = ((uint8_t *)guid_wide)[i * 2 + 1] == 0 ? ((char *)guid_wide)[i * 2] : ' ';
        }
        guid_ascii[i] = '\0';
        strncpy(guid_ascii_trimmed, guid_ascii, 0x26);

        length = (uint32_t)wcslen(record.name);
        if (length >= 0x105) {
            continue;
        }
        for (i = 0; i < length; i++) {
            name_ascii[i] = ((uint8_t *)record.name)[i * 2 + 1] == 0 ? ((char *)record.name)[i * 2] : ' ';
        }
        name_ascii[i] = '\0';

        console_printf_verbose((ColorARGB *)0, "%d) deviceid %s - %s", index, guid_ascii_trimmed, name_ascii);
    }
}

#if 0
Original Ghidra decompilation (0x491750):

void input_device_list_print(void)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  char local_3d5;
  char local_3d0 [40];
  char local_3a8 [38];
  undefined1 local_382;
  undefined2 local_380;
  undefined1 local_330 [264];
  undefined4 local_228;
  GUID local_1c;

  sVar1 = 0;
  if (0 < (short)DAT_006b1844) {
    do {
      iVar5 = (int)sVar1;
      if (iVar5 < DAT_006b1844) {
        puVar6 = &DAT_006b1868 + iVar5 * 0x90;
        puVar7 = &local_228;
        for (iVar3 = 0x88; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar7 = *puVar6;
          puVar6 = puVar6 + 1;
          puVar7 = puVar7 + 1;
        }
        local_3d5 = '\x01';
LAB_004917aa:
        iVar3 = StringFromGUID2(&local_1c,&local_380,0x27);
        if (-1 < iVar3) {
          uVar2 = FUN_00625b7a(&local_380);
          if (uVar2 < 0x27) {
            uVar4 = 0;
            if (uVar2 != 0) {
              do {
                if (*(char *)((int)&local_380 + uVar4 * 2 + 1) == '\0') {
                  local_3d0[uVar4] = *(char *)(&local_380 + uVar4);
                }
                else {
                  local_3d0[uVar4] = ' ';
                }
                uVar4 = uVar4 + 1;
              } while (uVar4 < uVar2);
            }
            local_3d0[uVar4] = '\0';
            _strncpy(local_3a8,local_3d0,0x26);
            local_382 = 0;
            uVar2 = FUN_00625b7a(&local_228);
            if (uVar2 < 0x105) {
              uVar4 = 0;
              if (uVar2 != 0) {
                do {
                  if (*(char *)((int)&local_228 + uVar4 * 2 + 1) == '\0') {
                    local_330[uVar4] = *(undefined1 *)((int)&local_228 + uVar4 * 2);
                  }
                  else {
                    local_330[uVar4] = 0x20;
                  }
                  uVar4 = uVar4 + 1;
                } while (uVar4 < uVar2);
              }
              local_330[uVar4] = 0;
              FUN_00496a80("%d) deviceid %s - %s",iVar5,local_3a8,local_330);
            }
          }
        }
      }
      else if (local_3d5 != '\0') goto LAB_004917aa;
      sVar1 = sVar1 + 1;
    } while (sVar1 < (short)DAT_006b1844);
  }
  return;
}
#endif
