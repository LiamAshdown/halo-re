// Blam effects module (halo.exe 1.0.10 retail, 0x44c800..0x53fc80, 124 Ghidra functions).
// Six related subsystems live in this address range, each with its own datum table and its own
// runtime record:
//
//   contrails          0x44c8b0..0x44d820    cont tag
//   decals             0x44dc30..0x450590    deca tag
//   effects            0x4505b0..0x4535b0    effe tag  (the effect_ family; Ghidra named
//                                            several of these particle_system_* by mistake)
//   particle systems   0x453600..0x455740    pctl tag
//   particles          0x455740..0x4566f0    part tag
//   weather            0x457e20..0x458bf0 and 0x53f5c0   rain tag
//
// plus two smaller things that share the range: the per local player screen and camera feedback
// state driven from 0x456730..0x457d50 (a player_effect system, see the note on it below), and
// the ambient colour noise grid at 0x53f860..0x53fc80 that the weather and marker colour code
// samples.
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries a
// layout it is preferred over the decompiler and the fact is called out:
//
//   - Every tag side layout already exists in types/tags.h and none of them is redefined here.
//     Each one was re-derived from the arithmetic in this module and agreed: Contrail 0x144 with
//     point_states at 0x138, ContrailPointState 0x68 with scale_flags at 0x64, Decal 0x10c with
//     lifetime at 0x78 and decay_time at 0x80, Effect 0x40 with locations at 0x28 and events at
//     0x34, EffectEvent 0x44, EffectPart 0x68, EffectParticle 0xe8, ParticleSystem 0x68 with
//     particle_types at 0x5c, ParticleSystemType 0x80 with states at 0x68 and particle_states at
//     0x74, ParticleSystemTypeStates 0xc0, ParticleSystemTypeParticleState 0x178, Particle 0x164,
//     WeatherParticleSystem 0x30, WeatherParticleSystemParticleType 0x25c, ObjectAttachment 0x48.
//
//   - Three static dispatch tables in .rdata fix three enum widths exactly (PE image base
//     0x400000, .rdata VA 0x63a000 maps to file offset 0x23a000):
//       0x0065743c  2 entries  {0x4552a0, 0x4554d0}             system update physics
//       0x00657444  3 entries  {0x455310, 0x4554e0, 0x455610}   particle creation physics
//       0x00657450  1 entry    {0x455350}                       particle update physics
//     which matches ParticleSystemSystemUpdatePhysics (2 values),
//     ParticleSystemParticleCreationPhysics (3 values) and
//     ParticleSystemParticleUpdatePhysics (1 value) in types/tags.h. All six procedures sit
//     inside this address range and Ghidra never created functions for them.
//
//   - k_decal_type_parameters is read straight out of .rdata at 0x006573f8: four rows of 0x10
//     bytes, {40.0, 110.0, 1.5, 1} three times then {10.0, 10.0, 1.5, 0}, indexed by DecalType.
//
//   - k_projection_axes is read straight out of .rdata at 0x0065c29c: six rows of two int16,
//     {2,1} {1,2} {0,2} {2,0} {1,0} {0,1}, indexed by major_axis*2 plus the sign bit.
//
//   - The weather instance array is exactly one element: the next referenced global after
//     0x006b0ae4 is 0x006b0b80, which is 0x006b0ae4 + 0x9c.
//
//   - The ambient noise grid is exactly 0x900 bytes: it starts at 0x00746284 and the next
//     global, the weather palette count, is at 0x00746b84.
//
//   - effect.tint_source is a data pointer followed by a procedure pointer rather than a
//     vector, which the decompiler could not decide. Raw code at 0x004526ca:
//       mov eax,[edi+0x34]; test eax,eax; je; mov ecx,[edi+0x30]; push ecx; lea edx,...;
//       push edx; lea ecx,...; push ecx; call eax
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h   data_array, datum_index
//   types/math.h     real_point3d, real_vector3d, real_matrix4x3, random_seed
//   types/objects.h  object, bsp_leaf_reference, object_marker
//   types/tags.h     every tag structure named above, plus ColorARGB and ColorRGB
//
// Functions in this address range that belong to other modules are listed at the end of
// out/phase4/effects_types_notes.md. The only ones whose types are defined here anyway are the
// player_effect group, because no other module header owns them yet.

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// module wide constants
// The table capacities are the literals game_state_new is called with in contrails_initialize
// 0x44c8b0 and decals_initialize 0x44df90; the rest are the literals the allocation, eviction
// and iteration paths compare against.
// ---------------------------------------------------------------------------
typedef enum effect_constants {
    k_maximum_contrails = 0x100,               // contrails_initialize
    k_maximum_contrail_points = 0x400,         // contrails_initialize
    k_contrail_point_lists = 4,                // one list per marker permutation
    k_maximum_decals = 0x800,                  // decals_initialize
    k_decal_layers = 5,                        // DecalLayer has 5 values; decal_grid rows
    k_decal_grid_clusters = 0x200,             // 512 structure BSP clusters
    k_maximum_temporary_decals = 0x200,        // decal_new 0x44dd90 starts evicting past this
    k_temporary_decal_eviction_target = 0x100, // and stops once the count drops to this
    k_decal_eviction_attempt_limit = 99,       // give up after 99 empty iterator passes
    k_decal_permanent_percent = 0x9fff6,       // random*100 below this keeps a decal temporary
    k_decal_evict_percent = 0x28ffd7,          // random*100 below this evicts an old temporary
    k_maximum_effect_locations = 32,           // effect 0x5c holds 32 list heads
    k_maximum_effect_particle_types = 32,      // effect 0xdc holds 32 counts
    k_effect_events_per_update = 8,            // effect_update advances at most 8 events a tick
    k_maximum_particle_system_types = 4,       // (0x158 - 0x58) / 0x40
    k_particle_system_spawn_burst_limit = 0x80,// particle_system_spawn 0x453b10
    k_maximum_weather_instances = 1,           // the array at 0x006b0ae4 is one element
    k_maximum_weather_particle_types = 8,      // (0x9c - 0x1c) / 0x10
    k_maximum_weather_regions = 8,             // weather_find_nearby_regions 0x458b50
    k_particle_stale_tick_limit = 0x10,        // particles_update 0x455b60 deletes past this
    k_ambient_noise_bands = 3,                 // ambient_color_sample 0x53fc80
    k_ambient_noise_rows = 8,                  // ambient_color_randomize 0x53fa70 outer loop
    k_ambient_noise_columns = 8,               // 0x60 row stride over 0x0c entries
    k_maximum_local_player_effects = 1         // player_effect_globals holds one record
} effect_constants;

// projection_axis_pair and its table k_projection_axes at 0x0065c29c already exist in
// types/math.h and are not redefined here. This module reads them in
// decal_plane_solve_third_axis 0x44d860, decal_build_projection 0x44e460 and
// decal_flood_surfaces 0x44e730, indexed the same way: major_axis*2 plus the sign bit.

// ---------------------------------------------------------------------------
// decal_type_parameters  (the .rdata table at 0x006573f8, indexed by DecalType)
// decal_flood_surfaces 0x44e730 reads maximum_edge_angle, converted to radians by the
// 0.017453292 literal, to decide whether to wrap onto the next surface, and radius_scale to
// size the ray_intersects_sphere_test that bounds the flood.
// ---------------------------------------------------------------------------
typedef struct decal_type_parameters {
    float maximum_edge_angle;       // 0x00 degrees; 40.0 for scratch, splatter and burn, 10.0
                                    //      for painted_sign
    float unknown_04;               // 0x04 110.0 for the first three, 10.0 for painted_sign;
                                    //      no reader in this module
    float radius_scale;             // 0x08 1.5 for every type
    int32_t unknown_0c;             // 0x0c 1 for the first three, 0 for painted_sign
} decal_type_parameters;            // size 0x10

