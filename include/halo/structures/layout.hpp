/**
 * @file include/halo/structures/layout.hpp
 * Named constants, flag sets and layout checks for the structure bsp tag data the structures module reads.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include "halo/core/bit_array.hpp"
#include "halo/core/collision_flags.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/flags.hpp"
#include "halo/structures/limits.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/render/shader_types.hpp"
#include "halo/tags/flags.hpp"
#include "halo/structures/structures_types.hpp"

namespace halo::structures {

/** First byte of a decal's shader record: flags, bit 0x10 means the decal does not spawn from structure bsp placement. */
inline constexpr uint8_t k_decal_shader_skip_structure_flag = 0x10;

/** The cluster fog word: when this bit is set the low bits index a fog plane, otherwise a fog region. */
inline constexpr uint16_t k_cluster_fog_plane_flag = 0x8000;
inline constexpr uint16_t k_cluster_fog_index_mask = 0x7fff;

/** Rendered vertex type stored by some compiled maps for uncompressed vertices (treated like type 0). */
inline constexpr int16_t k_vertex_type_uncompressed_rendered_alias = 0xc;

/** Largest number of clusters the flood fills track; also the size of the per-cluster scratch tables. */
inline constexpr int32_t k_maximum_cluster_object_references = 0x800;
inline constexpr int32_t k_maximum_object_clusters = 0x40;
inline constexpr int32_t k_maximum_clip_polygon_points = 0x100;
inline constexpr int32_t k_maximum_query_surfaces = 0x1000;

/** Index of the (single) bit-bound byte pair of a compressed bsp3d node bound that means "no bound, use the parent". */
inline constexpr uint8_t k_compressed_bound_none = 0xff;
inline constexpr float k_compressed_bound_scale = 0.003921569f;

/** Draw-pass identifiers for collision queries the structures code starts. */
inline constexpr collision_test_flag k_structure_surface_query =
    collision_test_flag::structure_bsp | collision_test_flag::front_face;
inline constexpr collision_test_flag k_decal_placement_query =
    collision_test_flag::unstick | collision_test_flag::water_surface | collision_test_flag::structure_bsp |
    collision_test_flag::front_face;

/** All eight corners of a box are outside one plane. */
inline constexpr uint8_t k_box_all_corners_mask = 0xff;

/** Offset of the plane array pointer inside the owner record passed to plane_fetch_signed. */
inline constexpr size_t k_plane_owner_planes_offset = 0x10;

/** Signed plane/leaf indices keep their magnitude in the low 31 bits; the sign bit selects the flipped plane. */
inline constexpr uint32_t k_index_magnitude_mask = k_leaf_index_mask;

/** The six frustum plane bits of a point classification. */
inline constexpr uint16_t k_frustum_plane_mask = 0x3f;

/** Seed constant xored with the decal position bits before a placement test. */
inline constexpr uint32_t k_decal_placement_seed_xor = 0xdeadc0de;

/** Offset of the float the fog code scales when building a fog plane distance. */
inline constexpr size_t k_fog_plane_seed_offset = 4;

/** Offset of the base map tag id inside a shader_environment record. */
inline constexpr size_t k_shader_environment_base_map_tag_offset = 0x94;

/** Offset of the bitmap data count inside a bitmap tag. */
inline constexpr size_t k_bitmap_data_count_offset = 0x60;

/** Offset of the detail object globals' "render list is valid" byte. */
inline constexpr size_t k_detail_objects_valid_offset = 0x520e;

/** The leaf map tail of the structure bsp tag, starting at the unnamed dword in front of leaf_map_leaves. */
inline structure_bsp_leaf_map *leaf_map_of(ScenarioStructureBSP *bsp) noexcept {
    return reinterpret_cast<structure_bsp_leaf_map *>(&bsp->_pad_26c);
}

/** Byte offset of the first stored cluster frustum plane inside a visible-cluster record. */
inline constexpr size_t k_visible_cluster_screen_bounds_offset = 4;
inline constexpr size_t k_visible_cluster_frustum_offset = 0x14;

}  // namespace halo::structures

namespace halo::structures {

static_assert(sizeof(ScenarioStructureBSPCluster) == 0x68);
static_assert(sizeof(ScenarioStructureBSPClusterPortal) == 0x40);
static_assert(sizeof(BitmapData) == 0x30);
static_assert(sizeof(ScenarioStructureBSPMaterialUncompressedRenderedVertex) == 0x38);
static_assert(sizeof(ScenarioStructureBSPMaterialCompressedRenderedVertex) == 0x20);
static_assert(sizeof(ScenarioStructureBSPMaterialUncompressedLightmapVertex) == 0x14);
static_assert(offsetof(Bitmap, bitmap_data) + offsetof(TagReflexive, count) == k_bitmap_data_count_offset);
static_assert(offsetof(ShaderEnvironment, base_map) + offsetof(TagDependency, tag_id) == k_shader_environment_base_map_tag_offset);
static_assert(offsetof(ScenarioStructureBSPMaterial, centroid) == 0x1c);
static_assert(offsetof(ScenarioStructureBSPMaterial, ambient_color) == 0x28);
static_assert(offsetof(ScenarioStructureBSPMaterial, plane) == 0x9c);
static_assert(offsetof(ScenarioStructureBSPMaterial, rendered_vertices_type) == 0xb0);
static_assert(offsetof(ScenarioStructureBSPCluster, portals) == 0x5c);
static_assert(offsetof(ScenarioStructureBSPClusterPortal, back_cluster) == 2);
static_assert(offsetof(ScenarioStructureBSPClusterPortal, centroid) == 8);
static_assert(offsetof(ScenarioStructureBSPClusterPortal, bounding_radius) == 0x14);
static_assert(offsetof(ScenarioStructureBSP, collision_bsp) == 0xb0);
static_assert(offsetof(ScenarioStructureBSP, _pad_26c) == 0x26c);

}  // namespace halo::structures
