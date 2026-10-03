#include "halo/game/game2_engine_hud.hpp"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/ai/api.hpp"

static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &game_engine_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_variant);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &empty_string = halo::link::ref<wchar_t>(halo::game::vars().empty_string);
static auto &hud_messaging_parameters = halo::link::ref<uint8_t *>(halo::ui::vars().hud_messaging_parameters);
static auto &global_white_argb = halo::link::ref<const ColorARGB *>(halo::networking::vars().global_white_argb);
static auto &scoreboard_server_address_raw = halo::link::ref<uint32_t>(halo::game::vars().scoreboard_server_address_raw);
static auto &scoreboard_server_port = halo::link::ref<uint32_t>(halo::game::vars().scoreboard_server_port);
static auto &hud_text_draw_color_r = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_r);
static auto &hud_text_draw_color_g = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_g);
static auto &hud_text_draw_color_b = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_b);
static auto &hud_text_draw_color_a = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_a);
static auto &hud_text_draw_color_or_flags = halo::link::ref<uint16_t>(halo::ui::vars().hud_text_draw_color_or_flags);
static auto &hud_text_draw_font_tag_id = halo::link::ref<int32_t>(halo::ui::vars().hud_text_draw_font_tag_id);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &multiplayer_sound_enabled = halo::link::ref<uint8_t []>(halo::game::vars().multiplayer_sound_enabled);
static auto &multiplayer_sound_queue_count = halo::link::ref<int32_t>(halo::game::vars().multiplayer_sound_queue_count);
static auto &multiplayer_sound_queue = halo::link::ref<multiplayer_sound_request [k_maximum_queued_multiplayer_sounds]>(halo::game::vars().multiplayer_sound_queue);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &custom_waypoints = halo::link::ref<custom_waypoint [k_maximum_custom_waypoints]>(halo::game::vars().custom_waypoints);

namespace halo::game {

/**
 * Returns a string of the ui\multiplayer_game_text tag, or the empty string when the tag is missing.
 */
wchar_t * EngineHud::multiplayer_game_text_string(int16_t index)
{
    datum_index tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, halo::tag_paths::multiplayer_game_text);

    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string(tag_id, index));
}

/**
 * The font the scoreboard text uses: the messaging globals' +0x64 font when more than one local player exists
 * (and it is set), otherwise +0x54 (0x466043..0x466050, 0x4662c1..0x4662ce).
 */
int32_t EngineHud::scoreboard_text_font(void)
{
    HUDGlobals *messaging = (HUDGlobals *)hud_messaging_parameters;
    int32_t font = *(int32_t *)&messaging->fullscreen_font.tag_id;

    if (local_player_globals->local_player_count > 1) {
        int32_t preferred = *(int32_t *)&messaging->splitscreen_font.tag_id;

        if (preferred != -1) {
            font = preferred;
        }
    }
    return font;
}

/**
 * Draws one line of already-formatted text in white through the shared HUD text state, alpha = opacity.
 */
void EngineHud::scoreboard_draw_white_line(Rectangle2D *rect, wchar_t *text, float opacity)
{
    hud_text_draw_font_tag_id = scoreboard_text_font();
    hud_text_draw_color_a = opacity;
    hud_text_draw_color_r = global_white_argb->red;
    hud_text_draw_color_g = global_white_argb->green;
    hud_text_draw_color_b = global_white_argb->blue;
    hud_text_draw_color_or_flags = 0xffffu;
    halo::text::globals().hud_text_draw_column = 1;
    halo::text::globals().hud_text_draw_unknown_4730 = 0;
    halo::rasterizer::chimera__draw_16_bit_text(0, (int32_t *)rect, 0, 0, (const int16_t *)text);
}

/**
 * Renders the in-game multiplayer scoreboard overlay: a background panel, the result line, a header row
 * (column labels plus "Ping" and the engine's live header text), one row per visible player (place / name /
 * status / four stats / ping, prefixed with '*' when the player's unit has the flagged weapon, coloured by
 * team, highlighted when the row is `subject_player`), a bottom-of-screen prompt, and, when a network ...
 *
 * @address 0x465690
 */
