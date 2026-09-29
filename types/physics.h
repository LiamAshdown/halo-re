#pragma once
// Blam physics / collision module (halo.exe 1.0.10 retail, 0x4ffde0..0x50b530, 80 Ghidra
// functions). Three layers live here:
//
//   1. collision_bsp queries (0x501340..0x503360). Sphere, segment and swept-sphere ("pill")
//      recursive descents of a ModelCollisionGeometryBSP, plus the 2D boundary helpers that
//      project a surface onto a plane and clip against its edge loop.
//   2. the physics_model (0x503360..0x504bb0). A scratch list of sphere / pill / polygon
//      collision proxies built out of whatever geometry a query touched, which the point and
//      ray tests then run against. This is the structure every routine that slides a mover
//      back out of the world works on.
//   3. object physics (0x5074b0..0x50b530). The per-tick force and torque integration of an
//      object that carries a Physics tag, one mass point at a time, and the point_physics
//      integrator for particles and other massless movers.
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries
// the layout it is used in preference to the decompiler, and the fact is called out:
//
//   - The whole collision BSP is a tag block that already exists in types/tags.h. Every
//     descent in this module indexes it through exactly the TagReflexive pointer offsets of
//     ModelCollisionGeometryBSP (size 0x60):
//       +0x04 bsp3d_nodes.pointer    stride 0x0c   ModelCollisionGeometryBSP3DNode
//       +0x10 planes.pointer         stride 0x10   ModelCollisionGeometryBSPPlane
//       +0x1c leaves.pointer         stride 0x08   ModelCollisionGeometryBSPLeaf
//       +0x28 bsp2d_references.ptr   stride 0x08   ModelCollisionGeometryBSP2DReference
//       +0x34 bsp2d_nodes.pointer    stride 0x14   ModelCollisionGeometryBSP2DNode
//       +0x40 surfaces.pointer       stride 0x0c   ModelCollisionGeometryBSPSurface
//       +0x4c edges.pointer          stride 0x18   ModelCollisionGeometryBSPEdge
//       +0x58 vertices.pointer       stride 0x10   ModelCollisionGeometryBSPVertex
//     0x005013a0 reads +0x04 and +0x10; 0x00501340 is handed the address of bsp2d_nodes so
//     its +0x04 is +0x34; 0x00501400 walks +0x40, +0x4c and +0x58. Surface +0x08 / +0x09 /
//     +0x0a are read as flags / breakable_surface / material by 0x00502140 and 0x005027a0,
//     which is exactly ModelCollisionGeometryBSPSurface. Nothing in the module contradicts
//     the tag definition, so no BSP node, plane, leaf, surface, edge or vertex struct is
//     redeclared here.
//
//   - The physics_model size is pinned by the binary twice over. The proxy strides and bases
//     that 0x00504260 and 0x00504bb0 use to walk back from a hit index are
//       spheres  base + 0x0008 + i * 0x1c   (0x00503360 writes it)
//       pills    base + 0x1c08 + i * 0x28   (0x00503490)
//       shapes   base + 0x4408 + i * 0x68   (0x005038a0)
//     with every writer refusing at 0x100 entries, so 0x08 + 0x100*0x1c = 0x1c08,
//     0x1c08 + 0x100*0x28 = 0x4408 and 0x4408 + 0x100*0x68 = 0xac08. 0x00507170 and
//     0x00507ac0 both reserve exactly 0xac08 bytes of stack for one.
//
//   - The Physics tag layout is taken from types/tags.h and confirmed field by field:
//     0x00507590 and 0x00507610 index mass_points with +0x74 count / +0x78 pointer, stride
//     0x80, reading +0x38 as PhysicsMassPoint.position and +0x68 as its radius;
//     0x00507cc0 reads +0x44 forward, +0x50 up, +0x2c mass, +0x34 density, +0x5c
//     friction_type, +0x60 and +0x64 the two friction scales, +0x20 the powered_mass_point
//     index, and the powered block through +0x6c / +0x70 with the same 0x80 stride.
//
//   - The PointPhysics tag drives 0x0050b530 entirely through named fields: flags +0x00
//     (bit 0x08 uses_simple_wind, 0x10 uses_damped_wind, 0x20 no_gravity, 0x04
//     collides_with_water_surface, 0x02 collides_with_structures, 0x01
//     flamethrower_particle_collision), +0x04 mass_scale, +0x08 water_gravity_scale,
//     +0x0c air_gravity_scale, +0x24 air_friction, +0x28 water_friction,
//     +0x2c surface_friction, +0x30 elasticity.
//
//   - The scenario structure BSP tag is reached through three fixed offsets that all land on
//     named ScenarioStructureBSP fields: +0xa8 collision_materials.pointer (stride 0x14,
//     +0x12 material), +0xe4 leaves.pointer (stride 0x10, +0x08 cluster) and +0x170
//     breakable_surfaces.pointer (stride 0x30 = ScenarioStructureBSPBreakableSurface, whose
//     centroid / radius / collision_surface_index at 0x00 / 0x0c / 0x10 are what 0x004fff20
//     reads).
//
//   - The DamageEffect tag confirms the breakable-surface damage path: 0x004ffde0 reads
//     +0x1d0 damage_lower_bound, +0x1d4 and +0x1d8 damage_upper_bound[2], and indexes the
//     per-material damage modifier array that starts at +0x200 (dirt, sand, stone, snow,
//     wood, ...) with a material type it takes out of damage_data +0x4c.
//
// collision_result is NOT declared here. types/projectiles.h already owns it, and every
// caller in this module (0x00505880, 0x00506040, 0x005055b0, 0x0050b530) agrees with that
// 0x50-byte layout. What this module adds to it is listed near the bottom of this file.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum physics_constants {
    k_maximum_collision_bsp_query_results = 0x100,  // every result list in this module refuses
                                                    //   at 256 (0x00501a10, 0x00501d20,
                                                    //   0x00502140, 0x005027a0)
    k_maximum_collision_bsp_query_planes = 0x80,    // the plane stack between the query header
                                                    //   and the projection cache is 0x200 bytes
    k_maximum_physics_model_spheres = 0x100,        // 0x00503360 refuses at 0x100
    k_maximum_physics_model_pills = 0x100,          // 0x00503490
    k_maximum_physics_model_shapes = 0x100,         // 0x005038a0
    k_maximum_physics_model_shape_vertices = 8,     // the vertex array is 0x28..0x68, and
                                                    //   0x005038a0 does NOT clamp the count it
                                                    //   copies out of the surface edge loop
    k_maximum_breakable_surfaces_per_bsp = 0x100,   // one 256-bit vector and 256 floats per BSP
    k_maximum_structure_bsps = 16,                  // INFERRED: 0x203 bytes of bit vector at
                                                    //   0x20 bytes each
    k_physics_displacement_direction_count = 17,    // 0x00507170 samples 0x11 offsets
    k_physics_collision_iterations = 3,             // 0x0050b530 resolves at most three bounces
    k_physics_integration_substeps = 4              // 0x005097e0 runs its collision loop 4x
} physics_constants;

