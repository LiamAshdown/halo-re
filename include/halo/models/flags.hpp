#pragma once

#include <cstddef>
#include <cstdint>
#include "halo/core/flags.hpp"

namespace halo::models {

/**
 * GBXModel.flags (the tag side ModelFlags bitfield).
 */
enum class model_tag_flags : uint32_t {
    none = 0,
    blend_shared_normals = 0x1,
    parts_have_local_nodes = 0x2,
    ignore_skinning = 0x4,
};

/**
 * Bits of the draw context flags word that render_model hands to the rasterizer.
 */
enum class model_draw_flags : uint32_t {
    none = 0,
    base = 0x1f,
    outside_fog_plane = 0x40,
    frustum_z = 0x80,
    parts_have_local_nodes = 0x100,
    ignore_skinning = 0x200,
};

}

namespace halo {
template <> struct enable_bit_flags<models::model_tag_flags> : std::true_type {};
template <> struct enable_bit_flags<models::model_draw_flags> : std::true_type {};
}
