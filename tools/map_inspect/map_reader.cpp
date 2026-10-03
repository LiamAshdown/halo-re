#include "halo/cache/map_reader.hpp"

#include <cstdio>
#include <cstring>

namespace halo::cache {

namespace {

/** Writes the four group characters of a stored group code (little-endian bytes spell the text backwards). */
std::string group_text(uint32_t group)
{
    if (group == 0xffffffffu) {
        return {};
    }
    char text[5] = { static_cast<char>(group >> 24), static_cast<char>(group >> 16), static_cast<char>(group >> 8), static_cast<char>(group), 0 };
    return std::string(text);
}

}  // namespace

/**
 * Reads the header, checks it, then loads the tag data block and resolves every tag record against it. A stored address
 * becomes an offset into the tag data block by subtracting k_map_tag_data_base; anything that falls outside the block is
 * reported instead of followed.
 */
MapError MapFile::open(const std::string &path)
{
    std::FILE *file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) {
        return MapError::cannot_open;
    }
    struct Closer {
        std::FILE *file;
        ~Closer() { std::fclose(file); }
    } closer{ file };

    if (std::fread(&header_, 1, sizeof(header_), file) != sizeof(header_)) {
        return MapError::short_header;
    }
    if (MapError error = validate_map_header(header_); error != MapError::none) {
        return error;
    }

    std::fseek(file, 0, SEEK_END);
    disk_size_ = static_cast<uint64_t>(std::ftell(file));
    if (disk_size_ != static_cast<uint64_t>(header_.file_size)) {
        return MapError::size_mismatch;
    }

    if (static_cast<uint64_t>(header_.tag_data_offset) + header_.tag_data_size > disk_size_ || header_.tag_data_size < k_map_tag_header_size) {
        return MapError::cannot_read_tag_data;
    }
    tag_data_.resize(header_.tag_data_size);
    std::fseek(file, static_cast<long>(header_.tag_data_offset), SEEK_SET);
    if (std::fread(tag_data_.data(), 1, tag_data_.size(), file) != tag_data_.size()) {
        return MapError::cannot_read_tag_data;
    }

    std::memcpy(&tag_header_, tag_data_.data(), sizeof(tag_header_));
    if (tag_header_.tag_count < 0 || tag_header_.tags_address < k_map_tag_data_base) {
        return MapError::bad_tag_header;
    }

    const uint64_t table_offset = tag_header_.tags_address - k_map_tag_data_base;
    const uint64_t table_end = table_offset + static_cast<uint64_t>(tag_header_.tag_count) * k_map_tag_record_size;
    if (table_end > tag_data_.size()) {
        return MapError::bad_tag_table;
    }

    tags_.clear();
    tags_.reserve(static_cast<size_t>(tag_header_.tag_count));
    for (int32_t index = 0; index < tag_header_.tag_count; index++) {
        MapTagRecord record;
        std::memcpy(&record, tag_data_.data() + table_offset + static_cast<uint64_t>(index) * k_map_tag_record_size, sizeof(record));

        MapTag tag;
        tag.group = group_text(record.group);
        tag.tag_id = record.tag_id;
        if (record.path_address >= k_map_tag_data_base && record.path_address - k_map_tag_data_base < tag_data_.size()) {
            const char *text = reinterpret_cast<const char *>(tag_data_.data() + (record.path_address - k_map_tag_data_base));
            const size_t room = tag_data_.size() - (record.path_address - k_map_tag_data_base);
            tag.path.assign(text, strnlen(text, room));
        }
        if (record.data_address >= k_map_tag_data_base && record.data_address - k_map_tag_data_base < tag_data_.size()) {
            tag.data_offset = record.data_address - k_map_tag_data_base;
        }
        tags_.push_back(std::move(tag));
    }
    return MapError::none;
}

/**
 * Linear search of the tag table, the same lookup the engine performs by group and path.
 */
const MapTag *MapFile::find(std::string_view group, std::string_view path) const
{
    for (const MapTag &tag : tags_) {
        if (tag.group == group && tag.path == path) {
            return &tag;
        }
    }
    return nullptr;
}

/**
 * Counts the tags that belong to `group`.
 */
size_t MapFile::count_group(std::string_view group) const
{
    size_t count = 0;
    for (const MapTag &tag : tags_) {
        count += (tag.group == group) ? 1 : 0;
    }
    return count;
}

}  // namespace halo::cache
