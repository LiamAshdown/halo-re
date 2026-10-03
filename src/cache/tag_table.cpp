#include "tags.h"

#include "halo/cache/cache.hpp"

#include "crt.h"
#include "halo/cache/globals.hpp"
#include "halo/core/win32_constants.hpp"
#include "halo/core/datum.hpp"


namespace halo::cache {

/**
 * Advances through the resident tag table and returns the id of the next tag whose group, parent group
 * or grandparent group matches the iterator's group filter (-1 matches all), or k_datum_index_none at
 * the end.
 *
 * @address 0x4425d0
 */
datum_index tag_iterator_view::next()
{
    tag_instance *entry;

    if (this->next_index >= globals().tag_header->tag_count) {
        return halo::k_dword_none;
    }

    for (;;) {
        entry = &globals().tag_instances[this->next_index];
        this->next_index = this->next_index + 1;

        if (entry != 0 &&
            ((int32_t)this->group_tag == -1 ||
             (int32_t)this->group_tag == (int32_t)entry->group_tag ||
             (int32_t)this->group_tag == (int32_t)entry->parent_group_tag ||
             (int32_t)this->group_tag == (int32_t)entry->grandparent_group_tag)) {
            break;
        }
        if (this->next_index >= globals().tag_header->tag_count) {
            return halo::k_dword_none;
        }
    }
    return entry->tag_id;
}

/**
 * Finds the tag with the given group and a case-insensitive path match. Returns its id, or
 * k_datum_index_none when no map is loaded or nothing matches.
 *
 * @address 0x442550
 */
datum_index tag_table::lookup(tag_group group, char *path)
{
    int16_t index;

    if (!globals().cache_file_loaded) {
        return halo::k_dword_none;
    }

    for (index = 0; index < globals().tag_header->tag_count; index++) {
        if (globals().tag_instances[index].group_tag == group) {
            if (_stricmp(path, globals().tag_instances[index].path) == 0) {
                return globals().tag_instances[index].tag_id;
            }
        }
    }
    return halo::k_dword_none;
}

} // namespace halo::cache
