#include "halo/game/gamerest_hud.hpp"
#include <string.h>
#include <wchar.h>
#include "halo/cache/api.hpp"
#include "halo/text/api.hpp"
#include "halo/rasterizer/api.hpp"

extern "C" {
extern data_array *player_data;
extern datum_index hud_find_nearby_teammate_for_nameplate(datum_index player_handle);
extern void hud_draw_teammate_nameplate_text(wchar_t *text, int32_t value);
extern double pow(double base, double exponent);
extern Globals *global_globals;
extern int32_t hud_text_draw_font_tag_id;
extern uint16_t hud_text_draw_color_or_flags;
extern float hud_text_draw_color_a;
extern float hud_text_draw_color_r;
extern float hud_text_draw_color_g;
extern float hud_text_draw_color_b;
extern void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override, uint32_t position_or_color1, uint32_t position_or_color2, const int16_t *text);
extern player_control_globals *player_control_globals_ptr;
extern data_array *object_data;
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void unit_get_camera_position(datum_index unit_index, real_point3d *out);
extern void game_engine_compute_local_player_look_vector(real_vector3d *out_forward, int16_t local_player_index);
extern int32_t object_collect_local_player_relevant_objects(real_point3d *point, uint8_t (*filter)(uint32_t, void *), void *filter_context, int32_t max_count, datum_index *out);
extern uint32_t camera_observer_target_direction(real_point3d *candidate_point, real_vector3d *facing, real_point3d *reference_position, datum_index object, datum_index exclude_object, real_vector3d *out_direction, real *out_distance, real *out_angle);
extern datum_index player_index_from_unit_index(datum_index unit_index);
extern uint8_t hud_nameplate_candidate_filter(uint32_t object_index, void *player_handle);
extern int16_t current_local_player_index;
extern uint8_t local_player_hud_status_table[];
extern float game_engine_nameplate_fade_opacity_array[];
extern player_globals *local_player_globals;
extern game_engine_definition *current_game_engine;
extern game_engine_state game_engine_state_value;
extern void hud_draw_teammate_nameplate(datum_index player_handle);
extern void game_engine_rasterize_in_game_score(datum_index subject_player, float opacity);
extern uint32_t render_viewport_top;
extern uint32_t screen_safe_area_right;
extern uint32_t screen_safe_area_bottom;
extern uint32_t text_tab_stops;
extern uint32_t hud_text_draw_box_field_474e;
extern uint32_t hud_text_draw_tabstop_c;
extern int16_t hud_text_draw_box_field_4756;
extern uint8_t debug_print_enabled_flag;
extern int32_t game_engine_build_sorted_player_list(uint8_t invert_low_stat, scoreboard_entry entries[16], int32_t mode);
extern void console_printf_verbose(const char *format, ...);
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints];
extern uint8_t game_engine_ctf_unit_is_flag_holder(player *p);
extern int16_t hud_waypoint_arrow_find(const char *name);
extern int16_t network_game_mode;
extern uint8_t game_engine_build_kill_feed_message_text(datum_index recipient, wchar_t *out, uint32_t message_type, datum_index subject, size_t buffer_size);
extern void game_engine_notify_kill_event(uint32_t player_index, int32_t hash_key, int32_t message_type, datum_index subject);
extern void chimera__multiplayer_message(wchar_t *text);
extern void chimera__hud_message(int16_t local_player_index, wchar_t *text);
extern wchar_t *unicode_string_list_get_string(char *path, int16_t index);
extern void ticker_text_buffer_append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self);
extern void server_browser_custom_options_unpack(char *text, server_browser_custom_options *out);
extern void server_browser_gametype1_flags_unpack(uint32_t code, server_browser_gametype1_decoded *out);
extern void server_browser_gametype3_flags_unpack(uint32_t code, server_browser_gametype3_options *out);
extern void server_browser_gametype5_flags_unpack(uint32_t code, int32_t *out);
extern wchar_t ticker_field_separator[];
extern wchar_t missing_string_text[];
extern wchar_t unicode_string_list_scratch_buffer;
}

namespace halo::game {

/**
 * blam-cc: EAX -> player
 * Advances `player`'s nameplate-target hysteresis counter (player+0x80, 0..15) toward whatever
 * hud_find_nearby_teammate_for_nameplate currently reports (ignoring a self-match), latching a
 * new tracked target (player+0x7c) only once the counter reaches either end. Draws the tracked
 * target's name when one is latched.
 *
 * @address 0x45e520
 */
void HudNameplates::draw_teammate_nameplate(datum_index player_handle)
{
    player *p;
    datum_index found;
    player *tracked;
    wchar_t name[12];

    p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    found = (datum_index)0xffffffff;

    if (p->local_player_index != -1 && p->unit != (datum_index)0xffffffff) {
        found = HudNameplates::find_nearby_teammate_for_nameplate(player_handle);
        if (found == player_handle) {
            found = (datum_index)0xffffffff;
        }
    }

    if ((datum_index)p->nameplate_target_player == found) {
        if (p->nameplate_fade_ticks < 0xf) {
            p->nameplate_fade_ticks = p->nameplate_fade_ticks + 1;
        }
    } else {
        if (0 < p->nameplate_fade_ticks) {
            p->nameplate_fade_ticks = p->nameplate_fade_ticks - 1;
        }
        if (p->nameplate_fade_ticks == 0) {
            p->nameplate_target_player = found;
        }
    }

    if (p->nameplate_target_player != (datum_index)0xffffffff) {
        int16_t index = (int16_t)p->nameplate_target_player;
        if (-1 < index && index < player_data->maximum_count) {
            tracked = (player *)((uint8_t *)player_data->data + player_data->size * index);
            if (tracked->identifier != 0 &&
                ((int16_t)((uint32_t)p->nameplate_target_player >> 16) == 0 ||
                 tracked->identifier == (int16_t)((uint32_t)p->nameplate_target_player >> 16))) {
                memset(name, 0, sizeof(name));
                wcsncpy(name, (const wchar_t *)tracked->name, 0x0b);
                name[0x0b] = 0;

                HudNameplates::draw_teammate_nameplate_text(name, (float)pow((double)((float)(p->nameplate_fade_ticks < 10 ? p->nameplate_fade_ticks : 10) * 0.1f), (double)1.9f) * 0.5f);
                return;
            }
        }
        p->nameplate_target_player = (datum_index)0xffffffff;
    }
}

/**
 * Draws `text` through the globals terminal font at a fixed screen position, with `value`
 * forwarded both into the shared draw state and as the rasterizer's own sixth argument. Resets
 * the background mode to none afterward.
 *
 * @address 0x461f20
 */
void HudNameplates::draw_teammate_nameplate_text(wchar_t *text, int32_t value)
{
    hud_text_bounds bounds;
    GlobalsInterfaceBitmaps *interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
        ? (GlobalsInterfaceBitmaps *)0
        : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;

    hud_text_draw_font_tag_id = (int32_t)interface_bitmaps->font_terminal.tag_id.index |
                                 ((int32_t)interface_bitmaps->font_terminal.tag_id.id << 16);
    halo::text::globals().hud_text_draw_unknown_4730 = 8;
    hud_text_draw_color_r = 0.45882353f;
    *(int32_t *)&hud_text_draw_color_a = value;
    hud_text_draw_color_b = 1.0f;
    hud_text_draw_color_or_flags = 0xffffu;
    halo::text::globals().hud_text_draw_column = 2;
    hud_text_draw_color_g = 0.7294118f;

    bounds.top = 0x46;
    bounds.bottom = 0x5e;
    bounds.right = 0x278;

    halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)&bounds, 0, 0, (const int16_t *)text);

    hud_text_draw_color_or_flags = 0xffffu;
    halo::text::globals().hud_text_draw_column = 0;
    halo::text::globals().hud_text_draw_unknown_4730 = 0;
    halo::text::globals().hud_text_draw_background_mode = 0;
}

