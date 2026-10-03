/**
 * @file src/networking/net2_server_commands.cpp
 * Server console commands.
 */
#include "tags.h"
#include "halo/core/cstring.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "crt.h"
#include <stdlib.h>
#include <wchar.h>
#include <stdint.h>
#include "halo/networking/net2_server_commands.hpp"
#include "halo/networking/server_command.hpp"
#include "halo/memory/api.hpp"
#include "halo/main/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern int16_t network_game_mode;
extern char sv_ban_penalty_arg_buffer[];
extern network_server_globals * network_server;
extern void * global_white_argb;
extern void * console_message_default_color;
extern int32_t sv_ban_penalty_seconds[4];
extern char network_banlist_full_path[0x104];
extern char profile_directory[0x105];
extern int32_t sv_friendly_fire_mode;
extern game_engine_definition * current_game_engine;
extern int32_t game_variant_history_current;
extern game_variant game_variant_saved_default;
extern uint8_t game_variant_saved_default_valid;
extern char game_engine_is_map_and_variant_valid(void);
extern void game_engine_free_custom_variant_cache(void);
extern uint32_t game_engine_variant_add_to_history(char *name, game_variant *options, char *path);
extern void widget_close_all(void);
extern void game_engine_begin_end_game_sequence(void);
extern uint8_t game_engine_get_variant_by_name(const char *name, game_variant *out);
extern game_engine_state game_engine_state_value;
extern void game_engine_reset_round_objects(void);
extern void game_engine_send_round_reset_message(void);
extern void game_engine_player_profile_cache_sync_all(int32_t commit);
extern int32_t sv_maxplayers_value;
extern uint16_t network_server_name[64];
extern uint8_t network_server_name_is_default;
extern uint16_t network_server_password[9];
extern uint8_t network_server_password_is_default;
extern data_array * player_data;
extern wchar_t k_empty_string[];
extern char network_team_color_name_red[];
extern char network_team_color_name_blue[];
extern void * console_color_00685214;
extern void * console_color_00686af8;
extern char sv_rcon_password_value[9];
extern uint8_t network_single_flag_force_reset_value;
extern char network_build_string[];
extern int32_t sv_timelimit_minutes;
extern int32_t sv_tk_cooldown_ticks;
extern char sv_tk_grace_arg_buffer[];
extern int32_t sv_tk_grace_ticks;
}

static player *sv_players_resolve_player(uint32_t handle)
{
    int16_t index = (int16_t)handle;
    int16_t salt = (int16_t)(handle >> 16);
    uint8_t *element;

    if (index < 0 || index >= halo::game::globals().player_data->maximum_count) {
        return 0;
    }
    element = (uint8_t *)halo::game::globals().player_data->data + (int32_t)halo::game::globals().player_data->size * (int32_t)index;
    if (*(int16_t *)element == 0) {
        return 0;
    }
    if (salt != 0 && *(int16_t *)element != salt) {
        return 0;
    }
    return (player *)element;
}

namespace halo::networking {

void ServerCommands::ban(uint32_t argument_count, int32_t *arguments)
{
    int32_t duration = 0;
    network_player_entry *player;
    network_machine *machine;

    if (network_game_mode != halo::networking::k_game_mode_host) {
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, halo::mutable_literal("sv_ban is a server-only function!"));
        return;
    }
    if (0 < (int32_t)argument_count && (int32_t)argument_count < 3) {
        if (argument_count == 2) {
            duration = halo::networking::parse_time_duration_string((char *)arguments[1], 'm', (uint8_t *)sv_ban_penalty_arg_buffer);
            if (duration == -1) {
                halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_ban for more information."));
                return;
            }
        }
        player = halo::networking::sv_find_client_by_name_or_index((char *)arguments[0]);
        if (player != 0) {
            machine = halo::networking::network_machine_find_by_id(network_server, player->machine_index);
            if (machine != 0 && machine->channel != 0 && machine->channel->connected != 0) {
                halo::interface::chimera__console_out((ColorARGB *)console_message_default_color, halo::mutable_literal("sv_ban:  Can't ban a local client!"));
                return;
            }
            halo::networking::network_banlist_add_ban(machine->gcd_user_id, duration, player);
            halo::networking::network_server_notify_or_resend_challenge(6, machine, network_server);
        }
        return;
    }
    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_ban for more information."));
}

