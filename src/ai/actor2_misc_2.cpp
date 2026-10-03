#include "halo/ai/actor_view.hpp"
#include "halo/scenario/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"

namespace halo::ai {

namespace actor_reassign_vehicle_seat_local {
extern "C" {
extern data_array *object_data;
extern void *object_try_and_get(datum_index object_index, int32_t kind);
extern int8_t teams_are_enemies(int16_t team_a, int16_t team_b);
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern void ai_conversation_clear_object_references(datum_index object_index, uint8_t force_full_scan);
extern void encounters_note_hostile_object(datum_index object_index);
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
        vehicle_obj = (object *)object_try_and_get(vehicle_object_index, 3);
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
        object *occupant_obj = ((object_header *)object_data->data)[occupant & 0xffff].data;
        object *self_obj = ((object_header *)object_data->data)[self_object_index & 0xffff].data;
        reason = teams_are_enemies(((struct object *)occupant_obj)->owner_team,
                               ((struct object *)self_obj)->owner_team) ? 3 : 2;
    }

    ai_communication_broadcast(0, self_object_index, occupant, reason, seat_selector,
                                (datum_index)k_datum_index_none, 0);
    ai_conversation_clear_object_references(self_object_index, 0);
    encounters_note_hostile_object(self_object_index);
    return 0;
}

namespace actor_refresh_combat_context_local {
extern "C" {
extern data_array *actor_data;
extern data_array *swarm_data;
extern data_array *swarm_component_data;
extern data_array *encounter_data;
extern encounter_squad_state *encounter_squad_states;
extern data_array *object_data;
extern game_engine_definition *current_game_engine;
extern uint8_t *team_pair_data;
extern const real_point3d *global_zero_vector3d_pointer;
extern char ai_marker_name_b[];
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern void actor_fill_unit_position_context(datum_index unit_index, actor_unit_position_context *out_context);
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name, object_marker *marker,
    uint32_t flags);
extern uint8_t halo::scenario::scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf,
    int16_t *weather_index_out);
extern void *actor_get_actor_definition(datum_index actor_index);
extern void actor_reset_squad_link_for_type_change(datum_index actor_index, datum_index encounter_index,
    int16_t squad_index);
extern void unit_get_forward_vector_or_marker_normal(uint32_t unit_index, real_vector3d *out);
#define A_U8(offset) (*(uint8_t *)(self + (offset)))
#define A_I16(offset) (*(int16_t *)(self + (offset)))
#define A_I32(offset) (*(int32_t *)(self + (offset)))
static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
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
    uint8_t *self = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag = (uint8_t *)halo::cache::globals().tag_instances[((struct actor *)self)->actor_definition_tag & 0xffff].data;
    uint8_t *unit;
    uint8_t *parent = 0;
    datum_index parent_index;
    datum_index child;

    if (A_U8(0x06)) {
        uint8_t *swarm = (uint8_t *)swarm_data->data + (((struct actor *)self)->swarm_index & 0xffff) * 0x98;
        real_point3d *center = (real_point3d *)(swarm + 0xc);
        int16_t count = ((struct swarm *)swarm)->component_count;
        int16_t i;

        *center = *global_zero_vector3d_pointer;
        for (i = 0; i < count; i++) {
            uint8_t *creature = (uint8_t *)swarm_component_data->data +
                (*(datum_index *)(swarm + 0x58 + i * 4) & 0xffff) * 0x40;
            datum_index creature_unit = *(datum_index *)(swarm + 0x18 + i * 4);
            uint8_t *creature_object = object_get(creature_unit);
            datum_index vehicle = ((struct object *)creature_object)->type == 0 ?
                *(datum_index *)(creature_object + 0x4d8) : k_datum_index_none;

            object_get_position((real_point3d *)(creature + 4), creature_unit);
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
        memset(self + 0x120, 0, 0x2a * 4);
        A_I32(0x158) = -1;
        A_I32(0x164) = -1;
        if (A_I32(0x24) != -1) {
            actor_fill_unit_position_context(A_I32(0x24), (actor_unit_position_context *)(self + 0x120));
        }
        return;
    }

    unit = object_get(A_I32(0x18));
    parent_index = ((unit_object *)unit)->base.parent_object;
    if (parent_index != k_datum_index_none) {
        parent = object_get(parent_index);
    }
    actor_fill_unit_position_context(A_I32(0x18), (actor_unit_position_context *)(self + 0x120));
    {
        object_marker marker;
        real_point3d head;

        object_get_node_local_transform(A_I32(0x18), ai_marker_name_b, &marker, 1);
        head = marker.node_transform.position;
        A_U8(0x15d) = halo::scenario::scenario_location_get_water_and_weather(&head, (bsp_leaf_reference *)(self + 0x144), 0);
    }
    A_U8(0x99) = (uint8_t)((*(uint32_t *)actor_tag >> 21) & 1);

    if (parent != 0 && *(int16_t *)(parent + 0xb4) == 1) {
        uint8_t *vehicle_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)parent & 0xffff].data;
        uint32_t vehicle_flags;

