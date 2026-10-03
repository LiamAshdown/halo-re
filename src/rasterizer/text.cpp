/**
 * @file src/rasterizer/text.cpp
 * Debug text drawing and the font glyph atlas.
 * The original author notes and decompiles are in docs/original/rasterizer/.
 */

#include "internal/state.hpp"
#include "halo/cache/api.hpp"
#include "halo/shell/api.hpp"

extern "C" {

extern void text_wrap_and_draw_wide(void *glyph_callback, void *dest_rect, uint32_t position_or_color1, void *clip_rect, uint32_t position_or_color2, const int16_t *text);
extern void text_wrap_and_draw_narrow(void *glyph_callback, void *dest_rect, uint32_t position_or_color1, void *clip_rect, uint32_t position_or_color2, const char *text);
extern uint16_t *bitmap_data_get_row_address(BitmapData *bitmap, int32_t mip_level, int32_t x, int32_t y);
extern uint32_t bitmap_data_calculate_pixel_data_size(BitmapData *bitmap);

}  // extern "C"

namespace halo::rasterizer {

/**
 * Direct3D 9 back end function chimera__draw_16_bit_text. The original author notes are in
 * docs/original/rasterizer/chimera__draw_16_bit_text.c.txt.
 *
 * @address 0x514ab0
 */
void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override, uint32_t position_or_color1, uint32_t position_or_color2, const int16_t *text)
{
    void *atlas;
    int32_t dest_rect[2];
    int32_t clip_rect[2];
    uint32_t glyph_state[0x23];
    int i;

    if (text_rendering_enabled == 0 || rasterizer_window.type != 1) {
        return;
    }

    rasterizer_frame_index = rasterizer_frame_index + 1;
    atlas = (g_font_glyph_cache.initialized != 0) ? (void *)g_font_glyph_cache.atlas : (void *)0;
    if (atlas == (void *)0 || *text == 0) {
        return;
    }

    wcslen((const wchar_t *)text);

    if (dest_rect_override == (int32_t *)0) {
        int16_t neg_origin_x = (int16_t)(-render_viewport_top[0]);
        dest_rect[0] = (uint16_t)(int16_t)(screen_safe_area_right[0] + neg_origin_x) |
                       ((uint16_t)(int16_t)(screen_safe_area_right[1] - render_viewport_top[1]) << 16);
        dest_rect[1] = (uint16_t)(int16_t)(screen_safe_area_bottom[0] + neg_origin_x) |
                       ((uint16_t)(int16_t)(screen_safe_area_bottom[1] - render_viewport_top[1]) << 16);
    } else {
        dest_rect[0] = dest_rect_override[0];
        dest_rect[1] = dest_rect_override[1];
    }

    if (clip_rect_override == (Rectangle2D *)0) {
        clip_rect[0] = 0;
        clip_rect[1] = (uint16_t)(int16_t)(render_viewport_bottom[0] - render_viewport_top[0]) |
                       ((uint16_t)(int16_t)(render_viewport_bottom[1] - render_viewport_top[1]) << 16);
    } else {
        int16_t *r = (int16_t *)clip_rect_override;
        int16_t clip_w = (r[2] > 0x1df) ? 0x1e0 : r[2];
        int16_t clip_h = (r[3] > 0x27f) ? 0x280 : r[3];
        int16_t x0 = (r[0] < 0) ? 0 : r[0];
        int16_t y0 = (r[1] < 0) ? 0 : r[1];
        clip_rect[0] = (uint16_t)x0 | ((uint16_t)y0 << 16);
        clip_rect[1] = (uint16_t)clip_w | ((uint16_t)clip_h << 16);
    }

    for (i = 0; i < 0x23; i++) {
        glyph_state[i] = 0;
    }
    glyph_state[0] = 0;
    glyph_state[3] = (uint32_t)atlas;

    ((float *)glyph_state)[10] = 1.0f;
    ((float *)glyph_state)[11] = 1.0f;
    ((float *)glyph_state)[16] = 1.0f / (float)(int32_t)*(int16_t *)((uint8_t *)atlas + 4);
    ((float *)glyph_state)[17] = 1.0f / (float)(int32_t)*(int16_t *)((uint8_t *)atlas + 6);

    rasterizer_draw_text_begin((ui_quad_render_state *)glyph_state);
    text_wrap_and_draw_wide((void *)text_draw_glyph_callback, dest_rect, position_or_color1, clip_rect,
                 position_or_color2, text);
    rasterizer_draw_text_end();
}

/**
 * Direct3D 9 back end function chimera__draw_8_bit_text. The original author notes are in
 * docs/original/rasterizer/chimera__draw_8_bit_text.c.txt.
 *
 * @address 0x5148b0
 */