// ===========================================================================
// contrails
// contrails_initialize 0x44c8b0 creates both tables,
//   game_state_new("contrail", 0x100)        element size 0x44
//   game_state_new("contrail point", 0x400)  element size 0x38
// and clears both handles when only one of the two allocations succeeded.
// ===========================================================================
typedef enum contrail_flags {
    _contrail_emitting_bit = 0x01   // contrail 0x02 bit 0. contrail_update 0x44cb50 keeps it in
                                    // step with object.function_valid_flags for
                                    // scale_function_index and regenerates the first point
                                    // whenever the two disagree.
} contrail_flags;

typedef enum contrail_point_flags {
    _contrail_point_skip_render_bit = 0x01,   // consumed and cleared once, so a point created
                                              // this tick is not drawn
    _contrail_point_in_transition_bit = 0x02, // aging through ContrailPointState
                                              // transition_duration rather than duration
    _contrail_point_expired_bit = 0x04        // ran off the end of point_states;
                                              // contrail_age_points 0x44d470 collapses runs of
                                              // these from the tail of the list
} contrail_point_flags;

// ---------------------------------------------------------------------------
// contrail  (element of contrail_data, 0x0087abec)
// Established by contrail_new 0x44c910 (every field at create), contrail_update 0x44cb50
// (flags, scale, the texture offsets, both timers), contrail_advance 0x44ca60
// (accumulated_delta_time, detaching object_index), contrail_points_due 0x44cf80
// (generation_timer against Contrail.point_generation_rate at +0x04 with scale flag bit 0),
// contrail_next_sequence 0x44ced0 (sequence_index and frame_index against Contrail
// first_sequence_index at +0x40 and sequence_count at +0x42), contrail_generate_points 0x44d020
// and contrail_delete 0x44cad0 (the four point lists).
// ---------------------------------------------------------------------------
typedef struct contrail {
    uint16_t identifier;            // 0x00 datum header
    uint8_t flags;                  // 0x02 contrail_flags
    uint8_t unknown_03;             // 0x03 never read
    datum_index definition_index;   // 0x04 the Contrail tag; every tag lookup starts here
    datum_index object_index;       // 0x08 owner object; contrail_advance sets it to -1 to
                                    //      detach, and contrail_update deletes the contrail
                                    //      once it is -1 and all four lists are empty
    int16_t attachment_index;       // 0x0c index into Object tag attachments, stride 0x48; the
                                    //      marker name at +0x10 goes to
                                    //      object_get_node_local_transform with room for 4
                                    //      permutations
    int16_t scale_function_index;    // 0x0e ObjectAttachment.primary_scale minus 1, so -1 means
                                    //      a constant scale of 1.0; otherwise it indexes both
                                    //      object.function_out_values and
                                    //      object.function_valid_flags
    float scale;                    // 0x10 the current value of that function, 1.0 when none
    int16_t sequence_index;         // 0x14 random in first_sequence_index plus
                                    //      [0, sequence_count)
    int16_t frame_index;            // 0x16 bumped per generated point, re-rolled once it
                                    //      reaches the BitmapSequence bitmap_count at +0x22
    float texture_offset_u;         // 0x18 decremented by Contrail.texture_animation_u times dt
    float texture_offset_v;         // 0x1c incremented by Contrail.texture_animation_v times dt
    float generation_timer;         // 0x20 seconds until the next point, reloaded with
                                    //      1 / point_generation_rate
    float animation_timer;          // 0x24 seconds accumulated against 1 / Contrail.animation_rate
    float accumulated_delta_time;   // 0x28 time already advanced by contrail_advance this frame;
                                    //      contrail_update subtracts it out and then zeroes it
    int16_t point_count[4];         // 0x2c one count per marker permutation list
    datum_index first_point[4];     // 0x34 heads of the four contrail_point lists, -1 when empty
} contrail;                         // size 0x44

// ---------------------------------------------------------------------------
// contrail_point  (element of contrail_point_data, 0x0087abe8)
// Established by contrail_generate_points 0x44d020 (every field at create, plus the
// interpolation of scale, position and velocity between the previous point and the marker),
// contrail_age_points 0x44d470 (flags, state_index, age, inverse_duration, and the render
// submit at 0x50b530 which is handed location and reaches position through it) and
// contrail_refresh_lightmap 0x44cda0 (location only).
// ---------------------------------------------------------------------------
typedef struct contrail_point {
    uint16_t identifier;            // 0x00 datum header
    uint8_t flags;                  // 0x02 contrail_point_flags; 0x03 at create
    int8_t state_index;             // 0x03 index into Contrail.point_states, stride 0x68; -1 at
                                    //      create so the first age step advances it to 0
    float age;                      // 0x04 0..1 inside the current state
    float inverse_duration;         // 0x08 1 / seconds; 0 means the state never expires
    float scale;                    // 0x0c the owning contrail scale at create, interpolated
                                    //      toward the previous point when subdividing
    uint32_t unknown_10;            // 0x10 neither written at create nor read here; most likely
                                    //      consumed by the renderer
    bsp_leaf_reference location;    // 0x14 bsp3d_node_find_leaf result plus its cluster
    real_point3d position;          // 0x1c
    real_vector3d velocity;         // 0x28 a random cone direction times point_velocity, plus
                                    //      inherited_velocity_fraction times the root object
                                    //      velocity at object +0x68
    datum_index next_point;         // 0x34 -1 at the tail
} contrail_point;                   // size 0x38

// ===========================================================================
// decals
// decals_initialize 0x44df90 creates game_state_new("decals", 0x800) with element size 0x38,
// marks the data_array valid immediately, carves 0x280c bytes of game state for decal_grid,
// folds that size into the game state CRC and then calls rasterizer_decals_initialize.
// ===========================================================================
typedef enum decal_flags {
    _decal_temporary_bit = 0x01,      // counted in decal_grid.temporary_count and subject to
                                      // budget eviction
    _decal_object_attached_bit = 0x02 // counted in decal_grid.object_count and linked on
                                      // decal_grid.first_object_decal instead of a cluster row
} decal_flags;

// ---------------------------------------------------------------------------
// decal  (element of decal_data, 0x0087abe4)
// Established by decal_place 0x44edc0 (position, creation_game_time, sequence_index, lifetime,
// decay_time, color, alpha, triangle_count and definition_index), decal_new 0x44dd90 and
// decal_link 0x44dd30 (flags, cluster_index, layer and the list links), decal_update_fade
// 0x44dc30 (alpha from lifetime and decay_time against the game tick at 0x006f1d6c +0x0c) and
// decal_delete 0x44e3c0 plus decal_rehash_object_decals 0x44e000 (the list links again).
// definition_index is proved by the raw code at 0x004502b4, mov edx,[ebp+0x8];
// mov [esi+0x2c],edx, where ebp+0x8 is the tag index decal_place resolves its tag data from.
// ---------------------------------------------------------------------------
typedef struct decal {
    uint16_t identifier;            // 0x00 datum header
    uint16_t flags;                 // 0x02 decal_flags; decal_new writes 0, 1 or 2 outright
    int16_t cluster_index;          // 0x04 structure BSP cluster, the column of decal_grid; -1
                                    //      for a decal on the object list
    int16_t layer;                  // 0x06 DecalLayer, copied from Decal tag +0x04; the row of
                                    //      decal_grid
    real_point3d position;          // 0x08 copied from the placement block
    int32_t creation_game_time;     // 0x14 game tick stamp; the fade is (now - this) * 1/30
    uint8_t sequence_index;         // 0x18 random in [0, bitmap sequence count)
    uint8_t unknown_19;             // 0x19 never written by decal_place
    uint8_t unknown_1a;             // 0x1a written as 0 by decal_place
    uint8_t unknown_1b;             // 0x1b a per surface index computed during projection; 0 on
                                    //      the path that is not media mapped
    float lifetime;                 // 0x1c random in Decal.lifetime; 0 means it never expires
    float decay_time;               // 0x20 random in Decal.decay_time
    uint32_t color;                 // 0x24 packed as alpha, red, green, blue from high byte
                                    //      down; alpha from Decal.intensity and the RGB from
                                    //      color_interpolate over the Decal colour bounds
    uint8_t alpha;                  // 0x28 0xff until the decay window, then the fade byte
    uint8_t unknown_29;             // 0x29 never written
    int16_t triangle_count;         // 0x2a sum over the clipped surface polygons of
                                    //      (vertex_count - 1) / 2
    datum_index definition_index;   // 0x2c the Decal tag
    datum_index previous_decal;     // 0x30 -1 at the head of its row
    datum_index next_decal;         // 0x34 -1 at the tail
} decal;                            // size 0x38

