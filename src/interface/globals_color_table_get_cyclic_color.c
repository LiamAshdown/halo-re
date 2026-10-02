// globals_color_table_get_cyclic_color  (Ghidra: FUN_00494430, unnamed)
// address 0x494430, size 140 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: out/phase4/interface_functions.md "Looks up a cyclic color entry (e.g. a flash/blink
// color) from a HUD meter's color-animation tag block."; types/tags.h ColorTable/ColorTableColor
// (0x30 byte records, name then ColorARGB at +0x20) match this function's stride and skip
// exactly; GlobalsInterfaceBitmaps' four *_color_table TagDependency fields (screen/hud/editor/
// dialog, indices 2-5 counting from font_system=0) are laid out as consecutive 0x10-byte
// records, which is what lets this function select one by a plain index instead of a named
// field.
// register convention: table_index and color_index as the two recognized stack parameters
// (param_1, param_2); output ColorARGB* in ECX (in_ECX). // blam-cc: stack -> (table_index,
// color_index), ECX -> out

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
extern Globals *global_globals;     // 0x00746fa0
extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: stack -> (table_index, color_index), ECX -> out
// Resolves the globals interface_bitmaps' table_index'th color-table TagDependency (0 =
// font_system, 1 = font_terminal, 2 = screen_color_table, 3 = hud_color_table, 4 =
// editor_color_table, 5 = dialog_color_table -- treating the run of leading TagDependency
// fields as an array), then writes out the color_index'th entry's color (cycling modulo the
// table's actual color count) into *out. Leaves *out at opaque white (1,1,1,1) if the table
// tag isn't assigned or has no colors.
void globals_color_table_get_cyclic_color(int16_t table_index, int16_t color_index, ColorARGB *out)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    TagDependency *dependency;
    ColorTable *color_table;

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    dependency = (TagDependency *)((char *)interface_bitmaps + table_index * 0x10);

    out->alpha = 1.0f;
    out->red = 1.0f;
    out->green = 1.0f;
    out->blue = 1.0f;

    if (dependency->tag_id.index != 0xffff || dependency->tag_id.id != 0xffff) {
        color_table = (ColorTable *)tag_instances[dependency->tag_id.index].data;
        if (color_table->colors.count != 0) {
            ColorTableColor *entry =
                (ColorTableColor *)((char *)color_table->colors.pointer +
                                     (color_index % color_table->colors.count) *
                                         sizeof(ColorTableColor));
            *out = entry->color;
        }
    }
}

#if 0
Original Ghidra decompilation (0x494430):

void FUN_00494430(short param_1,short param_2)

{
  undefined4 *puVar1;
  uint uVar2;
  int *piVar3;
  int iVar4;
  undefined4 *in_ECX;

  if (*(int *)(DAT_00746fa0 + 0x140) == 0) {
    iVar4 = 0;
  }
  else {
    iVar4 = *(int *)(DAT_00746fa0 + 0x144);
  }
  uVar2 = *(uint *)(param_1 * 0x10 + 0xc + iVar4);
  in_ECX[3] = 0x3f800000;
  in_ECX[2] = 0x3f800000;
  in_ECX[1] = 0x3f800000;
  *in_ECX = 0x3f800000;
  if (uVar2 != 0xffffffff) {
    piVar3 = *(int **)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    iVar4 = *piVar3;
    if (iVar4 != 0) {
      puVar1 = (undefined4 *)((short)((int)param_2 % iVar4) * 0x30 + 0x20 + piVar3[1]);
      *in_ECX = *puVar1;
      in_ECX[1] = puVar1[1];
      in_ECX[2] = puVar1[2];
      in_ECX[3] = puVar1[3];
    }
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
