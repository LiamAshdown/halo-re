#pragma once
// Blam structure BSP module (halo.exe 1.0.10 retail, 0x53f150..0x555330, 48 Ghidra functions).
// Everything here hangs off one resident tag: the ScenarioStructureBSP whose tag data pointer
// lives in the global at 0x00746f9c. types/tags.h already carries the whole layout of that tag, so
// this module barely owns any structure of its own -- what it owns is the per-frame visibility
// state derived from the tag, plus the game-state block the detail objects render out of.
//
// Four layers live in the address range:
//
//   1. cluster reference lists and the cluster flood fill (0x551e30..0x552260, 0x554cb0..0x554fa0).
//      The reference-list element type is object_cluster_reference and the three-global group it
//      chains off is object_placement_cursor, both already in types/objects.h; only the flood
//      stamps below are new.
//   2. the detail object render pass (0x5522d0..0x552780) which owns detail_object_globals.
//   3. the debug / picked-polygon BSP surface visualisation (0x5527f0..0x552de0).
//   4. the per-frame visibility pass (0x5530d0..0x555330): the leaf and cluster of the camera, the
//      portal flood, the cluster and surface bitsets, and the fog / ambient environment lookup.
//
// Offsets in comments are byte offsets from the struct base. Where the binary carries the layout
// it is used in preference to the decompiler and the fact is called out:
//
//   - Every cluster, leaf, lightmap, material, portal, mirror, fog and detail-object walk in this
//     module indexes ScenarioStructureBSP through exactly the TagReflexive offsets types/tags.h
//     already declares. The pointer offsets the code uses, all confirmed by the strides:
//       +0x0b4 collision_bsp.pointer       ModelCollisionGeometryBSP
//       +0x0c8 world_bounds_x/y/z          the real_rectangle3d that decompresses node bounds
//       +0x0e0 leaves.count  +0x0e4 ptr    stride 0x10  ScenarioStructureBSPLeaf
//       +0x0ec leaf_surfaces.count +0x0f0  stride 0x08  ScenarioStructureBSPSurfaceReference
//       +0x0f8 surfaces.count +0x0fc ptr   stride 0x06  ScenarioStructureBSPSurface
//       +0x104 lightmaps.count +0x108 ptr  stride 0x20  ScenarioStructureBSPLightmap
//       +0x134 clusters.count +0x138 ptr   stride 0x68  ScenarioStructureBSPCluster
//       +0x14c cluster_data.pointer        the cluster PVS bit rows, see below
//       +0x158 cluster_portals.pointer     stride 0x40  ScenarioStructureBSPClusterPortal
//       +0x17c fog_planes.pointer          stride 0x20  ScenarioStructureBSPFogPlane
//       +0x188 fog_regions.pointer         stride 0x28  ScenarioStructureBSPFogRegion
//       +0x194 fog_palette.pointer         stride 0x88  ScenarioStructureBSPFogPalette
//       +0x1fc background_sound_palette.count +0x200 ptr  stride 0x74
//       +0x20c sound_environment_palette.pointer         stride 0x50
//       +0x24c detail_objects.count +0x250 ptr  stride 0x40  ScenarioStructureBSPDetailObjectData
//       +0x258 runtime_decals.count +0x25c ptr  stride 0x10  ScenarioStructureBSPRuntimeDecal
//       +0x26c the leaf map, see structure_bsp_leaf_map
//   - ScenarioStructureBSPRuntimeDecal is confirmed field by field: 0x005530d0 reads the packed
//     decal_type byte at +0x0c and the signed yaw / pitch bytes at +0x0e and +0x0f, turning the
//     latter into a direction with the 0x02473695 and 0x012368475 radians-per-count scales.
//   - ScenarioStructureBSPGlobalDetailObjectCell is confirmed the same way by the two binary
//     searches at 0x552710 / 0x552780 (stride 0x20, key = the three int16 at +0x00) and by
//     0x5522d0 reading valid_layers_flags at +0x08, start_index at +0x0c and count_index at +0x10.
//   - SoundEnvironment (size 0x48, priority at +0x04) is exactly the 18 dwords 0x53f150 copies
//     and the 12 reals at +0x08..+0x34 it rate-limits; that is what proved 0x53f150 belongs to
//     the sound module and not here.
//
// Globals are listed at the bottom of the file.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// module constants
// ---------------------------------------------------------------------------
// The cluster count limit is the one types/objects.h calls k_maximum_clusters (0x200) and
// types/effects.h calls k_decal_grid_clusters; every per-cluster table below is sized by it.
typedef enum structure_bsp_constants {
    k_maximum_visible_clusters = 0x80,     // 0x007c3390 is 0xd000 bytes == 0x80 * 0x1a0
    k_maximum_visible_surfaces = 0x4000,   // 0x00553920 / 0x00553a70 stop at 0x3fff, and the
                                           // index array at 0x00850398 is 0x10000 bytes
    k_maximum_visible_surface_bits = 0x1000,  // dwords; 0x00553d80 zeroes a 0x4000-byte local
                                           // bitset the same way it zeroes the global one
    k_maximum_cluster_flood_results = 0x40,   // 0x00551f00 caps the flood fill at 64 clusters
    k_maximum_polygon2d_points = 0x100,    // the 0x100 handed to polygon2d_clip_to_planes
    k_maximum_detail_object_layers = 0x20, // one bit per layer in cell.valid_layers_flags
    k_maximum_detail_object_batches_per_layer = 0x1b,
    k_detail_object_cell_size = 8          // 0x5522d0 scales the render position by 0.125
} structure_bsp_constants;