// ---------------------------------------------------------------------------
// decal_grid  (the 0x280c byte game state block at decal_grid_block, 0x006b0ad8)
// One decal list head per (layer, cluster) pair plus the object attached list and the two
// budget counters. decal_link 0x44dd30 indexes it as (layer * 0x200 + cluster) * 4,
// decal_clear_flags 0x44e220 and decal_evict_object_decals 0x44e310 walk it, and decal_new
// 0x44dd90 is the only writer of the counters.
// ---------------------------------------------------------------------------
typedef struct decal_grid {
    datum_index cluster_first[5][0x200]; // 0x0000 head per (DecalLayer, cluster), -1 when empty
    datum_index first_object_decal; // 0x2800 head of the decals attached to moving objects
    int32_t temporary_count;        // 0x2804 live decals carrying _decal_temporary_bit
    int32_t object_count;           // 0x2808 live decals carrying _decal_object_attached_bit
} decal_grid;                       // size 0x280c

// ---------------------------------------------------------------------------
// decal_projected_corner  (one flattened corner of a decal quad)
// ---------------------------------------------------------------------------
typedef struct decal_projected_corner {
    float u;                        // 0x00
    float v;                        // 0x04
} decal_projected_corner;           // size 0x08

// ---------------------------------------------------------------------------
// decal_projection  (built by decal_build_projection 0x44e460, consumed by
// decal_flood_surfaces 0x44e730 and decal_place 0x44edc0)
// The texture projection basis for one decal: the placement matrix, the decal plane in world
// and in placement space, the flattening axes, the four projected corners, the two edge
// gradients and the inverse determinant that turns a surface point into a uv pair.
// decal_flood_surfaces reads major_axis at 0x54, normal_positive at 0x56 and takes 0x58 as the
// base of the corner array.
// ---------------------------------------------------------------------------
typedef struct decal_projection {
    real_matrix4x3 placement;       // 0x00 13 dwords copied verbatim from the caller matrix
    float plane_i;                  // 0x34 the decal plane as the caller supplied it
    float plane_j;                  // 0x38
    float plane_k;                  // 0x3c
    float plane_d;                  // 0x40
    float transformed_i;            // 0x44 the same plane rotated into placement space
    float transformed_j;            // 0x48
    float transformed_k;            // 0x4c
    float transformed_d;            // 0x50
    int16_t major_axis;             // 0x54 0, 1 or 2; the largest magnitude component
    uint8_t normal_positive;        // 0x56 1 when that component is greater than zero
    uint8_t unknown_57;             // 0x57 padding
    decal_projected_corner corners[4]; // 0x58 the quad flattened onto the two chosen axes
    float du_edge0;                 // 0x78 corners[1].u minus corners[0].u
    float dv_edge0;                 // 0x7c corners[1].v minus corners[0].v
    float du_edge1;                 // 0x80 corners[3].u minus corners[0].u
    float dv_edge1;                 // 0x84 corners[3].v minus corners[0].v
    float inverse_determinant;      // 0x88 1 / (dv_edge1 * du_edge0 - dv_edge0 * du_edge1)
} decal_projection;                 // size 0x8c

// ---------------------------------------------------------------------------
// decal_flood_vertex_record / decal_flood_accumulator  (the caller owned scratch block
// decal_place 0x44edc0 keeps on its own stack and hands to decal_flood_surfaces 0x44e730)
// decal_flood_surfaces appends one record per clipped polygon corner at +0x0000 with stride
// 0x14, keeps the running count at +0x5000, and pushes the BSP surface indices it has already
// visited into the int16 list at +0x5002 with its count at +0x5802. The two counts and the two
// strides are forced by that function's own arithmetic; 0x400 elements is what the span between
// them allows, so it is the maximum rather than a proved capacity.
// TYPES-GAP: decal_place's Ghidra locals undersize this block (local_cb44 is only ever seen as
// 16 bytes there); the size below is what decal_flood_surfaces actually requires.
// ---------------------------------------------------------------------------
typedef struct decal_flood_vertex_record {
    real_point3d position;          // 0x00 back solved from the uv pair by
                                    //      decal_plane_solve_third_axis 0x44d860
    float u;                        // 0x0c
    float v;                        // 0x10
} decal_flood_vertex_record;        // size 0x14

typedef struct decal_flood_accumulator {
    decal_flood_vertex_record vertices[0x400]; // 0x0000
    int16_t vertex_count;                      // 0x5000
    int16_t visited_surfaces[0x400];           // 0x5002 BSP surface indices already flooded
    int16_t visited_surface_count;             // 0x5802
} decal_flood_accumulator;          // size 0x5804

// The BSP surface, edge and vertex tables decal_flood_surfaces 0x44e730 walks are NOT redefined
// here: 0x00746f98 is structure_collision_bsp, a ModelCollisionGeometryBSP from types/tags.h,
// and this module's +0x40 / +0x4c / +0x58 reads with strides 0xc / 0x18 / 0x10 are exactly its
// surfaces.pointer / edges.pointer / vertices.pointer and the matching
// ModelCollisionGeometryBSPSurface / ...Edge / ...Vertex records. src/physics and src/items
// already declare that global with the same type.

// ===========================================================================
// effects (the effe tag instances; Ghidra calls several of these particle_system_*)
// effect_data is the 0xfc byte table at 0x0087abdc and effect_location_data the 0x3c byte table
// at 0x0087abe0. Neither initializer is inside this address range.
// ===========================================================================
typedef enum effect_flags {
    _effect_event_started_bit = 0x0001,    // the current event has begun spawning
    _effect_looping_bit = 0x0002,          // attached to an object, so the loop_start_event and
                                           // loop_stop_event pair applies
    _effect_stopping_bit = 0x0004,         // playing the loop stop event, set by effect_stop
                                           // 0x450b20
    _effect_finished_bit = 0x0008,         // ran out of events; effect_update returns at once
    _effect_hidden_bit = 0x0010,           // cluster missing from the local player visible
                                           // bitmask, so events advance but nothing spawns
    _effect_stop_immediately_bit = 0x0020, // effect_stop was asked not to play the stop event
    _effect_first_person_bit = 0x0040      // created on the first person weapon, so
                                           // EffectPart.create picks the first person variant
} effect_flags;

// A location marker index carries a flag in its top bit.
typedef enum effect_marker_constants {
    k_effect_marker_none = 0xffff,
    k_effect_marker_first_person_bit = 0x8000,
    k_effect_marker_index_mask = 0x7fff
} effect_marker_constants;

// ---------------------------------------------------------------------------
// effect_tint_source  (the three dword descriptor effect_set_placement 0x451600 copies into
// effect +0x30)
// effect_spawn_particles 0x451f90 calls proc(out_rgb, position, data) when proc is not null and
// falls back to global_origin3d_pointer when it is. effect_set_placement clears proc and
// unknown_08 but not data when the caller passes no descriptor, which is why this is three
// separate fields rather than a vector.
// ---------------------------------------------------------------------------
typedef struct effect_tint_source {
    uint32_t data;                  // 0x00 third argument of proc
    uint32_t proc;                  // 0x04 void (*)(ColorRGB *out, real_point3d *at, void *data);
                                    //      held as uint32_t rather than a pointer so the 0x0c
                                    //      size survives a 64 bit host compiler
    uint32_t unknown_08;            // 0x08 third dword of the descriptor, cleared with proc
} effect_tint_source;               // size 0x0c

