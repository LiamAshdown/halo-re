#include "halo/interface/ifr1_first_person_weapon_controller.hpp"
#include <stdint.h>
#include <string.h>
#include "halo/cache/api.hpp"

extern "C" {
extern first_person_weapon_interface *first_person_weapon_interfaces;
extern int32_t local_player_index_for_unit(datum_index unit_index);
extern uint32_t first_person_weapon_get_marker_data(datum_index weapon_index, const char *marker_name,
                                                      object_marker *out, uint32_t name_arg);
extern void *object_try_and_get(datum_index object_index, uint32_t mask);
extern int16_t camera_get_type_for_player(int16_t player_index);
extern int32_t local_player_index_for_weapon(datum_index weapon_index);
extern int16_t model_markers_get_by_name(datum_index model_tag_id, const char *name, uint8_t *region_permutations,
    int16_t *node_remap, real_matrix4x3 *node_matrices, uint8_t mirrored, object_marker *out, int16_t maximum);
extern data_array *object_data;
extern Globals *global_globals;
extern void effect_reattach_markers_for_object(int16_t local_player_index, datum_index weapon_index);
extern void effect_release_first_person_markers(int16_t local_player_index);
extern void particles_delete_by_first_person_weapon(uint8_t local_player_index);
extern void first_person_weapon_set_state(int16_t local_player_index, uint8_t force_pose_snapshot,
                                           int16_t new_state);
extern uint8_t hud_meter_find_matching_elements(uint32_t source_tag_ref, uint32_t target_tag_ref,
                                                 int16_t *out);
extern void first_person_weapon_interface_tick_reset(int16_t local_player_index);
extern player_globals *local_player_globals;
extern data_array *player_data;
extern void first_person_weapon_interface_initialize(int16_t local_player_index);
extern void first_person_weapon_update(int32_t local_player_index);
extern void unit_invalidate_local_player_zoom_level(datum_index unit);
extern int16_t item_type_to_message_stage(int16_t item_type_code);
extern int16_t item_type_to_animation_stage(int16_t message_stage);
extern void first_person_weapon_snapshot_pose(int16_t local_player_index, int16_t blend_gap);
extern void sound_impulse_fade_out(datum_index sound_index);
extern int16_t current_local_player_index;
extern int32_t local_player_get_zoom_level(int16_t local_player_index);
extern void first_person_weapon_set_attached(int16_t local_player_index, uint8_t attached);
extern void first_person_weapon_update_animation_controls(int16_t local_player_index);
extern float render_camera_global;
extern float camera_position_y;
extern float camera_position_z;
extern void hud_meter_permute_node_records(uint8_t *dest, uint8_t *source,
                                            uint32_t target_tag_ref, int16_t *lookup);
extern void *object_get_cached_render_lighting(datum_index object_index, real level_of_detail_pixels);
extern void render_model(uint32_t model_tag_ref, uint8_t *node_records,
                          int32_t unknown_0, int32_t unknown_1, ColorRGB *change_colors,
                          float *function_out_values, int32_t light_sample, float *camera_position,
                          int32_t unknown_6, first_person_light_parameters *light_params,
                          datum_index weapon_index, int32_t unknown_9, int32_t unknown_10);
extern player_control_globals *player_control_globals_ptr;
extern float camera_field_of_view;
extern uint32_t rasterizer_device_version;
extern uint8_t rasterizer_caps_flag_68a;
extern game_engine_definition *current_game_engine;
extern game_engine_state game_engine_state_value;
extern int32_t local_player_get_weapon_hud_interface(float *out_intensity);
extern int16_t render_local_view_count(void);
extern float cinematic_screen_effect_get_script_value(uint16_t source);
extern void rasterizer_screen_effect_render(weapon_screen_effect_parameters *parameters);
extern void rasterizer_screen_effect_render_fixed_function(weapon_screen_effect_parameters *parameters);
extern void hud_update_player(void);
extern void game_engine_post_rasterize_post_game(void);
extern void hud_update_teammate_nameplate_fade(void);
}

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
    if (cinematic_screen_effect_get_script_value(source) < 0.0f) {
        return 0.0f;
    }
    if (cinematic_screen_effect_get_script_value(source) > 1.0f) {
        return 1.0f;
    }
    return cinematic_screen_effect_get_script_value(source);
}

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

    local_player = local_player_index_for_unit(unit_index);
    if ((int16_t)local_player == -1) {
        return;
    }

    fp = &first_person_weapon_interfaces[local_player];
    if (fp->attached == 0) {
        return;
    }

    result = (int16_t)first_person_weapon_get_marker_data(fp->weapon_index, "flashlight", &marker, 1);
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
    uint8_t *item_tag_data;

    obj = (object *)object_try_and_get(weapon_index, 4);
    if (obj == 0) {
        return 0;
    }

    local_player = local_player_index_for_weapon(weapon_index);
    if ((int16_t)local_player == -1) {
        return 0;
    }

    camera_type = camera_get_type_for_player((int16_t)local_player);
    if (camera_type != 0) {
        return 0;
    }

    fp = &first_person_weapon_interfaces[local_player];
    item_tag_data = *(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances +
                                   (obj->definition_tag & 0xffff) * 0x20 + 0x14);

    if (fp->weapon_hud_valid != 0 && *(int32_t *)(item_tag_data + 0x468) != -1 &&
        *(int32_t *)(item_tag_data + 0x478) != -1) {
        return (uint32_t)model_markers_get_by_name(*(datum_index *)(item_tag_data + 0x468), marker_name,
            (uint8_t *)0, fp->weapon_hud_element, (real_matrix4x3 *)fp->node_matrices, 0, out, (int16_t)maximum);
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
    char *weapon_tag_data;
    char *hud_interface_tag_data;
    int32_t *node_array_block;
    int16_t marker_node_index;
    uint8_t weapon_hud_matched;
    uint32_t weapon_tag_ref;
    uint32_t hud_interface_tag_ref;

    fp->weapon_index = (datum_index)0xffffffff;

    if (was_attached != 0) {
        effect_release_first_person_markers(local_player_index);
        particles_delete_by_first_person_weapon((uint8_t)local_player_index);
        fp->attached = 0;
    }

    if (fp->unit_index == (datum_index)0xffffffff) {
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    unit = *(struct object **)((char *)object_data->data + 8 +
                                (uint16_t)fp->unit_index * 0xc);
    current_weapon_slot = ((unit_object *)unit)->unit.current_weapon_index;
    if (current_weapon_slot == -1) {
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }
    weapon_index = *(datum_index *)((char *)unit + 0x2f8 + current_weapon_slot * 4);
    if (weapon_index == (datum_index)0xffffffff) {
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    weapon_object = *(struct object **)((char *)object_data->data + 8 +
                                         (uint16_t)weapon_index * 0xc);
    weapon_tag_ref = *(uint32_t *)weapon_object;
    weapon_tag_data = (char *)halo::cache::globals().tag_instances[(uint16_t)weapon_tag_ref].data;
    if (*(int32_t *)(weapon_tag_data + 0x468) == -1) {
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    hud_interface_tag_ref = *(uint32_t *)(weapon_tag_data + 0x478);
    hud_interface_tag_data = (char *)halo::cache::globals().tag_instances[(uint16_t)hud_interface_tag_ref].data;
    if (*(int32_t *)(hud_interface_tag_data + 0x48) == 0) {
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }
    node_array_block = *(int32_t **)(hud_interface_tag_data + 0x4c);
    if (node_array_block == (int32_t *)0) {
        first_person_weapon_interface_tick_reset(local_player_index);
        return;
    }

    fp->animation_index = -1;
    if (node_array_block[4] > 4) {
        marker_node_index = *(int16_t *)(*(int32_t *)((char *)node_array_block + 0x14) + 8);
        if (marker_node_index != -1 &&
            *(int16_t *)(*(char **)(hud_interface_tag_data + 0x78) + 0x22 +
                          (int32_t)marker_node_index * 0xb4) >= 9) {
            fp->animation_index = marker_node_index;
        }
    }

    {
        uint32_t hands_model = *(uint32_t *)(*(int32_t *)((char *)global_globals + 0x180) + 0xc);

        if (hands_model != 0xffffffff) {
            fp->device_hud_valid = hud_meter_find_matching_elements(hud_interface_tag_ref, hands_model,
                fp->device_hud_element);
        }
    }
    weapon_hud_matched = hud_meter_find_matching_elements(hud_interface_tag_ref,
        *(uint32_t *)(weapon_tag_data + 0x468), fp->weapon_hud_element);
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
        first_person_weapon_set_state(local_player_index, 1, 0);
        fp->blend_end = 0;
        if (was_attached != 0 && fp->attached != 1) {
            effect_reattach_markers_for_object(local_player_index, fp->weapon_index);
            fp->attached = 1;
        }
    }

    first_person_weapon_interface_tick_reset(local_player_index);
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

    if (local_player_globals->local_players[0] != (datum_index)0xffffffff) {
        record = (player *)((char *)player_data->data +
                             (local_player_globals->local_players[0] & 0xffff) * sizeof(player));
        unit_index = record->unit;

        if (fp->unit_index != unit_index) {
            fp->unknown_30[0x20] = 0;
            fp->unit_index = unit_index;
            first_person_weapon_interface_initialize(0);
        }
        if (fp->weapon_index == (datum_index)0xffffffff) {
            first_person_weapon_interface_initialize(0);
        }
        first_person_weapon_update(0);
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

    if (fp->weapon_index != (datum_index)0xffffffff) {
        struct object *weapon_object = *(struct object **)((char *)object_data->data + 8 +
                                                             (uint16_t)fp->weapon_index * 0xc);
        char *weapon_tag_data =
            (char *)halo::cache::globals().tag_instances[(uint16_t)(*(uint32_t *)weapon_object)].data;
        halo::cache::predicted_resource_list_touch(&((Weapon *)weapon_tag_data)->more_predicted_resources);
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
    int16_t new_state;

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
            unit_invalidate_local_player_zoom_level(fp->unit_index);
            break;
        case 0xc:
            first_person_weapon_interface_initialize(local_player_index);
            break;
        case 0xd:
            fp->weapon_index = (datum_index)0xffffffff;
            break;
    }

    if (fp->weapon_index != (datum_index)0xffffffff) {
        uint8_t *weapon_obj = *(uint8_t **)((char *)object_data->data + 8 + (uint16_t)fp->weapon_index * 0xc);
        datum_index definition = *(datum_index *)weapon_obj;

        if (definition != (datum_index)0xffffffff) {
            uint8_t *weapon_tag_data = (uint8_t *)halo::cache::globals().tag_instances[(uint16_t)definition].data;

            if (*(int16_t *)(weapon_tag_data + 0x4e2) == 1 && (action_code == 9 || action_code == 10)) {
                uint8_t *magazine_def = *(uint8_t **)(weapon_tag_data + 0x4f4);
                int16_t rounds_loaded = *(int16_t *)(weapon_obj + 0x2b8);
                int16_t rounds_unloaded = *(int16_t *)(weapon_obj + 0x2b6);
                int32_t clamped = (int32_t)*(int16_t *)(magazine_def + 10) - (int32_t)rounds_loaded;
                int16_t state = fp->state;
                int16_t marker;

                if (clamped > rounds_unloaded) {
                    clamped = rounds_unloaded;
                }
                if (state == 0xf || state == 0x16 || state == 0x10 || state == 0x11 || state == 0xd ||
                    state == 0xe || *(int16_t *)(weapon_obj + 0x2b0) != 0) {
                    *(int16_t *)(fp_raw + 0x1e94) = (clamped == 1) ? 1 : -1;
                } else {
                    *(int16_t *)(fp_raw + 0x1e92) = (int16_t)clamped;
                    *(uint8_t *)(fp_raw + 0x1e90) = (uint8_t)(rounds_loaded == 0);
                    *(int16_t *)(fp_raw + 0x1e94) = ((int16_t)clamped == 1) ? 2 : 0;
                }
                marker = *(int16_t *)(fp_raw + 0x1e94);
                if (marker == -1) {
                    new_state = 0xd;
                    goto set_state;
                }
                if (marker == 0 || marker == 2) {
                    new_state = 0xf;
                    goto set_state;
                }
            }
        }
    }

    new_state = item_type_to_message_stage(action_code);
    if (new_state == -1) {
        goto skip_state_change;
    }
set_state:
    first_person_weapon_set_state(local_player_index, 1, new_state);
skip_state_change:
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
            effect_reattach_markers_for_object(local_player_index, fp->weapon_index);
        } else {
            effect_release_first_person_markers(local_player_index);
            particles_delete_by_first_person_weapon((uint8_t)local_player_index);
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
    uint8_t *item_tag_data;
    uint8_t *hud_tag_data;
    uint8_t *block_a_base;
    int32_t block_a_count;

    fp = &first_person_weapon_interfaces[local_player_index];

    if (fp->weapon_index != (datum_index)0xffffffff) {
        weapon_obj = (uint8_t *)((object_header *)object_data->data)[fp->weapon_index & 0xffff].data;
        if ((*(uint8_t *)(weapon_obj + 0x22c) & 1) != 0) {
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
    if (fp->weapon_index == (datum_index)0xffffffff) {
        return;
    }

    weapon_obj = (uint8_t *)((object_header *)object_data->data)[fp->weapon_index & 0xffff].data;
    item_tag_data = *(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances +
                                   (*(uint32_t *)weapon_obj & 0xffff) * 0x20 + 0x14);
    if (*(int16_t *)(item_tag_data + 0x4e2) == 3 && new_state == 3 &&
        (*(uint32_t *)(weapon_obj + 0x22c) & 1) == 0) {
        new_state = 0;
    }

    animation_stage = item_type_to_animation_stage(new_state);

    if (*(int16_t *)(item_tag_data + 0x4e2) == 1 && fp->state == 0x10) {
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

    if (fp->unit_index == (datum_index)0xffffffff) {
        return;
    }

    hud_tag_data = *(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances +
                                  ((*(uint32_t *)(item_tag_data + 0x478)) & 0xffff) * 0x20 + 0x14);
    block_a_count = *(int32_t *)(hud_tag_data + 0x48);
    if (block_a_count == 0) {
        return;
    }
    block_a_base = *(uint8_t **)(hud_tag_data + 0x4c);
    if (block_a_base == 0) {
        return;
    }
    if (animation_stage < 0 || animation_stage >= *(int32_t *)(block_a_base + 0x10)) {
        return;
    }
    animation_stage = *(int16_t *)(*(uint8_t **)(block_a_base + 0x14) + animation_stage * 2);
    if (animation_stage == -1) {
        return;
    }

    if (force_pose_snapshot != 0 && fp->frame_sound_index != -1 && fp->frame_sound_state != 1) {
        sound_impulse_fade_out((datum_index)fp->frame_sound_index);
        fp->frame_sound_index = -1;
        fp->frame_sound_state = -1;
    }
    if (blend_gap != 0) {
        first_person_weapon_snapshot_pose(local_player_index, blend_gap);
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
    uint8_t *item_tag_data;
    ModelAnimations *graph;
    int32_t dword_count;
    uint32_t *src;
    uint32_t *dst;

    fp = &first_person_weapon_interfaces[local_player_index];

    weapon_obj = (uint8_t *)((object_header *)object_data->data)[fp->weapon_index & 0xffff].data;
    item_tag_data = *(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances +
                                   (*(uint32_t *)weapon_obj & 0xffff) * 0x20 + 0x14);
    graph = (ModelAnimations *)*(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances +
                                  (*(uint32_t *)(item_tag_data + 0x478) & 0xffff) * 0x20 + 0x14);
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

    if (fp->unit_index == (datum_index)0xffffffff) {
        return;
    }
    if (fp->weapon_index == (datum_index)0xffffffff) {
        return;
    }

    camera_type = camera_get_type_for_player(current_local_player_index);
    if (camera_type == 0) {
        zoom_level = (int16_t)local_player_get_zoom_level(current_local_player_index);
        attach = 1;
        if (zoom_level != -1) {
            attach = 0;
        }
    } else {
        attach = 0;
    }

    first_person_weapon_set_attached(current_local_player_index, attach);

    if (fp->attached != 0) {
        first_person_weapon_update_animation_controls(current_local_player_index);
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
    player_handle = local_player_globals->local_players[current_local_player_index];
    if (player_handle == 0xffffffff) {
        return;
    }
    player_record = (player *)((char *)player_data->data + (uint16_t)player_handle * player_data->size);
    unit_handle = player_record->unit;
    if (unit_handle == 0xffffffff) {
        return;
    }
    if (fp->attached == 0 || fp->unit_index == (datum_index)0xffffffff) {
        return;
    }
    if (fp->weapon_index == (datum_index)0xffffffff) {
        return;
    }

    unit_obj = *(object **)((char *)object_data->data + 8 + (uint16_t)unit_handle * 0xc);
    weapon_obj = *(object **)((char *)object_data->data + 8 + (uint16_t)fp->weapon_index * 0xc);
    weapon_tag_data = (char *)halo::cache::globals().tag_instances[(uint16_t)weapon_obj->definition_tag].data;
    if (*(int32_t *)(weapon_tag_data + 0x478) == -1) {
        return;
    }

    first_person_interface = (GlobalsFirstPersonInterface *)global_globals->first_person_interface.pointer;
    light_sample = (int32_t)(uintptr_t)object_get_cached_render_lighting((datum_index)unit_handle, 3.4028235e+38f);
    light_params.modifier_shader = 0;

    if ((*(uint8_t *)((char *)unit_obj + 0x204) & 0x10) != 0 ||
        *(float *)((char *)unit_obj + 0x37c) > 0.0f) {
        light_params.unit_37c = *(float *)((char *)unit_obj + 0x37c);
        light_params.unit_380 = *(float *)((char *)unit_obj + 0x380);
        light_params.type = 1;
        light_params.centroid[0] = render_camera_global;
        light_params.centroid[1] = camera_position_y;
        light_params.centroid[2] = camera_position_z;
        light_params.object_index = unit_handle;
    } else {
        light_params.type = 0;
    }

    if (fp->weapon_hud_valid != 0 && *(int32_t *)(weapon_tag_data + 0x468) != -1) {
        uint32_t model_tag_ref = *(uint32_t *)(weapon_tag_data + 0x468);

        hud_meter_permute_node_records(node_scratch, fp->node_matrices, model_tag_ref,
                                        fp->weapon_hud_element);
        render_model(model_tag_ref, node_scratch, 0, 0, (ColorRGB *)((char *)weapon_obj + 0x1b8),
                     (float *)((char *)weapon_obj + 0x134), light_sample, &render_camera_global, 0,
                     &light_params, fp->weapon_index, 0, 8);
    }
    if (fp->device_hud_valid != 0 &&
        *(int32_t *)&((struct GlobalsFirstPersonInterface *)first_person_interface)->first_person_hands.tag_id != -1) {
        uint32_t model_tag_ref = *(uint32_t *)&((struct GlobalsFirstPersonInterface *)first_person_interface)->first_person_hands.tag_id;

        hud_meter_permute_node_records(node_scratch, fp->node_matrices, model_tag_ref,
                                        fp->device_hud_element);
        render_model(model_tag_ref, node_scratch, 0, 0, (ColorRGB *)((char *)unit_obj + 0x1b8),
                     (float *)((char *)unit_obj + 0x134), light_sample, &render_camera_global, 0,
                     &light_params, fp->weapon_index, 0, 8);
    }
}

/**
 * Original engine function first_person_weapon_update_screen_effects; the author notes are in
 * docs/original/interface/first_person_weapon_update_screen_effects.txt.
 *
 * @address 0x494730
 */
void FirstPersonWeaponController::update_screen_effects(void)
{
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

    hud_interface = local_player_get_weapon_hud_interface(&intensity);
    if (hud_interface == -1 ||
        (int32_t)((WeaponHUDInterface *)halo::cache::globals().tag_instances[hud_interface & 0xffff].data)->screen_effect.count < 1) {
        if (rasterizer_device_version >= 0xffff0101u && rasterizer_caps_flag_68a == 0) {
            rasterizer_screen_effect_render((weapon_screen_effect_parameters *)0);
        } else {
            rasterizer_screen_effect_render_fixed_function((weapon_screen_effect_parameters *)0);
        }
        goto post_hud;
    }

    effect = (WeaponHUDInterfaceScreenEffect *)
        ((WeaponHUDInterface *)halo::cache::globals().tag_instances[hud_interface & 0xffff].data)->screen_effect.pointer;
    desired_zoom_level = -1;
    if (current_local_player_index != -1) {
        desired_zoom_level =
            player_control_globals_ptr->local_players[current_local_player_index].desired_zoom_level;
    }
    zoomed = (desired_zoom_level != -1);
    memset(&parameters, 0, sizeof(parameters));

    if (zoomed || (effect->mask_flags & 1) == 0) {
        datum_index mask = (render_local_view_count() > 1) ? *(datum_index *)&effect->mask_splitscreen.tag_id
                                                : *(datum_index *)&effect->mask_fullscreen.tag_id;
        if (mask != (datum_index)-1) {
            parameters.mask_bitmap_data =
                *(uint32_t *)((uint8_t *)halo::cache::globals().tag_instances[mask & 0xffff].data + 0x64);
            parameters.night_vision_masked = (uint8_t)((effect->even_more_flags >> 2) & 1);
            parameters.desaturation_masked = (uint8_t)((effect->desaturation_flags >> 3) & 1);
        }
    }

    if (render_local_view_count() <= 1 && (zoomed || (effect->convolution_flags & 1) == 0)) {
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

    if (zoomed || (effect->even_more_flags & 1) == 0) {
        float value = effect->night_vision_intensity;
        if ((effect->even_more_flags & 2) != 0) {
            value = clamp_unit(intensity) * value;
        }
        value = script_source_value((uint16_t)effect->night_vision_script_source) * value;
        if (value > 0.0f) {
            parameters.night_vision_intensity = value;
        }
    }

    if (zoomed || (effect->desaturation_flags & 1) == 0) {
        float value = effect->desaturation_intensity;
        if ((effect->desaturation_flags & 2) != 0) {
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
        rasterizer_screen_effect_render(&parameters);
    } else {
        rasterizer_screen_effect_render_fixed_function(&parameters);
    }

post_hud:
    hud_update_player();
    if (current_game_engine != (void *)0) {
        if ((int32_t)game_engine_state_value > 1) {
            game_engine_post_rasterize_post_game();
            return;
        }
        hud_update_teammate_nameplate_fade();
    }
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
    uint8_t *item_tag_data;
    int16_t device_entry;
    uint8_t flag;

    fp = &first_person_weapon_interfaces[local_player_index];
    fpb = (uint8_t *)fp;

    switch (fp->state) {
    case 0: case 5: case 6: case 7: case 8: case 9: case 10: case 0xb: case 0xc:
    case 0x10: case 0x11: case 0x13: case 0x14: case 0x16:
        first_person_weapon_set_state(local_player_index, 0, 0);
        return;
    case 1: case 2: case 0x15: case 0x17:
        first_person_weapon_set_state(local_player_index, 0, 3);
        return;
    case 3: case 4:
        return;
    case 0xd: case 0xe:
        weapon_obj = (uint8_t *)((object_header *)object_data->data)[fp->weapon_index & 0xffff].data;
        item_tag_data = *(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances +
                                       (*(uint32_t *)weapon_obj & 0xffff) * 0x20 + 0x14);
        device_entry = *(int16_t *)(fpb + 0x1e94);
        if (*(int16_t *)(item_tag_data + 0x4e2) != 1 || device_entry == 0 || device_entry == -1) {
            first_person_weapon_set_state(local_player_index, 0, 0);
            return;
        }
        break;
    case 0xf:
        weapon_obj = (uint8_t *)((object_header *)object_data->data)[fp->weapon_index & 0xffff].data;
        item_tag_data = *(uint8_t **)((uint8_t *)halo::cache::globals().tag_instances +
                                       (*(uint32_t *)weapon_obj & 0xffff) * 0x20 + 0x14);
        if (*(int16_t *)(item_tag_data + 0x4e2) != 1 || *(int16_t *)(fpb + 0x1e94) != 2) {
            first_person_weapon_set_state(local_player_index, 0, 0);
            return;
        }
        break;
    case 0x12:
        *(int16_t *)(fpb + 0x18) = *(int16_t *)(fpb + 0x18) - 1;
        return;
    default:
        return;
    }

    flag = *(uint8_t *)(fpb + 0x1e90);
    first_person_weapon_set_state(local_player_index, 0, (flag != 0) ? 0x10 : 0x11);
}

}
