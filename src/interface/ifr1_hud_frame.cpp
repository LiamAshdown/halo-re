#include "halo/interface/ifr1_hud_frame.hpp"
#include <string.h>
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"

extern "C" {
extern data_array *player_data;
extern player_globals *local_player_globals;
extern HUDGlobals *hud_globals_tag_data;
extern int16_t render_viewport_top;
extern int16_t render_viewport_left;
extern float hud_damage_indicator_screen_center_x;
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void hud_meter_resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index,
                                           void **out_data, int32_t *out_offset);
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing);
extern void hud_draw_bitmap_at(const float *uv, BitmapData *bitmap, uint8_t pixel_uvs, int16_t anchor,
                               const Point2DInt *screen_position, float scale, float rotation, uint32_t color);
extern data_array *object_data;
extern tag_instance *tag_instances;
extern Globals *global_globals;
extern game_time_globals *game_time;
extern hud_weapon_interface_state *hud_weapon_state;
extern int8_t unit_get_current_grenade_index(uint32_t unit_index);
extern int32_t unit_get_grenade_count(uint32_t unit_index, int16_t grenade_type);
extern void hud_draw_static_element(int16_t local_player_index, uint16_t *anchor,
                                    const hud_static_element_placement *element, uint32_t draw_flags,
                                    int32_t flash_start_time);
extern void hud_draw_number(void *unused, uint16_t *anchor, const hud_number_placement *placement, int16_t value,
                            int16_t fraction, uint32_t flags, int32_t flash_start_time, float scale);
extern void hud_draw_overlays(uint16_t *anchor, const hud_overlay_list *list, uint32_t type_mask,
                              int32_t flash_start_time, uint32_t draw_flags, uint8_t split_screen);
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index);
extern int16_t unit_count_deployed_weapons(datum_index unit_index);
extern void hud_weapon_crosshairs_draw(datum_index hud_tag, const player *p, const weapon_hud_ammo_state *ammo);
extern void hud_weapon_interface_draw_elements(datum_index hud_tag, int16_t local_player_index, const Weapon *weapon_tag,
                                               const weapon_hud_ammo_state *ammo, const uint16_t *parent_state_flags,
                                               const uint16_t *parent_overlay_types, const int16_t *parent_numbers);
extern void hud_draw_grenade_interface(int16_t local_player_index, datum_index unit_index);
extern hud_unit_meter_globals *hud_unit_meters;
extern int16_t current_local_player_index;
extern game_engine_definition *current_game_engine;
extern game_variant game_engine_variant;
extern int32_t hud_overshield_layer_count;
extern datum_index hud_team_icon_bitmap;
extern datum_index hud_team_background_bitmap;
extern long lrint(double x);
extern int32_t ui_real_to_int_truncate(float value);
extern uint8_t game_engine_is_valid_team_player(uint32_t identifier);
extern uint8_t game_engine_scores_tracked_individually(void);
extern float *game_engine_get_player_color(uint32_t player_index, float *out_rgb);
extern TagID unit_get_hud_interface_tag_id(Unit *unit_tag, uint8_t use_second);
extern TagID unit_get_seat_hud_interface_tag_id(Unit *unit_tag, int16_t seat_index, uint8_t use_second);
extern datum_index tag_lookup(tag_group group, char *path);
extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence);
extern uint32_t color_rgb_float_to_int(const float *rgb);
extern void hud_anchor_offset_to_screen_position(uint16_t *anchor, uint8_t has_scale, float scale,
                                                 const int16_t *offset, int16_t *out, int32_t selector);
extern void hud_meter_draw_fill(void *dest, uint8_t value_a, uint8_t value_b, uint32_t flags,
                                float fraction, float fraction_2, const hud_meter_placement *meter);
extern void hud_draw_rotated_bitmap_quad(const Point2DInt *screen_position, const float *scale,
                                         void *meter_parameters, BitmapData *bitmap, const float *uv,
                                         const float *extents, float rotation, uint32_t color);
extern void motion_sensor_update_for_player(int16_t local_player_index);
extern void motion_sensor_render(uint8_t splitscreen, const int16_t *screen_center,
                                 int16_t local_player_index);
extern int32_t game_state_cursor;
extern uint8_t *game_state_base;
extern uint32_t game_state_crc;
extern hud_globals_flags *hud_flags;
extern hud_messaging_globals *hud_messaging;
extern hud_waypoint_state *hud_waypoints;
extern motion_sensor_globals *motion_sensor;
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length);
extern HUDGlobals *hud_messaging_parameters;
extern void motion_sensor_reset(void);
extern void hud_weapon_interface_state_update(void);
extern void hud_unit_meters_update(void);
extern void hud_waypoints_update(void);
extern void hud_unit_sounds_update(player *p, uint8_t hud_enabled);
extern int32_t game_engine_state_value;
}

static int32_t hud_alpha_round(float value)
{
    int32_t rounded = (int32_t)lrint((double)value);
    if (rounded < 0) {
        return 0;
    }
    if (rounded > 0xff) {
        return 0xff;
    }
    return rounded;
}

static int32_t hud_alpha_truncate(float value)
{
    int32_t truncated = ui_real_to_int_truncate(value);
    if (truncated < 0) {
        return 0;
    }
    if (truncated > 0xff) {
        return 0xff;
    }
    return truncated;
}

