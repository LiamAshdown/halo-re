#include "halo/ai/actor_props.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"

namespace c_actor_danger_update_reaction {
extern "C" {
extern data_array *actor_data;
extern data_array *prop_data;
extern data_array *encounter_data;
extern data_array *object_data;

extern double sqrt(double x);
extern int32_t fistp_round(float x);

extern void *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern uint32_t object_get_root_object_index(uint32_t object_index);
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point);
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index);
extern int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster,
    real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask,
    datum_index exclude_object_index, uint8_t flying);
extern int16_t actor_dispatch_look_handler_by_posture(int16_t posture, uint32_t actor_index, void *origin, void *target,
    uint8_t stance_a, uint8_t check_facing, uint16_t range_class);
extern uint16_t actor_target_hearing_check(void *record, int16_t stance, datum_index actor_index,
    void *target_ref, int16_t gate, real_point3d *listener_position);
extern int32_t unit_get_animation_frames_remaining(uint32_t unit_index, int16_t *out_animation_state);
extern uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant, void *context);

#define A_W(o) (*(int16_t *)(actor + (o)))
#define A_D(o) (*(uint32_t *)(actor + (o)))
#define A_F(o) (*(float *)(actor + (o)))

static uint8_t actor_danger_prop_seen_twice(datum_index actor_index, datum_index object_index)
{
    datum_index prop_index = actor_find_prop_for_object(object_index, actor_index);

    if (prop_index == k_datum_index_none) {
        return 0xff;
    }
    return *(int16_t *)((uint8_t *)prop_data->data + (prop_index & halo::k_slot_mask) * k_prop_size + 0x30) >= 2;
}

static uint8_t actor_danger_asleep(uint8_t *actor)
{
    uint8_t *encounter = 0;

    if (A_D(0x34) != halo::k_dword_none) {
        encounter = (uint8_t *)encounter_data->data + (A_D(0x34) & halo::k_slot_mask) * k_encounter_size;
    }
    return A_W(0x6a) == 1 || (encounter != 0 && encounter[0x40] != 0);
}

static uint8_t actor_danger_stance(datum_index actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;

    return A_W(0x6e) >= 2 ? 2 : (A_W(0x6a) >= 3);
}
}
}

extern "C" void actor_danger_update_reaction(datum_index actor_index);

/**
 * actor_danger_update_reaction: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_danger_update_reaction.c.txt.
 *
 * @address 0x41eda0
 */
