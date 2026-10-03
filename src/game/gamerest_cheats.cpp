#include "halo/game/gamerest_cheats.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include <stdint.h>
#include "halo/memory/api.hpp"
#include <string.h>
#include "halo/cache/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern Globals *global_globals;
extern data_array *player_data;
extern player_globals *local_player_globals;
extern uint32_t cheat_get_target_object_index(void);
extern void *object_type_definitions[12];
extern double atan2(double y, double x);
extern double sin(double x);
extern double cos(double x);
}

namespace halo::game {

/**
 * Spawns one of every weapon near the camera: the globals tag's own weapon_list when it has
 * entries, otherwise the first 16 'weap' tags in the tag index.
 *
 * @address 0x45a530
 */
void Cheats::all_weapons()
{
    int16_t count;
    tag_iterator iterator;
    TagDependency slots[16];
    datum_index tag;

    count = 0;
    iterator.next_index = 0;
    iterator.group_tag = (tag_group)0x77656170;

    if (global_globals->weapon_list.count != 0 && global_globals->weapon_list.pointer != 0) {
        Cheats::spawn_objects_near_camera((TagDependency *)global_globals->weapon_list.pointer, (int16_t)global_globals->weapon_list.count);

        return;
    }

    tag = halo::cache::tag_iterator_next(&iterator);
    while (tag != k_datum_index_none && (uint16_t)count < 0x10) {
        *(datum_index *)&slots[count].tag_id = tag;
        count = count + 1;
        tag = halo::cache::tag_iterator_next(&iterator);
    }
    Cheats::spawn_objects_near_camera(slots, count);
}

/**
 * Implements the original `cheat_get_target_object_index`.
 *
 * @address 0x45a7a0
 */
uint32_t Cheats::get_target_object_index()
{
    data_iterator iterator;
    player *p;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)halo::memory::data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (p->unit != k_datum_index_none) {
            return iterator.index;
        }
        p = (player *)halo::memory::data_iterator_next(&iterator);
    }
    return halo::k_dword_none;
}

/**
 * Forces local player `local_player_slot`'s controlled unit's shield-like field to full and sets
 * its invincibility-style flag bits.
 *
 * @address 0x45a720
 */