// ---------------------------------------------------------------------------
// effect_marker_node_context  (the 0x18 byte caller stack block that
// effect_marker_callback_context 0x006b0adc points at while effect_rebuild_markers 0x451710
// and the marker resolution thunk at 0x00451850 run)
// Built identically by the two creation wrappers that use a marker callback,
// effect_new_on_object_with_node_table 0x450870 and effect_new_with_color 0x450980: both write
// the same six slots in the same order, which is where the layout comes from. Ghidra's locals
// are local_18[2] / local_14 / local_10 / local_c / local_8 / local_4 in 0x450870 and
// local_1c[2] / local_18 / local_14 / local_10 / local_c / local_8 in 0x450980, and the address
// stored into the global is the first of them.
// UNSURE: only node_index and node_table_entry have recovered meaning. The other four are
// forwarded caller arguments whose consumer, LAB_00451850, Ghidra never split into a function.
// ---------------------------------------------------------------------------
typedef struct effect_marker_node_context {
    uint16_t node_index;            // 0x00 0 when the caller passed 0xffff (0x450870 masks it
                                    //      with (index == 0xffff) - 1), 0xffff in 0x450980
                                    //      which has no node table at all
    uint16_t unknown_02;            // 0x02 upper half of the ushort[2]; never written
    int32_t node_table_entry;       // 0x04 node_index * 0x34 + *(int16 *)(object + 0x1f2) plus
                                    //      the object base, or 0 when there is no node table
    uint16_t unknown_08;            // 0x08
    uint16_t unknown_0a;            // 0x0a never written
    uint32_t unknown_0c;            // 0x0c
    uint32_t unknown_10;            // 0x10
    uint32_t unknown_14;            // 0x14
} effect_marker_node_context;       // size 0x18

// ---------------------------------------------------------------------------
// effect  (element of effect_data, 0x0087abdc)
// Established by effect_new 0x451500 (definition_index at 0x04, creator_object_index at 0x40,
// first_person_weapon_index, flags), effect_set_placement 0x451600 (color, tint_source, both
// scales), the four creation wrappers 0x4506d0, 0x4507a0, 0x450870 and 0x450980
// (change_color_index, object_index, location, colour defaults, the 32 location list heads),
// effect_start_event 0x451660 (event_index, event_time and event_duration from
// EffectEvent.duration_bounds at +0x10), effect_update 0x451a30 (flags, location, velocity,
// colour from object.change_colors, particle_counts) and effect_spawn_particles 0x451f90
// (previous_event_fraction).
// ---------------------------------------------------------------------------
typedef struct effect {
    uint16_t identifier;            // 0x00 datum header
    uint16_t flags;                 // 0x02 effect_flags
    datum_index definition_index;   // 0x04 the Effect tag
    int16_t unknown_08;             // 0x08 written by effect_new_at_texture_coordinate 0x4506d0
    int16_t unknown_0a;             // 0x0a written by the same call; neither is read here
    int16_t change_color_index;     // 0x0c -1 selects the default white; otherwise indexes
                                    //      object.change_colors at object +0x1b8
    int16_t unknown_0e;             // 0x0e never written
    bsp_leaf_reference location;    // 0x10 mirrored from object.location_leaf_index and
                                    //      location_cluster_index, or probed for a free
                                    //      standing effect; cluster -1 means nowhere
    ColorRGB color;                 // 0x18 tint handed to every part and particle
    real_vector3d velocity;         // 0x24 inherited from the root object velocity
    effect_tint_source tint_source; // 0x30 optional per position tint callback
    datum_index object_index;       // 0x3c the object the effect is attached to, -1 when free
                                    //      standing; effect_update deletes the effect once the
                                    //      object is gone
    datum_index creator_object_index;// 0x40 handed to object_new as the owner and used as the
                                    //      responsible object for a jpt! damage part. UNSURE:
                                    //      effect_new stores its second argument here and three
                                    //      of the four wrappers pass the same value they later
                                    //      store in object_index
    float a_scale;                  // 0x44 the A scale every EffectPartScalesValues bit selects
    float b_scale;                  // 0x48 the B scale
    int16_t first_person_weapon_index;// 0x4c -1 when not on a first person weapon; otherwise a
                                    //      row of the first person weapon globals at
                                    //      0x006b2d98, stride 0x1ea0
    int16_t event_index;            // 0x4e index into Effect.events, stride 0x44
    float event_time;               // 0x50 seconds elapsed inside the event
    float event_duration;           // 0x54 random in EffectEvent.duration_bounds
    float previous_event_fraction;  // 0x58 event_time over event_duration as of the previous
                                    //      tick, -1.0 at event start. A spawn count for a tick
                                    //      is the difference of the distribution function
                                    //      evaluated at the two fractions
    datum_index location_markers[32];// 0x5c head of the effect_location_marker list for each
                                    //      EffectLocation, all -1 at create
    uint8_t particle_counts[32];    // 0xdc per EffectParticle spawn count for the current event,
                                    //      rolled at event start from EffectParticle.count
} effect;                           // size 0xfc

// ---------------------------------------------------------------------------
// effect_location_marker  (element of effect_location_data, 0x0087abe0)
// One resolved marker for one EffectLocation. effect_marker_new 0x4517d0 allocates it and
// copies object_marker.transform straight in, effect_rebuild_markers 0x451710 drives that per
// location, effect_marker_next 0x453180 filters the list by the requested evaluation mode and
// effect_release_first_person_markers 0x450d50 drops the first person entries.
// ---------------------------------------------------------------------------
typedef struct effect_location_marker {
    uint16_t identifier;            // 0x00 datum header
    uint16_t marker_index;          // 0x02 object_marker.node_index, or 0xffff for the object
                                    //      origin; bit 15 marks a first person weapon marker
    datum_index next_marker;        // 0x04 -1 at the tail
    real_matrix4x3 transform;       // 0x08 13 dwords copied from object_marker.transform, so
                                    //      scale at 0x08, rotation at 0x0c, position at 0x30
} effect_location_marker;           // size 0x3c

// ===========================================================================
// particle systems (the pctl tag instances)
// ===========================================================================
typedef enum particle_system_flags {
    _particle_system_emitting_bit = 0x00000001, // the driving object function is non zero
    _particle_system_in_update_bit = 0x00000002 // held for the whole of particle_system_update
                                    // 0x4544f0; particle_system_spawn 0x453b10 reads it to tell
                                    // the initial burst from the steady state
} particle_system_flags;

// ---------------------------------------------------------------------------
// particle_state_values  (the seven interpolated floats every particle carries twice)
// particle_system_roll_particle_state 0x454250 fills one from ParticleSystemTypeParticleState
// scale at 0x48, animation_rate at 0x50, rotation_rate at 0x58 and the colour pair at 0x60 and
// 0x70, all with one shared random fraction for the colour.
// ---------------------------------------------------------------------------
typedef struct particle_state_values {
    float scale;                    // 0x00 random in ParticleSystemTypeParticleState.scale
    float animation_rate;           // 0x04 random in animation_rate
    float rotation_rate;            // 0x08 random in rotation_rate
    ColorARGB color;                // 0x0c one lerp of color_1 and color_2
} particle_state_values;            // size 0x1c

