#include "halo/interface/ifr1_hud_meters.hpp"
#include "halo/bitmaps/api.hpp"
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern int32_t ui_real_to_int_truncate(float value);
extern uint32_t color_rgb_float_to_int(const float *rgb);
extern uint32_t color_pack_argb_from_real(ColorARGB *color);
extern void hud_draw_bitmap_element(const float *uv, const hud_element_placement *placement, uint8_t pixel_uvs,
                                    void *meter_parameters, BitmapData *bitmap, uint16_t *anchor,
                                    float scale, float rotation, uint32_t color, uint8_t split_screen);
extern double cos(double x);
extern double sqrt(double x);
extern double fmod(double x, double y);
extern game_time_globals *game_time;
extern int32_t bitmap_group_sequence_get_bitmap_offset(datum_index bitmap_tag, int16_t sequence_index,
                                                         int16_t frame_index);
extern data_array *player_data;
extern hud_unit_meter_globals *hud_unit_meters;
extern player_globals *local_player_globals;
extern void hud_unit_meters_update_for_player(int16_t local_player_index);
extern data_array *object_data;
extern hud_globals_flags *hud_flags;
extern cinematic_globals *cinematic_globals_ptr;
extern void hud_unit_sounds_update(player *p, uint8_t hud_enabled);
}

static int32_t hud_meter_alpha(const hud_meter_placement *meter, uint8_t value)
{
    int32_t rounded = ui_real_to_int_truncate((float)(int32_t)(meter->alpha_multiplier * value + meter->alpha_bias));
    int32_t clamped = (rounded < 0) ? 0 : (rounded > 0xff) ? 0xff : rounded;
    return ((int32_t)meter->minimum_meter_value > clamped) ? (int32_t)meter->minimum_meter_value : clamped;
}

static uint32_t hud_flash_blend(ColorARGB *a, ColorARGB *b, float s)
{
    ColorARGB out;
    halo::math::vector3d_lerp(*((real_vector3d *)&out), *(real_vector3d *)a, *(real_vector3d *)b, s);
    out.blue = (1.0f - s) * b->blue + a->blue * s;
    return color_pack_argb_from_real(&out);
}