// ---------------------------------------------------------------------------
// collision_bsp sphere query  (0x00501980 sets it up, 0x00501a10 / 0x00501c90 / 0x00501d20
// recurse through it)
// The record is 0x228 bytes of caller stack. 0x00501980 fills 0x08 / 0x0c / 0x10 from its
// three arguments; 0x00 (the bsp), 0x04 and 0x14 (the result block) arrive in registers and
// Ghidra lost the stores, but every reader agrees on what they hold.
// ---------------------------------------------------------------------------
typedef struct collision_bsp_sphere_query {
    void *bsp;                      // 0x000 ModelCollisionGeometryBSP *
    int16_t breakable_surface_count;// 0x004 a leaf breakable_surface index at or above this is
                                    //       treated as already broken
    int16_t unknown_006;            // 0x006
    uint32_t *breakable_surfaces;   // 0x008 bit vector, one bit per breakable surface; a clear
                                    //       bit with flag 0x08 on the leaf skips the leaf
    void *center;                   // 0x00c real_point3d *
    float radius;                   // 0x010
    void *result;                   // 0x014 collision_bsp_sphere_result *
    int32_t plane_count;            // 0x018 depth of the split-plane stack below
    int32_t planes[128];            // 0x01c plane index, bit 31 set when the sphere is on the
                                    //       back side; a bsp2d_reference is only descended
                                    //       while its plane is still on this stack
    int16_t projection_axis;        // 0x21c dominant axis of the plane being projected away
    uint8_t projection_sign;        // 0x21e second half of the k_projection_axes index
    uint8_t pad_21f;            // 0x21f
    float projected_center_i;       // 0x220 center projected onto the surviving axis pair
    float projected_center_j;       // 0x224
} collision_bsp_sphere_query;       // size 0x228

// Filled by 0x00501a10 (leaves) and 0x00501d20 (vertices, edges, surfaces). Every list is
// append-with-dedupe and silently drops entries past 256. 0x00503d90 turns the three geometry
// lists into physics_model proxies, in the order vertices then edges then surfaces.
typedef struct collision_bsp_sphere_result {
    int32_t surface_count;          // 0x000
    int32_t surfaces[256];          // 0x004
    int32_t edge_count;             // 0x404
    int32_t edges[256];             // 0x408
    int32_t vertex_count;           // 0x808
    int32_t vertices[256];          // 0x80c
    int32_t leaf_count;             // 0xc0c
    int32_t leaves[256];            // 0xc10
} collision_bsp_sphere_result;      // size 0x1010

// ---------------------------------------------------------------------------
// collision_bsp segment query  (0x00502060 sets it up, 0x00502140 recurses, 0x00502460 and
// 0x00502600 resolve which surface of a leaf the crossing point lands in)
// ---------------------------------------------------------------------------
// leaf_type is 1 for a double-sided leaf, 2 for a normal leaf and 3 for off the end of the
// tree. 0x00502140 computes it as (leaf->flags & 1) + 1, or 3 when the child index is -1.
typedef enum collision_bsp_leaf_type {
    _collision_bsp_leaf_type_double_sided = 1,
    _collision_bsp_leaf_type_normal = 2,
    _collision_bsp_leaf_type_none = 3
} collision_bsp_leaf_type;

