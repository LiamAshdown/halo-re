// text_draw_glyph_callback  (not a Ghidra function; formerly referenced as LAB_00514ce0_glyph_callback)
// address 0x514ce0, size 493 bytes
// name confidence: 0.75  rewrite confidence: 0.85
// evidence: chimera__draw_16_bit_text / chimera__draw_8_bit_text pass 0x514ce0 to text_wrap_and_draw_* as the glyph
//   draw callback (the ten-argument text_glyph_draw_proc shape of text_measure_glyph_callback 0x556260).
//   First-boot track: the first text drawn on the UI map. objdump 0x514ce0..0x514ecc:
//   - font_glyph_cache_allocate_and_upload(font, character); a character left without a cache slot (+0xc == -1)
//     draws nothing
//   - two passes over one 4-vertex fan {x, y, z 0, colour, u, v} (0x18 bytes, texture coordinates in cache
//     pixels): first the shadow, offset by one pixel down and right, in text_shadow_color_argb (0x0071d144) or,
//     when that is 0, black with the text colour's alpha; then the glyph in the text colour. Corners are x..x+width,
//     y..y+height; texture coordinates start at the cache slot's position (0x006d8838 + 8 * slot: +4 u, +6 v)
//     plus source_x/source_y
//   - each pass is drawn (DrawPrimitiveUP, triangle fan, 2 primitives, stride 0x18, device vtable +0x14c) only
//     while 0x00689402 is set and 0x007c1220 == 1
// blam-cc: stack -> state, font, character, color, x, y, source_x, source_y, width, height (cdecl)
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint32_t text_shadow_color_argb; // 0x0071d144
extern uint8_t font_glyph_cache_slots[]; // 0x006d8838, 8 bytes per slot: +4 u, +6 v
extern uint8_t text_rendering_enabled; // 0x00689402
extern int16_t rasterizer_text_mode; // 0x007c1220
extern void *rasterizer_device; // 0x0071d174
extern void font_glyph_cache_allocate_and_upload(void *font, void *character); // 0x514ed0

typedef struct text_glyph_vertex {
    float x, y, z;
    uint32_t color;
    float u, v;
} text_glyph_vertex;

typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *device, uint32_t type, uint32_t primitive_count,
    const void *vertices, uint32_t stride);

void text_draw_glyph_callback(void *state, void *font, uint8_t *character, uint32_t color, int16_t x, int16_t y,
    int16_t source_x, int16_t source_y, int16_t width, int16_t height)
{
    text_glyph_vertex vertices[4];
    uint32_t shadow_color;
    float offset = 1.0f;
    int32_t pass;

    (void)state;
    font_glyph_cache_allocate_and_upload(font, character);
    if (*(int16_t *)(character + 0xc) == -1) {
        return;
    }
    shadow_color = text_shadow_color_argb != 0 ? text_shadow_color_argb : (color & 0xff000000);
    for (pass = 0; pass < 2; pass++) {
        int16_t slot = *(int16_t *)(character + 0xc);
        int16_t u0 = (int16_t)(*(int16_t *)(font_glyph_cache_slots + slot * 8 + 4) + source_x);
        int16_t v0 = (int16_t)(*(int16_t *)(font_glyph_cache_slots + slot * 8 + 6) + source_y);
        uint32_t pass_color = pass == 0 ? shadow_color : color;
        float left = (float)x + offset, right = (float)(x + width) + offset;
        float top = (float)y + offset, bottom = (float)(y + height) + offset;
        float u_left = (float)u0, u_right = (float)(u0 + width);
        float v_top = (float)v0, v_bottom = (float)(v0 + height);
        int32_t i;

        vertices[0].x = left;  vertices[0].y = top;    vertices[0].u = u_left;  vertices[0].v = v_top;
        vertices[1].x = right; vertices[1].y = top;    vertices[1].u = u_right; vertices[1].v = v_top;
        vertices[2].x = right; vertices[2].y = bottom; vertices[2].u = u_right; vertices[2].v = v_bottom;
        vertices[3].x = left;  vertices[3].y = bottom; vertices[3].u = u_left;  vertices[3].v = v_bottom;
        for (i = 0; i < 4; i++) {
            vertices[i].z = 0.0f;
            vertices[i].color = pass_color;
        }
        if (text_rendering_enabled && rasterizer_text_mode == 1) {
            void **vtable = *(void ***)rasterizer_device;
            ((d3d_draw_primitive_up_fn)vtable[0x14c / 4])(rasterizer_device, 6 /* D3DPT_TRIANGLEFAN */, 2, vertices,
                                                          sizeof(text_glyph_vertex));
        }
        offset = 0.0f;
    }
}
