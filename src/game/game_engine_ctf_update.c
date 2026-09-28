// game_engine_ctf_update  (not a Ghidra function; the ctf game engine definition's +0x38 slot (update); no C existed, so that
//   stored pointer trapped as unlisted_468a20)
// address 0x468a20, size 295 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x468a20..0x468b46: resets the unit gauge when its weapon must be readied; then
//   as the server, for a unit holding a must-be-readied weapon (the enemy flag) while the game runs, within 1.0 of
//   its own team's flag (the unit origin +0x5c): with variant +0x7f set and +0x80 zero, a team flag away from its
//   stand (bit 6 of +0x22c) only triggers the throttled carried notice; otherwise the player touches the flag and
//   drops the carried one (the weapon).
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data; // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern game_engine_definition *current_game_engine;
extern int16_t network_game_mode; // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88
extern int32_t game_engine_state_value; // 0x0087aa10
extern datum_index ctf_team_flag_object[2]; // 0x006b0e90
extern uint8_t unit_has_must_be_readied_weapon(uint32_t player_index); // 0x463300, blam-cc: ECX player_index
extern void unit_reset_gauge_if_flagged(uint32_t player_index); // 0x4633a0, blam-cc: EAX player_index
extern uint32_t weapon_must_be_readied(datum_index item_index); // 0x4c2ea0, blam-cc: EAX item_index
extern uint8_t game_engine_ctf_point_within_team_flag_radius(float radius, int32_t team, real_point3d *point); // 0x468990, blam-cc: EAX team, ECX point
extern void game_engine_ctf_notify_flag_carried_throttled(int32_t target_player); // 0x4689e0, blam-cc: EDI target_player
extern void game_engine_ctf_player_touch_flag(uint32_t player_index, int32_t team); // 0x468910, blam-cc: EAX team
extern void game_engine_ctf_player_drop_flag(uint32_t player_index, datum_index flag_object_index); // 0x4688b0, blam-cc: EAX player_index

void game_engine_ctf_update(datum_index player_index)
{
    uint8_t *player = ((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200);
    datum_index unit_index;
    uint8_t *unit;
    int16_t weapon_slot;
    datum_index weapon;
    int32_t team;

    if (unit_has_must_be_readied_weapon(player_index) != 0) {
        unit_reset_gauge_if_flagged(player_index);
    }
    if (network_game_mode != 2) {
        return;
    }
    unit_index = *(datum_index *)(player + 0x34);
    if (unit_index == 0xffffffff) {
        return;
    }
    unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 12 + 8);
    weapon_slot = *(int16_t *)(unit + 0x2f2);
    if (weapon_slot == -1) {
        return;
    }
    weapon = *(datum_index *)(unit + 0x2f8 + weapon_slot * 4);
    if (weapon == 0xffffffff) {
        return;
    }
    if (current_game_engine != 0 && game_engine_state_value != 0) {
        return;
    }
    if (weapon_must_be_readied(weapon) == 0) {
        return;
    }
    team = *(int32_t *)(player + 0x20);
    if (game_engine_ctf_point_within_team_flag_radius(1.0f, team, (real_point3d *)(unit + 0x5c)) == 0) {
        return;
    }
    if (game_engine_variant.ctf_option_7f != 0 && game_engine_variant.ctf_value_80 == 0) {
        uint8_t *flag = *(uint8_t **)((uint8_t *)object_data->data + (ctf_team_flag_object[team] & 0xffff) * 12 + 8);

        if (((*(uint32_t *)(flag + 0x22c) >> 6) & 1) != 0) {
            game_engine_ctf_notify_flag_carried_throttled((int32_t)player_index);
            return;
        }
    }
    game_engine_ctf_player_touch_flag(player_index, team);
    game_engine_ctf_player_drop_flag(player_index, weapon);
}