typedef enum collision_bsp_segment_flags {
    _collision_bsp_test_front_face = 0x01,   // stop entering a solid leaf from outside
    _collision_bsp_test_back_face = 0x02,    // stop leaving a solid leaf
    _collision_bsp_test_double_sided = 0x04, // stop on a double-sided transition
    _collision_bsp_ignore_invisible = 0x08,  // surface flag 0x02 (invisible) is not a hit
    _collision_bsp_ignore_breakable = 0x10   // surface flag 0x08 (breakable) is not a hit
} collision_bsp_segment_flags;

typedef struct collision_bsp_segment_query {
    uint32_t flags;                 // 0x00 collision_bsp_segment_flags
    void *bsp;                      // 0x04 ModelCollisionGeometryBSP *
    int16_t breakable_surface_count;// 0x08
    int16_t pad_0a;             // 0x0a
    uint32_t *breakable_surfaces;   // 0x0c
    void *origin;                   // 0x10 real_point3d *
    void *delta;                    // 0x14 real_vector3d *, the whole swept segment
    void *result;                   // 0x18 collision_bsp_segment_result *; set by the caller
                                    //      in ECX, not by 0x00502060
    int32_t last_leaf;              // 0x1c leaf the walk was inside when it last crossed a
                                    //      plane, -1 before the first crossing
    uint8_t last_leaf_type;         // 0x20 collision_bsp_leaf_type of last_leaf
    uint8_t pad_21[3];          // 0x21
    int32_t crossing_plane;         // 0x24 plane index of the crossing being resolved
} collision_bsp_segment_query;      // size 0x28

typedef struct collision_bsp_segment_result {
    float t;                        // 0x00 fraction of delta consumed; 0x00502060 clamps the
                                    //      starting maximum into the 0..1 range
    void *plane;                    // 0x04 real_plane3d *, straight into the BSP plane block
    int32_t surface_index;          // 0x08
    int32_t plane_index;            // 0x0c surface->plane, sign bit set when the segment hit
                                    //      the back face
    uint8_t surface_flags;          // 0x10 ModelCollisionGeometryBSPSurfaceFlags
    int8_t breakable_surface_index; // 0x11
    int16_t material_index;         // 0x12 index into ScenarioStructureBSP collision_materials
    int32_t leaf_count;             // 0x14
    int32_t leaves[256];            // 0x18 every leaf the segment passed through, in order;
                                    //      once full the overflow is written over leaves[255]
} collision_bsp_segment_result;     // size 0x418

// ---------------------------------------------------------------------------
// collision_bsp swept-sphere ("pill") query  (0x00502730 sets it up, 0x005027a0 recurses,
// 0x00502d60 / 0x00502e70 / 0x00503050 / 0x00503290 handle the leaf boundary)
// Identical in spirit to the segment query, but the plane tests are widened by the radius and
// the projected origin AND direction are cached, because leaf edges are tested in 2D.
// ---------------------------------------------------------------------------
typedef struct collision_bsp_pill_query {
    void *bsp;                      // 0x000 ModelCollisionGeometryBSP *
    void *origin;                   // 0x004 real_point3d *
    void *delta;                    // 0x008 real_vector3d *
    float radius;                   // 0x00c
    void *result;                   // 0x010 collision_bsp_pill_result *; caller ECX
    int32_t plane_count;            // 0x014
    int32_t planes[128];            // 0x018 same encoding as the sphere query
    int16_t projection_axis;        // 0x218
    uint8_t projection_sign;        // 0x21a
    uint8_t unknown_21b;            // 0x21b
    float projected_origin_i;       // 0x21c
    float projected_origin_j;       // 0x220
    float projected_delta_i;        // 0x224
    float projected_delta_j;        // 0x228
} collision_bsp_pill_query;         // size 0x22c

typedef struct collision_bsp_pill_result {
    float t;                        // 0x000 deepest contact fraction found so far
    float plane_i;                  // 0x004 contact plane; 0x005027a0 copies the surface plane
    float plane_j;                  // 0x008 (negated for a back-face hit), 0x00502e70 builds
    float plane_k;                  // 0x00c one from the closest point on the edge and leaves
    float plane_d;                  // 0x010 d = FLT_MAX to mark "edge, not face"
    int32_t surface_index;          // 0x014
    int16_t unknown_018;            // 0x018
    int16_t material_index;         // 0x01a index into ScenarioStructureBSP collision_materials
    int32_t leaf_count;             // 0x01c
    int32_t leaves[256];            // 0x020
} collision_bsp_pill_result;        // size 0x420

