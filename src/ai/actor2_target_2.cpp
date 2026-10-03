#include "halo/ai/actor_view.hpp"
#include "halo/physics/api.hpp"

namespace halo::ai {

namespace actor_target_evaluate_squad_link_local {
extern "C" {
extern data_array *actor_data;
extern data_array *object_data;
extern data_array *encounter_data;
extern tag_instance *tag_instances;
extern game_time_globals *game_time;
extern double sqrt(double x);
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point);
extern datum_index object_find_nearest_squad_member(datum_index actor_index, void *reference, datum_index exclude_index,
    char stamp_group);
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b);
extern uint8_t actor_danger_register_point(datum_index actor_index, datum_index source_object_index, float radius,
    float distance, char accept_flag, uint8_t unknown_byte);
extern uint8_t actor_danger_register_stationary_object(const float *reference, datum_index actor_index,
    datum_index object_index, uint8_t unknown_byte);
extern int16_t actor_get_current_mode_combat_grade(datum_index actor_index);
extern datum_index actor_find_or_allocate_prop(datum_index actor_index, uint32_t object_index, char kind);
extern void actor_target_data_refresh(uint32_t actor_index, uint32_t target_prop_index, void *reference, char force,
    char allow_reassign);
#define OBJ(i) ((uint8_t *)((object_header *)object_data->data)[(i) & 0xffff].data)
static void squad_link_add_far(uint8_t *list, datum_index object_index, float distance_squared)
{
    int16_t count = *(int16_t *)(list + 2);
    uint8_t *entry;

    if (count >= 0x80) {
        return;
    }
    entry = list + 4 + count * 0xc;
    *(datum_index *)(entry + 0) = object_index;
    *(int32_t *)(entry + 4) = -1;
    *(float *)(entry + 8) = distance_squared;
    *(int16_t *)(list + 2) = (int16_t)(count + 1);
}
static void squad_link_evaluate_biped(uint32_t actor_index, uint8_t *self, datum_index object_index, uint8_t *object,
    uint8_t *list_enemy, uint8_t *list_friend)
{
    real_point3d position;
    uint32_t block[14];
    real_point3d *block_point = (real_point3d *)&block[3];
    datum_index target = object_index;
    datum_index target_actor_index;
    uint8_t *unit = object;
    uint8_t *unit_tag;
    uint8_t *target_actor = 0;
    uint8_t controlled;
    uint8_t enemies;
    uint8_t firing;
    uint8_t far_flag = 0;
    int16_t since_fired;
    float radius;
    float distance_squared;
    uint8_t *list;

    object_get_position(&position, object_index);
    actor_get_firing_positions(actor_index, block, &position);
    if (*(datum_index *)(object + 0x1f8) != k_datum_index_none) {
        target_actor_index = *(datum_index *)(object + 0x1f8);
        target = object_find_nearest_squad_member(target_actor_index, block, k_datum_index_none, 1);
        if (target == k_datum_index_none) {
            return;
        }
        unit = OBJ(target);
        object_get_position(&position, target);
    } else {
        target_actor_index = *(datum_index *)(object + 0x1f4);
    }
    if (target_actor_index == actor_index) {
        return;
    }

    unit_tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data;
    controlled = ((unit_object *)unit)->unit.controlling_player != k_datum_index_none;
    enemies = teams_are_enemies(((unit_object *)unit)->base.owner_team, ((actor *)self)->team);
    if ((unit[0x106] & 4) != 0 && ((struct unit_object *)unit)->unit.feign_death_ticks == 0) {
        int32_t fired = ((struct unit_object *)unit)->unit.death_time;

        firing = 1;
        since_fired = fired == -1 ? 0x7fff : (int16_t)((int16_t)game_time->game_time - (int16_t)fired);
    } else {
        firing = 0;
        since_fired = 0;
    }
    radius = *(float *)(unit_tag + 0x284);
    {
        float dx = position.x - block_point->x;
        float dy = position.y - block_point->y;
        float dz = position.z - block_point->z;

        distance_squared = dz * dz + dy * dy + dx * dx;
    }
    if (radius > 0.0f && (firing || (int8_t)unit[0x2a3] == 0x1e)) {
        actor_danger_register_point(actor_index, target, radius, (float)sqrt((double)distance_squared), (char)enemies, 0);
    }
    if (target_actor_index != k_datum_index_none) {
        target_actor = (uint8_t *)actor_data->data + (target_actor_index & 0xffff) * 0x724;
    }

    if (!controlled) {
        if (target_actor != 0 && (target_actor[8] == 0 || target_actor[0x13] != 0)) {
            return;
        }
        if (distance_squared > 1600.0f) {
            return;
        }
        if (firing) {
            datum_index encounter_index = ((actor *)self)->encounter_index;

            if (encounter_index == k_datum_index_none) {
                goto check_radius;
            } else {
                uint8_t *encounter = (uint8_t *)encounter_data->data + (encounter_index & 0xffff) * 0x6c;
                uint8_t *target_unit = OBJ(target);
                int32_t reference = ((struct encounter *)encounter)->last_idle_time;
                uint8_t counts = 1;
                uint8_t calm;

                if (!(reference > *(int32_t *)&((struct actor *)self)->found_body_time)) {
                    reference = *(int32_t *)&((struct actor *)self)->found_body_time;
                }
                if (reference != -1) {
                    int32_t fired = *(int32_t *)(target_unit + 0x41c);

                    if (fired == -1 || fired < reference) {
                        counts = 0;
                    }
                }
                calm = encounter[0x45] == 0 && encounter[0x44] == 0 && encounter[0x42] == 0;
                if (!counts) {
                    return;
                }
                if (!calm) {
                    goto check_radius;
                }
                if (!(distance_squared < 225.0f)) {
                    return;
                }
            }
        } else if (enemies) {
            far_flag = distance_squared > 36.0f;
        } else {
            uint8_t near = distance_squared < 225.0f;

            if (((struct actor *)self)->combat_status >= 4) {
                far_flag = 1;
            } else {
                far_flag = self[0x1cc] == 0 && distance_squared > 16.0f;
            }
            if (!near) {
                return;
            }
            list = list_friend;
            goto add;
        }
        goto add_by_team;

check_radius:
        if (!(radius > 0.0f)) {
            float limit;

            if (enemies && since_fired > 0x96) {
                return;
            }
            if (actor_get_current_mode_combat_grade(actor_index) > 1) {
                return;
            }
            limit = 16.0f;
            if (!enemies && ((actor *)self)->awareness_level < 3) {
                limit = 64.0f;
            }
            if (!(distance_squared < limit)) {
                return;
            }
        }
    }

add_by_team:
    list = enemies ? list_enemy : list_friend;
add:
    if (far_flag) {
        squad_link_add_far(list, target, distance_squared);
        return;
    }
    {
        datum_index prop_index = actor_find_or_allocate_prop(actor_index, target, (char)enemies);

        if (prop_index == k_datum_index_none) {
            return;
        }
        actor_target_data_refresh(actor_index, prop_index, block, 0, 0);
        if (!firing) {
            *(int16_t *)list = (int16_t)(*(int16_t *)list + 1);
        }
    }
}
static void squad_link_evaluate_projectile(uint32_t actor_index, uint8_t *self, datum_index object_index, uint8_t *object)
{
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
    float radius = *(float *)(tag + 0x1a8);
    real_point3d position;
    uint32_t block[14];
    real_point3d *block_point = (real_point3d *)&block[3];
    float distance;
    datum_index owner;
    datum_index owner_unit = k_datum_index_none;

    if (!(radius > 0.0f)) {
        return;
    }
    if (((struct object *)object)->parent_object != k_datum_index_none && (object[0x22c] & 0x20) == 0) {
        return;
    }
    object_get_position(&position, object_index);
    actor_get_firing_positions(actor_index, block, &position);
    {
        float dx = position.x - block_point->x;
        float dy = position.y - block_point->y;
        float dz = position.z - block_point->z;

        distance = (float)sqrt((double)(dz * dz + dy * dy + dx * dx));
    }
    if (!(radius + 10.0f > distance)) {
        return;
    }
    if (((actor *)self)->danger_type >= 2) {
        if (((actor *)self)->danger_type != 2 || ((actor *)self)->danger_object_index == object_index ||
            !(distance < ((actor *)self)->danger_unknown_2d4)) {
            return;
        }
    }
    memset(self + 0x280, 0, 0x6c);
    ((actor *)self)->danger_type = 2;
    ((actor *)self)->danger_object_index = object_index;
    ((actor *)self)->danger_unknown_294 = radius;
    *(real_point3d *)&((actor *)self)->danger_unknown_298 = position;
    *(real_vector3d *)&((actor *)self)->danger_unknown_2a4 = *(real_vector3d *)&((struct object *)object)->velocity.i;
    ((actor *)self)->danger_unknown_284 = 0x1e;
    self[0x286] = 0;
    ((actor *)self)->danger_unknown_282 = 0;
    owner = ((struct object *)object)->creator_object;
    if (owner != k_datum_index_none) {
        uint8_t *owner_object = (uint8_t *)object_try_and_get(owner, 0xffffffff);

        if (owner_object != 0 && ((1u << owner_object[0xb4]) & 3) != 0) {
            owner_unit = owner;
            if (((actor *)self)->unit_index != k_datum_index_none && owner == ((actor *)self)->unit_index) {
                ((actor *)self)->danger_unknown_282 = 2;
            } else if (!teams_are_enemies(((struct object *)object)->owner_team, ((actor *)self)->team)) {
                ((actor *)self)->danger_unknown_282 = 1;
            }
        }
    }
    *(datum_index *)&((actor *)self)->danger_unknown_290 = owner_unit;
}
extern void actor_target_evaluate_squad_link(uint32_t actor_index, datum_index object_index, int16_t *candidates_a, int16_t *candidates_b);
}
}

