exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
EXT.update({
    'ball_tick': ('extern int32_t oddball_ball_timers_006b11cc[16]; // 0x006b11cc\n'
                  'extern void game_engine_koth_relocate_hill_marker(int32_t ball_index); // 0x46bfe0, blam-cc: ESI ball_index\n'
                  'extern uint8_t custom_waypoints[]; // 0x006f1888 (0x20 bytes each)\n'
                  'extern int16_t hud_waypoint_arrow_find(const char *name); // 0x4af070, blam-cc: EDI name'),
    'race_spawn': ('extern uint8_t *global_scenario; // 0x00746f8c\n'
                   'extern uint8_t *global_globals; // 0x00746fa0 (Globals *; +0x168 the multiplayer information)\n'
                   'extern int32_t race_used_locations[8]; // 0x006b139c, the starting locations given a vehicle so far\n'
                   'extern int32_t race_used_location_count; // 0x006b13bc\n'
                   'extern uint32_t race_vehicle_counts[4]; // 0x006b13c0, custom vehicle set: warthogs, ghosts, rocket warthogs, the sixth vehicle\n'
                   'extern int32_t game_engine_find_nearest_unused_type4_location(int32_t *excluded_indices, int32_t excluded_count,\n'
                   '    real_point3d *reference_point); // 0x46d520, blam-cc: EDI excluded_count, EBX reference_point\n'
                   'extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag,\n'
                   '    datum_index role); // 0x4f53a0, blam-cc: EAX placement\n'
                   'extern datum_index object_new(object_placement_data *placement); // 0x4f5460, blam-cc: ECX placement\n'
                   'extern double cos(double x); // C runtime\n'
                   'extern double sin(double x); // C runtime'),
})

emit(0x46c750, 390, 'game_engine_oddball_unknown_48',
     'also the race engine\'s +0xf8 slot. At tick 60 queues sound 0x21 with teams, 0x13 without. As the server each running ball timer counts down and at zero queues sound 0 and respawns that ball (0x46bfe0). With variant +0x8c in 1..2 each ball waypoint follows its carrier: cleared when uncarried, else owner = the carrier, arrow "target_blue", visible, at the carrier unit\'s origin 0.63 higher, both filters -1.',
     ['object_data', 'player_data', 'game_time', 'current_game_engine', 'teams', 'network_game_mode', 'variant', 'sound',
      'king_hill_occupant_table', 'ball_tick'], 'void %s(void)',
     '    int32_t count;\n    int32_t i;\n\n'
     '    if (*(int32_t *)(game_time + 0xc) == 0x3c) {\n'
     '        uint8_t teams = current_game_engine != 0 ? game_engine_teams_enabled_flag : 0;\n\n'
     '        game_engine_queue_multiplayer_sound(teams != 0 ? 0x21 : 0x13, 0xffffffff, 0);\n    }\n'
     '    count = game_engine_variant.unknown_90;\n'
     '    if (network_game_mode == 2) {\n        for (i = 0; i < count; i++) {\n'
     '            if (oddball_ball_timers_006b11cc[i] > 0 && --oddball_ball_timers_006b11cc[i] == 0) {\n'
     '                game_engine_queue_multiplayer_sound(0, 0xffffffff, 0);\n                game_engine_koth_relocate_hill_marker(i);\n            }\n        }\n    }\n'
     '    if (game_engine_variant.unknown_8c <= 0 || game_engine_variant.unknown_8c > 2) {\n        return;\n    }\n'
     '    for (i = 0; i < count; i++) {\n'
     '        uint8_t *waypoint = custom_waypoints + (int16_t)i * 0x20;\n        datum_index carrier = king_hill_occupant_table[i];\n        datum_index unit_index;\n        uint8_t *unit;\n\n'
     '        if (carrier == 0xffffffff) {\n            memset(waypoint, 0, 0x20);\n            continue;\n        }\n'
     '        unit_index = *(datum_index *)(%s + 0x34);\n' % P('carrier') +
     '        if (unit_index == 0xffffffff) {\n            continue;\n        }\n'
     '        unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 12 + 8);\n'
     '        *(datum_index *)(waypoint + 0x18) = carrier;\n'
     '        *(int16_t *)(waypoint + 0x1c) = hud_waypoint_arrow_find("target_blue");\n'
     '        waypoint[0x0c] = 1;\n        *(real_point3d *)waypoint = *(real_point3d *)(unit + 0xa0);\n'
     '        *(float *)(waypoint + 0x08) += 0.63f;\n'
     '        *(int16_t *)(waypoint + 0x14) = -1;\n        *(int32_t *)(waypoint + 0x10) = -1;\n    }\n',
     extra_inc='#include <string.h>\n')

HELP = '''// 0x46d5d0 (EAX index): the vehicle tag for the index-th race vehicle, by the variant's vehicle set (low nibble;
//   the multiplayer information's vehicle references are 0x10 bytes, tag index at +0x0c). Set 8 is custom: per-type
//   limits in 3-bit fields, consumed through race_vehicle_counts.
static datum_index race_pick_vehicle_tag(int32_t index)
{
    uint32_t vehicle_set = game_engine_variant.vehicle_set;
    uint8_t *information = *(uint8_t **)(global_globals + 0x168);
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
    datum_index unit_index = *(datum_index *)(%s + 0x34);
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
    location = *(uint8_t **)(global_scenario + 0x37c) + location_index * 0x94;
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
''' % P('player_index')

emit(0x46e980, 67, 'game_engine_race_allow_grenade_counts',
     'the race engine\'s allow_grenade_counts slot, used as a spawn hook: as the server, for a player that has not died yet (+0xae) while fewer vehicles were placed than there are players, places the next race vehicle (0x46d6f0, with its tag picker 0x46d5d0; both exist only for this and are statics here). Returns 1.',
     ['player_data', 'object_data', 'network_game_mode', 'variant', 'race_spawn'], 'uint8_t %s(datum_index player_index)',
     '    if (network_game_mode == 2 && *(int16_t *)(%s + 0xae) == 0 &&\n' % P('player_index') +
     '        race_used_location_count < *(int16_t *)((uint8_t *)player_data + 0x30)) {\n'
     '        race_spawn_next_vehicle(player_index);\n    }\n    return 1;\n',
     extra_inc='#include "objects.h"\n', helper=HELP)
print('ok')