void ServerCommands::ban_penalty(uint32_t argument_count, int32_t *arguments)
{
    int32_t saved[4];
    int32_t i;

    if (argument_count == 0) {
    print_table:
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Offense    %s"), "Ban Time");
        i = 0;
        do {
            int32_t seconds = sv_ban_penalty_seconds[i];
            char buf[16];
            if (seconds == -1) {
                halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("%1d          %s"), i + 1, "Infinite");
                break;
            }
            {
                int32_t days = (seconds % 0x15180) / 0xe10;
                int32_t rem = (seconds % 0x15180) % 0xe10;
                int32_t hours = rem / 0x3c;
                rem = rem % 0x3c;
                if (seconds / 0x15180 == 0) {
                    if (days == 0) {
                        if (hours == 0) {
                            if (rem == 0) {
                                sprintf(buf, "---");
                            } else {
                                sprintf(buf, "%ds", rem);
                            }
                        } else {
                            sprintf(buf, "%dm %ds", hours, rem);
                        }
                    } else {
                        sprintf(buf, "%dh %dm %ds", days, hours, rem);
                    }
                } else {
                    sprintf(buf, "%dd %dh %dm %ds", seconds / 0x15180, days, hours, rem);
                }
            }
            i = i + 1;
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("%1d          %s"), i, buf);
        } while (i < 4);
        if (i == 4 && sv_ban_penalty_seconds[3] != -1) {
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal(">%1d         %s"), 4, "Infinite");
        }
        return;
    }
    if (0 < (int32_t)argument_count && (int32_t)argument_count < 5) {
        saved[0] = sv_ban_penalty_seconds[0];
        saved[1] = sv_ban_penalty_seconds[1];
        saved[2] = sv_ban_penalty_seconds[2];
        saved[3] = sv_ban_penalty_seconds[3];
        i = 0;
        do {
            int32_t value = halo::networking::parse_time_duration_string((char *)arguments[i], 'm', (uint8_t *)sv_ban_penalty_arg_buffer);
            if (value == 0) {
                sv_ban_penalty_seconds[i] = -1;
                break;
            }
            if (value < 1) {
                sv_ban_penalty_seconds[0] = saved[0];
                sv_ban_penalty_seconds[1] = saved[1];
                sv_ban_penalty_seconds[2] = saved[2];
                sv_ban_penalty_seconds[3] = saved[3];
                halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_ban_penalty for more information."));
                return;
            }
            sv_ban_penalty_seconds[i] = value;
            i = i + 1;
        } while (i < (int32_t)argument_count);
        goto print_table;
    }
    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_ban_penalty for more information."));
}

void ServerCommands::banlist_file(uint32_t argument_count, int32_t *arguments)
{
    if (argument_count == 0) {
    report:
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_banlist_file: %s"), network_banlist_full_path);
        return;
    }
    if (argument_count == 1) {
        char *suffix = (char *)arguments[0];
        int32_t len = strlen(suffix);

        if (len == 0) {
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Ban file names must not be empty."));
        } else if (0xf5 < len) {
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Ban file names cannot be longer than %d characters."), 0xfa);
        } else {
            int32_t i;
            for (i = 0; suffix[i] != 0; i = i + 1) {
                if (!isalnum((uint8_t)suffix[i])) {
                    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Ban list file names must be alphanumeric."));
                    goto usage;
                }
            }
            strcpy(network_banlist_full_path, suffix);
            sprintf(network_banlist_full_path, "%s\\banned%s.txt", profile_directory, suffix);
            halo::networking::network_banlist_load();
            goto report;
        }
    }
usage:
    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_banlist_file for more information."));
}