void chimera__draw_8_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override, uint32_t position_or_color1, uint32_t position_or_color2, const char *text)
{
    void *atlas;
    int32_t dest_rect[2];
    int32_t clip_rect[2];
    uint32_t glyph_state[0x23];
    int i;

    if (text_rendering_enabled == 0 || rasterizer_window.type != 1) {
        return;
    }

    rasterizer_frame_index = rasterizer_frame_index + 1;
    atlas = (g_font_glyph_cache.initialized != 0) ? (void *)g_font_glyph_cache.atlas : (void *)0;
    if (atlas == (void *)0 || *text == '\0') {
        return;
    }

    if (dest_rect_override == (int32_t *)0) {
        dest_rect[0] = ((int32_t)screen_safe_area_right[0] - render_viewport_top[0]) |
                       (((int32_t)screen_safe_area_right[1] - render_viewport_top[1]) << 16);
        dest_rect[1] = ((int32_t)screen_safe_area_bottom[0] - render_viewport_top[0]) |
                       (((int32_t)screen_safe_area_bottom[1] - render_viewport_top[1]) << 16);
    } else {
        dest_rect[0] = dest_rect_override[0];
        dest_rect[1] = dest_rect_override[1];
    }

    if (clip_rect_override == (Rectangle2D *)0) {
        clip_rect[0] = 0;
        clip_rect[1] = ((int32_t)render_viewport_bottom[0] - render_viewport_top[0]) |
                       (((int32_t)render_viewport_bottom[1] - render_viewport_top[1]) << 16);
    } else {
        int16_t *r = (int16_t *)clip_rect_override;
        int32_t width = render_viewport_bottom[0] - render_viewport_top[0];
        int32_t height = render_viewport_bottom[1] - render_viewport_top[1];
        int32_t clip_w = (r[2] < width) ? r[2] : width;
        int32_t clip_h = (r[3] < height) ? r[3] : height;
        int16_t x0 = (r[0] < 0) ? 0 : r[0];
        int16_t y0 = (r[1] < 0) ? 0 : r[1];
        clip_rect[0] = (uint16_t)x0 | ((uint16_t)y0 << 16);
        clip_rect[1] = (uint16_t)(int16_t)clip_w | ((uint16_t)(int16_t)clip_h << 16);
    }

    for (i = 0; i < 0x23; i++) {
        glyph_state[i] = 0;
    }
    glyph_state[0] = 0;
    glyph_state[3] = (uint32_t)atlas;

    ((float *)glyph_state)[10] = 1.0f;
    ((float *)glyph_state)[11] = 1.0f;
    ((float *)glyph_state)[16] = 1.0f / (float)(int32_t)*(int16_t *)((uint8_t *)atlas + 4);
    ((float *)glyph_state)[17] = 1.0f / (float)(int32_t)*(int16_t *)((uint8_t *)atlas + 6);

    rasterizer_draw_text_begin((ui_quad_render_state *)glyph_state);
    text_wrap_and_draw_narrow((void *)text_draw_glyph_callback, dest_rect, position_or_color1, clip_rect,
                 position_or_color2, text);
    rasterizer_draw_text_end();
}

/**
 * Initializes the fixed 5x vec4 vertex-shader constant block (a 320x240-reference-resolution screen-space
 * scale/pixel-alignment transform) used when rendering HUD/UI text or the first-person view model.
 *
 * @address 0x531ab0
 */