namespace halo::interface {

/**
 * Original engine function hud_meter_draw_fill; the author notes are in
 * docs/original/interface/hud_meter_draw_fill.txt.
 * blam-cc: placement -> ESI
 *
 * @address 0x4abbc0
 */
void HudMeters::draw_fill(void *dest, uint8_t value_a, uint8_t value_b, uint32_t flags, float fraction, float fraction_2, const hud_meter_placement *meter)
{
    datum_index bitmap_tag = *(datum_index *)&meter->meter_bitmap.tag_id;
    uint8_t *bitmap_tag_data = (uint8_t *)halo::cache::globals().tag_instances[bitmap_tag & 0xffff].data;
    BitmapData *bitmap = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(bitmap_tag, 0, (int16_t)meter->sequence_index);
    const uint8_t *sprite_rect = 0;
    uint8_t is_sprite_bitmap;
    int32_t alpha_a;
    int32_t alpha_b;
    float alpha_scale = 1.0f;
    hud_meter_color_block block;
    ColorARGB gray;

    if (halo::cache::texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }

    if (bitmap_tag != (datum_index)-1 && meter->sequence_index != 0xffff) {
        uint8_t *bitmap_definition = (uint8_t *)halo::cache::globals().tag_instances[bitmap_tag & 0xffff].data;
        int16_t sequence = (int16_t)meter->sequence_index;
        if (sequence < *(int32_t *)(bitmap_definition + 0x54)) {
            uint8_t *sequence_entry = *(uint8_t **)(bitmap_definition + 0x58) + sequence * 0x40;
            int32_t sprite_count = *(int32_t *)(sequence_entry + 0x34);
            if (sprite_count != 0) {
                sprite_rect = *(uint8_t **)(sequence_entry + 0x38) + (0 % sprite_count) * 0x20 + 8;
            }
        }
    }
    is_sprite_bitmap = (*(int16_t *)bitmap_tag_data == 4);

    alpha_a = hud_meter_alpha(meter, value_a);
    alpha_b = hud_meter_alpha(meter, value_b);
    if (meter->scaling_flags & 4) {
        alpha_scale = 0.5f;
    }

    if (flags & 2) {
        block.primary = 0;
        block.tint = 0;
        block.secondary = 0;
    } else if ((meter->flags & 1) == 0) {
        ColorRGB flash;
        float t;
        if (fraction < 0.0f) {
            t = 0.0f;
        } else {
            t = 1.0f - fraction;
            if (t < 0.0f) {
                t = 0.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
            }
        }
        halo::bitmaps::color_rgb_int_to_real(&flash, *(uint32_t *)&meter->flash_color);
        flash.red *= t;
        flash.green *= t;
        flash.blue *= t;
        block.primary = (*(uint32_t *)&meter->color_at_meter_minimum & 0xffffff) | ((uint32_t)(int16_t)alpha_a << 24);
        block.secondary = *(uint32_t *)&meter->color_at_meter_maximum & 0xffffff;
        block.tint = (color_rgb_float_to_int((const float *)&flash) & 0xffffff) | ((uint32_t)(int16_t)alpha_b << 24);
    } else if ((flags & 1) && (meter->flags & 2)) {
        ColorRGB minimum, maximum, blended;
        uint32_t alpha = (uint32_t)(int16_t)alpha_a << 24;
        float t = (meter->flags & 0x10) ? 1.0f - fraction_2 : fraction_2;
        halo::bitmaps::color_rgb_int_to_real(&minimum, *(uint32_t *)&meter->color_at_meter_minimum);
        halo::bitmaps::color_rgb_int_to_real(&maximum, *(uint32_t *)&meter->color_at_meter_maximum);
        halo::bitmaps::color_interpolate(&maximum, &minimum, &blended, static_cast<color_interpolation_flags>(0), t);
        block.primary = color_rgb_float_to_int((const float *)&blended) | alpha;
        block.secondary = color_rgb_float_to_int((const float *)&blended);
        block.tint = alpha;
    } else {
        uint32_t rgb = *(uint32_t *)(((flags & 1) == 0) ? &meter->color_at_meter_minimum
                                                         : &meter->color_at_meter_maximum) & 0xffffff;
        uint32_t alpha = (uint32_t)(int16_t)alpha_a << 24;
        block.primary = rgb | alpha;
        block.tint = alpha;
        block.secondary = rgb;
    }

    {
        uint32_t empty = *(uint32_t *)&meter->empty_color;
        float inverse_opacity = 1.0f - meter->opacity;
        block.empty = ((uint32_t)(-1 - (int32_t)(empty >> 24)) << 24) | (empty & 0xffffff);
        gray.alpha = meter->translucency;
        gray.red = inverse_opacity;
        gray.green = inverse_opacity;
        gray.blue = inverse_opacity;
        block.opacity = color_pack_argb_from_real(&gray);
    }
    block.scale = 1.0f;
    block.flag_10 = 0;
    block.flag_11 = 1;

    hud_draw_bitmap_element((const float *)sprite_rect, (const hud_element_placement *)meter, is_sprite_bitmap,
                            &block, bitmap, (uint16_t *)dest, alpha_scale, 0.0f, 0xffffffffu,
                            (uint8_t)((flags >> 2) & 1));
}

/**
 * For every entry in target's element list (target tag data + 0xb8 count, +0xbc pointer to 0x9c-byte name
 * records), searches source's element list (source tag data + 0x68 count, +0x6c pointer to 0x40-byte name
 * records, matched in name order starting after the previous match) for a name match, writing the found source
 * index into out[i] on success. Returns 1 (as the low byte of a value whose upper bits are decompiler noise)
 * only if every target entry found a match; returns 0 as soon as one target entry runs out of source entries
 * to check, but keeps scanning the remaining target entries regardless.
 * blam-cc: EAX -> source_tag_ref, ECX -> target_tag_ref, stack -> out
 *
 * @address 0x493f00
 */
uint8_t HudMeters::find_matching_elements(uint32_t source_tag_ref, uint32_t target_tag_ref, int16_t *out)
{
    char *target_data = (char *)halo::cache::globals().tag_instances[(uint16_t)target_tag_ref].data;
    char *source_data = (char *)halo::cache::globals().tag_instances[(uint16_t)source_tag_ref].data;
    uint8_t all_matched = 1;
    int32_t target_count = *(int32_t *)(target_data + 0xb8);
    int32_t target_index;
    int32_t source_count = *(int32_t *)(source_data + 0x68);
    char *source_names = *(char **)(source_data + 0x6c);
    char *target_names = *(char **)(target_data + 0xbc);

    if (target_count <= 0) {
        return 1;
    }

    for (target_index = 0; target_index < target_count; target_index++) {
        char *target_name = target_names + target_index * 0x9c;
        int16_t source_index;
        uint8_t found = 0;

        for (source_index = 0; source_index < source_count; source_index++) {
            char *source_name = source_names + source_index * 0x40;
            if (strcmp(source_name, target_name) == 0) {
                found = 1;
                break;
            }
        }
        if (found) {
            out[target_index] = source_index;
        } else {
            all_matched = 0;
        }
    }
    return all_matched;
}

/**
 * Returns the packed 0xAARRGGBB color of a HUD element with flashing parameters at this moment.
 * blam-cc: flash -> ESI, start_time -> EDI
 *
 * @address 0x4ab980
 */
uint32_t HudMeters::flash_color_blend(const hud_flash_parameters *flash, int32_t start_time)
{
    ColorARGB default_color;
    ColorARGB flashing_color;
    float cycle_time;
    float flash_time;

    if (flash->flash_period == 0.0f || flash->flash_length == 0.0f) {
        halo::bitmaps::color_argb_int_to_real(&default_color, *(uint32_t *)&flash->default_color);
        return color_pack_argb_from_real(&default_color);
    }

    cycle_time = (float)fmod((float)(game_time->game_time - start_time) * (1.0f / 30.0f),
                             flash->flash_period);
    halo::bitmaps::color_argb_int_to_real(&default_color, *(uint32_t *)&flash->default_color);
    halo::bitmaps::color_argb_int_to_real(&flashing_color, *(uint32_t *)&flash->flashing_color);

    if ((float)flash->number_of_flashes * (flash->flash_delay + flash->flash_length) <= cycle_time) {
        return color_pack_argb_from_real(&default_color);
    }
    flash_time = (float)fmod(cycle_time, flash->flash_delay + flash->flash_length);

    if (start_time == 0) {
        return color_pack_argb_from_real((flash->flash_flags & 1) ? &default_color : &flashing_color);
    }
    if (flash_time < flash->flash_length) {
        double wave = 1.0 - (cos(flash_time / flash->flash_length * 6.283f) + 1.0) * 0.5;
        float s;
        if (wave < 0.0) {
            wave = 0.0;
        } else if (wave > 1.0) {
            wave = 1.0;
        }
        s = (float)sqrt(wave);
        if (flash->flash_flags & 1) {
            return hud_flash_blend(&default_color, &flashing_color, s);
        }
        return hud_flash_blend(&flashing_color, &default_color, s);
    }
    return color_pack_argb_from_real((flash->flash_flags & 1) ? &flashing_color : &default_color);
}

/**
 * For each of target_tag_ref's element-list entries (target tag data + 0xb8 count, the same reflexive
 * hud_meter_find_matching_elements.c reads), copies one 0x34 byte record from source[lookup[i]] into dest[i]
 * -- i.e. re-orders `source` into `dest` according to a previously-built match table.
 * blam-cc: EAX -> target_tag_ref, EBX -> lookup, stack -> (dest, source)
 *
 * @address 0x493ea0
 */
void HudMeters::permute_node_records(uint8_t *dest, uint8_t *source, uint32_t target_tag_ref, int16_t *lookup)
{
    char *target_data = (char *)halo::cache::globals().tag_instances[(uint16_t)target_tag_ref].data;
    int32_t count = *(int32_t *)(target_data + 0xb8);
    int32_t i;

    for (i = 0; i < count; i++) {
        memcpy(dest + i * 0x34, source + lookup[i] * 0x34, 0x34);
    }
}

/**
 * Original engine function hud_meter_resolve_bitmap_frame; the author notes are in
 * docs/original/interface/hud_meter_resolve_bitmap_frame.txt.
 * blam-cc: EAX -> frame_index, stack -> bitmap_tag, sequence_index, out_data, out_offset
 *
 * @address 0x4ab8d0
 */
void HudMeters::resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index, void **out_data, int32_t *out_offset)
{
    int32_t frame = frame_index;

    if (bitmap_tag != (datum_index)-1) {
        Bitmap *bitmap = (Bitmap *)halo::cache::globals().tag_instances[bitmap_tag & 0xffff].data;
        int16_t sequence = (int16_t)sequence_index;

        if (sequence < (int32_t)bitmap->bitmap_group_sequence.count) {
            BitmapGroupSequence *group = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer + sequence;
            int32_t sprite_count = (int32_t)group->sprites.count;

            frame &= 0x7fff;
            if (sprite_count != 0) {
                BitmapGroupSprite *sprite = (BitmapGroupSprite *)group->sprites.pointer + (int16_t)frame % sprite_count;

                *out_data = (BitmapData *)bitmap->bitmap_data.pointer + (int16_t)sprite->bitmap_index;
            } else {
                *out_data = halo::bitmaps::bitmap_group_sequence_get_bitmap_data(bitmap_tag, (int16_t)frame, sequence);
            }
        }
    }
    if (*out_data != 0) {
        *out_offset = bitmap_group_sequence_get_bitmap_offset(bitmap_tag, (int16_t)sequence_index, (int16_t)frame);
    } else {
        *out_offset = 0;
    }
}

