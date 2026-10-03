#include "halo/game/gamerest_teams.hpp"
#include "halo/memory/api.hpp"
#include "halo/saved_games/api.hpp"

extern "C" {
extern team_pair_globals *team_pair_data;
extern void ai_notify_actors_of_encounter_state_change(int16_t team_a, int16_t team_b, uint8_t active, uint8_t clear_secondary);
extern void __cdecl standalone_log(const char *format, ...);
extern game_engine_definition *current_game_engine;
extern void team_pair_set(team_pair_override *entry, uint8_t active, uint8_t clear_secondary);
}

namespace halo::game {

/**
 * Updates one pair's cached `active` flag and both 10x10 relationship bitmasks (secondary_bits
 * governed by `clear_secondary`, enemy_bits governed by `active`), skipping the work entirely
 * when neither would actually change anything.
 *
 * @address 0x45c130
 */
void TeamPairOverride::set(uint8_t active, uint8_t clear_secondary)
{
    standalone_log("DIAG team_pair_set a=%d b=%d BL=%d clear=%d entry_active=%d refcount=%d", entry->index_a,
        entry->index_b, active, clear_secondary, entry->active, entry->refcount);
    int32_t index;
    int32_t reverse_index;

    if (clear_secondary != 0 || entry->active != active) {
        entry->active = active;
        if (entry->index_a < 10 && entry->index_b < 10) {
            index = (int32_t)entry->index_b + entry->index_a * 10;
            reverse_index = (int32_t)entry->index_a + entry->index_b * 10;
            if (clear_secondary == 0) {
                team_pair_data->secondary_bits[index >> 5] |= 1u << (index & 0x1f);
                team_pair_data->secondary_bits[reverse_index >> 5] |= 1u << (reverse_index & 0x1f);
            } else {
                team_pair_data->secondary_bits[index >> 5] &= ~(1u << (index & 0x1f));
                team_pair_data->secondary_bits[reverse_index >> 5] &= ~(1u << (reverse_index & 0x1f));
            }

            index = (int32_t)entry->index_b + entry->index_a * 10;
            if (active == 0) {
                team_pair_data->enemy_bits[index >> 5] |= 1u << (index & 0x1f);
                reverse_index = (int32_t)entry->index_a + entry->index_b * 10;
                team_pair_data->enemy_bits[reverse_index >> 5] |= 1u << (reverse_index & 0x1f);
            } else {
                team_pair_data->enemy_bits[index >> 5] &= ~(1u << (index & 0x1f));
                reverse_index = (int32_t)entry->index_a + entry->index_b * 10;
                team_pair_data->enemy_bits[reverse_index >> 5] &= ~(1u << (reverse_index & 0x1f));
            }
        }
        entry->status = 1;
        ai_notify_actors_of_encounter_state_change(entry->index_a, entry->index_b, active, clear_secondary);
    }
}

/**
 * Returns whether two 0-9 team indices are hostile. Outside a loaded multiplayer game engine
 * this just compares the indices; inside one, out-of-range indices default to "enemies" and
 * in-range ones consult the enemy_bits table (reflexively seeded, so same-team never matches).
 *
 * @address 0x45bd50
 */
uint8_t TeamPairTable::are_enemies(int16_t team_a, int16_t team_b)
{
    int32_t index;

    if (current_game_engine != (game_engine_definition *)0) {
        return team_b != team_a;
    }
    if (-1 < team_b && team_b < 10 && -1 < team_a && team_a < 10) {
        index = (int32_t)team_a + team_b * 10;
        return (uint8_t)(1 - ((team_pair_data->enemy_bits[index >> 5] & (1u << (index & 0x1f))) != 0));
    }
    return 1;
}

/**
 * Tests the secondary per-pair flag bit (distinct from the enemy_bits bitmask) for a pair of
 * 0-9 team indices.
 *
 * @address 0x45bdb0
 */
uint8_t TeamPairTable::flag_test(int16_t team_a, int16_t team_b)
{
    int32_t index;

    if (-1 < team_a && team_a < 10 && -1 < team_b && team_b < 10) {
        index = (int32_t)team_b + team_a * 10;
        return (team_pair_data->secondary_bits[index >> 5] & (1u << (index & 0x1f))) != 0;
    }
    return 0;
}

/**
 * Finds an existing override for the (index_a, index_b) pair, or allocates a new slot (capped
 * at k_maximum_team_pair_overrides), then (re)initializes its fields and activates it via
 * team_pair_set.
 *
 * @address 0x45be50
 */
void TeamPairTable::override_add(int16_t index_a, uint8_t unknown_08, int16_t index_b, uint8_t unknown_09, int16_t threshold, int16_t timer_reset, uint8_t unknown_0c)
{
    int16_t count;
    int16_t i;
    team_pair_override *entry;

    count = team_pair_data->override_count;
    i = 0;
    for (i = 0; i < count; i = i + 1) {
        entry = &team_pair_data->overrides[i];
        if ((entry->index_a == index_a && entry->index_b == index_b) ||
            (entry->index_b == index_a && entry->index_a == index_b)) {
            break;
        }
    }

    if (count <= i && count < 8) {
        team_pair_data->override_count = count + 1;
        i = count;
    }

    if (i < team_pair_data->override_count) {
        entry = &team_pair_data->overrides[i];
        entry->unknown_09 = unknown_09;
        entry->unknown_08 = unknown_08;
        entry->threshold = threshold;
        entry->index_a = index_a;
        entry->refcount = 0;
        entry->timer = 0;
        entry->index_b = index_b;
        entry->timer_reset = timer_reset;
        entry->unknown_0c = unknown_0c;
        entry->active = 1;

        TeamPairOverride(entry).set(0, 0);
        entry->status = 0;
    }
}

/**
 * Finds the directional override entry matching (index_a, index_b) -- the (a,b) order requires
 * unknown_09, the (b,a) order requires unknown_08 -- and adjusts its refcount by +1, +3 or -1
 * per `delta_selector` (0, 1, 2). Refreshes the countdown timer from timer_reset, and once
 * refcount reaches the entry's threshold, activates it via team_pair_set and optionally reports
 * whether unknown_0c was zero through `out_flag`.
 *
 * @address 0x45bfc0
 */
uint32_t TeamPairTable::override_adjust_counter(int16_t index_a, int16_t index_b, int16_t delta_selector, uint8_t *out_flag)
{
    int16_t i;
    team_pair_override *entry;
    int16_t delta;

    if (0 < team_pair_data->override_count) {
        for (i = 0; ; i = i + 1) {
            entry = &team_pair_data->overrides[i];
            if ((entry->index_a == index_a && entry->index_b == index_b && entry->unknown_09 != 0) ||
                (entry->index_b == index_a && entry->index_a == index_b && entry->unknown_08 != 0)) {
                break;
            }
            if (team_pair_data->override_count <= i + 1) {
                return 0;
            }
        }

        delta = 0;
        if (delta_selector == 0) {
            delta = 1;
        } else if (delta_selector == 1) {
            delta = 3;
        } else if (delta_selector == 2) {
            delta = -1;
        }
        entry->refcount = entry->refcount + delta;
        if (entry->timer_reset != -1) {
            entry->timer = entry->timer_reset;
        }
        if (entry->threshold != -1 && entry->threshold <= entry->refcount) {
            TeamPairOverride(entry).set(1, 0);
            if (out_flag != (uint8_t *)0) {
                *out_flag = entry->unknown_0c == 0;
            }
            return 1;
        }
    }
    return 0;
}

/**
 * Finds an override matching (index_a, index_b) in either order and clears its status byte.
 *
 * @address 0x45c0f0
 */
void TeamPairTable::override_clear_flag(int16_t index_b, int16_t index_a)
{
    int16_t i;
    team_pair_override *entry;

    if (0 < team_pair_data->override_count) {
        for (i = 0; ; i = i + 1) {
            entry = &team_pair_data->overrides[i];
            if ((entry->index_a == index_a && entry->index_b == index_b) ||
                (entry->index_b == index_a && entry->index_a == index_b)) {
                break;
            }
            if (team_pair_data->override_count <= i + 1) {
                return;
            }
        }
        entry->status = 0;
    }
}

/**
 * Scans the override list for a pair (either index order) and returns its status byte, or 0 if
 * no matching entry exists.
 *
 * @address 0x45be00
 */
uint8_t TeamPairTable::override_get_flag(int16_t index_a, int16_t index_b)
{
    int16_t i;

    if (0 < team_pair_data->override_count) {
        for (i = 0; i < team_pair_data->override_count; i = i + 1) {
            team_pair_override *entry = &team_pair_data->overrides[i];
            if ((entry->index_a == index_a && entry->index_b == index_b) ||
                (entry->index_b == index_a && entry->index_a == index_b)) {
                return entry->status;
            }
        }
    }
    return 0;
}

/**
 * Finds the directional override entry matching (index_a, index_b) (same matching rule as
 * team_pair_override_adjust_counter) and, if it still has a positive refcount, resets its
 * countdown timer back to timer_reset.
 *
 * @address 0x45c090
 */
void TeamPairTable::override_refresh(int16_t index_b, int16_t index_a)
{
    int16_t i;
    team_pair_override *entry;

    if (0 < team_pair_data->override_count) {
        for (i = 0; ; i = i + 1) {
            entry = &team_pair_data->overrides[i];
            if ((entry->index_a == index_a && entry->index_b == index_b && entry->unknown_09 != 0) ||
                (entry->index_b == index_a && entry->index_a == index_b && entry->unknown_08 != 0)) {
                break;
            }
            if (team_pair_data->override_count <= i + 1) {
                return;
            }
        }
        if (0 < entry->refcount && entry->timer_reset != -1) {
            entry->timer = entry->timer_reset;
        }
    }
}

/**
 * Finds the override matching (index_a, index_b) in either order, clears its bit via
 * team_pair_set, then compacts the list by moving the last entry into the freed slot. Returns 0
 * if no matching entry was found.
 *
 * @address 0x45bf10
 */
uint32_t TeamPairTable::override_remove(int16_t index_a, int16_t index_b)
{
    int16_t i;
    team_pair_override *entry;

    if (team_pair_data->override_count < 1) {
        return 0;
    }
    for (i = 0; ; i = i + 1) {
        entry = &team_pair_data->overrides[i];
        if ((entry->index_a == index_b && entry->index_b == index_a) ||
            (entry->index_b == index_b && entry->index_a == index_a)) {
            break;
        }
        if (team_pair_data->override_count <= i + 1) {
            return 0;
        }
    }

    TeamPairOverride(entry).set(1, 1);
    team_pair_data->override_count = team_pair_data->override_count - 1;
    if (i < team_pair_data->override_count) {
        *entry = team_pair_data->overrides[team_pair_data->override_count];
    }
    return 1;
}

/**
 * Ticks every active override's countdown timer; once a countdown reaches zero, decrements the
 * entry's refcount, and once THAT reaches zero, deactivates the pair -- otherwise the countdown
 * is refreshed from the entry's own reset value.
 *
 * @address 0x45bcf0
 */
void TeamPairTable::overrides_tick()
{
    int16_t i;
    team_pair_override *entry;

    for (i = 0; i < team_pair_data->override_count; i = i + 1) {
        entry = &team_pair_data->overrides[i];
        if (0 < entry->timer) {
            entry->timer = entry->timer - 1;
            if (entry->timer == 0) {
                entry->refcount = entry->refcount - 1;
                if (entry->refcount == 0) {
                    TeamPairOverride(entry).set(0, 0);
                } else {
                    entry->timer = entry->timer_reset;
                }
            }
        }
    }
}

/**
 * Bump-allocates and zeroes the team-pair relationship table from the game-state arena.
 *
 * @address 0x45bc30
 */
void TeamPairTable::allocate()
{
    int32_t count;
    uint32_t *cursor;
    uint32_t size;

    cursor = (uint32_t *)(halo::saved_games::globals().game_state_cursor + halo::saved_games::globals().game_state_base);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 0xb4;
    size = 0xb4;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);
    team_pair_data = (team_pair_globals *)cursor;
    for (count = 0x2d; count != 0; count = count - 1) {
        *cursor = 0;
        cursor = cursor + 1;
    }
}

