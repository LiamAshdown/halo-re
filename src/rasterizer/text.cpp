/**
 * @file src/rasterizer/text.cpp
 * Debug text drawing and the font glyph atlas.
 */

#include "halo/render/d3d9.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/state.hpp"
#include "halo/rasterizer/constants.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/core/datum.hpp"
#include "halo/tags/flags.hpp"
#include "halo/rasterizer/constants.hpp"
#include "halo/text/api.hpp"
#include "halo/bitmaps/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/bitmaps/bitmaps.hpp"
#include "halo/text/text.hpp"




namespace {

constexpr uint32_t k_bitmap_signature = halo::fourcc('b', 'i', 't', 'm');
constexpr uint16_t k_font_atlas_format = 9;
constexpr uint16_t k_font_atlas_flags = static_cast<uint16_t>(halo::tags::bitmap_data_tag_flag::power_of_two_dimensions) | static_cast<uint16_t>(halo::tags::bitmap_data_tag_flag::unused);

/** Slots of the glyph ring, the texel a glyph border is filled with (white, no alpha) and the reference screen size. */
constexpr int32_t k_glyph_cache_slots = 512;
constexpr int32_t k_glyph_cache_slot_mask = k_glyph_cache_slots - 1;
constexpr int32_t k_glyph_atlas_edge = 512;
constexpr uint16_t k_glyph_texel_white = 0x0fff;
constexpr int16_t k_reference_screen_width = 640;
constexpr int16_t k_reference_screen_height = 480;

/** Flags the keystone documents of the chat box are created with. */
constexpr uint32_t k_keystone_window_flags = 0x10000000;

}  // namespace

static_assert(sizeof(font_glyph_cache) == 0x1010, "font glyph cache size");
static_assert(sizeof(ui_quad_render_state) == 0x8c, "ui quad render state size");

namespace halo::rasterizer {

/**
 * Direct3D 9 back end function chimera__draw_16_bit_text.
 *
 * @address 0x514ab0
 */
void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override, uint32_t position_or_color1, uint32_t position_or_color2, const int16_t *text)
{
    BitmapData *atlas;
    int32_t dest_rect[2];
    int32_t clip_rect[2];
    ui_quad_render_state glyph_state;
    int i;

    if (text_rendering_enabled == 0 || rasterizer_window.type != 1) {
        return;
    }

    rasterizer_frame_index = rasterizer_frame_index + 1;
    atlas = (g_font_glyph_cache.initialized != 0) ? g_font_glyph_cache.atlas : (BitmapData *)0;
    if (atlas == (BitmapData *)0 || *text == 0) {
        return;
    }

    wcslen((const wchar_t *)text);

    if (dest_rect_override == nullptr) {
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
        const Rectangle2D *r = clip_rect_override;
        int16_t clip_w = (r->bottom >= k_reference_screen_height) ? k_reference_screen_height : r->bottom;
        int16_t clip_h = (r->right >= k_reference_screen_width) ? k_reference_screen_width : r->right;
        int16_t x0 = (r->top < 0) ? 0 : r->top;
        int16_t y0 = (r->left < 0) ? 0 : r->left;
        clip_rect[0] = (uint16_t)x0 | ((uint16_t)y0 << 16);
        clip_rect[1] = (uint16_t)clip_w | ((uint16_t)clip_h << 16);
    }

    memset(&glyph_state, 0, sizeof(glyph_state));
    glyph_state.maps[0] = atlas;

    glyph_state.map_scales[0].x = 1.0f;
    glyph_state.map_scales[0].y = 1.0f;
    glyph_state.map_texel_scales[0].x = 1.0f / (float)(int32_t)(int16_t)atlas->width;
    glyph_state.map_texel_scales[0].y = 1.0f / (float)(int32_t)(int16_t)atlas->height;

    rasterizer_draw_text_begin(&glyph_state);
    halo::text::wide_text_strategy::instance().wrap_and_draw(reinterpret_cast<text_glyph_draw_proc>(text_draw_glyph_callback), reinterpret_cast<Rectangle2D *>(dest_rect), reinterpret_cast<Point2DInt *>(position_or_color1), reinterpret_cast<Rectangle2D *>(clip_rect), position_or_color2, reinterpret_cast<void *>(const_cast<int16_t *>(text)));
    rasterizer_draw_text_end();
}

/**
 * Direct3D 9 back end function chimera__draw_8_bit_text.
 *
 * @address 0x5148b0
 */