network_player_entry * ServerCommands::find_client_by_name_or_index(char *name_or_index)
{
    network_game_session *session = &network_server->session;
    int32_t i;

    if (halo::networking::string_is_numeric(name_or_index) == 0) {
        uint16_t wide_name[13];

        halo::text::string_convert_ascii_to_unicode(wide_name, 0x1a, name_or_index);
        for (i = 0; i < 0x10; i = i + 1) {
            network_player_entry *entry = &session->players[i];
            if (halo::networking::network_player_entry_validate(entry) != 0 && wcscmp((const wchar_t *)wide_name, (const wchar_t *)entry->name) == 0) {
                return entry;
            }
        }
        return 0;
    } else {
        int32_t index = atol(name_or_index) - 1;
        if (index < 0 || 0x10 <= index) {
            return 0;
        }
        for (i = 0; i < 0x10; i = i + 1) {
            network_player_entry *entry = &session->players[i];
            if (halo::networking::network_player_entry_validate(entry) != 0 && entry->slot_index == index) {
                return entry;
            }
        }
        return 0;
    }
}

void ServerCommands::friendly_fire(uint32_t argument_count, int32_t *arguments)
{
    uint8_t changed = 0;
    const char *label;

    if (argument_count != 0) {
        if (argument_count != 1) {
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_friendly_fire for more information."));
            return;
        }
        {
            char *arg = (char *)arguments[0];
            if (_stricmp(arg, "0") == 0 || _stricmp(arg, "default") == 0) {
                sv_friendly_fire_mode = 0;
                changed = 1;
            } else if (_stricmp(arg, "1") == 0 || _stricmp(arg, "off") == 0) {
                sv_friendly_fire_mode = 1;
                changed = 1;
            } else if (_stricmp(arg, "2") == 0 || _stricmp(arg, "shields") == 0) {
                sv_friendly_fire_mode = 2;
                changed = 1;
            } else if (_stricmp(arg, "3") == 0 || _stricmp(arg, "on") == 0) {
                sv_friendly_fire_mode = 3;
                changed = 1;
            } else {
                halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_friendly_fire:  invalid parameter %s"), arg);
                halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_friendly_fire for more information."));
                return;
            }
        }
    }
    switch (sv_friendly_fire_mode) {
    case 1: label = "1 = off"; break;
    case 2: label = "2 = shields"; break;
    case 3: label = "3 = on"; break;
    default:
        sv_friendly_fire_mode = 0;

    case 0: label = "0 = default"; break;
    }
    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_friendly_fire: %s"), label);
    if (changed && halo::game::globals().current_engine != 0) {
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("   Game in progress...  Changes will apply to the next game."));
    }
}

void ServerCommands::kick(char *name_or_index)
{
    network_player_entry *player;
    network_machine *machine;

    if (network_game_mode != halo::networking::k_game_mode_host) {
        halo::interface::chimera__console_out((ColorARGB *)console_message_default_color, halo::mutable_literal("sv_kick is a server-only function!"));
        return;
    }
    player = halo::networking::sv_find_client_by_name_or_index(name_or_index);
    if (player != 0) {
        machine = halo::networking::network_machine_find_by_id(network_server, player->machine_index);
        if (machine != 0 && machine->channel != 0 && machine->channel->connected != 0) {
            halo::interface::chimera__console_out((ColorARGB *)global_white_argb, halo::mutable_literal("sv_kick:  Can't kick a local client!"));
            return;
        }
        halo::networking::network_server_notify_or_resend_challenge(7, machine, network_server);
    }
}

