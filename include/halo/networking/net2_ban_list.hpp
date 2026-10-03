/**
 * @file include/halo/networking/net2_ban_list.hpp
 * Server ban list lookups.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Server ban list lookups.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class BanList {
public:
    /**
     * 0x496b50, EAX color (NULL = default) Looks up `key` (a CD-key hash) in the ban list; if found and either indefinite or not yet expired, prints a rejection message and returns 1 (reject), otherwise returns 0 (allow).
     *
     * @address 0x4e3820
     */
    static uint8_t check_and_reject_player(char *key);

    /**
     * 0x006b859c, element_size 0x38; see network_banlist_save.c Linear-searches the ban list for an entry whose stored CD-key hash (+0x0d) case-insensitively matches `key`. Returns the matching entry, or 0 if none matches.
     *
     * @address 0x4e37d0
     */
    static ban_list_entry * find_by_name(char *key);

    /**
     * 0x4cf810, ESI -> array Finds the existing ban-list entry for `cd_key_hash`, or appends and initializes a new one storing `name` (up to 12 characters) and `cd_key_hash` (up to 32 characters). Returns 0 if the list could not grow.
     *
     * @address 0x4e3890
     */
    static ban_list_entry * get_or_add_entry(char *name, char *cd_key_hash);

};

}  // namespace halo::networking
