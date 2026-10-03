#include "halo/interface/ifr1_hud_draw.hpp"
#include "halo/text/api.hpp"
#include "halo/bitmaps/api.hpp"
#include <string.h>
#include "halo/cache/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/text/text.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern int32_t ROUND(float x);
extern uint32_t render_viewport_top;
extern HUDGlobals *hud_globals_tag_data;
extern int32_t __ftol(double x);
extern float sinf(float x);
extern float cosf(float x);
extern float sqrtf(float x);
extern float atan2f(float y, float x);
extern long lrint(double x);
extern float hud_multitexture_effector_counter;
extern Globals *global_globals;
}

static datum_index hud_local_player_index_to_player(int16_t local_player_index)
{
    if (local_player_index != -1 && local_player_index < 1) {
        return halo::game::globals().local_player_globals->local_players[local_player_index];
    }
    return (datum_index)-1;
}

typedef struct hud_number_pen {
    datum_index digits_bitmap;
    uint8_t is_sprite_bitmap;
    uint16_t anchor;
    int16_t x;
    int16_t y;
    float advance;
    float scale;
    uint32_t color;
} hud_number_pen;

static void hud_number_draw_glyph(hud_number_pen *pen, uint16_t glyph, uint8_t advance)
{
    void *bitmap_data = 0;
    int32_t sprite_rect = 0;
    Point2DInt position;

    position.x = pen->x;
    position.y = pen->y;
    halo::interface::hud_meter_resolve_bitmap_frame(pen->digits_bitmap, 0, glyph, &bitmap_data, &sprite_rect);
    halo::interface::hud_draw_bitmap_at((const float *)sprite_rect, (BitmapData *)bitmap_data, pen->is_sprite_bitmap, (int16_t)pen->anchor,
                       &position, pen->scale, 0.0f, pen->color);
    if (advance) {
        pen->x = (int16_t)(int32_t)((float)pen->x - pen->advance);
    }
}