// ---------------------------------------------------------------------------
// collision_bsp_boundary_clip  (0x005017f0)
// Clips a 2D parametric line against a projected surface edge loop. Both halves are seeded to
// -FLT_MAX and +FLT_MAX and the caller treats exit.t < enter.t as "no overlap".
// ---------------------------------------------------------------------------
typedef struct collision_bsp_boundary_hit {
    float t;                        // 0x00
    int32_t edge_index;             // 0x04 held in a float register, read back as an index
    int32_t surface_index;          // 0x08 edge->left_surface or right_surface, whichever is
                                    //      on the far side of the edge
} collision_bsp_boundary_hit;       // size 0x0c

typedef struct collision_bsp_boundary_clip {
    collision_bsp_boundary_hit enter; // 0x00
    collision_bsp_boundary_hit exit;  // 0x0c
} collision_bsp_boundary_clip;        // size 0x18

// ---------------------------------------------------------------------------
// physics_model  (0x00503360 / 0x00503490 / 0x005038a0 append, 0x00504260 and 0x00504bb0
// query, 0x00506440 and 0x00505200 fill it from the world and from objects)
// The scratch collision volume set a mover is tested against for one step. Every proxy starts
// with the same six bytes of provenance, which is what 0x00504260 and 0x00504bb0 copy into
// the contact record so the caller can find out what it hit.
// ---------------------------------------------------------------------------
typedef enum physics_model_shape_type {
    _physics_model_shape_sphere = 0, // 0x00504260 / 0x00504bb0 dispatch on the list index
    _physics_model_shape_pill = 1,
    _physics_model_shape_polygon = 2
} physics_model_shape_type;

typedef struct physics_model_sphere {
    uint32_t object_index;          // 0x00 datum_index of the owning object, -1 for the world
    int32_t surface_index;          // 0x04 collision BSP surface, -1 when object_index is set
    uint8_t surface_flags;          // 0x08
    int8_t breakable_surface_index; // 0x09
    int16_t material_type;          // 0x0a global material type index, 0xffff when unknown
    float center_x;                 // 0x0c
    float center_y;                 // 0x10
    float center_z;                 // 0x14 the second sphere of a pair sits at center_z minus
                                    //      the caller height, which is how a standing capsule
                                    //      is approximated
    float radius;                   // 0x18
} physics_model_sphere;             // size 0x1c

typedef struct physics_model_pill {
    uint32_t object_index;          // 0x00
    int32_t surface_index;          // 0x04
    uint8_t surface_flags;          // 0x08
    int8_t breakable_surface_index; // 0x09
    int16_t material_type;          // 0x0a
    float origin_x;                 // 0x0c
    float origin_y;                 // 0x10
    float origin_z;                 // 0x14
    float extent_i;                 // 0x18 origin + extent is the far end of the capsule axis
    float extent_j;                 // 0x1c
    float extent_k;                 // 0x20
    float radius;                   // 0x24
} physics_model_pill;               // size 0x28

typedef struct physics_model_shape {
    uint32_t object_index;          // 0x00
    int32_t surface_index;          // 0x04
    uint8_t surface_flags;          // 0x08
    int8_t breakable_surface_index; // 0x09
    int16_t material_type;          // 0x0a
    float plane_i;                  // 0x0c the supporting plane of the polygon
    float plane_j;                  // 0x10
    float plane_k;                  // 0x14
    float plane_d;                  // 0x18 pushed out by thickness * plane_k when the source
                                    //      surface faces downward
    float thickness;                // 0x1c a point is inside while 0 <= distance < thickness
    int16_t projection_axis;        // 0x20 dominant axis of the plane
    uint8_t projection_sign;        // 0x22
    uint8_t pad_23;             // 0x23
    int32_t vertex_count;           // 0x24
    float vertices[8][2];           // 0x28 the boundary projected onto the surviving axis pair
} physics_model_shape;              // size 0x68

typedef struct physics_model {
    int16_t sphere_count;           // 0x0000 the three counts are indexed as counts[type]
    int16_t pill_count;             // 0x0002
    int16_t shape_count;            // 0x0004
    int16_t unknown_0006;           // 0x0006
    physics_model_sphere spheres[256]; // 0x0008
    physics_model_pill pills[256];     // 0x1c08
    physics_model_shape shapes[256];   // 0x4408
} physics_model;                    // size 0xac08

// The one record both physics_model query entry points produce. 0x00504260 (deepest point
// overlap) leaves the point untouched and puts the penetration depth in t; 0x00504bb0
// (closest ray hit) fills the point and puts the hit fraction in t, and writes t = 1 with
// point = origin + delta when nothing was hit. 0x005067b0 keeps an array of these, one per
// slide iteration, at this same 0x2c stride.
typedef struct physics_model_contact {
    float t;                        // 0x00 penetration depth, or segment fraction
    float point_x;                  // 0x04
    float point_y;                  // 0x08
    float point_z;                  // 0x0c
    float plane_i;                  // 0x10 separating normal, pointing at the mover
    float plane_j;                  // 0x14
    float plane_k;                  // 0x18
    float plane_d;                  // 0x1c
    uint32_t object_index;          // 0x20 copied out of the proxy that won
    int32_t surface_index;          // 0x24
    uint8_t surface_flags;          // 0x28
    int8_t breakable_surface_index; // 0x29
    int16_t material_type;          // 0x2a
} physics_model_contact;            // size 0x2c

