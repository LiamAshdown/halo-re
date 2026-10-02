// ui_search_replace_function_call  (Ghidra: FUN_004a8730, named in phase 4)
// address 0x4a8730, size 32 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: rewritten from objdump 0x4a8730..0x4a874f in the phase-4 review. Renamed from
// ui_network_group_name_get: the table at 0x00692c08 is the four entry search and replace
// function table that widget_instance_render_text_box.c calls inline (0x4a8750, 0x4a8840,
// 0x4a8760, 0x4a8810), and the only caller (widget_instance_render_list_head @0x49b88b)
// passes the replace function index of a list item (+0x20) in AX and the widget in ECX. ECX
// is forwarded as the one stack argument of the table function. An index outside 0..3
// returns L"<invalid>" (0x0066a8a0).
// register convention: AX index, ECX widget.
//   // blam-cc: index -> AX, widget -> ECX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *ui_replace_function_table[4]; // 0x00692c08, ui_search_replace_function
extern uint16_t ui_invalid_replacement_text[]; // 0x0066a8a0, L"<invalid>"

// blam-cc: index -> AX, widget -> ECX
const uint16_t *ui_search_replace_function_call(int16_t index, widget_instance *widget)
{
    if (index >= 0 && (uint16_t)index < 4) {
        return (const uint16_t *)((ui_search_replace_function)ui_replace_function_table[index])(widget);
    }
    return ui_invalid_replacement_text;
}

#if 0
Original Ghidra decompilation (0x4a8730):

undefined ** FUN_004a8730(void)

{
  ushort in_AX;
  undefined **ppuVar1;

  if ((-1 < (short)in_AX) && (in_AX < 4)) {
    ppuVar1 = (undefined **)(*(code *)(&PTR_LAB_00692c08)[(short)in_AX])();
    return ppuVar1;
  }
  return &PTR_DAT_0066a8a0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
