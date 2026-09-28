// control_binding_table_register_single  (orphan pass 4: FUN_004f37d0, no Ghidra name)
// address 0x4f37d0, size 162 bytes
// name confidence: 0.3 (inferred from behaviour: finds a device-slot row by a target value,
//   then -- unless that device type is already bound on the tested bit -- appends a
//   {raw_id, raw_value} pair into the same 6-row/2-column binding-table layout that
//   control_binding_table_update_a/_b (this pass) index)
// rewrite confidence: 0.3 (control flow and the final table write are confirmed against
//   objdump and cross-checked against the identical arithmetic in
//   control_binding_table_update_a.c's LAB_004f396d path; the device-type bit-test order
//   disagrees with Ghidra's own switch case labels, see UNSURE)
// evidence: out/phase4/objects_types_notes.md / src/objects/README.md: these six functions
//   "read the packed control words at 0x006f1cec/0x006f1ce8 and belong to the input or game
//   module." The row-search table at 0x008603ec (stride 0xa0, bound 0x008607ac) and the
//   {count@+0, id-array@0x8603f0, value-array@0x8603f6} write pattern at the end are the same
//   ones control_binding_table_update_a.c writes via its LAB_004f396d path.
// register convention (confirmed via objdump): search target in EDX (in_EDX), selector
//   (clamped to 0/1) in EAX (in_EAX, preserved across the search in ESI), raw device/control id
//   in EDI (unaff_EDI), raw value in EBX (unaff_EBX, whose low 16 bits are stored and whose
//   bits 8-11 gate the "already bound" test -- see UNSURE).
// Device-type bit test: the jump table at 0x4f3874 (indexed by control_binding_device_type - 1)
//   holds 0x4f380e (shr 9), 0x4f380a (bh, i.e. bit 8), 0x4f381c (shr 0xb), 0x4f3815 (shr 0xa), so
//   types 1..4 test bits 9, 8, 11, 10 -- exactly Ghidra's case labels. (Orphan pass 4 review: the
//   draft read the blocks in address order as bits 8..11 and so swapped 1/2 and 3/4; it also let
//   the row search read one row past 0x008607ac, where the original stops at `cmp eax,0x8607ac;
//   jl`.)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern uint32_t control_binding_device_type; // 0x006f1cb8, UNSURE: 1..4, device type selector

extern uint8_t g_control_binding_region_ec[0x3c0]; // base 0x008603ec, UNSURE: same region as control_binding_table_query.c
extern int32_t g_control_binding_region_e0[6 * 2 * (0x50 / 4)]; // base 0x008603e0, stride 0x50 bytes per (selector, row) cell
extern int32_t g_control_binding_id[];   // base 0x008603f0, stride 8 bytes, UNSURE size
extern int16_t g_control_binding_value[]; // base 0x008603f6, stride 8 bytes, UNSURE size

// blam-cc: EDX -> target, EAX -> selector, EDI -> raw_id, EBX -> raw_value
void control_binding_table_register_single(int32_t target, int32_t selector, int32_t raw_id, uint32_t raw_value)
{
    uint8_t *cursor = g_control_binding_region_ec;                    // base 0x008603ec
    uint8_t *region_end = g_control_binding_region_ec + sizeof(g_control_binding_region_ec); // 0x008607ac
    int32_t row = 0;

    while (*(int32_t *)cursor != target) {
        cursor += 0xa0;
        row++;
        if (cursor >= region_end) { // 0x4f37ea: `cmp eax,0x8607ac; jl` continues only below the end
            return;
        }
    }

    {
        // Registration proceeds when the tested bit of raw_value is set, or unconditionally
        // when control_binding_device_type doesn't match any of the 4 known device types
        // (Ghidra's `default: goto switchD_004f3803_default` jumps straight into the
        // registration block, bypassing the bit test entirely).
        uint32_t do_register;
        switch (control_binding_device_type) {
        case 1: do_register = (raw_value >> 9) & 1; break;  // 0x4f380e
        case 2: do_register = (raw_value >> 8) & 1; break;  // 0x4f380a (`mov al,bh`)
        case 3: do_register = (raw_value >> 11) & 1; break; // 0x4f381c
        case 4: do_register = (raw_value >> 10) & 1; break; // 0x4f3815
        default: do_register = 1; break;
        }
        if (!do_register) return;
    }

    if (row >= 0 && row < 6 && raw_id != -1) {
        int32_t idx;
        int32_t *count_cell;
        int32_t slot;

        if (selector < 0 || selector > 1) selector = 0;
        idx = selector + row * 2;
        count_cell = (int32_t *)((uint8_t *)g_control_binding_region_e0 + idx * 0x50);
        slot = *count_cell + idx * 10;
        *count_cell = *count_cell + 1;
        *(int32_t *)((uint8_t *)g_control_binding_id + slot * 8) = raw_id;
        *(int16_t *)((uint8_t *)g_control_binding_value + slot * 8) = (int16_t)raw_value;
    }
}

#if 0
Original Ghidra decompilation (0x4f37d0):

void FUN_004f37d0(void)

{
  int iVar1;
  byte bVar2;
  int in_EAX;
  int *piVar3;
  int iVar4;
  int in_EDX;
  uint unaff_EBX;
  int unaff_EDI;

  iVar4 = 0;
  piVar3 = &DAT_008603ec;
  while (*piVar3 != in_EDX) {
    piVar3 = piVar3 + 0x28;
    iVar4 = iVar4 + 1;
    if (0x8607ab < (int)piVar3) {
      return;
    }
  }
  if (iVar4 == -1) {
    return;
  }
  switch(DAT_006f1cb8) {
  case 1:
    bVar2 = (byte)(unaff_EBX >> 9);
    break;
  case 2:
    bVar2 = (byte)(unaff_EBX >> 8);
    break;
  case 3:
    bVar2 = (byte)(unaff_EBX >> 0xb);
    break;
  case 4:
    bVar2 = (byte)(unaff_EBX >> 10);
    break;
  default:
    goto switchD_004f3803_default;
  }
  if ((bVar2 & 1) != 0) {
switchD_004f3803_default:
    if (((-1 < iVar4) && (iVar4 < 6)) && (unaff_EDI != -1)) {
      if ((in_EAX < 0) || (1 < in_EAX)) {
        in_EAX = 0;
      }
      iVar4 = in_EAX + iVar4 * 2;
      iVar1 = (&DAT_008603e0)[iVar4 * 0x14] + iVar4 * 10;
      (&DAT_008603e0)[iVar4 * 0x14] = (&DAT_008603e0)[iVar4 * 0x14] + 1;
      (&DAT_008603f0)[iVar1 * 2] = unaff_EDI;
      *(short *)(&DAT_008603f6 + iVar1 * 8) = (short)unaff_EBX;
    }
  }
  return;
}
#endif