// ---------------------------------------------------------------------------
// particle_system_type_state  (one per ParticleSystemType, inline in particle_system at 0x58)
// particle_system_new_type_states 0x4538b0 initialises it, particle_system_advance_type_state
// 0x4543b0 steps state_index using the loop and forward_backward bits of
// ParticleSystemType.flags against the state count at +0x68, particle_system_update 0x4544f0
// re-derives the ten multipliers every tick out of ParticleSystemTypeStates +0x34 and scales
// them by the owning system where flag bits 9 and 11 to 15 ask, and particle_system_spawn
// 0x453b10 owns creation_fraction, particle_count and first_particle.
// ---------------------------------------------------------------------------
typedef struct particle_system_type_state {
    int16_t state_index;            // 0x00 index into ParticleSystemType.states, stride 0xc0;
                                    //      -1 once the type has finished
    int16_t next_state_index;       // 0x02 -1 unless a transition is in flight
    float state_time_remaining;     // 0x04 seconds, counted down by the tick delta
    float state_duration;           // 0x08 the duration this state was rolled with
    float scale;                    // 0x0c the ten interpolated multipliers, laid out exactly as
    float animation_rate;           // 0x10 ParticleSystemTypeStates 0x34 through 0x5c
    float rotation_rate;            // 0x14
    ColorARGB color;                // 0x18
    float radius;                   // 0x28
    float minimum_particle_count;   // 0x2c
    float particle_creation_rate;   // 0x30
    float creation_fraction;        // 0x34 carried remainder of the per tick particle count
    uint8_t ping_pong_forward;      // 0x38 direction for the forward_backward state flag
    uint8_t unknown_39;             // 0x39 padding
    int16_t particle_count;         // 0x3a live particles on this list
    datum_index first_particle;     // 0x3c head of the particle_system_particle list
} particle_system_type_state;       // size 0x40

// ---------------------------------------------------------------------------
// particle_system  (element of particle_system_data, 0x0087abd4)
// Established by particle_system_new_at_point 0x453600 (definition_index, object_index -1,
// position, velocity, color and the ambient sample at 0x48 through
// object_sample_ambient_lightmap_point), particle_system_new_on_marker 0x4536f0 (object_index,
// attachment_index, scale_function_index, colour from object.change_colors and a velocity
// scaled by 30 from per tick to per second), particle_system_new_type_states 0x4538b0
// (location, flags, the inline type states) and particle_system_update 0x4544f0.
// ---------------------------------------------------------------------------
typedef struct particle_system {
    uint16_t identifier;            // 0x00 datum header
    uint16_t unknown_02;            // 0x02 never written
    uint32_t flags;                 // 0x04 particle_system_flags
    datum_index definition_index;   // 0x08 the ParticleSystem tag
    datum_index object_index;       // 0x0c -1 for a free standing system
    int16_t attachment_index;       // 0x10 index into Object tag attachments
    int16_t scale_function_index;   // 0x12 ObjectAttachment.primary_scale minus 1
    float scale;                    // 0x14 the value of that function; multiplies the per type
                                    //      count, rate, scale and animation multipliers
    bsp_leaf_reference location;    // 0x18 probed at create and again each tick
    real_point3d position;          // 0x20
    real_vector3d velocity;         // 0x2c world units per second
    ColorARGB color;                // 0x38 the tint the per type colour multipliers scale by
    ColorRGB ambient_color;         // 0x48 object_sample_ambient_lightmap_point result; the
                                    //      renderer multiplies the particle colour by it
    uint32_t unknown_54;            // 0x54 no writer or reader found in this module
    particle_system_type_state type_states[4]; // 0x58 one per ParticleSystemType, bounded by the
                                    //      struct size rather than by a check
} particle_system;                  // size 0x158

// ---------------------------------------------------------------------------
// particle_system_particle  (element of particle_system_particle_data, 0x0087abd8)
// Established by particle_system_spawn 0x453b10 (identifier through rotation, after which one of
// the three creation physics procedures at 0x00657444 fills position and the vectors),
// particle_system_update 0x4544f0 (the state machine, rotation, frame and the two value blocks),
// particle_system_advance_particle_state 0x454450 (state_index and next_state_index ping pong
// against the particle_states count at ParticleSystemType +0x74),
// particle_system_render 0x454bf0 (location, position, direction, frame) and
// particle_system_resolve_local_players 0x454080 (location only).
// ---------------------------------------------------------------------------
typedef struct particle_system_particle {
    uint16_t identifier;            // 0x00 datum header
    uint8_t ping_pong_forward;      // 0x02 direction for the particle_states forward_backward bit
    uint8_t active;                 // 0x03 1 at create; clearing it makes the update force
                                    //      state_index to -1 and the renderer skip the particle
    datum_index next_particle;      // 0x04 -1 at the tail
    int16_t state_index;            // 0x08 index into ParticleSystemType.particle_states, stride
                                    //      0x178; -1 means finished
    int16_t next_state_index;       // 0x0a -1 unless a transition is in flight
    float state_time_remaining;     // 0x0c seconds
    float state_duration;           // 0x10
    bsp_leaf_reference location;    // 0x14
    real_point3d position;          // 0x1c the point matrix4x3_transform_point is handed; proved
                                    //      by lea edx,[edi+0x1c] at 0x00454ccc
    real_point3d unknown_28;        // 0x28 filled by the creation physics procedure; UNSURE,
                                    //      never read inside this module
    real_vector3d direction;        // 0x34 rotated into view space to orient the sprite
    float rotation;                 // 0x40 radians, advanced by rotation_rate; random at create
    float frame;                    // 0x44 sprite frame, -1.0 at create so the renderer rolls a
                                    //      random starting frame out of the BitmapSequence
                                    //      sprite count at +0x34
    particle_state_values values;   // 0x48 the values of state_index
    particle_state_values next_values;// 0x64 the values of next_state_index, copied down over
                                    //      values when the transition completes
} particle_system_particle;         // size 0x80

// ===========================================================================
// particles (the part tag instances)
// ===========================================================================
typedef enum particle_flags {
    _particle_animating_backwards_bit = 0x0001, // rolled when Particle can_animate_backwards
    _particle_at_rest_bit = 0x0002,             // stops the frame animation when the Particle
                                                // animation_stops_at_rest flag is set
    _particle_mirror_horizontal_bit = 0x0004,   // rolled when random_horizontal_mirroring
    _particle_mirror_vertical_bit = 0x0008,     // rolled when random_vertical_mirroring
    _particle_unknown_10_bit = 0x0010,          // copied from particle_creation_data +0x0d
    _particle_unknown_20_bit = 0x0020,          // copied from particle_creation_data +0x0e
    _particle_first_person_bit = 0x0040         // lives in first person weapon space, so
                                                // first_person_weapon_index is meaningful
} particle_flags;

// The sequence state machine particle_next_sequence 0x455e60 walks. It reads Particle
// first_sequence_index at 0x98, initial_sequence_count at 0x9a, looping_sequence_count at 0x9c
// and final_sequence_count at 0x9e.
typedef enum particle_sequence_state {
    _particle_sequence_state_new = 0,
    _particle_sequence_state_initial = 1,
    _particle_sequence_state_looping = 2,
    _particle_sequence_state_final = 3,
    _particle_sequence_state_finished = 4
} particle_sequence_state;

// ---------------------------------------------------------------------------
// particle_creation_data  (the caller supplied block particle_new 0x455740 reads through EDI)
// A stack temporary rather than a module owned record; the offsets below are exactly the reads
// particle_new performs, so the block is at least 0x5c bytes and may be longer.
// ---------------------------------------------------------------------------
typedef struct particle_creation_data {
    datum_index definition_index;   // 0x00 the Particle tag; -1 aborts the create
    datum_index object_index;       // 0x04 -1 for a world space particle
    int16_t marker_index;           // 0x08 read as a short out of the dword at 0x08
    uint8_t first_person_weapon_index;// 0x0a also read as a short, to index the first person
                                    //      weapon globals at 0x006b2d98
    uint8_t unknown_0b;             // 0x0b
    uint8_t first_person;           // 0x0c selects the first person marker table and sets
                                    //      _particle_first_person_bit
    uint8_t third_person_only;      // 0x0d effect_spawn_particles sets (EffectParticle.create == 2); particle_new ->
                                    //    particle flag 0x10, which render_particles skips for the owning viewer
    uint8_t first_person_only;      // 0x0e effect_spawn_particles sets (create == 1); particle_new -> flag 0x20,
                                    //    render_particles skips unless owned by the viewer
    uint8_t unknown_0f;             // 0x0f
    real_point3d position;          // 0x10
    real_point3d direction;         // 0x1c effect_spawn_particles: rotated raw_direction from
                                    //    effect_random_velocity_vector (marker space or world);
                                    //    breakable_surface_shatter writes a direction; particle_new copies it
    real_vector3d velocity;         // 0x28
    real_vector3d gravity;          // 0x34 folded into velocity, scaled by the current radius
                                    //      squared and the point_physics density at +0x04, only
                                    //      for a world space particle
    float rotation;                 // 0x40 effect_spawn_particles: random 0..2pi when EffectParticle flags bit 1
                                    //    (random initial angle) else 0; shatter random 2pi; particle_new copies to
                                    //    particle+0x54
    float angular_velocity;         // 0x44 effect_spawn_particles: effect_property_random_value(3, EffectParticle
                                    //    +0x90/+0x94 angular velocity range); particle_new copies to particle+0x58
    float scale;                    // 0x48 becomes particle.scale
    ColorARGB color;                // 0x4c becomes particle.color
} particle_creation_data;           // size 0x5c, at least