void ServerCommands::map(uint32_t argument_count, uint16_t **arguments)
{
    if (argument_count == 0 || arguments == 0 || halo::game::game_engine_is_map_and_variant_valid((const char *)(uintptr_t)argument_count, (const char *)arguments) == 0) {
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, halo::mutable_literal("sv_map specified invalid map or game variant"));
        return;
    }

    if (network_game_mode == halo::networking::k_game_mode_host) {
        halo::game::game_engine_free_custom_variant_cache();
        halo::game::game_engine_variant_add_to_history(0, 0, 0);
        halo::game::globals().variant_history_current = -1;
        halo::interface::widget_close_all();
        halo::game::game_engine_begin_end_game_sequence();
        halo::main::console_deactivate();
        return;
    }

    if (network_game_mode == halo::networking::k_game_mode_local) {
        game_variant new_variant;

        halo::main::main_queue_map_change_by_name_or_clear(halo::mutable_literal(""));
        halo::game::game_engine_get_variant_by_name(0, &new_variant);
        memcpy(&game_variant_saved_default, &new_variant, sizeof(game_variant));
        game_variant_saved_default_valid = 1;
        if (halo::networking::network_game_start_new_server_from_profile(0) == 0) {
            return;
        }
        halo::main::console_deactivate();
        return;
    }

    halo::interface::chimera__console_out((ColorARGB *)console_message_default_color, halo::mutable_literal("sv_map is a server-only function!"));
}

void ServerCommands::map_reset(void)
{
    if (network_game_mode != halo::networking::k_game_mode_host) {
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_map_reset is a server-only function!"));
        return;
    }
    halo::interface::widget_close_all();
    if (network_game_mode == halo::networking::k_game_mode_host) {
        if (halo::game::globals().state == _game_engine_state_not_started) {
            halo::game::game_engine_reset_round_objects();
            halo::game::game_engine_send_round_reset_message();
            halo::game::game_engine_player_profile_cache_sync_all(0, (void *)halo::k_dword_none);
            halo::interface::chimera__console_out((ColorARGB *)console_message_default_color, halo::mutable_literal("Map reset."));
            return;
        }
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Cannot restart the map when the game is over."));
    }
    halo::interface::chimera__console_out((ColorARGB *)global_white_argb, halo::mutable_literal("Map reset."));
}

void ServerCommands::maxplayers(uint32_t argument_count, int32_t *arguments)
{
    if (argument_count == 0) {
    report:
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_maxplayers: %d"), sv_maxplayers_value);
        return;
    }
    if (argument_count == 1) {
        int32_t value = atol((char *)arguments[0]);
        if (0 < value && value < 0x11) {
            sv_maxplayers_value = value;
            if (network_server != 0) {
                network_server->session.maximum_players = (uint8_t)value;
            }
            if (value == 1) {
                halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("WARNING: sv_maxplayers set to 1, are you sure you want to do this?"));
            }
            goto report;
        }
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_maxplayers must be between 1 and %d"), 0x10);
    }
    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_maxplayers for more information."));
}

void ServerCommands::name(uint32_t argument_count, char **arguments)
{
    wchar_t scratch[64];

    if (argument_count == 0) {
    report:
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_name: %ls"), network_server_name);
        return;
    }
    if (argument_count == 1) {
        char *name = arguments[0];
        int32_t length = (int32_t)strlen(name);

        if (length != 0 && (uint32_t)length < 0x40) {
            wchar_t *result = reinterpret_cast<wchar_t *>(halo::text::string_convert_ascii_to_unicode(reinterpret_cast<uint16_t *>(scratch), 0x80, name));
            if (result == scratch) {
                if (halo::networking::network_name_string_is_valid_for_mode(name, scratch, 3) != 0) {
                    network_server_globals *server = network_server;
                    wcsncpy((wchar_t *)network_server_name, (const wchar_t *)scratch, 0x3f);
                    network_server_name_is_default = 0;
                    if (server != 0) {
                        halo::networking::network_password_field_set((uint8_t *)server, (wchar_t *)scratch);
                    }
                    goto report;
                }
            }
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Server names must only contain printable ASCII characters supported by the Halo UI."));
            goto usage;
        }
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Server names must be between 1 and %d characters."), 0x3f);
    }
usage:
    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_name for more information."));
}

