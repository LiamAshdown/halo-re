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

/** The reference string that stands for no ai reference in a script. */
inline constexpr char k_ai_reference_none[] = "none";

}  // namespace halo::ai