// The 0 / 1 / 2 result aabb_overlap_classify (0x5541b0), frustum_planes_classify_box (0x554260)
// and every caller that takes the min of the two agree on. 2 means "no clipping needed", which
// is why bsp3d_node_query_recursive skips both tests once it sees it.
typedef enum structure_bsp_overlap {
    _structure_bsp_overlap_none = 0,
    _structure_bsp_overlap_partial = 1,
    _structure_bsp_overlap_contained = 2
} structure_bsp_overlap;

// structure_fog_environment.plane_mode, written by 0x00555330.
typedef enum structure_fog_plane_mode {
    _structure_fog_plane_none = 0,          // no fog at all
    _structure_fog_plane_bounded = 1,       // cluster.fog had bit 0x8000: a real fog plane
    _structure_fog_plane_unbounded = 2      // the cluster is entirely inside the fog region
} structure_fog_plane_mode;

// ---------------------------------------------------------------------------
// cluster_reference_group  (the three-global group cluster_partition_new 0x551e30 fills in ESI)
// types/objects.h documents the two instances of it by address but never names the record:
// {0x008603c0, 0x008603c4, 0x008603c8} for noncollideable objects and {0x008603d0, 0x008603d4,
// 0x008603d8} for collideable ones. object_placement_cursor (types/objects.h) holds the address
// of one of them and reaches the object side of the chain through cluster_globals[2], which is
// the object_cluster_references slot of this record.
// Confirmed by disassembly: cluster_partition_new stores the 0x800-byte head table at [esi+0x00]
// and the two game_state_new pools (element size 0xc == sizeof(object_cluster_reference), forced
// through EBX) at [esi+0x04] then [esi+0x08]; cluster_reference_add_within_radius (0x551f00)
// allocates the object's own chain entry from [edi+0x08] and the cluster's from [edi+0x04], and
// cluster_reference_remove_all (0x552020) deletes from [ebx+0x08] then [ebx+0x04] while walking
// [ebx+0x00]. Note that the global name types/objects.h gives the third slot
// ("*_cluster_partition") predates this and is a misnomer: it is a data_array, not a partition.
// ---------------------------------------------------------------------------
typedef struct cluster_reference_group {
    datum_index *cluster_first;            // 0x00 datum_index[k_maximum_clusters], head per cluster
    data_array *cluster_object_references; // 0x04 "cluster <name> reference": each cluster's chain
                                           //      of the objects that reference it
    data_array *object_cluster_references; // 0x08 "<name> cluster reference": each object's chain
                                           //      of the clusters it references
} cluster_reference_group;                 // size 0x0c

