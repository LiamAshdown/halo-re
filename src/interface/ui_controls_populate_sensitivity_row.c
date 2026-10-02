// ui_controls_populate_sensitivity_row  (Ghidra: FUN_004a20f0, renamed)
// renamed from FUN_004a20f0 in the naming pass
// address 0x4a20f0, size 160 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: functions.md: "Populates three list-widget display values from byte fields in an
// unidentified settings/profile structure." types/interface.h's own player_control_settings
// comment pins the "unidentified structure" down: "sensitivity_01_a // 0x828 table 01 at
// min(profile+0x954, 9)", "sensitivity_01_b // 0x82c table 01 at min(profile+0x955, 9)" and
// "unknown_859 // profile+0x131" (this function instead reads +0x12f, the sibling field two
// bytes earlier, presumably unknown_858's own source) -- i.e. unaff_ESI is the RAW 0x2004-byte
// saved profile record (types/game.h / types_notes.md: "not declared here", owned by the profile
// module), not the derived player_control_settings cache.
// register convention: widget in EAX (in_EAX), profile record in ESI (unaff_ESI), both unresolved
// register reads. // blam-cc: EAX -> widget, ESI -> profile_record

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

// blam-cc: EAX -> widget, ESI -> profile_record
// Finds the first spinner_list among each of widget's first three child "rows" and sets each
// one's selection_index from a byte in the raw saved-profile record: rows 0/1 from a 1..10
// sensitivity index (clamped to 0 when out of range, else index-1), row 2 from a plain boolean.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void ui_controls_populate_sensitivity_row(widget_instance *widget, const uint8_t *profile_record)
{
    widget_instance *row = widget->first_child;
    widget_instance *control;
    uint8_t value;

    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != 2;
         control = control->next_sibling) {
    }
    value = profile_record[0x954];
    control->selection_index = (value == 0 || value > 10) ? 0 : (int16_t)(value - 1);

    row = row->next_sibling;
    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != 2;
         control = control->next_sibling) {
    }
    value = profile_record[0x955];
    control->selection_index = (value == 0 || value > 10) ? 0 : (int16_t)(value - 1);

    row = row->next_sibling;
    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != 2;
         control = control->next_sibling) {
    }
    control->selection_index = (profile_record[0x12f] != 0) ? 1 : 0;
}

#if 0
Original Ghidra decompilation (0x4a20f0):

void FUN_004a20f0(void)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  int in_EAX;
  int unaff_ESI;

  iVar2 = *(int *)(in_EAX + 0x34);
  for (iVar3 = *(int *)(iVar2 + 0x34); (iVar3 != 0 && (*(short *)(iVar3 + 0xe) != 2));
      iVar3 = *(int *)(iVar3 + 0x2c)) {
  }
  bVar1 = *(byte *)(unaff_ESI + 0x954);
  if ((bVar1 == 0) || (10 < bVar1)) {
    *(undefined2 *)(iVar3 + 0x40) = 0;
  }
  else {
    *(ushort *)(iVar3 + 0x40) = bVar1 - 1;
  }
  iVar2 = *(int *)(iVar2 + 0x2c);
  for (iVar3 = *(int *)(iVar2 + 0x34); (iVar3 != 0 && (*(short *)(iVar3 + 0xe) != 2));
      iVar3 = *(int *)(iVar3 + 0x2c)) {
  }
  bVar1 = *(byte *)(unaff_ESI + 0x955);
  if ((bVar1 == 0) || (10 < bVar1)) {
    *(undefined2 *)(iVar3 + 0x40) = 0;
  }
  else {
    *(ushort *)(iVar3 + 0x40) = bVar1 - 1;
  }
  for (iVar2 = *(int *)(*(int *)(iVar2 + 0x2c) + 0x34);
      (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2)); iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  *(ushort *)(iVar2 + 0x40) = (ushort)(*(char *)(unaff_ESI + 0x12f) != '\0');
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
