#include "halo/game/gamerest_player.hpp"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/scenario/api.hpp"
#include <string.h>
#include <stdint.h>
#include <wchar.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/devices/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern void *const network_index_cache_table;
extern data_array *player_data;
extern player_globals *local_player_globals;
extern uint8_t *main_game_globals;
extern void player_trigger_shield_recharge_effect(uint32_t player_index);
extern void player_trigger_full_health_effect(uint32_t player_index);
extern uint8_t player_add_kill_streak(int32_t slot, int16_t amount, uint32_t player_handle);
extern void player_trigger_kill_streak_effect(uint32_t player_index);
extern void hud_post_item_message(int16_t count, int32_t source, uint8_t kind, int16_t local_player_index, int8_t machine_id);
extern game_time_globals *game_time;
extern uint8_t player_find_placement_position(uint32_t player_index, datum_index target_object, real_point3d *point);
extern void player_set_pending_interaction_action(int16_t priority_type, int16_t seat, uint32_t player_index, uint32_t candidate_object);
extern void hud_add_item_message(int16_t local_player_index, int32_t source, uint8_t source_kind, int16_t count);
extern void player_apply_pickup_effect(uint32_t player_index, uint32_t pickup_object);
extern uint8_t player_is_busy_with_interaction(uint32_t candidate_object, uint32_t unit_or_player_index);
extern void unit_invalidate_local_player_zoom_level(datum_index unit);
extern void game_engine_notify_player_interaction(uint32_t primary_key, uint32_t edi_key, uint32_t mode, int32_t interaction_type, int32_t interaction_seat, int32_t secondary_key);
extern Globals *global_globals;
extern double cos(double x);
extern double fabs(double x);
extern void player_update_history_free_all(void *queue);
extern real_vector3d *global_origin3d_pointer;
extern real_point3d player_placement_ring[9];
extern game_engine_definition *current_game_engine;
extern game_engine_state game_engine_state_value;
extern player_control_globals *player_control_globals_ptr;
extern data_array *update_server_queues;
extern uint16_t global_006889d0;
extern uint16_t global_007102e4;
extern uint32_t global_006889e0;
extern uint32_t global_007102e8;
extern uint32_t global_006889d4;
extern uint32_t global_007102ec;
extern uint32_t global_006889d8;
extern uint32_t global_006889dc;
extern double sqrt(double x);
extern int32_t multikill_medal_threshold;
extern int32_t sv_tk_grace_ticks;
extern int32_t sv_tk_cooldown_ticks;
extern uint8_t shared_hud_text_draw_state;
extern uint8_t *machine_table;
extern uint8_t network_message_scratch[0x7ff8];
extern uint16_t global_006889e4;
extern uint16_t global_007102f0;
extern uint32_t global_006889f4;
extern uint32_t global_007102f4;
extern uint32_t global_006889e8;
extern uint32_t global_006889f0;
extern uint32_t global_006889ec;
extern uint32_t global_007102f8;
extern real weapon_get_zoom_magnification(datum_index item_index, int16_t zoom_level);
extern wchar_t empty_string;
extern datum_index machine_to_player[16];
extern void player_update_queue_create(player_update_queue *queue);
extern void position_update_queue_create(circular_queue *queue);
extern void vehicle_update_queue_create(circular_queue *queue);
extern void game_engine_player_changed_object(uint32_t param);
extern void network_queue_destroy(circular_queue *queue);
extern data_array *update_client_queues;
extern player_profile player_profile_cache[16];
extern int32_t player_profile_cache_count;
extern void player_delete(uint32_t machine_index, datum_index player_handle);
extern int32_t game_engine_player_profile_cache_find(datum_index player_handle);
extern object_type_definition *object_type_definitions[k_maximum_object_types];
extern int32_t catchup_backlog_threshold;
extern int32_t catchup_time_threshold;
extern void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch, real_vector3d *out_forward);
extern uint8_t player_unit_has_parent(datum_index player_handle);
extern void apply_remote_player_position_update(player *plr, object *unit_obj);
extern void apply_remote_player_vehicle_position_update(player *plr, object *unit_obj);
extern data_array *team_data;
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern float camera_point[];
extern float camera_position_y_table[];
extern float camera_position_z_table[];
}

namespace {
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[(h) & 0xffff].data)
#define OBJECT_HEADER(h) (((object_header *)halo::objects::globals().object_data->data)[(h) & 0xffff])
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
static void player_unit_exit_seat(uint32_t object_index, datum_index vehicle_index)
{
    uint8_t *self = OBJECT_DATA(object_index);
    uint8_t *vehicle = OBJECT_DATA(vehicle_index);
    uint8_t *nodes = self + ((struct object *)self)->nodes.offset;
    uint8_t *seat = *(uint8_t **)(TAG_DATA(*(datum_index *)vehicle) + 0x2e8) + ((::unit_object *)self)->unit.vehicle_seat_index * 0x11c;
    uint8_t *model_nodes;
    object_marker marker;
    real_point3d offset;
    real_point3d default_translation;
    real_point3d position;
    real_matrix4x3 basis;

    halo::objects::object_get_node_local_transform(vehicle_index, (char *)(seat + 0x24), &marker, 1);
    offset.x = *(float *)(nodes + 0x28) - marker.node_transform.position.x;
    offset.y = *(float *)(nodes + 0x2c) - marker.node_transform.position.y;
    offset.z = *(float *)(nodes + 0x30) - marker.node_transform.position.z;
    model_nodes = *(uint8_t **)(TAG_DATA(*(datum_index *)(TAG_DATA(*(datum_index *)self) + 0x34)) + 0xbc);
    default_translation = *(real_point3d *)(model_nodes + 0x28);
    if (((vehicle_object *)vehicle)->unit.driver_unit_index == object_index && vehicle[0x2a3] != 0x25 &&
        ((struct object *)self)->parent_object != k_datum_index_none) {
        halo::units::unit_try_set_animation_state(((struct object *)self)->parent_object, 0x25);
    }
    ((::unit_object *)self)->unit.last_parent_object_index = vehicle_index;
    ((::unit_object *)self)->unit.last_seat_change_tick = game_time->game_time;
    if (((::unit_object *)self)->unit.driver_unit_index == object_index) {
        ((::unit_object *)self)->unit.driver_unit_index = k_datum_index_none;
    }
    if (((::unit_object *)self)->unit.gunner_unit_index == object_index) {
        ((::unit_object *)self)->unit.gunner_unit_index = k_datum_index_none;
    }
    halo::objects::object_snap_to_parent_marker_and_detach(object_index);
    position.x = offset.x + ((struct object *)self)->position.x;
    position.y = offset.y + ((struct object *)self)->position.y;
    position.z = offset.z + ((struct object *)self)->position.z - default_translation.z;
    halo::objects::object_set_position_and_orientation(object_index, 0, 0, &position);
    {
        uint8_t *reloaded = OBJECT_DATA(object_index);

        halo::math::matrix4x3_multiply((real_matrix4x3 *)(reloaded + ((struct object *)reloaded)->nodes.offset),
            (real_matrix4x3 *)(model_nodes + 0x68), &basis);
    }
    *(real_vector3d *)&((struct object *)self)->forward.i = basis.forward;
    *(real_vector3d *)&((struct object *)self)->up.i = basis.up;
    {
        uint8_t *object = OBJECT_DATA(object_index);
        uint8_t *object_tag = TAG_DATA(*(datum_index *)object);

        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1 && (object[0x10] & 1) != 0) {
            halo::objects::object_for_each_light_attachment(object_index, 0, 1);
        }
        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
            *(uint32_t *)(object + 0x10) &= ~1u;
            OBJECT_HEADER(object_index).flags |= 2;
        }
    }
    ((::unit_object *)self)->unit.vehicle_seat_index = -1;
    self[0x2a7] = 2;
    if (((vehicle_object *)vehicle)->unit.driver_unit_index == object_index) {
        ((vehicle_object *)vehicle)->unit.driver_unit_index = k_datum_index_none;
    }
    if (((vehicle_object *)vehicle)->unit.gunner_unit_index == object_index) {
        ((vehicle_object *)vehicle)->unit.gunner_unit_index = k_datum_index_none;
    }
    halo::units::unit_recompute_seat_occupants(vehicle_index);
    halo::units::unit_pick_and_ready_next_weapon(object_index);
    {
        int8_t request[2] = { 0x14, 0 };

        halo::units::unit_update_animation_state_machine(object_index, request);
    }
    *(real_point3d *)(self + ((struct object *)self)->node_function_values.offset + 0x10) = default_translation;
    if (((struct object *)self)->type == 0) {
        halo::units::unit_reset_orientation_and_find_position(object_index, vehicle_index);
    }
    halo::objects::object_recalculate_bounding_radius_recursive(object_index);
    if (halo::units::unit_all_seats_unoccupied(vehicle_index) == 1) {
        uint8_t *empty = (uint8_t *)halo::objects::object_try_and_get(vehicle_index, 2);

        if (empty != 0) {
            *(int32_t *)(empty + 0x5ac) = game_time->game_time;
        }
    }
    if (halo::networking::globals().game_mode == 1) {
        uint8_t *player = (uint8_t *)halo::memory::datum_get(((::unit_object *)self)->unit.controlling_player, player_data);

        if (player != 0 && ((struct player *)player)->local_player_index == -1) {
            ((struct player *)player)->position_updates.read_index = 0;
            ((struct player *)player)->position_updates.write_index = 0;
            ((struct player *)player)->vehicle_updates.read_index = 0;
            ((struct player *)player)->vehicle_updates.write_index = 0;
        }
    }
}
static int32_t update_queue_count(const circular_queue *queue)
{
    int32_t write_index = queue->write_index;
    int32_t read_index = queue->read_index;

    if (write_index > read_index) {
        return write_index - read_index;
    }
    if (write_index < read_index) {
        return (queue->capacity - read_index) + write_index;
    }
    return 0;
}
static void players_clear_bsp_cluster(void)
{
    data_iterator iterator;
    player *entry;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)iterator.data ^ k_data_iterator_signature;
    while ((entry = (player *)halo::memory::data_iterator_next(&iterator)) != 0) {
        entry->bsp_cluster = -1;
    }
}
}

