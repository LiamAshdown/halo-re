// hud_render_unit_interface  (Ghidra: hud_render_unit_interface, already named; the Chimera
// signature gametype_skip_draw_sig sits at the loop tail 0x4b148e)
// address 0x4b0320, size 4468 bytes (to 0x4b14a0; jump table 0x4b14a4 by game engine index)
// name confidence: 0.85   rewrite confidence: 0.7
// evidence: rewritten from objdump 0x4b0320..0x4b14a0 in the phase-4 review. The first rewrite
// was an acknowledged incomplete transliteration (the overshield meter loop, the motion sensor
// and the team icon were left out). The argument is a player record; nothing is drawn unless it
// is the current local player (0x007c3108) and has a unit.
// Flow: collects up to 18 {object, UnitHUDInterface tag} pairs: the unit with its new hud
// interface (split screen picks the second entry), then, when the unit rides a seat whose Unit
// tag seat flags have bit 2 set, the parent vehicle (unit_get_hud_interface_tag_id, EDX tag, AL
// split) and every unit riding a seat of that parent (children chain +0x118/+0x114,
// unit_get_seat_hud_interface_tag_id, ECX tag, AX seat, stack split). They are drawn from the
// last to the first, the unit last. Per entry: the HUD background; the shield panel (skipped
// with game variant flags bit 3 or script flag bit 2): the overshield layers 0..4 (0x00692fcc)
// each drawn with hud_meter_draw_fill from a copy of the meter placement whose min and max
// colors are the layer color (none, red, green, yellow, purple), layer 0 with the tag
// placement itself; the health panel (script flag bit 0) with the medium/min/max health color
// cutoffs; for the unit itself the motion sensor (script flag bit 4 hides it, 0x4635e0 must
// allow it; anchor 2, center from +0x35c, motion_sensor_update_for_player and
// motion_sensor_render); the auxiliary overlays of a team game (the team color written into
// the overlay tag default color when overlay flags bit 0 is set); the auxiliary meters
// (integrated light: unit +0x344 energy, enabled while unit +0x340 is 1.0 for a valid team
// player, blinking below 0.2 while the light is off); and in multiplayer the team icon of the
// game type over the team background (red/blue in a team game, else the player color).
// Behaviour kept from the binary: the flash times, script flags and the overlay color are
// written into game state and tag data exactly as the binary does; the overshield alphas round
// with fistp while the health alphas truncate (0x4ab590); an unknown game engine index draws
// the icon with a stale position and scale.
// register convention: plain cdecl (EBP frame), one stack argument.
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances;            // 0x0087bc14
extern player_globals *local_player_globals;   // 0x0087a478
extern game_time_globals *game_time;           // 0x006f1d6c
extern hud_unit_meter_globals *hud_unit_meters; // 0x0071942c
extern int16_t current_local_player_index; // 0x007c3108
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;       // 0x006f1c88 (index +0x30, teams +0x34, flags +0x38)
extern int32_t hud_overshield_layer_count;     // 0x00692fcc, 4
extern datum_index hud_team_icon_bitmap;       // 0x00692fc4, cached team_icon_<game type> bitmap
extern datum_index hud_team_background_bitmap; // 0x00692fc8, cached ui\shell\bitmaps\team_background

extern long lrint(double x); // x87 fistp under the default control word (round-half-to-even)
extern int32_t ui_real_to_int_truncate(float value); // 0x4ab590
extern uint8_t game_engine_is_valid_team_player(uint32_t identifier); // 0x466b60
extern uint8_t game_engine_scores_tracked_individually(void); // 0x4635e0, UNSURE name: here it gates the motion sensor
extern float *game_engine_get_player_color(uint32_t player_index, float *out_rgb); // 0x463290, blam-cc: EAX player_index, ESI out_rgb
extern TagID unit_get_hud_interface_tag_id(Unit *unit_tag, uint8_t use_second); // 0x560c70, blam-cc: EDX unit_tag, AL use_second
extern TagID unit_get_seat_hud_interface_tag_id(Unit *unit_tag, int16_t seat_index, uint8_t use_second); // 0x560cb0, blam-cc: ECX unit_tag, AX seat_index
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence); // 0x43f290, blam-cc: EAX tag, DI frame
extern uint32_t color_rgb_float_to_int(const float *rgb); // 0x4ab5d0
extern void hud_anchor_offset_to_screen_position(uint16_t *anchor, uint8_t has_scale, float scale,
                                                 const int16_t *offset, int16_t *out, int32_t selector); // 0x4ab690, blam-cc: AL has_scale, EDX offset, ECX child placement (selector)
