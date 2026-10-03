/**
 * @file include/halo/interface/net_session.hpp
 * The network game session record the lobby screens read: the host's own session while hosting, the client's copy otherwise.
 */
#pragma once

#include "networking.h"
#include "halo/networking/api.hpp"

namespace halo::interface {

/** The session of the hosted game, else the session the client joined, else null. */
inline network_game_session *current_game_session() {
    auto &networking = halo::networking::globals();
    if (networking.server != nullptr) {
        return &networking.server->session;
    }
    return networking.client != nullptr ? &networking.client->session : nullptr;
}

}  // namespace halo::interface
