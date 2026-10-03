#include "tags.h"

#include "halo/cache/cache.hpp"

#include "crt.h"

extern "C" {
extern cache_file_tag_header *tag_header;
extern tag_instance *tag_instances;
extern uint8_t cache_file_loaded;
}

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

    if (this->next_index >= tag_header->tag_count) {
        return (datum_index)0xffffffff;
    }

    for (;;) {
        entry = &tag_instances[this->next_index];
        this->next_index = this->next_index + 1;

        if (entry != 0 &&
            ((int32_t)this->group_tag == -1 ||
             (int32_t)this->group_tag == (int32_t)entry->group_tag ||
             (int32_t)this->group_tag == (int32_t)entry->parent_group_tag ||
             (int32_t)this->group_tag == (int32_t)entry->grandparent_group_tag)) {
            break;
        }
        if (this->next_index >= tag_header->tag_count) {
            return (datum_index)0xffffffff;
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

    if (!cache_file_loaded) {
        return (datum_index)0xffffffff;
    }

    for (index = 0; index < tag_header->tag_count; index++) {
        if (tag_instances[index].group_tag == group) {
            if (_stricmp(path, tag_instances[index].path) == 0) {
                return tag_instances[index].tag_id;
            }
        }
    }
    return (datum_index)0xffffffff;
}

} // namespace halo::cache