void chimera__draw_8_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override, uint32_t position_or_color1, uint32_t position_or_color2, const char *text)
{
    BitmapData *atlas;
    int32_t dest_rect[2];
    int32_t clip_rect[2];
    ui_quad_render_state glyph_state;
    int i;

    if (text_rendering_enabled == 0 || rasterizer_window.type != 1) {
        return;
    }

    rasterizer_frame_index = rasterizer_frame_index + 1;
    atlas = (g_font_glyph_cache.initialized != 0) ? g_font_glyph_cache.atlas : (BitmapData *)0;
    if (atlas == (BitmapData *)0 || *text == '\0') {
        return;
    }

    if (dest_rect_override == nullptr) {
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
        const Rectangle2D *r = clip_rect_override;
        int32_t width = render_viewport_bottom[0] - render_viewport_top[0];
        int32_t height = render_viewport_bottom[1] - render_viewport_top[1];
        int32_t clip_w = (r->bottom < width) ? r->bottom : width;
        int32_t clip_h = (r->right < height) ? r->right : height;
        int16_t x0 = (r->top < 0) ? 0 : r->top;
        int16_t y0 = (r->left < 0) ? 0 : r->left;
        clip_rect[0] = (uint16_t)x0 | ((uint16_t)y0 << 16);
        clip_rect[1] = (uint16_t)(int16_t)clip_w | ((uint16_t)(int16_t)clip_h << 16);
    }

    memset(&glyph_state, 0, sizeof(glyph_state));
    glyph_state.maps[0] = atlas;

    glyph_state.map_scales[0].x = 1.0f;
    glyph_state.map_scales[0].y = 1.0f;
    glyph_state.map_texel_scales[0].x = 1.0f / (float)(int32_t)(int16_t)atlas->width;
    glyph_state.map_texel_scales[0].y = 1.0f / (float)(int32_t)(int16_t)atlas->height;

    rasterizer_draw_text_begin(&glyph_state);
    halo::text::narrow_text_strategy::instance().wrap_and_draw(reinterpret_cast<text_glyph_draw_proc>(text_draw_glyph_callback), reinterpret_cast<Rectangle2D *>(dest_rect), reinterpret_cast<Point2DInt *>(position_or_color1), reinterpret_cast<Rectangle2D *>(clip_rect), position_or_color2, reinterpret_cast<void *>(const_cast<char *>(text)));
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
        entry->character->hardware_character_index = halo::k_word_none;
        entry->character = nullptr;
    }
    g_font_glyph_cache.oldest_slot = (uint16_t)((g_font_glyph_cache.oldest_slot + 1) & k_glyph_cache_slot_mask);
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
    character->last_used_frame = (int16_t)rasterizer_frame_index;

    if (character->bitmap_width + g_font_glyph_cache.cursor_x + 2 > k_glyph_atlas_edge) {
        g_font_glyph_cache.cursor_y = (int16_t)(g_font_glyph_cache.cursor_y + g_font_glyph_cache.row_height);
        g_font_glyph_cache.cursor_x = 0;
        g_font_glyph_cache.row_height = 0;
    }

    if (character->bitmap_height + g_font_glyph_cache.cursor_y + 2 >= k_glyph_atlas_edge) {
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

    if (((g_font_glyph_cache.next_slot + 1) & k_glyph_cache_slot_mask) == g_font_glyph_cache.oldest_slot) {
        font_glyph_cache_evict_oldest();
    }

    slot = (int16_t)g_font_glyph_cache.next_slot;
    character->hardware_character_index = (uint16_t)slot;
    entry = &g_font_glyph_cache.entries[slot];
    entry->character = character;
    entry->x = g_font_glyph_cache.cursor_x;
    entry->y = g_font_glyph_cache.cursor_y;

    pixels = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(font->pixels.pointer) + character->pixels_offset);
    for (row = 0; row < character->bitmap_height + 2; row++) {
        atlas = g_font_glyph_cache.atlas;
        texel = static_cast<uint16_t *>(halo::bitmaps::bitmap_data_view(atlas).get_row_address(0, (uint16_t)entry->x, (uint16_t)(entry->y + row)));
        for (column = 0; column < character->bitmap_width + 2; column++) {
            if (row < 1 || row > character->bitmap_height ||
                column < 1 || column > character->bitmap_width) {
                *texel = k_glyph_texel_white;
            } else {
                *texel = (uint16_t)((*pixels << 8) | k_glyph_texel_white);
                pixels++;
            }
            texel++;
        }
    }

    entry->x++;
    entry->y++;

    atlas = g_font_glyph_cache.atlas;
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
    g_font_glyph_cache.next_slot = (uint16_t)((g_font_glyph_cache.next_slot + 1) & k_glyph_cache_slot_mask);
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
    for (i = 0; i < k_glyph_cache_slots; i++) {
        if (g_font_glyph_cache.entries[i].character != 0) {
            g_font_glyph_cache.entries[i].character->hardware_character_index = halo::k_word_none;
        }
        g_font_glyph_cache.entries[i].character = nullptr;
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
    BitmapData *part0;
    int16_t part;

    if (text_rendering_enabled == 0 || rasterizer_window.type != 1) {
        return;
    }

    chimera__widescreen_text_scaling();
    chimera__rasterizer_set_framebuffer_blend_function(state->framebuffer_blend_function);

    part0 = state->maps[0];
    if (part0 != 0) {
        halo::cache::texture_cache_get(part0, 1, 1);
        set_texture(0, part0->hardware_texture);
    }

    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 1);
    set_render_state(halo::d3d9::rs::alpha_ref, 0);
    set_render_state(halo::d3d9::rs::z_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    if (halo::rasterizer::fields::rasterizer_wireframe != 0) {
        set_render_state(halo::d3d9::rs::fill_mode, 3);
    }

    render_device().set_vertex_declaration(rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    render_device().set_software_vertex_processing((rasterizer_software_vertex_processing != 0 ? 0x10u : 0u) | (rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage & 0x10));
    render_device().set_vertex_shader(rasterizer_vertex_shaders[35].shader);
    render_device().set_pixel_shader(0);

    rasterizer_ui_text_constants[16] = state->map_texel_scales[0].x;
    rasterizer_ui_text_constants[17] = state->map_texel_scales[0].y;
    render_device().set_vertex_shader_constant_f(0xd, rasterizer_ui_text_constants, 5);

    for (part = 0; part < 3; part++) {
        BitmapData *part_texture = state->maps[part];
        uint32_t address_mode, filter_value;

        if (part_texture == 0) {
            set_texture(part, 0);
            break;
        }
        halo::cache::texture_cache_get(part_texture, 1, 1);
        set_texture(part, part_texture->hardware_texture);

        address_mode = (state->wrap_modes[part] == 0) ? 3u : 1u;
        set_sampler_state(part, halo::d3d9::ss::address_u, address_mode);
        set_sampler_state(part, halo::d3d9::ss::address_v, address_mode);
        filter_value = (state->single_local_player == 0) ? 2u : 1u;
        set_sampler_state(part, halo::d3d9::ss::mag_filter, filter_value);
        set_sampler_state(part, halo::d3d9::ss::min_filter, filter_value);
        set_sampler_state(part, halo::d3d9::ss::mip_filter, filter_value);
    }

    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::diffuse);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg2, halo::d3d9::ta::diffuse);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
}

