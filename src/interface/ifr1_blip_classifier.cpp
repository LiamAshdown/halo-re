#include "halo/interface/ifr1_blip_classifier.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include <string.h>
#include "halo/cache/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"


static player *blip_player(datum_index player_index)
{
    return halo::interface::player_record(player_index);
}

namespace halo::interface {

/**
 * Original engine function blip_type_get; the author notes are in docs/original/interface/blip_type_get.txt.
 * blam-cc: object -> EBX
 *
 * @address 0x4b3450
 */
uint8_t BlipClassifier::type_get(int16_t local_player_index, datum_index object_index)
{
    datum_index viewer = (local_player_index != -1 && local_player_index < 1)
                             ? halo::game::globals().local_player_globals->local_players[local_player_index] : (datum_index)-1;
    int32_t viewer_team = blip_player(viewer)->team;
    int32_t owner_local_index;
    uint8_t *object_data_ptr;

    if (object_index == (datum_index)-1) {
        return _blip_type_unavailable;
    }
    owner_local_index = halo::game::player_index_from_unit_index(object_index) == (datum_index)-1
                            ? -1 : blip_player(halo::game::player_index_from_unit_index(object_index))->local_player_index;
    if (owner_local_index == local_player_index) {
        return _blip_type_friendly;
    }
    if (halo::objects::object_try_and_get(object_index, 3) == 0) {
        return _blip_type_enemy_special;
    }
    object_data_ptr = halo::interface::object_record(object_index);
    if (halo::objects::object_try_and_get(object_index, 2) != 0) {
        datum_index occupant = *(datum_index *)(object_data_ptr + 0x328);

        if (occupant == (datum_index)-1) {
            occupant = *(datum_index *)(object_data_ptr + 0x324);
        }
        if (occupant != (datum_index)-1) {
            uint8_t *occupant_data = halo::interface::object_record(occupant);
            return (uint8_t)((halo::game::teams_are_enemies((int16_t)viewer_team, ((object *)occupant_data)->owner_team) != 0) + 3);
        }
        {
            uint8_t *vehicle_tag = halo::interface::tag_data<uint8_t>(*(datum_index *)object_data_ptr);
            if (*(int32_t *)(vehicle_tag + 0x2e4) > 1 &&
                strncmp(*(char **)(vehicle_tag + 0x2e8) + 4, "c_dropship", 10) == 0) {
                return _blip_type_vehicle_special;
            }
        }
        return _blip_type_vehicle;
    }
    return (uint8_t)((halo::game::teams_are_enemies((int16_t)blip_player(halo::game::local_player_to_player_index(local_player_index))->team,
                                        *(int16_t *)(object_data_ptr + 0xb8)) != 0) + 1);
}

}