/**
 * Finds the teammate biped the player is aiming at (within the 0.1308 aim cone and 20 world units) to show its
 * nameplate, preferring the player's cached nameplate target while its weight is positive. Returns the target's
 * player index, or -1.
 *
 * @address 0x45e340
 */
datum_index HudNameplates::find_nearby_teammate_for_nameplate(datum_index player_handle)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    datum_index best = (datum_index)0xffffffff;
    real_point3d camera;
    real_vector3d look;
    real_vector3d direction;
    real_point3d closest_point;
    real distance;
    real angle;
    datum_index candidates[32];
    int32_t candidate_count;
    int32_t i;

    if (p->local_player_index != -1) {
        local_player_control *track = &player_control_globals_ptr->local_players[p->local_player_index];

        if (track->nameplate_weight > 0.0f) {
            datum_index target = track->nameplate_target;

            best = object_try_and_get(target, 0xffffffff) != 0 ? target : (datum_index)0xffffffff;
            if (best != (datum_index)0xffffffff) {
                return player_index_from_unit_index(best);
            }
        }
    }

    unit_get_camera_position(p->unit, &camera);
    game_engine_compute_local_player_look_vector(&look, p->local_player_index);
    candidate_count = object_collect_local_player_relevant_objects(&camera, hud_nameplate_candidate_filter,
        &player_handle, 0x20, candidates);
    if (candidate_count <= 0) {
        return best;
    }

    for (i = 0; i < candidate_count; i++) {
        uint8_t *candidate = (uint8_t *)((object_header *)object_data->data)[candidates[i] & 0xffff].data;
        real dx = ((struct object *)candidate)->position.x - camera.x;
        real dy = ((struct object *)candidate)->position.y - camera.y;
        real dz = ((struct object *)candidate)->position.z - camera.z;
        real distance_squared = dz * dz + dx * dx + dy * dy;

        if (!(*(real *)(candidate + 0x37c) < 1.0f) &&
            p->nameplate_target_player != player_index_from_unit_index(candidates[i])) {
            continue;
        }
        if (camera_observer_target_direction(&closest_point, &look, &camera, candidates[i], p->unit, &direction,
                &distance, &angle) == 0) {
            continue;
        }
        if ((double)(angle < 0.0f ? -angle : angle) < 0.13083334267139435 && distance_squared < 400.0f && distance_squared < 900.0f) {
            best = candidates[i];
        }
    }

    if (best == (datum_index)0xffffffff) {
        return best;
    }
    return player_index_from_unit_index(best);
}

/**
 * Implements the original `hud_nameplate_candidate_filter`.
 *
 * @address 0x45e2e0
 */
uint8_t HudNameplates::nameplate_candidate_filter(uint32_t object_index, void *context)
{
    datum_index player_handle = *(datum_index *)context;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;

    if ((obj[0x10] & 1) != 0) {
        return 0;
    }
    if (((1u << obj[0xb4]) & 1) == 0) {
        return 0;
    }
    if ((obj[0x106] & 4) != 0) {
        return 0;
    }
    return player_index_from_unit_index(object_index) != player_handle ? 1 : 0;
}

/**
 * Fades the teammate-nameplate HUD element in or out for the current local player, then, once
 * it is at least partly visible, draws the in-game scoreboard line for that player at the
 * resulting opacity.
 *
 * @address 0x45f220
 */
void HudNameplates::update_teammate_nameplate_fade()
{
    int16_t local_player = current_local_player_index;
    datum_index player_handle;
    float opacity;

    if (local_player == -1 || 0 < local_player) {
        player_handle = (datum_index)0xffffffff;
    } else {
        player_handle = ((datum_index *)((uint8_t *)local_player_globals + 4))[local_player];
    }

    if (current_game_engine != 0 &&
        (player_handle & 0xffff) * sizeof(player) + (uint32_t)player_data->data != 0) {
        HudNameplates::draw_teammate_nameplate(player_handle);
    }

    if (local_player_hud_status_table[local_player * 0x28] == 0 && game_engine_state_value != 1) {
        opacity = game_engine_nameplate_fade_opacity_array[local_player] - 0.06666667f;
    } else {
        opacity = game_engine_nameplate_fade_opacity_array[local_player] + 0.06666667f;
    }

    if (opacity < 0.0f) {
        game_engine_nameplate_fade_opacity_array[local_player] = 0.0f;
        return;
    }

    if (opacity <= 1.0f) {
        if (opacity <= 0.0f) {
            game_engine_nameplate_fade_opacity_array[local_player] = opacity;
            return;
        }
    } else {
        opacity = 1.0f;
    }

    game_engine_rasterize_in_game_score(player_handle, (float)pow((double)opacity, (double)1.9f));
    game_engine_nameplate_fade_opacity_array[local_player] = opacity;
}