// ---------------------------------------------------------------------------
// object collision context  (built by 0x00504e10, consumed by 0x00504e90, 0x00504f60,
// 0x005050b0 and 0x00505200)
// Everything needed to run a BSP query against the collision model of one object: which
// object, its ModelCollisionGeometry, the per-region permutation bytes that pick a BSP inside
// each collision node, and the node matrix array. 0x00504e10 fails when the Object tag has no
// collision_model, that is when tag offset 0x7c is -1. 0x005055b0 reserves 16 bytes for one.
// ---------------------------------------------------------------------------
typedef struct object_collision_context {
    uint32_t object_index;          // 0x00 datum_index
    void *definition;               // 0x04 ModelCollisionGeometry *
    uint8_t *region_permutations;   // 0x08 object + 0x180, one byte per region
    void *nodes;                    // 0x0c real_matrix4x3 *, at object + object nodes offset
} object_collision_context;         // size 0x10

// Result of a per-node query (0x00504f60 SEGMENT, 0x005050b0 PILL -- the two were labelled the
// other way round in phase 2 and in an earlier copy of this comment; 0x00504f60's BSP callee is
// 0x00502060, the segment init, and 0x005050b0's is 0x00502730, the pill init). The three
// indices say which collision node, region and permutation won, and the rest is the ordinary BSP
// segment result, which is why the pill variant cannot store its whole 0x420-byte pill result
// here and keeps only the fraction.
typedef struct object_node_collision_result {
    int16_t node_index;             // 0x00 index into ModelCollisionGeometry nodes
    int16_t region_index;           // 0x02 node->region
    int16_t permutation_index;      // 0x04 clamped to node->bsps.count - 1
    int16_t unknown_06;             // 0x06
    collision_bsp_segment_result segment; // 0x08
} object_node_collision_result;     // size 0x420

// ---------------------------------------------------------------------------
// object physics context  (built by 0x005074b0, consumed by 0x00507590, 0x00507610,
// 0x00507790, 0x00507cc0, 0x005090c0, 0x005097e0 and 0x00509e80)
// 0x005074b0 fails when the Object tag has no physics reference, that is when tag offset 0x8c
// is -1. The matrix is the model space to world transform with the scale forced to 1.0, built
// from object_get_position and object_get_orientation plus a cross product for the left
// column, and its translation is shifted by the centre of mass of the definition (the stored
// position of a physics object is its centre of mass).
// 0x005055b0 reserves exactly 60 bytes for one, which fixes the size at 0x3c.
// ---------------------------------------------------------------------------
typedef struct object_physics_context {
    uint32_t object_index;          // 0x00 datum_index
    void *definition;               // 0x04 Physics *
    float scale;                    // 0x08 always 1.0 here; 0x005090c0 skips the multiply
                                    //      while it still is
    float forward_i;                // 0x0c the three basis columns, in matrix4x3 order
    float forward_j;                // 0x10
    float forward_k;                // 0x14
    float left_i;                   // 0x18
    float left_j;                   // 0x1c
    float left_k;                   // 0x20
    float up_i;                     // 0x24
    float up_j;                     // 0x28
    float up_k;                     // 0x2c
    float position_x;               // 0x30
    float position_y;               // 0x34
    float position_z;               // 0x38
} object_physics_context;           // size 0x3c

// Output of 0x00507610: a ray tested against the mass point spheres in object space, with the
// fraction and the hit plane transformed back to world space.
typedef struct object_physics_ray_result {
    float t;                        // 0x00 FLT_MAX until something is hit
    float plane_i;                  // 0x04
    float plane_j;                  // 0x08
    float plane_k;                  // 0x0c
    float plane_d;                  // 0x10
} object_physics_ray_result;        // size 0x14

// ---------------------------------------------------------------------------
// mass_point_state  (0x00507cc0 zeroes definition mass_points.count * 0x130 bytes and then
// fills one record per mass point; 0x005097e0 integrates the totals; 0x00507ac0 fills the
// contact half; 0x00507c00 blends each friction triple)
// The 0x130 stride is read straight out of the memset at the top of 0x00507cc0, which clears
// mass_points.count * 0x130 bytes, and the last field written is the torque at 0x124..0x12f,
// which closes the record exactly.
// Each of the three friction blocks is the nine-float layout 0x00507c00 expects: the blended
// result first, then the two candidate axes it mixes, chosen by
// PhysicsMassPoint.friction_type and scaled by friction_parallel_scale and
// friction_perpendicular_scale.
// ---------------------------------------------------------------------------
typedef enum mass_point_flags {
    _mass_point_at_rest_bit = 0x01,           // squared velocity below 0.0011111
    _mass_point_ground_contact_bit = 0x02,    // ground_depth > 0
    _mass_point_on_ground_surface_bit = 0x04, // 0x00507ac0: the contact is not a breakable
                                              //   surface and is either the world or an object
                                              //   whose type is scenery
    _mass_point_water_contact_bit = 0x08,     // water_depth > 0
    _mass_point_antigrav_bit = 0x10           // the antigrav branch of the powered mass point
                                              //   ran this tick
} mass_point_flags;