void EngineHud::rasterize_in_game_score(datum_index subject_player, float opacity)
{
    uint8_t teams_enabled;
    wchar_t result_text[0x50];
    scoreboard_entry visible[16];
    int32_t visible_count;
    wchar_t row_buffer[0x200];
    wchar_t header_names_buf[0x100];
    hud_world_text_params params_result;
    hud_world_text_params params_default;
    hud_world_text_params params_header;
    hud_world_text_params team_params[2];
    real_vector3d bg_color;
    Rectangle2D bg_rect;
    wchar_t *col_a, *col_b, *col_c, *col_d, *col_e;
    int32_t highlight_team;
    int32_t pass;
    int32_t row_count;
    int32_t ui_state;
    wchar_t *prompt;

    teams_enabled = (current_game_engine != 0) ? (uint8_t)game_engine_variant.teams : 0;

    halo::game::game_engine_build_end_game_result_text(subject_player, result_text);
    visible_count = halo::game::select_players_to_display(0, 16, visible);

    bg_color.i = 0.125f;
    bg_color.j = 0.125f;
    bg_color.k = 0.125f;
    bg_rect.top = 0x3c;
    bg_rect.left = 0xa;
    bg_rect.bottom = 0x186;
    bg_rect.right = 0x276;
    halo::interface::ui_draw_filled_rectangle(halo::math::color_real_to_argb_pack(opacity * 0.69f, &bg_color.i), &bg_rect);

    params_result.alpha = opacity;
    params_result.red = 0.7f;
    params_result.green = 0.7f;
    params_result.blue = 0.7f;
    halo::game::hud_draw_world_relative_text(&params_result, 0, result_text, 0);

    params_default.alpha = opacity;
    params_default.red = ((HUDGlobals *)hud_messaging_parameters)->icon_color.red;
    params_default.green = ((HUDGlobals *)hud_messaging_parameters)->icon_color.green;
    params_default.blue = ((HUDGlobals *)hud_messaging_parameters)->icon_color.blue;
    team_params[0].alpha = opacity;
    team_params[0].red = 0.6f;
    team_params[0].green = 0.3f;
    team_params[0].blue = 0.3f;
    team_params[1].alpha = opacity;
    team_params[1].red = 0.3f;
    team_params[1].green = 0.3f;
    team_params[1].blue = 0.6f;
    params_header.alpha = opacity;
    params_header.red = 0.5f;
    params_header.green = 0.5f;
    params_header.blue = 0.5f;

    col_a = multiplayer_game_text_string(0x43);
    col_b = multiplayer_game_text_string(0x44);
    col_c = multiplayer_game_text_string(0x45);
    col_d = multiplayer_game_text_string(0x46);
    col_e = multiplayer_game_text_string(0x47);
    ((void (*)(wchar_t *))current_game_engine->build_score_header_text)(header_names_buf);
    halo::text::string_format_wide_va((uint16_t *)row_buffer, (const uint16_t *)(L"\t%s\t%s\t%s\t%s\t%s\t%s\t%s"), col_a, col_b, header_names_buf,
                          col_c, col_d, col_e, L"Ping");
    halo::game::hud_draw_world_relative_text(&params_header, 1, row_buffer, 0);

    highlight_team = -1;
    if (subject_player != (datum_index)halo::k_dword_none && (int16_t)subject_player >= 0 &&
        (int16_t)subject_player < player_data->maximum_count) {
        player *subj = halo::game::player_at(subject_player);
        if (subj->identifier != 0 &&
            ((int16_t)(subject_player >> 16) == 0 || subj->identifier == (int16_t)(subject_player >> 16))) {
            highlight_team = subj->team;
        }
    }

    row_count = 0;
    for (pass = teams_enabled ? 2 : 1; pass != 0; pass = pass - 1) {
        int32_t i;

        for (i = 0; i < visible_count; i = i + 1) {
            datum_index row_player = visible[i].player;
            uint8_t is_subject = (uint8_t)(subject_player == row_player);
            player *p;

            if (row_player == (datum_index)halo::k_dword_none || (int16_t)row_player < 0 ||
                (int16_t)row_player >= player_data->maximum_count) {
                continue;
            }
            p = halo::game::player_at(row_player);
            if (p->identifier == 0) {
                continue;
            }
            if ((int16_t)(row_player >> 16) != 0 && p->identifier != (int16_t)(row_player >> 16)) {
                continue;
            }
            if (teams_enabled != 0 && highlight_team != p->team) {
                continue;
            }

            {
                wchar_t *status_text;
                wchar_t *place_text;
                uint8_t starred = 0;
                hud_world_text_params *row_params;

                ((void (*)(datum_index, wchar_t *))current_game_engine->build_player_text)(row_player, header_names_buf);

                if (game_engine_variant.lives_per_round >= 1 && p->unit == (datum_index)halo::k_dword_none &&
                    (int32_t)(int16_t)p->deaths >= game_engine_variant.lives_per_round) {
                    status_text = multiplayer_game_text_string(0x8a);
                } else if (p->marked_for_deletion != 0) {
                    status_text = multiplayer_game_text_string(0x8b);
                } else {
                    status_text = header_names_buf;
                }

                if (p->unit != (datum_index)halo::k_dword_none) {
                    int16_t unit_index = (int16_t)p->unit;
                    int16_t unit_salt = (int16_t)((uint32_t)p->unit >> 16);

                    if (unit_index >= 0 && unit_index < halo::objects::globals().object_data->maximum_count) {
                        object_header *unit_header = (object_header *)((uint8_t *)halo::objects::globals().object_data->data +
                                                                        (int32_t)halo::objects::globals().object_data->size * unit_index);

                        if (unit_header->identifier != 0 && (unit_salt == 0 || unit_header->identifier == unit_salt) &&
                            (((1u << (unit_header->type & 0x1f)) & 3) != 0) && unit_header->data != 0) {
                            starred = (uint8_t)halo::units::unit_find_weapon_index_by_flag(p->unit, 3);
                        }
                    }
                }

                place_text = halo::game::game_engine_get_default_multiplayer_string(&visible[i]);
                halo::text::string_format_wide_va((uint16_t *)row_buffer, (const uint16_t *)(starred ? L"*\t%s\t%s\t%s\t%d\t%d\t%d\t%d"
                                                          : L"\t%s\t%s\t%s\t%d\t%d\t%d\t%d"),
                                      place_text, p->name, status_text,
                                      visible[i].key_1, visible[i].key_3, visible[i].key_2,
                                      p->ping);

                if (teams_enabled != 0) {
                    int32_t team = p->team;

                    if (team < 0) {
                        team = 0;
                    } else if (team > 1) {
                        team = 1;
                    }
                    row_params = &team_params[team];
                } else {
                    row_params = &params_default;
                }
                halo::game::hud_draw_world_relative_text(row_params, (int16_t)(row_count + 2), row_buffer, is_subject);
                row_count = row_count + 1;
            }
        }
        if (highlight_team != -1) {
            highlight_team = 1 - highlight_team;
        }
    }

    ui_state = halo::game::game_engine_multiplayer_ui_state_id();
    if ((int16_t)ui_state != 8) {
        wchar_t *word;
        Rectangle2D prompt_rect;

        prompt = multiplayer_game_text_string((int16_t)ui_state);
        if (prompt != (wchar_t *)0) {
            if (current_game_engine != 0 && game_engine_variant.teams == 1) {
                word = multiplayer_game_text_string(0xc);
            } else {
                word = multiplayer_game_text_string(0xd);
            }
            halo::text::string_format_wide_va((uint16_t *)row_buffer, (const uint16_t *)(L"%s (%s)"), prompt, word);

            prompt_rect.top = 0x1b8;
            prompt_rect.left = 0xa;
            prompt_rect.bottom = 0x1cc;
            prompt_rect.right = 0x27b;
            scoreboard_draw_white_line(&prompt_rect, row_buffer, opacity);
        }
    }

    {
        uint32_t port = 0;
        char *address_text;
        s_network_address address;

        if (halo::networking::globals().server != 0) {
            struct in_addr in;
            uint32_t raw = scoreboard_server_address_raw;

            in.s_addr = ((raw << 0x10 | (raw & 0xff00) | (raw >> 0x10 & 0xff)) << 8) | (raw >> 0x18);
            address_text = inet_ntoa(in);
            port = scoreboard_server_port;
        } else {
            network_receive_queue *queue;

            if (halo::networking::globals().client == 0) {
                return;
            }
            queue = halo::networking::globals().client->channel->endpoint;
            if (queue != 0) {
                if (halo::networking::network_channel_get_remote_address(&address, queue) != 0) {
                    memset(&address, 0, 0x18);
                    address.size = 4;
                }
            } else {
                memset(&address, 0, 0x18);
                address.size = 4;
            }
            address_text = halo::networking::network_address_to_string(&address);
        }

        if (address_text != (char *)0) {
            wchar_t *label = multiplayer_game_text_string(0xbe);
            wchar_t address_wide[0x100];
            Rectangle2D address_rect;
            int32_t len = (int32_t)strlen(address_text);
            int32_t n;

            if (len * 2 + 2 > 0x200) {
                len = 0xff;
            }
            for (n = 0; n < len; n = n + 1) {
                address_wide[n] = (uint8_t)address_text[n];
            }
            address_wide[len] = 0;

            if ((uint16_t)port != 0) {
                halo::text::string_format_wide_va_bounded(0x200, (uint16_t *)row_buffer, (const uint16_t *)(L"%s%s:%u"), label, address_wide, (uint32_t)(uint16_t)port);
            } else {
                halo::text::string_format_wide_va_bounded(0x200, (uint16_t *)row_buffer, (const uint16_t *)(L"%s%s"), label, address_wide);
            }

            address_rect.top = 0x1cc;
            address_rect.left = 0xa;
            address_rect.bottom = 0x1e0;
            address_rect.right = 0x27b;
            scoreboard_draw_white_line(&address_rect, row_buffer, opacity);
        }
    }
}

