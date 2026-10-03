/**
 * Screen-space quad builders, the button-prompt text drawing and the generic UI rectangle fill.
 */

#include "crt.h"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/text/api.hpp"
#include "halo/bitmaps/api.hpp"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include <string.h>

#include "halo/interface/uis_draw.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/bitmaps/bitmaps.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/text/text.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/constants.hpp"
#include "halo/interface/wide_text.hpp"
#include "halo/interface/color_bits.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/core/x87.hpp"
#include "halo/game/api.hpp"


static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &ui_button_caption = halo::link::ref<uint16_t *[0x28]>(halo::ui::vars().ui_button_caption);
static auto &ui_network_wait_start_time = halo::link::ref<int32_t>(halo::ui::vars().ui_network_wait_start_time);
static auto &trouble_brewing_bitmap_tag = halo::link::ref<datum_index>(halo::ui::vars().trouble_brewing_bitmap_tag);
static auto &formatted_prompt_scratch = halo::link::ref<uint16_t[halo::interface::k_text_buffer_chars]>(halo::ui::vars().formatted_prompt_scratch);
static auto &prompt_percent_text = halo::link::ref<uint16_t[]>(halo::ui::vars().prompt_percent_text);
static auto &hud_text_quote = halo::link::ref<uint16_t[]>(halo::ui::vars().hud_text_quote);
static auto &hud_text_unbound = halo::link::ref<uint16_t[]>(halo::ui::vars().hud_text_unbound);
static auto &prompt_key_token_table = halo::link::ref<int8_t[]>(halo::ui::vars().prompt_key_token_table);
static auto &prompt_icon_override_table = halo::link::ref<uint8_t[0x12]>(halo::ui::vars().prompt_icon_override_table);
static auto &hud_text_draw_color_a = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_a);
static auto &hud_text_draw_color_r = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_r);
static auto &hud_text_draw_color_g = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_g);
static auto &hud_text_draw_color_b = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_b);
static auto &hud_globals_tag_data = halo::link::ref<HUDGlobals *>(halo::ui::vars().hud_globals_tag_data);