// ---------------------------------------------------------------------------
// polygon2d  (the clipped screen-space polygon every portal and mirror test passes around)
// 0x00554850 writes the count at +0x00 and then perspective-projects the surviving vertices to
// +0x04 onwards, and the stack buffer of every caller is 0x804 bytes: 4 + 0x100 * 8. The 0x100 limit
// is also the literal handed to polygon2d_clip_to_planes / polygon3d_clip_to_plane (types/math.h
// owns those routines but declares no record for the polygon itself).
// ---------------------------------------------------------------------------
typedef struct polygon2d {
    int16_t point_count;           // 0x000 -1 from polygon2d_clip_to_planes means "unchanged"
    int16_t unknown_02;            // 0x002 alignment; never read
    real_point2d points[0x100];    // 0x004
} polygon2d;                       // size 0x804

// ---------------------------------------------------------------------------
// structure_bsp_leaf_map  (the tail of ScenarioStructureBSP, from +0x26c to its end at +0x288)
// 0x005520b0 is handed this address in ECX and reads leaves.pointer at +0x08 and
// portals.pointer at +0x14, i.e. the leaf_map_leaves / leaf_map_portals reflexives of the tag with
// one dword in front of them. 0x005527f0 bounds-checks the same two blocks straight off the tag
// at +0x270 and +0x27c, so the sub-struct starts at +0x26c and ends exactly where the tag does.
// This is the "leaf map" the AI sound-occlusion tools build, not the render leaves.
// ---------------------------------------------------------------------------
typedef struct structure_bsp_leaf_map {
    uint32_t unknown_00;           // 0x00 ScenarioStructureBSP _pad_26c; never read here
    TagReflexive leaves;           // 0x04 ScenarioStructureBSPGlobalMapLeaf, stride 0x18
    TagReflexive portals;          // 0x10 ScenarioStructureBSPGlobalLeafPortal, stride 0x18
} structure_bsp_leaf_map;          // size 0x1c

// ---------------------------------------------------------------------------
// structure_bsp_cluster_surface_run
// The surface_indices block of a cluster is a flat int32 list in types/tags.h, but 0x00553a70 walks
// it as a sequence of runs: three dwords of header whose last member is the run length, then
// that many surface indices. The two leading dwords are only consumed by the three
// render_frustum_classify_point_side_planes calls the loop makes per surface, whose arguments
// Ghidra lost to registers, so they stay unknown.
// This encoding is only used on BSPs whose clusters have no subclusters -- 0x005537c0 tests
// clusters[0].subclusters.count and sends those maps down 0x00553a70 instead of 0x00553920.
// ---------------------------------------------------------------------------
typedef struct structure_bsp_cluster_surface_run {
    int32_t unknown_00;            // 0x00
    int32_t unknown_04;            // 0x04
    int32_t surface_count;         // 0x08 surface indices follow, one int32 each
} structure_bsp_cluster_surface_run;  // size 0x0c plus surface_count * 4

// ---------------------------------------------------------------------------
// the four surface-render callbacks structure_leaf_faces_for_each (0x552de0) drives, in the
// order it calls them: once per lightmap, once per opaque material run, once at the end of the
// lightmap, and once per transparent material run instead of the opaque one. Every caller in
// this module (0x552980, 0x552a60, 0x552b40, 0x5528f0) passes the same four function pointers
// straight through as stack arguments, so the signatures are pinned by the argument counts the
// original pushes rather than by any callee this module owns -- the callees live in the render
// module. The transparent variant additionally receives the fog plane vector
// (0x006e3ae4) instead of the default whenever the material has the coplanar/fog flag set.
// ---------------------------------------------------------------------------
typedef void (*structure_lightmap_begin_callback)(void *bitmap_data);
typedef void (*structure_material_callback)(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t surface_offset, int16_t surface_count, void *material_extra);
typedef void (*structure_lightmap_end_callback)(void);
typedef void (*structure_transparent_material_callback)(void *shader_data, int16_t shader_permutation,
    void *bitmap, int32_t render_context, int32_t surface_offset, int16_t surface_count,
    void *material_extra, void *rendered_vertices, void *lightmap_vertices, void *coplanar_vector,
    void *lightmap_vertices_offset, int32_t zero);