/**
 * Stub with no body in this retail build; presumably an on-screen game-engine message rasterizer that was
 * compiled out or disabled.
 *
 * @address 0x462a80
 */
void EngineHud::rasterize_message(void)
{
}

/**
 * Determines and dispatches which contextual HUD hint (for example leader, score-limit reached, eliminated)
 * should currently be shown to a player.
 *
 * @address 0x463150
 */
uint8_t EngineHud::pick_hud_hint(datum_index player_index, int32_t maximum_length, uint16_t *out_text)
{
    wchar_t *out = (wchar_t *)out_text;
    uint32_t buffer_size = (uint32_t)maximum_length;
    player *p = halo::game::player_at(player_index);

    if (current_game_engine == 0) {
        return 0;
    }

    if (0x16 < (int32_t)p->hud_message_index && (int32_t)p->hud_message_index < 0x1b) {
        p->hud_message_index = (datum_index)halo::k_dword_none;
    }

    if (p->unit == (datum_index)halo::k_dword_none) {
        uint32_t message_type;
        int32_t extra = 0;

        if (p->marked_for_deletion == 1) {
            message_type = 0x1b;
        } else if (halo::game::game_engine_player_is_eliminated(player_index) != 0) {
            message_type = 0x18;
        } else if (halo::game::game_engine_player_has_respawn_priority(player_index) != 0) {
            message_type = 0x17;
        } else if (p->respawn_timer < 1) {
            message_type = 0x1a;
        } else {
            extra = p->respawn_timer / 30;
            message_type = 0x19;
        }

        if (current_game_engine->build_message_text != 0) {
            char handled = ((char (*)(datum_index, uint32_t, int32_t, wchar_t *, uint32_t))
                current_game_engine->build_message_text)(player_index, message_type, extra, out, buffer_size);
            if (handled != 0) {
                return (uint8_t)handled;
            }
        }
        return halo::game::game_engine_build_kill_feed_message_text(player_index, out, message_type, (datum_index)extra, buffer_size);
    } else {
        if (game_time->game_time < halo::game::k_ticks_per_fifteen_seconds) {
            if (p->hud_message_index == (datum_index)halo::k_dword_none ||
                game_engine_variant.game_engine_index != _game_engine_ctf ||
                game_engine_variant.engine.ctf.single_flag_time < 1) {
                return halo::game::game_engine_build_message_text(out, buffer_size, (datum_index)halo::k_dword_none, player_index, 0x1d);
            }
        } else if (p->hud_message_index == (datum_index)halo::k_dword_none) {
            return 0;
        }
        return halo::game::game_engine_build_message_text(out, buffer_size, p->hud_message_player, player_index, p->hud_message_index);
    }
}