void ServerCommands::password(uint32_t argument_count, char **arguments)
{
    wchar_t scratch[9];

    if (argument_count == 0) {
    report:
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_password: %ls"), network_server_password);
        return;
    }
    if (argument_count == 1) {
        char *text = arguments[0];
        int32_t length = (int32_t)strlen(text);

        if ((uint32_t)length < 9) {
            wchar_t *result = reinterpret_cast<wchar_t *>(halo::text::string_convert_ascii_to_unicode(reinterpret_cast<uint16_t *>(scratch), 0x12, text));
            if (result == scratch) {
                uint8_t ok = 1;
                if (text[0] != '\0') {
                    ok = halo::networking::network_name_string_is_valid_for_mode(text, scratch, 0);
                }
                if (ok != 0) {
                    network_server_globals *server = network_server;
                    wcsncpy((wchar_t *)network_server_password, (const wchar_t *)scratch, 8);
                    network_server_password_is_default = 0;
                    if (server != 0) {
                        halo::networking::network_server_password_set(scratch, server);
                    }
                    goto report;
                }
            }
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Server passwords must only contain printable ASCII characters supported by the Halo UI."));
        } else {
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Server passwords must be no more than %d characters."), 8);
        }
    }
    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_password for more information."));
}

void ServerCommands::players(void)
{
    char line[256];
    uint16_t score_text[256];
    char ascii_name[16];
    network_player_entry *entry;
    int32_t remaining;

    if (network_game_mode != halo::networking::k_game_mode_host) {
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, halo::mutable_literal("sv_players is a server-only function!"));
        return;
    }

    sprintf(line, "%-8s%-*s %-6s %-6s %-6s %-6s %-8s", "Number", 0xc, "Name", "Team", "Ping",
            "Score", "TK Num", "TK Timer");
    halo::interface::chimera__console_out((ColorARGB *)console_color_00685214, line);

    entry = network_server->session.players;
    remaining = 16;
    do {
        if (halo::networking::network_player_entry_validate(entry) != 0) {
            uint32_t found = halo::networking::sv_players_find_by_team_index_desired(entry->slot_index);
            player *p = 0;
            int32_t ping;
            int32_t tk_num;
            int32_t tk_timer;
            uint16_t *score_display;
            char *team_color;

            if (found != halo::k_dword_none) {
                p = sv_players_resolve_player(found);
            }

            score_text[0] = 0;
            if (found != halo::k_dword_none) {
                void (*resolve_score_text)(uint32_t, uint16_t *) =
                    *(void (**)(uint32_t, uint16_t *))((uint8_t *)halo::game::globals().current_engine + 0x54);
                resolve_score_text(found, score_text);
            }

            ascii_name[0] = 0;
            halo::text::string_convert_unicode_to_ascii((uint8_t *)ascii_name, entry->name, 0xc);

            if (p == 0) {
                ping = 0;
                tk_num = 0;
                tk_timer = 0;
                score_display = (uint16_t *)k_empty_string;
            } else {
                ping = p->ping;
                tk_num = p->medal_streak_count;
                tk_timer = (p->medal_streak_timer < 0) ? 0 : p->medal_streak_timer / 30;
                score_display = score_text;
            }

            team_color = (entry->team_index == 0) ? network_team_color_name_red : network_team_color_name_blue;

            sprintf(line, "%-3d     %-*s %-6s %-4d   %-6ls %-3d    %-4d",
                    (int32_t)entry->machine_index + 1, 0xc, ascii_name, team_color, ping,
                    score_display, tk_num, tk_timer);
            halo::interface::chimera__console_out((ColorARGB *)console_color_00686af8, line);
        }
        entry = entry + 1;
        remaining = remaining - 1;
    } while (remaining != 0);
}

uint32_t ServerCommands::players_find_by_team_index_desired(int8_t team_index_desired)
{
    data_iterator iterator;
    player *p;

    iterator.data = halo::game::globals().player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    p = (player *)halo::memory::data_iterator_next(&iterator);
    while (p != 0) {
        if (team_index_desired == p->team_index_desired) {
            return (uint32_t)(datum_index)iterator.index;
        }
        p = (player *)halo::memory::data_iterator_next(&iterator);
    }
    return halo::k_dword_none;
}

