/**
 * @file include/halo/game/lockstep.hpp
 * Lockstep co-op support. For now only the determinism probe (src/game/lockstep_probe.cpp).
 */
#pragma once

struct player_action;

namespace halo::game::lockstep {

/** True while a lockstep game (or the determinism probe) runs: presentation must not feed the simulation. */
bool active();
bool probe_active();
void probe_frame_begin();
void probe_override_actions(player_action *actions);
void probe_tick_end();
/** Prints a LSP-LEAK line when the simulation state changed since the last tick or checkpoint. */
void probe_checkpoint(const char *where);

/**
 * The effects random stream is seeded from the clock and drawn both inside ticks (sound permutations, effect event
 * delays, animation picks, shatters) and per frame (particles, camera shake). Under lockstep the draws inside a tick
 * use their own stream, seeded per map from the game's seed, so every machine draws the same numbers.
 */
void tick_effect_random_seed_reset(uint32_t game_seed);
void tick_begin();
void tick_end();

}  // namespace halo::game::lockstep
