#include "halo/units/unit.hpp"
#include "halo/cache/api.hpp"

#include <string.h>
extern "C" {
extern data_array *object_data;
extern int tolower(int c);
}

namespace halo::units {

/**
 * Scans the unit's seat/marker definitions for unoccupied entries whose name matches (or is empty) and whose
 * type flags match flag_selector, returning up to max_indices matching seat indices.
 *
 * @address 0x56a310
 */
int16_t UnitView::find_seats_matching_name_and_flags(char *name_filter, uint16_t flag_selector, int16_t *out_indices, int16_t max_indices)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *unit_tag = (Unit *)tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;

    uint8_t name_is_empty;
    if (name_filter != (char *)0) {
        int32_t len = 0;
        while (name_filter[len] != '\0') len++;
        name_is_empty = (len == 0);
    } else {
        name_is_empty = 1;
    }

    int16_t out_count = 0;
    if ((int32_t)unit_tag->seats.count < 1) {
        return 0;
    }

    UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;
    for (int16_t seat_index = 0; seat_index < (int32_t)unit_tag->seats.count; seat_index++) {
        if (max_indices <= out_count) break;

        char lowered[256];
        char *src = seats[seat_index].label.string;
        int32_t i = 0;
        do {
            lowered[i] = (char)tolower((uint8_t)src[i]);
            i++;
        } while (src[i - 1] != '\0');

        if ((!name_is_empty) && (strstr((const char *)lowered, name_filter) == 0)) {
            continue;
        }

        uint32_t flags = seats[seat_index].flags;
        uint8_t match;
        switch (flag_selector) {
        case 0:
            match = (~(uint8_t)(flags >> 2)) & 1;
            break;
        case 1:
            match = (uint8_t)(flags >> 3) & 1;
            break;
        case 2:
            if ((flags & 4) || (flags & 8)) {
                continue;
            }
            match = 1;
            break;
        case 3:
            match = (uint8_t)(flags >> 2) & 1;
            break;
        default:
            match = 1;
            break;
        }

        if (match & 1) {
            if (::halo::units::unit_is_seat_occupied(unit_index, seat_index) == 0) {
                out_indices[out_count] = seat_index;
                out_count++;
            }
        }
    }
    return out_count;
}

}
