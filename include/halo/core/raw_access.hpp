/**
 * @file include/halo/core/raw_access.hpp
 * Byte-offset access into a record whose layout is not (yet) a named struct member.
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace halo {

/** Reference to a T located `offset` bytes after `base`. Every use is a record field still waiting for a name. */
template <typename T>
inline T &raw_at(void *base, std::size_t offset) noexcept
{
    return *reinterpret_cast<T *>(static_cast<uint8_t *>(base) + offset);
}

}  // namespace halo
