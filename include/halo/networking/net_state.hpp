#pragma once

#include <cstdint>
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"

static auto &channel_timeout_grace_seconds = halo::link::ref<int32_t>(halo::networking::vars().channel_timeout_grace_seconds);
static auto &last_connect_progress_percent = halo::link::ref<int32_t>(halo::networking::vars().last_connect_progress_percent);

namespace halo::networking::net_state {

/**
 * Seconds after the game clock starts during which a channel that exceeded its activity timeout is still serviced
 * (the comparison is against the game tick count, 30 ticks per second); defaults to 3.
 *
 * @address 0x00697ed8
 */
static int32_t &channel_timeout_grace_seconds = ::channel_timeout_grace_seconds;

/**
 * Connect progress percentage last reported by the client connection state machine; starts at 0xffff and is never read.
 *
 * @address 0x006982e8
 */
static int32_t &last_connect_progress_percent = ::last_connect_progress_percent;

}
