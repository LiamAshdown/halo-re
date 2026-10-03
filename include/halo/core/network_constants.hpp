#pragma once

#include <cstdint>

namespace halo {

/** Size of the shared network message scratch buffer every message_delta_encode_message call writes into. */
inline constexpr uint32_t k_network_message_scratch_size = 0x7ff8;

}  // namespace halo
