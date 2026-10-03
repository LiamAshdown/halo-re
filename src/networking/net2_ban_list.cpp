/**
 * @file src/networking/net2_ban_list.cpp
 * Server ban list lookups.
 */
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <time.h>
#include "crt.h"
#include <string.h>
#include "halo/networking/net2_ban_list.hpp"
#include "halo/memory/api.hpp"
#include "halo/networking/api.hpp"

extern "C" {
extern void chimera__console_out(ColorARGB *color, char *format, ...);
extern growable_array ban_list;
}


namespace halo::networking {

uint8_t BanList::check_and_reject_player(char *key)
{
    ban_list_entry *entry;
    time_t now;

    entry = halo::networking::ban_list_find_by_name(key);
    if (entry != 0) {
        if (entry->indefinite == 0) {
            time(&now);
            if (now < entry->expiry_time) {
                return 0;
            }
        }
        chimera__console_out((ColorARGB *)0, (char *)"Rejecting banned player %s (%s).", entry, key);
        return 1;
    }
    return 0;
}

ban_list_entry * BanList::find_by_name(char *key)
{
    ban_list_entry *entries = (ban_list_entry *)ban_list.data;
    int32_t row = 0;
    int32_t i;

    if (0 < ban_list.count) {
        i = 0;
        do {
            if (_stricmp(entries[i].cd_key_hash, key) == 0) {
                return &entries[i];
            }
            row = row + 1;
            i = i + 1;
        } while (row < ban_list.count);
    }
    return 0;
}

ban_list_entry * BanList::get_or_add_entry(char *name, char *cd_key_hash)
{
    ban_list_entry *entry;
    int32_t index;

    entry = halo::networking::ban_list_find_by_name(cd_key_hash);
    if (entry != 0) {
        return entry;
    }
    index = halo::memory::growable_array_add_element(&ban_list);
    if (index != -1) {
        entry = &((ban_list_entry *)ban_list.data)[index];
        strncpy(entry->name, name, 0xc);
        entry->name[0xc] = 0;
        strncpy(entry->cd_key_hash, cd_key_hash, 0x20);
        entry->cd_key_hash[0x20] = 0;
        entry->indefinite = 0;
        entry->ban_count = 0;
        entry->expiry_time = 0;
        return entry;
    }
    return 0;
}

}  // namespace halo::networking

namespace halo::networking {
uint8_t ban_list_check_and_reject_player(char *key)
{
    return halo::networking::BanList::check_and_reject_player(key);
}

ban_list_entry * ban_list_find_by_name(char *key)
{
    return halo::networking::BanList::find_by_name(key);
}

ban_list_entry * ban_list_get_or_add_entry(char *name, char *cd_key_hash)
{
    return halo::networking::BanList::get_or_add_entry(name, cd_key_hash);
}

}
