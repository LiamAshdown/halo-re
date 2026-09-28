// actor_seek_vehicle_to_board  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x40ac70, size 1029 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x40ac70..0x40b075.
//   At most every 45 ticks (+0x384), an actor not panicking (flee kind +0xa8) and not obeying looks for a vehicle to
//   board (mode 9 via 0x408920): with Actor flag 0x1000 the vehicle a nearby friendly driver (prop kinds 2..3 riding
//   +0x110 as its driver, within 10) is in, nearest first (radii 8 / 10); failing that, after 60 ticks alive (+0x84),
//   the scripted vehicle offers in ai_globals (+0x3b6 / +0x3b8, 0x28 each: vehicle, radius, team mask, actor-type
//   mask, up to n encounter / squad / platoon filters) within their radius (radii radius + 3 / + 6). The vehicle
//   last left is skipped until +0x394 (0x40ac30). Returns 1 when a boarding order was set.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *prop_data; // 0x008802c0
extern game_time_globals *game_time; // 0x006f1d6c

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

extern uint8_t *ai_globals_ptr; // 0x00880354
extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, EAX, ECX
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX, ECX
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern uint8_t actor_vehicle_not_recently_left(datum_index actor_index, datum_index vehicle_index); // 0x40ac30, EAX, stack
extern uint8_t actor_build_order_search_object(uint32_t vehicle_index, uint32_t actor_index, float radius_a,
                                               float radius_b, uint8_t *order); // 0x408920, EBX, stack
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0

uint8_t actor_seek_vehicle_to_board(datum_index actor_index)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    int32_t now = game_time->game_time;
    int16_t mode = ((actor *)act)->mode;
    float best_distance = 3.4028235e38f;
    float radius_a = 3.4028235e38f;
    float radius_b = 3.4028235e38f;
    datum_index best_vehicle = k_datum_index_none;
    uint8_t order[k_actor_mode_data_size];
    real_point3d position;

    if ((mode == 4 && ((struct actor *)act)->mode_data.flee.panic > 0) || mode == 11) {
        return 0;
    }
    if (*(int32_t *)&((struct actor *)act)->unknown_384 != -1 && *(int32_t *)&((struct actor *)act)->unknown_384 + 45 >= now) {
        return 0;
    }
    *(int32_t *)&((struct actor *)act)->unknown_384 = now;
    if (*(uint32_t *)actor_tag & 0x1000) {
        datum_index prop_index = ((actor *)act)->first_prop;

        while (prop_index != k_datum_index_none) {
            uint8_t *p = PROP(prop_index);
            int16_t kind = *(int16_t *)(p + 0x24);
            datum_index vehicle = *(datum_index *)(p + 0x110);
            uint8_t *vehicle_object;
            float distance_squared;

            prop_index = *(datum_index *)(p + 0x8);
            if (kind < 2 || kind > 3 || !p[0x12e] || p[0x60] || vehicle == k_datum_index_none ||
                !actor_vehicle_not_recently_left(actor_index, vehicle)) {
                continue;
            }
            vehicle_object = (uint8_t *)object_try_and_get(vehicle, 2);
            if (vehicle_object == 0 || *(datum_index *)(vehicle_object + 0x324) != *(datum_index *)(p + 0x18)) {
                continue;
            }
            object_get_position(&position, vehicle);
            distance_squared = vector3d_distance_squared(&position, (real_point3d *)(act + 0x12c));
            if (distance_squared < 100.0f && distance_squared < best_distance) {
                float distance = *(float *)(p + 0x11c);

                best_vehicle = *(datum_index *)(p + 0x110);
                radius_a = 8.0f;
                radius_b = 10.0f;
                best_distance = distance * distance;
            }
        }
    }
    if (best_vehicle == k_datum_index_none) {
        int16_t i;

        if (((struct actor *)act)->unknown_84 < 60) {
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

            if (object_try_and_get(vehicle, 2) == 0 || !actor_vehicle_not_recently_left(actor_index, vehicle)) {
                continue;
            }
            object_get_position(&position, vehicle);
            dx = ((actor *)act)->body_position.x - position.x;
            dy = ((actor *)act)->body_position.y - position.y;
            dz = ((actor *)act)->body_position.z - position.z;
            distance_squared = dz * dz + dx * dx + dy * dy;
            if (!(distance_squared < best_distance)) {
                continue;
            }
            if (*(uint32_t *)(offer + 0x4) != 0x7f7fffff && distance_squared > radius * radius) {
                continue;
            }
            if (team_mask > 0) {
                int16_t team = ((actor *)act)->team;

                if (team == -1 || (team_mask & (1 << team)) == 0) {
                    continue;
                }
            }
            if (type_mask > 0 && (type_mask & (1 << act[0x4])) == 0) {
                continue;
            }
            if (filter_count > 0) {
                uint8_t match = 0;
                int16_t j;

                for (j = 0; j < filter_count; j++) {
                    uint32_t filter = *(uint32_t *)(offer + 0x10 + j * 4);

                    if (filter == 0xffffffff) {
                        continue;
                    }
                    match = (uint8_t)(((*(uint32_t *)&((actor *)act)->encounter_index ^ filter) & 0xffff) == 0);
                    if (!match) {
                        continue;
                    }
                    switch (filter >> 30) {
                    case 1:
                        match = (uint8_t)(((actor *)act)->platoon_index == (int16_t)(uint8_t)(filter >> 16));
                        break;
                    case 2:
                        match = (uint8_t)(((actor *)act)->squad_index == (int16_t)(uint8_t)(filter >> 16));
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
    if (actor_build_order_search_object(best_vehicle, actor_index, radius_a, radius_b, order)) {
        actor_set_mode(actor_index, 9, order);
        return 1;
    }
    return 0;
}