void ServerCommands::rcon_password(uint32_t argument_count, int32_t *arguments)
{
    if (argument_count == 0) {
    report:
        if (sv_rcon_password_value[0] == 0) {
            halo::interface::chimera__console_out((ColorARGB *)global_white_argb, halo::mutable_literal("sv_rcon_password: '' (rcon is DISABLED)"));
            return;
        }
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, halo::mutable_literal("sv_rcon_password: '%s'"), sv_rcon_password_value, strlen(sv_rcon_password_value));
        return;
    }
    if (argument_count == 1) {
        char *arg = (char *)arguments[0];
        if (strlen(arg) < 9) {
            strcpy(sv_rcon_password_value, arg);
            goto report;
        }
        halo::interface::chimera__console_out((ColorARGB *)global_white_argb, halo::mutable_literal("Maximum rcon password length is %d characters"), 8);
    }
    halo::interface::chimera__console_out((ColorARGB *)global_white_argb, halo::mutable_literal("Incorrect usage. Type help sv_rcon_password for more information."));
}

void ServerCommands::single_flag_force_reset(uint32_t argument_count, char **arguments)
{
    uint8_t old_value = network_single_flag_force_reset_value;
    uint8_t new_value = old_value;

    halo::networking::console_command_bool_get_set(argument_count, &new_value, arguments, "sv_single_flag_force_reset");

    if (new_value != old_value && halo::game::globals().current_engine != 0) {
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Game in progress...  Changes will apply to the next game."));
    }
    network_single_flag_force_reset_value = new_value;
}

void ServerCommands::status(void)
{
    if (network_game_mode == halo::networking::k_game_mode_host) {
        if (network_server != 0) {
            int32_t player_count_info = halo::game::players_active_count();
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Dedicated server is running on map %s (%d / %d players)"),
                                  network_build_string, player_count_info);
            if (halo::game::globals().state == _game_engine_state_not_started) {
                halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Use the 'sv_end_game' command to stop the game."));
                return;
            }
            halo::interface::chimera__console_out((ColorARGB *)console_message_default_color, halo::mutable_literal("Game is ending..."));
        }
        return;
    }
    halo::interface::chimera__console_out((ColorARGB *)global_white_argb, halo::mutable_literal("%s is a server-only function!"), "sv_status");
}

void ServerCommands::timelimit(uint32_t argument_count, int32_t *arguments)
{
    uint8_t changed = 0;

    if (argument_count != 0) {
        if (argument_count != 1) {
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_timelimit for more information."));
            return;
        }
        {
            char *arg = (char *)arguments[0];
            if (_stricmp(arg, "-1") == 0 || _stricmp(arg, "default") == 0) {
                sv_timelimit_minutes = -1;
                changed = 1;
            } else if (_stricmp(arg, "0") == 0 || _stricmp(arg, "infinite") == 0) {
                sv_timelimit_minutes = 0;
                changed = 1;
            } else {
                int32_t value = atol(arg);
                if (value < 1 || 599 < value) {
                    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_timelimit:  invalid parameter %s"), arg);
                    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_timelimit for more information."));
                    return;
                }
                changed = 1;
                sv_timelimit_minutes = value;
            }
        }
    }
    if (sv_timelimit_minutes == -1) {
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_timelimit: -1 = default"));
    } else if (sv_timelimit_minutes != 0) {
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_timelimit: %d minutes"), sv_timelimit_minutes);
        goto done;
    } else {
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_timelimit: 0 = infinite"));
    }
done:
    if (changed && halo::game::globals().current_engine != 0) {
        halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("   Game in progress...  Changes will apply to the next game."));
    }
}

void ServerCommands::tk_cooldown(uint32_t argument_count, int32_t *arguments)
{
    if (argument_count != 0) {
        if (argument_count != 1) {
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_tk_cooldown for more information."));
            return;
        }
        {
            int32_t seconds = halo::networking::parse_time_duration_string((char *)arguments[0], 's', (uint8_t *)sv_tk_grace_arg_buffer);
            if (seconds < 0) {
                halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_tk_cooldown for more information."));
                return;
            }
            sv_tk_cooldown_ticks = seconds * 30;
        }
    }
    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_tk_cooldown: %ds"), sv_tk_cooldown_ticks / 30);
}

