#include "halo/units/unit.hpp"
#include "halo/sound/api.hpp"

extern "C" {
extern data_array *object_data;
extern data_array *player_data;
extern tag_instance *tag_instances;
extern datum_index player_index_from_unit_index(datum_index unit_index);
extern void unit_invalidate_local_player_zoom_level(datum_index unit);
}

namespace halo::units {

/**
 * Engine function unit_validate_and_clear_weapon_switch.
 *
 * @address 0x5659c0
 */
void UnitView::validate_and_clear_weapon_switch()
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;

    if (player_index_from_unit_index(unit_index) != k_datum_index_none) {
        datum_index player_index = player_index_from_unit_index(unit_index);

        if (*(int16_t *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200 + 2) != -1 &&
            obj[0x320] != 0xff) {
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
            int16_t slot = ((unit_object *)unit)->unit.current_weapon_index;

            if (slot != -1 && *(datum_index *)(unit + 0x2f8 + slot * 4) != k_datum_index_none) {
                uint8_t *weapon = (uint8_t *)((object_header *)object_data->data)
                    [*(datum_index *)(unit + 0x2f8 + slot * 4) & 0xffff].data;
                datum_index zoom_sound = *(datum_index *)((uint8_t *)tag_instances[*(datum_index *)weapon & 0xffff].data + 0x4bc);

                if (zoom_sound != k_datum_index_none) {
                    halo::sound::sound_start_unspatialized(zoom_sound, 1.0f);
                }
            }
        }
    }
    obj[0x320] = 0xff;
    obj[0x321] = 0xff;
    ((struct unit_object *)obj)->unit.integrated_night_vision_power = 0.0f;
    unit_invalidate_local_player_zoom_level(unit_index);
}

}
