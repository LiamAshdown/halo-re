#include "halo/objects/record_access.hpp"
#include "halo/units/unit.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"


namespace halo::units {

/**
 * Engine function unit_validate_and_clear_weapon_switch.
 *
 * @address 0x5659c0
 */
void UnitView::validate_and_clear_weapon_switch()
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = halo::objects::object_record_bytes(unit_index);

    if (halo::game::player_index_from_unit_index(unit_index) != k_datum_index_none) {
        datum_index player_index = halo::game::player_index_from_unit_index(unit_index);

        if (*(int16_t *)((uint8_t *)halo::game::globals().player_data->data + halo::datum_slot(player_index) * 0x200 + 2) != -1 &&
            (uint8_t)((struct unit_object *)obj)->unit.zoom_level != 0xff) {
            uint8_t *unit = halo::objects::object_record_bytes(unit_index);
            int16_t slot = ((unit_object *)unit)->unit.current_weapon_index;

            if (slot != -1 && *(datum_index *)(unit + 0x2f8 + slot * 4) != k_datum_index_none) {
                uint8_t *weapon = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)
                    [halo::datum_slot(*(datum_index *)(unit + 0x2f8 + slot * 4))].data;
                datum_index zoom_sound = *(datum_index *)(halo::objects::tag_record_bytes(*(datum_index *)weapon) + 0x4bc);

                if (zoom_sound != k_datum_index_none) {
                    halo::sound::sound_start_unspatialized(zoom_sound, 1.0f);
                }
            }
        }
    }
    ((struct unit_object *)obj)->unit.zoom_level = 0xff;
    ((struct unit_object *)obj)->unit.desired_zoom_level = 0xff;
    ((struct unit_object *)obj)->unit.integrated_night_vision_power = 0.0f;
    halo::game::unit_invalidate_local_player_zoom_level(unit_index);
}

}
