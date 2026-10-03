#include "halo/effects/effects.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/interface/api.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern datum_index effect_marker_new(effect *self, int16_t location_index, object_marker *resolved_marker, uint8_t first_person);
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
    Effect *tag = (Effect *)halo::cache::globals().tag_instances[(uint16_t)self->definition_index].data;
    uint8_t *locations = (uint8_t *)tag->locations.pointer;
    int16_t location_index;

    for (location_index = 0; (int32_t)location_index < (int32_t)tag->locations.count; location_index++) {
        object_marker markers[16];
        int16_t count = resolve_marker(self->object_index, (const char *)(locations + location_index * 0x20),
            markers, 0x10);
        int16_t i;
        uint8_t first_person = (uint8_t)((uint32_t)resolve_marker == 0x492ad0u ||
            resolve_marker == (effect_marker_resolver)halo::interface::first_person_weapon_get_marker_data);

        for (i = 0; i < count; i++) {
            if (halo::effects::effect_marker_new(self, location_index, &markers[i], first_person) == k_datum_index_none) {
                break;
            }
        }
    }
}

}

namespace halo::effects {

void effect_rebuild_markers(effect *self, effect_marker_resolver resolve_marker)
{
    halo::effects::effect_view(self).rebuild_markers(resolve_marker);
}

}
