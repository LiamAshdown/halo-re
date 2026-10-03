#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

namespace halo::game {

/**
 * Non-owning view of one team_pair_override entry.
 */
class TeamPairOverride {
public:
    team_pair_override * entry;

    explicit constexpr TeamPairOverride(team_pair_override * entry_) : entry(entry_) {}

    void set(uint8_t active, uint8_t clear_secondary);
};

/**
 * Facade over the global team relationship table (enemy and secondary bitmasks plus the
 * override list).
 */
class TeamPairTable {
public:
    TeamPairTable() = delete;

    static uint8_t are_enemies(int16_t team_a, int16_t team_b);
    static uint8_t flag_test(int16_t team_a, int16_t team_b);
    static void override_add(int16_t index_a, uint8_t unknown_08, int16_t index_b, uint8_t unknown_09, int16_t threshold, int16_t timer_reset, uint8_t unknown_0c);
    static uint32_t override_adjust_counter(int16_t index_a, int16_t index_b, int16_t delta_selector, uint8_t *out_flag);
    static void override_clear_flag(int16_t index_b, int16_t index_a);
    static uint8_t override_get_flag(int16_t index_a, int16_t index_b);
    static void override_refresh(int16_t index_b, int16_t index_a);
    static uint32_t override_remove(int16_t index_a, int16_t index_b);
    static void overrides_tick();
    static void allocate();
    static void init_defaults();
};

}  // namespace halo::game
