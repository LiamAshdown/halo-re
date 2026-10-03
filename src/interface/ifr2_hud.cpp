#include "halo/interface/ifr2_hud.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include <string.h>
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/items/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/render/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/bit_cast.hpp"
#include "tags.h"
#include "units.h"
#include "game.h"
#include "halo/interface/flags.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"
#include "halo/interface/constants.hpp"
#include "halo/interface/color_bits.hpp"

#ifdef interface
#undef interface
#endif

static auto &hud_globals_tag_data = halo::link::ref<HUDGlobals *>(halo::ui::vars().hud_globals_tag_data);
static auto &hud_waypoints = halo::link::ref<hud_waypoint_state *>(halo::ui::vars().hud_waypoints);
static auto &custom_waypoints = halo::link::ref<custom_waypoint [k_maximum_custom_waypoints]>(halo::game::vars().custom_waypoints);
static auto &hud_weapon_state = halo::link::ref<hud_weapon_interface_state *>(halo::ui::vars().hud_weapon_state);
static auto &render_viewport_top = halo::link::ref<int16_t>(halo::ui::vars().render_viewport_top);
static auto &current_local_player_index = halo::link::ref<int16_t>(halo::ui::vars().current_local_player_index);
static auto &hud_flags = halo::link::ref<hud_globals_flags *>(halo::ui::vars().hud_flags);
static auto &motion_sensor_override_value = halo::link::ref<uint8_t>(halo::ui::vars().motion_sensor_override_value);
static auto &motion_sensor_force_moving = halo::link::ref<uint8_t>(halo::ui::vars().motion_sensor_force_moving);
static auto &motion_sensor_blip_subtype_size = halo::link::ref<float [3]>(halo::ui::vars().motion_sensor_blip_subtype_size);
static auto &motion_sensor_blip_colors = halo::link::ref<ColorRGB [6]>(halo::ui::vars().motion_sensor_blip_colors);
static auto &motion_sensor = halo::link::ref<motion_sensor_globals *>(halo::ui::vars().motion_sensor);
static auto &motion_sensor_render_local_player = halo::link::ref<int16_t>(halo::ui::vars().motion_sensor_render_local_player);
static auto &motion_sensor_render_icon_scale = halo::link::ref<float>(halo::ui::vars().motion_sensor_render_icon_scale);
static auto &motion_sensor_render_center = halo::link::ref<float [2]>(halo::ui::vars().motion_sensor_render_center);
static auto &motion_sensor_sweep = halo::link::ref<float>(halo::ui::vars().motion_sensor_sweep);

static_assert(offsetof(Weapon, hud_interface.tag_id) == 0x48c && offsetof(Unit, motion_sensor_blip_size) == 0x298, "tag field offsets read by the HUD");

