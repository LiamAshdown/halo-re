// game_engine_race_allow_grenade_counts  (not a Ghidra function; the race game engine definition's +0x78 slot (allow_grenade_counts); no C existed, so that
//   stored pointer trapped as unlisted_46e980)
// address 0x46e980, size 67 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46e980..0x46e9c2: the race engine's allow_grenade_counts slot, used as a spawn
//   hook: as the server, for a player that has not died yet (+0xae) while fewer vehicles were placed than there are
//   players, places the next race vehicle (0x46d6f0, with its tag picker 0x46d5d0; both exist only for this and are
//   statics here). Returns 1.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "objects.h"
#include "fn_game.h"
#include "fn_objects.h"

extern data_array *player_data; // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern int16_t network_game_mode; // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88
extern Scenario *global_scenario;
extern Globals *global_globals;
extern int32_t race_used_locations[8]; // 0x006b139c, the starting locations given a vehicle so far
extern int32_t race_used_location_count; // 0x006b13bc
extern uint32_t race_vehicle_counts[4]; // 0x006b13c0, custom vehicle set: warthogs, ghosts, rocket warthogs, the sixth vehicle

extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag,
    datum_index role); // 0x4f53a0, blam-cc: EAX placement

extern double cos(double x); // C runtime
extern double sin(double x); // C runtime

// 0x46d5d0 (EAX index): the vehicle tag for the index-th race vehicle, by the variant's vehicle set (low nibble;
//   the multiplayer information's vehicle references are 0x10 bytes, tag index at +0x0c). Set 8 is custom: per-type
//   limits in 3-bit fields, consumed through race_vehicle_counts.
static datum_index race_pick_vehicle_tag(int32_t index)
{
    uint32_t vehicle_set = game_engine_variant.vehicle_set;
    uint8_t *information = (uint8_t *)global_globals->multiplayer_information.pointer;
    uint8_t *vehicles = *(uint8_t **)(information + 0x24);
    datum_index tag = 0xffffffff;

#define VEHICLE(k) (*(datum_index *)(vehicles + (k) * 0x10 + 0x0c))
    switch (vehicle_set & 0xf) {
    case 0:
        if (index == 0) {
            return VEHICLE(0);
        }
        if (index == 1) {
            return VEHICLE(2);
        }
        return index < 6 ? VEHICLE(1) : 0xffffffff;
    case 2:
        return index < 4 ? VEHICLE(0) : 0xffffffff;
    case 3:
        return index < 8 ? VEHICLE(1) : 0xffffffff;
    case 4:
        return index < 4 ? VEHICLE(2) : 0xffffffff;
    case 5:
        return index < 4 ? VEHICLE(5) : 0xffffffff;
    case 8:
        if (race_vehicle_counts[0] < ((vehicle_set >> 4) & 7)) {
            tag = VEHICLE(0);
            race_vehicle_counts[0]++;
            if (tag != 0xffffffff) {
                return tag;
            }
        }
        if (race_vehicle_counts[1] < ((vehicle_set >> 7) & 7)) {
            tag = VEHICLE(1);
            race_vehicle_counts[1]++;
            if (tag != 0xffffffff) {
                return tag;
            }
        }
        if (race_vehicle_counts[3] < ((vehicle_set >> 13) & 7)) {
            tag = VEHICLE(5);
            race_vehicle_counts[3]++;
            if (tag != 0xffffffff) {
                return tag;
            }
        }
        if (race_vehicle_counts[2] < ((vehicle_set >> 10) & 7)) {
            tag = VEHICLE(2);
            race_vehicle_counts[2]++;
        }
        return tag;
    default:
        return 0xffffffff;
    }
#undef VEHICLE
}

// 0x46d6f0 (EAX player): while fewer than 8 have been placed, gives the player a vehicle at the nearest unused
//   type-4 starting location (from its unit's origin, if it has one), facing along the location's facing; the
//   vehicle's +0x5b0 remembers the location. The binary does not check object_new's result; neither does this.
static void race_spawn_next_vehicle(datum_index player_index)
{
    datum_index unit_index = *(datum_index *)(((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200) + 0x34);
    uint8_t *unit = 0;
    int32_t count = race_used_location_count;
    int32_t location_index;
    uint8_t *location;
    datum_index tag;
    object_placement_data placement;
    datum_index vehicle;
    float facing;

    if (unit_index != 0xffffffff) {
        unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 12 + 8);
    }
    if (count >= 8) {
        return;
    }
    location_index = game_engine_find_nearest_unused_type4_location(race_used_locations, count,
        unit != 0 ? (real_point3d *)(unit + 0x5c) : (real_point3d *)0);
    if (location_index == -1) {
        return;
    }
    race_used_locations[count] = location_index;
    race_used_location_count = count + 1;
    location = (uint8_t *)global_scenario->netgame_flags.pointer + location_index * 0x94;
    tag = race_pick_vehicle_tag(count);
    if (tag == 0xffffffff) {
        return;
    }
    object_placement_data_initialize(&placement, tag, 0xffffffff);
    placement.position = *(real_point3d *)location;
    facing = *(float *)(location + 0x0c);
    placement.forward.i = (float)cos(facing);
    placement.forward.j = (float)sin(facing);
    placement.forward.k = 0.0f;
    vehicle = object_new(&placement);
    *(int16_t *)(*(uint8_t **)((uint8_t *)object_data->data + (vehicle & 0xffff) * 12 + 8) + 0x5b0) = (int16_t)location_index;
}

uint8_t game_engine_race_allow_grenade_counts(datum_index player_index)
{
    if (network_game_mode == 2 && *(int16_t *)(((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200) + 0xae) == 0 &&
        race_used_location_count < *(int16_t *)((uint8_t *)player_data + 0x30)) {
        race_spawn_next_vehicle(player_index);
    }
    return 1;
}
