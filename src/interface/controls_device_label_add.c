// controls_device_label_add  (Ghidra: FUN_004b4830, still unnamed there; named here per
// src/interface/README.md's own calling-convention table, which already documents this
// exact call: "controls_device_label_add 0x4b4830 | wcsncpy(base + i*0x210, name, 0x104);
// terminator short at +0x208; id dword at +0x20c")
// address 0x4b4830, size 92 bytes
// name confidence: 0.6 (chosen, matches README)   rewrite confidence: 0.7
// phase-4 review: checked against objdump 0x4b4830..0x4b488b (the name[0x104] terminator fix is the only change).
// evidence: out/phase4/interface_functions.md "Registers a new named input-device entry in
// the controls-menu device label table."; types/interface.h controls_device_label (name
// wcsncpy limit 0x104, terminator short at +0x208, device_type at +0x20c); table bound
// 0x006932e8..0x006953e8 (16 entries) matches the loop's own `< 0x6953e8` test.
// register convention: wide name in the one recovered parameter (param_1, stack per
// Ghidra); device type/id as the second stack argument.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern controls_device_label controls_device_labels[0x10]; // 0x006932e8
extern int32_t controls_device_label_count;                 // 0x00719440


void controls_device_label_add(const uint16_t *name, int32_t device_type)
{
    int i;
    for (i = 0; i < 0x10; i++) {
        if (controls_device_labels[i].name[0] == 0) {
            wcsncpy(controls_device_labels[i].name, name, 0x104);
            controls_device_labels[i].name[0x104] = 0; // objdump 0x4b4876: word store at +0x208
            controls_device_labels[i].device_type = device_type;
            controls_device_label_count++;
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4b4830):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004b4830(wchar_t *param_1,undefined4 param_2)

{
  short *psVar1;
  int iVar2;

  iVar2 = 0;
  psVar1 = (short *)&DAT_006932e8;
  do {
    if (*psVar1 == 0) {
      _wcsncpy((wchar_t *)(&DAT_006932e8 + iVar2 * 0x84),param_1,0x104);
      *(undefined2 *)(iVar2 * 0x210 + 0x6934f0) = 0;
      *(undefined4 *)(&DAT_006934f4 + iVar2 * 0x210) = param_2;
      _DAT_00719440 = _DAT_00719440 + 1;
      return;
    }
    psVar1 = psVar1 + 0x108;
    iVar2 = iVar2 + 1;
  } while ((int)psVar1 < 0x6953e8);
  return;
}
#endif
