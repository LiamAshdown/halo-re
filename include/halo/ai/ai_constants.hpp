#pragma once

#include <cstdint>

namespace halo::ai {

/** The 'iter' tag the data_iterator signature is XORed with: signature = (uintptr_t)data_array ^ k_iterator_signature_key. */
inline constexpr uint32_t k_iterator_signature_key = 0x69746572;

/** Names the game state data arrays of the ai module are registered under (they appear in the saved game's allocation headers). */
inline constexpr char k_actor_data_name[] = "actor";
inline constexpr char k_swarm_data_name[] = "swarm";
inline constexpr char k_swarm_component_data_name[] = "swarm component";
inline constexpr char k_encounter_data_name[] = "encounter";
inline constexpr char k_ai_pursuit_data_name[] = "ai pursuit";
inline constexpr char k_ai_conversation_data_name[] = "ai conversation";

/** Timers, in game ticks (30 per second). */
inline constexpr int16_t k_friends_killed_timer_ticks = 750;      // how long an actor remembers that a friend was killed before the count decays
inline constexpr int32_t k_owner_suspicion_event_ticks = 450;     // lifetime of the perception event recorded for a suspicious owner
inline constexpr int32_t k_hostile_flee_event_ticks = 900;        // lifetime of the perception event recorded when an enemy causes a flee
inline constexpr int32_t k_encounter_engaged_expiry_ticks = 450;  // ticks since an encounter engaged after which its stand-down rules apply
inline constexpr int32_t k_encounter_hostile_memory_ticks = 270;  // ticks since an encounter engaged during which a communication target is treated as hostile

/** The reference string that stands for no ai reference in a script. */
inline constexpr char k_ai_reference_none[] = "none";

}  // namespace halo::ai