namespace halo::interface {

/**
 * Converts a HUD element's anchor + pixel offset into an absolute 640x480-canvas screen position (anchor 0..3,
 * one per corner) or a camera-viewport-relative position (anchor >= 4, via an unrecovered per-anchor handler
 * when selector is nonzero).
 * blam-cc: see header
 *
 * @address 0x4ab690
 */
void HudDraw::anchor_offset_to_screen_position(uint16_t *anchor, uint8_t has_scale, float scale, const int16_t *offset, int16_t *out, int32_t selector)
{
    float x, y;
    uint16_t anchor_value = *anchor;

    if (!has_scale || scale == 0.0f) {
        scale = 1.0f;
    }

    if ((int16_t)anchor_value < 4) {
        x = (float)((((anchor_value & 1) == 0) ? 1 : -1) * (int32_t)offset[0]) * scale +
            (float)((((anchor_value & 1) != 0) ? 0x270 : 0) + 8);
        y = (float)((((anchor_value & 2) == 0) ? 1 : -1) * (int32_t)offset[1]) * scale +
            (float)((((anchor_value & 2) != 0) ? 0x1d8 : 0) + 8);
    } else {
        int16_t offset_x = (int16_t)(render_viewport_top >> 16);
        int16_t offset_y = (int16_t)render_viewport_top;
        x = (float)(int32_t)offset[0] * scale + (float)(0x140 - offset_x);
        y = (float)(int32_t)offset[1] * scale + (float)(0xf0 - offset_y);
    }

    if (selector != 0) {
        const int16_t *child = (const int16_t *)(uint32_t)selector;
        int32_t dx = child[8];
        int32_t dy = child[9];

        switch ((int16_t)anchor_value) {
        case 0:
            break;
        case 1:
            dx -= child[2];
            break;
        case 2:
            dy -= child[3];
            break;
        case 3:
            dx -= child[2];
            dy -= child[3];
            break;
        case 4: {
            int32_t half = (int32_t)child[2] / 2;
            dx += half;
            dy += half;
            break;
        }
        default:
            goto store;
        }
        x = (float)dx * scale + x;
        y = (float)dy * scale + y;
    }
store:
    out[0] = (int16_t)(int32_t)ROUND(x);
    out[1] = (int16_t)(int32_t)ROUND(y);
}

/**
 * Size of one HUD bitmap quad in screen pixels, laid out around its anchor point. uv is the {u0, u1, v0, v1}
 * source rectangle: already in pixels for an interface bitmap (pixel_uvs), or normalized, in which case the
 * span is multiplied by the bitmap width and height. out_extents is {x0, x1, y0, y1} relative to the anchor
 * point.
 * blam-cc: CL -> pixel_uvs, ESI -> bitmap, EDX -> uv, EAX -> out_extents, stack -> anchor
 *
 * @address 0x4acc50
 */
void HudDraw::bitmap_anchor_extents(uint8_t pixel_uvs, const BitmapData *bitmap, const float *uv, float *out_extents, int16_t anchor)
{
    int32_t width_factor;
    int32_t height_factor;
    float width;
    float height;

    width_factor = pixel_uvs != 0 ? 1 : (int32_t)(int16_t)bitmap->width;
    width = (uv[1] - uv[0]) * (float)width_factor;
    height_factor = pixel_uvs != 0 ? 1 : (int32_t)(int16_t)bitmap->height;
    height = (uv[3] - uv[2]) * (float)height_factor;

    switch (anchor) {
    case 0:
        out_extents[0] = 0.0f;
        out_extents[1] = width;
        out_extents[2] = 0.0f;
        out_extents[3] = height;
        break;
    case 1:
        out_extents[0] = -width;
        out_extents[1] = 0.0f;
        out_extents[2] = 0.0f;
        out_extents[3] = height;
        break;
    case 2:
        out_extents[0] = 0.0f;
        out_extents[1] = width;
        out_extents[2] = -height;
        out_extents[3] = 0.0f;
        break;
    case 3:
        out_extents[0] = -width;
        out_extents[1] = 0.0f;
        out_extents[2] = -height;
        out_extents[3] = 0.0f;
        break;
    case 4:
        out_extents[0] = width * -0.5f;
        out_extents[1] = width * 0.5f;
        out_extents[2] = -0.5f * height;
        out_extents[3] = height * 0.5f;
        break;
    }
}

/**
 * Draws one HUD bitmap quad whose anchor corner (0..4, see hud_bitmap_anchor_extents) sits at screen_position,
 * scaled by scale on both axes and rotated by rotation radians.
 * blam-cc: uv -> EAX, bitmap -> EDX, pixel_uvs -> CL
 *
 * @address 0x4acbb0
 */
void HudDraw::bitmap_at(const float *uv, BitmapData *bitmap, uint8_t pixel_uvs, int16_t anchor, const Point2DInt *screen_position, float scale, float rotation, uint32_t color)
{
    float default_uv[4];
    float both_scale[2];
    float extents[4];

    default_uv[0] = 0.0f;
    default_uv[1] = 1.0f;
    default_uv[2] = 0.0f;
    default_uv[3] = 1.0f;
    if (pixel_uvs != 0) {
        default_uv[1] = (float)(int32_t)(int16_t)bitmap->width;
        default_uv[3] = (float)(int32_t)(int16_t)bitmap->height;
    }
    if (uv == 0) {
        uv = default_uv;
    }
    both_scale[0] = scale;
    both_scale[1] = scale;
    halo::interface::hud_bitmap_anchor_extents(pixel_uvs, bitmap, uv, extents, anchor);
    halo::interface::hud_draw_rotated_bitmap_quad(screen_position, both_scale, 0, bitmap, uv, extents, rotation, color);
}

/**
 * Draws one HUD bitmap quad placed by a HUD interface element: its anchor offset, width and height scale
 * (times scale) and the anchor corner. uv is {u0, u1, v0, v1}; color is packed ARGB; split_screen asks
 * 0x4ab690 to scale the anchor offset for a split screen view.
 * blam-cc: uv -> EAX, placement -> EDX, pixel_uvs -> BL
 *
 * @address 0x4acad0
 */
void HudDraw::bitmap_element(const float *uv, const hud_element_placement *placement, uint8_t pixel_uvs, void *meter_parameters, BitmapData *bitmap, uint16_t *anchor, float scale, float rotation, uint32_t color, uint8_t split_screen)
{
    float default_uv[4];
    float element_scale[2];
    float extents[4];
    Point2DInt screen_position;
    uint8_t scale_offset;

    default_uv[0] = 0.0f;
    default_uv[1] = 1.0f;
    default_uv[2] = 0.0f;
    default_uv[3] = 1.0f;
    if (pixel_uvs != 0) {
        default_uv[1] = (float)(int32_t)(int16_t)bitmap->width;
        default_uv[3] = (float)(int32_t)(int16_t)bitmap->height;
    }
    if (uv == 0) {
        uv = default_uv;
    }
    element_scale[0] = scale * placement->width_scale;
    element_scale[1] = scale * placement->height_scale;

    scale_offset = 0;
    if (split_screen != 0 && (*(const uint8_t *)&placement->scaling_flags & 1) == 0) {
        scale_offset = 1;
    }
    halo::interface::hud_anchor_offset_to_screen_position(anchor, scale_offset, 0.0f, &placement->anchor_offset.x,
                                         &screen_position.x, 0);
    halo::interface::hud_bitmap_anchor_extents(pixel_uvs, bitmap, uv, extents, (int16_t)*anchor);
    halo::interface::hud_draw_rotated_bitmap_quad(&screen_position, element_scale, meter_parameters, bitmap, uv, extents,
                                 rotation, color);
}

/**
 * Draws the icon of one HUD message icon argument at the text cursor and advances the cursor.
 * blam-cc: information -> ESI
 *
 * @address 0x4ad970
 */
void HudDraw::message_icon(const hud_messaging_information *information, Rectangle2D *cursor, uint32_t color)
{
    BitmapData *bitmap;
    int32_t uv_offset;
    const float *uv;
    int32_t frame;
    float scale;
    int16_t x;
    Point2DInt position;

    bitmap = 0;
    uv_offset = 0;
    frame = 0;
    if (information->frame_rate != 0) {
        frame = halo::game::globals().game_time->game_time / (int32_t)information->frame_rate;
    }
    halo::interface::hud_meter_resolve_bitmap_frame(*(datum_index *)&hud_globals_tag_data->icon_bitmap.tag_id,
                                   (int16_t)information->sequence_index, (uint16_t)frame, (void **)&bitmap,
                                   &uv_offset);
    if (bitmap == 0 || halo::cache::texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }
    uv = (const float *)uv_offset;

    scale = 0.75f;
    if (halo::game::globals().local_player_globals->local_player_count <= 1) {
        scale = 1.0f;
    }
    x = (int16_t)__ftol((double)((float)information->offset.x * scale + (float)cursor->left));
    position.x = x;
    position.y = (int16_t)__ftol((double)((float)cursor->bottom - (float)information->offset.y * scale));
    if ((information->flags & 2) != 0) {
        color = *(const uint32_t *)&information->override_icon_color;
    }
    halo::interface::hud_draw_bitmap_at(uv, bitmap, 0, 2, &position, scale, 0.0f, color);

    if ((information->flags & 4) != 0) {
        cursor->left = (int16_t)__ftol((double)((float)information->width_offset * scale + (float)x));
    } else if (uv != 0) {
        cursor->left = (int16_t)__ftol((double)(((uv[1] - uv[0]) * (float)(int16_t)bitmap->width +
                                                 (float)information->width_offset) * scale + (float)x));
    } else {
        cursor->left = (int16_t)__ftol((double)((float)((int16_t)bitmap->width + information->width_offset) * scale +
                                                (float)x));
    }
}

/**
 * Draws one text span of a HUD message line at the cursor and advances the cursor past it.
 * blam-cc: cursor -> EAX, origin -> ECX
 *
 * @address 0x4ad8e0
 */
void HudDraw::message_text_span(Rectangle2D *cursor, Rectangle2D *origin, const uint16_t *text, uint8_t allow_button_prompts)
{
    Rectangle2D bounds;

    halo::text::globals().ui_prompt_clip_x = (int16_t)(cursor->left - origin->left);
    halo::text::globals().ui_prompt_clip_y = 0;
    halo::text::text_context::measure_string_extents(origin, cursor, &bounds, reinterpret_cast<void *>(const_cast<uint16_t *>(text)));
    cursor->left = (int16_t)(cursor->left - 3);
    bounds.left = origin->left;
    if (allow_button_prompts != 0 && halo::game::globals().current_engine != 0) {
        halo::interface::ui_widget_draw_formatted_prompt_string(&bounds, 1, text);
    } else {
        halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)&bounds, 0, 0, (const int16_t *)text);
    }
    origin->top = cursor->top;
}

