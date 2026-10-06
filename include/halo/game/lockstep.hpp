/**
 * @file include/halo/game/lockstep.hpp
 * Lockstep co-op (src/game/lockstep.cpp) and its determinism probe (src/game/lockstep_probe.cpp).
 */
#pragma once

#include <stdint.h>

struct player_action;

namespace halo::game::lockstep {

/** True while a lockstep level (or the determinism probe) runs: presentation must not feed the simulation. */
bool active();
/** True while a co-op level runs with every player's actions coming through lockstep. */
bool session_active();
int32_t player_count();

/** Whether the campaign game the player starts may be joined (the CO-OP row of Choose Difficulty). */
bool coop_allowed();
void set_coop_allowed(bool allowed);
/** A co-op game's row in the server browser has this game type. */
bool is_coop_gametype(const char *gametype);
/** Joins the co-op game a server browser row (SBServer) advertises. False for any other row. */
bool join_from_browser(void *server);

/** Once per frame: handshake and incoming actions. */
void frame_begin();
/** A level started (seeds the in-tick effects stream) or the game reverted to a checkpoint. */
void on_new_map(uint32_t game_seed);
void on_revert();
/** Creates one player per slot in slot order, the local one marked local. False outside co-op. */
bool create_players();
/** The local action the frame's input built, and whether Back (player_control_input::melee) was held. */
void capture_local_action(const player_action &action, bool back);
/** True between tick_begin and tick_end. */
bool in_tick();
/**
 * A script started a camera move or animation lasting `seconds`. The camera counts its time down by frame time, which
 * differs between machines; hs camera_time reads camera_script_ticks_remaining instead under lockstep.
 */
void camera_script_started(float seconds);
int32_t camera_script_ticks_remaining();
/** The local player asked to skip the cinematic: it travels with the next action, so every machine skips together. */
void local_skip_pressed();
/** The local player picked "revert to checkpoint" / "restart level" in the menu: the same, for both machines. */
void local_revert_requested();
void local_restart_requested();
/** Stamps the local action onto upcoming ticks, sends it, and returns how many of the wanted ticks can run. */
int32_t schedule_ticks(int32_t wanted);
/** Fills the tick's actions, one per player in slot order. */
void apply_actions(player_action *actions);

/**
 * Around every tick. The effects random stream is seeded from the clock and feeds particles, sounds and screen shake
 * on each machine; the few gameplay draws from it (halo::math::simulation_effect_seed) get a stream seeded per level
 * from the game's seed under lockstep. tick_end also exchanges the state hash.
 */
void tick_begin();
void tick_end();
/**
 * Between ticks in a co-op level: the requests the main loop would otherwise act on at the next frame (polling for a
 * safe checkpoint, writing it, switching BSP, respawning co-op players), so every machine acts on the same tick
 * whatever its frame rate. Counters the main loop keeps in frames count ticks here. True when the frame must run no
 * more ticks: a cinematic skip was requested, which the main loop decides on the state of this tick.
 */
bool after_tick();

/** The probe: -lsprobe [jitter] plays b30 with a scripted action and prints state hashes. */
bool probe_active();
void probe_frame_begin();
void probe_override_actions(player_action *actions);
/** The probe's scripted action for a tick: walking, turning and firing in a fixed pattern. */
void probe_scripted_action(int32_t tick, player_action *action);
void probe_tick_end();
/** Prints a LSP-LEAK line when the simulation state changed since the last tick or checkpoint. */
void probe_checkpoint(const char *where);
/** The game RNG and every object's identity, position and velocity. */
uint32_t simulation_hash(int32_t *object_count);

}  // namespace halo::game::lockstep
