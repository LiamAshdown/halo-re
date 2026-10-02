// input_device_count_by_guid  (Ghidra: FUN_00491d30)
// address 0x491d30, size 57 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: out/phase4/input_types_notes.md names this "input_device_count_by_guid 0x491d30";
// 0x006b1a74 is input_devices[0].record.product_guid.words[0] (0x006b1868 + 0x20c), walked with a 0x90
// dword (0x240 byte) stride, matching input_device's size.
// register convention: guid pointer as the recognized parameter (param_1)
// reconciled: R20 controls_gamepad_record.device_key[5] -> input_guid product_guid (+0x20c, device_key[0..3]) and int32_t product_instance (+0x21c, device_key[4])

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t input_device_count;    // 0x006b1844
extern input_device input_devices[8]; // 0x006b1868

// Counts how many registered input devices have a product GUID (device_key[0..3]) matching the
// one pointed to by guid.
int32_t input_device_count_by_guid(const uint32_t *guid)
{
    int32_t count;
    int32_t i;
    int32_t k;
    uint8_t match;

    count = 0;
    for (i = 0; i < input_device_count; i++) {
        match = 1;
        for (k = 0; k < 4; k++) {
            if (guid[k] != input_devices[i].record.product_guid.words[k]) {
                match = 0;
                break;
            }
        }
        if (match) {
            count = count + 1;
        }
    }
    return count;
}

#if 0
Original Ghidra decompilation (0x491d30):

int FUN_00491d30(int *param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  int *piVar5;
  int *piVar6;
  bool bVar7;

  iVar1 = 0;
  if (0 < DAT_006b1844) {
    piVar3 = &DAT_006b1a74;
    iVar4 = DAT_006b1844;
    do {
      iVar2 = 4;
      bVar7 = true;
      piVar5 = param_1;
      piVar6 = piVar3;
      do {
        if (iVar2 == 0) break;
        iVar2 = iVar2 + -1;
        bVar7 = *piVar5 == *piVar6;
        piVar5 = piVar5 + 1;
        piVar6 = piVar6 + 1;
      } while (bVar7);
      if (bVar7) {
        iVar1 = iVar1 + 1;
      }
      piVar3 = piVar3 + 0x90;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  return iVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