/**
 * Draws one multitexture overlay of a HUD static element: up to three bitmaps combined by the overlay blend
 * functions, animated by its effectors (tint, horizontal and vertical offset, fade, geometry offset) from the
 * local player aim pitch, weapon ammo, heat or zoom level.
 * blam-cc: scale -> EAX
 *
 * @address 0x4acfe0
 */
void HudDraw::multitexture_overlay(const float *scale, const HUDInterfaceMultitextureOverlay *overlay, int16_t local_player_index, const Point2DInt *screen_position, const float *uv, const float *extents, float rotation, uint32_t color)
{
    ui_quad_render_state state;
    hud_quad_vertex vertices[4];
    weapon_hud_ammo_state ammo;
    Point2D offsets[3];
    ColorRGB tints[3];
    ColorRGB tint;
    float fades[3];
    float geometry_offset[2];
    float sine;
    float cosine;
    float value;
    float output;
    float x;
    int16_t i;

    offsets[0] = overlay->primary_offset;
    offsets[1] = overlay->secondary_offset;
    offsets[2] = overlay->tertiary_offset;
    memset(tints, 0, sizeof(tints));
    fades[0] = 1.0f;
    fades[1] = 1.0f;
    fades[2] = 1.0f;
    geometry_offset[0] = 0.0f;
    geometry_offset[1] = 0.0f;
    sine = sinf(rotation);
    cosine = cosf(rotation);

    halo::interface::hud_player_weapon_ammo_state(
        (const player *)((uint8_t *)halo::game::globals().player_data->data + (hud_local_player_index_to_player(local_player_index) & 0xffff) * 0x200),
        &ammo);

    x = 0.0f;
    for (i = 0; i < 4; i++) {
        int32_t corner = i + 1;
        float u = (corner & 2) != 0 ? uv[1] : uv[0];
        float v = i > 1 ? uv[3] : uv[2];
        float y = i > 1 ? extents[3] : extents[2];
        float rotated;

        x = (corner & 2) != 0 ? extents[1] : extents[0];
        rotated = (x * cosine - y * sine) * scale[0];
        vertices[i].x = (float)(screen_position->x + (int32_t)lrint(rotated));
        rotated = (y * cosine + x * sine) * scale[1];
        vertices[i].y = (float)(screen_position->y + (int32_t)lrint(rotated));
        vertices[i].z = 0.0f;
        vertices[i].color = color;
        vertices[i].u = u;
        vertices[i].v = v;
    }
    value = x;

    memset(&state, 0, sizeof(state));
    state.map_texel_scales[0].y = 1.0f;
    state.map_texel_scales[0].x = 1.0f;
    state.map_scales[0].y = 1.0f;
    state.map_scales[0].x = 1.0f;
    state.meter_parameters = 0;
    state.single_local_player = halo::game::globals().local_player_globals->local_player_count == 1;
    state.maps[0] = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(*(const datum_index *)&overlay->primary.tag_id, 0, 0);
    state.maps[1] = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(*(const datum_index *)&overlay->secondary.tag_id, 0, 0);
    state.maps[2] = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(*(const datum_index *)&overlay->tertiary.tag_id, 0, 0);

    for (i = 0; i < 3; i++) {
        const BitmapData *map = state.maps[i];

        if (map != 0) {
            const Point2D *map_scale = &(&overlay->primary_scale)[i];
            int32_t width = (int16_t)map->width;
            int32_t height = (int16_t)map->height;
            float u_scale = 1.0f;
            float v_scale = 1.0f;

            if (map_scale->x != 0.0f) {
                u_scale = 1.0f / map_scale->x;
            }
            if (map_scale->y != 0.0f) {
                v_scale = 1.0f / map_scale->y;
            }
            if ((width & (width - 1)) != 0 || (height & (height - 1)) != 0) {
                state.map_texel_scales[i].x = 1.0f / (float)(int16_t)map->width;
                state.map_texel_scales[i].y = 1.0f / (float)(int16_t)map->height;
            } else {
                state.map_texel_scales[i].x = 1.0f;
                state.map_texel_scales[i].y = 1.0f;
            }
            state.map_offsets[i] = &offsets[i];
            state.map_scales[i].x = u_scale;
            state.map_scales[i].y = v_scale;
            state.wrap_modes[i] = (uint8_t)(&overlay->primary_wrap_mode)[i];
        }
        if (i < 2) {
            int16_t *blend = i == 0 ? &state.zero_to_one_blend : &state.one_to_two_blend;

            switch ((&overlay->zero_to_one_blend_function)[i]) {
            case 0: *blend = 0; break;
            case 1: *blend = 2; break;
            case 2: *blend = 1; break;
            case 3: *blend = 3; break;
            case 4: *blend = 4; break;
            }
        } else {
            state.framebuffer_blend_function = overlay->framebuffer_blend_function;
        }
    }

    for (i = 0; (int32_t)i < (int32_t)overlay->effectors.count; i++) {
        const HUDInterfaceMultitextureOverlayEffector *effector =
            (const HUDInterfaceMultitextureOverlayEffector *)overlay->effectors.pointer + i;

        hud_multitexture_effector_counter += 0.05f;
        switch (effector->source) {
        case 0: {
            datum_index player_index = hud_local_player_index_to_player(local_player_index);
            datum_index unit_index = (datum_index)-1;
            const float *aim;

            if (player_index != (datum_index)-1) {
                unit_index = ((player *)((uint8_t *)halo::game::globals().player_data->data + (player_index & 0xffff) * 0x200))->unit;
            }
            aim = (const float *)((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[unit_index & 0xffff].data + 0x23c);
            value = atan2f(aim[2], sqrtf(aim[0] * aim[0] + aim[1] * aim[1]));
            break;
        }
        case 1:
        case 2:
            value = 0.0f;
            break;
        case 3:
            value = (float)ammo.magazines[0].rounds_loaded;
            break;
        case 4:
            value = (float)ammo.magazines[0].rounds_unloaded;
            break;
        case 5:
            value = ammo.heat;
            break;
        case 6:
            value = effector->in_bounds[0];
            break;
        case 7: {
            int16_t zoom = -1;
            if (local_player_index != -1) {
                zoom = halo::game::globals().player_control->local_players[local_player_index].desired_zoom_level;
            }
            value = (float)zoom;
            break;
        }
        }

        if (effector->in_bounds[0] != effector->in_bounds[1] && effector->out_bounds[0] != effector->out_bounds[1]) {
            float t = (value - effector->in_bounds[0]) / (effector->in_bounds[1] - effector->in_bounds[0]);
            if (t < 0.0f) {
                t = 0.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
            }
            output = (1.0f - t) * effector->out_bounds[0] + t * effector->out_bounds[1];
            halo::bitmaps::color_interpolate((ColorRGB *)&effector->tint_color_upper_bound, (ColorRGB *)&effector->tint_color_lower_bound, &tint, static_cast<color_interpolation_flags>(0), t);
        } else {
            output = effector->out_bounds[0];
            tint = effector->tint_color_lower_bound;
        }

        switch (effector->destination) {
        case 0:
            geometry_offset[0] = effector->destination_type == 1 ? output : 0.0f;
            geometry_offset[1] = effector->destination_type == 2 ? output : 0.0f;
            state.geometry_offset = geometry_offset;
            break;
        case 1:
        case 2:
        case 3: {
            int32_t map = effector->destination - 1;
            switch (effector->destination_type) {
            case 0:
                tints[map] = tint;
                state.map_tints[map] = &tints[map];
                break;
            case 1:
                state.map_offsets[map]->x += output;
                break;
            case 2:
                state.map_offsets[map]->y += output;
                break;
            case 3:
                fades[map] = output;
                state.map_fades[map] = &fades[map];
                break;
            }
            break;
        }
        }
    }

    halo::rasterizer::rasterizer_ui_quad_draw(&state, vertices);
}

/**
 * Original engine function hud_draw_number; the author notes are in
 * docs/original/interface/hud_draw_number.txt.
 *
 * @address 0x4ac0b0
 */
void HudDraw::number(void *unused, uint16_t *anchor, const hud_number_placement *placement, int16_t value, int16_t fraction, uint32_t flags, int32_t flash_start_time, float scale)
{
    GlobalsInterfaceBitmaps *interface_bitmaps = (global_globals->interface_bitmaps.count != 0)
        ? (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer
        : (GlobalsInterfaceBitmaps *)0;
    datum_index digits_tag = *(datum_index *)&interface_bitmaps->hud_digits_definition.tag_id;
    HUDNumber *digits;
    uint8_t *digits_bitmap_data;
    BitmapData *bitmap;
    uint8_t thousands;
    uint8_t negative;
    float width_digits;
    float width_decimal;
    int32_t magnitude;
    int32_t fraction_value = fraction;
    Point2DInt origin;
    hud_number_pen pen;

    (void)unused;
    if (digits_tag == (datum_index)-1) {
        return;
    }
    digits = (HUDNumber *)halo::cache::globals().tag_instances[digits_tag & 0xffff].data;
    pen.digits_bitmap = *(datum_index *)&digits->digits_bitmap.tag_id;
    digits_bitmap_data = (uint8_t *)halo::cache::globals().tag_instances[pen.digits_bitmap & 0xffff].data;
    bitmap = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(pen.digits_bitmap, 0, 0);
    thousands = (value > 999);
    if (halo::cache::texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }
    negative = (value < 0);

    {
        int32_t extra = 0;
        if (placement->number_of_fractional_digits != 0 && fraction != -1) {
            extra = ((placement->number_of_fractional_digits > 4) ? 4 : placement->number_of_fractional_digits) + 1;
        }
        width_digits = (float)(placement->maximum_number_of_digits + extra);
        width_decimal = (float)((placement->number_of_fractional_digits != 0) ? digits->decimal_point_width : 0);
    }
    pen.scale = (scale > 0.0f) ? scale : 1.0f;
    if (placement->scaling_flags & 4) {
        pen.scale = pen.scale * 0.5f;
    }
    if (placement->flags & 4) {
        width_digits = width_digits + 1.0f;
        if (thousands) {
            fraction_value = value * 10;
            value = (int16_t)(value / 1000);
        }
    }
    magnitude = (value < 0) ? -value : value;

    halo::interface::hud_anchor_offset_to_screen_position(anchor, (uint8_t)((flags >> 2) & 1), 0.0f,
                                         (const int16_t *)&placement->anchor_offset, (int16_t *)&origin, 0);
    switch (*anchor) {
    case 0:
    case 2:
        pen.x = (int16_t)(int32_t)(((width_digits - 2.0f) * digits->screen_digit_width + width_decimal) * pen.scale + origin.x);
        break;
    case 4:
        pen.x = (int16_t)(int32_t)(((width_digits - 1.0f) * digits->screen_digit_width + width_decimal) * pen.scale * 0.5f + origin.x);
        break;
    default:
        pen.x = origin.x;
        break;
    }
    if (bitmap == 0) {
        return;
    }

    if (flags & 2) {
        pen.color = *(uint32_t *)&placement->disabled_color;
    } else if (flags & 1) {
        pen.color = halo::interface::hud_meter_flash_color_blend(&placement->flash, flash_start_time);
    } else {
        pen.color = *(uint32_t *)&placement->flash.default_color;
    }
    pen.y = origin.y;
    pen.anchor = *anchor;
    pen.is_sprite_bitmap = (*(int16_t *)digits_bitmap_data == 4);
    pen.advance = (float)digits->screen_digit_width * pen.scale;

    if (placement->flags & 4) {
        hud_number_draw_glyph(&pen, (uint16_t)(0xd + (thousands != 0)), 1);
    }

    if (placement->number_of_fractional_digits != 0 && (int16_t)fraction_value >= 0) {
        int16_t count = (placement->number_of_fractional_digits > 4) ? 4 : placement->number_of_fractional_digits;
        int16_t i;
        if (count < 4) {
            for (i = (int16_t)(4 - count); i != 0; i--) {
                fraction_value = (int16_t)fraction_value / 10;
            }
        }
        for (i = count; i > 0; i--) {
            int16_t digit = (int16_t)((int16_t)fraction_value % 10);
            fraction_value = (int16_t)fraction_value / 10;
            hud_number_draw_glyph(&pen, (uint16_t)digit, 1);
        }
        {
            int16_t right = (int16_t)(int32_t)(pen.advance + (float)pen.x);
            pen.x = (int16_t)(int32_t)((float)right - (float)digits->decimal_point_width * pen.scale);
        }
        hud_number_draw_glyph(&pen, 0xa, 1);
    }

    if (placement->maximum_number_of_digits > 0) {
        int16_t i;
        for (i = 0; i < placement->maximum_number_of_digits; i++) {
            int16_t digit = (int16_t)((int16_t)magnitude % 10);
            int32_t quotient = (int16_t)magnitude / 10;
            if ((int16_t)magnitude == 0 && (placement->flags & 1) == 0) {
                break;
            }
            hud_number_draw_glyph(&pen, (uint16_t)digit, 1);
            magnitude = quotient;
        }
    }

    if (negative) {
        hud_number_draw_glyph(&pen, 0xc, 0);
    }
}

/**
 * Draws the overlays of one HUD overlay element whose type bits (show on flashing, empty, reload/overheat,
 * default, always) intersect type_mask. draw_flags bit 0 allows flashing and frame animation from
 * flash_start_time; split_screen is forwarded to 0x4acad0.
 *
 * @address 0x4ac950
 */
void HudDraw::overlays(uint16_t *anchor, const hud_overlay_list *list, uint32_t type_mask, int32_t flash_start_time, uint32_t draw_flags, uint8_t split_screen)
{
    int32_t i;

    for (i = 0; i < (int32_t)list->overlays.count; i++) {
        const WeaponHUDInterfaceOverlay *overlay = (const WeaponHUDInterfaceOverlay *)list->overlays.pointer + i;
        uint32_t overlay_flags = *(const uint32_t *)&overlay->flags;
        datum_index tag_id = *(const datum_index *)&list->overlay_bitmap.tag_id;
        const BitmapGroupSequence *sequence;
        uint32_t color;
        int32_t frame;
        BitmapData *bitmap;
        int32_t sprite_uv;

        if ((overlay_flags & 2) != 0 || (type_mask & (uint32_t)(int32_t)(int16_t)overlay->type) == 0) {
            continue;
        }
        sequence = (const BitmapGroupSequence *)((Bitmap *)halo::cache::globals().tag_instances[tag_id & 0xffff].data)
                       ->bitmap_group_sequence.pointer + (int16_t)overlay->sequence_index;

        if ((overlay_flags & 1) != 0 && (draw_flags & 1) != 0) {
            color = halo::interface::hud_meter_flash_color_blend((const hud_flash_parameters *)&overlay->default_color,
                                                flash_start_time);
        } else {
            color = *(const uint32_t *)&overlay->default_color;
        }

        if ((*(const uint8_t *)&overlay->flags & 1) != 0 && (draw_flags & 1) != 0 && overlay->frame_rate > 0) {
            frame = ((halo::game::globals().game_time->game_time - flash_start_time) / overlay->frame_rate) / 30 %
                    (int32_t)sequence->sprites.count;
        } else {
            frame = 0;
        }

        bitmap = 0;
        sprite_uv = 0;
        halo::interface::hud_meter_resolve_bitmap_frame(tag_id, (int16_t)overlay->sequence_index, (uint16_t)frame,
                                       (void **)&bitmap, &sprite_uv);
        if (bitmap != 0 && halo::cache::texture_cache_get(bitmap, 0, 1) != 0) {
            halo::interface::hud_draw_bitmap_element((const float *)sprite_uv, (const hud_element_placement *)overlay, 0, 0,
                                    bitmap, anchor, 1.0f, 0.0f, color, split_screen);
        }
    }
}

/**
 * Draws one rotated HUD quad. uv is {u0, u1, v0, v1}, extents {x0, x1, y0, y1} around the screen position (see
 * hud_bitmap_anchor_extents), rotation in radians, color packed ARGB. meter_parameters is the
 * hud_meter_color_block of a meter fill, NULL for a plain bitmap.
 * blam-cc: screen_position -> EAX, scale -> ESI
 *
 * @address 0x4acd50
 */
void HudDraw::rotated_bitmap_quad(const Point2DInt *screen_position, const float *scale, void *meter_parameters, BitmapData *bitmap, const float *uv, const float *extents, float rotation, uint32_t color)
{
    ui_quad_render_state state;
    hud_quad_vertex vertices[4];
    float sine;
    float cosine;
    int16_t i;

    sine = sinf(rotation);
    cosine = cosf(rotation);
    for (i = 0; i < 4; i++) {
        int32_t corner = i + 1;
        float u = (corner & 2) != 0 ? uv[1] : uv[0];
        float v = i > 1 ? uv[3] : uv[2];
        float x = (corner & 2) != 0 ? extents[1] : extents[0];
        float y = i > 1 ? extents[3] : extents[2];
        float rotated;

        rotated = (x * cosine - y * sine) * scale[0];
        vertices[i].x = (float)(screen_position->x + (int32_t)lrint(rotated));
        rotated = (y * cosine + x * sine) * scale[1];
        vertices[i].y = (float)(screen_position->y + (int32_t)lrint(rotated));
        vertices[i].z = 0.0f;
        vertices[i].color = color;
        vertices[i].u = u;
        vertices[i].v = v;
    }

    memset(&state, 0, sizeof(state));
    state.meter_parameters = meter_parameters;
    state.map_texel_scales[0].y = 1.0f;
    state.map_texel_scales[0].x = 1.0f;
    state.map_scales[0].y = 1.0f;
    state.map_scales[0].x = 1.0f;
    state.single_local_player = 0;
    state.framebuffer_blend_function = 7;
    state.maps[0] = bitmap;
    halo::rasterizer::rasterizer_ui_quad_draw(&state, vertices);
}

/**
 * Draws one HUD static element (its bitmap sprite plus every multitexture overlay attached to it) at the
 * element anchor. draw_flags: bit 0 blend in the flashing color from flash_start_time, bit 1 use the disabled
 * color, bit 2 split screen.
 *
 * @address 0x4ac6f0
 */
void HudDraw::static_element(int16_t local_player_index, uint16_t *anchor, const hud_static_element_placement *element, uint32_t draw_flags, int32_t flash_start_time)
{
    Bitmap *bitmap_tag;
    BitmapData *bitmap;
    const float *uv;
    uint32_t color;
    uint8_t pixel_uvs;
    float scale;
    datum_index tag_id;
    int16_t i;

    tag_id = *(const datum_index *)&element->interface_bitmap.tag_id;
    bitmap_tag = (Bitmap *)halo::cache::globals().tag_instances[tag_id & 0xffff].data;
    bitmap = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(tag_id, 0, (int16_t)element->sequence_index);
    if (halo::cache::texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }

    uv = 0;
    if (tag_id != (datum_index)-1 && element->sequence_index != 0xffff) {
        Bitmap *tag = (Bitmap *)halo::cache::globals().tag_instances[tag_id & 0xffff].data;
        if ((int32_t)(int16_t)element->sequence_index < (int32_t)tag->bitmap_group_sequence.count) {
            BitmapGroupSequence *sequence =
                (BitmapGroupSequence *)tag->bitmap_group_sequence.pointer + (int16_t)element->sequence_index;
            int32_t sprite_count = (int32_t)sequence->sprites.count;
            if (sprite_count != 0) {
                BitmapGroupSprite *sprite = (BitmapGroupSprite *)sequence->sprites.pointer + 0 % sprite_count;
                uv = &sprite->left;
            }
        }
    }

    if ((draw_flags & 2) != 0) {
        color = *(const uint32_t *)&element->disabled_color;
    } else if ((draw_flags & 1) != 0) {
        color = halo::interface::hud_meter_flash_color_blend(&element->flash, flash_start_time);
    } else {
        color = *(const uint32_t *)&element->flash.default_color;
    }

    pixel_uvs = bitmap_tag->type == 4;
    scale = (*(const uint8_t *)&element->scaling_flags & 4) != 0 ? 0.5f : 1.0f;
    halo::interface::hud_draw_bitmap_element(uv, (const hud_element_placement *)element, pixel_uvs, 0, bitmap, anchor,
                            scale, 0.0f, color, (uint8_t)((draw_flags >> 2) & 1));

    if ((int32_t)element->multitexture_overlays.count > 0) {
        float default_uv[4];

        default_uv[0] = 0.0f;
        default_uv[2] = 0.0f;
        i = 0;
        do {
            const HUDInterfaceMultitextureOverlay *overlay =
                (const HUDInterfaceMultitextureOverlay *)element->multitexture_overlays.pointer + i;
            float element_scale[2];
            float extents[4];
            Point2DInt screen_position;
            uint8_t scale_offset;

            default_uv[1] = 1.0f;
            default_uv[3] = 1.0f;
            if (pixel_uvs != 0) {
                default_uv[1] = (float)(int32_t)(int16_t)bitmap->width;
                default_uv[3] = (float)(int32_t)(int16_t)bitmap->height;
            }
            if (uv == 0) {
                uv = default_uv;
            }
            element_scale[0] = scale * element->width_scale;
            element_scale[1] = scale * element->height_scale;
            scale_offset = 0;
            if ((int16_t)(draw_flags & 4) != 0 && (*(const uint8_t *)&element->scaling_flags & 1) == 0) {
                scale_offset = 1;
            }
            halo::interface::hud_anchor_offset_to_screen_position(anchor, scale_offset, 0.0f, &element->anchor_offset.x,
                                                 &screen_position.x, 0);
            halo::interface::hud_bitmap_anchor_extents(pixel_uvs, bitmap, uv, extents, (int16_t)*anchor);
            halo::interface::hud_draw_multitexture_overlay(element_scale, overlay, local_player_index, &screen_position, uv,
                                          extents, 0.0f, color);
            i++;
        } while ((int32_t)i < (int32_t)element->multitexture_overlays.count);
    }
}

}