/**
 * Clears the override list and both relationship bitmasks, then marks every team index as
 * related to itself (the reflexive default) in enemy_bits.
 *
 * @address 0x45bc80
 */
void TeamPairTable::init_defaults()
{
    int32_t i;
    int32_t index;

    team_pair_data->override_count = 0;
    team_pair_data->secondary_bits[0] = 0;
    team_pair_data->secondary_bits[1] = 0;
    team_pair_data->secondary_bits[2] = 0;
    team_pair_data->secondary_bits[3] = 0;
    team_pair_data->enemy_bits[0] = 0;
    team_pair_data->enemy_bits[1] = 0;
    team_pair_data->enemy_bits[2] = 0;
    team_pair_data->enemy_bits[3] = 0;

    index = 0;
    for (i = 10; i != 0; i = i - 1) {
        team_pair_data->enemy_bits[index >> 5] |= 1u << (index & 0x1f);
        index = index + 0xb;
    }
}

}  // namespace halo::game

extern "C" {

/**
 * C entry point for halo::game::TeamPairOverride::set; forwards to the C++ implementation.
 * register convention: entry pointer in EAX (in_EAX); the new "active" state in EBX
 * (unaff_BL, also stored into the entry's active field); a secondary-bitmask gate as the
 * recognized stack parameter (param_1).
 * // blam-cc: EAX -> entry, EBX -> active, stack -> clear_secondary
 * blam-cc: EAX -> entry, EBX -> active, stack -> clear_secondary
 *
 * @address 0x45c130
 */
void team_pair_set(team_pair_override *entry, uint8_t active, uint8_t clear_secondary)
{
    halo::game::TeamPairOverride(entry).set(active, clear_secondary);
}

/**
 * C entry point for halo::game::TeamPairTable::are_enemies; forwards to the C++ implementation.
 * register convention: both team indices are the recognized register parameters (in_CX, in_DX).
 * // blam-cc: ECX -> team_a, EDX -> team_b (per Ghidra's in_CX/in_DX naming)
 * blam-cc: in_CX -> team_a, in_DX -> team_b
 *
 * @address 0x45bd50
 */
uint8_t teams_are_enemies(int16_t team_a, int16_t team_b)
{
    return halo::game::TeamPairTable::are_enemies(team_a, team_b);
}

/**
 * C entry point for halo::game::TeamPairTable::flag_test; forwards to the C++ implementation.
 * register convention: both team indices are the recognized register parameters (in_CX, in_DX).
 * // blam-cc: ECX -> team_a, EDX -> team_b
 * blam-cc: ECX -> team_a, EDX -> team_b
 *
 * @address 0x45bdb0
 */
uint8_t team_pair_flag_test(int16_t team_a, int16_t team_b)
{
    return halo::game::TeamPairTable::flag_test(team_a, team_b);
}

/**
 * C entry point for halo::game::TeamPairTable::override_add; forwards to the C++ implementation.
 * register convention: first index in EAX (in_AX); the rest are the recognized stack
 * parameters, in field-write order.
 * // blam-cc: EAX -> index_a, stack -> unknown_08, index_b, unknown_09, threshold,
 * //          timer_reset, unknown_0c
 * blam-cc: EAX -> index_a, stack -> unknown_08, index_b, unknown_09, threshold,
 *
 * @address 0x45be50
 */
void team_pair_override_add(int16_t index_a, uint8_t unknown_08, int16_t index_b, uint8_t unknown_09, int16_t threshold, int16_t timer_reset, uint8_t unknown_0c)
{
    halo::game::TeamPairTable::override_add(index_a, unknown_08, index_b, unknown_09, threshold, timer_reset, unknown_0c);
}

/**
 * C entry point for halo::game::TeamPairTable::override_adjust_counter; forwards to the C++ implementation.
 * register convention: first index in EAX (in_AX); the rest are the recognized stack
 * parameters.
 * // blam-cc: EAX -> index_a, stack -> index_b, delta_selector, out_flag
 * blam-cc: EAX -> index_a, stack -> index_b, delta_selector, out_flag
 *
 * @address 0x45bfc0
 */
uint32_t team_pair_override_adjust_counter(int16_t index_a, int16_t index_b, int16_t delta_selector, uint8_t *out_flag)
{
    return halo::game::TeamPairTable::override_adjust_counter(index_a, index_b, delta_selector, out_flag);
}

/**
 * C entry point for halo::game::TeamPairTable::override_clear_flag; forwards to the C++ implementation.
 * register convention: both indices are implicit registers (unaff_BX, unaff_DI); EBX read
 * first per the project's convention.
 * // blam-cc: EBX -> index_b, EDI -> index_a
 * blam-cc: EBX -> index_b, EDI -> index_a
 *
 * @address 0x45c0f0
 */
void team_pair_override_clear_flag(int16_t index_b, int16_t index_a)
{
    halo::game::TeamPairTable::override_clear_flag(index_b, index_a);
}

/**
 * C entry point for halo::game::TeamPairTable::override_get_flag; forwards to the C++ implementation.
 * register convention: first index in EBX (unaff_BX); the second index is the recognized stack
 * parameter (param_1).
 * // blam-cc: EBX -> index_a, stack -> index_b
 * blam-cc: EBX -> index_a, stack -> index_b
 *
 * @address 0x45be00
 */
uint8_t team_pair_override_get_flag(int16_t index_a, int16_t index_b)
{
    return halo::game::TeamPairTable::override_get_flag(index_a, index_b);
}

/**
 * C entry point for halo::game::TeamPairTable::override_refresh; forwards to the C++ implementation.
 * register convention: both indices are implicit registers (unaff_BX, unaff_DI); per the
 * project's EAX/ECX/EDX/EBX/ESI/EDI ordering, EBX is read first.
 * // blam-cc: EBX -> index_b, EDI -> index_a
 * blam-cc: EBX -> index_b, EDI -> index_a
 *
 * @address 0x45c090
 */
void team_pair_override_refresh(int16_t index_b, int16_t index_a)
{
    halo::game::TeamPairTable::override_refresh(index_b, index_a);
}

/**
 * C entry point for halo::game::TeamPairTable::override_remove; forwards to the C++ implementation.
 * register convention: first index in EAX (in_AX); the second index is the recognized stack
 * parameter (param_1).
 * // blam-cc: EAX -> index_a, stack -> index_b
 * blam-cc: EAX -> index_a, stack -> index_b
 *
 * @address 0x45bf10
 */
uint32_t team_pair_override_remove(int16_t index_a, int16_t index_b)
{
    return halo::game::TeamPairTable::override_remove(index_a, index_b);
}

/**
 * C entry point for halo::game::TeamPairTable::overrides_tick; forwards to the C++ implementation.
 * register convention: no arguments.
 *
 * @address 0x45bcf0
 */
void team_pair_overrides_tick(void)
{
    halo::game::TeamPairTable::overrides_tick();
}

/**
 * C entry point for halo::game::TeamPairTable::allocate; forwards to the C++ implementation.
 * register convention: no arguments.
 *
 * @address 0x45bc30
 */
void team_pair_table_allocate(void)
{
    halo::game::TeamPairTable::allocate();
}

/**
 * C entry point for halo::game::TeamPairTable::init_defaults; forwards to the C++ implementation.
 *
 * @address 0x45bc80
 */
void team_pair_table_init_defaults(void)
{
    halo::game::TeamPairTable::init_defaults();
}

}