// ---------------------------------------------------------------------------
// structure_bsp_visible_cluster  (0x007c3390, k_maximum_visible_clusters entries of 0x1a0)
// camera_cluster_portal_flood_recursive appends one per cluster it reaches, seeding the screen
// bounds from the four-float default at *0x00696744 and then growing them with
// polygon2d_bounds_expand over the clipped portal polygon. 0x005544f0 makes a second pass over
// the finished list and calls render_camera_compute_frustum_bounds plus the frustum builder once
// per entry, which is what fills the rest of the record; that part belongs to the render module.
// The stride is proved twice: by the visible_index * 0x1a0 inside
// camera_cluster_portal_flood_recursive, and 0xd0 int16 units in the cluster_index stores of 0x553920 /
// 0x553a70 / 0x554420.
// ---------------------------------------------------------------------------
typedef struct structure_bsp_visible_cluster {
    int16_t cluster_index;         // 0x000
    int16_t unknown_02;            // 0x002 alignment
    real_bounds screen_bounds_x;   // 0x004 min then max, seeded from *0x00696744
    real_bounds screen_bounds_y;   // 0x00c
    uint8_t unknown_014[0x18c];    // 0x014 the per-cluster clipped view frustum, written by the
                                   //       render module; no function here reads it
} structure_bsp_visible_cluster;   // size 0x1a0

// ---------------------------------------------------------------------------
// structure_bsp_mirror_result  (the out-block 0x00553560 fills through its third argument)
// The plane is copied straight out of ScenarioStructureBSPMirror (+0x00..+0x0f) and the two
// floats come from the mirror shader when it is a shadertype_environment: ShaderEnvironment
// +0x30c and +0x310 (types/tags.h runtime_mirror_value_0/_1). They are zeroed for any other
// shader type.
// ---------------------------------------------------------------------------
typedef struct structure_bsp_mirror_result {
    real_plane3d plane;            // 0x00 ScenarioStructureBSPMirror.plane
    float shader_mirror_value_0;   // 0x10 ShaderEnvironment.runtime_mirror_value_0 (+0x30c), 0 for
                                   //      other shader types
    float shader_mirror_value_1;   // 0x14 ShaderEnvironment.runtime_mirror_value_1 (+0x310)
    int16_t cluster_index;         // 0x18 the cluster the mirror was found in
    int16_t unknown_1a;            // 0x1a alignment; never written
} structure_bsp_mirror_result;     // size 0x1c

// ---------------------------------------------------------------------------
// structure_fog_environment  (the out-block 0x00555330 fills through ESI)
// Built from the Fog tag 0x00555270 resolves for a cluster -- cluster.fog names either a fog
// plane (bit 0x8000 set, index into fog_planes, whose front_region then names the fog region) or
// a fog region directly; the fog field of the region indexes fog_palette, whose dependency is the Fog
// tag. When the cluster has no fog the indoor_fog_screen of the sky tag (Sky +0xa4) is used instead
// and flags bit 0 records that.
// Only the fields listed are touched here; +0x04..+0x1b is left alone by this module, so some
// other producer owns it. 0x00555330 clears exactly +0x00, +0x1c and +0x48 on entry.
// ---------------------------------------------------------------------------
typedef struct structure_fog_environment {
    uint16_t fog_flags;            // 0x00 the low word of Fog.flags: is_water, atmosphere_dominant,
                                   //      fog_screen_only
    uint8_t flags;                 // 0x02 bit 0: the fog came from the sky tag, not a cluster
    uint8_t unknown_03;            // 0x03
    uint8_t unknown_04[0x18];      // 0x04 never read or written by this module
    int16_t plane_mode;            // 0x1c structure_fog_plane_mode
    int16_t unknown_1e;            // 0x1e
    real_plane3d plane;            // 0x20 ScenarioStructureBSPFogPlane.plane, only when
                                   //      plane_mode is _structure_fog_plane_bounded
    float color_red;               // 0x30 Fog.color   (Fog +0x78)
    float color_green;             // 0x34            (Fog +0x7c)
    float color_blue;              // 0x38            (Fog +0x80)
    float maximum_density;         // 0x3c Fog.maximum_density  (Fog +0x58)
    float opaque_distance;         // 0x40 Fog.opaque_distance  (Fog +0x60)
    float opaque_depth;            // 0x44 Fog.opaque_depth     (Fog +0x68)
    void *screen_parameters;       // 0x48 &Fog.flags_1, i.e. the fog tag data plus 0x84: the
                                   //      screen-layer block the fog screen renderer walks
} structure_fog_environment;       // size 0x4c

