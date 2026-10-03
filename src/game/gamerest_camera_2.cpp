#include "halo/game/gamerest_camera.hpp"
#include "halo/units/unit.hpp"
#include "halo/camera/api.hpp"
#include "halo/units/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern player_globals *local_player_globals;
extern data_array *player_data;
extern void *player_control_globals_ptr;
}

namespace halo::game {

/**
 * Resolves the best observer target for the given local-player slot and returns its weight
 * (via camera_observer_find_best_target's candidate), writing the target's object handle
 * through `out_id`.
 * REWRITTEN (first-boot track, objdump 0x459900..0x4599f7): camera_get_type_for_player gets CX = the slot; the
 *   autoaim cone call gets EDX = the player control record's +0x34 (0x006b145c + 0x40 * slot; -1 without a slot);
 *   the stack out value is the best candidate's primary weight (+0x30) and the result its object; the facing passed
 *   is the observer camera + 0x20 even for slot -1 (then 0x20), as in the original.
 * blam-cc: stack -> out_weight, SI -> local_player_slot
 *
 * @address 0x459900
 */
uint32_t CameraObserver::get_target_id(datum_index *out_id, int16_t local_player_slot)
{
    int16_t camera_type = halo::camera::camera_get_type_for_player(local_player_slot);
    datum_index player_index;
    uint8_t *player_record;
    uint32_t exclude_object;
    int16_t zoom_requirement = -1;
    real cone_buffer[6];
    observer_target_candidate candidate;
    uint8_t *observer_camera;

    *out_id = 0;
    if (camera_type != 0 && camera_type != 1) {
        return 0xffffffff;
    }
    player_index = (local_player_slot != -1 && local_player_slot < 1)
        ? local_player_globals->local_players[local_player_slot] : k_datum_index_none;
    player_record = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    exclude_object = halo::units::UnitView(*(uint32_t *)(player_record + 0x34)).resolve_camera_object();
    if (local_player_slot != -1) {
        zoom_requirement = *(int16_t *)(*(uint8_t **)&player_control_globals_ptr + local_player_slot * 0x40 + 0x34);
    }
    if (!halo::game::unit_get_current_weapon_autoaim_cone(*(uint32_t *)(player_record + 0x34), zoom_requirement, cone_buffer)) {
        return 0xffffffff;
    }
    observer_camera = local_player_slot == -1 ? 0 : (uint8_t *)halo::camera::globals().observers + local_player_slot * 0x29c + 0x74;
    if (!CameraObserver::find_best_target((real_point3d *)observer_camera, (observer_target_cone *)cone_buffer, (real_vector3d *)(observer_camera + 0x20), exclude_object, *(int16_t *)(player_record + 0x20), &candidate)) {
        return 0xffffffff;
    }
    *out_id = *(datum_index *)&candidate.weight_primary;
    return candidate.object;
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::CameraObserver::get_target_id; forwards to the C++ implementation.
 * register convention: local-player slot in SI (unaff_SI, -1 or >0 both disable the lookup, so
 * only slot 0 is ever valid -- consistent with k_maximum_local_players == 1); out id pointer
 * as the recognized stack parameter (param_1).
 * // blam-cc: unaff_SI -> local_player_slot, stack -> out_id
 * blam-cc: CX -> local_player_index
 * blam-cc: EAX -> unit_index, EDX -> require_zoomed, EDI -> out
 * blam-cc: stack -> out_weight, SI -> local_player_slot
 *
 * @address 0x459900
 */
uint32_t camera_observer_get_target_id(datum_index *out_id, int16_t local_player_slot)
{
    return halo::game::CameraObserver::get_target_id(out_id, local_player_slot);
}

}
