#pragma once

#include <cstddef>
#include <cstdint>

#include "halo/core/flags.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/cache/api.hpp"

#include "halo/effects/types.hpp"

namespace halo::effects {

/**
 * Bits of ParticleSystemType.flags, in tag order.
 */
enum class particle_type_flag : uint32_t {
    type_states_loop = 1u << 0,
    type_states_ping_pong = 1u << 1,
    particle_states_loop = 1u << 2,
    particle_states_ping_pong = 1u << 3,
    particles_die_in_water = 1u << 4,
    particles_die_in_air = 1u << 5,
    particles_die_on_ground = 1u << 6,
    rotational_sprites_animate_sideways = 1u << 7,
    disabled = 1u << 8,
    tint_by_effect_color = 1u << 9,
    initial_count_scales_with_effect = 1u << 10,
    do_not_draw_in_first_person = 1u << 16,
    do_not_draw_in_third_person = 1u << 17,
};

}  // namespace halo::effects

namespace halo {
template <>
struct enable_bit_flags<effects::particle_type_flag> : std::true_type {};
}  // namespace halo

namespace halo::effects {

inline bool particle_type_has(const ParticleSystemType *type, particle_type_flag flag) noexcept
{
    return has(static_cast<particle_type_flag>(type->flags), flag);
}

inline ParticleSystemType *particle_system_type_at(const ParticleSystem *definition, int32_t type_index) noexcept
{
    return &reinterpret_cast<ParticleSystemType *>(static_cast<uintptr_t>(definition->particle_types.pointer))[type_index];
}

/**
 * The ParticleSystem tag a runtime particle_system record was created from.
 */
inline ParticleSystem *particle_system_definition(const particle_system *system) noexcept
{
    return static_cast<ParticleSystem *>(
        halo::cache::globals().tag_instances[system->definition_index & halo::k_slot_mask].data);
}

inline ParticleSystemType *particle_system_type_at(const particle_system *system, int32_t type_index) noexcept
{
    return particle_system_type_at(particle_system_definition(system), type_index);
}

/**
 * The shader block embedded in a particle state starts at 0xb8 (the framebuffer blend, fade and bitmap
 * flag fields below follow it); the renderer writes the average radius at 0x98 of that block.
 */
inline constexpr size_t k_particle_state_shader_block_offset = 0xb8;
inline constexpr size_t k_particle_shader_average_radius_offset = 0x98;

inline uint8_t *particle_state_shader_block(ParticleSystemTypeParticleState *state) noexcept
{
    return reinterpret_cast<uint8_t *>(state) + k_particle_state_shader_block_offset;
}

/**
 * The 16-bit sprite parameter the non-rotational sprite builder reads from the otherwise unnamed word
 * at ParticleSystemType 0x2a.
 */
inline uint16_t particle_type_sprite_parameter(const ParticleSystemType *type) noexcept
{
    return *reinterpret_cast<const uint16_t *>(type->_pad_2a);
}

static_assert(sizeof(ParticleSystem) == 0x68);
static_assert(offsetof(ParticleSystem, particle_types) == 0x5c);
static_assert(sizeof(ParticleSystemType) == 0x80);
static_assert(offsetof(ParticleSystemType, flags) == 0x20);
static_assert(offsetof(ParticleSystemType, initial_particle_count) == 0x24);
static_assert(offsetof(ParticleSystemType, complex_sprite_render_mode) == 0x28);
static_assert(offsetof(ParticleSystemType, radius) == 0x2c);
static_assert(offsetof(ParticleSystemType, particle_creation_physics) == 0x54);
static_assert(offsetof(ParticleSystemType, states) == 0x68);
static_assert(offsetof(ParticleSystemType, particle_states) == 0x74);
static_assert(sizeof(ParticleSystemTypeStates) == 0xc0);
static_assert(offsetof(ParticleSystemTypeStates, particle_creation_physics) == 0xb0);
static_assert(sizeof(ParticleSystemTypeParticleState) == 0x178);
static_assert(offsetof(ParticleSystemTypeParticleState, bitmaps) == 0x30);
static_assert(offsetof(ParticleSystemTypeParticleState, sequence_index) == 0x40);
static_assert(offsetof(ParticleSystemTypeParticleState, framebuffer_blend_function) == 0xe2);
static_assert(offsetof(ParticleSystemTypeParticleState, bitmap_flags) == 0xe6);
static_assert(sizeof(particle_system) == 0x158);
static_assert(sizeof(particle_system_particle) == 0x80);
static_assert(offsetof(particle_system, ambient_color) == 0x48);
static_assert(offsetof(particle_system, type_states) == 0x58);
static_assert(offsetof(particle_system_type_state, first_particle) == 0x3c);
static_assert(offsetof(particle_system_particle, next_particle) == 0x4);
static_assert(offsetof(particle_system_particle, direction) == 0x34);

}  // namespace halo::effects