/**
 * blam-cc: CX -> row, stack -> text, column (original stack order)
 * Draws `text` right-aligned near the top of the screen, `row` line-heights (0x12 px) down from
 * a 0x1a px top margin, using the shared 16-bit text drawing state (`column` selects which of
 * several preset color/size slots that state holds -- see the UNSURE above).
 *
 * @address 0x45d670
 */
void HudText::scoreboard_row_text(int16_t row, wchar_t *text, int16_t column)
{
    int16_t safe_left;
    hud_text_bounds bounds;

    hud_text_draw_color_or_flags = 0xffffu;
    halo::text::globals().hud_text_draw_column = column;
    halo::text::globals().hud_text_draw_unknown_4730 = 0;

    safe_left = (int16_t)(render_viewport_top >> 16);
    bounds.top = (int16_t)(row * 0x12);
    bounds.left = (int16_t)((screen_safe_area_right >> 16) - (uint16_t)safe_left);
    bounds.bottom = (int16_t)(row * 0x12 + 0x1a);
    bounds.right = (int16_t)((screen_safe_area_bottom >> 16) - (uint16_t)safe_left);

    halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)&bounds, 0, 0, (const int16_t *)text);
}

/**
 * blam-cc: EAX -> params, EDX -> row, stack -> (text, highlighted)
 * Draws one row of world-relative HUD text through the globals terminal font, with the fixed
 * background box armed only when `row` is non-zero, and with its colour pushed toward white when
 * `highlighted` is set. Does nothing (beyond clearing the background mode) if the globals tag has
 * no font_terminal assigned.
 *
 * @address 0x4653f0
 */
int32_t HudText::world_relative_text(hud_world_text_params *params, int16_t row, wchar_t *text, uint8_t highlighted)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    int32_t font_terminal_id;
    real r, g, b;
    int16_t safe_left;
    hud_text_bounds bounds;
    int32_t result;

    r = params->red;
    g = params->green;
    b = params->blue;

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    font_terminal_id = (int32_t)interface_bitmaps->font_terminal.tag_id.index |
                       ((int32_t)interface_bitmaps->font_terminal.tag_id.id << 16);

    if (row != 0) {

        text_tab_stops = 0x005a0019u;
        hud_text_draw_box_field_474e = 0x01590118u;
        halo::text::globals().hud_text_draw_background_mode = 7;
        hud_text_draw_tabstop_c = 0x01e5019au;
        hud_text_draw_box_field_4756 = 0x230;
    } else {
        halo::text::globals().hud_text_draw_background_mode = 0;
    }

    safe_left = (int16_t)(render_viewport_top >> 16);
    bounds.left = (int16_t)((screen_safe_area_right >> 16) - (uint16_t)safe_left);
    bounds.right = (int16_t)((screen_safe_area_bottom >> 16) - (uint16_t)safe_left);
    bounds.top = (int16_t)(row * 0x0f + 0x3b);
    bounds.bottom = (int16_t)(bounds.top + 0x11);

    result = -(int32_t)render_viewport_top;
    if (font_terminal_id != -1) {
        if (highlighted != 0) {
            r = r + 0.4f;
            g = g + 0.4f;
            b = b + 0.4f;
            if (r > 1.0f) r = 1.0f;
            if (g > 1.0f) g = 1.0f;
            if (b > 1.0f) b = 1.0f;
        }
        hud_text_draw_color_r = r;
        hud_text_draw_color_g = g;
        hud_text_draw_color_b = b;
        hud_text_draw_color_or_flags = 0xffffu;
        halo::text::globals().hud_text_draw_column = 0;
        halo::text::globals().hud_text_draw_unknown_4730 = 0;
        hud_text_draw_font_tag_id = font_terminal_id;
        hud_text_draw_color_a = params->alpha;

        halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)&bounds, 0, 0, (const int16_t *)text);
    }

    halo::text::globals().hud_text_draw_background_mode = 0;
    return result;
}

/**
 * blam-cc: EBX -> max_count, stack -> out
 * Writes at most `max_count` scoreboard_entry records to `out`, taken from the sorted scoreboard
 * list, but guaranteed to include every local (split-screen) player even if their rank would
 * otherwise put them past the cutoff.
 *
 * @address 0x45d4a0
 */
int32_t Scoreboard::select_players_to_display(int32_t mode, int32_t max_count, scoreboard_entry *out)
{
    scoreboard_entry entries[16];
    int32_t total = game_engine_build_sorted_player_list(0, entries, mode);
    uint8_t debug = (debug_print_enabled_flag == 0x45);

    if (debug) {
        console_printf_verbose("player_count=%d, maxcount=%d", total, max_count);
    }

    if (max_count < total) {
        scoreboard_entry rescued[3];
        int32_t rescued_count = 0;
        int32_t remaining_to_scan = 0;

        {
            scoreboard_entry *scan = &entries[max_count];
            int32_t remaining = total - max_count;

            do {
                remaining_to_scan = remaining;
                player *candidate = (player *)((uint8_t *)player_data->data +
                    (scan->player & 0xffff) * sizeof(player));

                if (candidate != 0 && candidate->local_player_index != -1) {
                    if (debug) {
                        console_printf_verbose("found local player");
                    }
                    rescued[rescued_count] = *scan;
                    rescued_count = rescued_count + 1;
                }
                scan = scan + 1;
                remaining = remaining_to_scan - 1;
                remaining_to_scan = rescued_count;
            } while (remaining != 0);
        }

        if (0 < remaining_to_scan) {
            scoreboard_entry *rescue_src = rescued;
            do {
                int32_t slot = max_count - 1;

                if (-1 < slot) {
                    scoreboard_entry *victim = &entries[slot];

                    while (((player *)((uint8_t *)player_data->data +
                                (victim->player & 0xffff) * sizeof(player)))->local_player_index != -1) {
                        slot = slot - 1;
                        victim = victim - 1;
                        if (slot < 0) {
                            goto next_rescue;
                        }
                    }

                    memmove(&entries[slot], &entries[slot + 1],
                        (size_t)((max_count - slot) * sizeof(scoreboard_entry) - sizeof(scoreboard_entry)));
                    entries[max_count - 1] = *rescue_src;
                }

            next_rescue:
                rescue_src = rescue_src + 1;
                remaining_to_scan = remaining_to_scan - 1;
            } while (remaining_to_scan != 0);
        }
    }

    if (max_count <= total) {
        total = max_count;
    }
    {
        int32_t i;
        for (i = 0; i < total; i++) {
            out[i] = entries[i];
        }
    }

    return total;
}

