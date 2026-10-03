#include "halo/networking/net1_banlist.hpp"
#include "halo/text/api.hpp"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "halo/networking/api.hpp"

extern "C" {
extern int32_t network_console_connection_id;
extern int32_t sv_ban_penalty_seconds[4];
extern char *gcd_getkeyhash(int32_t connection_id, int32_t identity_lookup_key);
extern void chimera__console_out(ColorARGB *color, char *format, ...);
extern char network_banlist_full_path[0x104];
extern char network_ban_file_read_mode_string[];
extern char network_ban_indefinite_marker[];
extern growable_array ban_list;
extern void *console_color_00685214;
extern void *actor_mode_default_look_weights;
extern data_array *player_data;
extern network_server_globals *network_server;
}

namespace halo::networking {

/**
 * Bans target_player: resolves their CD-key hash and display name, adds or extends their ban
 * list entry (escalating through sv_ban_penalty_seconds when duration_override_seconds is 0,
 * or indefinitely once the tier table runs out at 4), and saves the list.
 *
 * @address 0x4e35c0
 */
uint8_t Banlist::add_ban(int32_t identity_lookup_key, int32_t duration_override_seconds, network_player_entry *target_player)
{
    char *cd_key_hash;
    char player_name[24];
    ban_list_entry *entry;
    time_t now;
    time_t expiry;
    char time_buf[32];
    char date_buf[32];

    cd_key_hash = gcd_getkeyhash(network_console_connection_id, identity_lookup_key);
    if (cd_key_hash == 0 || *cd_key_hash == 0) {
        return 1;
    }
    halo::text::string_convert_unicode_to_ascii(reinterpret_cast<uint8_t *>(player_name), reinterpret_cast<uint16_t *>(target_player), 0x18);
    player_name[0xc] = 0;
    halo::networking::network_banlist_load();
    entry = halo::networking::ban_list_get_or_add_entry(player_name, cd_key_hash);
    if (entry != 0) {
        if (duration_override_seconds == 0) {
            if (entry->ban_count < 4) {
                duration_override_seconds = sv_ban_penalty_seconds[entry->ban_count];
            } else {
                duration_override_seconds = -1;
            }
        }
        entry->ban_count = entry->ban_count + 1;
        if (duration_override_seconds == -1) {
            entry->indefinite = 1;
            entry->expiry_time = 0;
            chimera__console_out((ColorARGB *)0, (char *)"Banning %s (%s) indefinitely.", player_name, cd_key_hash);
            halo::networking::network_banlist_save();
            return 1;
        }
        time(&now);
        expiry = now + duration_override_seconds;
        entry->expiry_time = expiry;
        entry->indefinite = 0;
        halo::networking::format_local_time_and_date(date_buf, 0x20, expiry, time_buf);
        chimera__console_out((ColorARGB *)0, (char *)"Banning %s (%s) until %s %s.", player_name, cd_key_hash, date_buf, time_buf);
        halo::networking::network_banlist_save();
    }
    return 1;
}

/**
 * Reloads the server's ban list from banned<suffix>.txt (see sv_banlist_file / network_banlist_save
 * for the companion writer). Each non-comment line is "name,cd_key_hash,ban_count,expiry", where
 * the expiry is either the literal "--" (indefinite) or "YYYY-MM-DD HH:MM:SS".
 *
 * @address 0x4e3160
 */
void Banlist::load()
{
    FILE *file;
    char line[0x200];
    char *comma;
    char *name_ptr;
    char *hash_ptr;
    char *rest;
    char *count_end;
    char *date_or_dash;
    char date_token[32];
    char time_token[32];
    struct tm parsed_time;
    time_t expiry;
    long ban_count;
    ban_list_entry *entry;

    file = (FILE *)fopen(halo::networking::network_log_path_resolve(network_banlist_full_path),
                                 network_ban_file_read_mode_string);
    if (file == 0) {
        return;
    }
    while (fgets(line, 0x200, file) != 0) {
        if (line[0] == '#') {
            continue;
        }
        comma = strchr(line, ',');
        if (comma == 0) {
            continue;
        }
        *comma = 0;
        name_ptr = line;
        hash_ptr = comma + 1;
        rest = strchr(hash_ptr, ',');
        if (rest != 0) {
            *rest = 0;
            rest = rest + 1;
        }
        halo::networking::string_trim_whitespace(&name_ptr);
        halo::networking::string_trim_whitespace(&hash_ptr);
        entry = halo::networking::ban_list_get_or_add_entry(name_ptr, hash_ptr);
        if (entry == 0) {
            continue;
        }
        if (rest == 0) {
            entry->indefinite = 1;
            continue;
        }
        count_end = strchr(rest, ',');
        if (count_end == 0) {
            continue;
        }
        *count_end = 0;
        ban_count = atol(rest);
        entry->ban_count = (int16_t)ban_count;
        date_or_dash = count_end + 1;
        if (strncmp(date_or_dash, network_ban_indefinite_marker, 3) == 0) {
            entry->indefinite = 1;
            continue;
        }
        if (sscanf(date_or_dash, "%32s %32s", date_token, time_token) == 2) {
            memset(&parsed_time, 0, sizeof(parsed_time));
            if (sscanf(time_token, "%02d:%02d:%02d", &parsed_time.tm_hour, &parsed_time.tm_min, &parsed_time.tm_sec) == 3 &&
                sscanf(date_token, "%04d-%02d-%02d", &parsed_time.tm_year, &parsed_time.tm_mon, &parsed_time.tm_mday) == 3) {
                parsed_time.tm_year -= 0x76c;
                parsed_time.tm_mon -= 1;
                parsed_time.tm_isdst = -1;
                expiry = mktime(&parsed_time);
                if (expiry != -1) {
                    entry->expiry_time = (int32_t)expiry;
                    continue;
                }
            }
        }
        entry->expiry_time = 0;
        entry->indefinite = 1;
    }
    fclose(file);
}

/**
 * Prints "[Num Bans Name]" followed by rows of up to three "[index ban_count name]" entries
 * each, until every ban-list entry has been listed.
 *
 * @address 0x4e34e0
 */
void Banlist::print()
{
    ban_list_entry *entries = (ban_list_entry *)ban_list.data;
    int32_t i = 0;
    char entry_text[63];
    char line[257];

    chimera__console_out((ColorARGB *)console_color_00685214, (char *)"[Num Bans Name]");
    if (0 < ban_list.count) {
        do {
            int32_t in_line = 0;
            line[0] = 0;
            do {
                if (ban_list.count <= i) {
                    break;
                }
                sprintf(entry_text, "[%-4d %2d %*s]", i, entries[i].ban_count, 0xc, entries[i].name);
                strcat(line, entry_text);
                in_line = in_line + 1;
                i = i + 1;
            } while (in_line < 3);
            chimera__console_out((ColorARGB *)actor_mode_default_look_weights, line);
        } while (i < ban_list.count);
    }
}

/**
 * see sv_ban.c / sv_kick.c
 * Auto-bans and disconnects player_handle (used by server-side logic such as excessive
 * team-killing): finds the live player, finds the session machine slot whose machine_id matches
 * its low byte, refuses if that machine's channel is already connected/being torn down, then logs
 * the ban, adds it to the ban list, and kicks the machine.
 *
 * @address 0x4e36e0
 */
uint8_t Banlist::autoban_player(datum_index player_handle)
{
    int16_t index;
    int16_t salt;
    player *target_player;
    int8_t machine_key;
    network_machine *machine;
    int i;

    if (player_handle == k_datum_index_none) {
        return 0;
    }
    index = (int16_t)player_handle;
    if (index < 0) {
        return 0;
    }
    if (index >= player_data->maximum_count) {
        return 0;
    }
    target_player = (player *)((uint8_t *)player_data->data + (int32_t)player_data->size * (int32_t)index);
    if (target_player->identifier == 0) {
        return 0;
    }
    salt = (int16_t)(player_handle >> 16);
    if (salt != 0 && target_player->identifier != salt) {
        return 0;
    }
    machine_key = *(int8_t *)&target_player->machine_index;
    machine = 0;
    for (i = 0; i < k_network_maximum_machines; i++) {
        if (network_server->machines[i].machine_id == machine_key) {
            machine = &network_server->machines[i];
            break;
        }
    }
    if (machine != 0 && machine->channel != 0 && machine->channel->connected != 0) {
        return 0;
    }
    chimera__console_out((ColorARGB *)0, (char *)"AUTOBAN: Banning %S.", target_player->name);
    if (!halo::networking::network_banlist_add_ban(machine->gcd_user_id, 0, (network_player_entry *)target_player->name)) {
        return 0;
    }
    if (!halo::networking::network_server_notify_or_resend_challenge(6, machine, network_server)) {
        return 0;
    }
    return 1;
}

}