namespace rasterizer_draw_text_end_impl {



/**
 * Direct3D 9 back end function rasterizer_draw_text_end.
 *
 * @address 0x531e90
 */
void rasterizer_draw_text_end(void)
{

    if (halo::rasterizer::fields::rasterizer_wireframe != 0) {
        render_device().set_render_state(halo::d3d9::rs::fill_mode, 2);
    }
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_draw_text_end_impl

namespace text_draw_glyph_callback_impl {

typedef struct text_glyph_vertex {
    float x, y, z;
    uint32_t color;
    float u, v;
} text_glyph_vertex;


/**
 * Direct3D 9 back end function text_draw_glyph_callback.
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
    if ((int16_t)((FontCharacter *)character)->hardware_character_index == -1) {
        return;
    }
    shadow_color = text_shadow_color_argb != 0 ? text_shadow_color_argb : (color & k_color_alpha_mask);
    for (pass = 0; pass < 2; pass++) {
        int16_t slot = (int16_t)((FontCharacter *)character)->hardware_character_index;
        int16_t u0 = (int16_t)(g_font_glyph_cache.entries[slot].x + source_x);
        int16_t v0 = (int16_t)(g_font_glyph_cache.entries[slot].y + source_y);
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
        if (text_rendering_enabled && rasterizer_window.type == 1) {
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
    BitmapData *atlas;
    uint32_t pixel_data_size;
    void *pixels;
    uint32_t result;

    atlas = (BitmapData *)GlobalAlloc(0, sizeof(BitmapData));
    result = 0;
    if (atlas != (BitmapData *)0) {
        int i;
        memset(atlas, 0, sizeof(*atlas));

        atlas->width = k_font_atlas_size;
        atlas->height = k_font_atlas_size;
        atlas->bitmap_class = k_bitmap_signature;
        atlas->depth = 1;
        atlas->type = 0;
        atlas->format = k_font_atlas_format;
        atlas->mipmap_count = 0;
        atlas->flags = k_font_atlas_flags;

        pixel_data_size = halo::bitmaps::bitmap_data_view(atlas).calculate_pixel_data_size();
        pixels = GlobalAlloc(0, pixel_data_size);
        atlas->pixel_base = pixels;

        {
            memset(&g_font_glyph_cache, 0, sizeof(g_font_glyph_cache));
        }

        result = rasterizer_bitmap_create_hardware_texture(atlas);
        if ((uint8_t)result != 0) {
            g_font_glyph_cache.atlas = atlas;
            g_font_glyph_cache.initialized = 1;
            return (int32_t)halo::rasterizer::replace_low_byte(result, 1);
        }
    }
    return (int32_t)halo::rasterizer::replace_low_byte(result, 0);
}

}  // namespace halo::rasterizer
