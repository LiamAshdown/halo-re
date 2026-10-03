#include "halo/interface/ifr1_blip_classifier.hpp"
#include "halo/interface/api.hpp"

namespace halo::interface {

/**
 * C ABI entry point; forwards to halo::interface::BlipClassifier::type_get.
 * blam-cc: object -> EBX
 *
 * @address 0x4b3450
 */
uint8_t blip_type_get(int16_t local_player_index, datum_index object_index)
{
    return halo::interface::BlipClassifier::type_get(local_player_index, object_index);
}

}