// ---------------------------------------------------------------------------
// particle  (element of particle_data, 0x0087abd0)
// Established by particle_new 0x455740 (every field), particles_update 0x455b60
// (last_update_tick, age), particle_next_sequence 0x455e60 (sequence_state, sequence_index),
// particle_advance_frame 0x456000 (frame_index), particle_advance_animation 0x4560c0
// (animation_timer against inverse_animation_period), particle_update_motion 0x4561a0
// (position, velocity, location, flags) and particle_current_radius 0x4566f0 (age over lifespan
// against Particle.radius_animation at 0x74, times scale).
// ---------------------------------------------------------------------------
typedef struct particle {
    uint16_t identifier;            // 0x00 datum header
    uint16_t flags;                 // 0x02 particle_flags
    datum_index definition_index;   // 0x04 the Particle tag
    datum_index object_index;       // 0x08 -1 for a world space particle, in which case the
                                    //      motion path runs point physics and collision
    int16_t marker_index;           // 0x0c the marker the particle is pinned to
    uint8_t sequence_state;         // 0x0e particle_sequence_state
    uint8_t first_person_weapon_index;// 0x0f row of the first person weapon globals; the key
                                    //      particles_delete_by_first_person_weapon 0x455c80
                                    //      matches on
    int32_t last_update_tick;       // 0x10 render tick stamp from 0x007c3100; a particle more
                                    //      than 0x10 ticks stale is deleted instead of updated
    float age;                      // 0x14 seconds
    float lifespan;                 // 0x18 random in Particle.lifespan, with the part above 0.7
                                    //      divided by the local player count
    float animation_timer;          // 0x1c seconds into the current frame, -1.0 at create so the
                                    //      first update forces a frame advance
    float inverse_animation_period; // 0x20 1 over random(Particle.animation_rate), or 3.4e38
                                    //      when the rate is zero
    int16_t sequence_index;         // 0x24 clamped to the bitmap sequence count
    int16_t frame_index;            // 0x26 sprite index inside that sequence
    bsp_leaf_reference location;    // 0x28 the address handed to the point physics submit
    real_point3d position;          // 0x30
    real_vector3d unknown_3c;       // 0x3c copied from particle_creation_data +0x1c; UNSURE, no
                                    //      reader in this module
    real_vector3d velocity;         // 0x48 gravity is folded in at create for a world particle,
                                    //      and the velocity is damped by
                                    //      Particle.contact_deterioration at 0x88 on contact
    float unknown_54;               // 0x54 copied from particle_creation_data +0x40
    float unknown_58;               // 0x58 copied from particle_creation_data +0x44
    float scale;                    // 0x5c multiplies the Particle.radius_animation lerp
    ColorARGB color;                // 0x60 alpha then RGB; the RGB is multiplied by the ambient
                                    //      lightmap sample unless Particle is self_illuminated,
                                    //      and by the diffuse sample when tint_from_diffuse is
                                    //      set
} particle;                         // size 0x70

// ===========================================================================
// weather
// weather_particle_data is the 0x54 byte table at 0x0087abcc. The wind state array is indexed
// by Scenario weather palette row and the instance array by local player.
// ===========================================================================
// ---------------------------------------------------------------------------
// weather_particle_system_state  (element of weather_wind_states, 0x00746b88)
// weather_update 0x53f5c0 owns every field: three independent random walks that steer the wind,
// then the wind vector rebuilt from them out of the Wind tag the palette row points at.
// ---------------------------------------------------------------------------
typedef struct weather_particle_system_state {
    uint8_t active;                 // 0x00 1 while the palette row carries a wind tag
    uint8_t unknown_01[3];          // 0x01 padding
    float magnitude_walk;           // 0x04 random walk in steps of 0.01, clamped to 0..1
    float pitch_walk;               // 0x08 random walk in steps of 0.01, clamped to -1..1
    float yaw_walk;                 // 0x0c random walk in steps of 0.01, clamped to -1..1
    float magnitude;                // 0x10 lerp of the Wind velocity bounds by magnitude_walk
    float direction_i;              // 0x14 the wind vector, magnitude times the unit direction
    float direction_j;              // 0x18 built from the palette wind angles perturbed by
    float direction_k;              // 0x1c pitch_walk and yaw_walk against the variation area
} weather_particle_system_state;    // size 0x20

// ---------------------------------------------------------------------------
// weather_instance_type  (one per WeatherParticleSystemParticleType, inline in
// weather_instance at 0x1c)
// weather_instance_activate 0x457e20 seeds it, weather_instance_adjust_count 0x457fc0 grows or
// shrinks the list toward the target and weather_instance_update 0x458420 walks it.
// ---------------------------------------------------------------------------
typedef struct weather_instance_type {
    float target_count;             // 0x00 random in
                                    //      WeatherParticleSystemParticleType.particle_count at
                                    //      +0xa4
    float field_extent;             // 0x04 fade_out_end_distance at +0x30; the size of the box
                                    //      particles are scattered inside
    int16_t particle_count;         // 0x08 live weather_particle records on the list
    int16_t unknown_0a;             // 0x0a padding
    datum_index first_particle;     // 0x0c -1 when empty, and -1 at activate
} weather_instance_type;            // size 0x10

// ---------------------------------------------------------------------------
// weather_instance  (element of weather_instances, 0x006b0ae4)
// weather_instance_activate 0x457e20 and weather_instance_deactivate 0x457f00 own
// definition_index and the type array, weather_instance_update 0x458420 owns elapsed_time and
// delta_time, and weather_update_local_player 0x458a90 owns the render origin, cluster and sky
// flag.
// ---------------------------------------------------------------------------
typedef struct weather_instance {
    datum_index definition_index;   // 0x00 the WeatherParticleSystem tag, -1 when the slot is
                                    //      free
    float elapsed_time;             // 0x04 seconds since activate, accumulated by delta_time
    float delta_time;               // 0x08 seconds since the previous RENDERED frame, copied from
                                    //      0x007c3110 render_time_since_frame (render.h) by
                                    //      weather_instance_update (0x458429); render_frame stores
                                    //      that frame delta at 0x50bec2. Not a per-tick delta (R45)
    float intensity;                // 0x0c scales the per type target count
    uint32_t unknown_10;            // 0x10 copied from 0x007c3344, handed to FUN_0053ed60 as the
                                    //      sample point and to the render submit as the field
                                    //      origin. UNSURE of its real type
    int16_t unknown_14;             // 0x14 copied from 0x007c3348
    int16_t unknown_16;             // 0x16 never written
    int16_t cluster_index;          // 0x18 FUN_0053ed60 output, -1 when outside the BSP
    uint8_t in_sky;                 // 0x1a FUN_0053ed60 return; picks render mode 5 or 7
    uint8_t unknown_1b;             // 0x1b padding
    weather_instance_type types[8]; // 0x1c one per WeatherParticleSystemParticleType
} weather_instance;                 // size 0x9c

