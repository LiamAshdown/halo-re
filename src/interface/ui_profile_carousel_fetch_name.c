// ui_profile_carousel_fetch_name  (Ghidra: FUN_004a69f0, renamed)
// address 0x4a69f0, size 107 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.55
// evidence: types/interface.h widget_instance (controller_index, text); this widget's
// controller_index doubles as a carousel slot index into saved_profile_records, matching the
// convention established in ui_selection_list_mirror_value_build.c and
// ui_profile_carousel_fetch_sensitivity.c.
// register convention: widget as the recognized parameter (param_1).
// reconciled: R56 0x00712dd8 uint8_t saved_profile_records[3][0x2004] -> saved_games.h saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles] (one 0x2004-byte slot; a second would overlap 0x00714dde); same bytes copied/read

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles]; // 0x00712dd8, saved_games.h
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self

// Copies up to 11 wide characters of the carousel-slot profile's display name (starting 2
// bytes into the record, as in the sibling profile helpers) into a freshly allocated 24 byte
// widget text buffer.
void ui_profile_carousel_fetch_name(widget_instance *widget)
{
    uint8_t profile_record[0x1ffc];
    uint16_t *dest;

    memcpy(profile_record, &profile_globals_block[widget->controller_index].profile, sizeof(profile_record));

    dest = (uint16_t *)heap_reallocate(widget->text, 0x18, widget_memory_pool);
    widget->text = dest;
    if (dest != 0) {
        wcsncpy((wchar_t *)dest, (const wchar_t *)(profile_record + 2), 0x0b);
        dest[0x0b] = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4a69f0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a69f0(int param_1)

{
  wchar_t *_Dest;
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x4a6a00;
  puVar2 = &DAT_00712dd8 + *(short *)(param_1 + 8) * 0x801;
  puVar3 = &local_2008;
  for (iVar1 = 0x7ff; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  _Dest = (wchar_t *)heap_reallocate(0x18);
  *(wchar_t **)(param_1 + 0x3c) = _Dest;
  if (_Dest != (wchar_t *)0x0) {
    _wcsncpy(_Dest,(wchar_t *)((int)&local_2008 + 2),0xb);
    *(undefined2 *)(*(int *)(param_1 + 0x3c) + 0x16) = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
