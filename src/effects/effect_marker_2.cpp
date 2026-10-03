#include "halo/effects/effects.hpp"

extern "C" {
extern tag_instance *tag_instances;
extern datum_index effect_marker_new(effect *self, int16_t location_index, object_marker *resolved_marker, uint8_t first_person);
extern int32_t first_person_weapon_get_marker_data(uint32_t object_index, const char *location, object_marker *out, uint32_t max_count);
void effect_rebuild_markers(effect *self, effect_marker_resolver resolve_marker);
}

namespace halo::effects {

/**
 * For each EffectLocation of the effect's tag, resolves up to 16 matching object markers through
 * `resolve_marker` and creates an effect_location_marker for each one found.
 *
 * @address 0x451710
 */
void effect_view::rebuild_markers(effect_marker_resolver resolve_marker)
{
    effect * self = record;
    Effect *tag = (Effect *)tag_instances[(uint16_t)self->definition_index].data;
    uint8_t *locations = (uint8_t *)tag->locations.pointer;
    int16_t location_index;

    for (location_index = 0; (int32_t)location_index < (int32_t)tag->locations.count; location_index++) {
        object_marker markers[16];
        int16_t count = resolve_marker(self->object_index, (const char *)(locations + location_index * 0x20),
            markers, 0x10);
        int16_t i;
        uint8_t first_person = (uint8_t)((uint32_t)resolve_marker == 0x492ad0u ||
            resolve_marker == first_person_weapon_get_marker_data);

        for (i = 0; i < count; i++) {
            if (effect_marker_new(self, location_index, &markers[i], first_person) == k_datum_index_none) {
                break;
            }
        }
    }
}

}

extern "C" {

void effect_rebuild_markers(effect *self, effect_marker_resolver resolve_marker)
{
    halo::effects::effect_view(self).rebuild_markers(resolve_marker);
}

}
