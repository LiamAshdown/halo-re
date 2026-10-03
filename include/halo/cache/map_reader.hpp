/**
 * @file include/halo/cache/map_reader.hpp
 * Offline reader for Halo PC cache (.map) files: loads and validates a map from disk and resolves its tag table. Uses the
 * STL, so it is for tools and tests, not for the engine itself (which reads maps through halo::cache::cache_files).
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "halo/cache/data_map_file.hpp"
#include "halo/cache/map_file.hpp"

namespace halo::cache {

/** A tag table entry resolved against the loaded tag data block. */
struct MapTag {
    std::string group;            // four characters, e.g. "scnr"
    std::string path;             // tag path without the group extension
    uint32_t tag_id = 0;
    uint32_t data_offset = 0;     // offset of the tag structure inside the tag data block (0 when the tag has no data)
};

/**
 * A map file loaded from disk: the validated header, the tag data block and the resolved tag table. Reads only the
 * regions the engine itself reads (header and tag data); bitmaps, sounds and model data stay in the file.
 */
class MapFile {
public:
    /** Opens, reads and validates `path`; returns the failure reason or MapError::none. */
    MapError open(const std::string &path);

    const MapFileHeader &header() const { return header_; }
    const MapTagHeader &tag_header() const { return tag_header_; }
    const std::vector<MapTag> &tags() const { return tags_; }
    uint64_t file_size_on_disk() const { return disk_size_; }

    /** The tag whose group and path match, or nullptr. */
    const MapTag *find(std::string_view group, std::string_view path) const;

    /** Number of tags of a group. */
    size_t count_group(std::string_view group) const;

    /** The tag data block (header, tag table, paths and tag structures). */
    const std::vector<uint8_t> &tag_data() const { return tag_data_; }

private:
    MapFileHeader header_{};
    MapTagHeader tag_header_{};
    std::vector<uint8_t> tag_data_;
    std::vector<MapTag> tags_;
    uint64_t disk_size_ = 0;
};

/** One resource of a shared resource file with its name resolved. */
struct DataFileResource {
    std::string name;
    uint32_t size = 0;
    uint32_t file_offset = 0;
};

/**
 * A shared resource file (bitmaps.map or sounds.map) loaded from disk: the header, the name block and the resolved entry
 * table. The payloads stay in the file; payload() reads one on demand.
 */
class DataMapFile {
public:
    /** Opens `path`, which must be the file with id `expected_file_id`, and validates its layout and table. */
    DataFileError open(const std::string &path, int32_t expected_file_id);

    const DataFileHeader &header() const { return header_; }
    const std::vector<DataFileResource> &resources() const { return resources_; }

    /** Reads the payload bytes of resource `index` from disk; empty when the index is out of range or the read fails. */
    std::vector<uint8_t> payload(size_t index) const;

private:
    std::string path_;
    DataFileHeader header_{};
    std::vector<DataFileResource> resources_;
};

}  // namespace halo::cache