// ---------------------------------------------------------------------------
// weather_particle  (element of weather_particle_data, 0x0087abcc)
// weather_particle_new 0x458070 writes every field, weather_instance_update 0x458420 advances
// frame and rotation, and weather_particle_update 0x458630 owns velocity, acceleration and the
// position jitter.
// ---------------------------------------------------------------------------
typedef struct weather_particle {
    uint16_t identifier;            // 0x00 datum header
    uint16_t unknown_02;            // 0x02 never written
    real_point3d position;          // 0x04 random inside the field box, relative to the field
                                    //      origin
    real_vector3d velocity;         // 0x10 zero at create
    real_vector3d acceleration;     // 0x1c a random unit direction times a random magnitude in
                                    //      acceleration_magnitude at +0xcc, then blended each
                                    //      tick by acceleration_change_rate at +0xd4 and turned
                                    //      by acceleration_turning_rate at +0xd8
    int16_t sequence_index;         // 0x28 random in [0, bitmap sequence count)
    int16_t unknown_2a;             // 0x2a padding
    float frame;                    // 0x2c random start, advanced by animation_rate
    float rotation;                 // 0x30 random in 0..2pi when random_rotation is set, else 0
    float alpha;                    // 0x34 lerp of the two colour bound alphas at +0x134 and
                                    //      +0x144
    ColorRGB color;                 // 0x38 color_interpolate over the colour bounds
    float radius;                   // 0x44 random in particle_radius at +0xfc
    float rotation_rate;            // 0x48 random in rotation_rate at +0x10c, signed by the
                                    //      parity of the datum index
    float animation_rate;           // 0x4c random in animation_rate at +0x104
    datum_index next_particle;      // 0x50 -1 at the tail
} weather_particle;                 // size 0x54

// ===========================================================================
// ambient colour noise grid (0x53f860..0x53fc80)
// A 3 by 8 by 8 grid of vectors at 0x00746284, exactly 0x900 bytes.
// ambient_color_randomize 0x53fa70 rolls the column 0 entry of each of the 8 rows in each of the
// 3 bands out of sphere_point_table and then fills columns 1 to 7 with
// vector3d_catmull_rom_interpolate, so walking a row is a smooth transition between successive
// keyframes; the row stride is 0x60 and the band stride is 0x300.
// ambient_color_sample 0x53fc80 hashes a position and the frame counter into one entry per band,
// weights the three by the 0.1, 0.2 and 0.07 literals and scales the sum by one third of the
// caller intensity. ambient_color_for_marker 0x53f940 blends that against the wind colour of the
// weather palette row the marker sits in.
// ===========================================================================
typedef struct ambient_noise_grid {
    real_vector3d entries[3][8][8]; // 0x000 indexed as [band][row][column]
} ambient_noise_grid;               // size 0x900

// ===========================================================================
// player_effect (0x456730..0x457d50)
// This group is not part of the effe, pctl or part family: it is the per local player screen and
// camera feedback the damage system drives, keyed on player.local_player_index (player record
// +0x02, stride 0x200 in the table at 0x0087a480). It is defined here only because no other
// module header owns it yet; see out/phase4/effects_types_notes.md.
// The tag is ContinuousDamageEffect, proved by player_effect_apply_continuous_damage 0x4567c0:
// radius at 0x00 and 0x04 drive the distance falloff, and the four accumulators come from
// low_frequency_vibrate_frequency at 0x24, high_frequency_vibrate_frequency at 0x28,
// camera_shaking_random_translation at 0x44 and camera_shaking_random_rotation at 0x48, with the
// wobble taken from camera_shaking_wobble_period at 0x5c and camera_shaking_wobble_weight at
// 0x60 through periodic_function_evaluate.
// ===========================================================================
typedef enum player_effect_flags {
    _player_effect_screen_flash_bit = 0x00000001, // set by player_effect_set_screen_flash
                                                  // 0x4578a0
    _player_effect_camera_impulse_bit = 0x00000002,// set by player_effect_set_camera_impulse
                                                  // 0x4579b0
    _player_effect_camera_shake_bit = 0x00000004   // set by player_effect_set_camera_shake
                                                  // 0x457d50
} player_effect_flags;

// The int16 table at 0x00687218 maps a screen flash type index to a rasterizer pass, and entry
// 0 is 0, which is how player_effect_set_screen_flash 0x4578a0 rejects the none type.

// ---------------------------------------------------------------------------
// player_screen_flash  (0x38, the 14 dwords player_effect_set_screen_flash copies)
// ---------------------------------------------------------------------------
typedef struct player_screen_flash {
    int16_t type;                   // 0x00 index into the table at 0x00687218
    int16_t unknown_02;             // 0x02
    uint32_t unknown_04;            // 0x04
    uint32_t unknown_08;            // 0x08
    uint32_t unknown_0c;            // 0x0c
    float duration;                 // 0x10 seconds; multiplied by the caller scale and 30 to
                                    //      give the tick count
    uint32_t unknown_14;            // 0x14
    uint32_t unknown_18;            // 0x18
    uint32_t unknown_1c;            // 0x1c
    uint32_t unknown_20;            // 0x20
    float intensity;                // 0x24 the descriptor bounds blended by the caller falloff
                                    //      and clamped to the maximum
    ColorARGB color;                // 0x28
} player_screen_flash;              // size 0x38

// ---------------------------------------------------------------------------
// player_camera_impulse  (0x34, the 13 floats player_effect_set_camera_impulse copies)
// ---------------------------------------------------------------------------
typedef struct player_camera_impulse {
    float duration;                 // 0x00 seconds, scaled to ticks the same way
    uint32_t unknown_04;            // 0x04
    float unknown_08;               // 0x08
    float unknown_0c;               // 0x0c
    float magnitude_minimum;        // 0x10 bounds of the random magnitude the direction is
    float magnitude_maximum;        // 0x14 scaled by
    float intensity;                // 0x18 the blend the next impulse must beat to replace this
                                    //      one
    uint32_t unknown_1c;            // 0x1c
    uint32_t unknown_20;            // 0x20
    uint32_t unknown_24;            // 0x24
    uint32_t unknown_28;            // 0x28
    uint32_t unknown_2c;            // 0x2c
    uint32_t unknown_30;            // 0x30
} player_camera_impulse;            // size 0x34

// ---------------------------------------------------------------------------
// player_camera_shake  (0x48, the 18 floats player_effect_set_camera_shake copies)
// ---------------------------------------------------------------------------
typedef struct player_camera_shake {
    float duration;                 // 0x00 seconds, scaled to ticks
    uint32_t unknown_04;            // 0x04
    uint32_t unknown_08;            // 0x08
    uint32_t unknown_0c;            // 0x0c
    uint32_t unknown_10;            // 0x10
    uint32_t unknown_14;            // 0x14
    uint32_t unknown_18;            // 0x18
    uint32_t unknown_1c;            // 0x1c
    float unknown_20;               // 0x20 also multiplied by the caller scale and 30
    uint32_t unknown_24;            // 0x24
    float intensity;                // 0x28 the blend the next shake must beat to replace this
                                    //      one
    uint32_t unknown_2c;            // 0x2c
    uint32_t unknown_30;            // 0x30
    uint32_t unknown_34;            // 0x34
    uint32_t unknown_38;            // 0x38
    uint32_t unknown_3c;            // 0x3c
    uint32_t unknown_40;            // 0x40
    uint32_t unknown_44;            // 0x44
} player_camera_shake;              // size 0x48

