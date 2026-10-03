/**
 * @file include/halo/cache/layout.hpp
 * Named sizes of the cache module records and slot limits, with the layout checks that prove them.
 */
#pragma once

#include <cstdint>
#include "cache.h"

namespace halo::cache {

/** Number of asynchronous read requests the cache io queue holds (the queue is 0x6000 bytes). */
inline constexpr int32_t k_cache_io_request_count = 0x200;

/** Bytes of a data_file record data_files::zero clears before it is opened. */
inline constexpr uint32_t k_data_file_cleared_bytes = 0x40;

/** Smallest size the sound decode buffer is allocated with. */
inline constexpr uint32_t k_sound_decode_buffer_minimum_size = 0x100000;

/** Size limits of the cache file slots: the first two slots take any map, the third a small one, the rest 128 MB. */
inline constexpr int32_t k_cache_file_slot_limit_third = 0x02300000;
inline constexpr int32_t k_cache_file_slot_limit_rest = 0x08000000;

static_assert(sizeof(cache_file_header) == k_cache_file_header_size);
static_assert(sizeof(cache_file_slot) == 0x80c);
static_assert(sizeof(cache_io_request) * k_cache_io_request_count == 0x6000);

}  // namespace halo::cache