namespace halo::ui {

namespace {

/** Local helper shared by the handlers of this file. */
static void draw_span_inline(Rectangle2D *origin, Rectangle2D *cursor, const uint16_t *text)
{
    Rectangle2D out;
    int16_t delta = (int16_t)(cursor->left - origin->left);

    halo::text::globals().ui_prompt_clip_y = 0;
    halo::text::globals().ui_prompt_clip_x = (delta < 0) ? 0 : delta;
    halo::text::text_context::measure_string_extents(origin, cursor, &out, reinterpret_cast<void *>(const_cast<uint16_t *>(text)));
    cursor->left = (int16_t)(cursor->left - 3);
    out.left = origin->left;
    halo::interface::draw_text16((Rectangle2D *)0, &out, text);
    origin->top = cursor->top;
}

}

/**
 * Draws one HUD button icon (bitmap sequence icon->sequence_index), animated at icon->frame_rate frames per 30
 * ticks of a millisecond clock when that is nonzero.
 *
 * Register convention: ESI -> icon
 *
 * @address 0x49ac80
 */
void UiDraw::button_prompt_draw_icon(HUDGlobalsButtonIcon *icon)
{
    uint8_t *bitmaps = (global_globals->interface_bitmaps.count != 0)
                           ? (uint8_t *)global_globals->interface_bitmaps.pointer
                           : nullptr;
    datum_index bitmap_tag = *(datum_index *)(bitmaps + 0xec);
    int32_t zero = 0;
    int64_t counter;
    int32_t frame;

    if (icon->frame_rate == 0) {
        frame = 0;
    } else {
        uint32_t milliseconds;

        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        milliseconds = (uint32_t)((counter * 1000) / halo::cseries::globals().performance_frequency);
        frame = (int32_t)((milliseconds * 30u / 1000u) / (uint32_t)(int32_t)icon->frame_rate);
    }
    halo::interface::hud_meter_resolve_bitmap_frame(bitmap_tag, (int16_t)icon->sequence_index, (uint16_t)frame, (void **)&zero,
                                   (int32_t *)&counter);
}

/**
 * Looks up which button-prompt token (e.g. "a-button") `text` begins with, returning its index into the button
 * caption table or 0xFFFF if no token matches.
 *
 * Register convention: EBX -> text
 *
 * @address 0x49ac30
 */
int16_t UiDraw::button_prompt_index_from_string(uint16_t *text)
{
    uint16_t index = 0;

    do {
        uint32_t token_length = wcslen((const wchar_t *)ui_button_caption[index]);

        if (_wcsnicmp((const wchar_t *)text, (const wchar_t *)ui_button_caption[index], token_length) == 0) {
            break;
        }
        index = index + 1;
    } while (index < 0x28);

    if (index == 0x28) {
        return halo::k_word_none;
    }
    return index;
}

/**
 * Fills `rect` with `packed_color` by submitting a single opaque screen-space quad textured against the globals
 * tag's default_2d bitmap (its second BitmapData entry, sampled at a fixed (0,0) UV so no part of the bitmap
 * actually shows through the tint).
 *
 * Register convention: EAX -> packed_color, ECX -> rect
 *
 * @address 0x449780
 */
void UiDraw::draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect)
{
    uint8_t vertices[4 * sizeof(hud_quad_vertex)];
    hud_quad_vertex *v = (hud_quad_vertex *)vertices;
    ui_quad_render_state state;
    GlobalsRasterizerData *rasterizer_data;
    datum_index default_2d_tag;
    Bitmap *default_2d_bitmap;
    BitmapData *default_2d_bitmap_data;
    int32_t i;

    rasterizer_data = (global_globals->rasterizer_data.count == 0)
        ? (GlobalsRasterizerData *)0
        : (GlobalsRasterizerData *)global_globals->rasterizer_data.pointer;
    default_2d_tag = halo::interface::tag_handle(rasterizer_data->default_2d.tag_id);
    default_2d_bitmap = halo::interface::tag_data<Bitmap>(default_2d_tag);
    default_2d_bitmap_data = (BitmapData *)default_2d_bitmap->bitmap_data.pointer;

    v[0].x = (float)(int32_t)rect->left;  v[0].y = (float)(int32_t)rect->top;
    v[1].x = (float)(int32_t)rect->right; v[1].y = (float)(int32_t)rect->top;
    v[2].x = (float)(int32_t)rect->right; v[2].y = (float)(int32_t)rect->bottom;
    v[3].x = (float)(int32_t)rect->left;  v[3].y = (float)(int32_t)rect->bottom;
    for (i = 0; i < 4; i++) {
        v[i].z = 0.0f;
        v[i].u = 0.0f;
        v[i].v = 0.0f;
        v[i].color = packed_color;
    }

    for (i = 0; i < (int32_t)(sizeof(state) / sizeof(int32_t)); i++) {
        ((int32_t *)&state)[i] = 0;
    }
    state.meter_parameters = nullptr;
    state.maps[0] = &default_2d_bitmap_data[1];
    state.map_scales[0].x = 1.0f;
    state.map_scales[0].y = 1.0f;
    state.map_texel_scales[0].x = 1.0f;
    state.map_texel_scales[0].y = 1.0f;
    state.framebuffer_blend_function = 0;
    state.single_local_player = 0;

    halo::rasterizer::globals().vertex_buffer_lock_state = 8;
    halo::rasterizer::rasterizer_ui_quad_draw(&state, (hud_quad_vertex *)vertices);
    halo::rasterizer::globals().vertex_buffer_lock_state = 0;
}

