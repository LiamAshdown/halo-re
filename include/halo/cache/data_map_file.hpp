/**
 * @file include/halo/cache/data_map_file.hpp
 * The on-disk layout of Halo PC's shared resource files, bitmaps.map and sounds.map, independent of the engine.
 *
 * Both files are laid out the same way: a 16-byte header, the raw resource payloads back to back from offset 0x10, then a
 * block of NUL-terminated resource names, then a table of 12-byte entries that locate each payload and name. Map files
 * refer to a payload by its position in that table (see halo::cache::cache_io, data file index 1 for bitmaps and 2 for sounds).
 */
#pragma once

#include <cstdint>

namespace halo::cache {

/** Value of DataFileHeader::file_id for each shared resource file. */
inline constexpr int32_t k_data_file_id_bitmaps = 1;
inline constexpr int32_t k_data_file_id_sounds = 2;

/** Offset of the first payload; the 16-byte header comes first. */
inline constexpr uint32_t k_data_file_payload_offset = 0x10;

/** The 16-byte header at the start of bitmaps.map and sounds.map. */
struct DataFileHeader {
    int32_t file_id;        // 0x00 1 for bitmaps.map, 2 for sounds.map
    uint32_t names_offset;  // 0x04 file offset of the name block; also the end of the payloads
    uint32_t table_offset;  // 0x08 file offset of the entry table; also the end of the name block
    uint32_t entry_count;   // 0x0c number of entries in the table
};
static_assert(sizeof(DataFileHeader) == 0x10);

/** One 12-byte entry of the resource table. */
struct DataFileEntry {
    uint32_t name_offset;   // 0x00 offset of the resource name inside the name block
    uint32_t size;          // 0x04 payload size in bytes
    uint32_t file_offset;   // 0x08 file offset of the payload
};
static_assert(sizeof(DataFileEntry) == 0x0c);

/** Why a shared resource file was rejected. */
enum class DataFileError {
    none,
    cannot_open,
    short_header,
    wrong_file_id,
    bad_layout,
    bad_table,
};

/** Human-readable text for an error, for tools and logs. */
inline const char *data_file_error_text(DataFileError error)
{
    switch (error) {
    case DataFileError::none: return "ok";
    case DataFileError::cannot_open: return "cannot open the file";
    case DataFileError::short_header: return "file is shorter than the 16-byte header";
    case DataFileError::wrong_file_id: return "file id does not match the expected resource file";
    case DataFileError::bad_layout: return "payload, name and table regions do not fit the file";
    case DataFileError::bad_table: return "resource table entries are not contiguous or point outside the payload region";
    }
    return "unknown error";
}

/**
 * The check the engine makes when it opens a shared resource file: the file id must be the one expected for that file
 * (halo::cache::data_file_view::read_header). The offline reader adds the layout checks below on top.
 */
inline bool data_file_id_matches(const DataFileHeader &header, int32_t expected_file_id)
{
    return header.file_id == expected_file_id;
}

/**
 * Layout check for a header against the size of the file it came from: payloads start at 0x10 and end at names_offset,
 * names end at table_offset, and the table of entry_count entries ends exactly at the end of the file.
 */
inline bool data_file_layout_valid(const DataFileHeader &header, uint64_t file_size)
{
    return header.names_offset >= k_data_file_payload_offset && header.names_offset <= header.table_offset &&
           static_cast<uint64_t>(header.table_offset) + static_cast<uint64_t>(header.entry_count) * sizeof(DataFileEntry) == file_size;
}

}  // namespace halo::cache