static float hud_fraction_clamp(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

namespace halo::interface {

/**
 * Draws the local player's four directional damage indicators (edge-of-screen arrows) for every direction
 * whose pre-fade alpha byte is still in its "recently hit" window (1..0x1d); the value 0 means off and >= 0x1e
 * means fully faded/invisible. Called once per frame per local player; hides every indicator in one write when
 * the player currently has no unit.
 * blam-cc: local_player_index -> EAX
 *
 * @address 0x4b14c0
 */
void HudFrame::draw_damage_indicators(int16_t local_player_index)
{
    datum_index unit_index;
    object *unit;
    uint32_t previous_indicators;
    uint8_t *previous_bytes = (uint8_t *)&previous_indicators;
    int direction;

    if (local_player_index == -1) {
        return;
    }

    if (local_player_index >= 1) {
        unit_index = (datum_index)-1;
    } else {
        datum_index player_index = local_player_globals->local_players[local_player_index];
        if (player_index == (datum_index)-1) {
            unit_index = (datum_index)-1;
        } else {
            player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
            unit_index = p->unit;
        }
    }

    unit = object_try_and_get(unit_index, 3);
    if (unit == 0) {
        *(uint32_t *)halo::effects::globals().player_effect_state->players[local_player_index].damage_indicator_alpha = 0;
        return;
    }

    {
        uint8_t *hud = (uint8_t *)hud_globals_tag_data;
        uint8_t *edge_offsets = hud + 0x310;
        datum_index icon_bitmap = *(datum_index *)(hud + 0x344);
        uint16_t sequence_index = (local_player_globals->local_player_count <= 1)
            ? *(uint16_t *)(hud + 0x348)
            : *(uint16_t *)(hud + 0x34a);
        uint32_t icon_color = *(uint32_t *)(hud + 0x34c);

        halo::effects::player_effect_fade_damage_indicators(local_player_index, &previous_indicators);

        for (direction = 0; direction < 4; direction++) {
            float x, y;
            uint32_t rotation_bits;
            void *bitmap_data = 0;
            int32_t sprite_rect = 0;
            Point2DInt position;

            if (previous_bytes[direction] == 0 || previous_bytes[direction] >= 0x1e) {
                continue;
            }

            switch (direction) {
            case 1:
                x = (float)(*(int16_t *)(edge_offsets + 4) + 8);
                y = 240.0f;
                rotation_bits = 0x3fc90fdb;
                break;
            case 2:
                x = hud_damage_indicator_screen_center_x;
                y = (float)(0x1d8 - *(int16_t *)(edge_offsets + 2));
                rotation_bits = 0;
                break;
            case 3:
                x = (float)(0x278 - *(int16_t *)(edge_offsets + 6));
                y = 240.0f;
                rotation_bits = 0x4096cbe4;
                break;
            default:
                x = hud_damage_indicator_screen_center_x;
                y = (float)(*(int16_t *)edge_offsets + 8);
                rotation_bits = 0x40490fdb;
                break;
            }
            x -= (float)render_viewport_left;
            y -= (float)render_viewport_top;

            hud_meter_resolve_bitmap_frame(icon_bitmap, (int16_t)sequence_index, 0, &bitmap_data, &sprite_rect);
            if (bitmap_data == 0) {
                continue;
            }
            if (texture_cache_get((BitmapData *)bitmap_data, 0, 1) == 0) {
                continue;
            }
            position.x = (int16_t)(int32_t)x;
            position.y = (int16_t)(int32_t)y;
            hud_draw_bitmap_at((const float *)sprite_rect, (BitmapData *)bitmap_data, 0, 4, &position, 1.0f,
                               *(float *)&rotation_bits, icon_color);
        }
    }
}

/**
 * Original engine function hud_draw_grenade_interface; the author notes are in
 * docs/original/interface/hud_draw_grenade_interface.txt.
 *
 * @address 0x4b2ac0
 */
void HudFrame::draw_grenade_interface(int16_t local_player_index, datum_index unit_index)
{
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    int16_t slot = ((unit_object *)unit)->unit.current_weapon_index;
    datum_index weapon = slot != -1 ? *(datum_index *)(unit + 0x2f8 + slot * 4) : (datum_index)-1;
    int8_t grenade = ((unit_object *)unit)->unit.current_grenade_index;
    uint8_t *parent;
    datum_index hud_tag;
    GrenadeHUDInterface *hud;
    int32_t *flash_start_time;
    int8_t count;
    uint32_t flags;

    if (halo::items::weapon_prevents_grenade_throwing(weapon) != 0 || grenade == -1) {
        return;
    }
    parent = (uint8_t *)object_try_and_get(((unit_object *)unit)->base.parent_object, 3);
    if (parent != 0 && (*(datum_index *)(parent + 0x324) == unit_index || *(datum_index *)(parent + 0x328) == unit_index)) {
        return;
    }
    hud_tag = *(datum_index *)(*(uint8_t **)((uint8_t *)global_globals + 0x12c) + grenade * 0x44 + 0x20);
    flash_start_time = (int32_t *)((uint8_t *)hud_weapon_state + local_player_index * 0x28 + 0x24);
    if (hud_tag == (datum_index)-1) {
        return;
    }
    hud = (GrenadeHUDInterface *)tag_instances[hud_tag & 0xffff].data;

    count = *(int8_t *)(unit + 0x31e + grenade);
    flags = (count <= hud->flash_cutoff ? 1 : 0) | (count == 0 ? 2 : 0) | (local_player_globals->local_player_count > 1 ? 4 : 0);
    if ((flags & 1) != 0) {
        if (*flash_start_time == -1) {
            *flash_start_time = game_time->game_time;
        }
    } else {
        *flash_start_time = -1;
    }

    if (*(datum_index *)&((struct GrenadeHUDInterface *)hud)->background_interface_bitmap.tag_id != (datum_index)-1) {
        hud_draw_static_element(local_player_index, (uint16_t *)hud,
                                (const hud_static_element_placement *)&hud->background_anchor_offset, flags,
                                *flash_start_time);
    }
    if (*(datum_index *)&((struct GrenadeHUDInterface *)hud)->total_grenades_background_interface_bitmap.tag_id != (datum_index)-1) {
        hud_draw_static_element(local_player_index, (uint16_t *)hud,
                                (const hud_static_element_placement *)&hud->total_grenades_background_anchor_offset,
                                flags, *flash_start_time);
    }
    if (hud->total_grenades_numbers_maximum_number_of_digits != 0) {
        int32_t total = unit_get_grenade_count(unit_index, unit_get_current_grenade_index(unit_index));
        hud_draw_number((void *)(int32_t)local_player_index, (uint16_t *)hud,
                        (const hud_number_placement *)&hud->total_grenades_numbers_anchor_offset, (int16_t)total, -1,
                        flags, *flash_start_time, 0.0f);
    }
    if (*(datum_index *)&hud->total_grenades_overlay_bitmap.tag_id != (datum_index)-1) {
        uint16_t types;

        count = *(int8_t *)(unit + 0x31e + ((unit_object *)unit)->unit.current_grenade_index);
        types = (uint16_t)((count <= hud->flash_cutoff ? 1 : 0) | (count == 0 ? 2 : 0));
        types = types == 0 ? 4 : (uint16_t)(types & 0xfffb);
        hud_draw_overlays((uint16_t *)hud, (const hud_overlay_list *)&hud->total_grenades_overlay_bitmap,
                          (uint32_t)(int16_t)types | 8, *flash_start_time, flags,
                          local_player_globals->local_player_count > 1);
    }
}

/**
 * Original engine function hud_draw_weapon_interface; the author notes are in
 * docs/original/interface/hud_draw_weapon_interface.txt.
 *
 * @address 0x4b1e20
 */
void HudFrame::draw_weapon_interface(player *p)
{
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data;
    int16_t slot = ((unit_object *)unit)->unit.current_weapon_index;
    datum_index weapon = slot != -1 ? *(datum_index *)(unit + 0x2f8 + slot * 4) : (datum_index)-1;
    uint8_t no_weapon = 0;
    weapon_hud_ammo_state ammo;

    if (weapon == (datum_index)-1) {
        datum_index parent;
        int16_t seat;

        unit = (uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data;
        parent = ((unit_object *)unit)->base.parent_object;
        seat = ((unit_object *)unit)->unit.vehicle_seat_index;
        if (parent == (datum_index)-1 || seat == -1) {
            no_weapon = 1;
        } else {
            uint8_t *parent_object = (uint8_t *)((object_header *)object_data->data)[parent & 0xffff].data;
            uint8_t *seats = *(uint8_t **)((uint8_t *)tag_instances[*(datum_index *)parent_object & 0xffff].data + 0x2e8);

            if ((seats[seat * 0x11c] & 8) != 0) {
                weapon = unit_get_weapon_object_index(parent, *(int16_t *)(parent_object + 0x2f2));
                if (weapon == (datum_index)-1) {
                    no_weapon = 1;
                }
            }
        }
    }

    if (weapon != (datum_index)-1) {
        uint8_t *weapon_object = (uint8_t *)((object_header *)object_data->data)[weapon & 0xffff].data;
        Weapon *weapon_tag = (Weapon *)tag_instances[*(datum_index *)weapon_object & 0xffff].data;
        datum_index hud_tag;

        halo::items::weapon_build_hud_ammo_state(weapon, &ammo);
        hud_tag = *(datum_index *)&((struct Weapon *)weapon_tag)->hud_interface.tag_id;
        if (hud_tag != (datum_index)-1) {
            hud_weapon_crosshairs_draw(hud_tag, p, &ammo);
            hud_weapon_interface_draw_elements(hud_tag, p->local_player_index, weapon_tag, &ammo, 0, 0, 0);
        }
    } else if (no_weapon && unit_count_deployed_weapons(p->unit) == 0) {
        memset(&ammo, 0, sizeof(ammo));
        hud_weapon_crosshairs_draw(*(datum_index *)((uint8_t *)hud_globals_tag_data + 0x2cc), p, &ammo);
    }

    hud_draw_grenade_interface(p->local_player_index, p->unit);
    if (p->local_player_index != -1) {
        *(datum_index *)((uint8_t *)hud_weapon_state + p->local_player_index * 0x28 + 0x20) = weapon;
    }
}

/**
 * Fills out with the HUD ammo state of the weapon the player is holding, or of the seat weapon of the vehicle
 * it rides. Returns 0 (out untouched) when there is none.
 * blam-cc: player -> EAX
 *
 * @address 0x4acef0
 */
uint8_t HudFrame::player_weapon_ammo_state(const player *p, weapon_hud_ammo_state *out)
{
    uint8_t *unit;
    datum_index weapon;
    int16_t slot;

    unit = (uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data;
    slot = ((unit_object *)unit)->unit.current_weapon_index;
    weapon = (datum_index)-1;
    if (slot != -1) {
        weapon = *(datum_index *)(unit + 0x2f8 + slot * 4);
    }
    if (weapon == (datum_index)-1) {
        datum_index parent;
        int16_t seat;
        uint8_t *parent_object;
        uint8_t *seats;

        unit = (uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data;
        parent = ((unit_object *)unit)->base.parent_object;
        if (parent == (datum_index)-1) {
            return 0;
        }
        seat = ((unit_object *)unit)->unit.vehicle_seat_index;
        if (seat == -1) {
            return 0;
        }
        parent_object = (uint8_t *)((object_header *)object_data->data)[parent & 0xffff].data;
        seats = *(uint8_t **)((uint8_t *)tag_instances[*(datum_index *)parent_object & 0xffff].data + 0x2e8);
        if ((seats[seat * 0x11c] & 8) == 0) {
            return 0;
        }
        parent = ((unit_object *)unit)->base.parent_object;
        parent_object = (uint8_t *)((object_header *)object_data->data)[parent & 0xffff].data;
        weapon = unit_get_weapon_object_index(parent, *(int16_t *)(parent_object + 0x2f2));
        if (weapon == (datum_index)-1) {
            return 0;
        }
    }
    halo::items::weapon_build_hud_ammo_state(weapon, out);
    return 1;
}

/**
 * Original engine function hud_render_unit_interface; the author notes are in
 * docs/original/interface/hud_render_unit_interface.txt.
 *
 * @address 0x4b0320
 */
void HudFrame::render_unit_interface(player *p)
{
    datum_index objects[18];
    datum_index hud_tags[18];
    hud_unit_meter_state *state;
    uint8_t *unit_object;
    Unit *unit_tag;
    int16_t local_player_index;
    datum_index player_index;
    int32_t count;
    int32_t index;
    uint8_t split_screen;
    uint32_t enabled_meters;
    uint32_t blinking_meters;
    float meter_values[1];
    int32_t i;

    local_player_index = p->local_player_index;
    if (local_player_index != current_local_player_index || p->unit == (datum_index)-1) {
        return;
    }
    unit_object = (uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data;
    unit_tag = (Unit *)tag_instances[*(datum_index *)unit_object & 0xffff].data;
    player_index = (local_player_index != -1 && local_player_index < 1)
                       ? local_player_globals->local_players[local_player_index] : (datum_index)-1;
    state = &hud_unit_meters->players[local_player_index];

    for (i = 1; i < 18; i++) {
        objects[i] = 0;
        hud_tags[i] = 0;
    }
    objects[0] = p->unit;
    {
        int32_t last = (int32_t)((struct Unit *)unit_tag)->new_hud_interfaces.count - 1;
        int32_t choice = (int16_t)(local_player_globals->local_player_count > 1);
        if (choice > last) {
            choice = last;
        }
        hud_tags[0] = (int16_t)choice < 0
            ? (datum_index)-1
            : *(datum_index *)(*(uint8_t **)&((struct Unit *)unit_tag)->new_hud_interfaces.pointer + (int16_t)choice * 0x30 + 0xc);
    }
    count = 1;

    {
        uint8_t valid_team_player = game_engine_is_valid_team_player(p->unit);
        datum_index parent;

        if (state->last_unit == (datum_index)-1) {
            hud_unit_meter_state *reset = &hud_unit_meters->players[p->local_player_index];
            reset->auxiliary_meter_timers[0] = -1;
            reset->displayed_health = -1.0f;
            reset->displayed_shield = -1.0f;
            reset->health_flash_start_time = -1;
            reset->motion_sensor_flash_start_time = -1;
            reset->shield_drain_time = -1.0f;
            reset->last_unit = (datum_index)-1;
        }
        state->last_unit = p->unit;

        parent = *(datum_index *)(unit_object + 0x11c);
        if (parent != (datum_index)-1 && *(int16_t *)(unit_object + 0x2f0) != -1) {
            uint8_t *parent_object = (uint8_t *)((object_header *)object_data->data)[parent & 0xffff].data;
            Unit *parent_tag = (Unit *)tag_instances[*(datum_index *)parent_object & 0xffff].data;
            uint8_t split = local_player_globals->local_player_count > 1;
            TagID parent_hud = unit_get_hud_interface_tag_id(parent_tag, split);
            uint8_t *seats = *(uint8_t **)&((struct Unit *)parent_tag)->seats.pointer;

            if ((seats[*(int16_t *)(unit_object + 0x2f0) * 0x11c] & 4) != 0) {
                datum_index child;

                if (*(datum_index *)&parent_hud != (datum_index)-1) {
                    objects[1] = parent;
                    hud_tags[1] = *(datum_index *)&parent_hud;
                    count = 2;
                }
                for (child = ((object *)parent_object)->first_child_object; child != (datum_index)-1 && (uint32_t)count < 18;) {
                    uint8_t *child_data = (uint8_t *)((object_header *)object_data->data)[child & 0xffff].data;
                    uint8_t *child_object = (uint8_t *)object_try_and_get(child, 3);

                    if (child_object != 0 && ((struct object *)child_object)->parent_object == parent &&
                        *(int16_t *)(child_object + 0x2f0) != -1) {
                        TagID seat_hud = unit_get_seat_hud_interface_tag_id(parent_tag, *(int16_t *)(child_object + 0x2f0),
                                                                            split);
                        objects[count] = child;
                        hud_tags[count] = *(datum_index *)&seat_hud;
                        count++;
                    }
                    child = ((object *)child_data)->next_object;
                }
            }
        }

        enabled_meters = (valid_team_player != 0 && *(float *)(unit_object + 0x340) == 1.0f) ? 1 : 0;
        blinking_meters = 0;
        if ((*(uint32_t *)(unit_object + 0x204) & 0x80000) == 0 && *(float *)(unit_object + 0x344) < 0.2f &&
            (*(uint8_t *)(unit_object + 0x208) & 0x10) != 0) {
            blinking_meters = 1;
        }
        meter_values[0] = *(float *)(unit_object + 0x344);
    }

    split_screen = local_player_globals->local_player_count > 1;
    index = count;
    do {
        UnitHUDInterface *hud;
        uint8_t *object;
        datum_index hud_tag;
        uint32_t flags;

        index--;
        object = (uint8_t *)object_try_and_get(objects[index], 3);
        if (object == 0) {
            continue;
        }
        hud_tag = hud_tags[index];
        if (hud_tag == (datum_index)-1) {
            continue;
        }
        hud = (UnitHUDInterface *)tag_instances[hud_tag & 0xffff].data;

        if (*(datum_index *)&hud->hud_background_interface_bitmap.tag_id != (datum_index)-1) {
            flags = (uint32_t)((*(uint8_t *)(object + 0x106) >> 1) & 2);
            if (local_player_globals->local_player_count > 1) {
                flags |= 4;
            }
            hud_draw_static_element(local_player_index, (uint16_t *)hud,
                                    (const hud_static_element_placement *)&hud->hud_background_anchor_offset, flags, -1);
        }

        if ((current_game_engine == 0 || player_index == (datum_index)-1 ||
             ((game_engine_variant.flags >> 3) & 1) == 0) &&
            (hud_unit_meters->flags & 4) == 0) {
            float shield = *(float *)(object + 0xe4);
            flags = (shield < 0.25f || (hud_unit_meters->flags & 8) != 0) ? 1 : 0;
            if ((*(uint8_t *)(object + 0x106) & 4) != 0) {
                flags |= 2;
            }
            if (local_player_globals->local_player_count > 1) {
                flags |= 4;
            }
            if (index == 0) {
                if ((flags & 1) != 0) {
                    if (state->shield_flash_start_time == -1) {
                        state->shield_flash_start_time = game_time->game_time;
                    }
                } else {
                    state->shield_flash_start_time = -1;
                }
            }
            if (*(datum_index *)&hud->shield_panel_meter_meter_bitmap.tag_id != (datum_index)-1) {
                static const uint32_t layer_colors[5] = {0x000000, 0xff0000, 0x00ff00, 0xffff00, 0x7f00ff};
                hud_meter_placement layer_placement;
                float value = index == 0 ? state->displayed_shield : shield;
                int32_t value_scale = (uint16_t)hud->shield_panel_meter_value_scale;
                int32_t layer;

                if (value_scale == 0) {
                    value_scale = 0xff;
                }
                memcpy(&layer_placement, &hud->shield_panel_meter_anchor_offset, 0x68);
                for (layer = 0; layer <= hud_overshield_layer_count; layer++) {
                    float actual = hud_fraction_clamp(*(float *)(object + 0xe4) - (float)layer);
                    float displayed = hud_fraction_clamp(value - (float)layer);
                    float shown;
                    uint8_t dropping;
                    float scale;
                    int32_t alpha_shown;
                    int32_t alpha_actual;

                    if (displayed > actual) {
                        dropping = 1;
                        shown = displayed;
                    } else {
                        dropping = 0;
                        shown = actual;
                    }
                    if (!(actual > 0.0f) && !(shown > 0.0f)) {
                        break;
                    }
                    *(uint32_t *)&layer_placement.color_at_meter_minimum = layer_colors[layer];
                    *(uint32_t *)&layer_placement.color_at_meter_maximum = layer_colors[layer];
                    scale = (float)(int16_t)value_scale;
                    alpha_shown = hud_alpha_round(scale * shown);
                    alpha_actual = hud_alpha_round(scale * actual);
                    hud_meter_draw_fill(hud, (uint8_t)alpha_actual, (uint8_t)alpha_shown, flags,
                                        dropping ? state->shield_drain_time : -1.0f, actual,
                                        layer == 0 ? (const hud_meter_placement *)&hud->shield_panel_meter_anchor_offset
                                                   : &layer_placement);
                }
            }
            if (*(datum_index *)&hud->shield_panel_background_interface_bitmap.tag_id != (datum_index)-1) {
                hud_draw_static_element(local_player_index, (uint16_t *)hud,
                                        (const hud_static_element_placement *)&hud->shield_panel_background_anchor_offset,
                                        flags, state->shield_flash_start_time);
            }
        }

        if ((hud_unit_meters->flags & 1) == 0) {
            uint16_t object_flags = *(uint16_t *)(object + 0x106);

            flags = ((object_flags & 8) != 0 || (hud_unit_meters->flags & 2) != 0) ? 1 : 0;
            if ((object_flags & 4) != 0) {
                flags |= 2;
            }
            if (local_player_globals->local_player_count > 1) {
                flags |= 4;
            }
            if (index == 0) {
                if ((flags & 1) != 0) {
                    if (state->health_flash_start_time == -1) {
                        state->health_flash_start_time = game_time->game_time;
                    }
                } else {
                    state->health_flash_start_time = -1;
                }
            }
            if (*(datum_index *)&hud->health_panel_meter_meter_bitmap.tag_id != (datum_index)-1) {
                hud_meter_placement health_placement;
                float health = *(float *)(object + 0xe0);
                int32_t value_scale = (uint16_t)hud->health_panel_meter_value_scale;
                int32_t alpha_a;
                int32_t alpha_b;

                if (value_scale == 0) {
                    value_scale = 8;
                }
                memcpy(&health_placement, &hud->health_panel_meter_anchor_offset, 0x68);
                if (!(health < hud->health_panel_meter_max_color_health_fraction_cutoff)) {
                    health_placement.color_at_meter_minimum = health_placement.color_at_meter_maximum;
                } else if (health > hud->health_panel_meter_min_color_health_fraction_cutoff) {
                    health_placement.color_at_meter_maximum = hud->health_panel_meter_medium_health_left_color;
                    health_placement.color_at_meter_minimum = hud->health_panel_meter_medium_health_left_color;
                } else {
                    health_placement.color_at_meter_maximum = health_placement.color_at_meter_minimum;
                }
                alpha_a = hud_alpha_truncate((float)(int16_t)value_scale * health);
                alpha_b = hud_alpha_truncate((float)(int16_t)value_scale * health);
                hud_meter_draw_fill(hud, (uint8_t)alpha_b, (uint8_t)alpha_a, flags, -1.0f, health, &health_placement);
            }
            if (*(datum_index *)&hud->health_panel_background_interface_bitmap.tag_id != (datum_index)-1) {
                hud_draw_static_element(local_player_index, (uint16_t *)hud,
                                        (const hud_static_element_placement *)&hud->health_panel_background_anchor_offset,
                                        flags, state->health_flash_start_time);
            }
            state->displayed_health = *(float *)(object + 0xe0);
        }

        if (index == 0 && (hud_unit_meters->flags & 0x10) == 0 && game_engine_scores_tracked_individually() != 0) {
            uint16_t anchor[0x12];
            Point2DInt center;

            anchor[0] = 2;
            flags = local_player_globals->local_player_count > 1 ? 4 : 0;
            if ((hud_unit_meters->flags & 0x20) != 0) {
                flags |= 1;
            }
            if ((flags & 1) != 0) {
                if (state->motion_sensor_flash_start_time == -1) {
                    state->motion_sensor_flash_start_time = game_time->game_time;
                }
            } else {
                state->motion_sensor_flash_start_time = -1;
            }
            if (*(datum_index *)&hud->motion_sensor_background_interface_bitmap.tag_id != (datum_index)-1) {
                hud_draw_static_element(local_player_index, anchor,
                                        (const hud_static_element_placement *)&hud->motion_sensor_background_anchor_offset,
                                        flags, -1);
            }
            if (*(datum_index *)&hud->motion_sensor_foreground_interface_bitmap.tag_id != (datum_index)-1) {
                hud_draw_static_element(local_player_index, anchor,
                                        (const hud_static_element_placement *)&hud->motion_sensor_foreground_anchor_offset,
                                        flags, -1);
            }
            hud_anchor_offset_to_screen_position(anchor, local_player_globals->local_player_count > 1, 0.0f,
                                                 &hud->motion_sensor_center_anchor_offset.x, &center.x, 0);
            if (local_player_index != -1) {
                motion_sensor_update_for_player(local_player_index);
                motion_sensor_render(local_player_globals->local_player_count > 1, &center.x, local_player_index);
            }
        }

        {
            uint32_t overlay_types = (current_game_engine != 0 && game_engine_variant.teams != 0) ? 1 : 0;
            int16_t overlay_index;

            flags = local_player_globals->local_player_count > 1 ? 4 : 0;
            for (overlay_index = 0; (int32_t)overlay_index < (int32_t)hud->overlays.count; overlay_index++) {
                UnitHUDInterfaceAuxiliaryOverlay *overlay =
                    (UnitHUDInterfaceAuxiliaryOverlay *)hud->overlays.pointer + overlay_index;

                if ((overlay_types & (1u << *(uint8_t *)&overlay->type)) == 0) {
                    continue;
                }
                if ((*(uint8_t *)&overlay->flags & 1) != 0) {
                    *(uint32_t *)&overlay->default_color = color_rgb_float_to_int((const float *)(object + 0x188)) | 0xff000000;
                }
                hud_draw_static_element(local_player_index, (uint16_t *)&hud->auxiliary_overlay_anchor,
                                        (const hud_static_element_placement *)overlay, flags, -1);
            }
        }

        {
            int16_t meter_index;

            for (meter_index = 0; (int32_t)meter_index < (int32_t)hud->meters.count; meter_index++) {
                UnitHUDInterfaceAuxiliaryPanel *panel = (UnitHUDInterfaceAuxiliaryPanel *)hud->meters.pointer + meter_index;
                int16_t type = panel->type;
                uint32_t bit = 1u << type;
                int16_t *timer = &state->auxiliary_meter_timers[type];

                if ((state->auxiliary_meters_shown & bit) != 0 && (enabled_meters & bit) == 0) {
                    *timer = -1;
                }
                if ((enabled_meters & bit) != 0) {
                    datum_index background = *(datum_index *)&panel->background_interface_bitmap.tag_id;
                    datum_index meter = *(datum_index *)&panel->meter_meter_bitmap.tag_id;
                    int32_t period;

                    flags = local_player_globals->local_player_count > 1 ? 4 : 0;
                    if (meter_values[type] <= panel->meter_minimum_fraction_cutoff) {
                        flags |= 1;
                    }
                    *timer = (int16_t)(*timer + game_time->ticks_this_frame);
                    period = (int32_t)lrint((double)(panel->background_flash_period * 30.0f));
                    *timer = (int16_t)(*timer % (period * 2));
                    if (background != (datum_index)-1) {
                        hud_draw_static_element(local_player_index, (uint16_t *)hud,
                                                (const hud_static_element_placement *)&panel->background_anchor_offset,
                                                flags, game_time->game_time - *timer);
                    }
                    if (meter != (datum_index)-1) {
                        float scale = (float)(int32_t)panel->meter_value_scale;
                        int32_t alpha_a = hud_alpha_round(scale * meter_values[type]);
                        int32_t alpha_b = hud_alpha_round(scale * meter_values[type]);
                        hud_meter_draw_fill(hud, (uint8_t)alpha_b, (uint8_t)alpha_a, flags, -1.0f, meter_values[type],
                                            (const hud_meter_placement *)&panel->meter_anchor_offset);
                    }
                } else {
                    if ((blinking_meters & bit) == 0) {
                        if (*timer == -1 ||
                            (int32_t)*timer >= (int32_t)lrint((double)(panel->background_flash_period * 30.0f))) {
                            *timer = -1;
                            continue;
                        }
                    }
                    flags = local_player_globals->local_player_count > 1 ? 4 : 0;
                    *timer = (int16_t)(*timer + game_time->ticks_this_frame);
                    if (*(datum_index *)&panel->background_interface_bitmap.tag_id != (datum_index)-1) {
                        hud_draw_static_element(local_player_index, (uint16_t *)hud,
                                                (const hud_static_element_placement *)&panel->background_anchor_offset,
                                                flags | 1, game_time->game_time - *timer);
                    }
                }
            }
        }
        state->auxiliary_meters_shown = (uint16_t)enabled_meters;

        if (current_game_engine != 0) {
            static const float full_uv[4] = {0.0f, 1.0f, 0.0f, 1.0f};
            static char *icon_paths[5] = {
                (char *)"ui\\shell\\bitmaps\\team_icon_ctf", (char *)"ui\\shell\\bitmaps\\team_icon_slayer",
                (char *)"ui\\shell\\bitmaps\\team_icon_oddball", (char *)"ui\\shell\\bitmaps\\team_icon_king",
                (char *)"ui\\shell\\bitmaps\\team_icon_race"};
            static const int16_t icon_x[5] = {0x1c8, 0x1c9, 0x1c8, 0x1ca, 0x1c5};
            static const int16_t icon_y[5] = {6, 8, 7, 8, 6};
            static const float icon_scale[5] = {0.55f, 0.5f, 0.55f, 0.53f, 0.6f};
            static Point2DInt icon_position;
            static float icon_scales[2];
            uint32_t team_color;
            BitmapData *icon;
            BitmapData *background;
            Point2DInt background_position;
            float background_scale[2];
            float extents[4];

            if (game_engine_variant.teams != 0) {
                team_color = p->team != 0 ? 0xb00201e3 : 0xb0fe0000;
            } else {
                float rgb_buffer[3];
                float *rgb = game_engine_get_player_color(player_index, rgb_buffer);
                team_color = ((uint32_t)lrint((double)(rgb[0] * 255.0f)) & 0xff) << 16 |
                             ((uint32_t)lrint((double)(rgb[1] * 255.0f)) & 0xff) << 8 |
                             ((uint32_t)lrint((double)(rgb[2] * 255.0f)) & 0xff) | 0xff000000;
            }
            if ((uint32_t)(game_engine_variant.game_engine_index - 1) <= 4) {
                int32_t engine = game_engine_variant.game_engine_index - 1;
                hud_team_icon_bitmap = tag_lookup(0x6269746d, icon_paths[engine]);
                icon_position.x = icon_x[engine];
                icon_position.y = icon_y[engine];
                icon_scales[0] = icon_scale[engine];
                icon_scales[1] = icon_scale[engine];
            }
            icon = bitmap_group_sequence_get_bitmap_data(hud_team_icon_bitmap, 0, 0);
            hud_team_background_bitmap = tag_lookup(0x6269746d, (char *)"ui\\shell\\bitmaps\\team_background");
            background = bitmap_group_sequence_get_bitmap_data(hud_team_background_bitmap, 0, 0);
            if (icon != 0 && background != 0) {
                background_position.x = 0x1bd;
                background_position.y = 6;
                background_scale[0] = 0.6f;
                background_scale[1] = 0.64f;
                extents[0] = 0.0f;
                extents[1] = 128.0f;
                extents[2] = 0.0f;
                extents[3] = 64.0f;
                hud_draw_rotated_bitmap_quad(&background_position, background_scale, 0, background, full_uv, extents,
                                             0.0f, 0xffffffff);
                extents[0] = 0.0f;
                extents[1] = 64.0f;
                extents[2] = 0.0f;
                extents[3] = 64.0f;
                hud_draw_rotated_bitmap_quad(&icon_position, icon_scales, 0, icon, full_uv, extents, 0.0f, team_color);
            }
        }
    } while (index != 0);
}

/**
 * Reserves the six HUD runtime-state blocks from the game-state bump allocator, in order, and folds each
 * block's size (not its contents) into the running state checksum. VERIFIED against disassembly
 * 0x4a9780..0x4a98c9 (2026-09-30); fixed: the sixth block (motion_sensor, 0x570) is also checksummed (6
 * crc32_update calls, the draft made 5).
 *
 * @address 0x4a9780
 */
void HudFrame::state_allocate(void)
{
    int32_t size;

    size = sizeof(hud_globals_flags);
    hud_flags = (hud_globals_flags *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    size = sizeof(hud_messaging_globals);
    hud_messaging = (hud_messaging_globals *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    size = sizeof(hud_unit_meter_globals);
    hud_unit_meters = (hud_unit_meter_globals *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    size = sizeof(hud_weapon_interface_state);
    hud_weapon_state = (hud_weapon_interface_state *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    size = sizeof(hud_waypoint_state);
    hud_waypoints = (hud_waypoint_state *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    size = sizeof(motion_sensor_globals);
    motion_sensor = (motion_sensor_globals *)(game_state_cursor + (int32_t)game_state_base);
    game_state_cursor = game_state_cursor + size;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
}

/**
 * Clears and reinitializes the per-round HUD runtime-state buffers to their default values (unit meters snap
 * on the first update via -1.0 displayed values, weapon/waypoint slots start all free), looks up the current
 * hud_globals tag, and resets the motion sensor.
 *
 * @address 0x4a98d0
 */
void HudFrame::state_reset(void)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    int32_t i;

    hud_flags->hud_enabled = 0;
    hud_flags->help_text_shown = 0;
    hud_flags->unknown_02[0] = 0;
    hud_flags->unknown_02[1] = 0;
    hud_flags->hud_enabled = 1;

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    hud_globals_tag_data = (HUDGlobals *)(tag_instances[(*(int32_t *)&interface_bitmaps->hud_globals.tag_id) & 0xffff].data);
    hud_messaging_parameters = hud_globals_tag_data;

    memset(hud_messaging, 0, sizeof(*hud_messaging));
    memset(hud_unit_meters, 0, sizeof(*hud_unit_meters));

    hud_unit_meters->players[0].auxiliary_meter_timers[0] = -1;
    hud_unit_meters->players[0].displayed_health = -1.0f;
    hud_unit_meters->players[0].displayed_shield = -1.0f;
    hud_unit_meters->players[0].shield_drain_time = -1.0f;
    hud_unit_meters->players[0].health_flash_start_time = -1;
    hud_unit_meters->players[0].motion_sensor_flash_start_time = -1;
    hud_unit_meters->players[0].last_unit = (datum_index)-1;
    hud_unit_meters->players[0].sounds_playing = 0;
    for (i = 0; i < 12; i = i + 1) {
        hud_unit_meters->players[0].sound_handles[i] = -1;
    }

    for (i = 0; i < 0x1f; i = i + 1) {
        ((int32_t *)hud_weapon_state)[i] = -1;
    }

    memset(hud_waypoints, 0xff, sizeof(*hud_waypoints));

    motion_sensor_reset();
}

/**
 * Runs the weapon HUD, unit meter and waypoint per-frame updates, resets the messaging sequence counter, and
 * conditionally drives a fourth update when a local-player-count-like gate and this build's one local player
 * slot both look valid.
 *
 * @address 0x4a9990
 */
void HudFrame::update_dispatch(void)
{
    hud_weapon_interface_state_update();
    hud_unit_meters_update();
    hud_waypoints_update();
    hud_messaging->next_sequence = 0;

    if (current_game_engine != 0 && game_engine_state_value > 1 && game_engine_state_value < 4 &&
        local_player_globals->local_players[0] != (datum_index)-1) {
        hud_unit_sounds_update((player *)((uint8_t *)player_data->data +
                                          (local_player_globals->local_players[0] & 0xffff) * 0x200), 0);
    }
}

}
