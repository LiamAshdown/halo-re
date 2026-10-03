#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Behaviour group for the original `network_session_*` functions; every member is the original function body moved unchanged.
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