typedef struct mass_point_state {
    uint32_t flags;                 // 0x000 mass_point_flags
    float position_x;               // 0x004 definition position transformed to world
    float position_y;               // 0x008
    float position_z;               // 0x00c
    float forward_i;                // 0x010 definition forward rotated to world
    float forward_j;                // 0x014
    float forward_k;                // 0x018
    float unknown_01c[3];           // 0x01c zeroed and never written by this module; the gap
                                    //       where a left vector would sit
    float up_i;                     // 0x028 definition up rotated to world
    float up_j;                     // 0x02c
    float up_k;                     // 0x030
    int32_t leaf_index;             // 0x034 structure BSP leaf containing position, -1 outside
    int16_t cluster_index;          // 0x038 the cluster of that leaf, 0xffff outside
    int16_t pad_03a;            // 0x03a
    float offset_x;                 // 0x03c position minus object position, the torque arm
    float offset_y;                 // 0x040
    float offset_z;                 // 0x044
    float velocity_i;               // 0x048 object velocity plus angular_velocity cross offset
    float velocity_j;               // 0x04c
    float velocity_k;               // 0x050
    float tangential_velocity_i;    // 0x054 velocity with the resting plane normal component
    float tangential_velocity_j;    // 0x058 removed, plus the powered ground friction push
    float tangential_velocity_k;    // 0x05c
    float resting_plane_i;          // 0x060 0x00507ac0 seeds it from the constant at 0x0069c53c
    float resting_plane_j;          // 0x064 and overwrites it with the deepest contact plane
    float resting_plane_k;          // 0x068
    float resting_plane_d;          // 0x06c
    int16_t material_type;          // 0x070 global material type of that contact, 0xffff when
                                    //       nothing was hit
    int16_t pad_072;            // 0x072
    float ground_depth;             // 0x074 radius minus the distance above resting_plane; a
                                    //       positive value means the sphere is dug in
    uint32_t unknown_078;           // 0x078 zeroed, never written
    float water_depth;              // 0x07c how far below the water surface the point is
    float ground_normal_magnitude;  // 0x080 the spring plus damper reaction along the plane
    float ground_normal_force_i;    // 0x084 magnitude times the resting plane normal
    float ground_normal_force_j;    // 0x088
    float ground_normal_force_k;    // 0x08c
    float ground_friction_force[3];         // 0x090 blended result
    float ground_friction_parallel[3];      // 0x09c
    float ground_friction_perpendicular[3]; // 0x0a8
    float buoyancy_magnitude;       // 0x0b4 mass over density (that is, volume) times
                                    //       the tag water_density, faded in by water_depth over
                                    //       the tag water_depth
    float buoyancy_force_i;         // 0x0b8 always (0, 0, buoyancy_magnitude)
    float buoyancy_force_j;         // 0x0bc
    float buoyancy_force_k;         // 0x0c0
    float water_friction_force[3];          // 0x0c4
    float water_friction_parallel[3];       // 0x0d0
    float water_friction_perpendicular[3];  // 0x0dc
    float air_friction_force[3];            // 0x0e8
    float air_friction_parallel[3];         // 0x0f4
    float air_friction_perpendicular[3];    // 0x100
    float powered_force_i;          // 0x10c lift, thrust and antigrav from the powered mass
    float powered_force_j;          // 0x110 point, accumulated across all four branches
    float powered_force_k;          // 0x114
    float total_force_i;            // 0x118 ground normal plus all three frictions plus
    float total_force_j;            // 0x11c buoyancy plus powered, summed at the end of the
    float total_force_k;            // 0x120 mass point pass
    float torque_i;                 // 0x124 offset cross total_force
    float torque_j;                 // 0x128
    float torque_k;                 // 0x12c
} mass_point_state;                 // size 0x130

// ---------------------------------------------------------------------------
// powered_mass_point_state  (the array the caller of 0x00507840 owns; 0x00507cc0 indexes it
// with PhysicsMassPoint.powered_mass_point at stride 0x60)
// The seven leading scalars line up one for one with the seven PhysicsPoweredMassPointFlags
// bits: a branch only runs when the flag is set AND the matching scalar is non-zero, and
// 0x00507cc0 reads them in flag-bit order.
// The trailing matrix is built by 0x00507840 with matrix4x3_from_quaternion and then
// transposed in place (it swaps 3x3 entries 01/10, 02/20 and 12/21), and 0x00507cc0
// multiplies the object matrix by it to get the driven orientation of the mass point.
// ---------------------------------------------------------------------------
typedef struct powered_mass_point_state {
    float ground_friction;          // 0x00 flag 0x01
    float water_friction;           // 0x04 flag 0x02
    float air_friction;             // 0x08 flag 0x04
    float water_lift;               // 0x0c flag 0x08
    float air_lift;                 // 0x10 flag 0x10
    float thrust;                   // 0x14 flag 0x20
    float antigrav;                 // 0x18 flag 0x40
    uint8_t unknown_1c[0x10];       // 0x1c untouched by this module
    float matrix_scale;             // 0x2c the real_matrix4x3 0x00507840 writes
    float matrix[3][3];             // 0x30
    float matrix_position[3];       // 0x54
} powered_mass_point_state;         // size 0x60