/**
 * Builds a 4-vertex screen-space quad centered on `origin`, rotated by `rotation_radians` and scaled by `scale`:
 * each corner's local offset comes from source_record's four shorts (treated as {left, top, right,
 * bottom}-shaped extents) combined with corner_uvs (defaulting to the standard {0,1,0,1} unit square when NULL),
 * rotated and translated to `origin`, with every vertex's color forced to opaque white and its UV taken straight
 * from corner_uvs. Submits the result via rasterizer_ui_quad_draw, alongside a second, mostly-unused scratch
 * record that stores source_record and forces four of its floats to 1.0 (preserved verbatim from the decompile;
 * see ui_draw_screen_quad.c's identical note on this pattern).
 *
 * Register convention: EAX -> origin, stack -> (source_record, corner_uvs, scale, rotation_radians, alpha)
 *
 * @address 0x494d70
 */
void UiDraw::draw_rotated_screen_quad(int16_t *origin, int32_t source_record, float *corner_uvs,
                                  float scale, float rotation_radians, float alpha_fraction)
{
    float sin_r = (float)halo::x87::fsin((double)rotation_radians);
    float cos_r = (float)halo::x87::fcos((double)rotation_radians);
    float default_uvs[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    int32_t alpha = (int32_t)(alpha_fraction * 255.0f);
    int16_t left, right, top, bottom;
    int16_t origin_x, origin_y;
    int16_t corner;
    float vertices[4 * 6];
    uint8_t quad[0x60];
    ui_quad_render_state state;

    if (corner_uvs == nullptr) {
        corner_uvs = default_uvs;
    }

    left = *(int16_t *)((char *)source_record + 4);
    right = *(int16_t *)((char *)source_record + 0x10);
    top = *(int16_t *)((char *)source_record + 6);
    bottom = *(int16_t *)((char *)source_record + 0x12);
    origin_x = origin[0];
    origin_y = origin[1];

    for (corner = 0; corner < 4; corner++) {
        float u = (((corner + 1) & 2) == 0) ? corner_uvs[0] : corner_uvs[1];
        float v = (corner < 2) ? corner_uvs[2] : corner_uvs[3];
        float local_x = ((float)left * u - (float)right) * scale;
        float local_y = ((float)top * v - (float)bottom) * scale;
        float *vert = &vertices[corner * 6];

        vert[0] = (local_x * cos_r + (float)origin_x) - local_y * sin_r;
        vert[1] = local_x * sin_r + local_y * cos_r + (float)origin_y;
        vert[2] = 0.0f;
        *(int32_t *)&vert[3] = (alpha << 0x18) | halo::interface::k_rgb_mask;
        vert[4] = u;
        vert[5] = v;
    }
    memcpy(quad, vertices, sizeof(vertices));

    memset(&state, 0, sizeof(state));
    state.map_texel_scales[0].y = 1.0f;
    state.map_texel_scales[0].x = 1.0f;
    state.map_scales[0].y = 1.0f;
    state.map_scales[0].x = 1.0f;
    state.meter_parameters = 0;
    state.single_local_player = 0;
    state.framebuffer_blend_function = 7;
    state.maps[0] = (BitmapData *)source_record;

    halo::rasterizer::rasterizer_ui_quad_draw(&state, (hud_quad_vertex *)quad);
}

/**
 * Builds a screen-space quad (four vertices of {row, col, 0, w, u, v}) covering dest_rect (clamped against
 * clip_rect when given), with per-vertex UVs mapped from source_rect (defaulting to the bitmap's full
 * {0,0,width,height} extent) and every vertex's w set to vertex_w, then hands it to the render submission
 * routine.
 *
 * Register convention: EAX -> source_rect, ECX -> dest_rect, stack -> bitmap_data/clip_rect/vertex_color
 *
 * @address 0x498b20
 */
void UiDraw::draw_screen_quad(const Rectangle2D *source_rect, const Rectangle2D *dest_rect, const BitmapData *bitmap,
                          const Rectangle2D *clip_rect, uint32_t vertex_color)
{
    if (bitmap != nullptr && dest_rect != nullptr) {
        hud_quad_vertex vertices[4];
        ui_quad_render_state state;
        Rectangle2D fallback_rect;
        float corners[8];
        int32_t clamped_bottom;
        float scale_u, scale_v;
        int32_t axis;
        int32_t vertex_index;
        float *corner_pair;

        if (source_rect == nullptr) {
            fallback_rect.top = 0;
            fallback_rect.left = 0;
            fallback_rect.right = (int16_t)bitmap->width;
            fallback_rect.bottom = (int16_t)bitmap->height;
            source_rect = &fallback_rect;
        }

        corners[0] = (float)(int32_t)dest_rect->left;
        corners[1] = (float)(int32_t)dest_rect->top;
        corners[2] = (float)((int32_t)(int16_t)(dest_rect->right - dest_rect->left) + (int32_t)dest_rect->left);
        clamped_bottom = (int32_t)(int16_t)(dest_rect->bottom - dest_rect->top) + (int32_t)dest_rect->top;
        corners[3] = corners[1];
        corners[4] = corners[2];
        corners[5] = (float)clamped_bottom;
        corners[6] = corners[0];
        corners[7] = corners[5];

        if (clip_rect != nullptr) {
            int16_t v;

            v = clip_rect->left;
            if (dest_rect->left < v) {
                clamped_bottom = (int32_t)v;
                corners[6] = (float)(int32_t)v;
                corners[0] = corners[6];
            }
            v = clip_rect->right;
            if (v < dest_rect->right) {
                corners[4] = (float)(int32_t)v;
                corners[2] = corners[4];
            }
            v = clip_rect->top;
            if (dest_rect->top < v) {
                corners[3] = (float)(int32_t)v;
                corners[1] = corners[3];
            }
            if (clip_rect->bottom < dest_rect->bottom) {
                corners[7] = (float)(int32_t)clip_rect->bottom;
                corners[5] = corners[7];
            }
        }

        scale_u = (float)(int32_t)(int16_t)bitmap->width;
        if (scale_u < 1.0f) scale_u = 1.0f;
        scale_u = (float)(int32_t)(int16_t)(source_rect->right - source_rect->left) / scale_u;
        if (scale_u > 1.0f) scale_u = 1.0f;

        scale_v = (float)(int32_t)(int16_t)bitmap->height;
        if (scale_v < 1.0f) scale_v = 1.0f;
        scale_v = (float)(int32_t)(int16_t)(source_rect->bottom - source_rect->top) / scale_v;
        if (scale_v > 1.0f) scale_v = 1.0f;

        axis = 0;
        vertex_index = 0;
        corner_pair = corners + 1;
        do {
            hud_quad_vertex &vertex = vertices[vertex_index];

            vertex.x = corner_pair[-1];
            vertex.y = corner_pair[0];
            vertex.z = 0.0f;
            vertex.color = vertex_color;
            vertex.u = (vertex_index % 3 == 0) ? 0.0f : scale_u;
            vertex.v = (axis < 2) ? 0.0f : scale_v;
            axis = axis + 1;
            vertex_index = vertex_index + 1;
            corner_pair = corner_pair + 2;
        } while (axis < 4);

        memset(&state, 0, sizeof(state));
        state.meter_parameters = 0;
        state.single_local_player = 0;
        state.framebuffer_blend_function = 0;
        state.maps[0] = const_cast<BitmapData *>(bitmap);
        state.map_texel_scales[0].y = 1.0f;
        state.map_texel_scales[0].x = 1.0f;
        state.map_scales[0].y = 1.0f;
        state.map_scales[0].x = 1.0f;

        halo::rasterizer::rasterizer_ui_quad_draw(&state, vertices);
    }
}

/**
 * While a network wait is pending, resolves and draws the "trouble brewing" indicator bitmap; if the tag or its
 * bitmap data cannot be resolved, falls back to ui_draw_filled_rectangle instead.
 *
 * @address 0x49c870
 */
void UiDraw::draw_trouble_brewing_indicator(void)
{
    if (ui_network_wait_start_time != -1) {
        Rectangle2D rect;

        rect.top = halo::interface::k_base_screen_height - 74;
        rect.left = halo::interface::k_base_screen_width - 74;
        rect.bottom = halo::interface::k_base_screen_height - 10;
        rect.right = halo::interface::k_base_screen_width - 10;
        trouble_brewing_bitmap_tag = halo::interface::lookup_tag(halo::fourcc('b', 'i', 't', 'm'), halo::tag_paths::trouble_brewing);
        if (trouble_brewing_bitmap_tag != (datum_index)-1) {
            BitmapData *bitmap_data = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(trouble_brewing_bitmap_tag, 0, 0);

            if (bitmap_data != 0) {
                halo::interface::ui_draw_screen_quad(0, &rect, bitmap_data, 0, halo::k_dword_none);
                return;
            }
        }
        halo::interface::ui_draw_filled_rectangle(halo::interface::k_missing_bitmap_color, &rect);
    }
}

/**
 * Returns true if `text` contains at least one recognised "%buttonname" prompt-substitution token (a '%'
 * immediately followed by a known button caption prefix).
 *
 * Register convention: EAX -> text
 *
 * @address 0x49ada0
 */
uint8_t UiDraw::string_has_button_prompt_token(uint16_t *text)
{
    uint16_t *percent;

    while (text != nullptr && (percent = (uint16_t *)wcschr((const wchar_t *)text, L'%')) != nullptr) {
        text = percent + 1;
        if (halo::interface::ui_button_prompt_index_from_string(text) != halo::k_word_none) {
            return 1;
        }
    }
    return 0;
}

/**
 * Renders a caption string mixing plain text with %token placeholders, drawing each text span and substituting
 * the matching controller-button icon or key name for every recognised token.
 *
 * Register convention: stack -> bounds, use_text_color; EDX -> text
 *
 * @address 0x49ade0
 */
void UiDraw::widget_draw_formatted_prompt_string(Rectangle2D *bounds, uint8_t use_text_color, const uint16_t *text)
{
    Rectangle2D cursor_rect = *bounds;
    uint16_t *cursor = formatted_prompt_scratch;

    wcscpy((wchar_t *)formatted_prompt_scratch, (const wchar_t *)text);
    for (;;) {
        uint16_t *percent = (uint16_t *)(wcschr((const wchar_t *)cursor, 0x25));
        uint16_t *next;
        int16_t token;

        if (percent == nullptr) {
            if (cursor != nullptr) {
                halo::interface::ui_widget_draw_prompt_span(cursor, &cursor_rect, bounds);
            }
            halo::text::globals().ui_prompt_clip_x = 0;
            halo::text::globals().ui_prompt_clip_y = 0;
            return;
        }
        *percent = 0;
        next = percent + 1;
        draw_span_inline(bounds, &cursor_rect, cursor);
        cursor = next;

        token = halo::interface::ui_button_prompt_index_from_string(next);
        if (token == -1) {
            draw_span_inline(bounds, &cursor_rect, prompt_percent_text);
        } else {
            cursor = next + wcslen((const wchar_t *)ui_button_caption[token]);
            bool draw_icon = true;

            if (token > 0x11) {
                if (token > 0x1f) {
                    draw_icon = false;
                } else if (token <= 0x1c) {
                    uint8_t binding[12];
                    uint16_t key_name[0x40];

                    if (halo::input::Bindings::get_last_used_binding((int16_t)prompt_key_token_table[token], (control_binding_descriptor *)binding) != 0) {
                        halo::input::BindingNames::get_binding_display_name((control_binding_descriptor *)binding, key_name);
                        halo::interface::ui_widget_draw_prompt_span(hud_text_quote, &cursor_rect, bounds);
                        halo::interface::ui_widget_draw_prompt_span(key_name, &cursor_rect, bounds);
                        halo::interface::ui_widget_draw_prompt_span(hud_text_quote, &cursor_rect, bounds);
                    } else {
                        halo::interface::ui_widget_draw_prompt_span(hud_text_unbound, &cursor_rect, bounds);
                    }
                    draw_icon = false;
                } else {
                    switch (token) {
                    case 0x1d: token = 0xd; break;
                    case 0x1e: token = 0x10; break;
                    case 0x1f: token = 0x11; break;
                    }
                }
            }
            if (draw_icon) {
                HUDGlobalsButtonIcon *icon =
                    (HUDGlobalsButtonIcon *)*(uint8_t **)((uint8_t *)hud_globals_tag_data + 0xc8) + token;
                HUDInterfaceMessagingFlags saved_flags = icon->flags;
                int16_t saved_width = icon->width_offset;
                ColorARGB icon_color;
                ColorARGB text_color;
                uint32_t packed_color;

                halo::bitmaps::color_codec::argb_int_to_real(&icon_color, halo::interface::color_bits(icon->override_icon_color));
                icon->flags = (HUDInterfaceMessagingFlags)(saved_flags & 0xfd);
                if (prompt_icon_override_table[token] != 0) {
                    icon->flags = (HUDInterfaceMessagingFlags)(icon->flags & 0xfb);
                    icon->width_offset = -5;
                }
                text_color.alpha = hud_text_draw_color_a;
                text_color.red = hud_text_draw_color_r;
                text_color.green = hud_text_draw_color_g;
                text_color.blue = hud_text_draw_color_b;
                packed_color = (uint32_t)halo::x87::__ftol(text_color.alpha * 255.0f) << 24;
                if (halo::interface::color_bits(icon->override_icon_color) == 0 || use_text_color != 0) {
                    icon_color = text_color;
                }
                icon_color.red = icon_color.red * text_color.alpha;
                icon_color.green = icon_color.green * text_color.alpha;
                icon_color.blue = icon_color.blue * text_color.alpha;
                packed_color = (uint32_t)(int32_t)(icon_color.blue * 255.0f) |
                               ((uint32_t)(int32_t)(icon_color.green * 255.0f) << 8) |
                               ((uint32_t)(int32_t)(icon_color.red * 255.0f) << 16) |
                               ((uint32_t)(int32_t)(icon_color.alpha * 255.0f) << 24);
                (void)packed_color;
                halo::interface::ui_button_prompt_draw_icon(icon);
                bounds->left = (int16_t)(bounds->left + 1);
                icon->flags = saved_flags;
                icon->width_offset = saved_width;
            }
        }
        if (cursor == nullptr) {
            halo::text::globals().ui_prompt_clip_x = 0;
            halo::text::globals().ui_prompt_clip_y = 0;
            return;
        }
    }
}

/**
 * Draws one span of a button-prompt caption: sets the clip offset to the non-negative distance the cursor has
 * moved right of the origin, measures the span, retreats the cursor by 3, draws the span with its left edge at
 * origin->left, and copies the cursor top back to the origin.
 *
 * Register convention: EAX -> cursor, ECX -> origin, stack -> text
 *
 * @address 0x49ad30
 */
void UiDraw::widget_draw_prompt_span(const uint16_t *text, Rectangle2D *cursor, Rectangle2D *origin)
{
    Rectangle2D bounds;
    int16_t delta = (int16_t)(cursor->left - origin->left);

    halo::text::globals().ui_prompt_clip_y = 0;
    halo::text::globals().ui_prompt_clip_x = (delta < 0) ? 0 : delta;
    halo::text::text_context::measure_string_extents(origin, cursor, &bounds, reinterpret_cast<void *>(const_cast<uint16_t *>(text)));
    cursor->left = (int16_t)(cursor->left - 3);
    bounds.left = origin->left;
    halo::interface::draw_text16((Rectangle2D *)0, &bounds, text);
    origin->top = cursor->top;
}

}
