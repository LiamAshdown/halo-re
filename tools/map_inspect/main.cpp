/**
 * map_inspect: reads Halo PC .map files with halo::cache::MapFile and prints their header and tag index.
 *
 *   map_inspect <file.map> [--tags]      header summary, tag counts per group, optionally every tag
 *   map_inspect --check <maps folder>    validates every .map and the shared bitmaps.map / sounds.map; non-zero exit on a failure
 *   map_inspect --data <file.map> [--list]   summary (and optionally every resource) of bitmaps.map or sounds.map
 */
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <windows.h>

#include "halo/cache/map_reader.hpp"

using halo::cache::DataFileError;
using halo::cache::DataMapFile;
using halo::cache::MapError;
using halo::cache::MapFile;

namespace {

/** Prints the header fields and the per-group tag counts of an opened map. */
void print_summary(const std::string &path, const MapFile &map, bool list_tags)
{
    const halo::cache::MapFileHeader &header = map.header();
    std::printf("%s\n", path.c_str());
    std::printf("  scenario     %s\n", header.name);
    std::printf("  build        %.32s\n", header.build);
    std::printf("  map type     %d\n", header.map_type);
    std::printf("  crc32        %08x\n", header.crc32);
    std::printf("  file size    %d bytes\n", header.file_size);
    std::printf("  tag data     offset 0x%x, %u bytes\n", header.tag_data_offset, header.tag_data_size);
    std::printf("  tag count    %d (scenario tag id %08x)\n", map.tag_header().tag_count, map.tag_header().scenario_tag);

    std::map<std::string, int> per_group;
    for (const halo::cache::MapTag &tag : map.tags()) {
        per_group[tag.group]++;
    }
    for (const auto &[group, count] : per_group) {
        std::printf("    %-4s %d\n", group.c_str(), count);
    }
    if (list_tags) {
        for (const halo::cache::MapTag &tag : map.tags()) {
            std::printf("  %08x %-4s %s\n", tag.tag_id, tag.group.c_str(), tag.path.c_str());
        }
    }
}

int check_data_files(const std::string &folder);

/** Opens every .map in a folder (the data files bitmaps.map, sounds.map and loc.map are skipped) and reports problems. */
int check_folder(const std::string &folder)
{
    WIN32_FIND_DATAA found;
    HANDLE handle = FindFirstFileA((folder + "\\*.map").c_str(), &found);
    if (handle == INVALID_HANDLE_VALUE) {
        std::printf("no .map files in %s\n", folder.c_str());
        return 2;
    }
    int bad = 0;
    int good = 0;
    do {
        const std::string name = found.cFileName;
        if (name == "bitmaps.map" || name == "sounds.map" || name == "loc.map") {
            continue;
        }
        MapFile map;
        const MapError error = map.open(folder + "\\" + name);
        if (error == MapError::none) {
            const halo::cache::MapTag *scenario = map.find("scnr", map.tags().empty() ? "" : map.tags()[0].path);
            std::printf("ok   %-24s %4d tags  scenario %s\n", name.c_str(), map.tag_header().tag_count, scenario != nullptr ? scenario->path.c_str() : "?");
            good++;
        } else {
            std::printf("FAIL %-24s %s\n", name.c_str(), halo::cache::map_error_text(error));
            bad++;
        }
    } while (FindNextFileA(handle, &found) != 0);
    FindClose(handle);
    bad += check_data_files(folder);
    std::printf("%d maps ok, %d failed\n", good, bad);
    return bad == 0 ? 0 : 1;
}

/** Prints the summary of a shared resource file: resource count, payload bytes and optionally every resource. */
void print_data_summary(const std::string &path, const DataMapFile &data, bool list)
{
    uint64_t payload_bytes = 0;
    for (const halo::cache::DataFileResource &resource : data.resources()) {
        payload_bytes += resource.size;
    }
    std::printf("%s\n", path.c_str());
    std::printf("  file id      %d\n", data.header().file_id);
    std::printf("  resources    %zu (%llu payload bytes)\n", data.resources().size(), static_cast<unsigned long long>(payload_bytes));
    if (list) {
        for (const halo::cache::DataFileResource &resource : data.resources()) {
            std::printf("  %08x %8u  %s\n", resource.file_offset, resource.size, resource.name.c_str());
        }
    }
}

/** Validates bitmaps.map and sounds.map in a folder; returns the number that failed. */
int check_data_files(const std::string &folder)
{
    struct Entry {
        const char *file;
        int32_t id;
    };
    int bad = 0;
    for (const Entry &entry : { Entry{ "bitmaps.map", halo::cache::k_data_file_id_bitmaps }, Entry{ "sounds.map", halo::cache::k_data_file_id_sounds } }) {
        DataMapFile data;
        const DataFileError error = data.open(folder + "\\" + entry.file, entry.id);
        if (error == DataFileError::none) {
            std::printf("ok   %-24s %5zu resources\n", entry.file, data.resources().size());
        } else {
            std::printf("FAIL %-24s %s\n", entry.file, halo::cache::data_file_error_text(error));
            bad++;
        }
    }
    return bad;
}

}  // namespace

int main(int argc, char **argv)
{
    if (argc >= 3 && std::strcmp(argv[1], "--check") == 0) {
        return check_folder(argv[2]);
    }
    if (argc >= 3 && std::strcmp(argv[1], "--data") == 0) {
        DataMapFile data;
        const std::string file = argv[2];
        const bool sounds = file.find("sounds") != std::string::npos;
        const DataFileError error = data.open(file, sounds ? halo::cache::k_data_file_id_sounds : halo::cache::k_data_file_id_bitmaps);
        if (error != DataFileError::none) {
            std::printf("%s: %s\n", argv[2], halo::cache::data_file_error_text(error));
            return 1;
        }
        print_data_summary(argv[2], data, argc >= 4 && std::strcmp(argv[3], "--list") == 0);
        return 0;
    }
    if (argc >= 2) {
        MapFile map;
        const MapError error = map.open(argv[1]);
        if (error != MapError::none) {
            std::printf("%s: %s\n", argv[1], halo::cache::map_error_text(error));
            return 1;
        }
        print_summary(argv[1], map, argc >= 3 && std::strcmp(argv[2], "--tags") == 0);
        return 0;
    }
    std::printf("usage: map_inspect <file.map> [--tags] | --data <bitmaps.map|sounds.map> [--list] | --check <maps folder>\n");
    return 2;
}