/**
 * If `sound_index` names a valid GlobalsMultiplayerInformation sound, optionally sends the networked status
 * message (when `broadcast`), then plays the sound locally at full volume unless there is a specific
 * `recipient_player` who is not the local player.
 *
 * @address 0x46bd00
 */
void EngineHud::play_multiplayer_sound(int32_t sound_index, datum_index recipient_player, uint8_t broadcast)
{
    GlobalsMultiplayerInformation *mp_info =
        (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
    GlobalsSound *sound;

    if (mp_info == (GlobalsMultiplayerInformation *)0 || sound_index >= (int32_t)mp_info->sounds.count) {
        return;
    }
    sound = (GlobalsSound *)mp_info->sounds.pointer + sound_index;
    if (sound == (GlobalsSound *)0 || *(int32_t *)&sound->sound.tag_id == -1) {
        return;
    }

    if (broadcast == 1) {
        halo::game::game_engine_queue_status_sound_message(sound_index, recipient_player);
    }

    if (recipient_player == (datum_index)halo::k_dword_none || halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        halo::sound::sound_start_unspatialized(*(datum_index *)&sound->sound.tag_id, 1.0f);
    } else {
        player *p = (player *)halo::memory::datum_get(recipient_player, player_data);
        if (p != (player *)0 && p->local_player_index != -1) {
            halo::sound::sound_start_unspatialized(*(datum_index *)&sound->sound.tag_id, 1.0f);
        }
    }
}

/**
 * Appends a multiplayer sound request (sound index, target player, broadcast byte) to the announcer queue,
 * refusing once five are queued. The broadcast byte is forced to 0 unless hosting; playback starts at once
 * when the sound is disabled or the request is the only queued entry.
 *
 * @address 0x46be40
 */
void EngineHud::queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast)
{
    int32_t count;

    if (halo::networking::globals().game_mode != halo::networking::k_game_mode_host) {
        broadcast = 0;
    }
    if (multiplayer_sound_enabled[sound_index] != 0) {
        int32_t duration = halo::game::game_engine_get_multiplayer_sound_duration_ticks(sound_index) + 5;

        count = multiplayer_sound_queue_count;
        if (count < k_maximum_queued_multiplayer_sounds) {
            multiplayer_sound_request *slot = &multiplayer_sound_queue[count];

            slot->player = player;
            slot->sound_index = sound_index;
            slot->remaining_ticks = duration;
            slot->broadcast = broadcast;
            count++;
            multiplayer_sound_queue_count = count;
        }
        if (count != 1) {
            return;
        }
    }
    halo::game::game_engine_play_multiplayer_sound(sound_index, player, broadcast);
}