/**
 * Implements the original `scoreboard_entry_compare`.
 *
 * @address 0x45cbe0
 */
int32_t Scoreboard::compare(const scoreboard_entry *a, const scoreboard_entry *b)
{
    if (b->key_0 < a->key_0) {
        return -1;
    }
    if (b->key_0 <= a->key_0) {
        if (b->key_1 < a->key_1) {
            return -1;
        }
        if (b->key_1 <= a->key_1) {
            if (a->key_2 <= b->key_2) {
                if (a->key_2 < b->key_2) {
                    return -1;
                }
                if (b->key_3 < a->key_3) {
                    return -1;
                }
                if (b->key_3 <= a->key_3) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

/**
 * Implements the original `scoreboard_entry_compare_by_unknown_04`.
 *
 * @address 0x45cbc0
 */
uint32_t Scoreboard::compare_by_unknown_04(const scoreboard_entry *a, const scoreboard_entry *b)
{
    if (b->single_sort_key < a->single_sort_key) {
        return 0xffffffff;
    }
    return (uint32_t)(a->single_sort_key < b->single_sort_key);
}

/**
 * blam-cc: EAX -> out, CX -> slot
 *
 * @address 0x462230
 */
void CustomWaypoints::get_position(real_point3d *out, int16_t slot)
{
    out->x = custom_waypoints[slot].position.x;
    out->y = custom_waypoints[slot].position.y;
    out->z = custom_waypoints[slot].position.z;
}

/**
 * blam-cc: EAX -> reference_player, EDI -> slot_index, stack -> candidate
 *
 * @address 0x4620c0
 */
uint8_t CustomWaypoints::matches_filter(int32_t candidate, player *reference_player, int32_t slot_index)
{
    custom_waypoint *slot = &custom_waypoints[slot_index];

    if (current_game_engine == 0) {
        return 0;
    }

    if (current_game_engine->index == _game_engine_ctf && slot->active != 0) {
        if ((slot->player == (datum_index)0xffffffff || candidate == (int32_t)slot->player) &&
            (slot->team == -1 || reference_player->team == (int32_t)slot->team)) {
            if (slot->owner == (datum_index)0xffffffff || candidate != (int32_t)slot->owner) {
                return 1;
            }
        }
        if (reference_player->team == (int32_t)slot->team) {
            return 0;
        }
        if (slot->owner == (datum_index)0xffffffff) {
            return 0;
        }
        return game_engine_ctf_unit_is_flag_holder(reference_player) != 0;
    }

    if (current_game_engine->waypoint_filter != 0) {
        if (slot->active == 0) {
            return 0;
        }
        return ((uint8_t (*)(int32_t, int32_t))current_game_engine->waypoint_filter)(candidate, slot_index);
    }
    if (slot->active == 0) {
        return 0;
    }
    if (slot->player != (datum_index)0xffffffff && candidate != (int32_t)slot->player) {
        return 0;
    }
    if (slot->team != -1 && reference_player->team != (int32_t)slot->team) {
        return 0;
    }
    if (slot->owner == (datum_index)0xffffffff) {
        return 1;
    }
    return candidate != (int32_t)slot->owner;
}

/**
 * blam-cc: EAX -> owner, CX -> slot, EBX -> position, EDI -> icon_name, stack ->
 *          height_offset, player_filter, team_filter
 *
 * @address 0x462260
 */
void CustomWaypoints::custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position, const char *icon_name, float height_offset, datum_index player_filter, int16_t team_filter)
{
    custom_waypoint *w = &custom_waypoints[slot];

    w->owner = owner;
    w->icon = hud_waypoint_arrow_find(icon_name);
    w->active = 1;
    w->position.x = position->x;
    w->position.y = position->y;
    w->position.z = position->z;
    w->team = team_filter;
    w->player = player_filter;
    w->position.z = height_offset + w->position.z + 0.63f;
}

/**
 * blam-cc: unaff_EDI -> recipient, stack -> hash_key, message_type, subject, broadcast
 * Validates `recipient` against the live players array (index range, occupied slot, optional
 * salt match), and -- only for a recipient that is a local player -- builds the message text
 * (variant override first, default builder second) and routes it to either the multiplayer chat
 * line or the HUD message line depending on `message_type`. Independently of the local-player
 * check, when hosting and `broadcast` is set, forwards the event to other machines via
 * KillFeed::notify_kill_event.
 *
 * @address 0x460a30
 */
void ChimeraHooks::kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type, datum_index subject, char broadcast)
{
    int16_t index = (int16_t)recipient;

    if (recipient == (datum_index)0xffffffff || index < 0 || player_data->maximum_count <= index) {
        return;
    }
    {
        player *p = (player *)((uint8_t *)player_data->data + player_data->size * index);
        int16_t salt;

        if (p->identifier == 0) {
            return;
        }
        salt = (int16_t)((uint32_t)recipient >> 16);
        if (salt != 0 && p->identifier != salt) {
            return;
        }

        if (p->local_player_index != -1) {
            wchar_t message[1024];
            char built = 0;

            if (current_game_engine->build_message_text != 0) {
                built = ((char (*)(int32_t, uint32_t, datum_index, wchar_t *, size_t))
                    current_game_engine->build_message_text)
                    (hash_key, message_type, subject, message, 0x400);
            }
            if (built == 0) {
                built = game_engine_build_kill_feed_message_text(hash_key, message, message_type, subject, 0x400);
            }
            if (built != 0) {
                message[1023] = 0;
                switch (message_type) {
                case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 8:
                case 0x0d: case 0x13: case 0x1b: case 0x1c:
                    chimera__multiplayer_message(message);
                    break;
                default:
                    chimera__hud_message(p->local_player_index, message);
                    break;
                }
            }
        }

        if (network_game_mode == 2 && broadcast == 1) {

            game_engine_notify_kill_event(recipient, hash_key, (int32_t)message_type, subject);
        }
    }
}

/**
 * blam-cc: EDX -> variant_name; ticker, fraglimit, game_flags_wide, player_flags_wide are
 * ordinary cdecl stack parameters (this function's disassembly: param_1=ticker, param_2=fraglimit,
 * param_3=game_flags_wide, param_4=player_flags_wide).
 * Builds the multiline server-browser ticker text describing one multiplayer game variant.
 * `fraglimit` is GameSpy's packed "fraglimit" key: bits 0-2 select the game engine (1=ctf,
 * 2=slayer, 3=oddball, 4=king, 5=race; anything else shows nothing) and the remaining bits are a
 * handful of per-engine boolean/enum extras tested directly against `fraglimit` in the ctf/
 * slayer/king branches. `variant_name` is GameSpy's packed "gamevariant" key: despite the name it
 * is *not* shown as text here -- it is parsed as "%d,%d" into the bulk of the option fields.
 *
 * @address 0x4b8da0
 */
void VariantDescription::generate(char *variant_name, ticker_text_buffer *ticker, int32_t fraglimit, wchar_t *game_flags_wide, wchar_t *player_flags_wide)
{
    uint32_t engine_index = (uint32_t)fraglimit & 7;
    int is_custom_variant = 0;
    wchar_t line[256];
    wchar_t *label_text;
    wchar_t *suffix_text;

    union {
        server_browser_gametype1_decoded ctf;
        int32_t race[2];
    } engine_extra;

    server_browser_custom_options options;

    server_browser_gametype3_options oddball;

    server_browser_custom_options_unpack(variant_name, &options);
    ticker_text_buffer_append((wchar_t *)L"  ---  ", 0, ticker);

    if (engine_index == 1) {
        server_browser_gametype1_flags_unpack((uint32_t)fraglimit, &engine_extra.ctf);
        is_custom_variant = 1;
        if (game_flags_wide == 0 || game_flags_wide[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 24);
            swprintf(line, 0xff, L"(%s)", label_text);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 24);
            swprintf(line, 0xff, L"(%s - \"%s\")", label_text, game_flags_wide);
        }
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        if (options.teams != 0) {

            datum_index tag_id = halo::cache::tag_lookup(0x75737472,
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings");
            wchar_t *rules_text = missing_string_text;
            if (tag_id != k_datum_index_none) {
                UnicodeStringList *list = (UnicodeStringList *)halo::cache::globals().tag_instances[tag_id & 0xffff].data;
                if (list->strings.count > 0) {
                    UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer;
                    int32_t char_count = (int32_t)entry->string.size;
                    if (char_count > 0) {
                        rules_text = (wchar_t *)entry->string.pointer;
                        rules_text[(char_count / 2) - 1] = 0;
                    }
                }
            }
            wcscpy(&unicode_string_list_scratch_buffer, rules_text);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, &unicode_string_list_scratch_buffer);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        if (engine_extra.ctf.flags[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 25);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 26);
        }
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        if (engine_extra.ctf.flags[1] != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 27);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (engine_extra.ctf.flags[2] != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 28);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (engine_extra.ctf.flags[3] != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 29);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        {
            int32_t packed_low_high = *(int32_t *)&engine_extra.ctf.low;
            if (packed_low_high == 0x1518 || packed_low_high == 0x708 || packed_low_high == 0xe10 ||
                packed_low_high == 9000 || packed_low_high == 18000) {
                label_text = unicode_string_list_get_string(
                    (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 30);
                swprintf(line, 0xff, L"%s%s %d", ticker_field_separator, label_text, packed_low_high / 30);
                line[255] = 0;
                ticker_text_buffer_append(line, 0, ticker);
                is_custom_variant = 1;
            }
        }
    } else if (engine_index == 2) {
        if (game_flags_wide == 0 || game_flags_wide[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 31);
            swprintf(line, 0xff, L"(%s)", label_text);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 31);
            swprintf(line, 0xff, L"(%s - \"%s\")", label_text, game_flags_wide);
        }
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        if (options.teams != 0) {
            is_custom_variant = 1;
            datum_index tag_id = halo::cache::tag_lookup(0x75737472,
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings");
            wchar_t *rules_text = missing_string_text;
            if (tag_id != k_datum_index_none) {
                UnicodeStringList *list = (UnicodeStringList *)halo::cache::globals().tag_instances[tag_id & 0xffff].data;
                if (list->strings.count > 0) {
                    UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer;
                    int32_t char_count = (int32_t)entry->string.size;
                    if (char_count > 0) {
                        rules_text = (wchar_t *)entry->string.pointer;
                        rules_text[(char_count / 2) - 1] = 0;
                    }
                }
            }
            wcscpy(&unicode_string_list_scratch_buffer, rules_text);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, &unicode_string_list_scratch_buffer);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        if (((uint32_t)fraglimit >> 3 & 1) != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 32);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (((uint32_t)fraglimit >> 4 & 1) != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 33);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (((uint32_t)fraglimit >> 5 & 1) != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 34);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    } else if (engine_index == 3) {
        server_browser_gametype3_flags_unpack((uint32_t)fraglimit, &oddball);
        if (game_flags_wide == 0 || game_flags_wide[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 37);
            swprintf(line, 0xff, L"(%s)", label_text);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 37);
            swprintf(line, 0xff, L"(%s - \"%s\")", label_text, game_flags_wide);
        }
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        if (options.teams != 0) {
            is_custom_variant = 1;
            datum_index tag_id = halo::cache::tag_lookup(0x75737472,
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings");
            wchar_t *rules_text = missing_string_text;
            if (tag_id != k_datum_index_none) {
                UnicodeStringList *list = (UnicodeStringList *)halo::cache::globals().tag_instances[tag_id & 0xffff].data;
                if (list->strings.count > 0) {
                    UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer;
                    int32_t char_count = (int32_t)entry->string.size;
                    if (char_count > 0) {
                        rules_text = (wchar_t *)entry->string.pointer;
                        rules_text[(char_count / 2) - 1] = 0;
                    }
                }
            }
            wcscpy(&unicode_string_list_scratch_buffer, rules_text);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, &unicode_string_list_scratch_buffer);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        if (oddball.value_10 >= 0 && oddball.value_10 < 3) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\oddball_edit\\var_speed_with_ball",
                (int16_t)oddball.value_10);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (oddball.value_14 > 1) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 38);
            swprintf(line, 0xff, L"%s%s %d", ticker_field_separator, label_text, oddball.value_14);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (oddball.flag0 != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 39);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (oddball.value_10 == 0 || oddball.value_10 == 2) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 41);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);

            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\oddball_edit\\var_speed_with_ball",
                (int16_t)oddball.value_10);
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (oddball.value_08 > 0 && oddball.value_08 < 4) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 42);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\oddball_edit\\var_trait_with_ball",
                (int16_t)oddball.value_08);
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (oddball.value_0c > 0 && oddball.value_0c < 4) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 43);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\oddball_edit\\var_trait_with_ball",
                (int16_t)oddball.value_0c);
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    } else if (engine_index == 4) {
        if (game_flags_wide == 0 || game_flags_wide[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 35);
            swprintf(line, 0xff, L"(%s)", label_text);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 35);
            swprintf(line, 0xff, L"(%s - \"%s\")", label_text, game_flags_wide);
        }
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        if (options.teams != 0) {
            is_custom_variant = 1;
            datum_index tag_id = halo::cache::tag_lookup(0x75737472,
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings");
            wchar_t *rules_text = missing_string_text;
            if (tag_id != k_datum_index_none) {
                UnicodeStringList *list = (UnicodeStringList *)halo::cache::globals().tag_instances[tag_id & 0xffff].data;
                if (list->strings.count > 0) {
                    UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer;
                    int32_t char_count = (int32_t)entry->string.size;
                    if (char_count > 0) {
                        rules_text = (wchar_t *)entry->string.pointer;
                        rules_text[(char_count / 2) - 1] = 0;
                    }
                }
            }
            wcscpy(&unicode_string_list_scratch_buffer, rules_text);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, &unicode_string_list_scratch_buffer);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        if (((uint32_t)fraglimit >> 3 & 1) != 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 36);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    } else if (engine_index == 5) {
        server_browser_gametype5_flags_unpack((uint32_t)fraglimit, engine_extra.race);
        if (game_flags_wide == 0 || game_flags_wide[0] == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 44);
            swprintf(line, 0xff, L"(%s)", label_text);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 44);
            swprintf(line, 0xff, L"(%s - \"%s\")", label_text, game_flags_wide);
        }
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);

        if (options.teams != 0) {
            is_custom_variant = 1;
            datum_index tag_id = halo::cache::tag_lookup(0x75737472,
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings");
            wchar_t *rules_text = missing_string_text;
            if (tag_id != k_datum_index_none) {
                UnicodeStringList *list = (UnicodeStringList *)halo::cache::globals().tag_instances[tag_id & 0xffff].data;
                if (list->strings.count > 0) {
                    UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer;
                    int32_t char_count = (int32_t)entry->string.size;
                    if (char_count > 0) {
                        rules_text = (wchar_t *)entry->string.pointer;
                        rules_text[(char_count / 2) - 1] = 0;
                    }
                }
            }
            wcscpy(&unicode_string_list_scratch_buffer, rules_text);
            swprintf(line, 0xff, L"%s%s", ticker_field_separator, &unicode_string_list_scratch_buffer);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }

        if (engine_extra.race[0] >= 0 && engine_extra.race[0] < 3) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 45);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);

            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\race_edit\\var_race_type",
                (int16_t)engine_extra.race[0]);
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if (engine_extra.race[1] >= 0 && engine_extra.race[1] < 3) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 46);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\playlist_edit\\race_edit\\var_team_scoring",
                (int16_t)engine_extra.race[1]);
            wcscat(line, suffix_text);

            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    }

    ticker_text_buffer_append((wchar_t *)L"  ---  ", 0, ticker);
    label_text = unicode_string_list_get_string(
        (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 1);
    swprintf(line, 0xff, L" %s %s", label_text, player_flags_wide);
    line[255] = 0;
    ticker_text_buffer_append(line, 0, ticker);

    switch (options.lives_per_round) {
    case 0: case 1: case 3: case 5:
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 2);
        swprintf(line, 0xff, L"%s%s %d", ticker_field_separator, label_text, options.lives_per_round);
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);
        break;
    default:
        break;
    }

    {

        float speed_scale = *(float *)&options.health_bits;
        int32_t speed_percent = (int32_t)(speed_scale * 100.0f);
        if (speed_percent == 200 || speed_percent == 50 || speed_percent == 100 ||
            speed_percent == 150 || speed_percent == 300 || speed_percent == 400) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 3);
            swprintf(line, 0xff, L"%s%s %d", ticker_field_separator, label_text, speed_percent);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    }

    {

        int show_pair = 0;
        if (options.respawn_time == 0 || options.respawn_time == 0x96 ||
            options.respawn_time == 0x12c || options.respawn_time == 0x1c2) {
            show_pair = (options.respawn_time_growth == 0 || options.respawn_time_growth == 0x96 ||
                         options.respawn_time_growth == 0x12c || options.respawn_time_growth == 0x1c2);
        } else {
            line[0] = 0;
        }
        if (show_pair) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 4);
            swprintf(line, 0xff, L"%s%s %d+%d", ticker_field_separator, label_text,
                options.respawn_time / 30, options.respawn_time_growth / 30);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    }

    if (options.suicide_penalty == 0x96 || options.suicide_penalty == 300 || options.suicide_penalty == 0x1c2) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 5);
        swprintf(line, 0xff, L"%s%s %d", ticker_field_separator, label_text, options.suicide_penalty / 30);
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);
    }
    if ((options.flags & 8) == 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 6);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }
    if (options.odd_man_out != 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 7);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }
    if ((options.flags & 0x10) != 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 8);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }
    if ((options.flags & 4) != 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 9);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }

    if (is_custom_variant) {
        if (options.friendly_fire < 4) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 11);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);

            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\teamplay_options_edit\\var_friendly_fire",
                (int16_t)options.friendly_fire);
            wcscat(line, suffix_text);
            if ((options.friendly_fire == 1 || options.friendly_fire == 3) &&
                (options.betrayal_penalty == 0x96 || options.betrayal_penalty == 300 || options.betrayal_penalty == 0x1c2)) {

                int16_t penalty_display_index =
                    (options.betrayal_penalty == 0x96) ? 1 : (options.betrayal_penalty == 300) ? 2 : 3;
                wcscat(line, L" (+");
                suffix_text = unicode_string_list_get_string(
                    (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\teamplay_options_edit\\var_friendly_fire_penalty",
                    penalty_display_index);
                wcscat(line, suffix_text);
                wcscat(line, L")");
            }
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if ((options.red_vehicle_set & 0xf) < 9) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 12);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\vehicle_options_edit\\var_vehicle_set",
                (int16_t)(options.red_vehicle_set & 0xf));
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
        if ((options.blue_vehicle_set & 0xf) < 9) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 13);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\vehicle_options_edit\\var_vehicle_set",
                (int16_t)(options.blue_vehicle_set & 0xf));
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    } else if ((options.red_vehicle_set & 0xf) < 9) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 10);
        swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
        suffix_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\vehicle_options_edit\\var_vehicle_set",
            (int16_t)(options.red_vehicle_set & 0xf));
        wcscat(line, suffix_text);
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);
    }

    {

        int32_t vehicle_respawn_index = -1;
        switch (options.vehicle_respawn_time) {
        case 0:      vehicle_respawn_index = 0; break;
        case 0x384:  vehicle_respawn_index = 1; break;
        case 0x708:  vehicle_respawn_index = 2; break;
        case 0xa8c:  vehicle_respawn_index = 3; break;
        case 0xe10:  vehicle_respawn_index = 4; break;
        case 0x1518: vehicle_respawn_index = 5; break;
        case 0x2328: vehicle_respawn_index = 6; break;
        default: break;
        }
        if (vehicle_respawn_index >= 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 14);
            swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
            suffix_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\vehicle_options_edit\\var_vehicles_respawn",
                (int16_t)vehicle_respawn_index);
            wcscat(line, suffix_text);
            line[255] = 0;
            ticker_text_buffer_append(line, 0, ticker);
        }
    }

    if (options.weapon_set < 0xe) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 15);
        swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
        suffix_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\settings_select\\multiplayer_setup\\item_options_edit\\var_weapon_set",
            (int16_t)options.weapon_set);
        wcscat(line, suffix_text);
        line[255] = 0;
        ticker_text_buffer_append(line, 0, ticker);
    }

    if ((options.flags & 0x20) == 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 16);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
    } else {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 17);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
    }
    ticker_text_buffer_append(line, 0, ticker);

    if (options.objective_indicator == 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 18);
        swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
    } else {
        if (options.objective_indicator != 1) goto shared_tail;
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 18);
        swprintf(line, 0xff, L"%s%s ", ticker_field_separator, label_text);
    }
    suffix_text = unicode_string_list_get_string(
        (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 19);
    wcscat(line, suffix_text);
    ticker_text_buffer_append(line, 0, ticker);

shared_tail:
    if ((options.flags & 1) != 0) {
        if ((options.flags & 0x40) == 0) {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 21);
        } else {
            label_text = unicode_string_list_get_string(
                (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 22);
        }
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }
    if ((options.flags & 2) != 0) {
        label_text = unicode_string_list_get_string(
            (char *)"ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_rules_strings", 23);
        swprintf(line, 0xff, L"%s%s", ticker_field_separator, label_text);
        ticker_text_buffer_append(line, 0, ticker);
    }
    return;
}

}  // namespace halo::game