void chimera__widescreen_text_scaling(void)
{
    static const float constants[20] = {
        0.0031250000465661287f, 0.0f, 0.0f, -1.001562476158142f,
        0.0f, -0.004166666883975267f, 0.0f, 1.0020833015441895f,
        0.0f, 0.0f, 0.0f, 0.5f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    int i;
    for (i = 0; i < 20; i++) {
        rasterizer_ui_text_constants[i] = constants[i];
    }
}

static void font_glyph_cache_evict_oldest(void)
{
    font_glyph_cache_entry *entry = &g_font_glyph_cache.entries[(int16_t)g_font_glyph_cache.oldest_slot];

    if (entry->character != 0) {
        ((FontCharacter *)entry->character)->hardware_character_index = 0xffff;
        entry->character = 0;
    }
    g_font_glyph_cache.oldest_slot = (uint16_t)((g_font_glyph_cache.oldest_slot + 1) & 0x1ff);
}

/**
 * Makes sure `character` has a slot in the 512x512 glyph atlas: packs it at the row cursor (starting a new
 * row, or wrapping to the top and evicting what is in the way), copies its 8 bit coverage into the atlas as
 * A4R4G4B4-style texels (alpha from the glyph, color 0xfff) with a one texel border, and re-uploads the atlas
 * texture. A glyph that already has a slot is left alone.
 *
 * @address 0x514ed0
 */
void font_glyph_cache_allocate_and_upload(Font *font, FontCharacter *character)
{
    font_glyph_cache_entry *entry;
    BitmapData *atlas;
    uint8_t *pixels;
    uint16_t *texel;
    int16_t slot;
    int16_t row;
    int16_t column;

    if ((int16_t)character->hardware_character_index != -1) {
        return;
    }
    *(int16_t *)((uint8_t *)character + 0xe) = (int16_t)rasterizer_frame_index;

    if (character->bitmap_width + g_font_glyph_cache.cursor_x + 2 > 0x200) {
        g_font_glyph_cache.cursor_y = (int16_t)(g_font_glyph_cache.cursor_y + g_font_glyph_cache.row_height);
        g_font_glyph_cache.cursor_x = 0;
        g_font_glyph_cache.row_height = 0;
    }

    if (character->bitmap_height + g_font_glyph_cache.cursor_y + 2 >= 0x200) {
        g_font_glyph_cache.cursor_y = 0;
        g_font_glyph_cache.cursor_x = 0;
        g_font_glyph_cache.row_height = 0;
        while (g_font_glyph_cache.oldest_slot != g_font_glyph_cache.next_slot) {
            if (g_font_glyph_cache.entries[(int16_t)g_font_glyph_cache.oldest_slot].y <= 0) {
                break;
            }
            font_glyph_cache_evict_oldest();
        }
    }

    if (character->bitmap_height + 2 >= g_font_glyph_cache.row_height) {
        int16_t band_top = (int16_t)(g_font_glyph_cache.cursor_y + g_font_glyph_cache.row_height);
        int16_t band_bottom = (int16_t)(character->bitmap_height + g_font_glyph_cache.cursor_y + 2);

        while (g_font_glyph_cache.oldest_slot != g_font_glyph_cache.next_slot) {
            int16_t y = g_font_glyph_cache.entries[(int16_t)g_font_glyph_cache.oldest_slot].y;

            if (y < band_top || y >= band_bottom) {
                break;
            }
            font_glyph_cache_evict_oldest();
        }
        g_font_glyph_cache.row_height = (int16_t)(character->bitmap_height + 2);
    }

    if (((g_font_glyph_cache.next_slot + 1) & 0x1ff) == g_font_glyph_cache.oldest_slot) {
        font_glyph_cache_evict_oldest();
    }

    slot = (int16_t)g_font_glyph_cache.next_slot;
    character->hardware_character_index = (uint16_t)slot;
    entry = &g_font_glyph_cache.entries[slot];
    entry->character = (uint32_t)character;
    entry->x = g_font_glyph_cache.cursor_x;
    entry->y = g_font_glyph_cache.cursor_y;

    pixels = (uint8_t *)(font->pixels.pointer + character->pixels_offset);
    for (row = 0; row < character->bitmap_height + 2; row++) {
        atlas = (BitmapData *)g_font_glyph_cache.atlas;
        texel = bitmap_data_get_row_address(atlas, 0, (uint16_t)entry->x, (uint16_t)(entry->y + row));
        for (column = 0; column < character->bitmap_width + 2; column++) {
            if (row < 1 || row > character->bitmap_height ||
                column < 1 || column > character->bitmap_width) {
                *texel = 0x0fff;
            } else {
                *texel = (uint16_t)((*pixels << 8) | 0x0fff);
                pixels++;
            }
            texel++;
        }
    }

    entry->x++;
    entry->y++;

    atlas = (BitmapData *)g_font_glyph_cache.atlas;
    switch (atlas->type) {
    case 0:
        rasterizer_bitmap_upload_2d_mipmaps(atlas);
        break;
    case 1:
        rasterizer_bitmap_upload_cubemap_mipmaps(atlas);
        break;
    case 2:
        rasterizer_bitmap_upload_cubemap_mipmaps_by_face(atlas);
        break;
    }

    g_font_glyph_cache.cursor_x = (int16_t)(g_font_glyph_cache.cursor_x + character->bitmap_width + 2);
    g_font_glyph_cache.next_slot = (uint16_t)((g_font_glyph_cache.next_slot + 1) & 0x1ff);
}

/**
 * Invalidates every occupied font glyph cache slot (only while the cache is initialized).
 *
 * @address 0x514cb0
 */
void font_glyph_cache_clear_all(void)
{
    int i;

    if (g_font_glyph_cache.initialized == 0) {
        return;
    }
    for (i = 0; i < 0x200; i++) {
        if (g_font_glyph_cache.entries[i].character != 0) {
            *(int16_t *)(g_font_glyph_cache.entries[i].character + 0xc) = -1;
        }
        g_font_glyph_cache.entries[i].character = 0;
    }
}






static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture(uint32_t stage, uint32_t texture)
{
    render_device().set_texture(stage, texture);
}

static void set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(stage, type, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

/**
 * Renders a multi-part model (up to 3 attachment parts, consistent with the first-person view model) with
 * widescreen-corrected shader constants and per-part texture/blend state.
 *
 * @address 0x531b80
 */
void rasterizer_draw_text_begin(ui_quad_render_state *state)
{
    uint8_t *context = (uint8_t *)state;
    void *part0;
    int16_t part;

    if (text_rendering_enabled == 0 || rasterizer_window.type != 1) {
        return;
    }

    chimera__widescreen_text_scaling();
    chimera__rasterizer_set_framebuffer_blend_function(((struct ui_quad_render_state *)context)->framebuffer_blend_function);

    part0 = *(void **)(context + 0xc);
    if (part0 != 0) {
        halo::cache::texture_cache_get((BitmapData *)part0, 1, 1);
        set_texture(0, *(uint32_t *)((uint8_t *)part0 + 0x28));
    }

    set_render_state(0x16, 3);
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 1);
    set_render_state(0xf, 1);
    set_render_state(0x18, 0);
    set_render_state(0x7, 0);
    set_render_state(0x1c, 0);
    if (console_debug_toggle_6893e6 != 0) {
        set_render_state(8, 3);
    }

    render_device().set_vertex_declaration(rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    render_device().set_software_vertex_processing((rasterizer_software_vertex_processing != 0 ? 0x10u : 0u) | (rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage & 0x10));
    render_device().set_vertex_shader(rasterizer_vertex_shaders[35].shader);
    render_device().set_pixel_shader(0);

    rasterizer_ui_text_constants[16] = *(float *)(context + 0x40);
    rasterizer_ui_text_constants[17] = *(float *)(context + 0x44);
    render_device().set_vertex_shader_constant_f(0xd, rasterizer_ui_text_constants, 5);

    for (part = 0; part < 3; part++) {
        void *part_texture = *(void **)(context + 0xc + part * 4);
        uint32_t address_mode, filter_value;

        if (part_texture == 0) {
            set_texture(part, 0);
            break;
        }
        halo::cache::texture_cache_get((BitmapData *)part_texture, 1, 1);
        set_texture(part, *(uint32_t *)((uint8_t *)part_texture + 0x28));

        address_mode = (context[0x18 + part] == 0) ? 3u : 1u;
        set_sampler_state(part, 1, address_mode);
        set_sampler_state(part, 2, address_mode);
        filter_value = (context[0x8a] == 0) ? 2u : 1u;
        set_sampler_state(part, 5, filter_value);
        set_sampler_state(part, 6, filter_value);
        set_sampler_state(part, 7, filter_value);
    }

    set_texture_stage_state(0, 1, 4);
    set_texture_stage_state(0, 2, 2);
    set_texture_stage_state(0, 3, 0);
    set_texture_stage_state(0, 4, 4);
    set_texture_stage_state(0, 5, 2);
    set_texture_stage_state(0, 6, 0);
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);
}

namespace rasterizer_draw_text_end_impl {



/**
 * Direct3D 9 back end function rasterizer_draw_text_end. The original author notes are in
 * docs/original/rasterizer/rasterizer_draw_text_end.c.txt.
 *
 * @address 0x531e90
 */
void rasterizer_draw_text_end(void)
{

    if (console_debug_toggle_6893e6 != 0) {
        render_device().set_render_state(8, 2);
    }
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_draw_text_end_impl

/**
 * 0x5195f0 (this session) Lazily creates the debug KSML UI engine, then (re)loads the editbox and log KSML
 * layout files from a resolution-specific "content/" subfolder.
 *
 * @address 0x5196b0
 */
void rasterizer_editbox_log_dump(void)
{
    wchar_t editbox_path[64];
    wchar_t log_path[64];
    wchar_t height_text[32];
    void *rect_zero[4];
    int32_t document;

    if (chat_gui_root_handle == (void *)0) {
        if (unknown_00721ea0 == (void *)0) {
            return;
        }
        chat_gui_root_handle = unknown_00721ea0(halo::shell::globals().window, rasterizer_device, keystone_current_directory, 0, 0, 0, 0);
        if (chat_gui_root_handle == (void *)0) {
            return;
        }
    }

    rect_zero[0] = (void *)0;
    rect_zero[1] = (void *)0;
    rect_zero[2] = (void *)(uintptr_t)rasterizer_present_parameters.back_buffer_width;
    rect_zero[3] = (void *)(uintptr_t)rasterizer_present_parameters.back_buffer_height;

    wcscpy(log_path, L"content/");
    wcscpy(editbox_path, L"content/");

    _itow(rasterizer_round_up_resolution_height(rasterizer_present_parameters.back_buffer_height),
          height_text, 10);

    wcscat(editbox_path, height_text);
    wcscat(editbox_path, L"editbox.ksml");
    wcscat(log_path, height_text);
    wcscat(log_path, L"log.ksml");

    unknown_00721eb4(chat_gui_root_handle, editbox_path, chat_gui_find_object_arg, 0x10000000,   rect_zero, 0, 0, 0, 0, 0, 0);
    document = unknown_00721eb8(chat_gui_root_handle, chat_gui_find_object_arg);
    if (document != 0) {
        unknown_00721edc(document, 0);
        unknown_00721ec8(document);
    }
    unknown_00721eb4(chat_gui_root_handle, log_path, chat_listbox_gui_find_object_arg, 0x10000000,   rect_zero, 0, 0, 0, 0, 0, 0);
}

namespace text_draw_glyph_callback_impl {

typedef struct text_glyph_vertex {
    float x, y, z;
    uint32_t color;
    float u, v;
} text_glyph_vertex;


/**
 * Direct3D 9 back end function text_draw_glyph_callback. The original author notes are in
 * docs/original/rasterizer/text_draw_glyph_callback.c.txt.
 *
 * @address 0x514ce0
 */
void text_draw_glyph_callback(void *state, void *font, uint8_t *character, uint32_t color, int16_t x, int16_t y, int16_t source_x, int16_t source_y, int16_t width, int16_t height)
{
    text_glyph_vertex vertices[4];
    uint32_t shadow_color;
    float offset = 1.0f;
    int32_t pass;

    (void)state;
    font_glyph_cache_allocate_and_upload((Font *)font, (FontCharacter *)character);
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
        if (text_rendering_enabled && (*(int16_t *)&rasterizer_window) == 1) {
            render_device().draw_primitive_up(6, 2, vertices, sizeof(text_glyph_vertex));
        }
        offset = 0.0f;
    }
}

}  // namespace text_draw_glyph_callback_impl

/**
 * Allocates and fills the 512x512 font atlas BitmapData (a4r4g4b4, one mip level), clears font_glyph_cache,
 * uploads the atlas as a hardware texture, and marks the cache initialized. Returns a nonzero low byte on
 * success (0 on any allocation/upload failure).
 *
 * @address 0x514820
 */
int32_t text_font_system_initialize(void)
{
    uint8_t *atlas;
    uint32_t pixel_data_size;
    void *pixels;
    uint32_t result;

    atlas = (uint8_t *)GlobalAlloc(0, 0x30);
    result = 0;
    if (atlas != (uint8_t *)0) {
        int i;
        for (i = 0; i < 0x30; i++) {
            atlas[i] = 0;
        }

        *(uint16_t *)(atlas + 0x04) = 0x200;
        *(uint16_t *)(atlas + 0x06) = 0x200;
        *(uint32_t *)(atlas + 0x00) = 0x6269746d;
        *(uint16_t *)(atlas + 0x08) = 1;
        *(uint16_t *)(atlas + 0x0a) = 0;
        *(uint16_t *)(atlas + 0x0c) = 9;
        *(uint16_t *)(atlas + 0x14) = 0;
        *(uint16_t *)(atlas + 0x0e) = 0x41;

        pixel_data_size = bitmap_data_calculate_pixel_data_size((BitmapData *)atlas);
        pixels = GlobalAlloc(0, pixel_data_size);
        *(uint32_t *)(atlas + 0x2c) = (uint32_t)pixels;

        {
            uint8_t *cache = (uint8_t *)&g_font_glyph_cache;
            for (i = 0; i < 0x1010; i++) {
                cache[i] = 0;
            }
        }

        result = rasterizer_bitmap_create_hardware_texture((BitmapData *)atlas);
        if ((uint8_t)result != 0) {
            g_font_glyph_cache.atlas = (uint32_t)atlas;
            g_font_glyph_cache.initialized = 1;
            return (int32_t)((result & 0xffffff00) | 1);
        }
    }
    return (int32_t)(result & 0xffffff00);
}

}  // namespace halo::rasterizer