void ServerCommands::tk_grace(uint32_t argument_count, int32_t *arguments)
{
    if (argument_count != 0) {
        if (argument_count != 1) {
            halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_tk_grace for more information."));
            return;
        }
        {
            int32_t seconds = halo::networking::parse_time_duration_string((char *)arguments[0], 's', (uint8_t *)sv_tk_grace_arg_buffer);
            if (seconds < 0) {
                halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("Incorrect usage. Type help sv_tk_grace for more information."));
                return;
            }
            sv_tk_grace_ticks = seconds * 30;
        }
    }
    halo::interface::chimera__console_out((ColorARGB *)0, halo::mutable_literal("sv_tk_grace: %ds"), sv_tk_grace_ticks / 30);
}

}  // namespace halo::networking

namespace halo::networking {

namespace {
constexpr ArgumentCommand<int32_t, &ServerCommands::ban> k_ban(ServerCommandId::ban);
constexpr ArgumentCommand<int32_t, &ServerCommands::ban_penalty> k_ban_penalty(ServerCommandId::ban_penalty);
constexpr ArgumentCommand<int32_t, &ServerCommands::banlist_file> k_banlist_file(ServerCommandId::banlist_file);
constexpr ArgumentCommand<int32_t, &ServerCommands::friendly_fire> k_friendly_fire(ServerCommandId::friendly_fire);
constexpr NameCommand<&ServerCommands::kick> k_kick(ServerCommandId::kick);
constexpr ArgumentCommand<uint16_t *, &ServerCommands::map> k_map(ServerCommandId::map);
constexpr PlainCommand<&ServerCommands::map_reset> k_map_reset(ServerCommandId::map_reset);
constexpr ArgumentCommand<int32_t, &ServerCommands::maxplayers> k_maxplayers(ServerCommandId::maxplayers);
constexpr ArgumentCommand<char *, &ServerCommands::name> k_name(ServerCommandId::name);
constexpr ArgumentCommand<char *, &ServerCommands::password> k_password(ServerCommandId::password);
constexpr PlainCommand<&ServerCommands::players> k_players(ServerCommandId::players);
constexpr ArgumentCommand<int32_t, &ServerCommands::rcon_password> k_rcon_password(ServerCommandId::rcon_password);
constexpr ArgumentCommand<char *, &ServerCommands::single_flag_force_reset> k_single_flag_force_reset(ServerCommandId::single_flag_force_reset);
constexpr PlainCommand<&ServerCommands::status> k_status(ServerCommandId::status);
constexpr ArgumentCommand<int32_t, &ServerCommands::timelimit> k_timelimit(ServerCommandId::timelimit);
constexpr ArgumentCommand<int32_t, &ServerCommands::tk_cooldown> k_tk_cooldown(ServerCommandId::tk_cooldown);
constexpr ArgumentCommand<int32_t, &ServerCommands::tk_grace> k_tk_grace(ServerCommandId::tk_grace);

constexpr const ServerCommand *k_commands[] = {
    &k_ban,
    &k_ban_penalty,
    &k_banlist_file,
    &k_friendly_fire,
    &k_kick,
    &k_map,
    &k_map_reset,
    &k_maxplayers,
    &k_name,
    &k_password,
    &k_players,
    &k_rcon_password,
    &k_single_flag_force_reset,
    &k_status,
    &k_timelimit,
    &k_tk_cooldown,
    &k_tk_grace,
};

static_assert(sizeof(k_commands) / sizeof(k_commands[0]) == static_cast<size_t>(ServerCommandId::count));
}  // namespace

/**
 * Returns the console name of a command.
 */
const char *server_command_name(ServerCommandId id)
{
    switch (id) {
    case ServerCommandId::ban:
        return "sv_ban";
    case ServerCommandId::ban_penalty:
        return "sv_ban_penalty";
    case ServerCommandId::banlist_file:
        return "sv_banlist_file";
    case ServerCommandId::friendly_fire:
        return "sv_friendly_fire";
    case ServerCommandId::kick:
        return "sv_kick";
    case ServerCommandId::map:
        return "sv_map";
    case ServerCommandId::map_reset:
        return "sv_map_reset";
    case ServerCommandId::maxplayers:
        return "sv_maxplayers";
    case ServerCommandId::name:
        return "sv_name";
    case ServerCommandId::password:
        return "sv_password";
    case ServerCommandId::players:
        return "sv_players";
    case ServerCommandId::rcon_password:
        return "sv_rcon_password";
    case ServerCommandId::single_flag_force_reset:
        return "sv_single_flag_force_reset";
    case ServerCommandId::status:
        return "sv_status";
    case ServerCommandId::timelimit:
        return "sv_timelimit";
    case ServerCommandId::tk_cooldown:
        return "sv_tk_cooldown";
    case ServerCommandId::tk_grace:
        return "sv_tk_grace";
    default:
        return "";
    }
}

/**
 * Returns the command object registered for `id`.
 */
const ServerCommand &ServerCommandRegistry::get(ServerCommandId id)
{
    return *k_commands[static_cast<size_t>(id)];
}

/**
 * Finds a command by its console name (for example "sv_kick"); returns null when no command has that name.
 */
const ServerCommand *ServerCommandRegistry::find(const char *command_name)
{
    for (const ServerCommand *command : k_commands) {
        if (strcmp(command->name(), command_name) == 0) {
            return command;
        }
    }
    return 0;
}

}  // namespace halo::networking