namespace halo::game {

/**
 * Applies the pickup effect of `pickup_object`'s tag (discriminated by its +0x308 field) to
 * `player_index`, then always notifies via hud_post_item_message/equipment_pickup_play_sound and deletes the pickup.
 * No-op (does not delete the pickup) if the source amount (__ftol'd from an elided float) is
 * not positive, or if the specific effect's own gate (shield recharge / full vitality restore /
 * kill-streak) reports failure.
 * REWRITTEN from objdump 0x479930..0x479a98: the amount is __ftol(powerup tag +0x30c * 30) tested as a 16-bit value;
 * the shield / health calls take the player's UNIT (player+0x34) in EAX; the kill-streak effect fires when the low 16
 * bits of the slot are zero (test bp,bp); the HUD message gets count 0, the pickup's definition tag as its source,
 * kind 0, the local player index and the byte at player+0x64; the sound gets the pickup object; object_delete tail.
 * VERIFIED 2026-09-27 (static loop): 0x672ac8 = 30.0f; player_add_kill_streak takes EBX player (blam-cc) and stack
 * (slot, amount); the three player_trigger_* effects take EDX player; all callee conventions match.
 *
 * @address 0x479930
 */
void PlayerView::apply_pickup_effect(uint32_t pickup_object)
{
    player *p = halo::game::player_at(player_index);
    object *pickup = (object *)((object_header *)halo::objects::globals().object_data->data)[pickup_object & halo::k_datum_slot_mask].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[pickup->definition_tag & halo::k_datum_slot_mask].data;
    int16_t amount = (int16_t)(int32_t)(*(float *)(tag + 0x30c) * 30.0f);
    int16_t discriminator;

    if (amount < 1) {
        return;
    }

    discriminator = *(int16_t *)(tag + 0x308);
    if (discriminator == 1) {
        local_player_globals->respawn_stagger = local_player_globals->respawn_stagger + amount;
        main_game_globals[2] = 1;
    } else if (discriminator == 2) {
        if (halo::objects::object_shield_recharge_start(p->unit) == 0) {
            return;
        }
        if (halo::networking::globals().game_mode == 0) {
            PlayerView(player_index).trigger_shield_recharge_effect();
        }
    } else if (discriminator == 5) {
        if (halo::objects::object_restore_full_body_vitality(p->unit) == 0) {
            return;
        }
        if (halo::networking::globals().game_mode == 0) {
            PlayerView(player_index).trigger_full_health_effect();
        }
    } else {
        int32_t slot = (int32_t)player_index;
        if (discriminator == 3) {
            slot = 0;
        } else if (discriminator == 4) {
            slot = 1;
        }
        if (KillStreak(player_index).add_kill_streak(slot, amount) == 0) {
            return;
        }
        if ((int16_t)slot == 0 && halo::networking::globals().game_mode == 0) {
            KillStreak(player_index).trigger_kill_streak_effect();
        }
    }

    halo::interface::hud_post_item_message(0, (int32_t)pickup->definition_tag, 0, p->local_player_index,
                          (int8_t)*((uint8_t *)p + 0x64));
    if (p->local_player_index != -1) {
        halo::items::equipment_pickup_play_sound(pickup_object);
    }
    halo::objects::object_delete(pickup_object);
}

/**
 * 0x475cf9..0x476002: the seat-exit inline (same as unit_detach_from_seat's biped_detach_from_seat).
 *
 * @address 0x475c60
 */
uint8_t PlayerView::attach_unit_to_parent(uint32_t target_object, void *local_offset)
{
    uint32_t unit_index = (halo::game::player_at(player_index))->unit;
    uint8_t *biped = (uint8_t *)halo::objects::object_try_and_get(unit_index, 1);

    if (biped == 0) {
        return 0;
    }
    if (((biped_object *)biped)->base.parent_object != k_datum_index_none && halo::networking::globals().game_mode != 1) {
        uint8_t *self = OBJECT_DATA(unit_index);
        datum_index parent = ((struct object *)self)->parent_object;

        if (parent != k_datum_index_none && ((::unit_object *)self)->unit.vehicle_seat_index != -1) {
            player_unit_exit_seat(unit_index, parent);
        }

        if (((struct object *)self)->network_role == 0) {
            halo::units::unit_dispatch_scripted_event_9(1, (int32_t)unit_index);
        }
        if (halo::networking::globals().game_mode == 1) {
            datum_index player_handle = ((::unit_object *)self)->unit.controlling_player;
            int16_t index = (int16_t)player_handle;
            int16_t salt = (int16_t)(player_handle >> 16);

            if (player_handle != k_datum_index_none && index >= 0 &&
                index < player_data->maximum_count) {
                uint8_t *player = (uint8_t *)player_data->data + player_data->size * index;

                if (*(int16_t *)player != 0 && (salt == 0 || *(int16_t *)player == salt) &&
                    ((struct player *)player)->local_player_index != -1 && halo::networking::globals().client != 0) {
                    halo::networking::player_update_history_free_all((player_update_history *)(*(void **)&halo::networking::globals().client->update_history));
                }
            }
        }
    }
    return PlayerView(player_index).find_placement_position(target_object, (real_point3d *)local_offset);
}

/**
 * Checks whether `player_index`'s unit is positioned and facing correctly to assassinate
 * `candidate_object`: the candidate's bounding sphere must contain the player's camera position
 * along the unit's own aiming direction, the candidate device/unit must be front-facing toward
 * that aim, and device_can_change_position must approve the candidate. If all three pass, queues the
 * assassination interaction (priority 10).
 *
 * @address 0x478770
 */
void PlayerView::check_assassination_opportunity(uint32_t candidate_object)
{
    player *p = halo::game::player_at(player_index);
    object *unit = (object *)((object_header *)halo::objects::globals().object_data->data)[p->unit & halo::k_datum_slot_mask].data;
    object *candidate = (object *)((object_header *)halo::objects::globals().object_data->data)[candidate_object & halo::k_datum_slot_mask].data;
    real_point3d camera_position;

    halo::units::unit_get_camera_position(p->unit, &camera_position);

    if (halo::math::ray_intersects_sphere_test(candidate->bounding_center, camera_position,
            *(&((unit_data *)((uint8_t *)unit + k_unit_data_offset))->aiming_vector), candidate->bounding_radius)) {
        if (halo::devices::device_frontfacing(candidate_object, &((unit_data *)((uint8_t *)unit + k_unit_data_offset))->aiming_vector)) {
            if (halo::devices::device_can_change_position(candidate_object)) {
                PlayerView(player_index).set_pending_interaction_action(10, (int16_t)halo::k_word_none, candidate_object);
            }
        }
    }
}

/**
 * Implements the original `player_check_vehicle_boarding_interaction`.
 *
 * @address 0x4788a0
 */
void PlayerView::check_vehicle_boarding_interaction(uint32_t candidate_object)
{
    uint8_t *record = (uint8_t *)halo::game::player_at(player_index);
    datum_index unit_index = ((player *)record)->unit;
    uint8_t *unit = OBJECT_DATA(unit_index);
    uint8_t *item = OBJECT_DATA(candidate_object);
    int16_t local_player_index = ((player *)record)->local_player_index;
    int8_t machine = (int8_t)record[0x64];
    uint8_t *equipment;
    uint8_t *weapon;
    uint8_t *weapon_tag;
    uint8_t dual_flagged;
    uint8_t keep_current;
    datum_index current_weapon;
    int32_t weapon_count;
    int16_t i;

    if (((struct item_object *)item)->base.parent_object != k_datum_index_none || ((struct item_object *)item)->item.ignore_object_index == unit_index) {
        return;
    }
    for (i = 0; i < 4; i++) {
        datum_index carried = *(datum_index *)(unit + 0x2f8 + i * 4);
        int16_t transferred;

        if (carried != k_datum_index_none &&
            (uint8_t)halo::items::weapon_transfer_ammunition(carried, candidate_object, local_player_index, &transferred)) {
            if (transferred > 0) {
                halo::interface::hud_post_item_message(transferred, (int32_t)*(datum_index *)OBJECT_DATA(carried), 1,
                    local_player_index, machine);
            }
            break;
        }
    }

    equipment = (uint8_t *)halo::objects::object_try_and_get(candidate_object, 8);
    if (equipment != 0) {
        uint8_t *equipment_tag = TAG_DATA(*(datum_index *)equipment);
        int16_t type = *(int16_t *)(equipment_tag + 0x308);

        if (type == 6) {
            if (halo::units::unit_try_give_grenade(candidate_object, unit_index)) {
                halo::interface::hud_post_item_message(1, (int32_t)*(datum_index *)equipment, 0xff, local_player_index, machine);
            }
        } else if (type != 0) {
            if (*(datum_index *)(OBJECT_DATA(unit_index) + 0x318) == k_datum_index_none) {
                PlayerView(player_index).apply_pickup_effect(candidate_object);
            } else if (type != *(int16_t *)(equipment_tag + 0x308)) {

                PlayerView(player_index).set_pending_interaction_action(5, -1, candidate_object);
            }
        }
    }

    weapon = (uint8_t *)halo::objects::object_try_and_get(candidate_object, 4);
    if (weapon == 0 || !halo::units::unit_check_weapon_use_permission(unit_index, candidate_object)) {
        return;
    }
    weapon_tag = TAG_DATA(*(datum_index *)weapon);
    dual_flagged = (uint8_t)((((unit_object *)unit)->unit.control_flags & 0x1800) != 0);
    {
        uint8_t *holder = OBJECT_DATA(unit_index);
        int16_t slot = *(int16_t *)(holder + 0x2f2);

        current_weapon = (slot != -1) ? *(datum_index *)(holder + 0x2f8 + slot * 4) : k_datum_index_none;
    }
    weapon_count = halo::units::unit_count_deployed_weapons(unit_index);
    keep_current = 0;
    if (weapon_count >= 2 && current_weapon != k_datum_index_none && (weapon_tag[0x308] & 0x10) == 0 &&
        (TAG_DATA(*(datum_index *)OBJECT_DATA(current_weapon))[0x308] & 0x10) != 0) {
        keep_current = 1;
    }
    if (dual_flagged && (weapon_tag[0x308] & 8)) {
        return;
    }
    if (PlayerView(unit_index).is_busy_with_interaction(candidate_object)) {
        datum_index tag;

        if (!halo::units::unit_pickup_weapon(1, candidate_object, unit_index)) {
            return;
        }
        tag = *(datum_index *)OBJECT_DATA(candidate_object);
        if (halo::networking::globals().game_mode == 2) {
            halo::interface::hud_post_item_message(0, (int32_t)tag, 0, local_player_index, machine);
        } else {
            halo::interface::hud_add_item_message(local_player_index, (int32_t)tag, 0, 0);
        }
        LocalPlayerUnit(unit_index).invalidate_local_player_zoom_level();
        if (halo::networking::globals().game_mode == 2) {
            halo::game::game_engine_notify_player_interaction(player_index, candidate_object, 1, 7, -1, -1);
        }
        return;
    }
    if (keep_current || !halo::units::unit_weapon_is_best_of_type(candidate_object, unit_index)) {
        return;
    }
    {
        uint8_t *current = (uint8_t *)halo::objects::object_try_and_get(current_weapon, 4);

        if (weapon_count == 1 && current != 0 && *(datum_index *)current != *(datum_index *)weapon) {
            PlayerView(player_index).set_pending_interaction_action(7, -1, candidate_object);
        } else {
            PlayerView(player_index).set_pending_interaction_action(6, -1, candidate_object);
        }
    }
}

/**
 * Lightweight version of player_check_vehicle_boarding_interaction: no ammo-transfer or device
 * handling, only the weapon-pickup/swap/assassination decision.
 * REWRITTEN from objdump. The weapon-pickup prompt: an unparented weapon (type mask 4) that is not the player's
 *   unit's own +0x200 object, which the unit may use (ESI unit, EDI weapon), unless the unit has flag 0x1800 and
 *   the weapon tag +0x308 bit 3, or the player is busy (ESI weapon, EDI unit), or the unit already carries two
 *   weapons and holds a +0x308 bit 4 weapon while this one is not, and only when it is the best of its type
 *   (EAX weapon, ECX unit). The action is 7 when the unit holds exactly one weapon whose definition differs,
 *   else 6. The draft called four helpers without arguments, asked for weapon slot -1, and re-fetched the
 *   candidate where the binary fetches the current weapon.
 *
 * @address 0x478c40
 */
void PlayerView::check_vehicle_boarding_interaction_lightweight(uint32_t candidate_object)
{
    player *p = halo::game::player_at(player_index);
    datum_index unit_handle = p->unit;
    uint8_t *unit_obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[unit_handle & halo::k_datum_slot_mask].data;
    uint8_t *candidate = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[candidate_object & halo::k_datum_slot_mask].data;
    object *weapon_candidate;
    uint8_t *weapon_tag;
    uint8_t unit_flag_1800;
    datum_index current_weapon;
    int32_t weapon_count;
    uint8_t holds_exclusive = 0;
    object *current_weapon_obj;

    if (((object *)candidate)->parent_object != (datum_index)halo::k_dword_none ||
        *(uint32_t *)(candidate + 0x200) == (uint32_t)unit_handle) {
        return;
    }
    weapon_candidate = halo::objects::object_try_and_get(candidate_object, 4);
    if (weapon_candidate == 0 || halo::units::unit_check_weapon_use_permission((uint32_t)unit_handle, candidate_object) == 0) {
        return;
    }
    weapon_tag = (uint8_t *)halo::cache::globals().tag_instances[weapon_candidate->definition_tag & halo::k_datum_slot_mask].data;
    unit_flag_1800 = (uint8_t)((*(uint32_t *)(unit_obj + 0x208) & 0x1800) != 0);

    current_weapon = halo::units::unit_get_weapon_object_index((uint32_t)p->unit,
        *(int16_t *)((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[p->unit & 0xffff].data + 0x2f2));
    weapon_count = halo::units::unit_count_deployed_weapons((uint32_t)p->unit);
    if (weapon_count >= 2 && current_weapon != (datum_index)halo::k_dword_none && (weapon_tag[0x308] & 0x10) == 0) {
        object *held = ((object_header *)halo::objects::globals().object_data->data)[current_weapon & halo::k_datum_slot_mask].data;

        if ((((uint8_t *)halo::cache::globals().tag_instances[held->definition_tag & halo::k_datum_slot_mask].data)[0x308] & 0x10) != 0) {
            holds_exclusive = 1;
        }
    }
    if (unit_flag_1800 && (weapon_tag[0x308] & 8) != 0) {
        return;
    }
    if (PlayerView((uint32_t)p->unit).is_busy_with_interaction(candidate_object) != 0 || holds_exclusive) {
        return;
    }
    if (halo::units::unit_weapon_is_best_of_type(candidate_object, (uint32_t)p->unit) == 0) {
        return;
    }
    current_weapon_obj = halo::objects::object_try_and_get(current_weapon, 4);
    if (weapon_count == 1 && current_weapon_obj != 0 &&
        current_weapon_obj->definition_tag != weapon_candidate->definition_tag) {
        PlayerView(player_index).set_pending_interaction_action(7, (int16_t)halo::k_word_none, candidate_object);
    } else {
        PlayerView(player_index).set_pending_interaction_action(6, (int16_t)halo::k_word_none, candidate_object);
    }
}

/**
 * Examines whether `player_index`'s unit can interact with the vehicle `candidate_object`.
 * Unless it is vitality-frozen: if the vehicle is tipped past minimum_angle_for_vehicle_flipping
 * (its up.z at or below cos(pi/2 - that angle)), clears the player's pending interaction (when
 * nothing already occupies it); otherwise, if both the player's own unit and the vehicle are
 * nearly stationary, looks for a seat to enter and sets the pending interaction to "enter as
 * driver" (9) or "enter as passenger" (8) accordingly.
 *
 * @address 0x478600
 */
void PlayerView::check_vehicle_interaction(uint32_t candidate_object)
{
    object *vehicle = (object *)((object_header *)halo::objects::globals().object_data->data)[candidate_object & halo::k_datum_slot_mask].data;

    if ((*((uint8_t *)&vehicle->vitality_flags) & 4) == 0) {
        GlobalsPlayerControl *player_control = (GlobalsPlayerControl *)global_globals->player_control.pointer;
        double flip_threshold = cos(1.5707963705062866 - (double)player_control[0].minimum_angle_for_vehicle_flipping);

        if ((double)vehicle->up.k <= flip_threshold) {
            if ((*(uint8_t *)&((vehicle_object *)vehicle)->vehicle.flags & 0x10) == 0 &&
                *(int32_t *)&((vehicle_object *)vehicle)->unit.driver_unit_index == -1) {

                PlayerView(player_index).set_pending_interaction_action(0xb, (int16_t)halo::k_word_none, candidate_object);
            }
        } else {
            player *p = halo::game::player_at(player_index);
            uint32_t unit_index = (uint32_t)p->unit;

            if (halo::units::unit_current_weapon_type_is_2_or_3(unit_index) == 0) {
                object *unit_obj = (object *)((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_datum_slot_mask].data;
                float unit_speed_sq = unit_obj->velocity.k * unit_obj->velocity.k +
                    unit_obj->velocity.j * unit_obj->velocity.j + unit_obj->velocity.i * unit_obj->velocity.i;
                float vehicle_spin_sq = vehicle->angular_velocity.k * vehicle->angular_velocity.k +
                    vehicle->angular_velocity.j * vehicle->angular_velocity.j +
                    vehicle->angular_velocity.i * vehicle->angular_velocity.i;

                if (unit_speed_sq < 0.01f && vehicle_spin_sq < 0.01f) {
                    uint32_t seat = halo::k_dword_none;
                    int16_t result = halo::units::unit_find_best_seat_to_enter(unit_index, candidate_object, (int16_t *)&seat);
                    if (result == 1) {
                        PlayerView(player_index).set_pending_interaction_action(9, (int16_t)seat, candidate_object);
                        return;
                    }
                    if (result == 2) {
                        PlayerView(player_index).set_pending_interaction_action(8, (int16_t)seat, candidate_object);
                        return;
                    }
                }
            }
        }
    }
}

/**
 * Implements the original `player_execute_pending_interaction`.
 *
 * @address 0x4793a0
 */
uint8_t PlayerView::execute_pending_interaction()
{
    uint8_t *record = (uint8_t *)halo::game::player_at(player_index);
    datum_index unit_index = ((player *)record)->unit;
    uint8_t *unit = OBJECT_DATA(unit_index);
    datum_index target_index = ((player *)record)->interaction_object;
    uint16_t seat = *(uint16_t *)&((player *)record)->interaction_seat;
    uint8_t handled = 0;

    switch (((player *)record)->interaction_type) {
    case 5:
        halo::units::unit_clear_selected_equipment(unit_index);
        if (halo::units::unit_try_select_equipment(unit_index, target_index, 0)) {
            halo::interface::hud_post_item_message(0, (int32_t)*(datum_index *)OBJECT_DATA(target_index), 0,
                ((player *)record)->local_player_index, (int8_t)record[0x64]);
        }
        break;
    case 8:
    case 9: {
        uint32_t occupant = k_datum_index_none;

        if (halo::networking::globals().game_mode == 1 &&
            (unit_index == k_datum_index_none || halo::objects::object_try_and_get(unit_index, 3) == 0)) {
            return 0;
        }
        if (halo::networking::globals().game_mode == 1 && !halo::units::unit_seat_is_occupied_by_other(unit_index, (int16_t)seat, target_index,
                                                                      &occupant)) {
            datum_index self_index = ((player *)record)->unit;
            uint8_t *self = (uint8_t *)halo::objects::object_try_and_get(self_index, 3);

            if (self != 0 && self[0x2a3] == 0x1b) {
                halo::units::unit_detach_from_seat(self_index, 1, 1, 0);
            }
        }
        if (halo::units::unit_seat_is_occupied_by_other(((player *)record)->unit, (int16_t)*(uint16_t *)&((player *)record)->interaction_seat,
                                           ((player *)record)->interaction_object, &occupant)) {
            halo::units::unit_enter_vehicle_seat(((player *)record)->interaction_object, (int16_t)*(uint16_t *)&((player *)record)->interaction_seat,
                ((player *)record)->unit);
            handled = 1;
            if (halo::networking::globals().game_mode == 1) {
                if (((player *)record)->local_player_index != -1) {
                    if (halo::networking::globals().client != 0) {
                        halo::networking::player_update_history_free_all((player_update_history *)(*(void **)&halo::networking::globals().client->update_history));
                    }
                } else {
                    ((player *)record)->position_updates.read_index = 0;
                    ((player *)record)->position_updates.write_index = 0;
                    ((player *)record)->vehicle_updates.read_index = 0;
                    ((player *)record)->vehicle_updates.write_index = 0;
                }
            }
            goto notify;
        }
        if (occupant == k_datum_index_none ||
            *(datum_index *)(OBJECT_DATA(occupant) + 0x1f4) == k_datum_index_none) {
            return 0;
        }
        halo::ai::actor_check_vehicle_target_available(((player *)record)->unit,
            *(datum_index *)(OBJECT_DATA(occupant) + 0x1f4), 1);
        break;
    }
    case 10:
        halo::devices::device_control_touched(target_index);
        break;
    case 11: {
        uint8_t *target = OBJECT_DATA(target_index);
        int8_t direction;

        ((unit_object *)unit)->unit.last_parent_object_index = target_index;
        ((unit_object *)unit)->unit.last_seat_change_tick = game_time->game_time;
        if (fabs(((struct object *)target)->forward.k) > 0.7071067690849304) {
            direction = (int8_t)((((struct object *)target)->forward.k < 0.0f) ? 4 : 3);
        } else {
            real_matrix4x3 target_matrix;
            real_matrix4x3 unit_matrix;
            real_point3d *target_position =
                (real_point3d *)((uint8_t *)halo::objects::object_get_world_matrix(target_index, &target_matrix) + 0x28);
            real_point3d *unit_position =
                (real_point3d *)((uint8_t *)halo::objects::object_get_world_matrix(((player *)record)->unit, &unit_matrix) + 0x28);
            real_vector3d side;

            side.i = target_position->x - unit_position->x;
            side.j = target_position->y - unit_position->y;
            side.k = target_position->z - unit_position->z;
            halo::math::vector3d_cross_product(side, side, *halo::math::globals().global_up3d_pointer);
            direction = (int8_t)((side.k * ((struct object *)target)->forward.k + side.j * ((struct object *)target)->forward.j +
                                  side.i * ((struct object *)target)->forward.i > 0.0f) ? 2 : 1);
        }
        target[0x4cc] |= 0x10;
        target[0x4d1] = (uint8_t)direction;
        target[0x4d2] = 0;
        break;
    }
    default:
        return 0;
    }
    handled = 1;
notify:
    if (((unit_object *)unit)->base.network_role == 0) {
        halo::game::game_engine_notify_player_interaction(player_index, ((player *)record)->interaction_object, 0,
            *(uint16_t *)&((player *)record)->interaction_type, *(uint16_t *)&((player *)record)->interaction_seat, -1);
    }
    return handled;
}

/**
 * Implements the original `player_execute_weapon_drop_interaction`.
 *
 * @address 0x4790d0
 */
uint8_t PlayerView::execute_weapon_drop_interaction()
{
    uint8_t *record = (uint8_t *)halo::game::player_at(player_index);
    datum_index unit_index = ((player *)record)->unit;
    uint8_t *unit = OBJECT_DATA(unit_index);
    datum_index held_weapon = k_datum_index_none;
    uint8_t result = 0;

    switch (((player *)record)->interaction_type) {
    case 6: {
        uint8_t *current = OBJECT_DATA(unit_index);
        int16_t slot = *(int16_t *)(current + 0x2f2);
        uint8_t picked_up = 0;

        if (slot != -1) {
            held_weapon = *(datum_index *)(current + 0x2f8 + slot * 4);
        }
        if (halo::units::unit_drop_current_weapon(unit_index, 1) &&
            halo::units::unit_pickup_weapon(1, ((player *)record)->interaction_object, unit_index)) {
            halo::interface::hud_add_item_message(((player *)record)->local_player_index,
                (int32_t)*(datum_index *)OBJECT_DATA(((player *)record)->interaction_object), 0, 0);
            LocalPlayerUnit(unit_index).invalidate_local_player_zoom_level();
            picked_up = 1;
        }
        result = 1;
        if (picked_up != 1) {
            return result;
        }
        break;
    }
    case 7:
        if (!halo::units::unit_pickup_weapon(1, ((player *)record)->interaction_object, unit_index)) {
            return 0;
        }
        halo::interface::hud_add_item_message(((player *)record)->local_player_index,
            (int32_t)*(datum_index *)OBJECT_DATA(((player *)record)->interaction_object), 0, 0);
        break;
    default:
        return 0;
    }
    if (((unit_object *)unit)->base.network_role == 0) {
        halo::game::game_engine_notify_player_interaction(player_index, ((player *)record)->interaction_object, 1,
            *(uint16_t *)&((player *)record)->interaction_type, *(uint16_t *)&((player *)record)->interaction_seat, (int32_t)held_weapon);
    }
    return result;
}

/**
 * Implements the original `player_find_placement_position`.
 *
 * @address 0x4757b0
 */
uint8_t PlayerView::find_placement_position(datum_index target_object, real_point3d *point)
{
    uint8_t *player = (uint8_t *)halo::game::player_at(player_index);
    uint32_t unit_index = ((struct player *)player)->unit;
    uint8_t *unit = OBJECT_DATA(unit_index);
    uint8_t placed = 0;
    real_vector3d facing;

    if (target_object == k_datum_index_none ||
        halo::objects::object_get_root_object_index(target_object) == target_object) {
        placed = (uint8_t)halo::units::unit_find_placement_position(unit_index, target_object, 0, 2.0f, 0, 0, 1, 0,
            (real_vector3d *)point);
    } else {
        uint32_t root = halo::objects::object_get_root_object_index(target_object);
        uint8_t *root_object = OBJECT_DATA(root);
        float collision_radius;
        real_matrix4x3 ring;
        int16_t i;

        target_object = root;
        facing = *(real_vector3d *)&((struct object *)root_object)->velocity.i;
        if (!(facing.j * facing.j + facing.i * facing.i > 0.0f)) {
            if (((struct object *)root_object)->forward.k >= 0.70710677f) {
                facing = *(real_vector3d *)&((struct object *)root_object)->up.i;
            } else {
                facing = *(real_vector3d *)&((struct object *)root_object)->forward.i;
            }
        }
        collision_radius = *(float *)((uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)unit & halo::k_datum_slot_mask].data + 0x42c);
        facing.k = 0.0f;
        facing.i = -facing.i;
        facing.j = -facing.j;
        halo::math::vector3d_normalize_with_length(facing);
        halo::math::matrix4x3_from_forward_up(*halo::math::globals().global_up3d_pointer, facing, ring);
        ring.position = *(real_point3d *)&((struct object *)root_object)->bounding_center.x;
        ring.scale = collision_radius * 3.0f + ((struct object *)root_object)->bounding_radius;
        for (i = 0; !placed && (uint16_t)i < 9; i++) {
            real_point3d spot;
            int16_t attempt;

            halo::math::matrix4x3_transform_point(spot, player_placement_ring[i], ring);
            placed = (uint8_t)halo::units::unit_find_placement_position(unit_index, root, 0, 2.0f, 0, 0, 1, 0,
                (real_vector3d *)&spot);
            for (attempt = 0; !placed && attempt < 8; attempt++) {
                real_point3d jittered;
                int16_t index;

                halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
                index = (int16_t)(((halo::math::globals().random_seed_global >> 16) * (int32_t)halo::math::globals().sphere_point_table_count) >> 16);
                facing = *(real_vector3d *)&halo::math::globals().sphere_point_table[index];
                jittered.x = facing.i * collision_radius + spot.x;
                jittered.y = facing.j * collision_radius + spot.y;
                jittered.z = facing.k * collision_radius + spot.z;
                placed = (uint8_t)halo::units::unit_find_placement_position(unit_index, root, 0, 2.0f, 0, 0, 1, 0,
                    (real_vector3d *)&jittered);
            }
        }
    }
    ((struct player *)player)->bsp_cluster = -1;
    if (placed) {
        uint8_t *volumes = *(uint8_t **)((uint8_t *)halo::scenario::globals().scenario + 0x3a0);
        int16_t v;

        for (v = 0; (int32_t)v < *(int32_t *)((uint8_t *)halo::scenario::globals().scenario + 0x39c); v++) {
            uint8_t *volume = *(uint8_t **)((uint8_t *)halo::scenario::globals().scenario + 0x3a0) + v * 8;
            datum_index player_unit = ((struct player *)player)->unit;

            (void)volumes;
            if (*(int16_t *)(volume + 0x2) == halo::scenario::globals().structure_bsp_index && player_unit != k_datum_index_none &&
                halo::scenario::scenario_query::trigger_volume_contains_point(*(int16_t *)volume, (real_point3d *)(OBJECT_DATA(player_unit) + 0xa0))) {
                placed = 0;
                break;
            }
        }
    }
    if (!placed) {
        PlayerView(player_index).release_unit_and_reset((int32_t)target_object);
        return 0;
    }
    *(real_vector3d *)&((unit_object *)unit)->base.velocity.i = *global_origin3d_pointer;
    if (target_object == k_datum_index_none) {
        return placed;
    }
    facing = *(real_vector3d *)(OBJECT_DATA(target_object) + 0x74);
    {

        int16_t index = (int16_t)target_object;
        int16_t salt = (int16_t)(target_object >> 16);

        if (index >= 0 && index < *(int16_t *)((uint8_t *)halo::objects::globals().object_data + 0x20)) {
            uint8_t *header = (uint8_t *)halo::objects::globals().object_data->data + *(int16_t *)((uint8_t *)halo::objects::globals().object_data + 0x22) * index;

            if (*(int16_t *)header != 0 && (salt == 0 || *(int16_t *)header == salt) && header[0x3] == 0) {
                uint8_t *target = *(uint8_t **)(header + 0x8);

                if (target != 0 && *(int32_t *)(target + 0x4d4) != -1) {
                    *(int32_t *)(unit + 0x4d4) = *(int32_t *)(target + 0x4d4);
                    unit[0x4d3] = target[0x4d3];
                }
            }
        }
    }
    *(real_vector3d *)&((unit_object *)unit)->unit.desired_facing_vector.i = facing;
    *(real_vector3d *)&((unit_object *)unit)->unit.desired_aiming_vector.i = facing;
    *(real_vector3d *)&((unit_object *)unit)->unit.desired_looking_vector.i = facing;
    if (((struct player *)player)->local_player_index != -1) {
        halo::game::game_engine_compute_look_angles_from_vector(&facing, ((struct player *)player)->local_player_index);
    }
    {
        datum_index effect = *(datum_index *)((uint8_t *)global_globals->player_information.pointer + 0xc4);

        if (effect != k_datum_index_none) {
            halo::game::game_engine_build_visible_cluster_bitmask((uint32_t *)local_player_globals->cluster_pvs, 0);
            halo::effects::effect_new_on_object(unit_index, effect, unit_index, -1, 0.0f, 0.0f, 0, 0);
        }
    }
    return placed;
}

/**
 * If player_index is valid and its player has a unit: attributes a death for that unit (no
 * credited kill) unless a multiplayer game has already ended, optionally overwrites
 * player::respawn_timer, zeroes the unit's body_vitality, freezes its health, exits it from any
 * vehicle seat, and finally runs player_reset_after_unit_change.
 *
 * @address 0x476250
 */
void PlayerView::kill_and_release_unit(int32_t respawn_timer_override)
{
    player *plr;
    datum_index unit_handle;

    if (player_index == (uint32_t)-1) {
        return;
    }

    plr = halo::game::player_at(player_index);
    unit_handle = plr->unit;
    if (unit_handle == (datum_index)-1) {
        return;
    }

    if (current_game_engine == 0 || game_engine_state_value == _game_engine_state_not_started) {
        halo::game::game_engine_attribute_player_death(unit_handle, (datum_index)-1, (datum_index)-1, -1, 0);
    }

    if (respawn_timer_override != 0) {
        plr->respawn_timer = respawn_timer_override;
    }

    {
        object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[unit_handle & halo::k_datum_slot_mask].data;
        unit_obj->body_vitality = 0.0f;
    }
    halo::objects::object_set_health_frozen_flag(unit_handle);
    halo::units::unit_exit_vehicle_seat(player_index);
    PlayerView(player_index).reset_after_unit_change();
}

/**
 * If player_index's player has a unit: attributes a death for it (crediting kills, unless a
 * multiplayer game is already over), stashes the unit handle into
 * player_globals::local_player_units[local_player_index], resets the player's per-tick state
 * (player_reset_after_unit_change, which clears player::unit), then re-reads the stashed handle
 * to clear its controlling_player, clear object_header flag bit 0 and set object.flags bit 0
 * (propagating light attachments first if needed) on both the unit and its current weapon (if
 * any). Finally, if previous_unit_override is not the wildcard, overwrites
 * player::previous_unit with it, and clears player_globals::no_player_has_a_unit.
 *
 * @address 0x4760b0
 */
void PlayerView::release_unit_and_reset(int32_t previous_unit_override)
{
    player *plr;
    datum_index saved_unit;

    plr = halo::game::player_at(player_index);
    if (plr->unit == (datum_index)-1) {
        return;
    }

    if (current_game_engine == 0 || game_engine_state_value == _game_engine_state_not_started) {
        halo::game::game_engine_attribute_player_death(plr->unit, (datum_index)-1, (datum_index)-1, -1, 1);
    }

    local_player_globals->local_player_units[plr->local_player_index] = plr->unit;
    PlayerView(player_index).reset_after_unit_change();

    saved_unit = local_player_globals->local_player_units[plr->local_player_index];
    {
        object_header *unit_header = &((object_header *)halo::objects::globals().object_data->data)[saved_unit & halo::k_datum_slot_mask];
        object *unit_obj = unit_header->data;
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        datum_index weapon_handle = (datum_index)-1;

        if (unit->current_weapon_index != -1) {
            weapon_handle = unit->weapons[unit->current_weapon_index];
        }
        unit->controlling_player = (datum_index)-1;

        if ((unit_header->flags & 1) != 0) {
            unit_header->flags = unit_header->flags & ~1;
        }
        {
            uint8_t *unit_tag_data = (uint8_t *)halo::cache::globals().tag_instances[unit_obj->definition_tag & halo::k_datum_slot_mask].data;
            if (*(int32_t *)(unit_tag_data + 0x34) != -1 && (unit_obj->flags & 1) == 0) {
                halo::objects::object_for_each_light_attachment(1, 0, 0);
            }
        }
        unit_obj->flags = unit_obj->flags | 1;
        unit_header->flags = unit_header->flags & ~2;

        if (weapon_handle != (datum_index)-1) {
            object_header *weapon_header = &((object_header *)halo::objects::globals().object_data->data)[weapon_handle & halo::k_datum_slot_mask];
            object *weapon_obj = weapon_header->data;
            uint8_t *weapon_tag_data = (uint8_t *)halo::cache::globals().tag_instances[weapon_obj->definition_tag & halo::k_datum_slot_mask].data;
            if (*(int32_t *)(weapon_tag_data + 0x34) != -1 && (weapon_obj->flags & 1) == 0) {
                halo::objects::object_for_each_light_attachment(1, 0, 0);
            }
            weapon_obj->flags = weapon_obj->flags | 1;
            weapon_header->flags = weapon_header->flags & ~2;
        }
    }

    if (previous_unit_override != -1) {
        plr->previous_unit = (datum_index)previous_unit_override;
    }
    local_player_globals->no_player_has_a_unit = 0;
}

/**
 * Called after a player's controlled unit has just changed. Rolls the old unit into
 * previous_unit and clears unit; if the player is a local player, resets its whole
 * local_player_control record to defaults (unit -1, weapon/grenade/zoom -1, nameplate -1, pitch
 * clamps, everything else zeroed). Recomputes player_globals::no_player_has_a_unit by scanning
 * every player. If this player is now marked for deletion, removes it outright and returns.
 * Otherwise, while hosting, clears this player's server update-queue write/read cursors; while a
 * non-local player and there is a network client, frees its update-history queues; while a local
 * player and this machine is a network client, clears the three embedded queues' write/read
 * cursors and three of the update_history current-record slots.
 *
 * @address 0x474e10
 */
void PlayerView::reset_after_unit_change()
{
    player *plr;
    data_iterator iter;
    player *scan;

    plr = halo::game::player_at(player_index);
    plr->previous_unit = plr->unit;
    plr->unit = (datum_index)-1;

    if (plr->local_player_index != -1) {
        local_player_control *look =
            &player_control_globals_ptr->local_players[plr->local_player_index];

        memset(look, 0, sizeof(*look));
        look->unit = (datum_index)-1;
        look->desired_weapon_index = -1;
        look->desired_grenade_index = -1;
        look->desired_zoom_level = -1;
        look->autolevelling_active = 0;
        look->nameplate_target = (datum_index)-1;
        look->pitch_maximum = 1.4906585f;
        look->pitch_minimum = -1.4906585f;
        look->suppressed_buttons = 0;
        look->suppressed_until_released = 0;
    }

    local_player_globals->no_player_has_a_unit = 1;
    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    scan = (player *)halo::memory::data_iterator_next(&iter);
    while (scan != (player *)0) {
        if (scan->unit != (datum_index)-1) {
            local_player_globals->no_player_has_a_unit = 0;
        }
        scan = (player *)halo::memory::data_iterator_next(&iter);
    }

    if (plr->marked_for_deletion == 1) {
        Players::remove_player(player_index);
        return;
    }

    if (halo::networking::globals().game_mode == 2) {
        update_server_queue *entry =
            &((update_server_queue *)update_server_queues->data)[player_index];
        plr->unknown_f4 = (datum_index)-1;
        entry->queue.queue.read_index = 0;
        entry->queue.queue.write_index = 0;
        *(int32_t *)((uint8_t *)entry + 8) = 0;
        *(int32_t *)((uint8_t *)entry + 0x14) = 0;
        *(int32_t *)((uint8_t *)entry + 0x18) = 0;
    }

    if (plr->local_player_index != -1) {
        if (halo::networking::globals().client != 0) {
            halo::networking::player_update_history_free_all((player_update_history *)(*(void **)&halo::networking::globals().client->update_history));
        }
    } else if (halo::networking::globals().game_mode == 1) {
        plr->update_history.queue.read_index = 0;
        plr->update_history.queue.write_index = 0;
        plr->position_updates.read_index = 0;
        plr->position_updates.write_index = 0;
        plr->vehicle_updates.read_index = 0;
        plr->vehicle_updates.write_index = 0;
        plr->update_history.current[0] = 0;
        plr->update_history.current[3] = 0;
        plr->update_history.current[4] = 0;
    }
}

/**
 * blam-cc: EDX -> player_index
 *
 * @address 0x479890
 */
void PlayerView::trigger_full_health_effect()
{
    player *p = halo::game::player_at(player_index);

    if (player_index != halo::k_dword_none && p->local_player_index != -1) {
        uint8_t buffer[0x38];
        int32_t i;
        for (i = 0; i < 0x38; i++) {
            buffer[i] = 0;
        }

        *(uint16_t *)(buffer + 0x00) = 6;
        *(uint16_t *)(buffer + 0x02) = 2;
        *(uint32_t *)(buffer + 0x10) = 0x40000000;
        *(uint16_t *)(buffer + 0x14) = 1;
        *(uint32_t *)(buffer + 0x20) = 0x3f000000;
        *(uint32_t *)(buffer + 0x24) = 0;
        *(uint32_t *)(buffer + 0x28) = 0x3f800000;
        *(uint32_t *)(buffer + 0x2c) = 0x3f6aeaea;
        *(uint32_t *)(buffer + 0x30) = 0x3f6aeaea;
        *(uint32_t *)(buffer + 0x34) = 0x3f6aeaea;

        halo::effects::player_effect_set_screen_flash_for_player(player_index, (player_screen_flash *)buffer, 1.0f);
    }
}

/**
 * blam-cc: EDX -> player_index
 * For a local player only, builds a zeroed 0x38-byte effect-parameter buffer, fills it from six
 * module globals, and fires it via player_effect_set_screen_flash_for_player with a 1.0f scale.
 *
 * @address 0x479710
 */
void PlayerView::trigger_shield_recharge_effect()
{
    player *p = halo::game::player_at(player_index);

    if (player_index != halo::k_dword_none && p->local_player_index != -1) {
        uint8_t buffer[0x38];
        int32_t i;
        for (i = 0; i < 0x38; i++) {
            buffer[i] = 0;
        }

        *(uint16_t *)(buffer + 0x00) = global_006889d0;
        *(uint16_t *)(buffer + 0x02) = 2;
        *(uint32_t *)(buffer + 0x10) = global_006889e0;
        *(uint16_t *)(buffer + 0x14) = global_007102e4;
        *(uint32_t *)(buffer + 0x20) = global_006889d4;
        *(uint32_t *)(buffer + 0x24) = 0;
        *(uint32_t *)(buffer + 0x28) = global_007102e8;
        *(uint32_t *)(buffer + 0x2c) = global_006889d8;
        *(uint32_t *)(buffer + 0x30) = global_007102ec;
        *(uint32_t *)(buffer + 0x34) = global_006889dc;

        halo::effects::player_effect_set_screen_flash_for_player(player_index, (player_screen_flash *)buffer, 1.0f);
    }
}

/**
 * Implements the original `player_swap_to_weapon`.
 *
 * @address 0x479240
 */
uint8_t PlayerView::swap_to_weapon(datum_index target_weapon)
{
    player *p = halo::game::player_at(player_index);
    uint8_t *record = (uint8_t *)p;
    datum_index unit_index = ((struct player *)record)->unit;
    datum_index interaction_object = ((struct player *)record)->interaction_object;
    uint8_t *unit = OBJECT_DATA(unit_index);

    switch (((struct player *)record)->interaction_type) {
    case 6: {
        uint8_t *current = OBJECT_DATA(unit_index);
        int16_t slot = *(int16_t *)(current + 0x2f2);
        datum_index current_weapon = (slot != -1) ? *(datum_index *)(current + 0x2f8 + slot * 4) : k_datum_index_none;

        if (current_weapon != target_weapon) {
            int32_t i;

            for (i = 0; i < 4; i++) {
                if (*(datum_index *)(unit + 0x2f8 + i * 4) == target_weapon) {
                    ((unit_object *)unit)->unit.desired_weapon_index = (int16_t)i;
                    halo::units::unit_ready_desired_weapon(unit_index, 1);
                    break;
                }
            }
        }
        if (halo::units::unit_drop_current_weapon(unit_index, 1) &&
            halo::units::unit_pickup_weapon(1, interaction_object, unit_index)) {
            halo::interface::hud_add_item_message(((struct player *)record)->local_player_index,
                (int32_t)*(datum_index *)OBJECT_DATA(interaction_object), 0, 0);
            LocalPlayerUnit(unit_index).invalidate_local_player_zoom_level();
        }
        return 1;
    }
    case 7:
        if (halo::units::unit_pickup_weapon(1, interaction_object, unit_index)) {
            halo::interface::hud_add_item_message(((struct player *)record)->local_player_index,
                (int32_t)*(datum_index *)OBJECT_DATA(interaction_object), 0, 0);
        }
        return 0;
    default:
        return 0;
    }
}

/**
 * blam-cc: EDI -> player_index
 * Finds up to 16 objects near `player_index`'s unit's bounding sphere (only when that unit has
 * no parent) and, for each, dispatches on its object_type: vehicle (1) to
 * player_check_vehicle_interaction, weapon/equipment (2 or 3) to player_check_vehicle_boarding_interaction, and type 8 to
 * player_check_assassination_opportunity.
 *
 * @address 0x478400
 */
void PlayerView::update_nearby_interactions_primary()
{
    player *p = halo::game::player_at(player_index);
    datum_index unit_index = p->unit;

    if (unit_index == (datum_index)halo::k_dword_none) {
        return;
    }

    {
        object *unit = (object *)((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_datum_slot_mask].data;

        if (unit->parent_object == (datum_index)halo::k_dword_none) {
            datum_index candidates[16];
            int16_t count = halo::objects::object_find_in_sphere(0, 0x11f, &unit->location_leaf_index,
                &unit->bounding_center, unit->bounding_radius, candidates, 0x10);
            int16_t i;

            for (i = 0; i < count; i++) {
                object *candidate = (object *)((object_header *)halo::objects::globals().object_data->data)[candidates[i] & halo::k_datum_slot_mask].data;
                switch (candidate->type) {
                case 1:
                    PlayerView(player_index).check_vehicle_interaction(candidates[i]);
                    break;
                case 2:
                case 3:
                    PlayerView(player_index).check_vehicle_boarding_interaction(candidates[i]);
                    break;
                case 8:
                    PlayerView(player_index).check_assassination_opportunity(candidates[i]);
                    break;
                }
            }
        }
    }
}

/**
 * blam-cc: EDI -> player_index
 * As player_update_nearby_interactions_primary, but routes object_type 2/3 candidates to the
 * lightweight player_check_vehicle_boarding_interaction_lightweight instead of player_check_vehicle_boarding_interaction.
 *
 * @address 0x478500
 */
void PlayerView::update_nearby_interactions_secondary()
{
    player *p = halo::game::player_at(player_index);
    datum_index unit_index = p->unit;

    if (unit_index == (datum_index)halo::k_dword_none) {
        return;
    }

    {
        object *unit = (object *)((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_datum_slot_mask].data;

        if (unit->parent_object == (datum_index)halo::k_dword_none) {
            datum_index candidates[16];
            int16_t count = halo::objects::object_find_in_sphere(0, 0x11f, &unit->location_leaf_index,
                &unit->bounding_center, unit->bounding_radius, candidates, 0x10);
            int16_t i;

            for (i = 0; i < count; i++) {
                object *candidate = (object *)((object_header *)halo::objects::globals().object_data->data)[candidates[i] & halo::k_datum_slot_mask].data;
                switch (candidate->type) {
                case 1:
                    PlayerView(player_index).check_vehicle_interaction(candidates[i]);
                    break;
                case 2:
                case 3:
                    PlayerView(player_index).check_vehicle_boarding_interaction_lightweight(candidates[i]);
                    break;
                case 8:
                    PlayerView(player_index).check_assassination_opportunity(candidates[i]);
                    break;
                }
            }
        }
    }
}

/**
 * Validates player_handle against player_data, fetches its unit (_object_mask_unit) and, if it
 * has a parent object, forwards to unit_seat_flag_bit2 with that parent and the unit's
 * vehicle_seat_index. Returns false for an invalid handle, a player with no unit, or a unit with
 * no parent.
 *
 * @address 0x477210
 */
uint8_t PlayerView::unit_has_parent()
{
    datum_index player_handle = player_index;
    int16_t index;
    player *plr;
    object *unit_obj;

    if (player_handle == (datum_index)-1) {
        return 0;
    }
    index = (int16_t)player_handle;
    if (index < 0 || index >= player_data->maximum_count) {
        return 0;
    }
    plr = halo::game::player_at(index);
    {
        int16_t salt = (int16_t)(player_handle >> 16);
        if (plr->identifier == 0 || (salt != 0 && plr->identifier != salt)) {
            return 0;
        }
    }

    unit_obj = halo::objects::object_try_and_get(plr->unit, _object_mask_unit);
    if (unit_obj == (object *)0 || unit_obj->parent_object == (datum_index)-1) {
        return 0;
    }
    return halo::units::unit_seat_flag_bit2(unit_obj->parent_object, *(int16_t *)((uint8_t *)unit_obj + 0x2f0));
}

/**
 * blam-cc: EAX -> player_index, EBX -> candidate_object, stack -> priority_type, seat
 * Updates the player's pending interaction slot to `candidate_object` (with `priority_type`,
 * `seat`) unless: priority_type is 0xb (a request to clear, always accepted) is not the case
 * and there is already a higher-priority pending interaction, or, when priority_type ties the
 * existing one, the existing candidate is no farther from the player's own unit than the new
 * one is.
 *
 * @address 0x478e00
 */
void PlayerView::set_pending_interaction_action(int16_t priority_type, int16_t seat, uint32_t candidate_object)
{
    player *p = halo::game::player_at(player_index);

    if (priority_type != 0xb) {
        if (priority_type == p->interaction_type) {
            object *unit = (object *)((object_header *)halo::objects::globals().object_data->data)[p->unit & halo::k_datum_slot_mask].data;
            object *existing = (object *)((object_header *)halo::objects::globals().object_data->data)[p->interaction_object & halo::k_datum_slot_mask].data;
            object *candidate = (object *)((object_header *)halo::objects::globals().object_data->data)[candidate_object & halo::k_datum_slot_mask].data;

            float ex = existing->position.x - unit->position.x;
            float ey = existing->position.y - unit->position.y;
            float ez = existing->position.z - unit->position.z;
            float cx = candidate->position.x - unit->position.x;
            float cy = candidate->position.y - unit->position.y;
            float cz = candidate->position.z - unit->position.z;

            if (sqrt(ey * ey + ex * ex + ez * ez) <= sqrt(cx * cx + cy * cy + cz * cz)) {
                return;
            }
        } else if (priority_type <= p->interaction_type) {
            return;
        }
    }

    p->interaction_type = priority_type;
    p->interaction_object = (datum_index)candidate_object;
    p->interaction_seat = seat;
}

/**
 * blam-cc: ESI -> candidate_object, EDI -> unit_or_player_index
 * Returns 1 (busy) if the candidate is a weapon whose tag data has bit 0x10 set at +0x308, or if
 * unit_count_deployed_weapons reports a nonzero value AND (in single player, unit_lacks_weapon_type_of approves and that
 * value is under 2, OR game_engine_ctf_unit_weapon_must_be_readied rejects the candidate). Otherwise returns 0.
 *
 * @address 0x478820
 */
uint8_t PlayerView::is_busy_with_interaction(uint32_t candidate_object)
{
    uint32_t unit_or_player_index = player_index;
    int16_t value = halo::units::unit_count_deployed_weapons(unit_or_player_index);

    if (halo::units::unit_lacks_weapon_type_of(candidate_object, unit_or_player_index)) {
        object *tag_data = halo::objects::object_try_and_get(candidate_object, 4);

        datum_index definition_tag = *(datum_index *)tag_data;
        uint8_t *weapon_tag_data = (uint8_t *)halo::cache::globals().tag_instances[definition_tag & halo::k_datum_slot_mask].data;
        if ((weapon_tag_data[0x308] & 0x10) != 0) {
            return 1;
        }
    }

    if (value != 0) {
        if (current_game_engine == 0) {
            if (halo::units::unit_lacks_weapon_type_of(candidate_object, unit_or_player_index) != 0 && value < 2) {
                return 1;
            }
        }
        if (halo::game::game_engine_ctf_unit_weapon_must_be_readied(candidate_object) == 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * blam-cc: ECX -> player_handle
 * Returns whether the weapon the given player's unit currently has readied carries the
 * "does not depower active camouflage in multiplayer" flag. False whenever the player has no
 * unit, the unit has no readied weapon, or the weapon object/tag cannot be resolved.
 *
 * @address 0x466390
 */
uint8_t PlayerView::current_weapon_prevents_camo_depower()
{
    datum_index player_handle = player_index;
    player *p;
    object *unit_obj;
    unit_data *unit;
    int16_t weapon_slot;
    datum_index weapon_handle;
    object *weapon_obj;
    Weapon *weapon_tag;

    if (player_handle == (datum_index)halo::k_dword_none) {
        return 0;
    }

    p = halo::game::player_at(player_handle);
    if (p->unit == (datum_index)halo::k_dword_none) {
        return 0;
    }

    unit_obj = ((object_header *)halo::objects::globals().object_data->data)[p->unit & halo::k_datum_slot_mask].data;
    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    weapon_slot = unit->current_weapon_index;
    if (weapon_slot == -1) {
        return 0;
    }

    weapon_handle = unit->weapons[weapon_slot];
    if (weapon_handle == (datum_index)halo::k_dword_none) {
        return 0;
    }

    weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[weapon_handle & halo::k_datum_slot_mask].data;
    if (weapon_obj->definition_tag == (datum_index)halo::k_dword_none) {
        return 0;
    }

    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[weapon_obj->definition_tag & halo::k_datum_slot_mask].data;
    return (uint8_t)((weapon_tag->weapon_flags >> 13) & 1);
}

/**
 * blam-cc: ECX -> player_index
 *
 * @address 0x463300
 */
uint8_t PlayerView::has_must_be_readied_weapon()
{
    player *p;
    unit_data *unit;
    int32_t i;

    if (player_index == halo::k_dword_none) {
        return 0;
    }
    p = halo::game::player_at(player_index);
    if (p->unit == (datum_index)halo::k_dword_none) {
        return 0;
    }

    unit = (unit_data *)((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[p->unit & halo::k_datum_slot_mask].data);

    for (i = 0; i < 4; i++) {
        datum_index weapon = *(datum_index *)((uint8_t *)unit + 0x2f8 + i * 4);
        if (weapon != (datum_index)halo::k_dword_none) {
            object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[weapon & halo::k_datum_slot_mask].data;
            uint8_t *weapon_tag_data = (uint8_t *)halo::cache::globals().tag_instances[weapon_obj->definition_tag & halo::k_datum_slot_mask].data;
            if (((*(uint32_t *)(weapon_tag_data + 0x308) >> 3) & 1) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * blam-cc: EAX -> player_index
 *
 * @address 0x4633a0
 */
void PlayerView::reset_gauge_if_flagged()
{
    player *p;
    unit_data *unit;

    if (player_index == halo::k_dword_none) {
        return;
    }
    p = halo::game::player_at(player_index);
    if (p->unit == (datum_index)halo::k_dword_none) {
        return;
    }

    unit = (unit_data *)((uint8_t *)
        ((object_header *)halo::objects::globals().object_data->data)[p->unit & halo::k_datum_slot_mask].data + k_unit_data_offset);
    if ((unit->flags & _unit_flag_unknown_10) != 0) {
        unit->active_camouflage_power = 0.5f;
    }
}

/**
 * blam-cc: EBX -> player_handle
 * Counts a unit's active-camouflage timer (unit_data+0x37c) down toward a 0.05 floor once per
 * tick, at a rate that is 0.0 while the readied weapon prevents camo depower, the weapon tag's
 * own active_camo_ding when it supplies a nonzero one, or 0.1 by default. Does nothing while no
 * multiplayer engine is running, the player has no unit, or the timer is already at the floor.
 *
 * @address 0x466420
 */
void PlayerView::update_active_camouflage_depower()
{
    datum_index player_handle = player_index;
    player *p;
    datum_index unit_handle;
    object *unit_obj;
    unit_data *unit;
    int16_t weapon_slot;
    datum_index weapon_handle;
    uint8_t prevents_depower;
    float rate;

    if (current_game_engine == 0 || player_handle == (datum_index)halo::k_dword_none) {
        return;
    }

    p = halo::game::player_at(player_handle);
    unit_handle = p->unit;
    if (unit_handle == (datum_index)halo::k_dword_none) {
        return;
    }

    unit_obj = ((object_header *)halo::objects::globals().object_data->data)[unit_handle & halo::k_datum_slot_mask].data;
    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    weapon_slot = unit->current_weapon_index;

    weapon_handle = halo::units::unit_get_weapon_object_index(unit_handle, weapon_slot);
    prevents_depower = PlayerView(player_handle).current_weapon_prevents_camo_depower();

    rate = 0.1f;
    if (prevents_depower) {
        rate = 0.0f;
    } else if (weapon_handle != (datum_index)halo::k_dword_none) {
        object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[weapon_handle & halo::k_datum_slot_mask].data;
        Weapon *weapon_tag = (Weapon *)halo::cache::globals().tag_instances[weapon_obj->definition_tag & halo::k_datum_slot_mask].data;
        if (weapon_tag->active_camo_ding != 0.0f) {
            rate = weapon_tag->active_camo_ding;
        }
    }

    if (unit->active_camouflage_power >= 0.05f) {
        unit->active_camouflage_power = unit->active_camouflage_power - rate;
        unit->active_camouflage_regrowth = 1;
        if (unit->active_camouflage_power < 0.05f) {
            unit->active_camouflage_power = 0.05f;
        }
    }
}

/**
 * blam-cc: EBX -> player_handle, stack -> slot, amount
 * Validates `player_handle` (index in range, slot identifier nonzero and matching the handle's
 * own salt, if any) and `slot` (0 or 1; slot 0 additionally requires the player to have a live
 * unit without unit_flags bit 0x10 set). On success, fires the streak-start/streak-continue
 * notifications the first time this streak is touched, adds `amount` to
 * player::kill_streak[slot], and (dedicated server only, and only for the player's own
 * controlling unit) forwards the update via player_notify_kill_streak_update. Returns 1 on success, 0 if any check
 * failed.
 *
 * @address 0x479ba0
 */
uint8_t KillStreak::add_kill_streak(int32_t slot, int16_t amount)
{
    int16_t index;
    player *p;

    if (player_handle == halo::k_dword_none) {
        return 0;
    }
    index = (int16_t)player_handle;
    if (index < 0 || player_data->maximum_count <= index) {
        return 0;
    }

    p = halo::game::player_at(index);
    if (p->identifier == 0) {
        return 0;
    }
    {
        int16_t salt = (int16_t)(player_handle >> 0x10);
        if (salt != 0 && p->identifier != salt) {
            return 0;
        }
    }

    if (slot < 0 || 1 < slot) {
        return 0;
    }
    if (slot == 0) {
        object *unit = halo::objects::object_try_and_get(p->unit, 3);
        if (unit == 0 || (((unit_data *)((uint8_t *)unit + k_unit_data_offset))->flags & _unit_flag_unknown_10) != 0) {
            return 0;
        }
    }

    {
        int16_t *streak = &p->kill_streak[slot];
        if (*streak == 0) {
            halo::game::player_kill_streak_begin(slot, player_handle);
        } else if (current_game_engine == 0) {
            halo::game::player_kill_streak_continue(slot, player_handle);
        }
        *streak = *streak + amount;
    }

    if (halo::networking::globals().game_mode == 2) {
        object *owner_unit = (object *)((object_header *)halo::objects::globals().object_data->data)[p->unit & halo::k_datum_slot_mask].data;
        if (owner_unit->network_role == 0) {
            KillStreak(player_handle).notify_kill_streak_update(slot, amount);
        }
    }
    return 1;
}

/**
 * blam-cc: ECX -> player_handle
 * Validates `player_handle` exactly like player_add_kill_streak.c. If the player's
 * medal_streak_timer hasn't expired (still >= 0), increments medal_streak_count; if that count
 * reaches the configured threshold, fires the medal event (network_session_autoban_player) and returns without
 * resetting the timer. Otherwise resets medal_streak_timer to -sv_tk_grace_ticks,
 * falling back to sv_tk_cooldown_ticks if that negation is exactly zero.
 *
 * @address 0x479eb0
 */
void KillStreak::advance_multikill_medal()
{
    int16_t index;
    player *p;

    if (player_handle == halo::k_dword_none) {
        return;
    }
    index = (int16_t)player_handle;
    if (index < 0 || player_data->maximum_count <= index) {
        return;
    }

    p = halo::game::player_at(index);
    if (p->identifier == 0) {
        return;
    }
    {
        int16_t salt = (int16_t)(player_handle >> 0x10);
        if (salt != 0 && p->identifier != salt) {
            return;
        }
    }

    if (p->medal_streak_timer < 0) {
        return;
    }

    p->medal_streak_count = p->medal_streak_count + 1;
    if (multikill_medal_threshold != 0 && multikill_medal_threshold <= p->medal_streak_count) {
        halo::networking::network_session_autoban_player(player_handle);
        return;
    }

    p->medal_streak_timer = -sv_tk_grace_ticks;
    if (p->medal_streak_timer == 0) {
        p->medal_streak_timer = sv_tk_cooldown_ticks;
    }
}

/**
 * blam-cc: EAX -> player_index, ESI -> value, stack -> slot
 * The first time slot 0's streak is touched (it reads 0), marks the player's unit with
 * unit_flags bit 0x10 and stamps `slot` into unit::unknown_422. Either way, raises
 * player::kill_streak[slot] to `value` if `value` is larger.
 *
 * @address 0x479ca0
 */
void KillStreak::set_max(int16_t slot, int16_t value)
{
    uint32_t player_index = player_handle;
    player *p = halo::game::player_at(player_index);
    int16_t *streak = &p->kill_streak[slot];

    if (*streak == 0 && slot == 0) {
        unit_data *unit = (unit_data *)((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[p->unit & halo::k_datum_slot_mask].data + k_unit_data_offset);
        unit->flags = unit->flags | _unit_flag_unknown_10;
        unit->active_camouflage_regrowth = slot;
    }

    if (*streak <= value) {
        *streak = value;
    }
}

/**
 * blam-cc: EAX -> player_index
 * Decrements both of the player's kill_streak entries (floor 0), and when slot 0's countdown
 * reaches exactly zero, clears unit_flags bit 0x10 on the player's unit.
 *
 * @address 0x479d10
 */
void KillStreak::tick()
{
    uint32_t player_index = player_handle;
    player *p = halo::game::player_at(player_index);
    int32_t slot;

    for (slot = 0; slot < 2; slot++) {
        if (0 < p->kill_streak[slot]) {
            p->kill_streak[slot] = p->kill_streak[slot] - 1;
            if (p->kill_streak[slot] == 0 && slot == 0) {
                unit_data *unit = (unit_data *)((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[p->unit & halo::k_datum_slot_mask].data + k_unit_data_offset);
                unit->flags = unit->flags & ~_unit_flag_unknown_10;
            }
        }
    }
}

/**
 * blam-cc: ECX -> player_handle, stack -> slot, amount
 * Broadcasts a networked event 0xe carrying a hash of `player_handle` (0 if -1 or unmapped),
 * `slot` and `amount` (each truncated to 16 bits).
 *
 * @address 0x479aa0
 */
void KillStreak::notify_kill_streak_update(int32_t slot, int16_t amount)
{
    struct { int32_t hash_result; int16_t slot; int16_t amount; } fields;
    void *fields_ptr;
    int32_t encoded_bits;

    fields.hash_result = 0;
    if (player_handle != halo::k_dword_none) {
        fields.hash_result = halo::objects::hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)player_handle);
        if (fields.hash_result == -1) {
            fields.hash_result = 0;
        }
    }
    fields.slot = (int16_t)slot;
    fields.amount = amount;
    fields_ptr = &fields;

    encoded_bits = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0xe, 0, &fields_ptr, 0, 1, 0);
    if (0 < encoded_bits) {
        halo::networking::network_session_broadcast_to_flagged(encoded_bits, halo::networking::globals().server, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
    }
}

/**
 * blam-cc: EDX -> player_index
 *
 * @address 0x4797d0
 */
void KillStreak::trigger_kill_streak_effect()
{
    uint32_t player_index = player_handle;
    player *p = halo::game::player_at(player_index);

    if (player_index != halo::k_dword_none && p->local_player_index != -1) {
        uint8_t buffer[0x38];
        int32_t i;
        for (i = 0; i < 0x38; i++) {
            buffer[i] = 0;
        }

        *(uint16_t *)(buffer + 0x00) = global_006889e4;
        *(uint16_t *)(buffer + 0x02) = 2;
        *(uint32_t *)(buffer + 0x10) = global_006889f4;
        *(uint16_t *)(buffer + 0x14) = global_007102f0;
        *(uint32_t *)(buffer + 0x20) = global_006889e8;
        *(uint32_t *)(buffer + 0x24) = 0;
        *(uint32_t *)(buffer + 0x28) = global_007102f4;
        *(uint32_t *)(buffer + 0x2c) = global_006889ec;
        *(uint32_t *)(buffer + 0x30) = global_006889f0;
        *(uint32_t *)(buffer + 0x34) = global_007102f8;

        halo::effects::player_effect_set_screen_flash_for_player(player_index, (player_screen_flash *)buffer, 1.0f);
    }
}

/**
 * blam-cc: EAX -> unit
 * Returns the local_player_index of the player controlling `unit`, or -1 if it has no
 * controlling player. See the header note: despite the name, this does not return a weapon
 * index.
 *
 * @address 0x4726b0
 */
int32_t LocalPlayerUnit::get_local_player_weapon_index()
{
    unit_data *u = *(unit_data **)((uint8_t *)halo::objects::globals().object_data->data +
        (uint32_t)(uint16_t)unit * halo::objects::globals().object_data->size + 8);
    datum_index controlling_player = u->controlling_player;

    if (controlling_player != k_datum_index_none) {
        player *p = halo::game::player_at(controlling_player);
        return p->local_player_index;
    }
    return -1;
}

/**
 * blam-cc: EAX -> unit, stack -> weapon_index
 * If `unit` is controlled by a local player, sets that local player's desired_weapon_index.
 *
 * @address 0x472100
 */
void LocalPlayerUnit::set_local_player_weapon_index(int16_t weapon_index)
{
    unit_data *u = *(unit_data **)((uint8_t *)halo::objects::globals().object_data->data +
        (uint32_t)(uint16_t)unit * halo::objects::globals().object_data->size + 8);
    datum_index controlling_player = u->controlling_player;

    if (controlling_player != k_datum_index_none) {
        player *p = halo::game::player_at(controlling_player);

        if (p->local_player_index != -1) {
            player_control_globals_ptr->local_players[p->local_player_index].desired_weapon_index = weapon_index;
        }
    }
}

/**
 * blam-cc: EAX -> unit
 * If `unit` is controlled by a local player, invalidates that local player's cached
 * desired_zoom_level (sets it to -1).
 *
 * @address 0x4726f0
 */
void LocalPlayerUnit::invalidate_local_player_zoom_level()
{
    unit_data *u = *(unit_data **)((uint8_t *)halo::objects::globals().object_data->data +
        (uint32_t)(uint16_t)unit * halo::objects::globals().object_data->size + 8);
    datum_index controlling_player = u->controlling_player;

    if (controlling_player != k_datum_index_none) {
        player *p = halo::game::player_at(controlling_player);

        if (p->local_player_index != -1) {
            player_control_globals_ptr->local_players[p->local_player_index].desired_zoom_level = -1;
        }
    }
}

/**
 * Fills `out[0..3]` with the current weapon's autoaim/magnetism angle and range bounds
 * (aspect-corrected, forming an observer_target_cone), plus `out[4]` = the larger of the
 * weapon's deviation_angle and autoaim_angle. Fails if the unit has no ready weapon, or if the
 * weapon only assists while zoomed and `require_zoomed` says it is not.
 *
 * @address 0x459e80
 */
uint8_t LocalPlayerUnit::get_current_weapon_autoaim_cone(int16_t require_zoomed, real *out)
{
    datum_index unit_index = unit;
    object *unit_obj;
    unit_data *unit;
    datum_index weapon_index;
    object *weapon_obj;
    Weapon *weapon;
    real aspect;
    real inv_aspect;
    real deviation_or_autoaim;

    if (unit_index == k_datum_index_none) {
        return 0;
    }
    unit_obj = ((object_header *)halo::objects::globals().object_data->data)[unit_index & halo::k_datum_slot_mask].data;
    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    if (unit->current_weapon_index == -1) {
        return 0;
    }
    weapon_index = unit->weapons[unit->current_weapon_index];
    if (weapon_index == k_datum_index_none) {
        return 0;
    }
    weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[weapon_index & halo::k_datum_slot_mask].data;
    weapon = (Weapon *)halo::cache::globals().tag_instances[weapon_obj->definition_tag & halo::k_datum_slot_mask].data;
    if (require_zoomed == -1 && (weapon->weapon_flags & 0x20) != 0) {
        return 0;
    }

    aspect = halo::items::weapon_get_zoom_magnification(weapon_index, require_zoomed);
    inv_aspect = 1.0f / aspect;
    out[0] = inv_aspect * weapon->autoaim_angle;
    out[1] = aspect * weapon->autoaim_range;
    out[2] = inv_aspect * weapon->magnetism_angle;
    out[3] = aspect * weapon->magnetism_range;
    deviation_or_autoaim = (weapon->deviation_angle > weapon->autoaim_angle) ? weapon->deviation_angle : weapon->autoaim_angle;
    out[4] = deviation_or_autoaim * inv_aspect;
    return 1;
}

/**
 * If `obj`'s cached position differs from new_position by more than 1.1 world units, snaps its
 * velocity to zero and marks it at-rest (_object_at_rest_bit) so downstream physics doesn't try
 * to interpolate the jump. Then always calls object_set_position_and_recalculate.
 *
 * @address 0x4772e0
 */
void ObjectView::snap_position_if_far(real_point3d *new_position)
{
    float dx = new_position->x - obj->position.x;
    float dy = new_position->y - obj->position.y;
    float dz = new_position->z - obj->position.z;

    if (sqrt(dy * dy + dz * dz + dx * dx) > 1.1) {
        obj->velocity = reinterpret_cast<real_vector3d &>(halo::math::globals().global_origin3d);
        obj->flags = obj->flags | _object_at_rest_bit;
    }
    halo::objects::object_set_position_and_recalculate((real_point3d *)obj, 0);
}

/**
 * CORRECTED (phase 4 review, against objdump): Ghidra shows this as void; EAX carries the new
 * player's datum_index (or k_datum_index_none) on every return path, matching both callers.
 *
 * Creates a player datum (at a specific index+salt if requested_handle is not -1, otherwise the
 * next free one via datum_new) and initializes the same reduced field set player_new_network
 * does, plus -- only when local_player_index is -1, i.e. this player will be driven by updates
 * from elsewhere rather than this machine's own input -- builds the three embedded network
 * update queues (player_update_queue_create at +0x120, position_update_queue_create at +0x170,
 * vehicle_update_queue_create at +0x1d0) and zeroes every field between and around them. Copies
 * the caller's identifier record's name (defaulting to the empty string) and, on success, the
 * whole 32-byte identifier record a second time into +0x48. Finally registers the result (even
 * on failure) as machine_index's entry in machine_to_player, unless that slot is already
 * claimed.
 *
 * @address 0x473940
 */
datum_index Players::new_local(datum_index requested_handle, uint32_t machine_index, int16_t local_player_index, uint16_t *identifier_record)
{
    datum_index result;
    player *p;
    wchar_t *name_source;

    if (requested_handle == (datum_index)-1) {
        result = halo::memory::datum_new(player_data);
    } else {
        result = halo::memory::datum_new_at_index_with_salt(requested_handle, player_data);
    }

    if (result != (datum_index)-1) {
        p = halo::game::player_at(result);

        name_source = &empty_string;
        if (identifier_record != (uint16_t *)0) {
            name_source = (wchar_t *)identifier_record;
        }
        wcsncpy((wchar_t *)p->name, name_source, 11);
        p->name[11] = 0;

        p->ping = 0;
        p->medal_streak_count = 0;
        p->medal_streak_timer = 0;
        p->local_player_index = local_player_index;
        p->unit = (datum_index)-1;
        p->previous_unit = (datum_index)-1;
        p->unknown_1c = -1;
        p->bsp_cluster = -1;
        p->observer_target = (datum_index)-1;
        p->speed = 1.0f;
        p->team = 1;

        p->interaction_type = 0;
        p->interaction_object = (datum_index)-1;

        p->quit_tick = (datum_index)-1;
        p->marked_for_deletion = 0;
        p->baseline_update_id = (datum_index)-1;
        p->last_update_id = -1;

        if (local_player_index == -1) {
            p->last_remote_update_id = -1;
            p->last_position_update_id = (datum_index)-1;
            halo::game::player_update_queue_create(&p->update_history);

            memset((uint8_t *)p + 0xf0, 0, 0x30);

            p->position_baseline_x = 0;
            p->position_baseline_y = 0;
            p->position_baseline_z = 0;
            halo::game::position_update_queue_create(&p->position_updates);

            p->position_update_ignored_count = 0;
            p->last_vehicle_update_id = (datum_index)-1;

            memset((uint8_t *)p + 0x190, 0, 0x40);
            halo::game::vehicle_update_queue_create(&p->vehicle_updates);

            p->vehicle_update_ignored_count = 0;
            p->position_updates_applied_count = 0;
            p->position_update_error_total = 0;
            p->vehicle_updates_applied_count = 0;
            p->vehicle_update_error_total = 0;
        }

        if (identifier_record != (uint16_t *)0) {
            memcpy(&p->identifier_name, identifier_record, 32);
        }
    }

    if (machine_to_player[machine_index & halo::k_datum_slot_mask] == (datum_index)-1) {
        machine_to_player[machine_index & halo::k_datum_slot_mask] = result;
    }
    return result;
}

/**
 * CORRECTED (phase 4 review, against objdump): Ghidra shows this as void; it actually returns
 * the new player's datum_index (or k_datum_index_none on failure) in EAX.
 *
 * Creates a player datum (at a specific index if requested_index is not -1, otherwise the next
 * free one), gives it the reduced field set below (team defaults to 1, every handle field is
 * seeded to the wildcard, speed to 1.0), copies the caller's identifier record's name into the
 * player name (defaulting to the empty string) and, only on success, block-copies the whole
 * 32-byte identifier record a second time into player+0x48 (identifier_name plus its
 * unresolved tail through team_index_desired). Finally registers the result (even on failure,
 * where that just re-writes the wildcard) as machine_index's entry in machine_to_player, unless
 * that slot is already claimed.
 *
 * @address 0x473780
 */
datum_index Players::new_network(datum_index requested_index, uint32_t machine_index, int16_t local_player_index, uint16_t *identifier_record)
{
    datum_index result;
    player *p;
    wchar_t *name_source;

    if (requested_index == (datum_index)-1) {
        result = halo::memory::datum_new(player_data);
    } else {
        result = halo::memory::datum_new_at_index((int16_t)requested_index, player_data);
    }

    if (result != (datum_index)-1) {
        p = halo::game::player_at(result);

        name_source = &empty_string;
        if (identifier_record != (uint16_t *)0) {
            name_source = (wchar_t *)identifier_record;
        }
        wcsncpy((wchar_t *)p->name, name_source, 11);
        p->name[11] = 0;

        p->local_player_index = local_player_index;
        p->ping = 0;
        p->medal_streak_count = 0;
        p->medal_streak_timer = 0;
        p->unit = (datum_index)-1;
        p->previous_unit = (datum_index)-1;
        p->unknown_1c = -1;
        p->bsp_cluster = -1;
        p->observer_target = (datum_index)-1;
        p->speed = 1.0f;
        p->team = 1;

        p->interaction_type = 0;
        p->interaction_object = (datum_index)-1;

        p->quit_tick = (datum_index)-1;
        p->marked_for_deletion = 0;
        p->odd_man_out = 0;
        p->last_update_id = 0;
        p->baseline_update_id = (datum_index)-1;
        p->unknown_f0 = 0;
        p->unknown_f4 = (datum_index)-1;
        p->unknown_104 = -1;
        p->connection_quality_started = 0;
        p->loss_window_start_ms = 0;
        p->loss_window_units = 0;
        p->latency_last_sample_ms = 0;
        p->latency_bad_sample_count = 0;
        p->unknown_11c = 0;

        p->update_history.queue.capacity = -1;
        p->update_history.queue.record_size = -1;
        p->update_history.queue.records = (void **)(intptr_t)-1;
        p->update_history.queue.write_index = -1;

        memset((uint8_t *)p + 0x130, 0, 0x30);

        p->last_position_update_id = 0;
        p->position_baseline_x = -1;
        p->position_baseline_y = -1;
        p->position_baseline_z = -1;

        p->position_updates.capacity = 0;
        p->position_updates.record_size = 0;
        p->position_updates.records = (void **)0;
        p->position_updates.write_index = -1;
        p->position_updates.read_index = -1;
        p->position_updates.storage = (void *)(intptr_t)-1;

        memset((uint8_t *)p + 0x188, 0, 0x40);

        if (identifier_record != (uint16_t *)0) {
            memcpy(&p->identifier_name, identifier_record, 32);
        }
    }

    if (machine_to_player[machine_index & halo::k_datum_slot_mask] == (datum_index)-1) {
        machine_to_player[machine_index & halo::k_datum_slot_mask] = result;
    }
    return result;
}

/**
 * Notifies the active game engine that this player's object is going away, then -- only while
 * this machine is a network client (mode 1) -- tears down the player's incoming update-history
 * storage if it is still a non-local (remotely represented) player, or -- while hosting
 * (mode 2) -- runs the host-side per-player network cleanup. In either networked mode, clears
 * this machine's machine_to_player slot if it still points at this player. Always deletes the
 * player datum last.
 * The "goto the same tail either way" shape of the original (see #if 0) is folded here into an
 * `update_machine_slot` flag; every input reaches the identical final datum_delete call.
 *
 * @address 0x473ae0
 */
void Players::delete_player(uint32_t machine_index, datum_index player_handle)
{
    player *p;
    int16_t index;
    int16_t requested_salt;
    uint8_t update_machine_slot;

    halo::game::game_engine_player_changed_object(player_handle);

    update_machine_slot = 0;
    if (halo::networking::globals().game_mode == 1) {
        update_machine_slot = 1;
        if (player_handle != (datum_index)-1) {
            index = (int16_t)player_handle;
            if (index >= 0 && index < player_data->maximum_count) {
                p = halo::game::player_at(index);
                requested_salt = (int16_t)(player_handle >> 16);
                if (p->identifier != 0 &&
                    (requested_salt == 0 || p->identifier == requested_salt) &&
                    p->local_player_index == -1) {
                    GlobalFree(p->update_history.queue.storage);
                    p->update_history.queue.storage = (void *)0;
                    halo::game::network_queue_destroy(&p->position_updates);
                }
            }
        }
    } else if (halo::networking::globals().game_mode == 2) {
        update_machine_slot = 1;

        halo::networking::network_machine_clear_flag_by_id((network_server_globals *)halo::networking::globals().server, (int32_t)machine_index);
    }

    if (update_machine_slot && machine_to_player[machine_index & halo::k_datum_slot_mask] == player_handle) {
        machine_to_player[machine_index & halo::k_datum_slot_mask] = (datum_index)-1;
    }
    halo::memory::datum_delete(player_data, player_handle);
}

/**
 * Deletes this player's slot out of update_client_queues, and -- only while hosting -- also
 * frees update_server_queues's matching slot's queue storage and deletes that slot too (both
 * queue arrays are indexed the same way as player_data, so the player's own handle doubles as
 * the datum handle into each). Passes the low byte of player::unknown_64 to player_delete as its
 * machine_index (see UNSURE above), then runs the two networking-side cleanups and clears the
 * player's player-profile-cache entry, decrementing the cache count.
 *
 * @address 0x473bb0
 */
void Players::remove_player(datum_index player_handle)
{
    player *p;
    int32_t index;
    int32_t profile_index;
    update_server_queue *server_entry;

    index = (int32_t)(uint16_t)player_handle;
    p = (player *)((uint8_t *)player_data->data + (uint32_t)index * player_data->size);

    halo::memory::datum_delete(update_client_queues, player_handle);

    if (halo::networking::globals().game_mode == 2) {
        server_entry = &((update_server_queue *)update_server_queues->data)[index];
        GlobalFree(server_entry->queue.queue.storage);
        server_entry->queue.queue.storage = (void *)0;
        halo::memory::datum_delete(update_server_queues, player_handle);
    }

    Players::delete_player((uint32_t)(int8_t)*((uint8_t *)p + 0x64), player_handle);

    halo::networking::network_index_cache_remove((uint8_t *)network_index_cache_table, player_handle);
    profile_index = halo::game::game_engine_player_profile_cache_find(player_handle);
    player_profile_cache[profile_index].in_use = 0;
    player_profile_cache_count = player_profile_cache_count - 1;
}

/**
 * Returns the index of the player whose unit is unit_index, or -1 when no player drives it.
 *
 * @address 0x474db0
 */
datum_index Players::index_from_unit_index(datum_index unit_index)
{
    data_iterator iter;
    player *p;
    datum_index result = (datum_index)-1;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    while ((p = (player *)halo::memory::data_iterator_next(&iter)) != 0) {
        if (p->unit == unit_index) {
            result = iter.index;
        }
    }
    return result;
}

/**
 * blam-cc: EBX -> new_team, ESI -> target_team_index_desired
 * Finds the first live player whose team_index_desired equals `target_team_index_desired` and
 * sets both its team and team_index to `new_team`.
 *
 * @address 0x470630
 */
void Players::set_team_by_color(uint8_t new_team, int8_t target_team_index_desired)
{
    data_iterator player_iter;
    void *player_element;

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = k_datum_index_none;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    player_element = halo::memory::data_iterator_next(&player_iter);
    while (player_element != 0) {
        player *p = (player *)player_element;

        if (p->team_index_desired == target_team_index_desired) {
            p->team = new_team;
            p->team_index = (int8_t)new_team;
            break;
        }
        player_element = halo::memory::data_iterator_next(&player_iter);
    }
}

/**
 * blam-cc: ESI -> weapon_dependency, stack -> role
 * Spawns the weapon tag referenced by `weapon_dependency` (a ScenarioPlayerStartingProfile
 * TagDependency) as a fresh object with the given datum role -- downgraded from 3 to 0 when
 * running as a dedicated server and the weapon's object type carries a set unknown_10 field --
 * then stamps the caller's starting rounds_loaded/rounds_reserved onto the new object's first
 * weapon magazine. Returns the new object's datum index, or -1 if the dependency has no tag.
 *
 * @address 0x477810
 */
datum_index Players::spawn_starting_profile_weapon(TagDependency *weapon_dependency, uint32_t role)
{
    object_placement_data placement;
    datum_index new_object;

    new_object = (datum_index)halo::k_dword_none;
    if (*(datum_index *)&weapon_dependency->tag_id != (datum_index)halo::k_dword_none) {
        halo::objects::object_placement_data_initialize(&placement, *(datum_index *)&weapon_dependency->tag_id, (datum_index)role);

        {
            uint32_t datum_role = 3;
            if (halo::networking::globals().game_mode == 2) {
                tag_instance *inst = &halo::cache::globals().tag_instances[weapon_dependency->tag_id.index];
                Object *tag_data = (Object *)inst->data;
                object_type_definition *def = object_type_definitions[tag_data->object_type];
                if (def->network_delta_message_type != -1) {
                    datum_role = 0;
                }
            }
            new_object = halo::objects::object_new_with_datum_role_control(&placement, datum_role);
        }

        if (new_object != (datum_index)halo::k_dword_none) {
            weapon_data *weapon = (weapon_data *)((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[new_object & halo::k_datum_slot_mask].data +
                k_item_extension_offset);
            weapon->magazines[0].rounds_unloaded = *(int16_t *)((uint8_t *)weapon_dependency + 0x12);
            weapon->magazines[0].rounds_loaded = *(int16_t *)((uint8_t *)weapon_dependency + 0x10);
        }
    }
    return new_object;
}

/**
 * Counts the live player datums.
 *
 * @address 0x45c6a0
 */
int32_t Players::active_count()
{
    data_iterator iterator;
    player *p;
    int32_t count;

    count = 0;
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)halo::memory::data_iterator_next(&iterator);
    while (p != (player *)0) {
        count = count + 1;
        p = (player *)halo::memory::data_iterator_next(&iterator);
    }
    return count;
}

/**
 * For every player with a unit: if that unit's root ancestor (walking parent_object) has object
 * flags bit 0x200000 set, returns true immediately. Otherwise, if the unit itself has no parent:
 * a biped defers to biped_is_idle_eligible (true if it says so); a vehicle falls into the shared tail
 * below testing its OWN airborne_ticks. If the unit does have a parent, and that parent is a
 * vehicle (_object_mask_vehicle) whose tag's Item::item_flags has bit 0x40 set, falls into the
 * same shared tail testing the PARENT's airborne_ticks: true if greater than 2. Returns false if
 * no player matches any of this.
 *
 * @address 0x475090
 */
uint8_t Players::any_pending_seat_or_respawn()
{
    data_iterator iter;
    player *plr;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    plr = (player *)halo::memory::data_iterator_next(&iter);
    while (plr != (player *)0) {
        if (plr->unit != (datum_index)-1) {
            object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[plr->unit & halo::k_datum_slot_mask].data;
            datum_index walk = plr->unit;
            datum_index root;
            object *root_obj;
            object *airborne_check_obj;

            do {
                root = walk;
                walk = ((object_header *)halo::objects::globals().object_data->data)[root & halo::k_datum_slot_mask].data->parent_object;
            } while (walk != (datum_index)-1);
            root_obj = ((object_header *)halo::objects::globals().object_data->data)[root & halo::k_datum_slot_mask].data;
            if ((root_obj->flags & 0x200000) != 0) {
                return 1;
            }

            airborne_check_obj = (object *)0;
            if (unit_obj->parent_object == (datum_index)-1) {
                if (unit_obj->type == _object_type_biped) {
                    if (halo::units::biped_is_idle_eligible(plr->unit) != 0) {
                        return 1;
                    }
                } else if (unit_obj->type == _object_type_vehicle) {
                    airborne_check_obj = unit_obj;
                }
            } else {
                object_header *parent_header = 0;
                int16_t parent_index = (int16_t)unit_obj->parent_object;
                if (parent_index >= 0 && parent_index < halo::objects::globals().object_data->maximum_count) {
                    object_header *candidate = &((object_header *)halo::objects::globals().object_data->data)[parent_index];
                    int16_t parent_salt = (int16_t)((uint32_t)unit_obj->parent_object >> 16);
                    if (candidate->identifier != 0 &&
                        (parent_salt == 0 || candidate->identifier == parent_salt)) {
                        parent_header = candidate;
                    }
                }
                if (parent_header != 0 && (1u << (parent_header->type & 0x1f) & _object_mask_vehicle) != 0 &&
                    parent_header->data != 0) {
                    Item *parent_tag = (Item *)halo::cache::globals().tag_instances[parent_header->data->definition_tag & halo::k_datum_slot_mask].data;
                    if ((parent_tag->item_flags & 0x40) != 0) {
                        airborne_check_obj = parent_header->data;
                    }
                }
            }

            if (airborne_check_obj != (object *)0 &&
                *((uint8_t *)airborne_check_obj + 0x4d0) > 2) {
                return 1;
            }
        }
        plr = (player *)halo::memory::data_iterator_next(&iter);
    }
    return 0;
}

/**
 * Returns true if any live player currently has the given local_player_index (only ever 0 or
 * -1 in this build, since k_maximum_local_players is 1).
 *
 * @address 0x4736d0
 */
uint8_t Players::any_with_local_player_index(int16_t local_player_index)
{
    data_iterator iter;
    player *p;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)halo::k_dword_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    p = (player *)halo::memory::data_iterator_next(&iter);
    while (p != (player *)0) {
        if (p->local_player_index == local_player_index) {
            return 1;
        }
        p = (player *)halo::memory::data_iterator_next(&iter);
    }
    return 0;
}

/**
 * Returns true if any live player currently has no controlled unit.
 *
 * @address 0x475210
 */
uint8_t Players::any_without_unit()
{
    data_iterator iter;
    player *plr;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    plr = (player *)halo::memory::data_iterator_next(&iter);
    while (plr != (player *)0) {
        if (plr->unit == (datum_index)-1) {
            return 1;
        }
        plr = (player *)halo::memory::data_iterator_next(&iter);
    }
    return 0;
}

/**
 * For every non-local player, drains its update_history circular queue: while the backlog
 * (queued record count, or the summed per-record tick weight) exceeds the catch-up thresholds,
 * pops the head record, replays it into the player's controlled unit (rebuilding a
 * unit_control_data and running the ordinary per-tick update, or an object_update of the parent
 * while a network client with a seated unit), and applies a remote position/vehicle update.
 * Once caught up, logs how many updates/ticks were skipped.
 *
 * @address 0x476d40
 */
void Players::client_catchup_on_server_updates()
{
    data_iterator iter;
    player *plr;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    plr = (player *)halo::memory::data_iterator_next(&iter);
    while (plr != (player *)0) {
        int32_t updates_applied = 0;

        if (plr->local_player_index == -1) {
            circular_queue *queue = &plr->update_history.queue;
            int32_t initial_backlog = update_queue_count(queue);

            for (;;) {
                player_update_record record;
                int32_t read_index = queue->read_index;
                int32_t write_index = queue->write_index;

                if (update_queue_count(queue) <= catchup_backlog_threshold) {
                    int32_t summed_ticks = 0;
                    int32_t i = read_index;

                    if (i != write_index) {
                        do {
                            summed_ticks = summed_ticks +
                                ((player_update_record **)queue->records)[i]->references_remaining;
                            i = (i + 1) % 0x78;
                        } while (i != queue->write_index);
                    }
                    if (summed_ticks <= catchup_time_threshold) {
                        break;
                    }
                }

                record.field0 = halo::k_dword_none;
                record.references_remaining = -1;
                record.reference_count = -1;

                if (read_index != write_index) {
                    player_update_record *head = ((player_update_record **)queue->records)[read_index];

                    head->references_remaining = head->references_remaining - 1;
                    if (head->references_remaining == 0) {

                        queue->read_index = (read_index + 1) % queue->capacity;
                    }
                    memcpy(&record, head, sizeof(record));
                    plr->update_history.has_current = 1;
                    memcpy(plr->update_history.current, &record.action, sizeof(plr->update_history.current));
                }

                updates_applied = updates_applied + 1;

                if (record.references_remaining == record.reference_count - 1 && halo::networking::globals().game_mode == 1 &&
                    plr->local_player_index == -1 && plr->unit != (datum_index)-1) {
                    int16_t index = (int16_t)plr->unit;
                    int16_t salt = (int16_t)((uint32_t)plr->unit >> 16);
                    object_header *header = 0;

                    if (index >= 0 && index < halo::objects::globals().object_data->maximum_count) {
                        object_header *candidate = (object_header *)((uint8_t *)halo::objects::globals().object_data->data +
                                                                     (int32_t)halo::objects::globals().object_data->size * index);

                        if (candidate->identifier != 0 && (salt == 0 || candidate->identifier == salt)) {
                            header = candidate;
                        }
                    }
                    if (header != 0 && (((1u << (header->type & 0x1f)) & _object_mask_unit) != 0) &&
                        header->data != 0) {
                        object *unit_obj = header->data;
                        uint8_t seated = PlayerView(*(datum_index *)((uint8_t *)unit_obj + 0x218)).unit_has_parent();

                        *(uint32_t *)((uint8_t *)unit_obj + 0x4bc) = record.field0;
                        if (seated == 0) {
                            halo::game::apply_remote_player_position_update(plr, unit_obj);
                        } else {
                            halo::game::apply_remote_player_vehicle_position_update(plr, unit_obj);
                        }
                    }
                }

                if (plr->unit == (datum_index)-1) {
                    continue;
                }

                {
                    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[plr->unit & halo::k_datum_slot_mask].data;
                    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
                    unit_control_data control;
                    uint8_t apply = 0;

                    if ((unit->flags & 0x40) == 0) {
                        continue;
                    }

                    memset(&control, 0, sizeof(control));
                    control.animation_state = 3;
                    control.aiming_speed = 0;
                    if (local_player_globals->input_disabled == 0) {
                        const player_action *action = &record.action;

                        control.control_flags = (uint16_t)action->control_flags;
                        control.weapon_index = action->weapon_index;
                        control.grenade_index = action->grenade_index;
                        control.zoom_level = action->zoom_level;
                        control.throttle.i = action->throttle_x;
                        control.throttle.j = action->throttle_y;
                        control.throttle.k = 0.0f;
                        control.primary_trigger = action->primary_trigger;

                        PlayerView(iter.index).compute_view_forward_vector((real *)&action->desired_yaw, &control.aiming_vector);
                        control.facing_vector = control.aiming_vector;
                        control.looking_vector = control.aiming_vector;
                        apply = 1;
                    } else if (unit->swarm_actor_index == (datum_index)-1 && unit->actor_index == (datum_index)-1) {
                        control.control_flags = 0;
                        control.weapon_index = -1;
                        control.grenade_index = -1;
                        control.zoom_level = -1;
                        control.throttle = *global_origin3d_pointer;
                        control.primary_trigger = 0.0f;
                        control.facing_vector = unit->desired_facing_vector;
                        control.aiming_vector = unit->desired_aiming_vector;
                        control.looking_vector = unit->desired_looking_vector;
                        apply = 1;
                    }
                    if (apply) {
                        halo::units::unit_apply_control_block(plr->unit, &control, -1);
                    }

                    if (PlayerView(iter.index).unit_has_parent() != 0 && halo::networking::globals().client_vehicle_ack_enabled != 0) {
                        halo::objects::object_update((uint32_t)unit_obj->parent_object);
                    } else {
                        halo::units::unit_update(plr->unit);
                        halo::units::biped_update(plr->unit);
                    }
                }
            }

            if (updates_applied > 0) {
                int32_t remaining_backlog = update_queue_count(queue);

                halo::networking::player_update_history_log_printf_filtered(
                    plr, 1, "[%d]: Caught up on [%d] updates == [%d] ticks.\n", game_time->game_time,
                    initial_backlog - remaining_backlog, updates_applied);
            }
        }

        plr = (player *)halo::memory::data_iterator_next(&iter);
    }
}

/**
 * Zeroes player_globals, re-seeds its datum-index fields to the wildcard and its two counters
 * to zero, marks both the "players" and "teams" data arrays valid and deletes every live datum
 * out of them, then clears the whole machine-to-player mapping table back to the wildcard.
 *
 * @address 0x473670
 */
void Players::dispose()
{
    int32_t i;

    memset(local_player_globals, 0, sizeof(player_globals));
    local_player_globals->local_players[0] = (datum_index)-1;
    local_player_globals->local_player_units[0] = (datum_index)-1;
    local_player_globals->unknown_00 = (datum_index)-1;
    local_player_globals->input_disabled = 0;
    local_player_globals->respawn_stagger = 0;
    local_player_globals->no_player_has_a_unit = 0;
    local_player_globals->bsp_switch_trigger_volume_index = -1;
    local_player_globals->mode = 0;

    player_data->valid = 1;
    halo::memory::data_delete_all(player_data);
    team_data->valid = 1;
    halo::memory::data_delete_all(team_data);

    for (i = 0; i < 16; i = i + 1) {
        machine_to_player[i] = (datum_index)-1;
    }
}

/**
 * Scans players until one with local_player_index != -1 is found (or the iterator is
 * exhausted), then always returns the datum-index wildcard.
 *
 * @address 0x477280
 */
datum_index Players::find_local_owned_unclear()
{
    data_iterator iter;
    player *plr;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)-1;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    plr = (player *)halo::memory::data_iterator_next(&iter);
    while (plr != (player *)0) {
        if (plr->local_player_index != -1) {
            break;
        }
        plr = (player *)halo::memory::data_iterator_next(&iter);
    }
    return (datum_index)-1;
}

/**
 * Returns the handle of the index-th live player datum, or -1 when there are fewer.
 *
 * @address 0x45c6f0
 */
uint32_t Players::get_active_by_index(int32_t index)
{
    data_iterator iterator;
    player *p;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)halo::memory::data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (index == 0) {
            return iterator.index;
        }
        index = index - 1;
        p = (player *)halo::memory::data_iterator_next(&iterator);
    }
    return halo::k_dword_none;
}

/**
 * Implements the original `players_handle_deleted_unit`.
 *
 * @address 0x479e20
 */
void Players::handle_deleted_unit(uint32_t object_index)
{
    uint8_t *object = *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + (object_index & halo::k_datum_slot_mask) * 0xc + 8);
    data_iterator iterator;
    uint8_t *player;

    if (((1u << (object[0xb4] & 0x1f)) & 3) == 0) {
        return;
    }
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)player_data ^ k_data_iterator_signature;
    for (player = (uint8_t *)halo::memory::data_iterator_next(&iterator); player != 0;
         player = (uint8_t *)halo::memory::data_iterator_next(&iterator)) {
        if (*(uint32_t *)&((struct player *)player)->unit == object_index) {
            PlayerView(iterator.index).reset_after_unit_change();
        }
    }
}

/**
 * Allocates the 16-entry "players" and "teams" data arrays, then bump-allocates
 * player_globals (0x98 bytes) and player_control_globals (0x50 bytes) out of the game-state
 * arena, CRC-registering each allocation's size. Only player_globals gets explicit field
 * writes here: local_players[0] and unknown_00 are seeded to the datum-index wildcard and
 * local_player_count is cleared.
 *
 * @address 0x4735b0
 */
void Players::initialize()
{
    uint32_t size;

    player_data = (data_array *)halo::saved_games::game_state_new((char *)"players", k_maximum_players, k_player_size);
    team_data = (data_array *)halo::saved_games::game_state_new((char *)"teams", k_maximum_teams, k_team_size);

    local_player_globals = (player_globals *)(halo::saved_games::globals().game_state_cursor + halo::saved_games::globals().game_state_base);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + k_player_globals_size;
    size = k_player_globals_size;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);
    local_player_globals->local_players[0] = (datum_index)-1;
    local_player_globals->unknown_00 = (datum_index)-1;
    local_player_globals->local_player_count = 0;

    player_control_globals_ptr = (player_control_globals *)(halo::saved_games::globals().game_state_cursor + halo::saved_games::globals().game_state_base);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + k_player_control_globals_size;
    size = k_player_control_globals_size;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);
}

/**
 * Implements the original `players_structure_bsp_switch_regroup`.
 *
 * @address 0x4762f0
 */
void StructureBsp::switch_regroup()
{
    int16_t volume = local_player_globals->bsp_switch_trigger_volume_index;
    real_point3d target;
    float offset = 0.0f;
    uint8_t have_flag = 0;
    uint8_t found = 0;
    datum_index chosen_unit = k_datum_index_none;
    int16_t flag_index;
    data_iterator iterator;
    player *entry;

    if (volume == -1 || local_player_globals->local_player_count <= 1) {
        players_clear_bsp_cluster();
        return;
    }

    flag_index = *(int16_t *)((uint8_t *)halo::scenario::globals().scenario->bsp_switch_trigger_volumes.pointer + volume * 8 + 6);
    if (flag_index != -1) {
        target = *(real_point3d *)((uint8_t *)halo::scenario::globals().scenario->cutscene_flags.pointer + flag_index * 0x5c + 0x24);
        offset = 0.0f;
        while (halo::physics::object_collision_test_cluster_group(0x4029, &target, halo::k_dword_none)) {
            double sum;

            target.z = target.z + 0.05f;
            sum = (double)offset + (double)0.05f;
            offset = (float)sum;
            if (!(sum < (double)0.3f)) {
                break;
            }
        }
        have_flag = 1;
        if (!(offset < 0.3f)) {
            have_flag = 0;
        }
    }

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)iterator.data ^ k_data_iterator_signature;
    while (!found && (entry = (player *)halo::memory::data_iterator_next(&iterator)) != 0) {
        uint8_t *unit_object;
        int16_t trigger_volume;
        real_point3d probe;
        float height;
        float radius;
        uint32_t leaf;

        if (entry->unit == k_datum_index_none || local_player_globals->bsp_switch_trigger_volume_index == -1) {
            continue;
        }
        unit_object = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[entry->unit & halo::k_datum_slot_mask].data;
        trigger_volume = *(int16_t *)((uint8_t *)halo::scenario::globals().scenario->bsp_switch_trigger_volumes.pointer + local_player_globals->bsp_switch_trigger_volume_index * 8);
        if (!halo::scenario::scenario_query::trigger_volume_contains_point(trigger_volume, (real_point3d *)(unit_object + 0xa0))) {
            continue;
        }
        halo::units::unit_get_crouch_height_offset(&probe, entry->unit, &height, &radius);
        offset = radius;
        leaf = halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, &probe);
        if (leaf == halo::k_dword_none ||
            *(int16_t *)((uint8_t *)halo::scenario::globals().structure_bsp->leaves.pointer + (leaf & 0x7fffffff) * 0x10 + 8) == -1) {
            continue;
        }
        if (!have_flag) {
            target = probe;
        } else {
            target.z = target.z + offset;
        }
        chosen_unit = entry->unit;
        found = 1;
    }

    if (found && local_player_globals->local_players[0] != k_datum_index_none) {
        datum_index player_index = local_player_globals->local_players[0];
        player *local = halo::game::player_at(player_index);

        if (local->unit != k_datum_index_none && local->unit != chosen_unit) {
            halo::game::game_engine_reattach_player_unit_unused(player_index, chosen_unit, &target);
            (halo::game::player_at(player_index))->bsp_cluster = -1;
        }
    }
    local_player_globals->bsp_switch_trigger_volume_index = -1;
    players_clear_bsp_cluster();
}

/**
 * Picks a free local-player slot index (0..3). Prefers a slot that has a joystick mapped
 * and that no live player currently occupies; if none of the four hinted slots is free, falls
 * back to the first slot with no live player at all; returns -1 if every slot is taken.
 *
 * @address 0x473730
 */
int32_t LocalPlayers::find_free_slot_index()
{
    int32_t i;

    for (i = 0; i < 4; i = i + 1) {
        if (halo::input::globals().joystick_slot_devices[i] != -1 &&
            Players::any_with_local_player_index((int16_t)i) == 0) {
            return i;
        }
    }
    for (i = 0; i < 4; i = i + 1) {
        if (Players::any_with_local_player_index((int16_t)i) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * blam-cc: CX -> local_player_index
 * Returns the tracked desired_zoom_level for the given local player slot, or -1 if
 * local_player_index itself is -1.
 *
 * @address 0x472740
 */
int32_t LocalPlayers::get_zoom_level(int16_t local_player_index)
{
    if (local_player_index != -1) {
        return player_control_globals_ptr->local_players[local_player_index].desired_zoom_level;
    }
    return -1;
}

/**
 * Detaches the local player slot's currently controlled unit (clears its controlling_player and
 * notifies unit_refresh_targeting_flag_and_weapons), then attaches `new_unit` as this local player's controlled unit
 * (setting its controlling_player to the player bound to this slot, or the wildcard if the slot
 * index is out of range). Finally sets that player's unit/previous_unit fields and tail-calls
 * game_engine_init_player_look_state_from_object to (re)seed the look state.
 *
 * @address 0x474fc0
 */
void LocalPlayers::set_controlled_unit(datum_index new_unit, int16_t local_player_index)
{
    local_player_control *look = &player_control_globals_ptr->local_players[local_player_index];
    datum_index old_unit = look->unit;
    datum_index owner;
    object *obj;
    unit_data *unit;
    player *plr;

    if (old_unit != (datum_index)-1) {
        obj = ((object_header *)halo::objects::globals().object_data->data)[old_unit & halo::k_datum_slot_mask].data;
        unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        unit->controlling_player = (datum_index)-1;
        halo::units::unit_refresh_targeting_flag_and_weapons(old_unit, 0);
    }

    if (new_unit != (datum_index)-1) {
        obj = ((object_header *)halo::objects::globals().object_data->data)[new_unit & halo::k_datum_slot_mask].data;
        halo::units::unit_refresh_targeting_flag_and_weapons(new_unit, 1);
        owner = (datum_index)-1;
        if (local_player_index != -1 && local_player_index <= 0) {
            owner = local_player_globals->local_players[local_player_index];
        }
        unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        unit->controlling_player = owner;
    }

    owner = (datum_index)-1;
    if (local_player_index != -1 && local_player_index <= 0) {
        owner = local_player_globals->local_players[local_player_index];
    }
    plr = halo::game::player_at(owner);
    plr->unit = new_unit;
    plr->previous_unit = (datum_index)-1;

    halo::game::game_engine_init_player_look_state_from_object(new_unit, local_player_index);
}

/**
 * Returns the player handle currently bound to local-player slot `local_player_index`, or the
 * wildcard if the slot index is out of range (this build only ever has slot 0).
 *
 * @address 0x474d30
 */
datum_index LocalPlayers::to_player_index(int16_t local_player_index)
{
    if (local_player_index != -1 && local_player_index < 1) {
        return local_player_globals->local_players[local_player_index];
    }
    return (datum_index)-1;
}

/**
 * Implements the original `any_local_player_within_10_units`.
 *
 * @address 0x453330
 */
uint8_t LocalPlayers::any_within_10_units(const real_point3d *query_point)
{
    int16_t local_player_count = local_player_globals->local_player_count;
    int16_t slot;

    if (local_player_count > 2) {
        return 1;
    }

    for (slot = 0; slot < 1; slot++) {
        if (slot != -1 && slot < 1 && (int32_t)local_player_globals->local_players[slot] != -1) {
            float dx = query_point->x - camera_point[slot * 0xa7];
            float dy = query_point->y - camera_position_y_table[slot * 0xa7];
            float dz = query_point->z - camera_position_z_table[slot * 0xa7];
            if (dx * dx + dz * dz + dy * dy < 100.0f) {
                return 1;
            }
        }
    }
    return 0;
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::PlayerView::apply_pickup_effect; forwards to the C++ implementation.
 * register convention: none -- both are genuine stack parameters (Ghidra's own
 * param_1/param_2).
 *
 * @address 0x479930
 */
void player_apply_pickup_effect(uint32_t player_index, uint32_t pickup_object)
{
    halo::game::PlayerView(player_index).apply_pickup_effect(pickup_object);
}

/**
 * C entry point for halo::game::PlayerView::attach_unit_to_parent; forwards to the C++ implementation.
 * blam-cc: stack -> player_index, target_object, local_offset
 *
 * @address 0x475c60
 */
uint8_t player_attach_unit_to_parent(uint32_t player_index, uint32_t target_object, void *local_offset)
{
    return halo::game::PlayerView(player_index).attach_unit_to_parent(target_object, local_offset);
}

/**
 * C entry point for halo::game::PlayerView::check_assassination_opportunity; forwards to the C++ implementation.
 * register convention: none -- both are genuine stack parameters (Ghidra's own
 * param_1/param_2).
 *
 * @address 0x478770
 */


/**
 * C entry point for halo::game::PlayerView::check_vehicle_boarding_interaction; forwards to the C++ implementation.
 * blam-cc: stack -> (player_index, candidate_object)
 *
 * @address 0x4788a0
 */


/**
 * C entry point for halo::game::PlayerView::check_vehicle_boarding_interaction_lightweight; forwards to the C++ implementation.
 * register convention: none -- both are genuine stack parameters (Ghidra's own
 * param_1/param_2).
 *
 * @address 0x478c40
 */


/**
 * C entry point for halo::game::PlayerView::check_vehicle_interaction; forwards to the C++ implementation.
 * register convention: none -- both are genuine stack parameters (Ghidra's own param_1/param_2).
 *
 * @address 0x478600
 */


/**
 * C entry point for halo::game::PlayerView::execute_pending_interaction; forwards to the C++ implementation.
 * blam-cc: stack -> player_index
 *
 * @address 0x4793a0
 */
uint8_t player_execute_pending_interaction(uint32_t player_index)
{
    return halo::game::PlayerView(player_index).execute_pending_interaction();
}

/**
 * C entry point for halo::game::PlayerView::execute_weapon_drop_interaction; forwards to the C++ implementation.
 * blam-cc: stack -> player_index
 *
 * @address 0x4790d0
 */
uint8_t player_execute_weapon_drop_interaction(uint32_t player_index)
{
    return halo::game::PlayerView(player_index).execute_weapon_drop_interaction();
}

/**
 * C entry point for halo::game::PlayerView::find_placement_position; forwards to the C++ implementation.
 * blam-cc: stack -> player_index, target_object, point
 *
 * @address 0x4757b0
 */
uint8_t player_find_placement_position(uint32_t player_index, datum_index target_object, real_point3d *point)
{
    return halo::game::PlayerView(player_index).find_placement_position(target_object, point);
}

/**
 * C entry point for halo::game::PlayerView::kill_and_release_unit; forwards to the C++ implementation.
 * register convention: EBX -> player_index, stack -> respawn_timer_override.
 * // blam-cc: EBX -> player_index, stack -> respawn_timer_override
 * blam-cc: EBX -> player_index, stack -> respawn_timer_override
 *
 * @address 0x476250
 */
void player_kill_and_release_unit(uint32_t player_index, int32_t respawn_timer_override)
{
    halo::game::PlayerView(player_index).kill_and_release_unit(respawn_timer_override);
}

/**
 * C entry point for halo::game::PlayerView::release_unit_and_reset; forwards to the C++ implementation.
 * register convention: EAX -> player_index, stack -> previous_unit_override.
 * // blam-cc: EAX -> player_index, stack -> previous_unit_override
 * blam-cc: EAX -> player_index, stack -> previous_unit_override
 *
 * @address 0x4760b0
 */


/**
 * C entry point for halo::game::PlayerView::reset_after_unit_change; forwards to the C++ implementation.
 * register convention: stack -> player_index.
 * blam-cc: EDI -> iterator
 * blam-cc: EAX -> player_handle
 *
 * @address 0x474e10
 */
void player_reset_after_unit_change(uint32_t player_index)
{
    halo::game::PlayerView(player_index).reset_after_unit_change();
}

/**
 * C entry point for halo::game::PlayerView::trigger_full_health_effect; forwards to the C++ implementation.
 * register convention: a player index in EDX (in_EDX); no stack parameters.
 * // blam-cc: EDX -> player_index
 * blam-cc: EDX -> player_index
 *
 * @address 0x479890
 */
void player_trigger_full_health_effect(uint32_t player_index)
{
    halo::game::PlayerView(player_index).trigger_full_health_effect();
}

/**
 * C entry point for halo::game::PlayerView::trigger_shield_recharge_effect; forwards to the C++ implementation.
 * register convention: a player index in EDX (in_EDX); no stack parameters.
 * // blam-cc: EDX -> player_index
 * blam-cc: EDX -> player_index
 *
 * @address 0x479710
 */
void player_trigger_shield_recharge_effect(uint32_t player_index)
{
    halo::game::PlayerView(player_index).trigger_shield_recharge_effect();
}

/**
 * C entry point for halo::game::PlayerView::swap_to_weapon; forwards to the C++ implementation.
 * blam-cc: EAX -> player_index, stack -> target_weapon
 *
 * @address 0x479240
 */
uint8_t player_swap_to_weapon(uint32_t player_index, datum_index target_weapon)
{
    return halo::game::PlayerView(player_index).swap_to_weapon(target_weapon);
}

/**
 * C entry point for halo::game::PlayerView::update_nearby_interactions_primary; forwards to the C++ implementation.
 * register convention: a player index in EDI (unaff_EDI); no stack parameters.
 * // blam-cc: EDI -> player_index
 * blam-cc: EDI -> player_index
 *
 * @address 0x478400
 */
void player_update_nearby_interactions_primary(uint32_t player_index)
{
    halo::game::PlayerView(player_index).update_nearby_interactions_primary();
}

/**
 * C entry point for halo::game::PlayerView::update_nearby_interactions_secondary; forwards to the C++ implementation.
 * register convention: a player index in EDI (unaff_EDI); no stack parameters.
 * // blam-cc: EDI -> player_index
 * blam-cc: EDI -> player_index
 *
 * @address 0x478500
 */
void player_update_nearby_interactions_secondary(uint32_t player_index)
{
    halo::game::PlayerView(player_index).update_nearby_interactions_secondary();
}

/**
 * C entry point for halo::game::PlayerView::unit_has_parent; forwards to the C++ implementation.
 * register convention: ECX -> player_handle.
 * // blam-cc: ECX -> player_handle
 * blam-cc: EAX -> parent_object, CX -> vehicle_seat_index
 * blam-cc: ECX -> player_handle
 *
 * @address 0x477210
 */
uint8_t player_unit_has_parent(datum_index player_handle)
{
    return halo::game::PlayerView(player_handle).unit_has_parent();
}

/**
 * C entry point for halo::game::PlayerView::set_pending_interaction_action; forwards to the C++ implementation.
 * register convention: the acting player's index in EAX (in_EAX) and a candidate interaction
 * object in EBX (unaff_EBX); `priority_type` and `seat` are this function's own two stack
 * parameters.
 * // blam-cc: EAX -> player_index, EBX -> candidate_object, stack -> priority_type, seat
 * blam-cc: EAX -> player_index, EBX -> candidate_object, stack -> priority_type, seat
 *
 * @address 0x478e00
 */


/**
 * C entry point for halo::game::PlayerView::is_busy_with_interaction; forwards to the C++ implementation.
 * register convention: ESI -> candidate_object (used for unit_lacks_weapon_type_of/object_try_and_get/
 * game_engine_ctf_unit_weapon_must_be_readied), EDI -> unit_or_player_index (used only for unit_count_deployed_weapons and as unit_lacks_weapon_type_of's
 * second argument); no stack parameters.
 * // blam-cc: ESI -> candidate_object, EDI -> unit_or_player_index
 * blam-cc: ESI -> candidate_object, EDI -> unit_or_player_index
 *
 * @address 0x478820
 */


/**
 * C entry point for halo::game::PlayerView::current_weapon_prevents_camo_depower; forwards to the C++ implementation.
 * register convention: player handle in ECX (in_ECX); EAX is read by no instruction in this
 * function and is not part of its real signature.
 * // blam-cc: ECX -> player_handle
 * blam-cc: ECX -> player_handle
 *
 * @address 0x466390
 */


/**
 * C entry point for halo::game::PlayerView::has_must_be_readied_weapon; forwards to the C++ implementation.
 * register convention: a player index in ECX (in_ECX).
 * // blam-cc: ECX -> player_index
 * blam-cc: ECX -> player_index
 *
 * @address 0x463300
 */
uint8_t unit_has_must_be_readied_weapon(uint32_t player_index)
{
    return halo::game::PlayerView(player_index).has_must_be_readied_weapon();
}

/**
 * C entry point for halo::game::PlayerView::reset_gauge_if_flagged; forwards to the C++ implementation.
 * register convention: a player index in EAX (in_EAX).
 * // blam-cc: EAX -> player_index
 * blam-cc: EAX -> player_index
 *
 * @address 0x4633a0
 */
void unit_reset_gauge_if_flagged(uint32_t player_index)
{
    halo::game::PlayerView(player_index).reset_gauge_if_flagged();
}

/**
 * C entry point for halo::game::PlayerView::update_active_camouflage_depower; forwards to the C++ implementation.
 * register convention: player handle in EBX (unaff_EBX, the incoming argument no prologue code
 * initializes).
 * // blam-cc: EBX -> player_handle
 * blam-cc: EBX -> player_handle
 * blam-cc: EAX, CX
 * blam-cc: ECX
 *
 * @address 0x466420
 */
void unit_update_active_camouflage_depower(datum_index player_handle)
{
    halo::game::PlayerView(player_handle).update_active_camouflage_depower();
}

/**
 * C entry point for halo::game::KillStreak::add_kill_streak; forwards to the C++ implementation.
 * register convention: a player handle in EBX (unaff_EBX); `slot` and `amount` are this
 * function's own two stack parameters.
 * // blam-cc: EBX -> player_handle, stack -> slot, amount
 * blam-cc: EAX -> player_handle, stack -> slot
 * blam-cc: EAX -> player_handle, stack -> slot
 * blam-cc: ECX -> player_handle, stack -> slot, amount
 * blam-cc: EBX -> player_handle, stack -> slot, amount
 *
 * @address 0x479ba0
 */
uint8_t player_add_kill_streak(int32_t slot, int16_t amount, uint32_t player_handle)
{
    return halo::game::KillStreak(player_handle).add_kill_streak(slot, amount);
}

/**
 * C entry point for halo::game::KillStreak::advance_multikill_medal; forwards to the C++ implementation.
 * register convention: a player handle in ECX (in_ECX); no stack parameters.
 * // blam-cc: ECX -> player_handle
 * blam-cc: ECX -> player_handle
 *
 * @address 0x479eb0
 */
void player_advance_multikill_medal(uint32_t player_handle)
{
    halo::game::KillStreak(player_handle).advance_multikill_medal();
}

/**
 * C entry point for halo::game::KillStreak::set_max; forwards to the C++ implementation.
 * register convention: player index in EAX (in_EAX), candidate value in SI (unaff_SI); `slot`
 * is this function's own stack parameter.
 * // blam-cc: EAX -> player_index, ESI -> value, stack -> slot
 * blam-cc: EAX -> player_index, ESI -> value, stack -> slot
 *
 * @address 0x479ca0
 */
void player_kill_streak_set_max(int16_t slot, uint32_t player_index, int16_t value)
{
    halo::game::KillStreak(player_index).set_max(slot, value);
}

/**
 * C entry point for halo::game::KillStreak::tick; forwards to the C++ implementation.
 * register convention: a player index in EAX (in_EAX); no stack parameters.
 * // blam-cc: EAX -> player_index
 * blam-cc: EAX -> player_index
 *
 * @address 0x479d10
 */
void player_kill_streak_tick(uint32_t player_index)
{
    halo::game::KillStreak(player_index).tick();
}

/**
 * C entry point for halo::game::KillStreak::notify_kill_streak_update; forwards to the C++ implementation.
 * register convention: a hash-table key (the player handle) in ECX (in_ECX); `slot` and
 * `amount` are this function's own two stack parameters.
 * // blam-cc: ECX -> player_handle, stack -> slot, amount
 * blam-cc: ESI table, ECX key
 * blam-cc: ECX -> player_handle, stack -> slot, amount
 *
 * @address 0x479aa0
 */


/**
 * C entry point for halo::game::KillStreak::trigger_kill_streak_effect; forwards to the C++ implementation.
 * register convention: a player index in EDX (in_EDX); no stack parameters.
 * // blam-cc: EDX -> player_index
 * blam-cc: EDX -> player_index
 *
 * @address 0x4797d0
 */
void player_trigger_kill_streak_effect(uint32_t player_index)
{
    halo::game::KillStreak(player_index).trigger_kill_streak_effect();
}

/**
 * C entry point for halo::game::LocalPlayerUnit::get_local_player_weapon_index; forwards to the C++ implementation.
 * register convention: unit handle in EAX (Ghidra's `in_EAX`).
 * // blam-cc: EAX -> unit
 * blam-cc: EAX -> unit
 *
 * @address 0x4726b0
 */
int32_t unit_get_local_player_weapon_index(datum_index unit)
{
    return halo::game::LocalPlayerUnit(unit).get_local_player_weapon_index();
}

/**
 * C entry point for halo::game::LocalPlayerUnit::set_local_player_weapon_index; forwards to the C++ implementation.
 * register convention: unit handle in EAX (Ghidra's `in_EAX`); the new weapon index is this
 * blam-cc: EAX -> unit, stack -> weapon_index
 * blam-cc: EAX -> unit, stack -> weapon_index
 *
 * @address 0x472100
 */
void unit_set_local_player_weapon_index(datum_index unit, int16_t weapon_index)
{
    halo::game::LocalPlayerUnit(unit).set_local_player_weapon_index(weapon_index);
}

/**
 * C entry point for halo::game::LocalPlayerUnit::invalidate_local_player_zoom_level; forwards to the C++ implementation.
 * register convention: unit handle in EAX (Ghidra's `in_EAX`).
 * // blam-cc: EAX -> unit
 * blam-cc: EAX -> unit
 *
 * @address 0x4726f0
 */
void unit_invalidate_local_player_zoom_level(datum_index unit)
{
    halo::game::LocalPlayerUnit(unit).invalidate_local_player_zoom_level();
}

/**
 * C entry point for halo::game::LocalPlayerUnit::get_current_weapon_autoaim_cone; forwards to the C++ implementation.
 * register convention: unit object handle in EAX (in_EAX), a caller zoom-state flag in EDX
 * (in_DX; -1 disables the zoom gate), output float buffer in EDI (unaff_EDI, 5 floats).
 * // blam-cc: EAX -> unit_index, EDX -> require_zoomed, EDI -> out (5 floats)
 * blam-cc: EAX -> unit_index, EDX -> require_zoomed, EDI -> out
 *
 * @address 0x459e80
 */
uint8_t unit_get_current_weapon_autoaim_cone(datum_index unit_index, int16_t require_zoomed, real *out)
{
    return halo::game::LocalPlayerUnit(unit_index).get_current_weapon_autoaim_cone(require_zoomed, out);
}

/**
 * C entry point for halo::game::ObjectView::snap_position_if_far; forwards to the C++ implementation.
 * register convention: EAX -> new_position, ECX -> obj.
 * // blam-cc: EAX -> new_position, ECX -> obj
 * blam-cc: EAX -> new_position, ECX -> obj
 *
 * @address 0x4772e0
 */
void unit_snap_position_if_far(real_point3d *new_position, object *obj)
{
    halo::game::ObjectView(obj).snap_position_if_far(new_position);
}

/**
 * C entry point for halo::game::Players::new_local; forwards to the C++ implementation.
 * register convention: EAX -> requested_handle (index and salt, for datum_new_at_index_with_salt;
 * -1 for "any free slot" via plain datum_new); stack -> machine_index, local_player_index,
 * identifier_record.
 * // blam-cc: EAX -> requested_handle
 * blam-cc: EDX -> array
 * blam-cc: EAX -> requested_handle, EDX -> array
 * blam-cc: ESI -> queue (matches src/game/update_server_dispose.c)
 * blam-cc: ESI -> queue
 * blam-cc: ESI -> queue
 * blam-cc: EAX -> requested_handle, stack -> machine_index, local_player_index, identifier_record
 *
 * @address 0x473940
 */
datum_index player_new_local(datum_index requested_handle, uint32_t machine_index, int16_t local_player_index, uint16_t *identifier_record)
{
    return halo::game::Players::new_local(requested_handle, machine_index, local_player_index, identifier_record);
}

/**
 * C entry point for halo::game::Players::new_network; forwards to the C++ implementation.
 * register convention: EAX -> requested_index (datum_new_at_index's exact index request, or -1
 * for "any free slot"); stack -> machine_index, local_player_index, identifier_record.
 * // blam-cc: EAX -> requested_index
 * blam-cc: EDX -> array
 * blam-cc: AX -> index, EDX -> array (matches src/memory/datum_new_at_index.c)
 * blam-cc: EAX -> requested_index, stack -> machine_index, local_player_index, identifier_record
 *
 * @address 0x473780
 */
datum_index player_new_network(datum_index requested_index, uint32_t machine_index, int16_t local_player_index, uint16_t *identifier_record)
{
    return halo::game::Players::new_network(requested_index, machine_index, local_player_index, identifier_record);
}

/**
 * C entry point for halo::game::Players::delete_player; forwards to the C++ implementation.
 * register convention: EAX -> machine_index, EBX -> player_handle.
 * // blam-cc: EAX -> machine_index, EBX -> player_handle
 * blam-cc: ESI -> queue
 * blam-cc: EAX -> array,
 * blam-cc: EAX -> machine_index, EBX -> player_handle
 *
 * @address 0x473ae0
 */


/**
 * C entry point for halo::game::Players::remove_player; forwards to the C++ implementation.
 * register convention: player_handle in EAX (unaff_EAX/in_EAX in Ghidra's read of it).
 * // blam-cc: EAX -> player_handle
 * blam-cc: EAX -> array,
 * blam-cc: EAX -> table, ESI -> player_handle; UNSURE full behavior
 * blam-cc: ESI -> player_handle
 * blam-cc: EAX -> player_handle
 *
 * @address 0x473bb0
 */
void player_remove(datum_index player_handle)
{
    halo::game::Players::remove_player(player_handle);
}

/**
 * C entry point for halo::game::Players::index_from_unit_index; forwards to the C++ implementation.
 * register convention: stack -> unit_index; returns the player index in EAX. ESI is saved and
 * overwritten before use, so it is not an argument.
 * blam-cc: EDI -> iterator
 *
 * @address 0x474db0
 */
datum_index player_index_from_unit_index(datum_index unit_index)
{
    return halo::game::Players::index_from_unit_index(unit_index);
}

/**
 * C entry point for halo::game::Players::set_team_by_color; forwards to the C++ implementation.
 * register convention: fully reconstructed against
 * objdump -d -M intel --start-address=0x470630 --stop-address=0x470689 bin/halo.exe
 * blam-cc: EBX -> new_team, ESI -> target_team_index_desired
 * blam-cc: iterator in EDI
 * blam-cc: EBX -> new_team, ESI -> target_team_index_desired
 *
 * @address 0x470630
 */
void player_set_team_by_color(uint8_t new_team, int8_t target_team_index_desired)
{
    halo::game::Players::set_team_by_color(new_team, target_team_index_desired);
}

/**
 * C entry point for halo::game::Players::spawn_starting_profile_weapon; forwards to the C++ implementation.
 * register convention: the owning struct pointer (a TagDependency inside the caller's
 * ScenarioPlayerStartingProfile) in ESI (unaff_ESI); the datum role value ("param_2" to
 * object_placement_data_initialize) is this function's own single stack parameter.
 * // blam-cc: ESI -> weapon_dependency, stack -> role
 * blam-cc: ESI -> weapon_dependency, stack -> role
 *
 * @address 0x477810
 */
datum_index player_spawn_starting_profile_weapon(TagDependency *weapon_dependency, uint32_t role)
{
    return halo::game::Players::spawn_starting_profile_weapon(weapon_dependency, role);
}

/**
 * C entry point for halo::game::Players::active_count; forwards to the C++ implementation.
 * register convention: __cdecl, no arguments.
 * blam-cc: EDI -> iterator
 *
 * @address 0x45c6a0
 */
int32_t players_active_count(void)
{
    return halo::game::Players::active_count();
}

/**
 * C entry point for halo::game::Players::any_pending_seat_or_respawn; forwards to the C++ implementation.
 * register convention: no arguments; return value in EAX.
 * blam-cc: EDI -> iterator
 * blam-cc: EAX -> unit_handle; UNSURE purpose
 *
 * @address 0x475090
 */
uint8_t players_any_pending_seat_or_respawn(void)
{
    return halo::game::Players::any_pending_seat_or_respawn();
}

/**
 * C entry point for halo::game::Players::any_with_local_player_index; forwards to the C++ implementation.
 * register convention: ESI -> local_player_index (the only caller, this batch's
 * local_player_find_free_slot_index at 0x473730, loads it straight into ESI with no visible
 * mov ahead of the call, so Ghidra shows it as unaff_ESI).
 * // blam-cc: ESI -> local_player_index
 * blam-cc: EDI -> iterator
 * blam-cc: ESI -> local_player_index
 *
 * @address 0x4736d0
 */


/**
 * C entry point for halo::game::Players::any_without_unit; forwards to the C++ implementation.
 * register convention: no arguments; return value in EAX.
 * blam-cc: EDI -> iterator
 *
 * @address 0x475210
 */
uint8_t players_any_without_unit(void)
{
    return halo::game::Players::any_without_unit();
}

/**
 * C entry point for halo::game::Players::client_catchup_on_server_updates; forwards to the C++ implementation.
 * blam-cc: EDI -> iterator
 * blam-cc: EAX handle, ECX yaw_pitch, ESI out
 * blam-cc: ECX
 * blam-cc: EAX -> plr, EBX -> unit_obj
 * blam-cc: EAX -> plr, EBX -> unit_obj
 * blam-cc: EAX target_player
 * blam-cc: EAX unit_index, EDX control, stack source_id
 *
 * @address 0x476d40
 */
void players_client_catchup_on_server_updates(void)
{
    halo::game::Players::client_catchup_on_server_updates();
}

/**
 * C entry point for halo::game::Players::dispose; forwards to the C++ implementation.
 * register convention: no arguments.
 * blam-cc: ESI -> array (memory module)
 *
 * @address 0x473670
 */
void players_dispose(void)
{
    halo::game::Players::dispose();
}

/**
 * C entry point for halo::game::Players::find_local_owned_unclear; forwards to the C++ implementation.
 * register convention: no arguments; return value in EAX.
 * blam-cc: EDI -> iterator
 *
 * @address 0x477280
 */
datum_index players_find_local_owned_unclear(void)
{
    return halo::game::Players::find_local_owned_unclear();
}

/**
 * C entry point for halo::game::Players::get_active_by_index; forwards to the C++ implementation.
 * register convention: index in EAX (in_EAX).
 * // blam-cc: EAX -> index
 * blam-cc: EDI -> iterator
 * blam-cc: EAX -> index
 *
 * @address 0x45c6f0
 */
uint32_t players_get_active_by_index(int32_t index)
{
    return halo::game::Players::get_active_by_index(index);
}

/**
 * C entry point for halo::game::Players::handle_deleted_unit; forwards to the C++ implementation.
 * blam-cc: stack -> object_index (cdecl)
 * blam-cc: EDI
 *
 * @address 0x479e20
 */
void players_handle_deleted_unit(uint32_t object_index)
{
    halo::game::Players::handle_deleted_unit(object_index);
}

/**
 * C entry point for halo::game::Players::initialize; forwards to the C++ implementation.
 * register convention: no arguments.
 * blam-cc: EBX -> element_size, stack -> name, maximum_count
 * blam-cc: EBX -> element_size, then the stack pair (name, maximum_count).
 *
 * @address 0x4735b0
 */
void players_initialize(void)
{
    halo::game::Players::initialize();
}

/**
 * C entry point for halo::game::StructureBsp::switch_regroup; forwards to the C++ implementation.
 * blam-cc: no arguments
 * blam-cc: EDI -> iterator
 *
 * @address 0x4762f0
 */
void players_structure_bsp_switch_regroup(void)
{
    halo::game::StructureBsp::switch_regroup();
}

/**
 * C entry point for halo::game::LocalPlayers::find_free_slot_index; forwards to the C++ implementation.
 * register convention: no arguments; return value in EAX.
 * blam-cc: ESI -> local_player_index
 *
 * @address 0x473730
 */
int32_t local_player_find_free_slot_index(void)
{
    return halo::game::LocalPlayers::find_free_slot_index();
}

/**
 * C entry point for halo::game::LocalPlayers::get_zoom_level; forwards to the C++ implementation.
 * register convention: local-player index in CX (Ghidra's `in_CX`).
 * // blam-cc: CX -> local_player_index
 * blam-cc: CX -> local_player_index
 *
 * @address 0x472740
 */
int32_t local_player_get_zoom_level(int16_t local_player_index)
{
    return halo::game::LocalPlayers::get_zoom_level(local_player_index);
}

/**
 * C entry point for halo::game::LocalPlayers::set_controlled_unit; forwards to the C++ implementation.
 * register convention: ESI -> new_unit, DI -> local_player_index.
 * // blam-cc: ESI -> new_unit, DI -> local_player_index
 * blam-cc: stack -> unit_handle, CL -> attaching
 * blam-cc: ESI -> new_unit, DI -> local_player_index
 *
 * @address 0x474fc0
 */
void local_player_set_controlled_unit(datum_index new_unit, int16_t local_player_index)
{
    halo::game::LocalPlayers::set_controlled_unit(new_unit, local_player_index);
}

/**
 * C entry point for halo::game::LocalPlayers::to_player_index; forwards to the C++ implementation.
 * register convention: AX -> local_player_index.
 * // blam-cc: AX -> local_player_index
 * blam-cc: AX -> local_player_index
 *
 * @address 0x474d30
 */
datum_index local_player_to_player_index(int16_t local_player_index)
{
    return halo::game::LocalPlayers::to_player_index(local_player_index);
}

/**
 * C entry point for halo::game::LocalPlayers::any_within_10_units; forwards to the C++ implementation.
 * register convention: EDX = const real_point3d *query_point (in_EDX). The function takes no
 * other visible parameter; "10 units" (100 squared) is a literal, not an argument.
 * blam-cc: any_local_player_within_10_units(const real_point3d *query_point /*EDX* /), returns a
 * bool in AL (0x4533a4 `mov al,bl` / 0x4533a9 `mov al,1`; the orphan pass 4 review narrowed the
 * return type from uint32_t). The 0x006ac6d0 table is observers[i].camera.position
 * (types/camera.h observer_camera, 0x29c-byte observer stride), the same record
 * sound_environment_update and structure_regions_find_within_radius read.
 * blam-cc: EDX query_point; result in AL
 *
 * @address 0x453330
 */
uint8_t any_local_player_within_10_units(const real_point3d *query_point)
{
    return halo::game::LocalPlayers::any_within_10_units(query_point);
}

}
