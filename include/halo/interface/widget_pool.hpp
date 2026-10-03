/**
 * @file include/halo/interface/widget_pool.hpp
 * Typed helpers over the widget heap (the arena every widget instance, text block and history node is allocated from).
 */
#pragma once

#include <stdint.h>

#include "memory.h"
#include "halo/interface/api.hpp"
#include "halo/interface/constants.hpp"
#include "halo/memory/api.hpp"

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/** Size in bytes of the block header in front of every payload of the widget heap. */
inline constexpr uint32_t k_pool_block_header_size = 0x10;

/** The heap block header of a payload allocated from the widget heap. */
inline heap_block *widget_pool_block(void *payload) {
    return reinterpret_cast<heap_block *>(static_cast<uint8_t *>(payload) - k_pool_block_header_size);
}

/** Resizes (or first allocates, for a null `old_payload`) a text block of the widget heap and returns it as UTF-16 text; null when the heap is full. */
inline uint16_t *widget_pool_resize_text(void *old_payload, uint32_t bytes) {
    return static_cast<uint16_t *>(halo::memory::heap_reallocate(old_payload, bytes, globals().widget_memory_pool));
}

/** Allocates a text block of the widget heap and returns it as UTF-16 text; null when the heap is full. */
inline uint16_t *widget_pool_allocate_text(uint32_t bytes) {
    return static_cast<uint16_t *>(halo::memory::heap_allocate(bytes, globals().widget_memory_pool));
}

/** Unlinks the block of `payload` from the widget heap and takes its size and count out of the heap statistics. */
inline void widget_pool_free(void *payload) {
    heap *pool = globals().widget_memory_pool;
    heap_block *block = widget_pool_block(payload);
    uint32_t size = block->size;

    halo::memory::heap_unlink_block(block, pool);
    pool->bytes_allocated -= static_cast<int32_t>(size & k_pool_block_size_mask);
    pool->allocation_count -= 1;
}

}  // namespace halo::interface
