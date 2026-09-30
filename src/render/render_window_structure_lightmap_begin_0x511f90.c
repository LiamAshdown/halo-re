// render_window_structure_lightmap_begin_0x511f90  (not a Ghidra function; a structure pass callback thunk)
// address 0x511f90, size 65 bytes
// name confidence: 0.5  rewrite confidence: 0.95
// evidence: passed to structure_pass by render_window (render_window.c declares it by this name); code Ghidra never
//   made a function (int3 padding around it). Campaign track: the first level geometry pass.
// objdump 0x511f90: with debug toggle 0x006893e4 clear, 0x006893f7 set and a ps_1_4+ card (0x007c118c), a lightmap
//   argument goes to 0x006e0a6c with 0x006e0a68 = 0; a NULL one stores 0 and sets 0x006e0a68 = 1.
// blam-cc: stack -> bitmap_data (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "fn_render.h"

extern int16_t console_debug_toggle_6893e4; // 0x006893e4
extern uint8_t console_debug_toggle_6893f7; // 0x006893f7
extern uint32_t rasterizer_device_version; // 0x007c118c
extern void *rasterizer_lightmap_bitmap; // 0x006e0a6c
extern uint8_t rasterizer_lightmap_bitmap_missing; // 0x006e0a68

void render_window_structure_lightmap_begin_0x511f90(void *bitmap_data)
{
    if (console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f7 == 0 ||
        rasterizer_device_version < 0xffff0104) {
        return;
    }
    if (bitmap_data != 0) {
        rasterizer_lightmap_bitmap = bitmap_data;
        rasterizer_lightmap_bitmap_missing = 0;
    } else {
        rasterizer_lightmap_bitmap = 0;
        rasterizer_lightmap_bitmap_missing = 1;
    }
}