extern "C" {

/**
 * C entry point for halo::game::HudNameplates::draw_teammate_nameplate; forwards to the C++ implementation.
 * register convention: player handle in EAX (in_EAX).
 * // blam-cc: EAX -> player
 * blam-cc: EAX -> player
 *
 * @address 0x45e520
 */
void hud_draw_teammate_nameplate(datum_index player_handle)
{
    halo::game::HudNameplates::draw_teammate_nameplate(player_handle);
}

/**
 * C entry point for halo::game::HudNameplates::draw_teammate_nameplate_text; forwards to the C++ implementation.
 * register convention: no register-passed arguments; param_1/param_2 are this function's own
 * blam-cc: EAX -> unknown (0 here), ECX -> bounds (hud_text_bounds *),
 * blam-cc: EAX = 0, ECX = &bounds
 *
 * @address 0x461f20
 */
void hud_draw_teammate_nameplate_text(wchar_t *text, int32_t value)
{
    halo::game::HudNameplates::draw_teammate_nameplate_text(text, value);
}

/**
 * C entry point for halo::game::HudNameplates::find_nearby_teammate_for_nameplate; forwards to the C++ implementation.
 * blam-cc: stack -> player_handle
 *
 * @address 0x45e340
 */
datum_index hud_find_nearby_teammate_for_nameplate(datum_index player_handle)
{
    return halo::game::HudNameplates::find_nearby_teammate_for_nameplate(player_handle);
}

/**
 * C entry point for halo::game::HudNameplates::nameplate_candidate_filter; forwards to the C++ implementation.
 * blam-cc: cdecl, stack -> object_index, context
 *
 * @address 0x45e2e0
 */
uint8_t hud_nameplate_candidate_filter(uint32_t object_index, void *context)
{
    return halo::game::HudNameplates::nameplate_candidate_filter(object_index, context);
}

/**
 * C entry point for halo::game::HudNameplates::update_teammate_nameplate_fade; forwards to the C++ implementation.
 * register convention: this function takes no recognized parameters; every input is a global.
 * blam-cc: stack -> subject_player, opacity
 *
 * @address 0x45f220
 */
void hud_update_teammate_nameplate_fade(void)
{
    halo::game::HudNameplates::update_teammate_nameplate_fade();
}

/**
 * C entry point for halo::game::HudText::scoreboard_row_text; forwards to the C++ implementation.
 * register convention: row index in CX (in_CX, used only as a vertical-spacing multiplier),
 * blam-cc: CX -> row, stack -> text, column
 * blam-cc: EAX -> unknown (0 here), ECX -> bounds (hud_text_bounds *),
 * blam-cc: CX -> row, stack -> text, column (original stack order)
 * blam-cc: EAX = 0, ECX = &bounds
 *
 * @address 0x45d670
 */
void hud_draw_scoreboard_row_text(int16_t row, wchar_t *text, int16_t column)
{
    halo::game::HudText::scoreboard_row_text(row, text, column);
}

/**
 * C entry point for halo::game::HudText::world_relative_text; forwards to the C++ implementation.
 * register convention: a pointer to a hud_world_text_params block in EAX, a row index in EDX,
 * plus the two original stack parameters (text, highlighted).
 * // blam-cc: EAX -> params, EDX -> row, stack -> (text, highlighted)
 * blam-cc: EAX -> unknown (0 at both of this module's call sites),
 * blam-cc: EAX -> params, EDX -> row, stack -> (text, highlighted)
 * blam-cc: EAX = 0, ECX = &bounds
 *
 * @address 0x4653f0
 */
int32_t hud_draw_world_relative_text(hud_world_text_params *params, int16_t row, wchar_t *text, uint8_t highlighted)
{
    return halo::game::HudText::world_relative_text(params, row, text, highlighted);
}

/**
 * C entry point for halo::game::Scoreboard::select_players_to_display; forwards to the C++ implementation.
 * register convention: max display count in EBX (unaff_EBX).
 * // blam-cc: EBX -> max_count, stack -> out
 * blam-cc: EBX -> max_count, stack -> out
 * blam-cc: EAX -> mode, EBX -> max_count, stack -> out
 *
 * @address 0x45d4a0
 */
int32_t select_players_to_display(int32_t mode, int32_t max_count, scoreboard_entry *out)
{
    return halo::game::Scoreboard::select_players_to_display(mode, max_count, out);
}

/**
 * C entry point for halo::game::Scoreboard::compare; forwards to the C++ implementation.
 * register convention: __cdecl, two scoreboard_entry pointers (Ghidra's own recognized stack
 * parameters).
 *
 * @address 0x45cbe0
 */
int32_t scoreboard_entry_compare(const scoreboard_entry *a, const scoreboard_entry *b)
{
    return halo::game::Scoreboard::compare(a, b);
}

/**
 * C entry point for halo::game::Scoreboard::compare_by_unknown_04; forwards to the C++ implementation.
 * register convention: __cdecl, two scoreboard_entry pointers (Ghidra's own recognized stack
 * parameters).
 *
 * @address 0x45cbc0
 */
uint32_t scoreboard_entry_compare_by_unknown_04(const scoreboard_entry *a, const scoreboard_entry *b)
{
    return halo::game::Scoreboard::compare_by_unknown_04(a, b);
}

/**
 * C entry point for halo::game::CustomWaypoints::get_position; forwards to the C++ implementation.
 * register convention: output buffer in EAX (in_EAX); slot index in CX (in_CX).
 * // blam-cc: EAX -> out, CX -> slot
 * blam-cc: EAX -> out, CX -> slot
 *
 * @address 0x462230
 */
void custom_waypoint_get_position(real_point3d *out, int16_t slot)
{
    halo::game::CustomWaypoints::get_position(out, slot);
}

/**
 * C entry point for halo::game::CustomWaypoints::matches_filter; forwards to the C++ implementation.
 * register convention: EAX = a pointer to the reference player record (only its team, +0x20, is read; it is also
 * blam-cc: EAX -> reference_player, EDI -> slot_index, stack -> candidate
 * blam-cc: EAX -> reference_player, EDI -> slot_index, stack -> candidate
 *
 * @address 0x4620c0
 */
uint8_t custom_waypoint_matches_filter(int32_t candidate, player *reference_player, int32_t slot_index)
{
    return halo::game::CustomWaypoints::matches_filter(candidate, reference_player, slot_index);
}

/**
 * C entry point for halo::game::CustomWaypoints::custom_waypoint_register; forwards to the C++ implementation.
 * register convention: owner handle in EAX (in_EAX); slot index in CX (in_CX); the source
 * blam-cc: EAX -> owner, CX -> slot, EBX -> position, EDI -> icon_name, stack ->
 * //          height_offset, player_filter, team_filter
 * blam-cc: EDI -> name
 * blam-cc: EAX -> owner, CX -> slot, EBX -> position, EDI -> icon_name, stack ->
 * height_offset, player_filter, team_filter
 *
 * @address 0x462260
 */
void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position, const char *icon_name, float height_offset, datum_index player_filter, int16_t team_filter)
{
    halo::game::CustomWaypoints::custom_waypoint_register(owner, slot, position, icon_name, height_offset, player_filter, team_filter);
}