/**
 * Subtracts damage directly from a local player's displayed (smoothed) shield meter value, so the HUD shows
 * the hit immediately instead of waiting for the next authoritative object update to drive the smoothing in
 * hud_unit_meters_update_for_player.
 * blam-cc: player_index -> ECX, damage -> stack param_1
 *
 * @address 0x4b16e0
 */
void HudMeters::unit_meter_apply_predictive_damage(datum_index player_index, float damage)
{
    int16_t index;
    int16_t salt;
    player *p;

    if (player_index == (datum_index)-1) {
        return;
    }
    index = (int16_t)player_index;
    if (index < 0 || index >= player_data->maximum_count) {
        return;
    }
    p = (player *)((uint8_t *)player_data->data + player_data->size * index);
    if (p->identifier == 0) {
        return;
    }
    salt = (int16_t)((uint32_t)player_index >> 16);
    if (salt != 0 && p->identifier != salt) {
        return;
    }
    if (p->local_player_index == -1) {
        return;
    }
    {
        hud_unit_meters->players[p->local_player_index].displayed_shield -= damage;
    }
}

/**
 * Original engine function hud_unit_meters_update; the author notes are in
 * docs/original/interface/hud_unit_meters_update.txt.
 *
 * @address 0x4b0110
 */
void HudMeters::unit_meters_update(void)
{
    int16_t local_player_index = local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1;

    while (local_player_index != -1) {
        hud_unit_meters_update_for_player(local_player_index);
        local_player_index = (local_player_globals->local_players[0] != (datum_index)-1 && local_player_index < 0)
                                 ? 0 : -1;
    }
}

