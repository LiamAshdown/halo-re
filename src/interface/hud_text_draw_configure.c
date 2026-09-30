// hud_text_draw_configure  (Ghidra: FUN_004944c0, unnamed)
// address 0x4944c0, size 145 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: out/phase4/interface_functions.md "Selects a HUD meter flash color entry and caches
// it as the currently active flash-effect parameters."; reuses globals_color_table_get_cyclic_
// color.c's GlobalsInterfaceBitmaps TagDependency-array indexing and the shared hud_text_draw_*
// globals already named by src/game/hud_draw_world_relative_text.c.
// register convention: all six Ghidra-recognized stack parameters.
// blam-cc: stack -> (font_table_index, color_or_flags, column, unknown_4730, color_table_index,
// color_index)
// UNSURE: this function's own ECX argument to FUN_00494430 (globals_color_table_get_cyclic_color)
// is not visible at this call site; supplied as &color, the local staging struct that is
// immediately copied into the hud_text_draw_* color slots, which is this function's only
// plausible destination for it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern Globals *global_globals; // 0x00746fa0

extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734
extern int16_t hud_text_draw_column;          // 0x006e4736
extern uint32_t hud_text_draw_unknown_4730;   // 0x006e4730
extern int32_t hud_text_draw_font_tag_id;     // 0x006e472c
extern float hud_text_draw_color_a;           // 0x006e4738, alpha (ColorARGB order); float bits, stored with mov
extern float hud_text_draw_color_r;           // 0x006e473c
extern float hud_text_draw_color_g;           // 0x006e4740
extern float hud_text_draw_color_b;           // 0x006e4744


// blam-cc: stack -> (font_table_index, color_or_flags, column, unknown_4730, color_table_index,
// color_index)
// Configures the shared HUD text-draw state for a subsequent draw call: resolves
// font_table_index's globals TagDependency tag id as the font, resolves color_table_index's
// cyclic color entry at color_index as the color, and copies the three remaining flag/column
// arguments straight through.
void hud_text_draw_configure(int16_t font_table_index, uint16_t color_or_flags, int16_t column,
                              uint32_t unknown_4730, int16_t color_table_index, int16_t color_index)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    TagDependency *dependency;
    ColorARGB color;

    globals_color_table_get_cyclic_color(color_table_index, color_index, &color);

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    dependency = (TagDependency *)((char *)interface_bitmaps + font_table_index * 0x10);
    hud_text_draw_font_tag_id = *(int32_t *)&dependency->tag_id;

    hud_text_draw_color_a = color.alpha;
    hud_text_draw_color_r = color.red;
    hud_text_draw_color_g = color.green;
    hud_text_draw_color_b = color.blue;
    hud_text_draw_color_or_flags = color_or_flags;
    hud_text_draw_column = column;
    hud_text_draw_unknown_4730 = unknown_4730;
}

#if 0
Original Ghidra decompilation (0x4944c0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004944c0(short param_1,undefined2 param_2,undefined2 param_3,undefined4 param_4,
                 undefined4 param_5,undefined4 param_6)

{
  int iVar1;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  if (*(int *)(DAT_00746fa0 + 0x140) == 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = *(int *)(DAT_00746fa0 + 0x144);
  }
  FUN_00494430(param_5,param_6);
  DAT_006e472c = *(undefined4 *)(param_1 * 0x10 + 0xc + iVar1);
  DAT_006e4738 = local_10;
  DAT_006e473c = local_c;
  DAT_006e4740 = local_8;
  DAT_006e4744 = local_4;
  DAT_006e4734._0_2_ = param_2;
  DAT_006e4734._2_2_ = param_3;
  _DAT_006e4730 = param_4;
  return;
}
#endif
