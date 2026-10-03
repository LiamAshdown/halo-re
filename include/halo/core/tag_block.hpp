#pragma once

#include <cstddef>
#include <cstdint>

namespace halo {

/** The first element of a loaded tag block (reflexive), viewed as T. */
template <typename T, typename Block>
inline T *tag_block_elements(const Block &block) {
    return reinterpret_cast<T *>(static_cast<uintptr_t>(block.pointer));
}

/** Element `index` of a loaded tag block (reflexive), viewed as T; the index is not range checked. */
template <typename T, typename Block>
inline T &tag_block_at(const Block &block, std::ptrdiff_t index) {
    return tag_block_elements<T>(block)[index];
}

}  // namespace halo
