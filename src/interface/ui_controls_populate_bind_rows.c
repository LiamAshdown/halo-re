// ui_controls_populate_bind_rows  (Ghidra: FUN_004a3180, renamed)
// renamed from FUN_004a3180 in the naming pass
// address 0x4a3180, size 544 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: functions.md: "Decodes a packed bitfield (likely controller action bindings) into six
// list-widget values with per-field enable state." The six blocks are mechanically identical
// (3-bit fields at a fixed stride of 3 bits starting at bit 4), collapsed into one loop here
// rather than transcribed six times; every field/shift/constant matches the original exactly.
// register convention: cdecl, both recognized parameters (widget, the packed bitfield). If the
// bitfield's own low nibble is > 7, game_variant_option_default_by_index is consulted for a replacement 64-bit value
// whose low half becomes the new bitfield and whose high half becomes both the child-search
// "stop" sentinel and a per-row enable flag; otherwise that sentinel/flag stays 0 (an ordinary
// NULL-terminated search, rows always enabled).
// UNSURE: game_variant_option_default_by_index's real signature/purpose is not established anywhere in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern uint64_t game_variant_option_default_by_index(void); // 0x465380, UNSURE signature

static widget_instance *find_row_control_until(widget_instance *row, widget_instance *stop)
{
    widget_instance *control;

    for (control = row->first_child; control != stop && control->widget_type != 2;
         control = control->next_sibling) {
    }
    return control;
}

void ui_controls_populate_bind_rows(widget_instance *widget, uint32_t packed)
{
    widget_instance *stop = (widget_instance *)0;
    uint8_t disabled = (packed & 0xf) > 7;
    uint16_t fallback = 0;
    uint8_t enable_state = 0; // cVar5, low byte of the game_variant_option_default_by_index high-half result
    widget_instance *row = widget->first_child->next_sibling->next_sibling->next_sibling;
    int shift;

    if (!disabled) {
        uint64_t result = game_variant_option_default_by_index();

        stop = (widget_instance *)(int32_t)(uint32_t)(result >> 32);
        packed = (uint32_t)result;
        fallback = (uint16_t)(int32_t)stop;
        enable_state = (uint8_t)(int32_t)stop;
    }

    for (shift = 4; shift <= 19; shift += 3) {
        widget_instance *control = find_row_control_until(row, stop);
        uint8_t field = (uint8_t)(packed >> shift) & 7;

        control->selection_index = (field < 5) ? field : fallback;
        if (disabled == enable_state) {
            row->hidden = 1;
            row->scale = 0.333f;
        } else {
            row->hidden = enable_state;
            row->scale = 1.0f;
        }
        row = row->next_sibling;
    }
}

#if 0
Original Ghidra decompilation (0x4a3180):

void FUN_004a3180(int param_1,uint param_2)

{
  int iVar1;
  int iVar2;
  bool bVar3;
  ushort uVar4;
  char cVar5;
  undefined2 uVar6;
  int iVar7;
  undefined8 uVar8;

  iVar7 = 0;
  bVar3 = 7 < (param_2 & 0xf);
  if (!bVar3) {
    uVar8 = FUN_00465380();
    iVar7 = (int)((ulonglong)uVar8 >> 0x20);
    param_2 = (uint)uVar8;
  }
  iVar1 = *(int *)(*(int *)(*(int *)(*(int *)(param_1 + 0x34) + 0x2c) + 0x2c) + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != iVar7 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  uVar6 = (undefined2)iVar7;
  if (((byte)(param_2 >> 4) & 7) < 5) {
    *(ushort *)(iVar2 + 0x40) = (ushort)(param_2 >> 4) & 7;
  }
  else {
    *(undefined2 *)(iVar2 + 0x40) = uVar6;
  }
  cVar5 = (char)iVar7;
  if (bVar3 == (bool)cVar5) {
    *(undefined1 *)(iVar1 + 0x12) = 1;
    *(undefined4 *)(iVar1 + 0x24) = 0x3eaa7efa;
  }
  else {
    *(char *)(iVar1 + 0x12) = cVar5;
    *(undefined4 *)(iVar1 + 0x24) = 0x3f800000;
  }
  [... five more identical blocks at shifts 7, 10, 13, 0x10, 0x13 ...]
}
#endif
