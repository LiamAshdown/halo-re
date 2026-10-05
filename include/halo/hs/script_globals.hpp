#pragma once

#include <stdint.h>
#include "halo/core/link.hpp"
#include "halo/hs/vars.hpp"
#include "halo/core/shared_links.hpp"

/**
 * Link names of the script-visible cheat and debug flags (and the director camera request flag) that the
 * object, unit, AI and script code read. The definitions live in the standalone data layer under these
 * original names (the hs global table points at them); the named references of halo::hs::globals below are
 * what the engine code uses.
 */
static auto &g_0087abc0 = halo::link::ref<uint8_t>(halo::hs::vars().g_0087abc0);
static auto &jetpack = halo::link::ref<uint8_t>(halo::hs::vars().jetpack);
static auto &bump_possession = halo::link::ref<uint8_t>(halo::hs::vars().bump_possession);
static auto &g_0087abc5 = halo::link::ref<uint8_t>(halo::hs::vars().g_0087abc5);
static auto &ai_debug_gate_87abc6 = halo::link::ref<uint8_t>(halo::hs::vars().ai_debug_gate_87abc6);
static auto &g_0087abc7 = halo::link::ref<uint8_t>(halo::hs::vars().g_0087abc7);
static auto &cheat_super_jump = halo::link::ref<uint8_t>(halo::hs::vars().cheat_super_jump);
static auto &object_prediction = halo::link::ref<uint8_t>(halo::hs::vars().object_prediction);
static auto &g_00689481 = halo::link::ref<uint8_t>(halo::hs::vars().g_00689481);
static auto &recover_saved_games_hack = halo::link::ref<uint8_t>(halo::hs::vars().recover_saved_games_hack);
static auto &director_camera_target_changed = halo::link::ref<uint8_t>(halo::hs::vars().director_camera_target_changed);

namespace halo::hs::fields {

/**
 * hs global "cheat_deathless_player". While set, a player-controlled biped or vehicle (and the player-driven
 * children of a vehicle) is not frozen dead with its parent and its body vitality is clamped at zero instead
 * of going negative.
 *
 * @address 0x87abc0
 */
static uint8_t &deathless_player = g_0087abc0;

/**
 * hs global "cheat_jetpack". While set, a unit controlled by a player takes no fall damage.
 *
 * @address 0x87abc1
 */
static uint8_t &jetpack = ::jetpack;

/**
 * hs global "cheat_bump_possession". While set, a unit that has bumped a biped for more than three ticks
 * hands the control of its local player to the bumped biped.
 *
 * @address 0x87abc3
 */
static uint8_t &bump_possession = ::bump_possession;

/**
 * hs global "cheat_reflexive_damage_effects". When set, damage that has no other player effect route marks
 * the damage direction on the first local player's view (single player only).
 *
 * @address 0x87abc5
 */
static uint8_t &reflexive_damage_effects = g_0087abc5;

/**
 * hs global "cheat_medusa". While set, an actor that sees a parented enemy target flags its unit (bit 0x20 of
 * the object byte at +0x106) and, for a swarm, every unit of the cluster.
 *
 * @address 0x87abc6
 */
static uint8_t &medusa = ai_debug_gate_87abc6;

/**
 * hs global "cheat_super_jump". While set, the jump speed of a player-controlled unit is multiplied by 4.
 *
 * @address 0x87abc4
 */
static uint8_t &super_jump = cheat_super_jump;

/**
 * hs global "cheat_omnipotent". While set, any damage a player causes kills the damaged object.
 *
 * @address 0x87abc7
 */
static uint8_t &omnipotent = g_0087abc7;

/**
 * hs global "object_prediction" (default 1). Gates the nudge of an object towards its predicted position when
 * the position update moves it by 5 world units or less.
 *
 * @address 0x689471
 */
static uint8_t &object_prediction = ::object_prediction;

/**
 * hs global "should_play_multiplayer_hit_sound" (default 1). The throttled multiplayer sound event only plays
 * while it equals 1.
 *
 * @address 0x689481
 */
static uint8_t &should_play_multiplayer_hit_sound = g_00689481;

/**
 * hs global "recover_saved_games_hack" (default 0). When set, a revert proceeds even though no revert is
 * available instead of falling back to a map reset.
 *
 * @address 0x746fa4
 */
static uint8_t &recover_saved_games_hack = ::recover_saved_games_hack;

/**
 * Set whenever the director camera mode and target are changed (by the camera scripts, the followed object
 * and the camera control); the next point of view computation then creates a new dead camera for the target
 * and clears it.
 *
 * @address 0x6869d1
 */
static uint8_t &director_camera_target_changed = ::director_camera_target_changed;

/**
 * hs global "framerate_throttle". The 30 fps frame limiter: set when the profile's frame rate mode is 2 and read
 * back by the profile writer to recover that mode. The same variable is
 * halo::interface::state::frame_rate_limiter_enabled.
 *
 * @address 0x6894ba
 */
static uint8_t &framerate_throttle = ::framerate_throttle;

}