// ---------------------------------------------------------------------------
// breakable_surface_globals  (0x004ffde0 damages one, 0x004fff20 resets the ones an explosion
// reaches, 0x00500090 spawns the break effect, 0x00501980 and 0x00502060 are handed the bit
// vector of the current BSP so a broken surface stops blocking)
// The bit vector deliberately starts at offset 1, which is why every reader writes
// base + 1 + (bsp_index * 8 + (surface >> 5)) * 4. The 0x203-byte gap before the health array
// is what fixes k_maximum_structure_bsps at 16.
// ---------------------------------------------------------------------------
typedef struct breakable_surface_globals {
    uint8_t initialized;            // 0x000 every entry point bails when this is 0
    uint32_t active[16][8];         // 0x001 one bit per breakable surface, set while the
                                    //       surface is still intact. UNALIGNED on purpose.
    uint8_t unknown_201[3];         // 0x201 padding up to the health array
    float health[16][256];          // 0x204 remaining vitality; crossing zero clears the bit
                                    //       and fires the break effect at 0x00500090
} breakable_surface_globals;        // size 0x4204

// ---------------------------------------------------------------------------
// point_physics  (0x0050b530)
// One tick of a massless mover driven by a PointPhysics tag: wind, gravity, medium drag, then
// up to three collision, bounce and friction resolutions against the world through the shared
// collision_result path. The return value is a bit field, not an index.
// ---------------------------------------------------------------------------
typedef enum point_physics_result_flags {
    _point_physics_in_air_bit = 0x01,           // the medium test said air
    _point_physics_in_water_bit = 0x02,         // the medium test said water
    _point_physics_collided_bit = 0x04,         // hit a structure surface, or an object while
                                                //   the flamethrower-particle flag is set
    _point_physics_hit_water_surface_bit = 0x08 // collision_result type was 0
} point_physics_result_flags;

// ---------------------------------------------------------------------------
// scalar range helpers  (0x0050b290, 0x0050b2f0, 0x0050b370, 0x0050b460, 0x0050b4d0)
// A small family that moves one float toward a target inside a range, with optional
// wraparound. The range record is read UPPER FIRST: the code computes the span as
// range[0] - range[1] and clamps the low side against range[1], which is the opposite order
// from real_bounds in types/math.h, so it is declared here rather than reused.
// The rate record is the four scalars 0x0050b370 multiplies the signed step by.
// UNSURE which subsystem owns these five; see out/phase4/physics_types_notes.md.
// ---------------------------------------------------------------------------
typedef struct physics_scalar_range {
    float upper;                    // 0x00
    float lower;                    // 0x04
} physics_scalar_range;             // size 0x08

typedef struct physics_scalar_rates {
    float maximum_positive;         // 0x00 the step is clamped to rate times this going up
    float maximum_negative;         // 0x04 and to minus rate times this going down
    float acceleration_positive;    // 0x08 scales the step while the value is already moving
    float acceleration_negative;    // 0x0c the same, in the other direction
} physics_scalar_rates;             // size 0x10

// ---------------------------------------------------------------------------
// collision_test_movement_segment_flags  (0x00505880's param_1, extending
// collision_bsp_segment_flags past its low five face-side bits)
// Bit 0x20: also apply the structure-BSP segment clip result to the caller's collision_result.
// Bit 0x40: also test the destination leaf's cluster fog plane for a water-surface crossing.
// Bit 0x80: also walk nearby objects' collision. Bits 0x100..0x80000 are forwarded to
// 0x005055b0 as its own object-type test mask, defaulting to "every type" (0xfff) when the
// caller passes none, exactly like the low five bits default to "front and back face".
// Bit 0x100000: after a hit, nudge the result point back out of solid geometry.
// ---------------------------------------------------------------------------
typedef enum collision_test_movement_segment_flags {
    _collision_test_flag_structure_bsp = 0x20,
    _collision_test_flag_water_surface = 0x40,
    _collision_test_flag_nearby_objects = 0x80,
    _collision_test_object_type_mask_default = 0xfff00,
    _collision_test_flag_unstick = 0x100000
} collision_test_movement_segment_flags;

// ---------------------------------------------------------------------------
// object_physics_tick_accumulator  (0x00507840's local_54..local_40)
// The torque/force pair 0x00507840 builds on its stack, hands to
// object_physics_integrate_and_test_at_rest (0x005097e0) as one block, and which
// object_physics_compute_mass_point_forces (0x00507cc0) fills through its last two parameters.
// Torque comes FIRST in stack order.
// ---------------------------------------------------------------------------
typedef struct object_physics_tick_accumulator {
    real_vector3d torque;           // 0x00 local_54/50/4c
    real_vector3d force;            // 0x0c local_48/44/40
} object_physics_tick_accumulator;  // size 0x18

