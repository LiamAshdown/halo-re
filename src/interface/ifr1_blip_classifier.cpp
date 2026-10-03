#include "halo/interface/ifr1_blip_classifier.hpp"
#include <string.h>
#include "halo/cache/api.hpp"

extern "C" {
extern player_globals *local_player_globals;
extern data_array *player_data;
extern data_array *object_data;
extern datum_index player_index_from_unit_index(datum_index object_index);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b);
extern datum_index local_player_to_player_index(int16_t local_player_index);
}

static player *blip_player(datum_index player_index)
{
    return (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200);
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
                             ? local_player_globals->local_players[local_player_index] : (datum_index)-1;
    int32_t viewer_team = blip_player(viewer)->team;
    int32_t owner_local_index;
    uint8_t *object_data_ptr;

    if (object_index == (datum_index)-1) {
        return _blip_type_unavailable;
    }
    owner_local_index = player_index_from_unit_index(object_index) == (datum_index)-1
                            ? -1 : blip_player(player_index_from_unit_index(object_index))->local_player_index;
    if (owner_local_index == local_player_index) {
        return _blip_type_friendly;
    }
    if (object_try_and_get(object_index, 3) == 0) {
        return _blip_type_enemy_special;
    }
    object_data_ptr = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    if (object_try_and_get(object_index, 2) != 0) {
        datum_index occupant = *(datum_index *)(object_data_ptr + 0x328);

        if (occupant == (datum_index)-1) {
            occupant = *(datum_index *)(object_data_ptr + 0x324);
        }
        if (occupant != (datum_index)-1) {
            uint8_t *occupant_data = (uint8_t *)((object_header *)object_data->data)[occupant & 0xffff].data;
            return (uint8_t)((teams_are_enemies((int16_t)viewer_team, ((object *)occupant_data)->owner_team) != 0) + 3);
        }
        {
            uint8_t *vehicle_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)object_data_ptr & 0xffff].data;
            if (*(int32_t *)(vehicle_tag + 0x2e4) > 1 &&
                strncmp(*(char **)(vehicle_tag + 0x2e8) + 4, "c_dropship", 10) == 0) {
                return _blip_type_vehicle_special;
            }
        }
        return _blip_type_vehicle;
    }
    return (uint8_t)((teams_are_enemies((int16_t)blip_player(local_player_to_player_index(local_player_index))->team,
                                        *(int16_t *)(object_data_ptr + 0xb8)) != 0) + 1);
}

}