namespace halo::interface {

/**
 * trunc(value * 100) clamped to 0..100 (evaluated up to three times by the binary)
 */
int32_t WeaponHud::hud_percent(float value)
{
    int32_t percent = halo::interface::ui_real_to_int_truncate(value * 100.0f);
    if (percent < 0) {
        return 0;
    }
    if (percent > 100) {
        return 100;
    }
    return percent;
}

/**
 * bits 0..2 as computed, bit 3 when none of them is set, bit 4 always
 */
uint16_t WeaponHud::hud_overlay_type_bits(uint16_t bits)
{
    if (bits == 0) {
        bits = 8;
    } else {
        bits &= ~8u;
    }
    return (uint16_t)(bits | 0x10);
}

object * WeaponHud::object_get(datum_index object_index)
{
    return halo::interface::object_record<object>(object_index);
}

/**
 * blam-cc: local_player_index -> AX, eye -> ECX, target -> EDX
 *
 * @address 0x4af540
 */
int16_t HudWaypoints::visibility(const real_point3d *eye, const real_point3d *target, datum_index ignore_object)
{
    collision_result result;
    real_vector3d delta;
    datum_index unit_index = (datum_index)-1;

    if (local_player_index != -1 && local_player_index < 1 &&
        halo::game::globals().local_player_globals->local_players[local_player_index] != (datum_index)-1) {
        unit_index = (halo::interface::player_record(halo::game::globals().local_player_globals->local_players[local_player_index]))->unit;
    }
    delta.i = target->x - eye->x;
    delta.j = target->y - eye->y;
    delta.k = target->z - eye->z;
    if (halo::physics::collision_test_movement_segment(halo::interface::k_hud_sight_line_collision_mask, (real_point3d *)eye, &delta, unit_index, &result) != 0) {
        if (result.type != 3 || result.object_index != ignore_object) {
            return 2;
        }
    }
    return 0;
}

/**
 * @address 0x4afb90
 */
void HudWaypoints::draw_for_player()
{
    hud_waypoint *waypoints;
    datum_index player_index;
    int16_t i;

    if (local_player_index == -1 || local_player_index >= 1) {
        halo::game::game_engine_update_custom_waypoint_navpoints(local_player_index);
        return;
    }
    player_index = halo::game::globals().local_player_globals->local_players[local_player_index];
    if (player_index == (datum_index)-1 ||
        (halo::interface::player_record(player_index))->unit == (datum_index)-1 ||
        halo::interface::tag_handle(hud_globals_tag_data->arrow_bitmap.tag_id) == (datum_index)-1) {
        halo::game::game_engine_update_custom_waypoint_navpoints(local_player_index);
        return;
    }

    waypoints = hud_waypoints[local_player_index].waypoints;
    for (i = 0; i < 4; i++) {
        hud_waypoint *waypoint = &waypoints[i];
        uint16_t type_word;
        real_point3d position;
        float radius;

        if (waypoint->arrow_index == -1 || waypoint->object_index == (datum_index)-1 ||
            (waypoint->type & 0xf) == 0xf) {
            waypoint->type |= 0xf;
            continue;
        }
        type_word = (uint16_t)waypoint->type;
        switch ((int16_t)(type_word << 12) >> 12) {
        case 0:
            position = *(real_point3d *)&((ScenarioCutsceneFlag *)halo::scenario::globals().scenario->cutscene_flags.pointer +
                                          waypoint->object_index)->position;
            break;
        case 1:
            if (halo::objects::object_try_and_get(waypoint->object_index, halo::k_dword_none) == 0) {
                continue;
            }
            halo::objects::object_get_center_of_mass_and_scale(&position, waypoint->object_index, &radius);
            break;
        default:
            halo::game::custom_waypoint_get_position(&position, (int16_t)waypoint->object_index);
            break;
        }
        position.z = position.z + waypoint->vertical_offset;
        halo::interface::hud_waypoint_draw(&position, local_player_index, waypoint->arrow_index,
                          (int16_t)((int16_t)(type_word << 8) >> 12), 1);
    }
    halo::game::game_engine_update_custom_waypoint_navpoints(local_player_index);
}

/**
 * @address 0x4af320
 */
void HudWaypoints::update()
{
    int16_t local_player_index = halo::game::globals().local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1;

    while (local_player_index != -1) {
        halo::interface::hud_waypoints_update_for_player(local_player_index);
        local_player_index = (halo::game::globals().local_player_globals->local_players[0] != (datum_index)-1 && local_player_index < 0)
                                 ? 0 : -1;
    }
}

/**
 * @address 0x4af370
 */
void HudWaypoints::update_for_player()
{
    static char head_marker[] = "head";
    hud_waypoint *waypoint;
    datum_index unit_index;
    datum_index ignore_object;
    object_marker marker;
    real_point3d eye;
    real_point3d target;
    float radius;
    int32_t i;

    waypoint = hud_waypoints[local_player_index].waypoints;
    unit_index = (datum_index)-1;
    if (local_player_index != -1 && local_player_index < 1 &&
        halo::game::globals().local_player_globals->local_players[local_player_index] != (datum_index)-1) {
        unit_index = (halo::interface::player_record(halo::game::globals().local_player_globals->local_players[local_player_index]))->unit;
    }

    ignore_object = (datum_index)-1;
    for (i = 4; i != 0; i--, waypoint++) {
        uint16_t type_word;
        int16_t visibility;

        if (waypoint->arrow_index == -1 || waypoint->object_index == (datum_index)-1 ||
            (waypoint->type & 0xf) == 0xf) {
            waypoint->type |= 0xf;
            ignore_object = (datum_index)-1;
            continue;
        }
        if (unit_index == (datum_index)-1) {
            ignore_object = (datum_index)-1;
            continue;
        }
        halo::objects::object_get_node_local_transform(unit_index, head_marker, &marker, 1);
        eye = marker.node_transform.position;

        type_word = (uint16_t)waypoint->type;
        switch ((int16_t)(type_word << 12) >> 12) {
        case 0:
            target = *(real_point3d *)&((ScenarioCutsceneFlag *)halo::scenario::globals().scenario->cutscene_flags.pointer +
                                        waypoint->object_index)->position;
            break;
        case 1: {
            object *target_object;

            ignore_object = waypoint->object_index;
            target_object = halo::objects::object_try_and_get(ignore_object, halo::k_dword_none);
            if (target_object == 0 || halo::interface::has_bit(target_object->vitality_flags, halo::objects::vitality_flag::health_frozen)) {
                waypoint->type = (int16_t)(type_word | 0xf);
                waypoint->object_index = (datum_index)-1;
                waypoint->arrow_index = -1;
                ignore_object = (datum_index)-1;
                continue;
            }
            halo::objects::object_get_center_of_mass_and_scale(&target, ignore_object, &radius);
            break;
        }
        case 2:
            target = custom_waypoints[(int16_t)waypoint->object_index].position;
            break;
        }
        target.z = target.z + waypoint->vertical_offset;
        visibility = halo::interface::hud_waypoint_visibility(local_player_index, &eye, &target, ignore_object);
        waypoint->type = (int16_t)(waypoint->type ^ (((visibility << 4) ^ (uint8_t)waypoint->type) & 0xf0));
        ignore_object = (datum_index)-1;
    }
}

/**
 * blam-cc: EAX -> hud_tag, ECX -> p, stack -> ammo
 *
 * @address 0x4b2cf0
 */
void WeaponHud::crosshairs_draw(datum_index hud_tag, const player *p, const weapon_hud_ammo_state *ammo)
{
    int32_t *crosshair_state;
    uint32_t view_mask;
    uint32_t active_mask;
    uint8_t *unit;
    WeaponHUDInterface *chain[16];
    int16_t chain_count;
    int16_t chain_index;
    uint8_t triggered = 0;
    uint32_t color = 0;

    if ((hud_weapon_state->flags & 1) == 0 || hud_tag == (datum_index)-1) {
        return;
    }
    crosshair_state = hud_weapon_state->meters[p->local_player_index].values;
    view_mask = (halo::scenario::globals().scenario->type != scenariotype_user_interface ? 1 : 0) |
                (halo::game::globals().local_player_globals->local_player_count == 1 ? 2 : 0) | (halo::game::globals().local_player_globals->local_player_count > 1 ? 4 : 0);
    if (p->unit == (datum_index)-1) {
        return;
    }
    unit = halo::interface::object_record(p->unit);

    memset(chain, 0, sizeof(chain));
    chain[0] = halo::interface::tag_data<WeaponHUDInterface>(hud_tag);
    active_mask = (uint32_t)crosshair_state[0x13];
    chain_count = 1;
    do {
        datum_index child = *(datum_index *)&chain[chain_count - 1]->child_hud.tag_id;
        if (child == (datum_index)-1) {
            break;
        }
        chain[chain_count] = halo::interface::tag_data<WeaponHUDInterface>(child);
        chain_count++;
    } while (chain_count < 0x10);

    for (chain_index = 0; chain_index < chain_count; chain_index++) {
        WeaponHUDInterface *hud = chain[chain_index];
        uint16_t anchor[0x12];
        uint8_t split_screen = halo::game::globals().local_player_globals->local_player_count > 1;
        int16_t crosshair_index;

        memset(anchor, 0, sizeof(anchor));
        anchor[0] = 4;
        for (crosshair_index = 0; (int32_t)crosshair_index < (int32_t)hud->crosshairs.count; crosshair_index++) {
            WeaponHUDInterfaceCrosshair *crosshair = (WeaponHUDInterfaceCrosshair *)hud->crosshairs.pointer + crosshair_index;
            int16_t type = crosshair->crosshair_type;
            int32_t *state;
            int16_t overlay_index;

            if ((active_mask & (1u << type)) == 0 ||
                ((int32_t)(int16_t)view_mask & (1 << static_cast<uint8_t>(crosshair->allowed_view_type))) == 0) {
                continue;
            }
            state = &crosshair_state[type];
            for (overlay_index = 0; (int32_t)overlay_index < (int32_t)crosshair->crosshair_overlays.count; overlay_index++) {
                WeaponHUDInterfaceCrosshairOverlay *overlay =
                    (WeaponHUDInterfaceCrosshairOverlay *)crosshair->crosshair_overlays.pointer + overlay_index;
                uint32_t flags = halo::bit_cast<uint32_t>(overlay->flags);
                Bitmap *bitmap_tag;
                BitmapGroupSequence *sequence;
                BitmapData *bitmap;
                float scale;
                int16_t frame;
                const float *uv;
                float stretched_uv[4];
                uint8_t pixel_uvs;

                if ((int8_t)flags < 0) {
                    continue;
                }
                if ((flags & 4) != 0 && crosshair_state[1] <= 0) {
                    continue;
                }
                if ((flags & 0x40) != 0 && crosshair_state[1] != 0) {
                    continue;
                }
                scale = 1.0f;
                if (halo::game::globals().local_player_globals->local_player_count > 1 && !halo::interface::has_bit(overlay->scaling_flags, halo::tags::hud_interface_scaling_tag_flag::don_t_scale_size)) {
                    scale = 0.5f;
                }
                sequence = 0;
                if ((flags & 2) == 0) {
                    Bitmap *tag = halo::interface::tag_data<Bitmap>(halo::interface::tag_handle(crosshair->crosshair_bitmap.tag_id));
                    sequence = (BitmapGroupSequence *)tag->bitmap_group_sequence.pointer + (int16_t)overlay->sequence_index;
                }

                frame = 0;
                switch ((uint32_t)(int32_t)type > 0x12 ? -1 : type) {
                case 0:
                    if ((flags & 1) != 0) {
                        frame = 0;
                        color = *state > 0 ? halo::interface::hud_meter_flash_color_blend((const hud_flash_parameters *)&overlay->default_color, 0)
                                           : halo::interface::color_bits(overlay->default_color);
                    } else {
                        frame = (int16_t)*state;
                        color = halo::interface::color_bits(overlay->default_color);
                    }
                    break;
                case 1:
                    if ((flags & 0x20) != 0) {
                        if (*state == 0) {
                            continue;
                        }
                        frame = 0;
                    } else {
                        frame = (int16_t)((int16_t)*state - ((flags >> 2) & 1));
                    }
                    if ((flags & 1) != 0 && crosshair_state[0] > 0) {
                        color = halo::interface::hud_meter_flash_color_blend((const hud_flash_parameters *)&overlay->default_color, 0);
                    } else {
                        color = halo::interface::color_bits(overlay->default_color);
                    }
                    break;
                case 8: case 9: case 14: case 18:
                    if (type == 18) {
                        triggered = ammo->age == 0.0f && (((unit_object *)unit)->unit.control_flags & _unit_control_flag_primary_trigger) != 0;
                    } else if (type == 8) {
                        triggered = ammo->magazines[0].rounds_loaded == 0 && ammo->magazines[0].rounds_unloaded == 0 &&
                                    (((unit_object *)unit)->unit.control_flags & _unit_control_flag_primary_trigger) != 0;
                    } else if (type == 9) {
                        triggered = ((unit_object *)unit)->unit.grenade_counts[0] == 0 && ((unit_object *)unit)->unit.grenade_counts[1] == 0 && ((unit_object *)unit)->unit.throwing_grenade_state == 0 &&
                                    (((unit_object *)unit)->unit.control_flags & _unit_control_flag_grenade) != 0;
                    }
                    if (!triggered) {
                        int32_t duration = (int32_t)halo::libm::lrint((double)(overlay->flash_period * 30.0f));
                        if (halo::game::globals().game_time->game_time - *state >= duration) {
                            *state = -1;
                            continue;
                        }
                    }
                    if (*state == -1) {
                        *state = -1;
                        continue;
                    }

                default:
                    if ((uint32_t)(int32_t)type > 0x12) {
                        break;
                    }
                    if (overlay->frame_rate > 0) {
                        frame = (int16_t)((((halo::game::globals().game_time->game_time - *state) / overlay->frame_rate) / 30) %
                                          (int32_t)sequence->sprites.count);
                    } else {
                        frame = 0;
                    }
                    if (halo::interface::has_bit(overlay->flags, halo::tags::weapon_hud_interface_crosshair_overlay_tag_flag::flashes_when_active) && *state != -1) {
                        color = halo::interface::hud_meter_flash_color_blend((const hud_flash_parameters *)&overlay->default_color, *state);
                    } else {
                        color = halo::interface::color_bits(overlay->default_color);
                    }
                    break;
                }

                bitmap_tag = halo::interface::tag_data<Bitmap>(halo::interface::tag_handle(crosshair->crosshair_bitmap.tag_id));
                {
                    int32_t bitmap_index = sequence != 0
                        ? (int16_t)((BitmapGroupSprite *)sequence->sprites.pointer)[frame].bitmap_index
                        : (int16_t)overlay->sequence_index;
                    bitmap = (BitmapData *)bitmap_tag->bitmap_data.pointer + bitmap_index;
                }
                if (halo::cache::texture_cache_get(bitmap, 0, 1) == 0) {
                    continue;
                }
                pixel_uvs = bitmap_tag->type == 4;
                if ((flags & 0x10) != 0) {
                    float u_factor = 1.0f;
                    float v_factor = 1.0f;
                    float dx;
                    float dy;

                    if (sequence != 0) {
                        BitmapGroupSprite *sprite = (BitmapGroupSprite *)sequence->sprites.pointer + frame;
                        stretched_uv[0] = sprite->left;
                        stretched_uv[1] = sprite->right;
                        stretched_uv[2] = sprite->top;
                        stretched_uv[3] = sprite->bottom;
                    } else {
                        stretched_uv[0] = 0.0f;
                        stretched_uv[1] = (float)(pixel_uvs ? (int32_t)(int16_t)bitmap->width : 1);
                        stretched_uv[2] = 0.0f;
                        stretched_uv[3] = (float)(pixel_uvs ? (int32_t)(int16_t)bitmap->height : 1);
                        u_factor = pixel_uvs ? 1.0f : (float)((double)(1.0f / (float)(int16_t)bitmap->width) * 1.25);
                        v_factor = pixel_uvs ? 1.0f : (float)((double)(1.0f / (float)(int16_t)bitmap->height) * 1.25);
                    }
                    dx = ((float)(int16_t)bitmap->width - (float)(halo::render::globals().viewport_right - halo::render::globals().viewport_left) * (1.0f / scale)) *
                         u_factor * -0.5f;
                    dy = ((float)(int16_t)bitmap->height - (float)(halo::render::globals().viewport_bottom - render_viewport_top) * (1.0f / scale)) *
                         v_factor * -0.5f;
                    stretched_uv[0] = stretched_uv[0] - dx;
                    stretched_uv[1] = dx + stretched_uv[1];
                    stretched_uv[2] = stretched_uv[2] - dy;
                    stretched_uv[3] = dy + stretched_uv[3];
                    uv = stretched_uv;
                } else {
                    uv = sequence != 0 ? &((BitmapGroupSprite *)sequence->sprites.pointer)[frame].left : nullptr;
                }
                halo::interface::hud_draw_bitmap_element(uv, (const hud_element_placement *)overlay, pixel_uvs, 0, bitmap, anchor, scale, 0.0f,
                                        color, split_screen);
            }
        }
    }
}

/**
 * @address 0x4b1ff0
 */
void WeaponHud::draw_elements(datum_index hud_tag, int16_t local_player_index, const Weapon *weapon_tag, const weapon_hud_ammo_state *ammo, const uint16_t *parent_state_flags, const uint16_t *parent_overlay_types, const int16_t *parent_numbers)
{
    WeaponHUDInterface *hud = halo::interface::tag_data<WeaponHUDInterface>(hud_tag);
    int32_t *flash_start_times = hud_weapon_state->players[local_player_index].flash_start_times;
    uint16_t state_flags[8];
    uint16_t overlay_types[8];
    int16_t numbers[8];
    float values[8];
    uint16_t split = halo::game::globals().local_player_globals->local_player_count > 1 ? 4 : 0;
    uint32_t view_mask;
    int16_t i;

    memset(state_flags, 0, sizeof(state_flags));
    memset(overlay_types, 0, sizeof(overlay_types));
    memset(numbers, 0, sizeof(numbers));
    memset(values, 0, sizeof(values));

    if (halo::interface::has_bit(hud->flags, halo::tags::weapon_hud_interface_tag_flag::use_parent_hud_flashing_parameters) && parent_state_flags != 0 && parent_overlay_types != 0 &&
        parent_numbers != 0) {
        memcpy(state_flags, parent_state_flags, sizeof(state_flags));
        memcpy(overlay_types, parent_overlay_types, sizeof(overlay_types));
        memcpy(numbers, parent_numbers, sizeof(numbers));
    } else {
        const weapon_hud_magazine_state *primary = &ammo->magazines[0];
        const weapon_hud_magazine_state *secondary = &ammo->magazines[1];
        int16_t loaded;
        int16_t total;

        state_flags[0] = (uint16_t)((primary->rounds_unloaded <= hud->total_ammo_cutoff ? 1 : 0) |
                                    (primary->rounds_unloaded == 0 ? 2 : 0) | split);
        state_flags[1] = (uint16_t)((primary->rounds_loaded <= hud->loaded_ammo_cutoff && primary->reloading == 0 ? 1 : 0) |
                                    split);
        state_flags[2] = (uint16_t)(((float)hud->heat_cutoff <= ammo->heat * 100.0f ? 1 : 0) | split);
        state_flags[3] = (uint16_t)((!((float)hud->age_cutoff < (1.0f - ammo->age) * 100.0f) ? 1 : 0) |
                                    (100 - hud_percent(ammo->age) == 0 ? 2 : 0) | split);
        state_flags[4] = (uint16_t)((secondary->rounds_unloaded <= hud->total_ammo_cutoff ? 1 : 0) |
                                    (secondary->rounds_unloaded == 0 ? 2 : 0) | split);
        state_flags[5] = (uint16_t)((secondary->rounds_loaded <= hud->loaded_ammo_cutoff && secondary->reloading == 0 ? 1 : 0) |
                                    split);
        for (i = 0; i < 8; i++) {
            if ((state_flags[i] & 1) != 0) {
                if (flash_start_times[i] == -1) {
                    flash_start_times[i] = halo::game::globals().game_time->game_time;
                }
            } else {
                flash_start_times[i] = -1;
            }
        }

        total = primary->rounds_unloaded;
        overlay_types[0] = hud_overlay_type_bits((uint16_t)(
            (total <= hud->total_ammo_cutoff && primary->reloading == 0 ? 1 : 0) | (primary->reloading != 0 ? 4 : 0) |
            (total == 0 ? 2 : 0)));
        loaded = primary->rounds_loaded;
        overlay_types[1] = hud_overlay_type_bits((uint16_t)(
            (loaded <= hud->loaded_ammo_cutoff ? 1 : 0) | (primary->reloading != 0 ? 4 : 0) | (loaded == 0 ? 2 : 0)));
        overlay_types[2] = hud_overlay_type_bits((uint16_t)(
            ((float)hud->heat_cutoff <= ammo->heat * 100.0f ? 1 : 0) | (ammo->overheated != 0 ? 4 : 0) |
            (100 - hud_percent(ammo->age) == 0 ? 2 : 0)));
        overlay_types[3] = hud_overlay_type_bits((uint16_t)(
            (!((float)hud->age_cutoff < (1.0f - ammo->age) * 100.0f) ? 1 : 0) | (ammo->overheated != 0 ? 4 : 0) |
            (100 - hud_percent(ammo->age) == 0 ? 2 : 0)));

        overlay_types[0] = hud_overlay_type_bits((uint16_t)(
            (secondary->rounds_unloaded <= hud->total_ammo_cutoff && secondary->reloading == 0 ? 1 : 0) |
            (secondary->reloading != 0 ? 4 : 0) | (secondary->rounds_unloaded == 0 ? 2 : 0)));
        overlay_types[1] = hud_overlay_type_bits((uint16_t)(
            (secondary->rounds_loaded <= hud->loaded_ammo_cutoff ? 1 : 0) | (secondary->reloading != 0 ? 4 : 0) |
            (secondary->rounds_loaded == 0 ? 2 : 0)));

        numbers[0] = primary->rounds_unloaded;
        numbers[1] = primary->rounds_loaded;
        numbers[2] = (int16_t)halo::x87::__ftol((double)(ammo->heat * 255.0f));
        numbers[3] = (int16_t)halo::x87::__ftol((double)((1.0f - ammo->age) * 100.0f));
        numbers[4] = secondary->rounds_unloaded;
        numbers[5] = secondary->rounds_loaded;

        {
            local_player_control *control = &halo::game::globals().player_control->local_players[local_player_index];
            datum_index target = control->nameplate_target;
            datum_index valid_target = (datum_index)-1;

            if (halo::objects::object_try_and_get(target, _object_mask_all) != 0) {
                valid_target = target;
            }
            if (control->nameplate_weight == 1.0f && valid_target != (datum_index)-1) {
                datum_index unit_index = (datum_index)-1;
                real_point3d camera;
                real_point3d position;

                if (local_player_index != -1 && local_player_index < 1 &&
                    halo::game::globals().local_player_globals->local_players[local_player_index] != (datum_index)-1) {
                    unit_index = (halo::interface::player_record(halo::game::globals().local_player_globals->local_players[local_player_index]))->unit;
                }
                halo::units::unit_get_camera_position(unit_index, &camera);
                halo::objects::object_get_position(&position, valid_target);
                {
                    float dx = camera.x - position.x;
                    float dy = camera.y - position.y;
                    float dz = camera.z - position.z;
                    values[6] = halo::libm::sqrtf(dz * dz + dy * dy + dx * dx) * 3.048f;
                }
                values[7] = (position.z - camera.z) * 3.048f;
            } else {
                values[6] = halo::bit_cast<float>(halo::interface::k_float_indefinite_bits);
                values[7] = halo::bit_cast<float>(halo::interface::k_float_indefinite_bits);
            }
        }
    }

    if (halo::interface::tag_handle(hud->child_hud.tag_id) != (datum_index)-1) {
        halo::interface::hud_weapon_interface_draw_elements(halo::interface::tag_handle(hud->child_hud.tag_id), local_player_index, weapon_tag, ammo,
                                           state_flags, overlay_types, numbers);
    }

    view_mask = (halo::scenario::globals().scenario->type != scenariotype_user_interface ? 1 : 0) |
                (halo::game::globals().local_player_globals->local_player_count == 1 ? 2 : 0) | (halo::game::globals().local_player_globals->local_player_count > 1 ? 4 : 0);

    for (i = 0; (int32_t)i < (int32_t)hud->static_elements.count; i++) {
        WeaponHUDInterfaceStaticElement *element = (WeaponHUDInterfaceStaticElement *)hud->static_elements.pointer + i;
        int16_t state = element->state_attached_to;

        if (halo::bit_cast<int32_t>(element->flash_period) == halo::interface::k_weapon_hud_flash_unset_bits) {
            element->flash_period = 1.0f;
        }
        if (halo::bit_cast<int32_t>(element->flash_length) == halo::interface::k_weapon_hud_flash_unset_bits) {
            element->flash_length = 1.0f;
        }
        if ((element->_pad_2[0] & 1) != 0 || (view_mask & (1u << static_cast<uint8_t>(element->allowed_view_type))) == 0) {
            continue;
        }
        halo::interface::hud_draw_static_element(local_player_index, &hud->anchor,
                                (const hud_static_element_placement *)&element->anchor_offset, state_flags[state],
                                flash_start_times[state]);
    }

    for (i = 0; (int32_t)i < (int32_t)hud->meter_elements.count; i++) {
        WeaponHUDInterfaceMeter *element = (WeaponHUDInterfaceMeter *)hud->meter_elements.pointer + i;
        int16_t state = element->state_attached_to;
        uint8_t value;

        if ((element->_pad_2[0] & 1) != 0 || (view_mask & (1u << static_cast<uint8_t>(element->allowed_view_type))) == 0) {
            continue;
        }
        value = (uint8_t)numbers[state];
        halo::interface::hud_meter_draw_fill(&hud->anchor, value, value, (uint32_t)(int16_t)state_flags[state],
                            (float)flash_start_times[state], 0.0f,
                            (const hud_meter_placement *)&element->anchor_offset);
    }

    for (i = 0; (int32_t)i < (int32_t)hud->number_elements.count; i++) {
        WeaponHUDInterfaceNumber *element = (WeaponHUDInterfaceNumber *)hud->number_elements.pointer + i;
        int16_t state = element->state_attached_to;
        int16_t divisor;
        int16_t value;
        int16_t fraction;

        if ((element->_pad_2[0] & 1) != 0 || (view_mask & (1u << static_cast<uint8_t>(element->allowed_view_type))) == 0) {
            continue;
        }
        divisor = 1;
        if (halo::interface::has_bit(element->weapon_specific_flags, halo::tags::weapon_hud_interface_number_weapon_specific_tag_flag::divide_number_by_clip_size)) {
            divisor = ((WeaponMagazine *)weapon_tag->magazines.pointer)->rounds_loaded_maximum;
        }
        if (element->number_of_fractional_digits != 0) {
            float power;
            float scaled;

            if (halo::bit_cast<uint32_t>(values[state]) == halo::interface::k_float_indefinite_bits) {
                continue;
            }
            power = (float)halo::libm::pow(10.0, 4.0);
            scaled = power * values[state];
            fraction = (int16_t)halo::libm::lrint(halo::libm::fmod((double)(scaled < 0.0f ? -scaled : scaled), (double)power));
            value = (int16_t)halo::interface::ui_real_to_int_truncate(values[state] / (float)divisor);
        } else {
            fraction = -1;
            value = (int16_t)(numbers[state] / divisor);
        }
        halo::interface::hud_draw_number((void *)(int32_t)local_player_index, &hud->anchor,
                        (const hud_number_placement *)&element->anchor_offset, value, fraction, state_flags[state],
                        flash_start_times[state], 0.0f);
    }

    for (i = 0; (int32_t)i < (int32_t)hud->overlay_elements.count; i++) {
        WeaponHUDInterfaceOverlayElement *element = (WeaponHUDInterfaceOverlayElement *)hud->overlay_elements.pointer + i;
        int16_t state = element->state_attached_to;

        if ((element->_pad_2[0] & 1) != 0 || (view_mask & (1u << static_cast<uint8_t>(element->allowed_view_type))) == 0) {
            continue;
        }
        halo::interface::hud_draw_overlays(&hud->anchor, (const hud_overlay_list *)&element->overlay_bitmap,
                          (uint32_t)(int16_t)overlay_types[state], flash_start_times[state], state_flags[state],
                          halo::game::globals().local_player_globals->local_player_count > 1);
    }
}

/**
 * blam-cc: EAX -> hud_interface_tag_id, stack -> local_player_index, weapon_or_vehicle_index, state_ptr
 *
 * @address 0x4b1970
 */
void WeaponHud::meters_evaluate(datum_index hud_interface_tag_id, int16_t local_player_index, int32_t weapon_or_vehicle_index, const weapon_hud_ammo_state *state)
{
    player *player_record;
    WeaponHUDInterface *tag_data;
    WeaponHUDInterface *chain[17];
    uint32_t present_mask;
    int16_t gather_index;
    int32_t *out_array;
    datum_index player_index;
    unit_data *unit;

    if (local_player_index == -1 || local_player_index > 0) {
        player_index = (datum_index)-1;
    } else {
        player_index = halo::game::globals().local_player_globals->local_players[local_player_index];
    }
    player_record = halo::interface::player_record(player_index);
    chain[0] = nullptr;

    if (halo::objects::object_try_and_get(player_record->unit, 3) == 0) {
        return;
    }
    {
        datum_index unit_idx = player_record->unit;
        object *unit_object = ((object_header *)halo::objects::globals().object_data->data)[unit_idx & halo::k_slot_mask].data;
        unit = &((struct unit_object *)unit_object)->unit;
    }

    out_array = hud_weapon_state->meters[player_record->local_player_index].values;
    tag_data = halo::interface::tag_data<WeaponHUDInterface>(hud_interface_tag_id);
    chain[1] = tag_data;
    {
        int i;
        for (i = 2; i < 17; i++) chain[i] = 0;
    }
    present_mask = tag_data->crosshair_types;

    if (weapon_or_vehicle_index != (int32_t)hud_weapon_state->players[local_player_index].weapon
        && weapon_or_vehicle_index == -1) {
        int i;
        for (i = 0; i < 0x14; i++) out_array[i] = 0;
    }

    gather_index = 1;
    do {
        int32_t next = halo::tag_id_bits<int32_t>(chain[gather_index]->child_hud.tag_id);
        WeaponHUDInterface *resolved;
        if (next == -1) break;
        resolved = halo::interface::tag_data<WeaponHUDInterface>(next);
        present_mask |= resolved->crosshair_types;
        gather_index++;
        chain[gather_index] = resolved;
    } while (gather_index < 0x10);

    {
        int16_t case_index;
        uint32_t bit;
        int32_t result_accum = 0;
        const WeaponHUDInterface *edx = tag_data;
        
        for (case_index = 0; case_index < 0x13; case_index++) {
            int32_t value;
            uint8_t active;

            bit = 1u << case_index;
            if ((bit & present_mask) == 0) {
                continue;
            }

            switch (case_index) {
            case 0:
                active = weapon_or_vehicle_index != -1 &&
                         halo::game::globals().player_control->local_players[player_record->local_player_index].nameplate_weight == 1.0f;
                value = active;
                break;
            case 16: value = state->magazines[0].idle; active = value != 0 ? 1 : 0; break;
            case 7:  value = state->magazines[0].reloading; active = value != 0 ? 1 : 0; break;
            case 17: value = state->magazines[1].idle; active = value != 0 ? 1 : 0; break;

            case 1: {
                int16_t lp = player_record->local_player_index;
                int32_t r1 = halo::game::local_player_get_zoom_level(lp);
                if ((int16_t)r1 == -1) {
                    edx = tag_data;
                    active = 1; value = 1;
                } else {
                    int32_t r2 = halo::game::local_player_get_zoom_level(lp);
                    edx = tag_data;
                    value = (int16_t)(r2 + 2);
                    active = (int16_t)value > 0;
                }
                break;
            }

            case 2:
                active = 0;
                value = 0;
                break;

            case 3:
                if (state->magazines[0].rounds_unloaded == 0) {
                    active = 0; value = 0;
                } else if ((int16_t)state->magazines[0].rounds_loaded > edx->loaded_ammo_cutoff) {
                    active = 0; value = 0;
                } else {
                    active = 1; value = 1;
                }
                break;

            case 4: {
                float lhs = state->heat * 100.0f;
                int32_t rhs = edx->heat_cutoff;
                if (!(lhs >= (float)rhs)) { active = 0; value = 0; }
                else { active = 1; value = 1; }
                break;
            }

            case 5:
                if ((int16_t)state->magazines[0].rounds_unloaded > edx->total_ammo_cutoff) { active = 0; value = 0; }
                else if (state->magazines[0].reloading != 0) { active = 0; value = 0; }
                else { active = 1; value = 1; }
                break;

            case 6: {
                float s4 = state->age;
                if (!(s4 < 1.0f)) { active = 0; value = 0; }
                else {
                    float lhs = (1.0f - s4) * 100.0f;
                    int32_t rhs = edx->age_cutoff;

                    if ((float)rhs < lhs) { active = 0; value = 0; }
                    else { active = 1; value = 1; }
                }
                break;
            }

            case 8:
                if (state->magazines[0].rounds_loaded == 0 && state->magazines[0].rounds_unloaded == 0 &&
                    (unit->control_flags & _unit_control_flag_primary_trigger) != 0) {
                    active = 1; value = 1;
                } else if (out_array[case_index] != -1) {
                    active = 1; value = 1;
                } else {
                    active = 0; value = 0;
                }
                break;

            case 9: {

                uint8_t no_grenades = (unit->grenade_counts[0] == 0) && (unit->grenade_counts[1] == 0);
                if (no_grenades && unit->throwing_grenade_state == 0 &&
                    (unit->control_flags & _unit_control_flag_grenade) != 0) {
                    active = 1; value = 1;
                } else if (out_array[case_index] != -1) {
                    active = 1; value = 1;
                } else {
                    active = 0; value = 0;
                }
                break;
            }

            case 10:
                if (state->magazines[0].rounds_unloaded != 0) { active = 0; value = 0; }
                else if (state->magazines[0].rounds_loaded == 0) { active = 0; value = 0; }
                else if ((int16_t)state->magazines[0].rounds_loaded > edx->loaded_ammo_cutoff) { active = 0; value = 0; }
                else { active = 1; value = 1; }
                break;

            case 11:
                if (state->magazines[1].rounds_unloaded != 0) {
                    if ((int16_t)state->magazines[1].rounds_loaded > edx->loaded_ammo_cutoff) { active = 0; value = 0; }
                    else { active = 1; value = 1; }
                } else {
                    active = 0; value = 0;
                }
                break;

            case 12:
                if ((int16_t)state->magazines[1].rounds_unloaded > edx->total_ammo_cutoff) { active = 0; value = 0; }
                else if (state->magazines[1].reloading != 0) { active = 0; value = 0; }
                else { active = 1; value = 1; }
                break;

            case 13:
                value = state->magazines[1].reloading;
                active = value != 0 ? 1 : 0;
                break;

            case 14:
                if (state->magazines[1].rounds_loaded == 0 && state->magazines[1].rounds_unloaded == 0 &&
                    (unit->control_flags & _unit_control_flag_primary_trigger) != 0) {
                    active = 1; value = 1;
                } else if (out_array[case_index] != -1) {
                    active = 1; value = 1;
                } else {
                    active = 0; value = 0;
                }
                break;

            case 15:
                if (state->magazines[1].rounds_unloaded != 0) { active = 0; value = 0; }
                else if (state->magazines[1].rounds_loaded == 0) { active = 0; value = 0; }
                else if ((int16_t)state->magazines[1].rounds_loaded > edx->loaded_ammo_cutoff) { active = 0; value = 0; }
                else { active = 1; value = 1; }
                break;

            case 18:

                if (state->age == 1.0f && (unit->control_flags & _unit_control_flag_primary_trigger) != 0) {
                    active = 1; value = 1;
                } else if (out_array[case_index] != -1) {
                    active = 1; value = 1;
                } else {
                    active = 0; value = 0;
                }
                break;

            default:
                active = 0; value = 0;
                break;
            }

            if (active) {
                result_accum |= bit;
            } else {
                result_accum &= ~bit;
            }

            if (case_index == 0) {

                result_accum |= bit;
                out_array[0] = (int16_t)value;
            } else if (case_index == 1) {
                out_array[1] = (int16_t)value - 1;
            } else if (!active) {
                out_array[case_index] = -1;
            } else if (out_array[case_index] == -1) {
                out_array[case_index] = halo::game::globals().game_time->game_time;
            }

        }

        out_array[0x13] = result_accum;
    }
}

/**
 * @address 0x4b1740
 */
void WeaponHud::state_update()
{
    int16_t local_player_index = halo::game::globals().local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1;

    while (local_player_index != -1) {
        if (local_player_index >= 0 && local_player_index < 1 &&
            halo::game::globals().local_player_globals->local_players[local_player_index] != (datum_index)-1) {
            datum_index unit_index =
                (halo::interface::player_record(halo::game::globals().local_player_globals->local_players[local_player_index]))->unit;

            if (unit_index != (datum_index)-1) {
                uint8_t *unit = halo::interface::object_record(unit_index);
                int16_t slot = ((unit_object *)unit)->unit.current_weapon_index;
                datum_index weapon = slot != -1 ? ((unit_object *)unit)->unit.weapons[slot] : (datum_index)-1;
                uint8_t evaluate_default = 0;

                if (weapon == (datum_index)-1) {
                    datum_index parent = ((unit_object *)unit)->base.parent_object;
                    int16_t seat = ((unit_object *)unit)->unit.vehicle_seat_index;

                    if (parent == (datum_index)-1 || seat == -1) {
                        evaluate_default = 1;
                    } else {
                        uint8_t *parent_object = halo::interface::object_record(parent);
                        UnitSeat *seats = halo::interface::reflexive_elements<UnitSeat>(halo::interface::tag_data<Unit>(*(datum_index *)parent_object)->seats);

                        if ((seats[seat].flags & 8) != 0) {
                            int16_t parent_slot = ((struct unit_object *)parent_object)->unit.current_weapon_index;
                            weapon = parent_slot != -1 ? ((struct unit_object *)parent_object)->unit.weapons[parent_slot]
                                                       : (datum_index)-1;
                            if (weapon == (datum_index)-1) {
                                evaluate_default = 1;
                            }
                        }
                    }
                }

                if (weapon != (datum_index)-1) {
                    uint8_t *weapon_object = halo::interface::object_record(weapon);
                    Weapon *weapon_tag = halo::interface::tag_data<Weapon>(*(datum_index *)weapon_object);
                    weapon_hud_ammo_state ammo;

                    halo::items::weapon_build_hud_ammo_state(weapon, &ammo);
                    if (halo::interface::tag_handle(weapon_tag->hud_interface.tag_id) != (datum_index)-1) {
                        halo::interface::hud_weapon_interface_meters_evaluate(halo::interface::tag_handle(weapon_tag->hud_interface.tag_id), local_player_index,
                                                             weapon, &ammo);
                    }
                } else if (evaluate_default && halo::units::unit_count_deployed_weapons(unit_index) == 0) {
                    weapon_hud_ammo_state ammo;

                    memset(&ammo, 0, sizeof(ammo));
                    halo::interface::hud_weapon_interface_meters_evaluate(halo::interface::tag_handle(hud_globals_tag_data->default_weapon_hud.tag_id),
                                                         local_player_index, -1, &ammo);
                }
                hud_weapon_state->players[local_player_index].weapon = weapon;
            }
        }
        local_player_index = (halo::game::globals().local_player_globals->local_players[0] != (datum_index)-1 && local_player_index < 0)
                                 ? 0 : -1;
    }
}

/**
 * @address 0x492880
 */
int16_t WeaponHud::animation_stage(int16_t message_stage)
{
    switch (message_stage) {
    case 0:  return 0;
    case 1:  return 0x15;
    case 2:  return 0x16;
    case 3:  return 9;
    case 4:  return 0xc;
    case 5:  return 1;
    case 6:  return 2;
    case 7:  return 0xe;
    case 8:  return 0x12;
    case 9:  return 0x13;
    case 10: return 0xd;
    case 0xb: return 5;
    case 0xc: return 6;
    case 0xd: return 7;
    case 0xe: return 8;
    case 0xf: return 0x17;
    case 0x10: return 0x18;
    case 0x11: return 0x19;
    case 0x12: return 0xb;
    case 0x13: return 10;
    case 0x14: return 0x10;
    case 0x15: return 0x14;
    case 0x16: return 0x1a;
    case 0x17: return 0x1b;
    default: return -1;
    }
}

/**
 * @address 0x4927c0
 */
int16_t WeaponHud::message_stage(int16_t item_type_code)
{
    switch (item_type_code) {
    case 0:  return 6;
    case 1:  return 7;
    case 2:  return 8;
    case 3:  return 9;
    case 4:  return 10;
    case 5:  return 0xb;
    case 6:  return 0xc;
    case 9:  return 0xd;
    case 10: return 0xe;
    case 0xb: return 0x12;
    case 0xc: return 0x13;
    case 0xe: return 4;
    case 0xf: return 1;
    case 0x10: return 0x17;
    case 0x11: return 0x14;
    default: return -1;
    }
}

/**
 * Returns the weapon_hud_interface tag index for the current local player: the held weapon's, else (for a seat
 * with flag bit 3) the parent vehicle's current weapon's, else the hud_globals default weapon HUD when the
 * unit has no deployed weapons. Returns -1 when there is no local player unit, the HUD is off, or the camera
 * type is 2 or 3. *out_intensity gets unit +0x348 when the unit holds a weapon of its own, else 0.
 *
 * @address 0x494560
 */
int32_t WeaponHud::weapon_hud_interface(float *out_intensity)
{
    datum_index player_handle;
    int32_t result = -1;
    float intensity = 0.0f;

    if (current_local_player_index == -1 || current_local_player_index >= 1) {
        player_handle = (datum_index)-1;
    } else {
        player_handle = halo::game::globals().local_player_globals->local_players[current_local_player_index];
    }

    if (player_handle != (datum_index)-1) {
        player *player_record = halo::interface::player_record(player_handle);
        int16_t camera_type = halo::camera::camera_get_type_for_player(current_local_player_index);

        if (hud_flags != (hud_globals_flags *)0 && hud_flags->hud_enabled != 0 &&
            camera_type != 3 && camera_type != 2 && player_record->unit != (datum_index)-1) {
            datum_index unit_handle = player_record->unit;
            object *unit_obj = object_get(unit_handle);
            datum_index weapon_handle = halo::units::unit_get_weapon_object_index(
                unit_handle, ((struct unit_object *)unit_obj)->unit.current_weapon_index);

            if (weapon_handle == (datum_index)-1) {
                datum_index parent_handle = unit_obj->parent_object;
                int16_t seat_index = ((struct unit_object *)unit_obj)->unit.vehicle_seat_index;
                object *parent_obj;
                UnitSeat *seats;

                if (parent_handle == (datum_index)-1 || seat_index == -1) {
                    *out_intensity = intensity;
                    return result;
                }
                parent_obj = object_get(parent_handle);
                seats = halo::interface::reflexive_elements<UnitSeat>(halo::interface::tag_data<Unit>(parent_obj->definition_tag)->seats);
                if ((seats[seat_index].flags & 8) == 0) {
                    *out_intensity = intensity;
                    return result;
                }
                weapon_handle = halo::units::unit_get_weapon_object_index(
                    unit_obj->parent_object, ((struct unit_object *)parent_obj)->unit.current_weapon_index);
            } else {
                intensity = ((struct unit_object *)unit_obj)->unit.integrated_night_vision_power;
            }

            if (weapon_handle != (datum_index)-1) {
                Weapon *weapon_tag = halo::interface::tag_data<Weapon>(object_get(weapon_handle)->definition_tag);
                datum_index hud_interface = halo::interface::tag_handle(weapon_tag->hud_interface.tag_id);
                if (hud_interface != (datum_index)-1) {
                    *out_intensity = intensity;
                    return (int32_t)hud_interface;
                }
                if (halo::units::unit_count_deployed_weapons(unit_handle) == 0) {
                    result = static_cast<int32_t>(halo::interface::tag_handle(hud_globals_tag_data->default_weapon_hud.tag_id));
                }
            }
        }
    }

    *out_intensity = intensity;
    return result;
}

/**
 * blam-cc: EAX -> local_player_index, ECX -> object_index, ESI -> blip FIXED (register inputs, objdump): note
 * phrasing only -- rewritten from the reversed "name -> REG" form (and "object" for the object_index
 * parameter) the checker cannot parse.
 *
 * @address 0x4b35f0
 */
void MotionSensor::blip_fill(datum_index object_index, motion_sensor_blip *blip)
{
    blip->type = halo::interface::blip_type_get(local_player_index, object_index);
    if (object_index != (datum_index)-1 && halo::objects::object_try_and_get(object_index, 3) != 0) {
        uint8_t *object_ptr = halo::interface::object_record(object_index);
        int16_t subtype = halo::interface::tag_data<Unit>(*(datum_index *)object_ptr)->motion_sensor_blip_size;

        blip->subtype = (subtype >= 0 && subtype < 3) ? (uint8_t)subtype : 0;
        return;
    }
    blip->subtype = 0;
}

/**
 * @address 0x4b36a0
 */
uint8_t MotionSensor::object_is_detected(datum_index unit_index)
{
    unit_data *unit;
    real_vector3d velocity, angular;
    uint8_t visible;
    float speed_sq, threshold;

    if (halo::objects::object_try_and_get(unit_index, 3) == 0) {
        return 0;
    }
    if (halo::game::globals().current_engine != 0) {
        if ((motion_sensor_override_value & 1) == 0) {
            return 0;
        }

    }

    unit = &halo::interface::object_record<unit_object>(unit_index)->unit;
    if ((unit->control_flags & _unit_control_flag_primary_trigger) != 0) {
        return 1;
    }
    if (unit->throwing_grenade_state != 0 && unit->throwing_grenade_state != 3) {
        return 1;
    }

    halo::objects::object_get_root_object_velocities((uint32_t)unit_index, &velocity, (real_vector3d *)0);

    visible = (halo::game::globals().current_engine == 0 && (unit->flags & _unit_flag_unknown_10) != 0)
              ? 0 : 1;

    speed_sq = velocity.i * velocity.i + velocity.j * velocity.j + velocity.k * velocity.k;
    threshold = hud_globals_tag_data->motion_sensor_velocity_sensitivity;

    if (!(speed_sq < threshold)) {
        return visible ? 1 : 0;
    }
    if (motion_sensor_force_moving != 0) {
        return visible ? 1 : 0;
    }
    return 0;
}

/**
 * blam-cc: position -> EAX, type -> BL
 *
 * @address 0x4b37a0
 */
void MotionSensor::plot_blip(const float *position, uint8_t type, const motion_sensor_frame *frame, int8_t subtype, float pixels_per_unit, float alpha, float size_factor)
{
    float x = position[0];
    float y = position[1];
    float sine = halo::libm::sinf(-frame->viewer_facing);
    float cosine = halo::libm::cosf(-frame->viewer_facing);
    float u = y * cosine + x * sine;
    float v = x * cosine - y * sine;
    float range = hud_globals_tag_data->motion_sensor_range;
    float distance;
    float pulled;
    float point[2];
    float pulse;
    float size;

    if (!(v * v + u * u < range * range)) {
        return;
    }
    distance = halo::libm::sqrtf(v * v + u * u);
    if (distance < 0.015625f) {
        distance = 0.015625f;
    }
    pulled = (float)halo::libm::pow((double)(distance / range), 0.7) * range;
    point[0] = v * (1.0f / distance) * pulled * pixels_per_unit;
    point[1] = u * (1.0f / distance) * pulled * pixels_per_unit;
    size = motion_sensor_blip_subtype_size[subtype];
    pulse = 1.0f;
    if (type == 5) {
        pulse = (float)((halo::libm::sin((double)((float)halo::game::globals().game_time->game_time * 0.10471973568201065f)) + 1.0) * 0.3333333333333333 + 1.0);
    }
    halo::rasterizer::rasterizer_motion_sensor_blip_draw(point, (const float *)(&motion_sensor_blip_colors[(int8_t)type]), alpha, pulse * size_factor + size);
}

/**
 * blam-cc: screen_center -> EAX, local_player_index -> CX
 *
 * @address 0x4b4120
 */
void MotionSensor::render(uint8_t splitscreen, const int16_t *screen_center, int16_t local_player_index)
{
    float pixels_per_unit;
    float range;
    int32_t k;

    {
        int16_t camera_type = halo::camera::camera_get_type_for_player(local_player_index);
        if (camera_type == 3 || camera_type == 2) {
            return;
        }
    }
    range = hud_globals_tag_data->motion_sensor_range;
    pixels_per_unit = hud_globals_tag_data->motion_sensor_scale / range;
    motion_sensor_render_local_player = local_player_index;
    motion_sensor_render_icon_scale = 0.75f;
    if (splitscreen == 0) {
        motion_sensor_render_icon_scale = 1.0f;
    }
    motion_sensor_render_center[0] = (float)screen_center[0];
    motion_sensor_render_center[1] = (float)screen_center[1];
    halo::rasterizer::rasterizer_motion_sensor_begin();

    for (k = 0; k < 10; k++) {
        motion_sensor_player_state *state = &motion_sensor->players[local_player_index];
        motion_sensor_frame *frame = &state->history[(int16_t)((motion_sensor->frame_index - k + 10) % 10)];
        float age = (float)(10 - k) * 0.1f;
        float alpha = age * age;
        float size = (float)(halo::libm::pow((double)(1.0f - age), 3.5) * 7.0 + 1.0);
        int32_t i;

        for (i = 0; i < 0x10; i++) {
            motion_sensor_blip *blip = &frame->blips[i];
            float position[2];

            if (blip->type == _blip_type_empty) {
                continue;
            }
            if (halo::game::globals().current_engine != 0 && (halo::game::globals().variant.flags & 0x40) != 0 &&
                (blip->type == 2 || blip->type == 4)) {
                continue;
            }
            position[0] = (float)blip->x * range * 0.007874015718698502f;
            position[1] = (float)blip->y * range * 0.007874015718698502f;
            halo::interface::motion_sensor_plot_blip(position, blip->type, frame, (int8_t)blip->subtype, pixels_per_unit, alpha, size);
        }
        for (i = 0; (int16_t)i < (int32_t)frame->extra_blip_count; i++) {
            float position[2];

            if (custom_waypoints[(int8_t)frame->extra_sources[i]].active == 0) {
                continue;
            }
            position[0] = (float)frame->extra_blips[i * 2] * range * 0.007874015718698502f;
            position[1] = (float)frame->extra_blips[i * 2 + 1] * range * 0.007874015718698502f;
            halo::interface::motion_sensor_plot_blip(position, 5, frame, 0, pixels_per_unit, alpha, size);
        }
    }
    halo::rasterizer::rasterizer_motion_sensor_end(motion_sensor_render_center, motion_sensor_sweep);
}

/**
 * @address 0x4b3660
 */
void MotionSensor::reset()
{
    int32_t *clear = (int32_t *)motion_sensor;
    int i;
    uint8_t *type_byte;
    int group, slot;

    for (i = 0; i < halo::interface::k_motion_sensor_dwords; i++) {
        clear[i] = 0;
    }

    type_byte = (uint8_t *)motion_sensor + 2;
    for (group = 0; group < 10; group++) {
        uint8_t *slot_type = type_byte;
        for (slot = 0; slot < 0x10; slot++) {
            *slot_type = _blip_type_empty;
            slot_type += 4;
        }
        type_byte += 0x84;
    }
}

/**
 * @address 0x4b3e10
 */
void MotionSensor::update_for_player()
{
    motion_sensor_player_state *state = &motion_sensor->players[local_player_index];
    motion_sensor_frame *frame;
    datum_index player_index;
    datum_index unit_index;
    real_point3d camera;
    float waypoints[0x10][2];
    float range = hud_globals_tag_data->motion_sensor_range;
    uint8_t removed;
    int32_t i;

    if (motion_sensor->enabled == 0) {
        return;
    }
    frame = &state->history[motion_sensor->frame_index];
    unit_index = (datum_index)-1;
    if (local_player_index != -1 && local_player_index < 1 &&
        halo::game::globals().local_player_globals->local_players[local_player_index] != (datum_index)-1) {
        unit_index = (halo::interface::player_record(halo::game::globals().local_player_globals->local_players[local_player_index]))->unit;
    }
    if (unit_index == (datum_index)-1) {
        return;
    }
    halo::units::unit_get_camera_position(unit_index, &camera);
    frame->viewer_x = camera.x;
    frame->viewer_y = camera.y;

    for (i = 0; i < 0x10; i++) {
        datum_index tracked = state->tracked_objects[i];
        uint8_t detected;
        real_point3d position;
        float dx;
        float dy;

        if (halo::objects::object_try_and_get(tracked, 3) == 0) {
            continue;
        }
        detected = halo::interface::motion_sensor_object_is_detected(tracked);
        position = *(real_point3d *)(halo::interface::object_record(tracked) + 0xa0);
        dx = position.x - frame->viewer_x;
        dy = position.y - frame->viewer_y;
        if (detected != 0 && !(range * range < dy * dy + dx * dx)) {
            frame->blips[i].x = (int8_t)halo::x87::__ftol((double)(dx / range * 127.0f));
            frame->blips[i].y = (int8_t)halo::x87::__ftol((double)(dy / range * 127.0f));
        } else {
            frame->blips[i].type = _blip_type_empty;
            state->tracked_objects[i] = (datum_index)-1;
        }
    }

    frame->viewer_facing = halo::game::globals().player_control->local_players[local_player_index].yaw + 1.5707963705062866f;
    player_index = (local_player_index != -1 && local_player_index < 1)
                       ? halo::game::globals().local_player_globals->local_players[local_player_index] : (datum_index)-1;
    frame->extra_blip_count = (uint8_t)halo::game::game_engine_collect_matching_waypoints((int32_t)player_index, &waypoints[0][0],
                                                                               frame->extra_sources, 0x10);
    halo::units::unit_get_camera_position(unit_index, &camera);
    removed = 0;
    for (i = 0; i < frame->extra_blip_count; i++) {
        float dx;
        float dy;

        waypoints[i][0] = waypoints[i][0] - camera.x;
        waypoints[i][1] = waypoints[i][1] - camera.y;
        dx = waypoints[i][0];
        dy = waypoints[i][1];
        if (range * range < dx * dx + dy * dy) {
            removed++;
            continue;
        }
        frame->extra_blips[(i - removed) * 2] = (int8_t)halo::x87::__ftol((double)(waypoints[i][0] / range * 127.0f));
        frame->extra_blips[(i - removed) * 2 + 1] = (int8_t)halo::x87::__ftol((double)(waypoints[i][1] / range * 127.0f));
    }
    frame->extra_blip_count = (uint8_t)(frame->extra_blip_count - removed);
}

/**
 * blam-cc: object_index -> EAX
 *
 * @address 0x4a9b40
 */
int16_t WeaponHud::text_message_index(datum_index object_index)
{
    struct object *obj;
    Object *definition;

    if (object_index == (datum_index)-1) {
        return -1;
    }
    obj = ((object_header *)halo::objects::globals().object_data->data)[object_index & halo::k_slot_mask].data;
    definition = halo::interface::tag_data<Object>(obj->definition_tag);
    return definition->hud_text_message_index;
}

/**
 * Applies a weapon HUD action to unit_index's local first-person weapon interface if the unit belongs to a
 * local player; otherwise, if the unit has a current weapon, plays the fallback pickup/HUD notification
 * instead.
 *
 * @address 0x492730
 */
void WeaponHud::notify_for_unit(datum_index unit_index, int32_t action_code)
{
    int32_t local_player;
    unit_data *u;

    local_player = halo::interface::local_player_index_for_unit(unit_index);
    halo::interface::first_person_weapon_process_action(local_player, action_code);

    if (local_player == -1) {
        u = &halo::interface::object_record<unit_object>(unit_index)->unit;
        if (u->current_weapon_index != -1) {
            halo::interface::hud_play_pickup_notification((uint32_t)(uint16_t)u->current_weapon_index, (int16_t)action_code);
        }
    }
}

/**
 * Applies a weapon HUD action to weapon_index's local first-person weapon interface if the weapon is equipped
 * by a local player; otherwise plays the fallback pickup/HUD notification.
 *
 * @address 0x492790
 */
void WeaponHud::notify_for_weapon(datum_index weapon_index, int32_t action_code)
{
    int32_t local_player;

    local_player = halo::interface::local_player_index_for_weapon(weapon_index);
    halo::interface::first_person_weapon_process_action(local_player, action_code);

    if (local_player == -1) {
        halo::interface::hud_play_pickup_notification((uint32_t)weapon_index, (int16_t)action_code);
    }
}

/**
 * @address 0x4a9750
 */
uint8_t WeaponHud::ammo_state_is_empty(const weapon_hud_ammo_state *state)
{
    const weapon_hud_magazine_state *magazine = &state->magazines[0];

    if (magazine->rounds_loaded_maximum != 0 && magazine->rounds_loaded == 0 &&
        magazine->rounds_unloaded == 0) {
        return 1;
    }
    return state->age == 1.0f;
}

} // namespace halo::interface