/**
 * Original engine function hud_unit_meters_update_for_player; the author notes are in
 * docs/original/interface/hud_unit_meters_update_for_player.txt.
 * blam-cc: local_player_index -> DI
 *
 * @address 0x4b0160
 */
void HudMeters::unit_meters_update_for_player(int16_t local_player_index)
{
    datum_index player_index;

    if (local_player_index != -1 && local_player_index < 1) {
        player_index = local_player_globals->local_players[local_player_index];
        if (player_index != (datum_index)-1) {
            datum_index unit_index = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200))->unit;

            if (unit_index != (datum_index)-1) {
                uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
                hud_unit_meter_state *state = &hud_unit_meters->players[local_player_index];
                float shield = ((unit_object *)unit)->base.shield_vitality;

                if (state->displayed_health == -1.0f) {
                    state->displayed_health = ((unit_object *)unit)->base.body_vitality;
                }
                if (state->displayed_shield == -1.0f) {
                    state->displayed_shield = shield;
                }
                if (state->displayed_shield > shield) {
                    int32_t elapsed;

                    if (state->shield_drain_time < 0.0f || state->shield_drain_time > 1.0f) {
                        state->shield_update_time = game_time->game_time;
                    }
                    elapsed = game_time->game_time - state->shield_update_time;
                    if (elapsed < 0xf) {
                        state->shield_drain_time = 0.0f;
                    } else {
                        state->displayed_shield = ((unit_object *)unit)->base.shield_vitality;
                        state->shield_drain_time = (float)(game_time->game_time - state->shield_update_time) *
                                                       0.03333333507180214f + state->shield_drain_time;
                        state->shield_update_time = game_time->game_time;
                    }
                } else if (state->displayed_shield < shield) {
                    state->displayed_shield = shield;
                    state->shield_drain_time = -1.0f;
                    state->shield_update_time = game_time->game_time;
                } else {
                    state->displayed_shield = shield;
                    if (state->shield_drain_time > 0.0f) {
                        state->shield_drain_time = (float)(game_time->game_time - state->shield_update_time) *
                                                       0.03333333507180214f + state->shield_drain_time;
                    }
                    state->shield_update_time = game_time->game_time;
                }
            }
        }
    }

    if (cinematic_globals_ptr->in_progress != 0 && local_player_index != -1 && local_player_index < 1) {
        player_index = local_player_globals->local_players[local_player_index];
        if (player_index != (datum_index)-1) {
            hud_unit_sounds_update((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200),
                                   hud_flags->hud_enabled);
        }
    }
}

}
