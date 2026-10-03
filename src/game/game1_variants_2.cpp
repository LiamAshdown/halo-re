/**
 * Game variant selection, loading, validation and player profile capture.
 */

#include "tags.h"
#include "halo/game/records.hpp"
#include "halo/core/datum.hpp"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"

#include "halo/game/game1_variants.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern player_profile player_profile_cache[16];
extern data_array *player_data;
extern game_variant game_engine_variant;
extern uint8_t *machine_table;
}

namespace halo::game::engine1 {

/**
 * Captures the profile of a player slot into the profile cache.
 *
 * Original register convention: EAX -> slot, stack -> commit.
 *
 * @address 0x466ee0
 */
void Variants::capture_player_profile(int32_t slot, int32_t commit)
{
    datum_index player_handle;
    player *p;
    player_profile snapshot;
    int32_t lookup_result;

    player_handle = player_profile_cache[slot].player;
    p = halo::game::player_at(player_handle);

    lookup_result = 0;
    if (player_handle != (datum_index)halo::k_dword_none) {
        lookup_result = halo::objects::hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)player_handle);
        if (lookup_result == -1) {
            lookup_result = 0;
        }
    }

    snapshot.kills = p->kills;
    snapshot.unknown_0a = p->unknown_9e;
    snapshot.unknown_0c = p->unknown_a0;
    snapshot.assists = p->assists;
    snapshot.unknown_12 = p->unknown_a6;
    snapshot.unknown_14 = p->unknown_a8;
    snapshot.betrayals = p->betrayals;
    snapshot.deaths = p->deaths;
    snapshot.suicides = p->suicides;
    snapshot.objective_time = p->objective_time;
    snapshot.objective_score = p->objective_score;
    snapshot.slayer_target = p->slayer_target;
    snapshot.odd_man_out = p->odd_man_out;
    snapshot.speed = p->speed;

    if (game_engine_variant.game_engine_index == _game_engine_king) {
        int16_t *low_word = (int16_t *)&snapshot.objective_time;
        *low_word = *low_word / 30;
    }

    halo::game::game_engine_send_player_profile_update(&lookup_result, &snapshot.kills, -1);

    if (commit == 1) {
        player_profile_cache[slot].kills = snapshot.kills;
        player_profile_cache[slot].unknown_0a = snapshot.unknown_0a;
        player_profile_cache[slot].unknown_0c = snapshot.unknown_0c;
        player_profile_cache[slot].assists = snapshot.assists;
        player_profile_cache[slot].unknown_12 = snapshot.unknown_12;
        player_profile_cache[slot].unknown_14 = snapshot.unknown_14;
        player_profile_cache[slot].betrayals = snapshot.betrayals;
        player_profile_cache[slot].deaths = snapshot.deaths;
        player_profile_cache[slot].suicides = snapshot.suicides;
        player_profile_cache[slot].objective_time = snapshot.objective_time;
        player_profile_cache[slot].objective_score = snapshot.objective_score;
        player_profile_cache[slot].slayer_target = snapshot.slayer_target;
        player_profile_cache[slot].odd_man_out = snapshot.odd_man_out;
        player_profile_cache[slot].speed = snapshot.speed;
    }
}

}