// ---------------------------------------------------------------------------
// physics_point_walk_state  (0x005070d0's param_1, built by 0x00507170)
// The remaining step fraction plus the candidate position that 0x005070d0 backs off along
// -step_direction, 0.03125 at a time, until 0x00505490 reports it clear.
// ---------------------------------------------------------------------------
typedef struct physics_point_walk_state {
    float t;                        // 0x00 remaining fraction of the step still available
    real_point3d position;          // 0x04 current candidate position, tested each iteration
} physics_point_walk_state;         // size 0x10

// ---------------------------------------------------------------------------
// what this module adds to collision_result (types/projectiles.h, size 0x50)
// ---------------------------------------------------------------------------
// 0x00505880 is the producer every other consumer in the game reads, so its stores name the
// fields projectiles.h had to leave as unknowns:
//   0x00 type       0 is also produced here, by the fog-plane / water-surface test at the top
//                   of 0x00505880, which is why point_physics treats 0 as "hit water"
//   0x04 int32_t    the FIRST leaf of the segment walk, that is leaves[0] of the segment
//                   result
//   0x08 int16_t    the cluster of that leaf
//   0x0c int32_t    the LAST leaf of the walk, the leaf the endpoint is in
//   0x10 int16_t    the cluster of that leaf   (0x0c and 0x10 are the projectiles.h "leaf")
//   0x30 float      the d of the contact plane, completing the real_plane3d that starts at
//                   0x24
//   0x3c int16_t    collision node region index  (0x005055b0, from 0x00504f60)
//   0x3e int16_t    collision NODE index, not a marker index
//   0x40 int16_t    collision node permutation index; 0x42 is left untouched
//   0x48 int32_t    surface->plane, sign bit set on a back-face hit
//   0x4d uint8_t    the breakable_surface index of the surface
//   0x4e int16_t    index into ScenarioStructureBSP collision_materials, -1 for an object
// The material_type at 0x34 is resolved differently per source: from
// collision_materials[0x4e].material for the world, and through
// ModelCollisionGeometry materials (tag offset 0x238, stride 0x48, field 0x24) for an object,
// which is what 0x00505330 and 0x00507a40 do.

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// global 0x006b8d78: breakable_surface_globals *breakable_surface_globals
// global 0x0069e8d8: int16_t global_structure_bsp_index   the row of both arrays in use
// global 0x0069c460: float k_physics_displacement_directions[17][3]   0x00507170 samples this
//                    ring of unit offsets looking for a free spot for a stuck mover
// global 0x0069c53c: float k_default_resting_plane[4]      0x00507ac0 seeds every mass point
//                    resting plane with it before running the query
// global 0x0069c52c: float k_physics_gravity               scales the gravity term in
//                    0x00507cc0, 0x005090c0, 0x00509e80 and 0x0050b530
// global 0x0069c538: float k_physics_collision_damping     0x005090c0 divides the gravity
//                    constant by it to size the object-vs-object repulsion. UNSURE
// global 0x0069c54c: float k_physics_impact_damage_scale   0x00508b70. UNSURE
// global 0x006b8d7c: float k_water_density                 0x0050b530
// global 0x006b8d80: float k_air_density                   0x0050b530
// global 0x0071cfbc: uint8_t physics_disable_integration   0x005097e0 skips its whole sub-step
//                    loop when set, so this is a debug switch
// global 0x00689470: uint8_t breakable_surface_effects_enabled   0x00500090 bails on 0
// global 0x00721e4c: uint8_t material_table_warning_issued  0x00507cc0 latches it once when a
//                    mass point reports an out-of-range material index, and stamps -1 into
//                    0x006e3578 at the same time
//
// Globals this module reads but does not own, named by the module that does:
//   0x00746f90 / 0x00746f98 / 0x00746f9c / 0x00746fa0  the structure BSP globals: the
//     collision bsp pointer, the ScenarioStructureBSP tag data and the Globals tag data
//   0x008603b0  the object header data_array; 0x008603cc / 0x008603d0 / 0x008603d4 the
//     cluster-to-object lists and the per-gather stamp
//   0x0087bc14  the tag instance array (types/cache.h)
//   0x0065c29c  k_projection_axes (types/math.h); every 3D to 2D projection here indexes it
//   0x00719cd4  the cseries random seed, advanced by 0x00500090
//   0x006e3f01 / 0x006e3f04 / 0x006e3f08  the cluster visit stamps that
//     object_collect_in_clusters shares with 0x00505880 and 0x00506440
//   0x006721xx..0x006731xx and 0x0065c2xx  MSVC .rdata floating-point literals and the
//     0x7fffffff and 0x80000000 abs and negate masks, not engine state
#pragma pack(pop)