/**
 * Actor AI behaviour: target evaluate squad link.
 *
 * @address 0x41e320
 */
void ActorView::target_evaluate_squad_link(datum_index object_index, int16_t *candidates_a, int16_t *candidates_b)
{
    using namespace actor_target_evaluate_squad_link_local;
    uint8_t *self = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    while (object_index != k_datum_index_none) {
        uint8_t *object = OBJ(object_index);

        if (((struct object *)object)->cluster_stamp != halo::physics::globals().object_cluster_stamp) {
            ((struct object *)object)->cluster_stamp = halo::physics::globals().object_cluster_stamp;
            switch (((struct object *)object)->type) {
            case 0:
                squad_link_evaluate_biped(actor_index, self, object_index, object, (uint8_t *)candidates_a,
                    (uint8_t *)candidates_b);
                break;
            case 1:
                if (*(datum_index *)(object + 0x324) == k_datum_index_none) {
                    actor_danger_register_stationary_object(0, actor_index, object_index, 0);
                }
                break;
            case 5:
                squad_link_evaluate_projectile(actor_index, self, object_index, object);
                break;
            default:
                break;
            }
        }
        if (((struct object *)object)->first_child_object != k_datum_index_none) {
            actor_target_evaluate_squad_link(actor_index, ((struct object *)object)->first_child_object, candidates_a, candidates_b);
        }
        object_index = ((struct object *)object)->next_object;
    }
}

#undef OBJ

namespace actor_target_is_close_and_recognized_local {
extern "C" {
extern data_array *prop_data;
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag);
}
}

/**
 * Resolves actor_index to its prop record and returns 1 if it is within 5 world units and its unknown_38 field
 * is 0 or 1, else 0.
 *
 * @address 0x42f480
 */
uint8_t ActorOps::target_is_close_and_recognized(datum_index object_index, uint32_t param_2, datum_index actor_index)
{
    using namespace actor_target_is_close_and_recognized_local;
    datum_index prop_index;
    prop *p;

    if (actor_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    prop_index = actor_find_or_create_shared_prop(object_index, actor_index, 1, 1);
    if (prop_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    p = &((prop *)prop_data->data)[prop_index & 0xffff];
    if (p->distance < 5.0f && (p->obstruction == 0 || p->obstruction == 1)) {
        return 1;
    }
    return 0;
}

}