// ---------------------------------------------------------------------------
// player_effect  (one per local player, at the start of player_effect_globals)
// player_effect_clear_dead_players 0x456730 zeroes the whole record, the three set helpers own
// their sub blocks and their tick counters, player_effect_apply_continuous_damage 0x4567c0 owns
// the four accumulators and cancels a one shot through vibrate_ticks,
// player_effect_fade_damage_indicators 0x457220 fades the four indicator bytes up to 255, and
// player_effect_mark_damage_direction 0x456cf0 sets one of them to 1 from the angle between the
// camera forward and the damage source.
// ---------------------------------------------------------------------------
typedef struct player_effect {
    real_vector3d impulse_direction;// 0x00 cos and sin of a random yaw, k left at 0
    real_vector3d impulse_rotation; // 0x0c the rotation axis, scaled by the random magnitude
    player_screen_flash flash;      // 0x18
    player_camera_impulse impulse;  // 0x50
    player_camera_shake shake;      // 0x84
    float low_frequency_vibrate;    // 0xcc accumulated ContinuousDamageEffect +0x24
    float high_frequency_vibrate;   // 0xd0 accumulated ContinuousDamageEffect +0x28
    float shake_translation;        // 0xd4 accumulated ContinuousDamageEffect +0x44
    float shake_rotation;           // 0xd8 accumulated ContinuousDamageEffect +0x48
    int16_t vibrate_ticks;          // 0xdc when positive a one shot effect is running, and the
                                    //      four accumulators above are reset before a continuous
                                    //      one is added
    int16_t flash_ticks;            // 0xde
    int16_t impulse_ticks;          // 0xe0
    int16_t shake_ticks;            // 0xe2
    uint8_t damage_indicator_alpha[4];// 0xe4 the four directional hit markers; 0 is off and 255
                                    //      is fully faded
    uint32_t flags;                 // 0xe8 player_effect_flags
} player_effect;                    // size 0xec

// ---------------------------------------------------------------------------
// player_effect_globals  (the block player_effect_globals_pointer 0x006f1884 points at)
// One player_effect per local player followed by the scripted whole screen effect the HS
// player_effect_set family drives. player_effect_build_screen_flash 0x457000 reads the scripted
// block first and only falls back to the per player flash when it is inactive, and
// player_effect_build_camera_shake_matrix 0x457390 does the same for the shake.
// ---------------------------------------------------------------------------
typedef struct player_effect_globals {
    player_effect players[1];       // 0x000 one element; every access in the module is to
                                    //       base + i * 0xec with i the local player index, and
                                    //       every absolute access lands past the first record
    ColorRGB scripted_flash_color;  // 0x0ec
    int32_t scripted_flash_start_tick;// 0x0f8
    int16_t scripted_flash_ticks;   // 0x0fc -1 when no scripted flash is running
    uint8_t scripted_flash_fade_in; // 0x0fe 0 fades the intensity out instead of in
    uint8_t unknown_0ff;            // 0x0ff padding
    float scripted_shake_rotation[3];// 0x100 per axis random rotation amplitudes
    float scripted_shake_translation[3];// 0x10c per axis random translation amplitudes
    float scripted_shake_intensity; // 0x118
    int16_t scripted_shake_ticks;   // 0x11c counted down by the tick length at 0x006f1d6c +0x10
    int16_t scripted_shake_duration;// 0x11e the value scripted_shake_ticks started at
    uint32_t scripted_shake_flags;  // 0x120 bit 0 active, bit 1 fade direction
} player_effect_globals;            // size 0x124

// ===========================================================================
// globals this module owns
// ===========================================================================
// contrails
// global 0x0087abec: data_array *contrail_data                 contrails_initialize, 0x100 contrail
// global 0x0087abe8: data_array *contrail_point_data           contrails_initialize, 0x400 contrail_point
//
// decals
// global 0x0087abe4: data_array *decal_data                    decals_initialize, 0x800 decal
// global 0x006b0ad8: decal_grid *decal_grid_block              0x280c bytes of game state
// global 0x0071d1c0: void *decal_geometry_cache                cache_allocate_block and cache_evict_entry
// global 0x0071d1bc: void *decal_geometry_cache_secondary      second handle, UNSURE of its role
// global 0x00687004: uint8_t decals_enabled                    decal_spawn_for_response 0x44ece0
// global 0x006893f5: uint8_t decals_for_all_responses          the other half of that check
// 0x0069c632: int16_t rasterizer_vertex_buffer_lock_state (rasterizer.h) -- NOT owned here (R77).
//   All 17 accesses in .text are WORD stores (mov WORD PTR ds:0x69c632,...); decal_place 0x44edc0
//   is one of the writers. Formerly listed here as "uint8_t decal_place_unknown".
//
// effects
// global 0x0087abdc: data_array *effect_data                   0xfc effect
// global 0x0087abe0: data_array *effect_location_data          0x3c effect_location_marker
// global 0x006b0adc: void *effect_marker_callback_context      the scratch block the marker
//                                                             resolution thunk at 0x00451850 reads
// global 0x00687014: uint8_t first_person_effects_enabled      effect_new_on_object 0x4507a0 and 0x450870
// global 0x0068944c: int32_t effect_light_budget               effect_event_apply skips ligh parts at 0
//
// particle systems and particles
// global 0x0087abd4: data_array *particle_system_data          0x158 particle_system
// global 0x0087abd8: data_array *particle_system_particle_data 0x80 particle_system_particle
// global 0x0087abd0: data_array *particle_data                 0x70 particle
// global 0x0069c566: uint8_t particle_systems_enabled          particle_system_new 0x453600 and 0x4536f0
// global 0x0069c565: uint8_t effect_particles_enabled          effect_spawn_particles 0x451f90; the
//                                                             value 1 selects a deterministic path
// global 0x0065743c: void *particle_system_update_physics[2]   .rdata dispatch table
// global 0x00657444: void *particle_creation_physics[3]        .rdata dispatch table
// global 0x00657450: void *particle_update_physics[1]          .rdata dispatch table
//
// weather
// global 0x0087abcc: data_array *weather_particle_data         0x54 weather_particle
// global 0x006b0ae0: int32_t weather_instance_count            active weather_instance slots
// global 0x006b0ae4: weather_instance weather_instances[1]     0x9c each
// global 0x00746b84: int16_t weather_particle_system_count     Scenario weather_palette count
// global 0x00746b88: weather_particle_system_state weather_wind_states[8]
//                                                             0x20 each; the element count is
//                                                             inferred from the scenario palette
//                                                             maximum, only the stride is proved
// global 0x00746f88: int32_t weather_frame_counter             bumped once per weather_update
// global 0x00687350: uint8_t weather_enabled                   weather_update_local_player 0x458a90
//
// ambient colour noise
// global 0x00746284: ambient_noise_grid ambient_noise          0x900 bytes, 3 by 8 by 8 vectors
//
// player effect
// global 0x006f1884: player_effect_globals *player_effect_globals_pointer
// global 0x00719ccc: int32_t player_effect_reentry_count       bumped and dropped around
//                                                             player_effect_mark_damage_direction
// global 0x00687218: int16_t screen_flash_pass[8]              type index to rasterizer pass
// 0x006b7020: main.h console_globals.active (the console-open byte) -- NOT owned here (R08).
//   The console key handler 0x4c65c0 returns mov al,ds:0x6b7020; player_effect_build_screen_flash
//   0x457000 only reads it (mov al,ds:0x6b7020 at 0x457001) to skip the flash while the console
//   is open. Formerly listed here as "uint8_t player_effect_suppressed".
//
// random number generation
// global 0x00719cd4: random_seed effect_random_seed            the effects local stream, advanced
//                                                             by effect_random_fraction 0x4505b0,
//                                                             effect_random_range 0x44c800 and
//                                                             effect_random_scaled_range 0x44c840.
//                                                             Effects whose definition carries
//                                                             must_be_deterministic_pc use
//                                                             random_seed_global instead
//                                                             (0x00719cd0, types/math.h)
//
// .rdata constant tables
// global 0x006573f8: decal_type_parameters k_decal_type_parameters[4]
// (0x0065c29c projection_axis_pair k_projection_axes[6] is declared by types/math.h)

#pragma pack(pop)