/**
 * C entry point for halo::game::ChimeraHooks::kill_feed; forwards to the C++ implementation.
 * register convention: the recipient handle is unaff_EDI (never loaded from any stack slot in
 * blam-cc: unaff_EDI -> recipient, stack -> hash_key, message_type, subject, broadcast
 * blam-cc: EAX -> local_player_index, stack -> text. objdump 0x460b23: `mov ax,[esi+0x2]`
 * blam-cc: unaff_EDI -> recipient, stack -> hash_key, message_type, subject, broadcast
 *
 * @address 0x460a30
 */
void chimera__kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type, datum_index subject, char broadcast)
{
    halo::game::ChimeraHooks::kill_feed(recipient, hash_key, message_type, subject, broadcast);
}

/**
 * C entry point for halo::game::VariantDescription::generate; forwards to the C++ implementation.
 * register convention: EDX = variant_name (in_EDX; sscanf source for
 * blam-cc: EDX -> variant_name; ticker, fraglimit, game_flags_wide, player_flags_wide are
 * blam-cc: EDX=text, ESI=&options
 * blam-cc: EAX=fraglimit, ECX=&engine_extra
 * blam-cc: EAX=fraglimit, ECX=&oddball
 * blam-cc: ECX=fraglimit, EDX=&engine_extra
 *
 * @address 0x4b8da0
 */
void multiplayer_game_variant_description_generate(char *variant_name, ticker_text_buffer *ticker, int32_t fraglimit, wchar_t *game_flags_wide, wchar_t *player_flags_wide)
{
    halo::game::VariantDescription::generate(variant_name, ticker, fraglimit, game_flags_wide, player_flags_wide);
}

}
