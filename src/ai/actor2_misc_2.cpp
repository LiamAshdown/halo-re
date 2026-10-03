#include "halo/ai/actor_view.hpp"
#include "halo/scenario/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"

namespace halo::ai {

namespace actor_reassign_vehicle_seat_local {
extern "C" {
extern int8_t teams_are_enemies(int16_t team_a, int16_t team_b);
}
}

/**
 * Resolves which occupant (if any) currently sits in vehicle_object_index's gunner seat, or its driver seat when
 * the gunner seat is empty or seat_selector requests the driver seat directly (== 9), falling back to
 * vehicle_object_index itself when both seats are empty. If that occupant differs from self
 *
 * @address 0x42b880
 */
int32_t ActorOps::reassign_vehicle_seat(datum_index vehicle_object_index, datum_index self_object_index, int32_t seat_selector)
{
    using namespace actor_reassign_vehicle_seat_local;
    datum_index occupant;
    object *vehicle_obj;
    unit_data *vehicle_unit;
    int32_t reason;

    occupant = (datum_index)k_datum_index_none;
    if (vehicle_object_index != (datum_index)k_datum_index_none) {
        vehicle_obj = (object *)halo::objects::object_try_and_get(vehicle_object_index, 3);
        if (vehicle_obj != 0) {
            vehicle_unit = (unit_data *)((uint8_t *)vehicle_obj + k_unit_data_offset);
            occupant = (datum_index)k_datum_index_none;
            if (seat_selector != 9) {
                occupant = vehicle_unit->gunner_unit_index;
            }
            if (seat_selector == 9 || occupant == (datum_index)k_datum_index_none) {
                occupant = vehicle_unit->driver_unit_index;
                if (occupant == (datum_index)k_datum_index_none) {
                    occupant = vehicle_object_index;
                }
            }
        }
    }

    if (self_object_index == occupant) {
        reason = 1;
    } else if (occupant == (datum_index)k_datum_index_none) {
        reason = 0;
    } else {
        object *occupant_obj = halo::ai::object_at(occupant);
        object *self_obj = halo::ai::object_at(self_object_index);
        reason = halo::game::teams_are_enemies(((struct object *)occupant_obj)->owner_team,
                               ((struct object *)self_obj)->owner_team) ? 3 : 2;
    }

    halo::ai::ai_communication_broadcast(0, self_object_index, occupant, reason, seat_selector,
                                (datum_index)k_datum_index_none, 0);
    halo::ai::ai_conversation_clear_object_references(self_object_index, 0);
    halo::ai::encounters_note_hostile_object(self_object_index);
    return 0;
}

namespace actor_refresh_combat_context_local {
extern "C" {
extern game_engine_definition *current_game_engine;
extern uint8_t *team_pair_data;
extern const real_point3d *global_zero_vector3d_pointer;
extern char ai_marker_name_b[];
#define A_I16(offset) (*(int16_t *)((uint8_t *)self + (offset)))
#define A_I32(offset) (*(int32_t *)((uint8_t *)self + (offset)))
static uint8_t *object_get(datum_index object_index)
{
    return reinterpret_cast<uint8_t *>(halo::ai::object_at(object_index));
}
}
}

/**
 * Actor AI behaviour: refresh combat context.
 *
 * @address 0x4297a0
 */
void ActorView::refresh_combat_context()
{
    using namespace actor_refresh_combat_context_local;
    actor *self = halo::ai::actor_at(actor_index);
    uint8_t *actor_tag = (uint8_t *)halo::cache::globals().tag_instances[self->actor_definition_tag & halo::k_slot_mask].data;
    uint8_t *unit;
    uint8_t *parent = 0;
    datum_index parent_index;
    datum_index child;

    if (self->swarm) {
        struct swarm *swarm = halo::ai::swarm_at(self->swarm_index);
        real_point3d *center = &swarm->aggregate_position;
        int16_t count = swarm->component_count;
        int16_t i;

        *center = *global_zero_vector3d_pointer;
        for (i = 0; i < count; i++) {
            uint8_t *creature = (uint8_t *)halo::ai::globals().swarm_component_data->data +
                (*(datum_index *)((uint8_t *)swarm + 0x58 + i * 4) & halo::k_slot_mask) * 0x40;
            datum_index creature_unit = *(datum_index *)((uint8_t *)swarm + 0x18 + i * 4);
            uint8_t *creature_object = object_get(creature_unit);
            datum_index vehicle = ((struct object *)creature_object)->type == 0 ?
                *(datum_index *)(creature_object + 0x4d8) : k_datum_index_none;

            halo::objects::object_get_position((real_point3d *)(creature + 4), creature_unit);
            *(datum_index *)(creature + 0x10) = vehicle;
            center->x = *(float *)(creature + 4) + center->x;
            center->y = *(float *)(creature + 8) + center->y;
            center->z = *(float *)(creature + 0xc) + center->z;
        }
        if (count > 0) {
            float scale = 1.0f / (float)count;

            center->x = scale * center->x;
            center->y = scale * center->y;
            center->z = scale * center->z;
        }
        memset((uint8_t *)self + 0x120, 0, 0x2a * 4);
        self->active_unit_index = -1;
        self->pathfinding_surface_index = -1;
        if (self->cluster_unit_index != -1) {
            halo::ai::actor_fill_unit_position_context((int32_t)self->cluster_unit_index, (actor_unit_position_context *)((uint8_t *)self + 0x120));
        }
        return;
    }

    unit = object_get((int32_t)self->unit_index);
    parent_index = ((unit_object *)unit)->base.parent_object;
    if (parent_index != k_datum_index_none) {
        parent = object_get(parent_index);
    }
    halo::ai::actor_fill_unit_position_context((int32_t)self->unit_index, (actor_unit_position_context *)((uint8_t *)self + 0x120));
    {
        object_marker marker;
        real_point3d head;

        halo::objects::object_get_node_local_transform((int32_t)self->unit_index, ai_marker_name_b, &marker, 1);
        head = marker.node_transform.position;
        self->in_water = halo::scenario::scenario_location_get_water_and_weather(&head, (bsp_leaf_reference *)((uint8_t *)self + 0x144), 0);
    }
    self->flying = (uint8_t)((*(uint32_t *)actor_tag >> 21) & 1);

    if (parent != 0 && *(int16_t *)(parent + 0xb4) == 1) {
        uint8_t *vehicle_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)parent & halo::k_slot_mask].data;
        uint32_t vehicle_flags;

