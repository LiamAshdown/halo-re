#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Persistent ban list: add, load, save, print and the automatic ban of an offending player.
 */
class Banlist {
public:
    Banlist() = delete;

    static uint8_t add_ban(int32_t identity_lookup_key, int32_t duration_override_seconds, network_player_entry *target_player);
    static void load();
    static void print();
    static void save();
    static uint8_t autoban_player(datum_index player_handle);
};

}
