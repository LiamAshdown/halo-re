/**
 * map_inspect: reads Halo PC .map files with halo::cache::MapFile and prints their header and tag index.
 *
 *   map_inspect <file.map> [--tags]      header summary, tag counts per group, optionally every tag
 *   map_inspect --check <maps folder>    validates every .map in the folder and exits non-zero on the first bad one
 */
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <windows.h>

#include "halo/cache/map_reader.hpp"

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
    std::printf("%d ok, %d failed\n", good, bad);
    return bad == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char **argv)
{
    if (argc >= 3 && std::strcmp(argv[1], "--check") == 0) {
        return check_folder(argv[2]);
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
    std::printf("usage: map_inspect <file.map> [--tags] | --check <maps folder>\n");
    return 2;
}