namespace halo::interface {

int16_t hud_waypoint_visibility(int16_t local_player_index, const real_point3d *eye, const real_point3d *target, datum_index ignore_object)
{
    return halo::interface::HudWaypoints(local_player_index).visibility(eye, target, ignore_object);
}

void hud_waypoints_draw_for_player(int16_t local_player_index)
{
    halo::interface::HudWaypoints(local_player_index).draw_for_player();
}

void hud_waypoints_update(void)
{
    halo::interface::HudWaypoints::update();
}

void hud_waypoints_update_for_player(int16_t local_player_index)
{
    halo::interface::HudWaypoints(local_player_index).update_for_player();
}

void hud_weapon_crosshairs_draw(datum_index hud_tag, const player *p, const weapon_hud_ammo_state *ammo)
{
    halo::interface::WeaponHud::crosshairs_draw(hud_tag, p, ammo);
}

void hud_weapon_interface_draw_elements(datum_index hud_tag, int16_t local_player_index, const Weapon *weapon_tag, const weapon_hud_ammo_state *ammo, const uint16_t *parent_state_flags, const uint16_t *parent_overlay_types, const int16_t *parent_numbers)
{
    halo::interface::WeaponHud::draw_elements(hud_tag, local_player_index, weapon_tag, ammo, parent_state_flags, parent_overlay_types, parent_numbers);
}

void hud_weapon_interface_meters_evaluate(datum_index hud_interface_tag_id, int16_t local_player_index, int32_t weapon_or_vehicle_index, const weapon_hud_ammo_state *state_ptr)
{
    halo::interface::WeaponHud::meters_evaluate(hud_interface_tag_id, local_player_index, weapon_or_vehicle_index, state_ptr);
}

void hud_weapon_interface_state_update(void)
{
    halo::interface::WeaponHud::state_update();
}

int16_t item_type_to_animation_stage(int16_t message_stage)
{
    return halo::interface::WeaponHud::animation_stage(message_stage);
}

int16_t item_type_to_message_stage(int16_t item_type_code)
{
    return halo::interface::WeaponHud::message_stage(item_type_code);
}

int32_t local_player_get_weapon_hud_interface(float *out_intensity)
{
    return halo::interface::WeaponHud::weapon_hud_interface(out_intensity);
}

void motion_sensor_blip_fill(int16_t local_player_index, datum_index object_index, motion_sensor_blip *blip)
{
    halo::interface::MotionSensor(local_player_index).blip_fill(object_index, blip);
}

uint8_t motion_sensor_object_is_detected(datum_index unit_index)
{
    return halo::interface::MotionSensor::object_is_detected(unit_index);
}

void motion_sensor_plot_blip(const float *position, uint8_t type, const motion_sensor_frame *frame, int8_t subtype, float pixels_per_unit, float alpha, float size_factor)
{
    halo::interface::MotionSensor::plot_blip(position, type, frame, subtype, pixels_per_unit, alpha, size_factor);
}

void motion_sensor_render(uint8_t splitscreen, const int16_t *screen_center, int16_t local_player_index)
{
    halo::interface::MotionSensor::render(splitscreen, screen_center, local_player_index);
}

void __cdecl motion_sensor_reset(void)
{
    halo::interface::MotionSensor::reset();
}

void motion_sensor_update_for_player(int16_t local_player_index)
{
    halo::interface::MotionSensor(local_player_index).update_for_player();
}

int16_t object_get_hud_text_message_index(datum_index object_index)
{
    return halo::interface::WeaponHud::text_message_index(object_index);
}

void weapon_action_notify_for_unit(datum_index unit_index, int32_t action_code)
{
    halo::interface::WeaponHud::notify_for_unit(unit_index, action_code);
}

void weapon_action_notify_for_weapon(datum_index weapon_index, int32_t action_code)
{
    halo::interface::WeaponHud::notify_for_weapon(weapon_index, action_code);
}

uint8_t weapon_hud_ammo_state_is_empty(const weapon_hud_ammo_state *state)
{
    return halo::interface::WeaponHud::ammo_state_is_empty(state);
}

}