extern void hud_meter_draw_fill(void *dest, uint8_t value_a, uint8_t value_b, uint32_t flags,
                                float fraction, float fraction_2, const hud_meter_placement *meter); // 0x4abbc0, blam-cc: ESI meter
extern void hud_draw_static_element(int16_t local_player_index, uint16_t *anchor,
                                    const hud_static_element_placement *element, uint32_t draw_flags,
                                    int32_t flash_start_time); // 0x4ac6f0
extern void hud_draw_rotated_bitmap_quad(const Point2DInt *screen_position, const float *scale,
                                         void *meter_parameters, BitmapData *bitmap, const float *uv,
                                         const float *extents, float rotation, uint32_t color); // 0x4acd50, blam-cc: EAX screen_position, ESI scale
extern void motion_sensor_update_for_player(int16_t local_player_index); // 0x4b3e10
extern void motion_sensor_render(uint8_t splitscreen, const int16_t *screen_center,
                                 int16_t local_player_index); // 0x4b4120, blam-cc: EAX screen_center, CX local_player_index

// fistp of the product, clamped to 0..255 (evaluated up to three times by the binary)
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

// 0x4ab590 of the product, clamped to 0..255
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

void hud_render_unit_interface(player *p)
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
        int32_t last = (int32_t)*(uint32_t *)((uint8_t *)unit_tag + 0x2a8) - 1; // new_hud_interfaces count
        int32_t choice = (int16_t)(local_player_globals->local_player_count > 1);
        if (choice > last) {
            choice = last;
        }
        hud_tags[0] = (int16_t)choice < 0
            ? (datum_index)-1
            : *(datum_index *)(*(uint8_t **)((uint8_t *)unit_tag + 0x2ac) + (int16_t)choice * 0x30 + 0xc);
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

        parent = *(datum_index *)(unit_object + 0x11c); // object parent_object
        if (parent != (datum_index)-1 && *(int16_t *)(unit_object + 0x2f0) != -1) { // unit vehicle_seat_index
            uint8_t *parent_object = (uint8_t *)((object_header *)object_data->data)[parent & 0xffff].data;
            Unit *parent_tag = (Unit *)tag_instances[*(datum_index *)parent_object & 0xffff].data;
            uint8_t split = local_player_globals->local_player_count > 1;
            TagID parent_hud = unit_get_hud_interface_tag_id(parent_tag, split);
            uint8_t *seats = *(uint8_t **)((uint8_t *)parent_tag + 0x2e8);

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

                    if (child_object != 0 && *(datum_index *)(child_object + 0x11c) == parent &&
                        *(int16_t *)(child_object + 0x2f0) != -1) {
                        TagID seat_hud = unit_get_seat_hud_interface_tag_id(parent_tag, *(int16_t *)(child_object + 0x2f0),
                                                                            split);
                        objects[count] = child;
                        hud_tags[count] = *(datum_index *)&seat_hud;
                        count++;
                    }
                    child = ((object *)child_data)->next_object; // object next_object
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

        // hud background
        if (*(datum_index *)&hud->hud_background_interface_bitmap.tag_id != (datum_index)-1) {
            flags = (uint32_t)((*(uint8_t *)(object + 0x106) >> 1) & 2);
            if (local_player_globals->local_player_count > 1) {
                flags |= 4;
            }
            hud_draw_static_element(local_player_index, (uint16_t *)hud,
                                    (const hud_static_element_placement *)&hud->hud_background_anchor_offset, flags, -1);
        }

        // shield panel
        if ((current_game_engine == 0 || player_index == (datum_index)-1 ||
             ((game_engine_variant.flags >> 3) & 1) == 0) &&
            (hud_unit_meters->flags & 4) == 0) {
            float shield = *(float *)(object + 0xe4); // object shield vitality
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
                memcpy(&layer_placement, &hud->shield_panel_meter_anchor_offset, 0x68); // 0x1a dwords
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

        // health panel
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
                float health = *(float *)(object + 0xe0); // object body vitality
                int32_t value_scale = (uint16_t)hud->health_panel_meter_value_scale;
                int32_t alpha_a;
                int32_t alpha_b;

                if (value_scale == 0) {
                    value_scale = 8;
                }
                memcpy(&health_placement, &hud->health_panel_meter_anchor_offset, 0x68); // 0x1a dwords
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

        // motion sensor
        if (index == 0 && (hud_unit_meters->flags & 0x10) == 0 && game_engine_scores_tracked_individually() != 0) {
            uint16_t anchor[0x12];
            Point2DInt center;

            anchor[0] = 2; // bottom left; the rest of the record is not written
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

        // auxiliary overlays
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
                if ((*(uint8_t *)&overlay->flags & 1) != 0) { // use team color: written into the tag
                    *(uint32_t *)&overlay->default_color = color_rgb_float_to_int((const float *)(object + 0x188)) | 0xff000000;
                }
                hud_draw_static_element(local_player_index, (uint16_t *)&hud->auxiliary_overlay_anchor,
                                        (const hud_static_element_placement *)overlay, flags, -1);
            }
        }

        // auxiliary meters
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

        // multiplayer team icon
        if (current_game_engine != 0) {
            static const float full_uv[4] = {0.0f, 1.0f, 0.0f, 1.0f};
            static char *icon_paths[5] = {
                "ui\\shell\\bitmaps\\team_icon_ctf", "ui\\shell\\bitmaps\\team_icon_slayer",
                "ui\\shell\\bitmaps\\team_icon_oddball", "ui\\shell\\bitmaps\\team_icon_king",
                "ui\\shell\\bitmaps\\team_icon_race"};
            static const int16_t icon_x[5] = {0x1c8, 0x1c9, 0x1c8, 0x1ca, 0x1c5};
            static const int16_t icon_y[5] = {6, 8, 7, 8, 6};
            static const float icon_scale[5] = {0.55f, 0.5f, 0.55f, 0.53f, 0.6f};
            static Point2DInt icon_position;   // stack slots the binary leaves stale for an unknown engine
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
                hud_team_icon_bitmap = tag_lookup(0x6269746d, icon_paths[engine]); // 'bitm'
                icon_position.x = icon_x[engine];
                icon_position.y = icon_y[engine];
                icon_scales[0] = icon_scale[engine];
                icon_scales[1] = icon_scale[engine];
            }
            icon = bitmap_group_sequence_get_bitmap_data(hud_team_icon_bitmap, 0, 0);
            hud_team_background_bitmap = tag_lookup(0x6269746d, "ui\\shell\\bitmaps\\team_background");
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

#if 0
Original Ghidra decompilation (0x4b0320):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* WARNING: Restarted to delay deadcode elimination for space: stack */

void hud_render_unit_interface(int param_1)

{
  short *psVar1;
  ushort uVar2;
  uint *puVar3;
  float fVar4;
  float fVar5;
  char cVar6;
  byte bVar7;
  short sVar8;
  undefined4 *puVar9;
  uint uVar10;
  int iVar11;
  undefined4 uVar12;
  float *pfVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  uint uVar17;
  short sVar18;
  uint uVar19;
  undefined **ppuVar20;
  float *pfVar21;
  short *psVar22;
  uint *puVar23;
  undefined4 *puVar24;
  undefined4 uVar25;
  bool bVar26;
  int local_28c;
  uint local_26c;
  float local_268;
  uint local_264;
  float local_260;
  float local_25c;
  undefined4 local_258;
  undefined4 local_254;
  uint local_250;
  uint local_24c;
  int local_248;
  float local_244;
  int local_240;
  float local_23c;
  float local_238;
  undefined4 local_234;
  undefined4 local_230;
  undefined4 local_22c;
  undefined4 local_228;
  int local_224;
  int local_220;
  int local_21c;
  int local_218;
  float local_214;
  undefined4 local_210;
  float local_20c;
  int local_208;
  float local_204;
  int local_200;
  int local_1fc;
  int local_1f8;
  int local_1f4;
  int local_1f0;
  int local_1ec;
  float local_1e8;
  undefined1 local_1e4 [4];
  int local_1e0;
  int local_1dc;
  int local_1d8;
  float local_1d4;
  float local_1d0;
  float local_1cc;
  undefined4 local_1c8;
  undefined4 local_1c4;
  undefined4 local_1c0;
  undefined4 local_1bc;
  undefined4 local_1b8;
  undefined4 local_1b4;
  undefined *local_1b0 [5];
  undefined2 local_19c [26];
  undefined4 local_168 [13];
  undefined4 local_134;
  undefined4 local_130;
  uint local_100 [18];
  uint local_b8 [18];
  undefined4 local_70 [13];
  undefined *local_3c;
  undefined *local_38;

  uVar2 = *(ushort *)(param_1 + 2);
  local_264 = (uint)uVar2;
  if ((uVar2 == DAT_007c3108) && (uVar17 = *(uint *)(param_1 + 0x34), uVar17 != 0xffffffff)) {
    puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar17 & 0xffff) * 0xc);
    iVar16 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if ((uVar2 == 0xffff) || (0 < (short)uVar2)) {
      local_240 = -1;
    }
    else {
      local_240 = *(int *)(DAT_0087a478 + 4 + (short)uVar2 * 4);
    }
    pfVar21 = (float *)((short)uVar2 * 0x58 + DAT_0071942c);
    puVar23 = local_b8;
    for (iVar14 = 0x11; puVar23 = puVar23 + 1, iVar14 != 0; iVar14 = iVar14 + -1) {
      *puVar23 = 0;
    }
    iVar14 = *(int *)(iVar16 + 0x2a8) + -1;
    iVar15 = (int)(short)(ushort)(1 < *(short *)(DAT_0087a478 + 0xc));
    if (iVar15 <= iVar14) {
      iVar14 = iVar15;
    }
    if ((short)iVar14 < 0) {
      local_100[0] = 0xffffffff;
    }
    else {
      local_100[0] = *(uint *)((short)iVar14 * 0x30 + 0xc + *(int *)(iVar16 + 0x2ac));
    }
    puVar23 = local_100;
    local_b8[0] = uVar17;
    for (iVar16 = 0x11; puVar23 = puVar23 + 1, iVar16 != 0; iVar16 = iVar16 + -1) {
      *puVar23 = 0;
    }
    uVar19 = 1;
    cVar6 = FUN_00466b60(uVar17);
    if (pfVar21[7] == -NAN) {
      puVar9 = (undefined4 *)(*(short *)(param_1 + 2) * 0x58 + DAT_0071942c);
      *(undefined2 *)((int)puVar9 + 0x22) = 0xffff;
      puVar9[1] = 0xbf800000;
      *puVar9 = 0xbf800000;
      puVar9[5] = 0xffffffff;
      puVar9[6] = 0xffffffff;
      puVar9[2] = 0xbf800000;
      puVar9[7] = 0xffffffff;
    }
    pfVar21[7] = *(float *)(param_1 + 0x34);
    uVar17 = puVar3[0x47];
    if ((uVar17 != 0xffffffff) && ((short)puVar3[0xbc] != -1)) {
      local_248 = *(int *)(DAT_008603b0 + 0x34);
      puVar23 = *(uint **)(local_248 + 8 + (uVar17 & 0xffff) * 0xc);
      iVar16 = *(int *)((*puVar23 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      local_24c = CONCAT31(local_24c._1_3_,1 < *(short *)(DAT_0087a478 + 0xc));
      iVar14 = FUN_00560c70();
      if ((*(byte *)((short)puVar3[0xbc] * 0x11c + *(int *)(iVar16 + 0x2e8)) & 4) != 0) {
        if (iVar14 != -1) {
          uVar19 = 2;
          local_100[1] = iVar14;
          local_b8[1] = uVar17;
        }
        uVar10 = puVar23[0x46];
        while ((uVar10 != 0xffffffff && (uVar19 < 0x12))) {
          local_250 = *(int *)(local_248 + 8 + (uVar10 & 0xffff) * 0xc);
          iVar16 = object_try_and_get(3);
          if ((iVar16 != 0) &&
             ((*(uint *)(iVar16 + 0x11c) == uVar17 && (*(short *)(iVar16 + 0x2f0) != -1)))) {
            local_b8[uVar19] = uVar10;
            uVar10 = FUN_00560cb0(local_24c);
            local_100[uVar19] = uVar10;
            uVar19 = uVar19 + 1;
          }
          uVar10 = *(uint *)(local_250 + 0x114);
        }
      }
    }
    if ((cVar6 == '\0') || (local_26c = 1, puVar3[0xd0] != 0x3f800000)) {
      local_26c = 0;
    }
    if ((((puVar3[0x81] & 0x80000) != 0) || (0.2 <= (float)puVar3[0xd1])) ||
       (local_250 = 1, (puVar3[0x82] & 0x10) == 0)) {
      local_250 = 0;
    }
    local_268 = (float)puVar3[0xd1];
    while (uVar19 != 0) {
      uVar19 = uVar19 - 1;
      iVar16 = object_try_and_get(3);
      if ((iVar16 != 0) && (local_100[uVar19] != 0xffffffff)) {
        iVar14 = *(int *)((local_100[uVar19] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if (*(int *)(iVar14 + 0x54) != -1) {
          bVar7 = *(byte *)(iVar16 + 0x106) >> 1 & 2;
          if (1 < *(short *)(DAT_0087a478 + 0xc)) {
            bVar7 = bVar7 | 4;
          }
          FUN_004ac6f0(local_264,iVar14,iVar14 + 0x24,bVar7,0xffffffff);
        }
        if ((((DAT_006f1d20 == 0) || (local_240 == -1)) || ((~(byte)(DAT_006f1cc0 >> 3) & 1) != 0))
           && ((*(uint *)(DAT_0071942c + 0x58) & 4) == 0)) {
          if ((*(float *)(iVar16 + 0xe4) < 0.25) || ((*(uint *)(DAT_0071942c + 0x58) & 8) != 0)) {
            uVar17 = 1;
          }
          else {
            uVar17 = 0;
          }
          if ((*(byte *)(iVar16 + 0x106) & 4) != 0) {
            uVar17 = uVar17 | 2;
          }
          if (1 < *(short *)(DAT_0087a478 + 0xc)) {
            uVar17 = uVar17 | 4;
          }
          if (uVar19 == 0) {
            if ((uVar17 & 1) == 0) {
              pfVar21[4] = -NAN;
            }
            else if (pfVar21[4] == -NAN) {
              pfVar21[4] = *(float *)(DAT_006f1d6c + 0xc);
            }
          }
          if (*(int *)(iVar14 + 0x124) != -1) {
            if (uVar19 == 0) {
              local_23c = *pfVar21;
            }
            else {
              local_23c = *(float *)(iVar16 + 0xe4);
            }
            sVar18 = *(short *)(iVar14 + 0x13e);
            if (sVar18 == 0) {
              sVar18 = 0xff;
            }
            puVar9 = (undefined4 *)(iVar14 + 0xf4);
            puVar24 = local_70;
            for (iVar15 = 0x1a; iVar15 != 0; iVar15 = iVar15 + -1) {
              *puVar24 = *puVar9;
              puVar9 = puVar9 + 1;
              puVar24 = puVar24 + 1;
            }
            local_1b0[0] = (undefined *)0x0;
            local_1b0[1] = (undefined *)0xff0000;
            local_1b0[2] = (undefined *)0xff00;
            local_1b0[3] = (undefined *)0xffff00;
            local_1b0[4] = &DAT_007f00ff;
            ppuVar20 = local_1b0;
            local_28c = 0;
            if (-1 < DAT_00692fcc) {
              do {
                local_260 = *(float *)(iVar16 + 0xe4) - (float)local_28c;
                if (0.0 <= local_260) {
                  if (local_260 <= 1.0) {
                  }
                  else {
                    local_260 = 1.0;
                  }
                }
                else {
                  local_260 = 0.0;
                }
                fVar4 = local_23c - (float)local_28c;
                if (0.0 <= fVar4) {
                  if (1.0 < fVar4) {
                    fVar4 = 1.0;
                  }
                }
                else {
                  fVar4 = 0.0;
                }
                fVar5 = local_260;
                if (fVar4 > local_260) {
                  fVar5 = fVar4;
                }
                if ((local_260 < 0.0 != (local_260 == 0.0)) && (fVar5 < 0.0 != (fVar5 == 0.0)))
                break;
                local_3c = *ppuVar20;
                if (fVar4 <= local_260) {
                  local_238 = -1.0;
                }
                else {
                  local_238 = pfVar21[2];
                }
                local_244 = (float)(int)sVar18 * fVar5;
                local_1fc = (int)ROUND(local_244);
                if (local_1fc < 0) {
                  iVar15 = 0;
                }
                else {
                  local_208 = (int)ROUND(local_244);
                  iVar15 = 0xff;
                  local_204 = local_244;
                  if (local_208 < 0x100) {
                    iVar15 = (int)ROUND(local_244);
                    local_21c = iVar15;
                    local_1e8 = local_244;
                  }
                }
                local_25c = (float)(int)sVar18 * local_260;
                local_224 = (int)ROUND(local_25c);
                if (local_224 < 0) {
                  iVar11 = 0;
                }
                else {
                  local_1f4 = (int)ROUND(local_25c);
                  iVar11 = 0xff;
                  local_214 = local_25c;
                  if (local_1f4 < 0x100) {
                    iVar11 = (int)ROUND(local_25c);
                    local_20c = local_25c;
                    local_1ec = iVar11;
                  }
                }
                local_38 = local_3c;
                FUN_004abbc0(iVar14,iVar11,iVar15,uVar17,local_238,local_260);
                ppuVar20 = ppuVar20 + 1;
                local_28c = local_28c + 1;
              } while (local_28c <= DAT_00692fcc);
            }
          }
          if (*(int *)(iVar14 + 0xbc) != -1) {
            FUN_004ac6f0(local_264,iVar14,iVar14 + 0x8c,uVar17,pfVar21[4]);
          }
        }
        if ((*(uint *)(DAT_0071942c + 0x58) & 1) == 0) {
          if (((*(ushort *)(iVar16 + 0x106) & 8) == 0) &&
             ((*(uint *)(DAT_0071942c + 0x58) & 2) == 0)) {
            uVar17 = 0;
          }
          else {
            uVar17 = 1;
          }
          if ((*(ushort *)(iVar16 + 0x106) & 4) != 0) {
            uVar17 = uVar17 | 2;
          }
          if (1 < *(short *)(DAT_0087a478 + 0xc)) {
            uVar17 = uVar17 | 4;
          }
          if (uVar19 == 0) {
            if ((uVar17 & 1) == 0) {
              pfVar21[5] = -NAN;
            }
            else if (pfVar21[5] == -NAN) {
              pfVar21[5] = *(float *)(DAT_006f1d6c + 0xc);
            }
          }
          if (*(int *)(iVar14 + 0x214) != -1) {
            sVar18 = *(short *)(iVar14 + 0x22e);
            if (sVar18 == 0) {
              sVar18 = 8;
            }
            puVar9 = (undefined4 *)(iVar14 + 0x1e4);
            puVar24 = local_168;
            for (iVar15 = 0x1a; iVar15 != 0; iVar15 = iVar15 + -1) {
              *puVar24 = *puVar9;
              puVar9 = puVar9 + 1;
              puVar24 = puVar24 + 1;
            }
            if (*(float *)(iVar16 + 0xe0) < *(float *)(iVar14 + 0x250)) {
              if (*(float *)(iVar16 + 0xe0) < *(float *)(iVar14 + 0x254) ==
                  (*(float *)(iVar16 + 0xe0) == *(float *)(iVar14 + 0x254))) {
                local_130 = *(undefined4 *)(iVar14 + 0x24c);
              }
              else {
                local_130 = local_134;
              }
            }
            fVar4 = (float)(int)sVar18;
            local_134 = local_130;
            iVar15 = FUN_004ab590(fVar4 * *(float *)(iVar16 + 0xe0));
            if (iVar15 < 0) {
              uVar25 = 0;
            }
            else {
              iVar15 = FUN_004ab590(fVar4 * *(float *)(iVar16 + 0xe0));
              if (iVar15 < 0x100) {
                uVar25 = FUN_004ab590(fVar4 * *(float *)(iVar16 + 0xe0));
              }
              else {
                uVar25 = 0xff;
              }
            }
            iVar15 = FUN_004ab590(fVar4 * *(float *)(iVar16 + 0xe0));
            if (iVar15 < 0) {
              uVar12 = 0;
            }
            else {
              iVar15 = FUN_004ab590(fVar4 * *(float *)(iVar16 + 0xe0));
              if (iVar15 < 0x100) {
                uVar12 = FUN_004ab590(fVar4 * *(float *)(iVar16 + 0xe0));
              }
              else {
                uVar12 = 0xff;
              }
            }
            FUN_004abbc0(iVar14,uVar12,uVar25,uVar17,0xbf800000,*(undefined4 *)(iVar16 + 0xe0));
          }
          if (*(int *)(iVar14 + 0x1ac) != -1) {
            FUN_004ac6f0(local_264,iVar14,iVar14 + 0x17c,uVar17,pfVar21[5]);
          }
          pfVar21[1] = *(float *)(iVar16 + 0xe0);
        }
        if (((uVar19 == 0) && ((*(byte *)(DAT_0071942c + 0x58) & 0x10) == 0)) &&
           (cVar6 = FUN_004635e0(), uVar17 = local_264, cVar6 != '\0')) {
          local_19c[0] = 2;
          bVar7 = (*(short *)(DAT_0087a478 + 0xc) < 2) - 1U & 4;
          if ((*(byte *)(DAT_0071942c + 0x58) & 0x20) != 0) {
            bVar7 = bVar7 | 1;
          }
          if ((bVar7 & 1) == 0) {
            pfVar21[6] = -NAN;
          }
          else if (pfVar21[6] == -NAN) {
            pfVar21[6] = *(float *)(DAT_006f1d6c + 0xc);
          }
          if (*(int *)(iVar14 + 0x29c) != -1) {
            FUN_004ac6f0(local_264,local_19c,iVar14 + 0x26c,bVar7,0xffffffff);
          }
          if (*(int *)(iVar14 + 0x304) != -1) {
            FUN_004ac6f0(uVar17,local_19c,iVar14 + 0x2d4,bVar7,0xffffffff);
          }
          iVar15 = DAT_0087a478;
          FUN_004ab690(local_19c,0,local_1e4);
          sVar18 = *(short *)(iVar15 + 0xc);
          if ((short)uVar17 != -1) {
            motion_sensor_update_for_player(uVar17);
            motion_sensor_render(1 < sVar18);
          }
        }
        iVar15 = 0;
        if ((DAT_006f1d20 == 0) || (DAT_006f1cbc == '\0')) {
          uVar17 = 0;
        }
        else {
          uVar17 = 1;
        }
        sVar18 = *(short *)(DAT_0087a478 + 0xc);
        sVar8 = 0;
        if (0 < *(int *)(iVar14 + 0x3a4)) {
          do {
            iVar15 = iVar15 * 0x84 + *(int *)(iVar14 + 0x3a8);
            if ((uVar17 & 1 << (*(byte *)(iVar15 + 0x68) & 0x1f)) != 0) {
              if ((*(byte *)(iVar15 + 0x6a) & 1) != 0) {
                uVar10 = color_rgb_float_to_int((float *)(iVar16 + 0x188));
                *(uint *)(iVar15 + 0x34) = uVar10 | 0xff000000;
              }
              FUN_004ac6f0(local_264,iVar14 + 0x380,iVar15,(sVar18 < 2) - 1U & 4,0xffffffff);
            }
            sVar8 = sVar8 + 1;
            iVar15 = (int)sVar8;
          } while (iVar15 < *(int *)(iVar14 + 0x3a4));
        }
        iVar16 = 0;
        sVar18 = 0;
        if (0 < *(int *)(iVar14 + 0x3cc)) {
          do {
            sVar8 = *(short *)(iVar16 * 0x144 + *(int *)(iVar14 + 0x3d0));
            psVar22 = (short *)(iVar16 * 0x144 + *(int *)(iVar14 + 0x3d0));
            uVar17 = 1 << ((byte)sVar8 & 0x1f);
            if (((uVar17 & *(ushort *)(pfVar21 + 8)) != 0) && ((local_26c & uVar17) == 0)) {
              *(undefined2 *)((int)pfVar21 + sVar8 * 2 + 0x22) = 0xffff;
            }
            iVar16 = (int)*psVar22;
            uVar17 = 1 << ((byte)*psVar22 & 0x1f);
            if ((local_26c & uVar17) == 0) {
              if ((local_250 & uVar17) == 0) {
                if (*(short *)((int)pfVar21 + iVar16 * 2 + 0x22) != -1) {
                  local_218 = (int)ROUND(*(float *)(psVar22 + 0x28) * 30.0);
                  iVar16 = (int)*psVar22;
                  if ((int)*(short *)((int)pfVar21 + iVar16 * 2 + 0x22) <
                      (int)ROUND(*(float *)(psVar22 + 0x28) * 30.0)) goto LAB_004b1117;
                }
                *(undefined2 *)((int)pfVar21 + iVar16 * 2 + 0x22) = 0xffff;
              }
              else {
LAB_004b1117:
                iVar15 = *(int *)(psVar22 + 0x22);
                sVar8 = *(short *)(DAT_0087a478 + 0xc);
                psVar1 = (short *)((int)pfVar21 + iVar16 * 2 + 0x22);
                *psVar1 = *psVar1 + *(short *)(DAT_006f1d6c + 0x10);
                if (iVar15 != -1) {
                  FUN_004ac6f0(local_264,iVar14,psVar22 + 10,(sVar8 < 2) - 1U & 4 | 1,
                               *(int *)(DAT_006f1d6c + 0xc) -
                               (int)*(short *)((int)pfVar21 + *psVar22 * 2 + 0x22));
                }
              }
            }
            else {
              local_25c = *(float *)(psVar22 + 0x22);
              local_244 = *(float *)(psVar22 + 0x56);
              bVar7 = (*(short *)(DAT_0087a478 + 0xc) < 2) - 1U & 4;
              if ((&local_268)[iVar16] < *(float *)(psVar22 + 0x72) !=
                  ((&local_268)[iVar16] == *(float *)(psVar22 + 0x72))) {
                bVar7 = bVar7 | 1;
              }
              psVar1 = (short *)((int)pfVar21 + iVar16 * 2 + 0x22);
              *psVar1 = *psVar1 + *(short *)(DAT_006f1d6c + 0x10);
              local_1dc = (int)ROUND(*(float *)(psVar22 + 0x28) * 30.0);
              *(short *)((int)pfVar21 + *psVar22 * 2 + 0x22) =
                   (short)((int)*(short *)((int)pfVar21 + *psVar22 * 2 + 0x22) %
                          ((int)ROUND(*(float *)(psVar22 + 0x28) * 30.0) * 2));
              if (local_25c != -NAN) {
                FUN_004ac6f0(local_264,iVar14,psVar22 + 10,bVar7,
                             *(int *)(DAT_006f1d6c + 0xc) -
                             (int)*(short *)((int)pfVar21 + *psVar22 * 2 + 0x22));
              }
              if (local_244 != -NAN) {
                fVar4 = (float)(int)psVar22[99];
                local_1f8 = (int)ROUND(fVar4 * (&local_268)[*psVar22]);
                if ((int)ROUND(fVar4 * (&local_268)[*psVar22]) < 0) {
                  iVar16 = 0;
                }
                else {
                  local_1d8 = (int)ROUND(fVar4 * (&local_268)[*psVar22]);
                  iVar16 = 0xff;
                  if ((int)ROUND(fVar4 * (&local_268)[*psVar22]) < 0x100) {
                    iVar16 = (int)ROUND(fVar4 * (&local_268)[*psVar22]);
                    local_1f0 = iVar16;
                  }
                }
                local_1e0 = (int)ROUND(fVar4 * (&local_268)[*psVar22]);
                if ((int)ROUND(fVar4 * (&local_268)[*psVar22]) < 0) {
                  iVar15 = 0;
                }
                else {
                  local_200 = (int)ROUND(fVar4 * (&local_268)[*psVar22]);
                  iVar15 = 0xff;
                  if ((int)ROUND(fVar4 * (&local_268)[*psVar22]) < 0x100) {
                    iVar15 = (int)ROUND(fVar4 * (&local_268)[*psVar22]);
                    local_220 = iVar15;
                  }
                }
                FUN_004abbc0(iVar14,iVar15,iVar16,bVar7,0xbf800000,(&local_268)[*psVar22]);
              }
            }
            sVar18 = sVar18 + 1;
            iVar16 = (int)sVar18;
          } while (iVar16 < *(int *)(iVar14 + 0x3cc));
        }
        bVar26 = DAT_006f1d20 != 0;
        *(undefined2 *)(pfVar21 + 8) = (undefined2)local_26c;
        if (bVar26) {
          if (DAT_006f1cbc == '\0') {
            pfVar13 = (float *)game_engine_get_player_color();
            local_1d4 = *pfVar13;
            local_1d0 = pfVar13[1];
            local_1cc = pfVar13[2];
            local_210 = 0x437f0000;
            local_24c = (int)ROUND(local_1cc * 255.0) & 0xffU |
                        ((int)ROUND(local_1d0 * 255.0) & 0xffU) << 8 |
                        ((int)ROUND(local_1d4 * 255.0) & 0xffU) << 0x10;
            uVar17 = local_24c | 0xff000000;
          }
          else {
            uVar17 = (-(uint)(*(int *)(param_1 + 0x20) != 0) & 0xff0401e3) + 0xb0fe0000;
          }
          switch(DAT_006f1cb8) {
          case 1:
            DAT_00692fc4 = tag_lookup("ui\\shell\\bitmaps\\team_icon_ctf");
            local_258 = 0x3f0ccccd;
            local_254 = 0x3f0ccccd;
            break;
          case 2:
            DAT_00692fc4 = tag_lookup("ui\\shell\\bitmaps\\team_icon_slayer");
            local_258 = 0x3f000000;
            local_254 = 0x3f000000;
            break;
          case 3:
            DAT_00692fc4 = tag_lookup("ui\\shell\\bitmaps\\team_icon_oddball");
            local_258 = 0x3f0ccccd;
            local_254 = 0x3f0ccccd;
            break;
          case 4:
            DAT_00692fc4 = tag_lookup("ui\\shell\\bitmaps\\team_icon_king");
            local_258 = 0x3f07ae14;
            local_254 = 0x3f07ae14;
            break;
          case 5:
            DAT_00692fc4 = tag_lookup("ui\\shell\\bitmaps\\team_icon_race");
            local_258 = 0x3f19999a;
            local_254 = 0x3f19999a;
          }
          iVar16 = bitmap_group_sequence_get_bitmap_data(0);
          _DAT_00692fc8 = tag_lookup("ui\\shell\\bitmaps\\team_background");
          iVar14 = bitmap_group_sequence_get_bitmap_data(0);
          if ((iVar16 != 0) && (iVar14 != 0)) {
            local_1b8 = 0;
            local_1c0 = 0;
            local_1b4 = 0x3f800000;
            local_1bc = 0x3f800000;
            local_248 = 0x601bd;
            local_1c8 = 0x3f19999a;
            local_1c4 = 0x3f23d70a;
            local_234 = 0;
            local_22c = 0;
            local_230 = 0x43000000;
            local_228 = 0x42800000;
            FUN_004acd50(0,iVar14,&local_1c0,&local_234,0,0xffffffff);
            local_234 = 0;
            local_22c = 0;
            local_230 = 0x42800000;
            local_228 = 0x42800000;
            FUN_004acd50(0,iVar16,&local_1c0,&local_234,0,uVar17);
          }
        }
      }
    }
  }
  return;
}
#endif
