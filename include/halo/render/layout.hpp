/**
 * @file include/halo/render/layout.hpp
 * Named constants, flag sets and layout checks for the render module (frustum, particles, contrails, screen effects).
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include "halo/core/bit_array.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/objects/flags.hpp"
#include "halo/render/d3d9.hpp"
#include "halo/structures/limits.hpp"
#include "halo/render/shader_types.hpp"
#include "halo/tags/flags.hpp"

namespace halo::render {

using halo::operator|;
using halo::operator&;
using halo::operator~;
using halo::has;
using halo::to_bits;

/** One bit per side plane of the render frustum; all six set means "outside every plane". */
enum class frustum_plane_bit : uint8_t {
    none = 0,
    left = 0x01,
    right = 0x02,
    top = 0x04,
    bottom = 0x08,
    near_plane = 0x10,
    far_plane = 0x20,
    all = 0x3f,
};

/** Bits describing on which side of a box a frustum vertex lies (used by the separating-axis check). */
enum class box_side_bit : uint8_t {
    none = 0,
    x_lower = 0x01,
    x_upper = 0x02,
    y_upper = 0x04,
    y_lower = 0x08,
    z_lower = 0x10,
    z_upper = 0x20,
    all = 0x3f,
};

/** particle.flags. */
enum class particle_flag : uint16_t {
    none = 0,
    animating_backwards = 0x0001,
    at_rest = 0x0002,
    mirror_horizontal = 0x0004,
    mirror_vertical = 0x0008,
    third_person_only = 0x0010,
    first_person_only = 0x0020,
    first_person = 0x0040,
};

/** build_sprite_data.flags. */
enum class sprite_batch_flag : uint32_t {
    none = 0,
    screen_space = 0x01,
    first_person = 0x02,
    particles = 0x04,
};

/** Draw flags rasterizer_transparent_object_append takes for a sprite batch. */
inline constexpr uint32_t k_transparent_append_sprite_flag = 0x20;
inline constexpr uint32_t k_transparent_append_first_person_flag = 0x80;
inline constexpr uint32_t k_sprite_batch_first_person_shift = 6;

/** contrail_point.flags. */
enum class contrail_point_flag : uint8_t {
    none = 0,
    skip_render = 0x01,
    in_transition = 0x02,
    expired = 0x04,
};

/** Largest number of objects the render pass keeps in its candidate list. */
inline constexpr int32_t k_maximum_rendered_objects = 0x100;

/** Size of the model ambient reflection tint block carved out of the game state. */
inline constexpr int32_t k_model_ambient_reflection_tint_size = 0x10;

/** Result of an unbounded level of detail (cinematic objects report the maximum size). */
inline constexpr float k_maximum_level_of_detail_pixels = 3.4028235e+38f;

/** The 16 floats of the camera facing basis followed by the plane (i, j, k, d) built by facing_frame_build. */
struct facing_frame {
    float basis[16];
    float plane[4];
};

/** Offset of the bitmap data pointer inside a bitmap tag. */
inline constexpr size_t k_bitmap_data_pointer_offset = 0x64;

/** Left margin of the frame statistics graph and the screen size its text bounds are clamped to. */
inline constexpr int32_t k_frame_graph_margin = 0x40;
inline constexpr int16_t k_debug_screen_width = 0x280;
inline constexpr int16_t k_debug_screen_height = 0x1e0;

/** Packed ARGB colors used by the debug graphs. */
inline constexpr uint32_t k_argb_white = 0xffffffff;
inline constexpr uint32_t k_argb_yellow = 0xffffff00;

/** Method table index of the release call made on the rasterizer effect pool object. */
inline constexpr uint32_t k_effect_release_slot = 0x108 / 4;

/** Bit pattern of 1.0f, tested to skip the scale of a marker transform. */
inline constexpr uint32_t k_float_one_bits = 0x3f800000;

/** Device versions (pixel shader version encoding) the draw code gates features on. */
inline constexpr uint32_t k_device_version_lightmap_pass = 0xffff0104;
inline constexpr uint32_t k_device_version_mirror_pass = 0xffff0100;

/** Number of frame statistics graph vertices and layout used by the debug graphs. */
inline constexpr int32_t k_frame_graph_vertex_count = 0x200;
inline constexpr int32_t k_frame_graph_history_length = 0x3c;

}  // namespace halo::render

namespace halo {
template <> struct enable_bit_flags<render::frustum_plane_bit> : std::true_type {};
template <> struct enable_bit_flags<render::box_side_bit> : std::true_type {};
template <> struct enable_bit_flags<render::particle_flag> : std::true_type {};
template <> struct enable_bit_flags<render::sprite_batch_flag> : std::true_type {};
template <> struct enable_bit_flags<render::contrail_point_flag> : std::true_type {};
}  // namespace halo

namespace halo::render {

static_assert(sizeof(facing_frame) == 0x50);

}  // namespace halo::render