void halo::ai::prop_ops::danger_update_reaction()
{
    using namespace c_actor_danger_update_reaction;
    datum_index actor_index = datum;
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    uint8_t *object;
    uint32_t block[14];
    real_point3d *position = &((struct actor *)actor)->flee_from_point;
    real_point3d *block_point = (real_point3d *)&block[3];
    uint8_t noticed = 0;
    uint8_t own = 0;

    if (A_W(0x280) <= 0) {
        return;
    }
    object = (uint8_t *)object_try_and_get(A_D(0x28c), halo::k_dword_none);
    if (object == 0) {
        A_W(0x280) = 0;
        return;
    }
    object_get_position(position, A_D(0x28c));
    actor_get_firing_positions(actor_index, block, position);
    *(real_vector3d *)(actor + 0x2bc) = *(real_vector3d *)&((struct object *)object)->velocity.i;
    {
        float dx = position->x - block_point->x;
        float dy = position->y - block_point->y;
        float dz = position->z - block_point->z;

        A_F(0x2d4) = (float)sqrt((double)(dz * dz + dy * dy + dx * dx));
    }
    A_F(0x2c8) = A_F(0x2bc) * 45.0f + position->x;
    A_F(0x2cc) = A_F(0x2c0) * 45.0f + position->y;
    A_F(0x2d0) = A_F(0x2c4) * 45.0f + position->z;
    A_F(0x2dc) = (A_F(0x2c8) + position->x) * 0.5f;
    A_F(0x2e0) = (position->y + A_F(0x2cc)) * 0.5f;
    A_F(0x2e4) = (position->z + A_F(0x2d0)) * 0.5f;
    {
        float dx = position->x - A_F(0x2dc);
        float dy = position->y - A_F(0x2e0);
        float dz = position->z - A_F(0x2e4);

        A_F(0x2d8) = (float)sqrt((double)(dz * dz + dy * dy + dx * dx)) + A_F(0x294);
    }

    switch (A_W(0x280)) {
    case 1: {
        int16_t state = 0;
        int32_t frames;

        noticed = actor[0x286];
        if (!noticed) {
            uint8_t seen = actor_danger_prop_seen_twice(actor_index, A_D(0x28c));

            if (seen != 0xff) {
                noticed = seen;
            }
        }
        frames = unit_get_animation_frames_remaining(A_D(0x28c), &state);
        A_W(0x2e8) = state == 0x19 ? (int16_t)frames : -1;
        break;
    }
    case 2: {
        uint8_t *tag;
        int16_t cluster;
        int16_t status;

        if (A_D(0x18) != halo::k_dword_none && *(uint32_t *)&((struct object *)object)->parent_object == A_D(0x18)) {
            own = 1;
        }
        if (*(float *)(object + 0x240) > 0.0f && *(float *)(object + 0x244) > 0.0f) {
            A_W(0x2e8) = (int16_t)fistp_round((1.0f - *(float *)(object + 0x240)) / *(float *)(object + 0x244));
        } else {
            A_W(0x2e8) = -1;
        }
        if (actor[0x286] != 0 || own) {
            noticed = 1;
            break;
        }
        if (actor_danger_asleep(actor)) {
            break;
        }
        tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)object & halo::k_slot_mask].data;
        if (!(A_F(0x2d4) < *(float *)(tag + 0x19c))) {
            break;
        }
        cluster = ((struct object *)object)->location_cluster_index;
        if (*(uint32_t *)&((struct object *)object)->parent_object != halo::k_dword_none) {
            uint8_t *root = (uint8_t *)((object_header *)object_data->data)[object_get_root_object_index(A_D(0x28c)) & halo::k_slot_mask].data;

            cluster = ((struct object *)root)->location_cluster_index;
        }
        status = (int16_t)actor_evaluate_engagement_reachability(*(int16_t *)((uint8_t *)block + 0x28), cluster,
            position, (real_point3d *)block, 0, 0, A_D(0x28c), A_D(0x158) != halo::k_dword_none);
        actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
        if (actor_dispatch_look_handler_by_posture(status, actor_index, block, position, 0, 1,
                actor_danger_stance(actor_index)) >= 2) {
            noticed = 1;
        }
        break;
    }
    case 3: {
        uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)object & halo::k_slot_mask].data;
        float vi = ((struct object *)object)->velocity.i;
        float vj = ((struct object *)object)->velocity.j;
        float vk = ((struct object *)object)->velocity.k;
        uint8_t asleep;
        uint8_t *location;
        int16_t status;

        if (vi * vi + vj * vj + vk * vk < 4.4444445e-05f || ((struct Object *)tag)->bounding_radius + 10.0f < A_F(0x2d4)) {
            A_W(0x280) = 0;
            break;
        }
        noticed = actor[0x286];
        if (noticed) {
            break;
        }
        if (*(uint32_t *)(object + 0x324) != halo::k_dword_none) {
            uint8_t seen = actor_danger_prop_seen_twice(actor_index, *(uint32_t *)(object + 0x324));

            if (seen != 0xff) {
                noticed = seen;
                break;
            }
        }
        asleep = actor_danger_asleep(actor);
        location = object + 0x98;
        if (*(uint32_t *)&((struct object *)object)->parent_object != halo::k_dword_none) {
            location = (uint8_t *)((object_header *)object_data->data)[object_get_root_object_index(A_D(0x28c)) & halo::k_slot_mask].data + 0x98;
        }
        status = (int16_t)actor_evaluate_engagement_reachability(*(int16_t *)((uint8_t *)block + 0x28),
            *(int16_t *)(location + 4), position, (real_point3d *)block, 0, 0, A_D(0x28c), A_D(0x158) != halo::k_dword_none);
        if (!asleep &&
            actor_dispatch_look_handler_by_posture(status, actor_index, block, position, 0, 1,
                actor_danger_stance(actor_index)) >= 2) {
            noticed = 1;
            break;
        }
        if ((int16_t)actor_target_hearing_check(location, status, actor_index, block, *(int16_t *)(tag + 0x182),
                position) >= 2) {
            noticed = 1;
        }
        break;
    }
    default:
        break;
    }

    actor = (uint8_t *)actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    if (noticed && actor[0x286] == 0) {
        uint8_t payload[0x10];

        memset(payload, 0, sizeof(payload));
        *(int16_t *)payload = 5;
        actor_begin_vocalization(actor_index, 0xc, 1, payload);
    }
    actor[0x286] = noticed;
    actor[0x28a] = own;
}

extern "C" void actor_danger_update_reaction(datum_index actor_index)
{
    halo::ai::prop_ops(actor_index).danger_update_reaction();
}

#undef A_D
#undef A_F
#undef A_W

