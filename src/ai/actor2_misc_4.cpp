#include "halo/ai/actor_view.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"

namespace halo::ai {

namespace actor_seek_vehicle_to_board_local {
extern "C" {
extern game_time_globals *game_time;
#define ACTOR(h) ((uint8_t *)halo::ai::globals().actor_data->data + ((h) & halo::k_slot_mask) * k_actor_size)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
#define PROP(h) ((uint8_t *)halo::ai::globals().prop_data->data + ((h) & halo::k_slot_mask) * k_prop_size)
extern uint8_t *ai_globals_ptr;
}
}

/**
 * Actor AI behaviour: seek vehicle to board.
 *
 * @address 0x40ac70
 */
uint8_t ActorView::seek_vehicle_to_board()
{
    using namespace actor_seek_vehicle_to_board_local;
    actor *act = halo::ai::actor_at(actor_index);
    uint8_t *actor_tag = TAG_DATA(act->actor_definition_tag);
    int32_t now = game_time->game_time;
    int16_t mode = act->mode;
    float best_distance = 3.4028235e38f;
    float radius_a = 3.4028235e38f;
    float radius_b = 3.4028235e38f;
    datum_index best_vehicle = k_datum_index_none;
    uint8_t order[k_actor_mode_data_size];
    real_point3d position;

    if ((mode == 4 && act->mode_data.flee.panic > 0) || mode == 11) {
        return 0;
    }
    if (*(int32_t *)&act->last_vehicle_search_time != -1 && *(int32_t *)&act->last_vehicle_search_time + 45 >= now) {
        return 0;
    }
    *(int32_t *)&act->last_vehicle_search_time = now;
    if (*(uint32_t *)actor_tag & 0x1000) {
        datum_index prop_index = act->first_prop;

        while (prop_index != k_datum_index_none) {
            prop *p = halo::ai::prop_at(prop_index);
            int16_t kind = p->state;
            datum_index vehicle = (uint32_t)p->relationship_object_index;
            uint8_t *vehicle_object;
            float distance_squared;

            prop_index = p->next_in_actor;
            if (kind < 2 || kind > 3 || !p->is_parented || p->enemy || vehicle == k_datum_index_none ||
                !halo::ai::actor_vehicle_not_recently_left(actor_index, vehicle)) {
                continue;
            }
            vehicle_object = (uint8_t *)halo::objects::object_try_and_get(vehicle, 2);
            if (vehicle_object == 0 || *(datum_index *)(vehicle_object + 0x324) != p->object_index) {
                continue;
            }
            halo::objects::object_get_position(&position, vehicle);
            distance_squared = halo::math::vector3d_distance_squared(position, act->body_position);
            if (distance_squared < 100.0f && distance_squared < best_distance) {
                float distance = p->distance;

                best_vehicle = (uint32_t)p->relationship_object_index;
                radius_a = 8.0f;
                radius_b = 10.0f;
                best_distance = distance * distance;
            }
        }
    }
    if (best_vehicle == k_datum_index_none) {
        int16_t i;

        if (act->ticks_threatened < 60) {
            return 0;
        }
        for (i = 0; i < *(int16_t *)(ai_globals_ptr + 0x3b6); i++) {
            uint8_t *offer = ai_globals_ptr + 0x3b8 + i * 0x28;
            datum_index vehicle = *(datum_index *)(offer + 0x0);
            float radius = *(float *)(offer + 0x4);
            int16_t team_mask = *(int16_t *)(offer + 0x8);
            int16_t type_mask = *(int16_t *)(offer + 0xa);
            int16_t filter_count = *(int16_t *)(offer + 0xc);
            float dx;
            float dy;
            float dz;
            float distance_squared;

            if (halo::objects::object_try_and_get(vehicle, 2) == 0 || !halo::ai::actor_vehicle_not_recently_left(actor_index, vehicle)) {
                continue;
            }
            halo::objects::object_get_position(&position, vehicle);
            dx = act->body_position.x - position.x;
            dy = act->body_position.y - position.y;
            dz = act->body_position.z - position.z;
            distance_squared = dz * dz + dx * dx + dy * dy;
            if (!(distance_squared < best_distance)) {
                continue;
            }
            if (*(uint32_t *)(offer + 0x4) != 0x7f7fffff && distance_squared > radius * radius) {
                continue;
            }
            if (team_mask > 0) {
                int16_t team = act->team;

                if (team == -1 || (team_mask & (1 << team)) == 0) {
                    continue;
                }
            }
            if (type_mask > 0 && (type_mask & (1 << (uint8_t)act->type)) == 0) {
                continue;
            }
            if (filter_count > 0) {
                uint8_t match = 0;
                int16_t j;

                for (j = 0; j < filter_count; j++) {
                    uint32_t filter = *(uint32_t *)(offer + 0x10 + j * 4);

                    if (filter == halo::k_dword_none) {
                        continue;
                    }
                    match = (uint8_t)(((*(uint32_t *)&act->encounter_index ^ filter) & halo::k_slot_mask) == 0);
                    if (!match) {
                        continue;
                    }
                    switch (filter >> 30) {
                    case 1:
                        match = (uint8_t)(act->platoon_index == (int16_t)(uint8_t)(filter >> 16));
                        break;
                    case 2:
                        match = (uint8_t)(act->squad_index == (int16_t)(uint8_t)(filter >> 16));
                        break;
                    default:
                        break;
                    }
                    if (match) {
                        break;
                    }
                }
                if (!match) {
                    continue;
                }
            }
            radius_a = radius + 3.0f;
            best_vehicle = vehicle;
            best_distance = distance_squared;
            radius_b = radius + 6.0f;
        }
        if (best_vehicle == k_datum_index_none) {
            return 0;
        }
    }
    if (halo::ai::actor_build_order_search_object(best_vehicle, actor_index, radius_a, radius_b, order)) {
        halo::ai::actor_set_mode(actor_index, 9, order);
        return 1;
    }
    return 0;
}

#undef ACTOR
#undef TAG_DATA
#undef PROP

}
