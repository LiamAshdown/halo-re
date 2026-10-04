#include "halo/interface/ifr1_first_person_weapon_controller.hpp"
#include "halo/interface/records.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/models/api.hpp"
#include <stdint.h>
#include <string.h>
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/render/api.hpp"
#include "halo/main/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/models/models.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "interface.h"
#include "items.h"
#include "units.h"
#include "tags.h"
#include "tags.h"
#include "halo/interface/flags.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/render/vars.hpp"

static auto &first_person_weapon_interfaces = halo::link::ref<first_person_weapon_interface *>(halo::ui::vars().first_person_weapon_interfaces);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &current_local_player_index = halo::link::ref<int16_t>(halo::ui::vars().current_local_player_index);
static auto &render_camera_global = halo::link::ref<float>(halo::render::vars().render_camera_global);
static auto &camera_position_y = halo::link::ref<float>(halo::ui::vars().camera_position_y);
static auto &camera_position_z = halo::link::ref<float>(halo::effects::vars().camera_position_z);
static auto &camera_field_of_view = halo::link::ref<float>(halo::ui::vars().camera_field_of_view);
static auto &rasterizer_device_version = halo::link::ref<uint32_t>(halo::ui::vars().rasterizer_device_version);
static auto &rasterizer_caps_flag_68a = halo::link::ref<uint8_t>(halo::ui::vars().rasterizer_caps_flag_68a);
#include "halo/interface/layout_checks.hpp"
#include "halo/units/api.hpp"