// ---------------------------------------------------------------------------
// detail objects
// 0x00552260 reserves one 0xa430-byte game-state block (the pointer lands in 0x0072277c) and
// seeds a {0, 0, 1.0, 0} quad at its very end; 0x005522d0 fills it every frame. The sizes chain
// exactly, which is what pins the whole layout:
//   a batch is 0x18 bytes (the indexed store is base + slot * 0x18 with slot = count + layer*0x1b)
//   0x20 layers * 0x1b batches * 0x18 == 0x5100, where the 8-byte per-layer descriptors start
//   0x20 descriptors * 8 == 0x100, so the render list header lands at 0x5200
//   the cached cell follows at 0x5208..0x520f, ending the frame at 0x5210
//   0xa420 / 0x5210 == 2 exactly, and 0xa420 is where the default z reference vector sits
// Only frames[0] is ever written here (local_5c is the block base), so the second frame is an
// inference from the arithmetic rather than from a store. UNSURE.
// ---------------------------------------------------------------------------
typedef struct detail_object_batch {
    int32_t first_instance;        // 0x00 running sum of instance_count across the batch list
    int32_t instance_count;        // 0x04 detail_objects.counts[start + n]
    int16_t cell_x;                // 0x08 ScenarioStructureBSPGlobalDetailObjectCell.cell_x
    int16_t cell_y;                // 0x0a .cell_y
    float cell_z;                  // 0x0c cell_z + offset_z * (1/255)
    uint32_t unknown_10;           // 0x10 never written
    void *z_reference;             // 0x14 &detail_objects.z_reference_vectors[i], or
                                   //      &detail_object_globals.default_z_reference when the
                                   //      tag block is empty
} detail_object_batch;             // size 0x18

typedef struct detail_object_layer_batches {
    detail_object_batch *batches;  // 0x00 == &frame.batches[layer_index][0]
    int16_t batch_count;           // 0x04
    int16_t layer_index;           // 0x06
} detail_object_layer_batches;     // size 0x08

// The two-member header 0x005522d0 hands to the renderer (0x0051b6f0 to submit, 0x0051b890 to
// draw). It lives inside the frame and points back at the descriptor array of the frame itself.
typedef struct detail_object_render_list {
    detail_object_layer_batches *layers;  // 0x00 == &frame.layers[0]
    int16_t layer_count;                  // 0x04 layers with at least one batch
    int16_t unknown_06;                   // 0x06
} detail_object_render_list;               // size 0x08

typedef struct detail_object_frame {
    detail_object_batch batches[0x20][0x1b];       // 0x0000 [layer][slot]
    detail_object_layer_batches layers[0x20];      // 0x5100
    detail_object_render_list render_list;         // 0x5200
    int16_t cell_x;                                // 0x5208 the cached render-position cell,
    int16_t cell_y;                                // 0x520a compared against the new one to
    int16_t cell_z;                                // 0x520c decide whether to rebuild
    uint8_t valid;                                 // 0x520e 0 forces a rebuild
    uint8_t unknown_520f;                          // 0x520f written as 0 with cell_z
} detail_object_frame;                             // size 0x5210

typedef struct detail_object_globals {
    detail_object_frame frames[2];                  // 0x0000 and 0x5210; only [0] is written
    ScenarioStructureBSPGlobalZReferenceVector default_z_reference;  // 0xa420 {0, 0, 1.0, 0}
} detail_object_globals;                            // size 0xa430

// The (x, y, z) key the two detail-object cell binary searches compare. 0x00552710 finds the
// first cell not less than the key and 0x00552780 the first one greater, so together they
// bracket the cells of one column; the comparison is lexicographic over the three int16.
typedef struct detail_object_cell_key {
    int16_t cell_x;                // 0x00
    int16_t cell_y;                // 0x02
    int16_t cell_z;                // 0x04 stepped by the caller to sweep the z neighbourhood
    int16_t unknown_06;            // 0x06 zeroed by the caller; never compared
} detail_object_cell_key;          // size 0x08