void Cheats::make_player_invincible(int16_t local_player_slot)
{
    datum_index player_index;
    datum_index unit_index;
    object *unit_obj;
    unit_data *unit;

    if (-1 < local_player_slot && local_player_slot < 1 && local_player_slot != -1) {
        player_index = local_player_globals->local_players[local_player_slot];
        if (player_index != k_datum_index_none) {
            unit_index = (halo::game::player_at(player_index))->unit;
            unit_obj = ((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_datum_slot_mask].data;
            unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
            unit->active_camouflage_power = 1.0f;
            if ((unit->flags & 0x10) != 0) {
                unit->flags = unit->flags | 0x20;
            }
            unit->flags = unit->flags | 0x10;
        }
    }
}

/**
 * Forces the debug-selected player's controlled unit's shield-like field to full and sets its
 * invincibility-style flag bits.
 *
 * @address 0x45a6c0
 */
void Cheats::make_selected_object_invincible()
{
    uint32_t player_index;
    datum_index unit_index;
    object *unit_obj;
    unit_data *unit;

    player_index = Cheats::get_target_object_index();
    if (player_index != halo::k_dword_none) {
        unit_index = (halo::game::player_at(player_index))->unit;
        unit_obj = ((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_datum_slot_mask].data;
        unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        unit->active_camouflage_power = 1.0f;
        if ((unit->flags & 0x10) != 0) {
            unit->flags = unit->flags | 0x20;
        }
        unit->flags = unit->flags | 0x10;
    }
}

/**
 * REWRITTEN 2026-09-28 from objdump 0x45a800..0x45a9b8. 0x45a7a0 returns a PLAYER index (the first player with a
 *   unit); the position and basis come from that player's unit (player +0x34). The draft passed the player index
 *   to object_get_position as if it were an object, so the cheats spawned relative to a garbage object.
 *   Object k of `count` is placed 1.5 units out at yaw + (k - count/2) * min(2 pi / count, pi / 8), yaw being
 *   atan2(forward.i, forward.j) (the original's fpatan operand order), 0.8 above the unit's origin, with the
 *   unit's forward and up; role 3, or 0 for a network client (game mode 2) when the object type definition's
 *   +0x10 is not -1.
 *
 * @address 0x45a800
 */
void Cheats::spawn_objects_near_camera(TagDependency *tag_array, int16_t count)
{
    uint32_t player_index = Cheats::get_target_object_index();
    datum_index unit;
    real_point3d unit_position;
    real_vector3d unit_forward;
    real_vector3d unit_up;
    int32_t i;

    if (player_index == halo::k_dword_none) {
        return;
    }
    if (count <= 0) {
        return;
    }
    unit = *(datum_index *)((uint8_t *)halo::game::player_at(player_index) + 0x34);
    halo::objects::object_get_position(&unit_position, unit);
    halo::objects::object_get_orientation(&unit_forward, unit, &unit_up);

    for (i = 0; i < (int32_t)(uint16_t)count; i++) {
        datum_index tag_handle = *(datum_index *)&tag_array[i].tag_id;
        object_placement_data placement;
        double spacing;
        double angle;
        uint32_t role;

        if (tag_handle == k_datum_index_none) {
            continue;
        }
        spacing = (double)6.2831855f / (double)(int32_t)count;
        if (!(spacing <= (double)0.39269909f)) {
            spacing = (double)0.39269909f;
        }
        angle = atan2((double)unit_forward.i, (double)unit_forward.j) +
            (double)(i - (int32_t)count / 2) * spacing;
        halo::objects::object_placement_data_initialize(&placement, tag_handle, k_datum_index_none);
        placement.forward = unit_forward;
        placement.up = unit_up;
        role = 3;
        placement.position.x = (float)(cos(angle) * (double)1.5f + (double)unit_position.x);
        placement.position.y = (float)(sin(angle) * (double)1.5f + (double)unit_position.y);
        placement.position.z = unit_position.z + 0.8f;
        if (halo::networking::globals().game_mode == 2) {
            int16_t object_type = *(int16_t *)halo::cache::globals().tag_instances[placement.definition_tag & halo::k_datum_slot_mask].data;

            if (*(int32_t *)((uint8_t *)object_type_definitions[object_type] + 0x10) != -1) {
                role = 0;
            }
        }
        halo::objects::object_new_with_datum_role_control(&placement, role);
    }
}

/**
 * Finds the "warthog" vehicle tag by name in the loaded tag-index table and spawns one near the
 * camera.
 *
 * @address 0x45a5c0
 */
void Cheats::spawn_warthog()
{
    GlobalsMultiplayerInformation *info;
    GlobalsVehicle *vehicles;
    int32_t count;
    int32_t i;

    info = (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
    if (global_globals->multiplayer_information.count != 0) {
        count = (int32_t)info->vehicles.count;
        vehicles = (GlobalsVehicle *)info->vehicles.pointer;
        i = 0;
        if (0 < count) {
            while (strstr((const char *)vehicles[i].vehicle.path_pointer, "warthog") == 0) {
                i = i + 1;
                if (count <= i) {
                    return;
                }
            }
            Cheats::spawn_objects_near_camera(&vehicles[i].vehicle, 1);
        }
    }
}

/**
 * Teleports the debug-selected player's unit (or its root parent) to the current camera
 * position, or logs an error if the camera is outside the BSP.
 *
 * @address 0x45a630
 */
void Cheats::teleport_to_camera()
{
    uint32_t player_index;
    int16_t local_player_slot;
    datum_index unit_index;
    datum_index root;
    object *unit_obj;
    uint8_t *camera_row;

    player_index = Cheats::get_target_object_index();
    if (player_index != halo::k_dword_none) {
        local_player_slot = (halo::game::player_at(player_index))->local_player_index;
        if (local_player_slot != -1) {
            camera_row = (uint8_t *)&halo::camera::globals().observers[local_player_slot].camera;
            if (*(int16_t *)(camera_row + 0x10) != -1) {
                unit_index = (halo::game::player_at(player_index))->unit;
                unit_obj = ((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_datum_slot_mask].data;
                root = unit_obj->parent_object;
                if (root == k_datum_index_none) {
                    root = unit_index;
                }

                halo::objects::object_set_position_and_orientation(root, 0, 0, (real_point3d *)camera_row);
                return;
            }
            halo::interface::console_printf_verbose((ColorARGB *)0, (char *)("Camera is outside BSP... cannot initiate teleportation..."));
        }
    }
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::Cheats::all_weapons; forwards to the C++ implementation.
 * blam-cc: ESI ->
 *
 * @address 0x45a530
 */
void cheat_all_weapons(void)
{
    halo::game::Cheats::all_weapons();
}

/**
 * C entry point for halo::game::Cheats::get_target_object_index; forwards to the C++ implementation.
 * register convention: no arguments. data_iterator_next's iterator (EDI) is the inline
 * types/memory.h data_iterator over player_data built at 0x45a7a3..0x45a7c5 (data, WORD
 * next_index = 0, index = -1, signature = data ^ 'iter'); Ghidra lost it because it is a
 * stack object passed in a register.
 * blam-cc: EDI -> iterator
 *
 * @address 0x45a7a0
 */


/**
 * C entry point for halo::game::Cheats::make_player_invincible; forwards to the C++ implementation.
 * register convention: local-player slot in AX (in_AX; only slot 0 is ever valid, matching
 * k_maximum_local_players == 1).
 * // blam-cc: in_AX -> local_player_slot
 * blam-cc: in_AX -> local_player_slot
 *
 * @address 0x45a720
 */
void cheat_make_player_invincible(int16_t local_player_slot)
{
    halo::game::Cheats::make_player_invincible(local_player_slot);
}

/**
 * C entry point for halo::game::Cheats::make_selected_object_invincible; forwards to the C++ implementation.
 * register convention: no arguments.
 *
 * @address 0x45a6c0
 */
void cheat_make_selected_object_invincible(void)
{
    halo::game::Cheats::make_selected_object_invincible();
}

/**
 * C entry point for halo::game::Cheats::spawn_objects_near_camera; forwards to the C++ implementation.
 * register convention: `tag_array` and `count` are Ghidra's own recognized __cdecl parameters.
 * blam-cc: EAX -> out, ECX -> object_index (matches src/objects/object_get_position.c)
 * blam-cc: EAX -> out_forward, ECX -> object_index, stack -> out_up (matches src/objects/object_get_orientation.c)
 *
 * @address 0x45a800
 */
void cheat_spawn_objects_near_camera(TagDependency *tag_array, int16_t count)
{
    halo::game::Cheats::spawn_objects_near_camera(tag_array, count);
}

/**
 * C entry point for halo::game::Cheats::spawn_warthog; forwards to the C++ implementation.
 * register convention: no arguments.
 *
 * @address 0x45a5c0
 */
void cheat_spawn_warthog(void)
{
    halo::game::Cheats::spawn_warthog();
}

/**
 * C entry point for halo::game::Cheats::teleport_to_camera; forwards to the C++ implementation.
 * register convention: no arguments.
 * blam-cc: stack -> object_index,
 *
 * @address 0x45a630
 */
void cheat_teleport_to_camera(void)
{
    halo::game::Cheats::teleport_to_camera();
}

}