namespace halo::networking {
void sv_ban(uint32_t argument_count, int32_t *arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::ban).execute(argument_count, arguments);
}

void sv_ban_penalty(uint32_t argument_count, int32_t *arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::ban_penalty).execute(argument_count, arguments);
}

void sv_banlist_file(uint32_t argument_count, int32_t *arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::banlist_file).execute(argument_count, arguments);
}

network_player_entry * sv_find_client_by_name_or_index(char *name_or_index)
{
    return halo::networking::ServerCommands::find_client_by_name_or_index(name_or_index);
}

void sv_friendly_fire(uint32_t argument_count, int32_t *arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::friendly_fire).execute(argument_count, arguments);
}

void sv_kick(char *name_or_index)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::kick).execute(0, name_or_index);
}

void sv_map(uint32_t argument_count, uint16_t **arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::map).execute(argument_count, arguments);
}

void sv_map_reset(void)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::map_reset).execute(0, 0);
}

void sv_maxplayers(uint32_t argument_count, int32_t *arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::maxplayers).execute(argument_count, arguments);
}

void sv_name(uint32_t argument_count, char **arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::name).execute(argument_count, arguments);
}

void sv_password(uint32_t argument_count, char **arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::password).execute(argument_count, arguments);
}

void sv_players(void)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::players).execute(0, 0);
}

uint32_t sv_players_find_by_team_index_desired(int8_t team_index_desired)
{
    return halo::networking::ServerCommands::players_find_by_team_index_desired(team_index_desired);
}

void sv_rcon_password(uint32_t argument_count, int32_t *arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::rcon_password).execute(argument_count, arguments);
}

void sv_single_flag_force_reset(uint32_t argument_count, char **arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::single_flag_force_reset).execute(argument_count, arguments);
}

void sv_status(void)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::status).execute(0, 0);
}

void sv_timelimit(uint32_t argument_count, int32_t *arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::timelimit).execute(argument_count, arguments);
}

void sv_tk_cooldown(uint32_t argument_count, int32_t *arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::tk_cooldown).execute(argument_count, arguments);
}

void sv_tk_grace(uint32_t argument_count, int32_t *arguments)
{
    halo::networking::ServerCommandRegistry::get(halo::networking::ServerCommandId::tk_grace).execute(argument_count, arguments);
}

}