// ---------------------------------------------------------------------------
// globals owned by this module
// ---------------------------------------------------------------------------
// the current position of the camera in the BSP (0x00553490, from bsp3d_node_find_leaf)
// global 0x007c3344: int32_t render_leaf_index          -1 when the camera is outside the BSP
// global 0x007c3348: int32_t render_cluster_index        ScenarioStructureBSPLeaf.cluster, -1 outside
// global 0x007c334d: uint8_t render_cluster_has_sky      the sky tag of the cluster resolves and its
//                    model dependency is set
// global 0x007c334e: int16_t render_cluster_sky_index    ScenarioStructureBSPCluster.sky
//
// the per-frame visibility results (0x005537c0 resets them, the portal flood and the two
// expansion passes fill them)
// global 0x007c3350: uint32_t cluster_visible_bits[0x10]   one bit per cluster, 0x40 bytes,
//                    ending exactly where the visible cluster array begins. 0x005537c0 fills it
//                    with -1 when the camera is outside the BSP so that everything draws
// global 0x007c3390: structure_bsp_visible_cluster visible_clusters[0x80]
// global 0x007d0390: int16_t visible_cluster_count
// global 0x007d0394: uint32_t surface_visible_bits[0x1000]  one bit per BSP surface; only
//                    ((surfaces.count + 0x1f) >> 5) dwords are cleared each frame
// global 0x00850394: int16_t visible_surface_count          saturates at 0x3fff; every store is a
//                    WORD op, and the two debug readers that load it as a dword only need AX
// global 0x00850398: int32_t visible_surface_indices[0x4000]  ends at 0x00860398, just short of
//                    the object globals at 0x008603b0
//
// the picked-polygon debug visualisation (0x005527f0 refreshes it, 0x005528f0 draws it)
// global 0x006e3ad8: uint8_t picked_surfaces_valid        == (picked_surfaces_geometry != -1)
// global 0x006e3adc: int32_t picked_surfaces_geometry     geometry buffer handle, -1 on failure
// global 0x0069fa40: int32_t picked_leaf_map_leaf         debug index into leaf_map.leaves
// global 0x0069fa44: int32_t picked_leaf_map_portal       debug index into leaf_map.portals
// global 0x0069fa48: int16_t geometry_buffer_warning      latched to 0 whenever a buffer lock fails
//                    (WORD accesses throughout)
// global 0x00724a46: uint8_t debug_count_all_leaf_portals console toggle for the whole-map
//                    counting loop in 0x005527f0
// global 0x00724a45: uint8_t debug_render_cluster_pvs     makes 0x005537c0 replace the portal
//                    flood result with the raw PVS row of the camera cluster
//
// the fog plane vector the transparent-surface path hands to the renderer
// global 0x006e3ae0: uint8_t fog_plane_vector_valid       0x005527f0 clears it, 0x00555330 sets it
//                    (BYTE accesses throughout)
// global 0x006e3ae4: real_vector3d fog_plane_vector       reset from *global_origin3d_pointer
//                    (0x00696714, math.h: the zero vector), then rebuilt as
//                    the fog density times the fog plane normal. 0x00552de0 passes it instead of
//                    the default to the transparent callback when coplanar/fog
//                    flag bit 1 of the material is set
//
// the portal flood working state
// global 0x006e3af0: uint8_t no_subcluster_path_taken     latched once 0x005537c0 picks 0x553a70
// global 0x006e3af8: uint32_t *flood_recursion_bits       points at the 0x40-byte caller stack
//                    bitset, the "cluster is on the recursion stack" marker
// global 0x006e3afc: int16_t cluster_visible_index[0x200]  cluster -> visible_clusters slot
//
// the cluster flood-fill stamps, shared with object_collect_in_clusters (types/objects.h) and
// with the physics segment walks (types/physics.h)
// global 0x006e3f01: uint8_t cluster_flood_in_progress    raised for the duration of a flood
// global 0x006e3f04: int32_t cluster_flood_stamp          bumped once per flood
// global 0x006e3f08: int32_t cluster_flood_stamps[0x200]  per-cluster copy of the stamp
//
// detail objects
// global 0x0072277c: detail_object_globals *detail_objects  0xa430 of game state
//
// runtime decals
// global 0x0072278c: uint8_t *runtime_decals_suppressed   0x005530d0 clears the byte it points at
//                    on every exit path, and skips the spawn side while it is set. UNSURE
//
// tuning constants in .rdata
// global 0x0069fa4c: float k_cluster_query_radius_threshold  0x00553d80 takes the cheap
//                    cluster-flood path for radii at or above it and the recursive bsp3d descent
//                    below it
// (0x00696714 is types/math.h const real_point3d *global_origin3d_pointer, == 0x0065c230 which
//  holds (0,0,0); fog_plane_vector above is reset from it. It is not a structures global.)
// global 0x00696744: real_bounds *k_default_screen_bounds   the four floats every new visible
//                    screen bounds of a cluster start at, before polygon2d_bounds_expand grows them
//
// Globals this module reads but does not own, named by the module that does:
//   0x00746f90  ModelCollisionGeometryBSP *global_collision_bsp   the collision BSP of the
//               structure (scenario.h): 0x53ef6d / 0x541042 load ScenarioStructureBSP +0xb4 and
//               store it here and at 0x00746f98 together. 0x00553e4a passes it to
//               bsp3d_node_find_leaf in ECX and 0x00554b9f reads its +0x10 as planes.pointer.
//               0x00746f98 (structure_collision_bsp in types/effects.h / types/items.h, indexed
//               through its +0x40 surfaces.pointer) always holds the same pointer.
//               (Formerly misnamed global_globals, which is the matg globals at 0x00746fa0.)
//   0x00746f9c  ScenarioStructureBSP *global_structure_bsp (scenario.h) the resident tag data;
//               every walk in this file starts here (types/physics.h, types/objects.h)
//   0x0087bc14  tag_instance *tag_instances        stride 0x20, tag data at +0x14 (types/cache.h)
//   0x0087a478  player_globals *player_globals     local player count at +0x0c (types/game.h)
//   0x006ac6e0  the BSP cluster of the local player, read by 0x0053f150 (sound)
//   global_scenario                                skies at +0x30, decal palette at +0x3b8
//   0x006e2dc8 / 0x006e2dcc / 0x006e2dd4  the game-state arena, cursor and CRC that
//               cluster_partition_new and 0x00552260 carve their blocks out of (types/game.h)
//   0x006b8d78  breakable_surface_globals, and 0x0069e8d8 the bsp index that rows it;
//               0x00552de0 skips a material whose breakable surface bit is clear (types/physics.h)
//   0x008603cc / 0x008603d0 / 0x008603d4  the cluster-to-object lists 0x00554420 enumerates
//               through its callbacks (types/objects.h)
//   0x0065c29c  k_projection_axes (types/math.h); 0x00554b00 projects the portal to 2D with it
//   0x00719cd4  the cseries random seed, saved and restored per decal by 0x005530d0
//   0x007c3100..0x007c3170  the render camera block: +0x08 the render window index, +0x14 the
//               camera position, +0x20 its forward vector, +0x54 the portal tolerance and +0x68
//               the projection context 0x00554850 transforms portal vertices with
//   0x006e09e8 / 0x0071d174 / 0x007c118c  the rasterizer device and its version
//   0x0065e508  k_default_sound_environment (sound module, 0x0053f150)
//   0x00746f94  scenario_game_globals *global_scenario_game_globals (types/scenario.h): the
//               0x7c-byte scenario game-state block, stored at 0x45aa78 and read by the scenario
//               code (0x53e925, 0x53ef2b, 0x53ef54, 0x53efc6, 0x53f008, 0x53f2d2, 0x541017).
//               Only its +0x30..+0x7b tail is the interpolated sound environment state that
//               0x0053f150 keeps; the block itself is not a sound global.
//   0x0065e64c  the near clip plane polygon3d_clip_to_plane is given
//   0x006893f5 / 0x00687004  the enable toggles of the decal system
//   0x0069c67c  a render flag 0x005528f0 forces on while it draws
#pragma pack(pop)
