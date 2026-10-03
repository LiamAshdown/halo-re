#pragma once

#include <cstdint>
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"

inline auto &unknown_00697ed8 = halo::link::ref<int32_t>(halo::networking::vars().unknown_00697ed8);
inline auto &unknown_006982e8 = halo::link::ref<int32_t>(halo::networking::vars().unknown_006982e8);

namespace halo::networking::net_state {

/**
 * Seconds after the game clock starts during which a channel that exceeded its activity timeout is still serviced
 * (the comparison is against the game tick count, 30 ticks per second); defaults to 3.
 *
 * @address 0x00697ed8
 */
inline int32_t &channel_timeout_grace_seconds = ::unknown_00697ed8;

/**
 * Connect progress percentage last reported by the client connection state machine; starts at 0xffff and is never read.
 *
 * @address 0x006982e8
 */
inline int32_t &last_connect_progress_percent = ::unknown_006982e8;

}
