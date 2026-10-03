/**
 * @file include/halo/cache/layout.hpp
 * Named sizes of the cache module records and slot limits, with the layout checks that prove them.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include "cache.h"
#include "halo/cache/data_map_file.hpp"
#include "halo/cache/map_file.hpp"

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
static_assert(sizeof(cache_file_header) == sizeof(MapFileHeader));
static_assert(offsetof(cache_file_header, head) == offsetof(MapFileHeader, head) && offsetof(cache_file_header, version) == offsetof(MapFileHeader, version) &&
              offsetof(cache_file_header, file_size) == offsetof(MapFileHeader, file_size) && offsetof(cache_file_header, tag_data_offset) == offsetof(MapFileHeader, tag_data_offset) &&
              offsetof(cache_file_header, tag_data_size) == offsetof(MapFileHeader, tag_data_size) && offsetof(cache_file_header, name) == offsetof(MapFileHeader, name) &&
              offsetof(cache_file_header, map_type) == offsetof(MapFileHeader, map_type) && offsetof(cache_file_header, crc32) == offsetof(MapFileHeader, crc32) &&
              offsetof(cache_file_header, foot) == offsetof(MapFileHeader, foot));

/** True when the engine's copy of a map header passes the shared map-file checks (signatures, size range, name, version 7). */
inline bool map_header_valid(const cache_file_header &header)
{
    return validate_map_header(reinterpret_cast<const MapFileHeader &>(header)) == MapError::none;
}
static_assert(sizeof(cache_file_slot) == 0x80c);
static_assert(offsetof(data_file, file_id) == offsetof(DataFileHeader, file_id) && offsetof(data_file, data_offset) == offsetof(DataFileHeader, names_offset) &&
              offsetof(data_file, table_offset) == offsetof(DataFileHeader, table_offset) && offsetof(data_file, entry_count) == offsetof(DataFileHeader, entry_count),
              "the first 0x10 bytes of data_file are the on-disk DataFileHeader");
static_assert(sizeof(data_file_reference) == sizeof(DataFileEntry) && offsetof(data_file_reference, size) == offsetof(DataFileEntry, size) &&
              offsetof(data_file_reference, file_offset) == offsetof(DataFileEntry, file_offset));
static_assert(sizeof(cache_io_request) * k_cache_io_request_count == 0x6000);

}  // namespace halo::cache