static float clamp_unit(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static float script_source_value(uint16_t source)
{
    if (halo::render::cinematic_screen_effect_get_script_value(source) < 0.0f) {
        return 0.0f;
    }
    if (halo::render::cinematic_screen_effect_get_script_value(source) > 1.0f) {
        return 1.0f;
    }
    return halo::render::cinematic_screen_effect_get_script_value(source);
}

static_assert(offsetof(Globals, first_person_interface) == 0x17c, "Globals first person interface block");

namespace halo::interface {

/**
 * If unit_index belongs to a local player with an attached first-person weapon, looks up the weapon model's
 * "flashlight" marker and derives a centered origin (marker position offset back half the marker's forward
 * extent), the raw forward vector as an extents triple, and the marker's up vector as a direction triple.
 *
 * @address 0x492b80
 */
void FirstPersonWeaponController::center_flashlight(datum_index unit_index, real_point3d *out_origin, real_vector3d *out_extents, real_vector3d *out_direction)
{
    int32_t local_player;
    first_person_weapon_interface *fp;
    object_marker marker;
    int16_t result;

    local_player = halo::interface::local_player_index_for_unit(unit_index);
    if ((int16_t)local_player == -1) {
        return;
    }

    fp = &first_person_weapon_interfaces[local_player];
    if (fp->attached == 0) {
        return;
    }

    result = (int16_t)halo::interface::first_person_weapon_get_marker_data(fp->weapon_index, "flashlight", &marker, 1);
    if (result <= 0) {
        return;
    }

    out_origin->x = marker.node_transform.position.x - marker.node_transform.forward.i * 0.5f;
    out_origin->y = marker.node_transform.position.y - marker.node_transform.forward.j * 0.5f;
    out_origin->z = marker.node_transform.position.z - marker.node_transform.forward.k * 0.5f;

    out_extents->i = marker.node_transform.forward.i;
    out_extents->j = marker.node_transform.forward.j;
    out_extents->k = marker.node_transform.forward.k;

    out_direction->i = marker.node_transform.up.i;
    out_direction->j = marker.node_transform.up.j;
    out_direction->k = marker.node_transform.up.k;
}

/**
 * 0x4d7850, ECX model_tag_id, EAX name If weapon_index both exists as a live object and is the local player's
 * current first-person weapon, and the active camera is first-person, and the weapon's weapon_hud_interface
 * tag is both present and has both a marker-name table (+0x468) and a pickup-notification dependency (+0x478),
 * looks up a named marker on the first-person weapon model and returns its transform. Returns 0 on any failed
 * gate.
 *
 * @address 0x492ad0
 */
uint32_t FirstPersonWeaponController::get_marker_data(datum_index weapon_index, const char *marker_name, object_marker *out, uint32_t maximum)
{
    object *obj;
    int32_t local_player;
    int16_t camera_type;
    first_person_weapon_interface *fp;
    Weapon *item_tag_data;

    obj = (object *)halo::objects::object_try_and_get(weapon_index, 4);
    if (obj == 0) {
        return 0;
    }

    local_player = halo::interface::local_player_index_for_weapon(weapon_index);
    if ((int16_t)local_player == -1) {
        return 0;
    }

    camera_type = halo::camera::camera_get_type_for_player((int16_t)local_player);
    if (camera_type != 0) {
        return 0;
    }

    fp = &first_person_weapon_interfaces[local_player];
    item_tag_data = halo::interface::tag_data<Weapon>(obj->definition_tag);

    if (fp->weapon_hud_valid != 0 && halo::interface::tag_handle(item_tag_data->first_person_model.tag_id) != halo::k_dword_none &&
        halo::interface::tag_handle(item_tag_data->first_person_animations.tag_id) != halo::k_dword_none) {
        return (uint32_t)halo::models::model_markers::get_by_name(halo::interface::tag_handle(item_tag_data->first_person_model.tag_id), marker_name,
            nullptr, fp->weapon_hud_element, (real_matrix4x3 *)fp->node_matrices, 0, out, (int16_t)maximum);
    }
    return 0;
}

/**
 * name, see that file (Re)initializes local_player_index's first_person_weapon_interface when its controlled
 * unit's current weapon (and that weapon's hud_interface tag and first-person model) are all valid: if it was
 * already attached, detaches first; resets the animation/pose/blend fields; looks up whether the weapon and
 * device HUD elements match this globals-defined interface's message table (hud_meter_find_matching_elements);
 * and, only if both match, commits weapon_index, clears the remaining scratch fields, enters state 1 and re-
 * attaches. Always finishes by resetting the interface's shutdown countdown
 * (first_person_weapon_interface_tick_reset).
 * blam-cc: stack -> local_player_index
 *
 * @address 0x493c60
 */
void FirstPersonWeaponController::interface_initialize()
{
    first_person_weapon_interface *fp = &first_person_weapon_interfaces[local_player_index];
    uint8_t was_attached = fp->attached;
    struct object *unit;
    int16_t current_weapon_slot;
    datum_index weapon_index;
    struct object *weapon_object;
    Weapon *weapon_tag_data;
    ModelAnimations *animation_graph;
    ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *node_array_block;
    int16_t marker_node_index;
    uint8_t weapon_hud_matched;
    uint32_t hud_interface_tag_ref;

    fp->weapon_index = (datum_index)halo::k_dword_none;

    if (was_attached != 0) {
        halo::effects::effect_release_first_person_markers(local_player_index);
        halo::effects::particles_delete_by_first_person_weapon((uint8_t)local_player_index);
        fp->attached = 0;
    }

    if (fp->unit_index == (datum_index)halo::k_dword_none) {
        halo::interface::first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    unit = halo::interface::object_record<struct object>(fp->unit_index);
    current_weapon_slot = ((unit_object *)unit)->unit.current_weapon_index;
    if (current_weapon_slot == -1) {
        halo::interface::first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }
    weapon_index = ((unit_object *)unit)->unit.weapons[current_weapon_slot];
    if (weapon_index == (datum_index)halo::k_dword_none) {
        halo::interface::first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    weapon_object = halo::interface::object_record<struct object>(weapon_index);
    weapon_tag_data = halo::interface::tag_data<Weapon>(weapon_object->definition_tag);
    if (halo::interface::tag_handle(weapon_tag_data->first_person_model.tag_id) == halo::k_dword_none) {
        halo::interface::first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    hud_interface_tag_ref = halo::interface::tag_handle(weapon_tag_data->first_person_animations.tag_id);
    animation_graph = halo::interface::tag_data<ModelAnimations>(hud_interface_tag_ref);
    if (animation_graph->first_person_weapons.count == 0) {
        halo::interface::first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }
    node_array_block = halo::interface::reflexive_elements<ModelAnimationsAnimationGraphFirstPersonWeaponAnimations>(animation_graph->first_person_weapons);
    if (node_array_block == nullptr) {
        halo::interface::first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    fp->animation_index = -1;
    if ((int32_t)node_array_block->animations.count > 4) {
        marker_node_index = halo::interface::reflexive_elements<int16_t>(node_array_block->animations)[4];
        if (marker_node_index != -1 &&
            (int16_t)halo::interface::reflexive_elements<ModelAnimationsAnimation>(animation_graph->animations)[marker_node_index].frame_count >= 9) {
            fp->animation_index = marker_node_index;
        }
    }

    {
        uint32_t hands_model = *(uint32_t *)&halo::interface::reflexive_elements<GlobalsFirstPersonInterface>(global_globals->first_person_interface)->first_person_hands.tag_id;

        if (hands_model != halo::k_dword_none) {
            fp->device_hud_valid = halo::interface::hud_meter_find_matching_elements(hud_interface_tag_ref, hands_model,
                fp->device_hud_element);
        }
    }
    weapon_hud_matched = halo::interface::hud_meter_find_matching_elements(hud_interface_tag_ref,
        halo::interface::tag_handle(weapon_tag_data->first_person_model.tag_id), fp->weapon_hud_element);
    fp->weapon_hud_valid = weapon_hud_matched;

    if (weapon_hud_matched != 0 && fp->device_hud_valid != 0) {
        fp->weapon_index = weapon_index;
        fp->state = -1;
        fp->current_animation = -1;
        fp->moving_animation = -1;
        fp->overcharged_animation = -1;
        fp->recoil = 0.0f;
        fp->charge = 0.0f;
        fp->idle_ticks = 0;
        fp->frame_sound_index = -1;
        fp->frame_sound_state = -1;
        halo::interface::first_person_weapon_set_state(local_player_index, 1, 0);
        fp->blend_end = 0;
        if (was_attached != 0 && fp->attached != 1) {
            halo::effects::effect_reattach_markers_for_object(local_player_index, fp->weapon_index);
            fp->attached = 1;
        }
    }

    halo::interface::first_person_weapon_interface_tick_reset(local_player_index);
}

/**
 * Local player 0's per-frame first-person weapon entry point: if the current player slot is empty, does
 * nothing; otherwise re-initializes the cached weapon-interface record whenever the controlled unit changed
 * (or the record has no weapon yet), then runs the main weapon update.
 *
 * @address 0x4923d0
 */
void FirstPersonWeaponController::interface_tick(void)
{
    player *record;
    datum_index unit_index;
    first_person_weapon_interface *fp;

    fp = &first_person_weapon_interfaces[0];

    if (halo::game::globals().local_player_globals->local_players[0] != (datum_index)halo::k_dword_none) {
        record = halo::interface::player_record(halo::game::globals().local_player_globals->local_players[0]);
        unit_index = record->unit;

        if (fp->unit_index != unit_index) {
            fp->aim_seeded = 0;
            fp->unit_index = unit_index;
            halo::interface::first_person_weapon_interface_initialize(0);
        }
        if (fp->weapon_index == (datum_index)halo::k_dword_none) {
            halo::interface::first_person_weapon_interface_initialize(0);
        }
        halo::interface::first_person_weapon_update(0);
    }
}

/**
 * If local_player_index's interface currently has a weapon, touches that weapon's predicted resources (forcing
 * them into their streaming caches); either way, reseeds the interface's shutdown countdown to 0x1e.
 * blam-cc: AX -> local_player_index
 *
 * @address 0x4942e0
 */
void FirstPersonWeaponController::interface_tick_reset()
{
    first_person_weapon_interface *fp = &first_person_weapon_interfaces[local_player_index];

    if (fp->weapon_index != (datum_index)halo::k_dword_none) {
        struct object *weapon_object = halo::interface::object_record<struct object>(fp->weapon_index);
        Weapon *weapon_tag_data = halo::interface::tag_data<Weapon>(weapon_object->definition_tag);
        halo::cache::predicted_resource_list_touch(&weapon_tag_data->more_predicted_resources);
    }
    fp->shutdown_countdown = 0x1e;
}

/**
 * Applies weapon HUD action `action_code` to local_player_index's first-person weapon interface: 0 nudges the
 * charge float, 9/10 forward to unit_invalidate_local_player_zoom_level (presumably a reload/swap trigger), 12
 * re-initializes the whole interface, 13 clears the weapon index. Then, unless the weapon index is already
 * clear, validates the action against the weapon's magazine state (only for a "reload-family" animation state
 * or an active magazine) and the weapon tag's +0x4e2 field (first_person_weapon_set_state.c's unresolved
 * weapon-type-shaped enum, required == 1 for actions 9/10); on success, enters animation state 1. Action 12
 * additionally clears blend_end. REWRITTEN from objdump 0x4940f0..0x4942b4. Jump table 0x4942cc/0x4942b8: 0 ->
 * charge += 0.05, 9/10 -> unit_invalidate_local_player_zoom_level(interface unit), 12 -> interface initialize,
 * 13 -> weapon = none. For actions 9/10 on a weapon whose tag +0x4e2 == 1 the reload marker at +0x1e94 is
 * recomputed. Marker -1 enters state 0xd and marker 0 or 2 enters state 0xf. Otherwise (marker 1, other
 * actions or no weapon) the state is item_type_to_message_stage(action) unless that is -1. The draft dropped
 * the zoom-invalidate unit argument and always entered state 0.
 * blam-cc: stack -> local_player_index, action_code
 *
 * @address 0x4940f0
 */
void FirstPersonWeaponController::process_action(int16_t action_code)
{
    first_person_weapon_interface *fp;
    uint8_t *fp_raw;
    int16_t new_state = -1;

    if (local_player_index == -1) {
        return;
    }
    fp = &first_person_weapon_interfaces[local_player_index];
    fp_raw = (uint8_t *)fp;

    switch (action_code) {
        case 0:
            fp->charge = fp->charge + 0.05f;
            break;
        case 9:
        case 10:
            halo::game::unit_invalidate_local_player_zoom_level(fp->unit_index);
            break;
        case 0xc:
            halo::interface::first_person_weapon_interface_initialize(local_player_index);
            break;
        case 0xd:
            fp->weapon_index = (datum_index)halo::k_dword_none;
            break;
    }

    if (fp->weapon_index != (datum_index)halo::k_dword_none) {
        weapon_object *weapon_obj = halo::interface::object_record<weapon_object>(fp->weapon_index);
        datum_index definition = weapon_obj->base.definition_tag;

        if (definition != (datum_index)halo::k_dword_none) {
            uint8_t *weapon_tag_data = (uint8_t *)halo::cache::globals().tag_instances[(uint16_t)definition].data;

            if (static_cast<int16_t>(((struct Weapon *)weapon_tag_data)->weapon_type) == 1 && (action_code == 9 || action_code == 10)) {
                WeaponMagazine *magazine_def = halo::interface::reflexive_elements<WeaponMagazine>(((struct Weapon *)weapon_tag_data)->magazines);
                int16_t rounds_loaded = weapon_obj->weapon.magazines[0].rounds_loaded;
                int16_t rounds_unloaded = weapon_obj->weapon.magazines[0].rounds_unloaded;
                int32_t clamped = (int32_t)magazine_def->rounds_loaded_maximum - (int32_t)rounds_loaded;
                int16_t state = fp->state;
                int16_t marker;

                if (clamped > rounds_unloaded) {
                    clamped = rounds_unloaded;
                }
                if (state == 0xf || state == 0x16 || state == 0x10 || state == 0x11 || state == 0xd ||
                    state == 0xe || weapon_obj->weapon.magazines[0].state != 0) {
                    ((struct first_person_weapon_interface *)fp_raw)->device_reload_marker = (clamped == 1) ? 1 : -1;
                } else {
                    ((struct first_person_weapon_interface *)fp_raw)->device_reload_rounds = (int16_t)clamped;
                    ((struct first_person_weapon_interface *)fp_raw)->device_magazine_empty = (uint8_t)(rounds_loaded == 0);
                    ((struct first_person_weapon_interface *)fp_raw)->device_reload_marker = ((int16_t)clamped == 1) ? 2 : 0;
                }
                marker = ((struct first_person_weapon_interface *)fp_raw)->device_reload_marker;
                if (marker == -1) {
                    new_state = 0xd;
                }
                if (marker == 0 || marker == 2) {
                    new_state = 0xf;
                }
            }
        }
    }

    if (new_state == -1) {
        new_state = halo::interface::item_type_to_message_stage(action_code);
    }
    if (new_state != -1) {
        halo::interface::first_person_weapon_set_state(local_player_index, 1, new_state);
    }
    if (action_code == 0xc) {
        fp->blend_end = 0;
    }
}

/**
 * Attaches or detaches the first-person weapon model for one local player's interface record, (re)binding or
 * releasing its particle markers to match, but only when the requested state differs from the record's current
 * attached flag.
 *
 * @address 0x493e50
 */
void FirstPersonWeaponController::set_attached(uint8_t attached)
{
    first_person_weapon_interface *fp = &first_person_weapon_interfaces[local_player_index];

    if (fp->attached != attached) {
        if (attached != 0) {
            halo::effects::effect_reattach_markers_for_object(local_player_index, fp->weapon_index);
        } else {
            halo::effects::effect_release_first_person_markers(local_player_index);
            halo::effects::particles_delete_by_first_person_weapon((uint8_t)local_player_index);
        }
        fp->attached = attached;
    }
}

/**
 * Validates and applies a first-person weapon animation state transition. new_state (AX) is first remapped
 * 0x13->2 / 0x14->0x15 while the current weapon is overheated (weapon_data.flags & _weapon_overheated_bit,
 * object+0x22c); a battery of per-state gates then either rejects the transition outright or falls through to
 * look up an animation index via item_type_to_animation_stage and the weapon's hud_interface tag message
 * table, applying the new state (and snapshotting the previous pose first, when force_pose_snapshot is set and
 * no blend is already pending) only if that lookup succeeds.
 * blam-cc: AX -> new_state, stack -> local_player_index, force_pose_snapshot
 *
 * @address 0x492e60
 */
void FirstPersonWeaponController::set_state(uint8_t force_pose_snapshot, int16_t new_state)
{
    first_person_weapon_interface *fp;
    uint8_t *weapon_obj;
    int16_t current_state;
    uint8_t reject;
    int16_t animation_stage;
    int16_t blend_gap;
    Weapon *item_tag_data;
    ModelAnimations *hud_tag_data;
    ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *block_a_base;
    int32_t block_a_count;

    fp = &first_person_weapon_interfaces[local_player_index];

    if (fp->weapon_index != (datum_index)halo::k_dword_none) {
        weapon_obj = halo::interface::object_record(fp->weapon_index);
        if ((((weapon_object *)weapon_obj)->weapon.flags & _weapon_overheated_bit) != 0) {
            if (new_state == 0x13) {
                new_state = 2;
            } else if (new_state == 0x14) {
                new_state = 0x15;
            }
        }
    }

    reject = 0;
    switch (new_state) {
    case 6: case 7: case 8: case 9:
        current_state = fp->state;
        if (current_state != 0 && current_state != 5 && current_state != 6 &&
            current_state != 4 && current_state != 0xf && current_state != 0x16 &&
            current_state != 0x10 && current_state != 0x11 && current_state != 0xd &&
            current_state != 0xe) {
            reject = 1;
        }
        break;
    case 0xb: case 0xc:
        if (fp->state != 0 && fp->state != 5) {
            reject = 1;
        }
        break;
    case 0x13:
        if (fp->state == 0x13) {
            return;
        }
        break;
    default:
        break;
    }
    if (reject) {
        return;
    }

    if (new_state == -1) {
        return;
    }
    if (fp->weapon_index == (datum_index)halo::k_dword_none) {
        return;
    }

    weapon_obj = halo::interface::object_record(fp->weapon_index);
    item_tag_data = halo::interface::tag_data<Weapon>(((struct object *)weapon_obj)->definition_tag);
    if (static_cast<int16_t>(item_tag_data->weapon_type) == 3 && new_state == 3 &&
        (((struct weapon_object *)weapon_obj)->weapon.flags & 1) == 0) {
        new_state = 0;
    }

    animation_stage = halo::interface::item_type_to_animation_stage(new_state);

    if (static_cast<int16_t>(item_tag_data->weapon_type) == 1 && fp->state == 0x10) {
        blend_gap = 0;
    } else {
        switch (new_state) {
        case 3: case 10: case 0x13:
            blend_gap = 0;
            break;
        case 6: case 7: case 8: case 9:
            blend_gap = 3;
            break;
        default:
            blend_gap = 6;
            break;
        }
    }

    if (fp->unit_index == (datum_index)halo::k_dword_none) {
        return;
    }

    hud_tag_data = halo::interface::tag_data<ModelAnimations>(halo::interface::tag_handle(item_tag_data->first_person_animations.tag_id));
    block_a_count = (int32_t)hud_tag_data->first_person_weapons.count;
    if (block_a_count == 0) {
        return;
    }
    block_a_base = halo::interface::reflexive_elements<ModelAnimationsAnimationGraphFirstPersonWeaponAnimations>(hud_tag_data->first_person_weapons);
    if (block_a_base == 0) {
        return;
    }
    if (animation_stage < 0 || animation_stage >= (int32_t)block_a_base->animations.count) {
        return;
    }
    animation_stage = halo::interface::reflexive_elements<int16_t>(block_a_base->animations)[animation_stage];
    if (animation_stage == -1) {
        return;
    }

    if (force_pose_snapshot != 0 && fp->frame_sound_index != -1 && fp->frame_sound_state != 1) {
        halo::sound::sound_impulse_fade_out((datum_index)fp->frame_sound_index);
        fp->frame_sound_index = -1;
        fp->frame_sound_state = -1;
    }
    if (blend_gap != 0) {
        halo::interface::first_person_weapon_snapshot_pose(local_player_index, blend_gap);
    }
    fp->state = new_state;
    fp->current_animation = animation_stage;
    *(int16_t *)fp->current_animation_frame = 0;
}

/**
 * Copies the live animation_control block into previous_pose (for blending into the next state), with the copy
 * length derived from the first-person animation graph's node count, then extends blend_end to blend_gap
 * (resetting blend_start to 0) if the current blend window is shorter than blend_gap.
 *
 * @address 0x4930b0
 */
void FirstPersonWeaponController::snapshot_pose(int16_t blend_gap)
{
    first_person_weapon_interface *fp;
    uint8_t *weapon_obj;
    Weapon *item_tag_data;
    ModelAnimations *graph;
    int32_t dword_count;
    uint32_t *src;
    uint32_t *dst;

    fp = &first_person_weapon_interfaces[local_player_index];

    weapon_obj = halo::interface::object_record(fp->weapon_index);
    item_tag_data = halo::interface::tag_data<Weapon>(((struct object *)weapon_obj)->definition_tag);
    graph = halo::interface::tag_data<ModelAnimations>(halo::interface::tag_handle(item_tag_data->first_person_animations.tag_id));
    dword_count = (int32_t)(((uint32_t)graph->nodes.count << 5) >> 2);

    src = (uint32_t *)fp->animation_control;
    dst = (uint32_t *)fp->previous_pose;
    while (dword_count != 0) {
        *dst = *src;
        src++;
        dst++;
        dword_count--;
    }

    if ((int32_t)fp->blend_end - (int32_t)fp->blend_start <= blend_gap) {
        fp->blend_start = 0;
        fp->blend_end = blend_gap;
    }
}

/**
 * Each frame, decides whether the local player's first-person weapon model should be attached: only while its
 * unit and current weapon both exist, the active camera is first-person, and the zoom/action lookup above
 * reports no override in progress. Applies that decision through first_person_weapon_set_attached, then
 * refreshes the animation controls if it ends up attached.
 *
 * @address 0x492430
 */
void FirstPersonWeaponController::update_active_state(void)
{
    first_person_weapon_interface *fp;
    int16_t camera_type;
    int16_t zoom_level;
    uint8_t attach;

    if (current_local_player_index == -1) {
        return;
    }

    fp = &first_person_weapon_interfaces[current_local_player_index];

    if (fp->unit_index == (datum_index)halo::k_dword_none) {
        return;
    }
    if (fp->weapon_index == (datum_index)halo::k_dword_none) {
        return;
    }

    camera_type = halo::camera::camera_get_type_for_player(current_local_player_index);
    if (camera_type == 0) {
        zoom_level = (int16_t)halo::game::local_player_get_zoom_level(current_local_player_index);
        attach = 1;
        if (zoom_level != -1) {
            attach = 0;
        }
    } else {
        attach = 0;
    }

    halo::interface::first_person_weapon_set_attached(current_local_player_index, attach);

    if (fp->attached != 0) {
        halo::interface::first_person_weapon_update_animation_controls(current_local_player_index);
    }
}

/**
 * If the local player's first-person weapon is attached with a valid unit and weapon and that weapon's
 * hud_interface tag is assigned: samples the cluster ambient light at the unit's position, builds a small
 * "dynamic light" params block (armed only while the unit has flag 0x10 set or unknown_37c is positive, in
 * which case it carries unknown_37c/380 and the camera position), then for each of the weapon HUD element and
 * (if the globals have first-person hands assigned) the device HUD element, gathers the matching permuted node
 * records and dispatches them to render_model along with the weapon's own change_colors/function_out_values
 * (for the weapon-HUD case) or the unit's (for the device-HUD case).
 * blam-cc: EAX -> model_tag_ref, ECX -> node_records, 11 stack arguments (objdump 0x492636..0x49266a)
 * blam-cc: none
 *
 * @address 0x4924b0
 */
void FirstPersonWeaponController::update_lighting(void)
{
    first_person_weapon_interface *fp;
    uint32_t player_handle;
    uint32_t unit_handle;
    player *player_record;
    object *unit_obj;
    object *weapon_obj;
    char *weapon_tag_data;
    char *hud_interface_tag_data;
    GlobalsFirstPersonInterface *first_person_interface;
    int32_t light_sample;
    uint8_t node_scratch[3332];

    first_person_light_parameters light_params;

    if (current_local_player_index == -1) {
        return;
    }
    fp = &first_person_weapon_interfaces[current_local_player_index];

    if (current_local_player_index >= 1) {
        return;
    }
    player_handle = halo::game::globals().local_player_globals->local_players[current_local_player_index];
    if (player_handle == halo::k_dword_none) {
        return;
    }
    player_record = halo::interface::player_record(player_handle);
    unit_handle = player_record->unit;
    if (unit_handle == halo::k_dword_none) {
        return;
    }
    if (fp->attached == 0 || fp->unit_index == (datum_index)halo::k_dword_none) {
        return;
    }
    if (fp->weapon_index == (datum_index)halo::k_dword_none) {
        return;
    }

    unit_obj = halo::interface::object_record<object>(unit_handle);
    weapon_obj = halo::interface::object_record<object>(fp->weapon_index);
    weapon_tag_data = (char *)halo::interface::tag_data<Weapon>(weapon_obj->definition_tag);
    if (halo::interface::tag_handle(((struct Weapon *)weapon_tag_data)->first_person_animations.tag_id) == halo::k_dword_none) {
        return;
    }

    first_person_interface = (GlobalsFirstPersonInterface *)global_globals->first_person_interface.pointer;
    light_sample = (int32_t)(uintptr_t)halo::render::object_get_cached_render_lighting((datum_index)unit_handle, 3.4028235e+38f);
    light_params.modifier_shader = 0;

    if (halo::interface::has_bit(((struct unit_object *)unit_obj)->unit.flags, halo::units::unit_flag::active_camouflaged) ||
        ((struct unit_object *)unit_obj)->unit.active_camouflage_power > 0.0f) {
        light_params.unit_37c = ((struct unit_object *)unit_obj)->unit.active_camouflage_power;
        light_params.unit_380 = ((struct unit_object *)unit_obj)->unit.super_active_camouflage_power;
        light_params.type = 1;
        light_params.centroid[0] = render_camera_global;
        light_params.centroid[1] = camera_position_y;
        light_params.centroid[2] = camera_position_z;
        light_params.object_index = unit_handle;
    } else {
        light_params.type = 0;
    }

    if (fp->weapon_hud_valid != 0 && halo::interface::tag_handle(((struct Weapon *)weapon_tag_data)->first_person_model.tag_id) != halo::k_dword_none) {
        uint32_t model_tag_ref = halo::interface::tag_handle(((struct Weapon *)weapon_tag_data)->first_person_model.tag_id);

        halo::interface::hud_meter_permute_node_records(node_scratch, fp->node_matrices, model_tag_ref,
                                        fp->weapon_hud_element);
        halo::models::render_model(static_cast<TagID>(model_tag_ref), node_scratch, 0, 0, weapon_obj->change_colors,
                     weapon_obj->function_out_values, reinterpret_cast<render_lighting *>(light_sample), reinterpret_cast<real_point3d *>(&render_camera_global), 0,
                     reinterpret_cast<render_model_effect *>(&light_params), fp->weapon_index, 0, 8);
    }
    if (fp->device_hud_valid != 0 &&
        halo::interface::tag_handle(((struct GlobalsFirstPersonInterface *)first_person_interface)->first_person_hands.tag_id) != halo::k_dword_none) {
        uint32_t model_tag_ref = *(uint32_t *)&((struct GlobalsFirstPersonInterface *)first_person_interface)->first_person_hands.tag_id;

        halo::interface::hud_meter_permute_node_records(node_scratch, fp->node_matrices, model_tag_ref,
                                        fp->device_hud_element);
        halo::models::render_model(static_cast<TagID>(model_tag_ref), node_scratch, 0, 0, unit_obj->change_colors,
                     unit_obj->function_out_values, reinterpret_cast<render_lighting *>(light_sample), reinterpret_cast<real_point3d *>(&render_camera_global), 0,
                     reinterpret_cast<render_model_effect *>(&light_params), fp->weapon_index, 0, 8);
    }
}

/**
 *
 * @address 0x494730
 */
void FirstPersonWeaponController::update_screen_effects(void)
{
    auto post_hud = []() {
        halo::interface::hud_update_player();
        if (halo::game::globals().current_engine != nullptr) {
            if ((int32_t)halo::game::globals().state > 1) {
                halo::game::game_engine_post_rasterize_post_game();
                return;
            }
            halo::game::hud_update_teammate_nameplate_fade();
        }
    };
    float intensity;
    weapon_screen_effect_parameters parameters;
    int32_t hud_interface;
    WeaponHUDInterfaceScreenEffect *effect;
    int16_t desired_zoom_level;
    uint8_t zoomed;
    float amount;

    if (current_local_player_index == -1) {
        return;
    }

    hud_interface = halo::interface::local_player_get_weapon_hud_interface(&intensity);
    if (hud_interface == -1 ||
        (int32_t)(halo::interface::tag_data<WeaponHUDInterface>(hud_interface))->screen_effect.count < 1) {
        if (rasterizer_device_version >= 0xffff0101u && rasterizer_caps_flag_68a == 0) {
            halo::rasterizer::rasterizer_screen_effect_render((weapon_screen_effect_parameters *)0);
        } else {
            halo::rasterizer::rasterizer_screen_effect_render_fixed_function((weapon_screen_effect_parameters *)0);
        }
        post_hud();
        return;
    }

    effect = (WeaponHUDInterfaceScreenEffect *)
        (halo::interface::tag_data<WeaponHUDInterface>(hud_interface))->screen_effect.pointer;
    desired_zoom_level = -1;
    if (current_local_player_index != -1) {
        desired_zoom_level =
            halo::game::globals().player_control->local_players[current_local_player_index].desired_zoom_level;
    }
    zoomed = (desired_zoom_level != -1);
    memset(&parameters, 0, sizeof(parameters));

    if (zoomed || !halo::interface::has_bit(effect->mask_flags, halo::tags::weapon_hud_interface_screen_effect_definition_mask_tag_flag::only_when_zoomed)) {
        datum_index mask = (halo::main::render_local_view_count() > 1) ? halo::interface::tag_handle(effect->mask_splitscreen.tag_id)
                                                : halo::interface::tag_handle(effect->mask_fullscreen.tag_id);
        if (mask != k_datum_index_none) {
            parameters.mask_bitmap_data =
                halo::interface::tag_data<Bitmap>(mask)->bitmap_data.pointer;
            parameters.night_vision_masked = (uint8_t)((effect->even_more_flags >> 2) & 1);
            parameters.desaturation_masked = (uint8_t)((effect->desaturation_flags >> 3) & 1);
        }
    }

    if (halo::main::render_local_view_count() <= 1 && (zoomed || !halo::interface::has_bit(effect->convolution_flags, halo::tags::weapon_hud_interface_screen_effect_definition_mask_tag_flag::only_when_zoomed))) {
        if (effect->convolution_fov_in_bounds[0] == effect->convolution_fov_in_bounds[1]) {
            amount = effect->convolution_radius_out_bounds[1];
        } else {
            float t = clamp_unit((camera_field_of_view - effect->convolution_fov_in_bounds[0]) /
                                 (effect->convolution_fov_in_bounds[1] -
                                  effect->convolution_fov_in_bounds[0]));
            amount = (1.0f - t) * effect->convolution_radius_out_bounds[0] +
                     t * effect->convolution_radius_out_bounds[1];
        }
        if (amount > 0.0f) {
            parameters.convolution_amount = amount;
            parameters.convolution_type = 2;
        }
    }

    if (zoomed || !halo::interface::has_bit(effect->even_more_flags, halo::tags::weapon_hud_interface_screen_effect_definition_night_vision_tag_flag::only_when_zoomed)) {
        float value = effect->night_vision_intensity;
        if (halo::interface::has_bit(effect->even_more_flags, halo::tags::weapon_hud_interface_screen_effect_definition_night_vision_tag_flag::connect_to_flashlight)) {
            value = clamp_unit(intensity) * value;
        }
        value = script_source_value((uint16_t)effect->night_vision_script_source) * value;
        if (value > 0.0f) {
            parameters.night_vision_intensity = value;
        }
    }

    if (zoomed || !halo::interface::has_bit(effect->desaturation_flags, halo::tags::weapon_hud_interface_screen_effect_definition_desaturation_tag_flag::only_when_zoomed)) {
        float value = effect->desaturation_intensity;
        if (halo::interface::has_bit(effect->desaturation_flags, halo::tags::weapon_hud_interface_screen_effect_definition_desaturation_tag_flag::connect_to_flashlight)) {
            value = clamp_unit(intensity) * value;
        }
        value = script_source_value((uint16_t)effect->desaturation_script_source) * value;
        if (value > 0.0f) {
            parameters.desaturation_intensity = value;
            parameters.desaturation_additive = (uint8_t)((effect->desaturation_flags >> 2) & 1);
            parameters.desaturation_tint[0] = effect->effect_tint.red;
            parameters.desaturation_tint[1] = effect->effect_tint.green;
            parameters.desaturation_tint[2] = effect->effect_tint.blue;
        }
    }

    if (rasterizer_device_version >= 0xffff0101u && rasterizer_caps_flag_68a == 0) {
        halo::rasterizer::rasterizer_screen_effect_render(&parameters);
    } else {
        halo::rasterizer::rasterizer_screen_effect_render_fixed_function(&parameters);
    }

    post_hud();
}

/**
 * Looks at the local player's current first-person weapon animation state and either leaves it alone (states
 * 3, 4, and the default case), decrements a countdown in place (state 0x12), or requests a transition via
 * first_person_weapon_set_state: states 0xd/0xe and 0xf each gate on the weapon tag's +0x4e2 field and a
 * device_hud_element entry before either resetting to state 0 or falling through to the shared 0x10/0x11
 * "ready" transition that every other listed state (0, 5-11, 0x10, 0x11, 0x13, 0x14, 0x16 directly; 1, 2,
 * 0x15, 0x17 to state 3 instead) reaches.
 *
 * @address 0x492d20
 */
void FirstPersonWeaponController::update_state()
{
    first_person_weapon_interface *fp;
    uint8_t *fpb;
    uint8_t *weapon_obj;
    Weapon *item_tag_data;
    int16_t device_entry;
    uint8_t flag;

    fp = &first_person_weapon_interfaces[local_player_index];
    fpb = (uint8_t *)fp;

    switch (fp->state) {
    case 0: case 5: case 6: case 7: case 8: case 9: case 10: case 0xb: case 0xc:
    case 0x10: case 0x11: case 0x13: case 0x14: case 0x16:
        halo::interface::first_person_weapon_set_state(local_player_index, 0, 0);
        return;
    case 1: case 2: case 0x15: case 0x17:
        halo::interface::first_person_weapon_set_state(local_player_index, 0, 3);
        return;
    case 3: case 4:
        return;
    case 0xd: case 0xe:
        weapon_obj = halo::interface::object_record(fp->weapon_index);
        item_tag_data = halo::interface::tag_data<Weapon>(*(uint32_t *)weapon_obj);
        device_entry = ((struct first_person_weapon_interface *)fpb)->device_reload_marker;
        if (static_cast<int16_t>(item_tag_data->weapon_type) != 1 || device_entry == 0 || device_entry == -1) {
            halo::interface::first_person_weapon_set_state(local_player_index, 0, 0);
            return;
        }
        break;
    case 0xf:
        weapon_obj = halo::interface::object_record(fp->weapon_index);
        item_tag_data = halo::interface::tag_data<Weapon>(*(uint32_t *)weapon_obj);
        if (static_cast<int16_t>(item_tag_data->weapon_type) != 1 || ((struct first_person_weapon_interface *)fpb)->device_reload_marker != 2) {
            halo::interface::first_person_weapon_set_state(local_player_index, 0, 0);
            return;
        }
        break;
    case 0x12:
        *(int16_t *)(((struct first_person_weapon_interface *)fpb)->current_animation_frame) = *(int16_t *)(((struct first_person_weapon_interface *)fpb)->current_animation_frame) - 1;
        return;
    default:
        return;
    }

    flag = ((struct first_person_weapon_interface *)fpb)->device_magazine_empty;
    halo::interface::first_person_weapon_set_state(local_player_index, 0, (flag != 0) ? 0x10 : 0x11);
}

}
