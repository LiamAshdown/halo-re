/**
 * @file include/halo/cache/map_file.hpp
 * The on-disk layout of Halo PC cache (.map) files, independent of the engine: header, tag index header and tag records, plus
 * the checks the engine applies when it opens a map. Header-only and free of STL so the engine and offline tools share it.
 */
#pragma once

#include <cstdint>
#include <cstring>

namespace halo::cache {

/** Signatures, limits and fixed sizes of the map file format (retail PC, format version 7). */
inline constexpr uint32_t k_map_head_signature = 0x68656164;       // "head" at +0x000
inline constexpr uint32_t k_map_foot_signature = 0x666f6f74;       // "foot" at +0x7fc
inline constexpr int32_t k_map_format_version = 7;
inline constexpr uint32_t k_map_header_size = 0x800;
inline constexpr int32_t k_map_maximum_size = 0x18000000;          // 384 MB
inline constexpr uint32_t k_map_name_capacity = 0x20;
inline constexpr uint32_t k_map_tag_data_base = 0x40440000;        // address the tag data block is relocated to
inline constexpr uint32_t k_map_tag_header_size = 0x28;
inline constexpr uint32_t k_map_tag_record_size = 0x20;

/** Which kind of scenario the map holds; the engine uses it to pick the cache slot class. */
enum class MapType : int16_t {
    single_player = 0,
    multiplayer = 1,
    user_interface = 2,
};

/**
 * The 0x800-byte header at the start of every map file. Fields the engine never reads keep their position as padding.
 * `tag_data_offset` and `tag_data_size` locate the tag data block that the engine reads to k_map_tag_data_base.
 */
struct MapFileHeader {
    uint32_t head;               // 0x000 "head"
    int32_t version;             // 0x004 7 for retail PC
    int32_t file_size;           // 0x008 size of the whole file in bytes
    uint32_t reserved_00c;       // 0x00c
    uint32_t tag_data_offset;    // 0x010 file offset of the tag data block
    uint32_t tag_data_size;      // 0x014 bytes of the tag data block
    uint32_t reserved_018[2];    // 0x018
    char name[32];               // 0x020 scenario name, NUL terminated
    char build[32];              // 0x040 build string such as "01.00.00.0564"
    int16_t map_type;            // 0x060 a MapType
    int16_t reserved_062;        // 0x062
    uint32_t crc32;              // 0x064 checksum recorded for saved-game matching
    uint8_t reserved_068[0x794]; // 0x068
    uint32_t foot;               // 0x7fc "foot"
};
static_assert(sizeof(MapFileHeader) == k_map_header_size);

/** The first 0x28 bytes of the tag data block (the "tag index header"). Pointers are relative to k_map_tag_data_base. */
struct MapTagHeader {
    uint32_t tags_address;         // 0x00 address of the tag record table once relocated
    uint32_t scenario_tag;         // 0x04 tag id of the scenario
    uint32_t checksum;             // 0x08
    int32_t tag_count;             // 0x0c number of tag records
    uint32_t model_part_count;     // 0x10
    uint32_t model_data_offset;    // 0x14 file offset of the model data block
    uint32_t model_triangle_count; // 0x18
    uint32_t model_index_offset;   // 0x1c offset of the index data inside the model data block
    uint32_t model_data_size;      // 0x20 bytes of the model data block
    uint32_t signature;            // 0x24 "tags"
};
static_assert(sizeof(MapTagHeader) == k_map_tag_header_size);

/** One 0x20-byte record of the tag table. `path_address` and `data_address` are relocated addresses. */
struct MapTagRecord {
    uint32_t group;              // 0x00 four-character group code (big-endian text, e.g. "scnr")
    uint32_t parent_group;       // 0x04 0xffffffff when none
    uint32_t grandparent_group;  // 0x08 0xffffffff when none
    uint32_t tag_id;             // 0x0c salt << 16 | index
    uint32_t path_address;       // 0x10
    uint32_t data_address;       // 0x14
    uint32_t reserved_18[2];     // 0x18
};
static_assert(sizeof(MapTagRecord) == k_map_tag_record_size);

/** Why a map was rejected. */
enum class MapError {
    none,
    cannot_open,
    short_header,
    bad_head_signature,
    bad_foot_signature,
    bad_size,
    name_too_long,
    bad_version,
    size_mismatch,
    cannot_read_tag_data,
    bad_tag_header,
    bad_tag_table,
};

/** Human-readable text for an error, for tools and logs. */
inline const char *map_error_text(MapError error)
{
    switch (error) {
    case MapError::none: return "ok";
    case MapError::cannot_open: return "cannot open the file";
    case MapError::short_header: return "file is shorter than the 0x800-byte header";
    case MapError::bad_head_signature: return "header does not start with \"head\"";
    case MapError::bad_foot_signature: return "header does not end with \"foot\"";
    case MapError::bad_size: return "file_size is outside 0..384 MB";
    case MapError::name_too_long: return "scenario name is not NUL-terminated within 32 bytes";
    case MapError::bad_version: return "format version is not 7";
    case MapError::size_mismatch: return "file_size does not match the size of the file on disk";
    case MapError::cannot_read_tag_data: return "tag data block lies outside the file";
    case MapError::bad_tag_header: return "tag index header is invalid";
    case MapError::bad_tag_table: return "tag table lies outside the tag data block";
    }
    return "unknown error";
}

/**
 * The checks the engine applies when it opens a map: both signatures, 0 <= file_size <= 384 MB, a name shorter than 32
 * characters and format version 7. Shared by the engine's loader and the offline reader so both accept the same files.
 */
inline MapError validate_map_header(const MapFileHeader &header)
{
    if (header.head != k_map_head_signature) {
        return MapError::bad_head_signature;
    }
    if (header.foot != k_map_foot_signature) {
        return MapError::bad_foot_signature;
    }
    if (header.file_size < 0 || header.file_size > k_map_maximum_size) {
        return MapError::bad_size;
    }
    if (std::memchr(header.name, 0, k_map_name_capacity) == nullptr) {
        return MapError::name_too_long;
    }
    if (header.version != k_map_format_version) {
        return MapError::bad_version;
    }
    return MapError::none;
}

/** Packs a four-character group code the way the tag table stores it. */
constexpr uint32_t map_group_code(const char (&text)[5])
{
    return (static_cast<uint32_t>(static_cast<uint8_t>(text[0])) << 24) | (static_cast<uint32_t>(static_cast<uint8_t>(text[1])) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(text[2])) << 8) | static_cast<uint32_t>(static_cast<uint8_t>(text[3]));
}

}  // namespace halo::cache