/**
 * Encodes the multiplayer-sound status event (0x19) carrying the sound index and either broadcasts it
 * (recipient -1) or, when the recipient's machine record has flag bits 1 and 2 set, sends it to that machine
 * only.
 *
 * @address 0x46bbd0
 */
void EngineHud::queue_status_sound_message(int32_t sound_index, datum_index recipient_player)
{
    int32_t payload = sound_index;
    void *items[2];
    int32_t encoded_bits;

    items[0] = &payload;
    items[1] = 0;
    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, halo::networking::message_id(halo::networking::delta_message::status_sound), 0, items, 0, 1, 0);
    if (encoded_bits > 0) {
        if (recipient_player == (datum_index)halo::k_dword_none) {
            halo::networking::network_session_broadcast_to_flagged(encoded_bits, halo::networking::globals().server, 1, network_message_scratch, 1, 0, 0, 3);
        } else {
            int32_t machine_id = (int8_t)*((uint8_t *)halo::game::player_at(recipient_player) + 0x64);
            network_machine *machine = halo::networking::network_machine_find_by_id(halo::networking::globals().server, machine_id);

            if (machine != 0) {
                uint8_t flags = machine->flags;

                if ((flags >> 1 & 1) != 0 && (flags >> 2 & 1) != 0) {
                    halo::networking::network_session_send_to_machine(machine_id, halo::networking::globals().server, 1, network_message_scratch, encoded_bits, 1, 0, 0, 3);
                }
            }
        }
    }
}

/**
 * For each active custom waypoint that passes the player/team filter, pushes an add-or-update call into the
 * interface HUD nav-point system.
 *
 * @address 0x462a90
 */
void EngineHud::update_custom_waypoint_navpoints(int16_t local_player_slot)
{
    datum_index local_player;
    player *p;
    real_point3d eye;
    int32_t slot;

    if (current_game_engine == 0 || game_engine_variant.objective_indicator != 1 ||
        local_player_slot == -1 || 1 <= local_player_slot) {
        return;
    }
    local_player = local_player_globals->local_players[local_player_slot];
    if (local_player == (datum_index)halo::k_dword_none) {
        return;
    }
    p = halo::game::player_at(local_player);
    if (p->unit == (datum_index)halo::k_dword_none) {
        return;
    }

    halo::units::unit_get_primary_eye_marker_position(p->unit, &eye);

    for (slot = 0; slot < k_maximum_custom_waypoints; slot++) {
        if (halo::game::custom_waypoint_matches_filter((int32_t)local_player, p, slot) != 0) {
            custom_waypoint *waypoint = &custom_waypoints[slot];

            if (current_game_engine == 0 || current_game_engine->index != _game_engine_ctf ||
                game_engine_variant.engine.ctf.assault != 0 ||
                waypoint->team == p->team || waypoint->team == -1) {
                int16_t visibility = halo::interface::hud_waypoint_visibility(local_player_slot, &eye, &waypoint->position, (datum_index)halo::k_dword_none);

                halo::interface::hud_waypoint_draw(&waypoint->position, local_player_slot, (int16_t)(uint16_t)waypoint->icon, visibility, 1);
            } else {
                if (halo::interface::hud_waypoint_visibility(local_player_slot, &eye, &waypoint->position, (datum_index)halo::k_dword_none) != 0) {
                    continue;
                }
                halo::interface::hud_waypoint_draw(&waypoint->position, local_player_slot, (int16_t)(uint16_t)waypoint->icon, 0, 0);
            }
        }
    }
}

}