        self->vehicle_gunner = 0;
        self->vehicle_gunner_bombards[0] = 0;
        self->vehicle_driving_type = 0;
        self->active_unit_index = parent_index;
        if (*(int32_t *)(parent + 0x324) == self->unit_index) {
            self->vehicle_driving_type = 1;
            vehicle_flags = *(uint32_t *)(vehicle_tag + 0x2f0);
            if (vehicle_flags & 0x800) {
                if (vehicle_flags & 0x1000) {
                    self->vehicle_driving_type = 4;
                    self->flying = 1;
                } else if (vehicle_flags & 0x2000) {
                    self->vehicle_driving_type = (int16_t)((~(vehicle_flags >> 14) & 1) | 2);
                }
            }
        }
        if (*(int32_t *)(parent + 0x328) == self->unit_index) {
            self->vehicle_gunner = 1;
            self->vehicle_gunner_bombards[0] = *(float *)((uint8_t *)halo::ai::actor_get_actor_definition(actor_index) + 0x14c) > 0.0f;
        }
        self->order_committed = self->vehicle_driving_type <= 1;
        if (*(int16_t *)(parent + 0x334) != -1) {
            datum_index encounter = (int32_t)self->encounter_index;
            int16_t wanted_encounter = *(int16_t *)(parent + 0x334);
            int16_t wanted_squad = *(int16_t *)(parent + 0x336);
            uint8_t move = 1;

            if ((encounter & halo::k_slot_mask) == (uint32_t)(int32_t)wanted_encounter) {
                if (wanted_squad == -1 || self->squad_index == wanted_squad) {
                    move = 0;
                } else {
                    struct encounter *encounter_record = halo::ai::encounter_at(encounter);

                    if (encounter_record->follow_target_type > 0) {
                        int16_t first = encounter_record->first_squad;
                        uint8_t *states = (uint8_t *)halo::ai::globals().squad_states;

                        if (states[(int16_t)(first + self->squad_index) * 0x20 + 0x10] != 0 &&
                            states[(int16_t)(first + wanted_squad) * 0x20 + 0x10] != 0) {
                            move = 0;
                        }
                    }
                }
            }
            if (move) {
                if (self->unknown_40[0] == 0) {
                    A_I32(0x44) = encounter;
                    A_I16(0x48) = self->squad_index;
                    self->unknown_40[0] = 1;
                    if (encounter != k_datum_index_none) {
                        *((uint8_t *)halo::ai::globals().encounter_data->data + (encounter & halo::k_slot_mask) * 0x6c + 0x1e) = 1;
                    }
                }
                halo::ai::actor_reset_squad_link_for_type_change(actor_index, wanted_encounter, wanted_squad);
            }
        }
    } else {
        self->active_unit_index = -1;
        self->vehicle_driving_type = 0;
        self->order_committed = 0;
        self->vehicle_gunner = 0;
        if (self->unknown_40[0]) {
            halo::ai::actor_reset_squad_link_for_type_change(actor_index, A_I32(0x44), A_I16(0x48));
            self->unknown_40[0] = 0;
        }
    }

    self->unknown_1b4[1] = unit[0x28b] > 0;
    self->unknown_1b4[0] = 0;
    self->stuck_projectile_index = -1;
    for (child = ((unit_object *)unit)->base.first_child_object; child != k_datum_index_none;
         child = *(datum_index *)(object_get(child) + 0x114)) {
        uint8_t *child_object = object_get(child);
        int16_t type = ((struct object *)child_object)->type;

        if (type == 0) {
            int16_t actor_team = self->team;
            int16_t child_team = ((struct object *)child_object)->owner_team;
            uint8_t enemy;

            if (halo::game::globals().current_engine != 0) {
                enemy = actor_team != child_team;
            } else if (actor_team < 0 || actor_team >= 10 || child_team < 0 || child_team >= 10) {
                enemy = 1;
            } else {
                int32_t bit = actor_team * 10 + child_team;

                enemy = (((uint32_t *)(team_pair_data + 0xa4))[bit >> 5] & (1u << (bit & 0x1f))) == 0;
            }
            if (enemy) {
                self->unknown_1b4[0] = 1;
            }
        } else if (type == 5) {
            if ((int8_t)child_object[0x22c] < 0 || (self->danger_type == 2 && child == self->danger_object_index)) {
                self->stuck_projectile_index = child;
            }
        }
    }

    self->airborne = 0;
    self->pathfinding_surface_index = -1;
    if (((unit_object *)unit)->base.type == 0 && self->active_unit_index == -1) {
        uint8_t *unit_object = object_get((int32_t)self->unit_index);

        if ((int8_t)unit_object[0x501] >= 6) {
            self->airborne = 1;
        }
        self->pathfinding_surface_index = *(int32_t *)(unit_object + 0x4dc);
        *(real_vector3d *)&self->pathfinding_point = *(real_vector3d *)(unit_object + 0x4e0);
    }
    halo::units::unit_get_forward_vector_or_marker_normal(self->vehicle_driving_type > 0 ? (int32_t)self->active_unit_index : (int32_t)self->unit_index,
        &self->facing);
    if (self->flying == 0) {
        if (halo::math::vector2d_normalize_with_length(*(real_vector2d *)((uint8_t *)self + 0x174)) > 0.0f) {
            self->facing.k = 0.0f;
        } else {
            *(real_vector3d *)&self->facing.i = *halo::math::globals().global_forward3d_pointer;
        }
    }
    if (self->vehicle_gunner) {
        uint8_t *vehicle = object_get((int32_t)self->active_unit_index);
        uint8_t *vehicle_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)vehicle & halo::k_slot_mask].data;

        if (*(uint32_t *)(vehicle_tag + 0x2f0) & 0x100) {
            halo::units::unit_get_forward_vector_or_marker_normal((int32_t)self->unit_index, &self->unit_aiming_vector);
        } else {
            *(real_vector3d *)&self->unit_aiming_vector.i = *(real_vector3d *)&((vehicle_object *)vehicle)->unit.aiming_vector.i;
        }
    } else {
        *(real_vector3d *)&self->unit_aiming_vector.i = *(real_vector3d *)&((unit_object *)unit)->unit.aiming_vector.i;
    }
    *(real_vector3d *)&self->unit_looking_vector.i = *(real_vector3d *)&((unit_object *)unit)->unit.looking_vector.i;
    halo::math::vector3d_cross_product(self->looking_left_vector, self->unit_looking_vector, *halo::math::globals().global_up3d_pointer);
    halo::math::vector3d_normalize_with_length(self->looking_left_vector);
    halo::math::vector3d_cross_product(self->looking_up_vector, self->looking_left_vector,
        self->unit_looking_vector);
    A_I32(0x1b8) = *(int32_t *)&((unit_object *)unit)->base.body_vitality;
    A_I32(0x1bc) = *(int32_t *)&((unit_object *)unit)->base.shield_vitality;
    A_I32(0x1c0) = *(int32_t *)&((unit_object *)unit)->base.recent_body_damage;
    A_I32(0x1c4) = *(int32_t *)&((unit_object *)unit)->base.recent_shield_damage;
}

#undef A_I16
#undef A_I32

namespace actor_reset_queued_look_vector_local {
extern "C" {
extern const real_vector3d *global_origin3d_pointer;
}
}

/**
 * Actor AI behaviour: reset queued look vector.
 *
 * @address 0x417ae0
 */
uint8_t ActorView::reset_queued_look_vector()
{
    using namespace actor_reset_queued_look_vector_local;
    actor *self;

    self = halo::ai::actor_at(actor_index);

    if (self->secondary_action != (int16_t)-1) {
        return 0;
    }
    if (self->unit_index != (datum_index)k_datum_index_none) {
        if (halo::units::unit_is_in_busy_animation_state(self->unit_index)) {
            return 0;
        }
    }
    if (halo::ai::actor_wants_reload_or_swap(actor_index)) {
        return 0;
    }

    self->moving = 0;
    self->throttle = *global_origin3d_pointer;
    self->control_animation_impulse = -1;
    return 1;
}

}
