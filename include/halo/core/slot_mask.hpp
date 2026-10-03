#pragma once

namespace halo {

/** Mask that extracts the array slot from a datum handle, as the plain int the engine's index arithmetic uses (see k_datum_slot_mask for the unsigned form). */
inline constexpr int k_slot_mask = 0xffff;

}  // namespace halo