        A_U8(0x161) = 0;
        A_U8(0x162) = 0;
        A_I16(0x15e) = 0;
        A_I32(0x158) = parent_index;
        if (*(int32_t *)(parent + 0x324) == A_I32(0x18)) {
            A_I16(0x15e) = 1;
            vehicle_flags = *(uint32_t *)(vehicle_tag + 0x2f0);
            if (vehicle_flags & 0x800) {
                if (vehicle_flags & 0x1000) {
                    A_I16(0x15e) = 4;
                    A_U8(0x99) = 1;
                } else if (vehicle_flags & 0x2000) {
                    A_I16(0x15e) = (int16_t)((~(vehicle_flags >> 14) & 1) | 2);
                }
            }
        }
        if (*(int32_t *)(parent + 0x328) == A_I32(0x18)) {
            A_U8(0x161) = 1;
            A_U8(0x162) = *(float *)((uint8_t *)actor_get_actor_definition(actor_index) + 0x14c) > 0.0f;
        }
        A_U8(0x160) = A_I16(0x15e) <= 1;
        if (*(int16_t *)(parent + 0x334) != -1) {
            datum_index encounter = A_I32(0x34);
            int16_t wanted_encounter = *(int16_t *)(parent + 0x334);
            int16_t wanted_squad = *(int16_t *)(parent + 0x336);
            uint8_t move = 1;

            if ((encounter & 0xffff) == (uint32_t)(int32_t)wanted_encounter) {
                if (wanted_squad == -1 || A_I16(0x3a) == wanted_squad) {
                    move = 0;
                } else {
                    uint8_t *encounter_record = (uint8_t *)encounter_data->data + (encounter & 0xffff) * 0x6c;

                    if (((struct encounter *)encounter_record)->follow_target_type > 0) {
                        int16_t first = ((struct encounter *)encounter_record)->first_squad;
                        uint8_t *states = (uint8_t *)encounter_squad_states;

                        if (states[(int16_t)(first + A_I16(0x3a)) * 0x20 + 0x10] != 0 &&
                            states[(int16_t)(first + wanted_squad) * 0x20 + 0x10] != 0) {
                            move = 0;
                        }
                    }
                }
            }
            if (move) {
                if (A_U8(0x40) == 0) {
                    A_I32(0x44) = encounter;
                    A_I16(0x48) = A_I16(0x3a);
                    A_U8(0x40) = 1;
                    if (encounter != k_datum_index_none) {
                        *((uint8_t *)encounter_data->data + (encounter & 0xffff) * 0x6c + 0x1e) = 1;
                    }
                }
                actor_reset_squad_link_for_type_change(actor_index, wanted_encounter, wanted_squad);
            }
        }
    } else {
        A_I32(0x158) = -1;
        A_I16(0x15e) = 0;
        A_U8(0x160) = 0;
        A_U8(0x161) = 0;
        if (A_U8(0x40)) {
            actor_reset_squad_link_for_type_change(actor_index, A_I32(0x44), A_I16(0x48));
            A_U8(0x40) = 0;
        }
    }

    A_U8(0x1b5) = unit[0x28b] > 0;
    A_U8(0x1b4) = 0;
    A_I32(0x1b0) = -1;
    for (child = ((unit_object *)unit)->base.first_child_object; child != k_datum_index_none;
         child = *(datum_index *)(object_get(child) + 0x114)) {
        uint8_t *child_object = object_get(child);
        int16_t type = ((struct object *)child_object)->type;

        if (type == 0) {
            int16_t actor_team = A_I16(0x3e);
            int16_t child_team = ((struct object *)child_object)->owner_team;
            uint8_t enemy;

            if (current_game_engine != 0) {
                enemy = actor_team != child_team;
            } else if (actor_team < 0 || actor_team >= 10 || child_team < 0 || child_team >= 10) {
                enemy = 1;
            } else {
                int32_t bit = actor_team * 10 + child_team;

                enemy = (((uint32_t *)(team_pair_data + 0xa4))[bit >> 5] & (1u << (bit & 0x1f))) == 0;
            }
            if (enemy) {
                A_U8(0x1b4) = 1;
            }
        } else if (type == 5) {
            if ((int8_t)child_object[0x22c] < 0 || (A_I16(0x280) == 2 && child == A_I32(0x28c))) {
                A_I32(0x1b0) = child;
            }
        }
    }

    A_U8(0x15c) = 0;
    A_I32(0x164) = -1;
    if (((unit_object *)unit)->base.type == 0 && A_I32(0x158) == -1) {
        uint8_t *unit_object = object_get(A_I32(0x18));

        if ((int8_t)unit_object[0x501] >= 6) {
            A_U8(0x15c) = 1;
        }
        A_I32(0x164) = *(int32_t *)(unit_object + 0x4dc);
        *(real_vector3d *)&((struct actor *)self)->pathfinding_point = *(real_vector3d *)(unit_object + 0x4e0);
    }
    unit_get_forward_vector_or_marker_normal(A_I16(0x15e) > 0 ? A_I32(0x158) : A_I32(0x18),
        (real_vector3d *)(self + 0x174));
    if (A_U8(0x99) == 0) {
        if (halo::math::vector2d_normalize_with_length(*(real_vector2d *)(self + 0x174)) > 0.0f) {
            ((struct actor *)self)->facing.k = 0.0f;
        } else {
            *(real_vector3d *)&((struct actor *)self)->facing.i = *halo::math::globals().global_forward3d_pointer;
        }
    }
    if (A_U8(0x161)) {
        uint8_t *vehicle = object_get(A_I32(0x158));
        uint8_t *vehicle_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)vehicle & 0xffff].data;

        if (*(uint32_t *)(vehicle_tag + 0x2f0) & 0x100) {
            unit_get_forward_vector_or_marker_normal(A_I32(0x18), (real_vector3d *)(self + 0x180));
        } else {
            *(real_vector3d *)&((struct actor *)self)->facing_unknown_180.i = *(real_vector3d *)&((vehicle_object *)vehicle)->unit.aiming_vector.i;
        }
    } else {
        *(real_vector3d *)&((struct actor *)self)->facing_unknown_180.i = *(real_vector3d *)&((unit_object *)unit)->unit.aiming_vector.i;
    }
    *(real_vector3d *)&((struct actor *)self)->facing_unknown_18c.i = *(real_vector3d *)&((unit_object *)unit)->unit.looking_vector.i;
    halo::math::vector3d_cross_product(*(real_vector3d *)(self + 0x198), *(real_vector3d *)(self + 0x18c), *halo::math::globals().global_up3d_pointer);
    halo::math::vector3d_normalize_with_length(*(real_vector3d *)(self + 0x198));
    halo::math::vector3d_cross_product(*(real_vector3d *)(self + 0x1a4), *(real_vector3d *)(self + 0x198),
        *(real_vector3d *)(self + 0x18c));
    A_I32(0x1b8) = *(int32_t *)&((unit_object *)unit)->base.body_vitality;
    A_I32(0x1bc) = *(int32_t *)&((unit_object *)unit)->base.shield_vitality;
    A_I32(0x1c0) = *(int32_t *)&((unit_object *)unit)->base.recent_body_damage;
    A_I32(0x1c4) = *(int32_t *)&((unit_object *)unit)->base.recent_shield_damage;
}

#undef A_U8
#undef A_I16
#undef A_I32

namespace actor_reset_queued_look_vector_local {
extern "C" {
extern data_array *actor_data;
extern const real_vector3d *global_origin3d_pointer;
extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index);
extern uint8_t actor_wants_reload_or_swap(datum_index actor_index);
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

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->secondary_action != (int16_t)-1) {
        return 0;
    }
    if (self->unit_index != (datum_index)k_datum_index_none) {
        if (unit_is_in_busy_animation_state(self->unit_index)) {
            return 0;
        }
    }
    if (actor_wants_reload_or_swap(actor_index)) {
        return 0;
    }

    self->moving = 0;
    self->throttle = *global_origin3d_pointer;
    self->control_animation_impulse = -1;
    return 1;
}

}
