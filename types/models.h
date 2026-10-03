// Blam models module (halo.exe 1.0.10 retail, 0x4d4810..0x4d7850, 29 Ghidra functions).
// Despite the name, most of this range is the animation sampler: it reads a ModelAnimations
// tag (types/tags.h) and produces one real_orientation per node. The rest is the node
// hierarchy walk that turns those orientations into real_matrix4x3 node matrices, the two
// marker and animation name lookups, and the model render entry point with its part loop.
// Groups of functions:
//   - animation frame sampling: 0x4d4810 frame data pointer, 0x4d4a80 base frame, 0x4d4dd0
//     replacement frame, 0x4d4f90 / 0x4d51a0 overlay frame (added in / weighted), 0x4d53f0 /
//     0x4d57d0 fractional frame (plain / weighted overlay), 0x4d5c00 aiming screen blend
//     (a 2D yaw by pitch grid of overlay frames), 0x4d49b0 / 0x4d4a00 root node matrix and
//     root translation delta, 0x4d4850 frame info distance sum.
//   - compressed animation decoding: 0x4d6b60 / 0x4d6cf0 / 0x4d6e80 per node rotation,
//     translation and scale curves, 0x4d6b10 keyframe time binary search, 0x4d6330
//     int16x4 quaternion decode, 0x4d6380 48 bit quaternion decode.
//   - animation graph: 0x4d48d0 advance an animation_state by one frame, 0x4d6280 weighted
//     random pick along a next_animation chain, 0x4d6ab0 animation index by name.
//   - node transforms: 0x4d7610 default orientations, 0x4d69e0 blend two orientation arrays,
//     0x4d6880 / 0x4d7690 node matrices from orientations (animation graph nodes / model
//     nodes), 0x4d6440 two bone IK.
//   - markers: 0x4d77c0 marker index by name, 0x4d7850 fill object_marker records.
//   - rendering: 0x4d6fc0 render one model, 0x4d72a0 walk its regions, permutations and
//     geometry parts and hand each part to the rasterizer.
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself fixes a
// layout it is preferred over the decompiler and the fact is called out:
//
//   - The tag records every function here reads are already defined in types/tags.h and are
//     NOT redefined. The arithmetic of this module re-derives and agrees with each of them:
//     ModelAnimations (0x80: sound_references +0x54, nodes +0x68, animations +0x74),
//     ModelAnimationsAnimation (0xb4, every field from type +0x20 to frame_data +0xa0),
//     ModelAnimationsAnimationGraphNode (0x40: next_sibling +0x20, first_child +0x22,
//     parent +0x24), ModelAnimationsAnimationGraphSoundReference (0x14, tag_id +0x0c),
//     the frame info records ModelAnimationsFrameInfoDxDy / DxDyDyaw / DxDyDzDyaw,
//     GBXModel (0xe8: flags +0x00, node_list_checksum +0x04, cutoffs +0x08..+0x18,
//     base_map_u/v_scale +0x30/+0x34, markers +0xac, nodes +0xb8, regions +0xc4,
//     geometries +0xd0, shaders +0xdc), ModelNode (0x9c: default_translation +0x28,
//     default_rotation +0x34, the inverse bind matrix +0x68..+0x9b), ModelMarker (0x40),
//     ModelMarkerInstance (0x20), ModelRegion (0x4c), ModelRegionPermutation (0x58, the five
//     geometry indices +0x40..+0x48), GBXModelGeometry (0x30), GBXModelGeometryPart (0x84),
//     ModelShaderReference (0x20), Shader (+0x24 shader_type) and ShaderModel (+0x28 flags).
//
//   - real_orientation is the 0x20 element of every node array in this module: 0x4d7610
//     writes it field by field (rotation from ModelNode +0x34, translation from +0x28, scale
//     1.0), and every sampler steps it by 0x20. The stack arrays of 0x4d49b0 (0x800 bytes)
//     and 0x4d4a00 (two of 0x800) hold 64 of them.
//
//   - animation_compressed_header is the 11 dword offset table at
//     frame_data.pointer + offset_to_compressed_data; the three curve evaluators read every
//     dword of it (see the struct).
//
//   - animation_aiming_screen is the 0x18 block 0x4d5c00 takes by pointer. Its fields match
//     ModelAnimationsAnimationGraphUnitSeat +0x20..+0x37, ModelAnimationsAnimationGraphWeapon
//     +0x60..+0x77 and ModelAnimationsAnimationGraphVehicleAnimations +0x00..+0x17 in
//     types/tags.h one for one. The vehicle caller 0x571974 passes vehicles[0] itself; the
//     unit callers 0x563f46 / 0x56408c read +0x00, +0x04, +0x0a, +0x0c, +0x10, +0x14, +0x16 of
//     the block they pass to scale the aiming bounds they store at unit +0x2b8..+0x2d4.
//
//   - The object_marker record that model_markers_get_by_name 0x4d7850 fills is the one in
//     types/objects.h (0x6c) and is NOT redefined; this module pins it (see the notes file).
//
//   - The model draw context 0x4d6fc0 builds on its stack (ebp-0xdc .. ebp-0x11) is the
//     rasterizer_model_draw_context of types/rasterizer.h and is NOT redefined; 0x4d6fc0 is the
//     producer that file was missing and it fills all 0xcc bytes (see the notes file).
//     What 0x4d6fc0 stores, by context offset: +0x00 flags, +0x04 object_index, +0x08 the
//     local node matrix array, +0x0c node count (word), +0x10 render_lighting (0x74 bytes,
//     from the lighting argument, no default), +0x84 change_colors pointer, +0x88
//     function_out_values pointer (rasterizer.h still calls these two unknown_84[2]), +0x8c
//     the 0x28 byte render_model_effect, +0xb4 bounding_center (defaults to the root node
//     matrix position, node_matrices + 0x28, when NULL), +0xc0 bounding_radius, +0xc4 / +0xc8
//     GBXModel base_map_u_scale / base_map_v_scale.
//
// Types this module operates on that already have a definition elsewhere:
//   types/math.h        real_quaternion, real_point3d, real_vector3d, real_matrix4x3
//   types/objects.h     object_marker (0x4d7850 output)
//   types/rasterizer.h  rasterizer_model_draw_context, rasterizer_node_matrices (the third stack
//                       argument of 0x4d72a0: the matrices pointer and node count at context
//                       +0x08), render_lighting, rasterizer_geometry_group_parameters,
//                       transparent_geometry_group_link (the first 0x0c bytes of
//                       model_part_group_link below)
//   types/render.h      render_model_effect (0x4d6fc0 eighth argument, 0x28 bytes)
//   types/cache.h       tag_instance (0x0087bc14, tag data at +0x14; every tag handle this
//                       module takes is resolved through it)
//   types/tags.h        the tag records listed above, ColorRGB
//
// Functions in this range that are misnamed or misattributed are listed at the end of
// out/phase4/models_types_notes.md.

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// module wide constants
// ---------------------------------------------------------------------------
typedef enum model_constants {
    k_maximum_nodes_per_model = 64,              // 0x4d49b0 / 0x4d4a00 orientation arrays of
                                                 // 0x800 bytes, the 0xd00 byte matrix array of
                                                 // 0x4d6fc0, the int16 queue[64] of the node
                                                 // walks 0x4d6880 / 0x4d7690, and the two
                                                 // uint32 bit masks per animation (node >> 5)
    k_model_level_of_detail_count = 5,           // 0x4d6fc0 counts down from 4 to 0
    k_maximum_model_part_group_links = 32,       // 0x4d72a0 stops recording links at 0x20
    k_model_render_pass_count = 3,               // 0x4d72a0 passes 0, 1, 2
    k_animation_keyframe_count_mask = 0xfff,     // low 12 bits of a keyframe header
    k_animation_keyframe_index_shift = 12,       // high 20 bits: first keyframe index
    k_model_first_person_node_list_checksum = 0x0769c097 // 0x4d6fc0 compares GBXModel +0x04
                                                 // against it before arming the first person
                                                 // sort flag 0x007c0478
} model_constants;

// The int16 quaternion components of the uncompressed frame and default data, and all four
// components of the 48 bit form, are scaled by 1/32767 (the float 3.051851e-05 at
// 0x00672bd0). 0x4d6280 scales the high 16 bits of its random seed by 1/65535 (0x00672b84).
// 0x4d6440 clamps the reach of the IK chain to 0.98 of the two bone lengths (0x00672ff4).

// ---------------------------------------------------------------------------
// model_level_of_detail  (0x4d6fc0 local at ebp-0x08, handed to 0x4d72a0 as the fourth
// argument)
// One index selects both the pixel cutoff and the permutation geometry: 0x4d6fc0 reads the
// float at GBXModel +0x08 + lod*4 and 0x4d72a0 reads the int16 at ModelRegionPermutation
// +0x40 + lod*2. So lod 0 pairs the cutoff that tags.h calls super_high_detail_cutoff with
// the geometry tags.h calls super_low. The code meaning is fixed: +0x08 is the minimum pixel
// size at which the model is drawn at all (unless the immediate flag is set), and the search
// starts at lod 4 and stops at the first lod whose cutoff is at or below the pixel size.
// The console global at 0x006893e8 (int16, -1 by default) overrides the result and is
// clamped to 0..4.
// ---------------------------------------------------------------------------
typedef enum model_level_of_detail : int {
    _model_lod_super_low = 0,    // permutation +0x40, cutoff GBXModel +0x08
    _model_lod_low = 1,          // permutation +0x42, cutoff +0x0c
    _model_lod_medium = 2,       // permutation +0x44, cutoff +0x10
    _model_lod_high = 3,         // permutation +0x46, cutoff +0x14
    _model_lod_super_high = 4    // permutation +0x48, cutoff +0x18
} model_level_of_detail;

// ---------------------------------------------------------------------------
// real_orientation  (the per node local transform every sampler reads and writes)
// Writers: model_nodes_get_default_transforms 0x4d7610 (all four fields, scale = 1.0),
// 0x4d4a80 (rotation +0x00..+0x0c, translation +0x10..+0x18, scale +0x1c), 0x4d4dd0
// (replace), 0x4d4f90 (quaternion_multiply into +0x00, add into +0x10, multiply +0x1c),
// 0x4d51a0 / 0x4d57d0 (the weighted forms), 0x4d53f0, 0x4d5c00, 0x4d69e0 (lerp both arrays).
// Readers: 0x4d6880 and 0x4d7690 feed +0x00 to matrix4x3_from_quaternion 0x4cbad0 and copy
// +0x1c into matrix.scale and +0x10 into matrix.position; 0x4d49b0 and 0x4d4a00 read node 0.
// ---------------------------------------------------------------------------
typedef struct real_orientation {
    real_quaternion rotation;       // 0x00 (i, j, k, w)
    real_point3d translation;       // 0x10
    float scale;                    // 0x1c
} real_orientation;                 // size 0x20

// ---------------------------------------------------------------------------
// animation_state  (the ESI in/out block of animation_state_advance 0x4d48d0)
// Callers pass the address of a pair embedded in a larger record: object +0x0d0 / +0x0d2
// (types/objects.h animation_index / animation_frame) and the first person weapon record
// (0x49324e passes ebp+0x16). 0x4d48d0 reads +0x00 as the animation index (stride 0xb4 into
// ModelAnimations.animations) and increments, compares and rewrites +0x02.
// ---------------------------------------------------------------------------
typedef struct animation_state {
    int16_t animation_index;        // 0x00 ModelAnimations.animations index; 0x4d48d0 replaces
                                    //      it with the 0x4d6280 pick at the end of a
                                    //      non looping animation
    int16_t frame_index;            // 0x02 incremented first, then compared against
                                    //      frame_count (+0x22), loop_frame_index (+0x2e) and
                                    //      the two key frame indices (+0x34, +0x36)
} animation_state;                  // size 0x04

// Return value of animation_state_advance 0x4d48d0 (the callers at 0x493256 and 0x49325c test
// 1 and 2).
typedef enum animation_state_advance_result : int {
    _animation_advance_none = 0,            // an ordinary frame
    _animation_advance_key_frame = 1,       // frame_index == key_frame_index or
                                            //   second_key_frame_index
    _animation_advance_last_frame = 2,      // frame_index + 1 == frame_count and the animation
                                            //   does not loop (loop_frame_index == 0)
    _animation_advance_next_animation = 3,  // ran off the end: animation_index = the 0x4d6280
                                            //   pick starting at main_animation_index (+0x42),
                                            //   frame_index = 0
    _animation_advance_looped = 4           // ran off the end: frame_index =
                                            //   min(loop_frame_index, frame_count - 1)
} animation_state_advance_result;

// The first stack argument of 0x4d48d0 and of 0x4d6280: which random stream picks the next
// animation. 1 advances random_seed_global 0x00719cd0, anything else local_random_seed
// 0x00719cd4 (both types/math.h, LCG 0x19660d / 0x3c6ef35f).
typedef enum animation_random_stream : int {
    _animation_random_local = 0,
    _animation_random_global = 1
} animation_random_stream;

// ---------------------------------------------------------------------------
// animation_quaternion48  (the 6 byte rotation of the compressed animation codec)
// Decoded by animation_quaternion48_decode 0x4d6380 (ECX = source, ESI = real_quaternion out)
// into four 12 bit fields, each shifted up to 16 bits and scaled by 1/32767:
//   i = (w0 & 0xfff0) | (w0 >> 12)
//   j = (w0 << 12) | (w0 & 0xf) | ((w1 >> 4) & 0x0ff0)
//   k = (w1 << 8) | (((w1 & 0xf0) | ((w2 >> 4) & 0xf00)) >> 4)
//   w = (w2 << 4) | ((w2 >> 8) & 0xf)
// The decoder itself does not normalize; its caller 0x4d6b60 runs quaternion_normalize
// 0x4cdb20 on the result. The stride of 6 is fixed by the lea x*3 then *2 index arithmetic
// in 0x4d6b60.
// ---------------------------------------------------------------------------
typedef struct animation_quaternion48 {
    uint16_t packed[3];             // 0x00 w0, w1, w2 above
} animation_quaternion48;           // size 0x06

// The uncompressed stream uses four int16 components (types/tags.h ModelAnimationsRotation,
// 8 bytes), decoded by animation_quaternion16_decode 0x4d6330 (ECX = source, EAX = out) with
// no normalize.

// ---------------------------------------------------------------------------
// animation_compressed_header  (at ModelAnimationsAnimation frame_data.pointer (+0xac) +
// offset_to_compressed_data (+0x88))
// Eleven dword offsets, each relative to the start of this header, followed directly by the
// rotation keyframe headers. Readers: animation_node_get_rotation 0x4d6b60 (+0x00, +0x04,
// +0x08 and the inline headers at +0x2c), animation_node_get_translation 0x4d6cf0
// (+0x0c..+0x18), animation_node_get_scale 0x4d6e80 (+0x1c..+0x28).
// A keyframe header is a uint32: bits 0..11 keyframe count (0 means the node uses its
// default), bits 12..31 index of its first keyframe in the time and value arrays. Keyframe
// times are uint16 frame numbers searched by 0x4d6b10 (EAX = count, BX = frame, stack =
// times). Frames before the first keyframe interpolate from the default value at time 0.
// Headers are indexed by the running count of animated nodes of that kind; defaults are
// indexed by node index (rotation, translation) - the scale evaluator indexes both its header
// and its default with the same DX value.
// Only used when the animation has compressed_data (flags +0x3a bit 0) and the global at
// 0x006894b4 is set (it is 1 and nothing writes it); 0x4d4a80 and the others also take this
// path when offset_to_compressed_data is 0.
// ---------------------------------------------------------------------------
typedef struct animation_compressed_header {
    int32_t rotation_keyframe_times;        // 0x00 uint16[], one run per animated node
    int32_t rotation_defaults;              // 0x04 animation_quaternion48[node count]
    int32_t rotation_keyframes;             // 0x08 animation_quaternion48[]
    int32_t translation_keyframe_headers;   // 0x0c uint32[animated translation count]
    int32_t translation_keyframe_times;     // 0x10 uint16[]
    int32_t translation_defaults;           // 0x14 real_point3d[node count]; 0x4d4a80 also reads
                                            //      it for nodes without a translation bit
    int32_t translation_keyframes;          // 0x18 real_point3d[]
    int32_t scale_keyframe_headers;         // 0x1c uint32[]
    int32_t scale_keyframe_times;           // 0x20 uint16[]
    int32_t scale_defaults;                 // 0x24 float[]
    int32_t scale_keyframes;                // 0x28 float[]
} animation_compressed_header;              // size 0x2c, followed by
                                            // uint32 rotation_keyframe_headers[animated rotation count]

// Uncompressed data (no struct, a byte stream): the frame at frame_data.pointer +
// frame_size (+0x24) * frame holds, for each node in order, an int16x4 rotation if the
// bit is set in node_rotation_flag_data (+0x6c), a real_point3d if set in
// node_transform_flag_data (+0x5c) and a float scale if set in node_scale_flag_data (+0x7c).
// default_data.pointer (+0x98) holds the same three kinds, in the same order, for every node
// whose bit is clear. 0x4d4a80 walks both streams in one pass.

// ---------------------------------------------------------------------------
// animation_aiming_screen  (the first stack argument of animation_aiming_screen_blend 0x4d5c00)
// Yaw selects +0x04 when positive, else +0x00, and the frame column is clamped to
// [-right_frame_count, left_frame_count - 1] before right_frame_count is added back; pitch
// does the same with +0x10 / +0x0c and +0x14 / +0x16. The animation must be an overlay
// (type 1) with at least (right + left + 1) * (down + up + 1) frames; four corner frames are
// fetched through 0x4d4810 and bilinearly blended into the orientation array.
// ---------------------------------------------------------------------------
typedef struct animation_aiming_screen {
    float right_yaw_per_frame;      // 0x00 divisor for yaw <= 0
    float left_yaw_per_frame;       // 0x04 divisor for yaw > 0
    uint16_t right_frame_count;     // 0x08
    uint16_t left_frame_count;      // 0x0a
    float down_pitch_per_frame;     // 0x0c divisor for pitch <= 0
    float up_pitch_per_frame;       // 0x10 divisor for pitch > 0
    uint16_t down_pitch_frame_count; // 0x14
    uint16_t up_pitch_frame_count;  // 0x16
} animation_aiming_screen;          // size 0x18

// ---------------------------------------------------------------------------
// model_render_flags  (the eleventh stack argument of render_model 0x4d6fc0, passed on as the
// sixth argument of model_render_parts 0x4d72a0)
// Each bit is mapped onto rasterizer_model_draw_context.flags (types/rasterizer.h); the
// GBXModel flags add 0x200 for ignore_skinning (bit 2) and 0x100 for parts_have_local_nodes
// (bit 1).
// ---------------------------------------------------------------------------
typedef enum model_render_flags {
    _model_render_flag_1_bit = 0x01,            // context flags |= 0x1f; 0x4d72a0 records no
                                                // part group links
    _model_render_immediate_bit = 0x02,         // skip the cutoff test, upload skinning at once
                                                // and install the context at 0x0071d260
                                                // instead of FUN_00526f50 / FUN_0052b530;
                                                // 0x4d72a0 runs only pass 0 and draws through
                                                // FUN_00531350
    _model_render_outside_fog_plane_bit = 0x04, // context flags |= 0x40; render_object 0x50f044
                                                // sets it from object_render_data +0x09
    _model_render_frustum_z_bit = 0x08          // context flags |= 0x80
} model_render_flags;

// The three passes of model_render_parts 0x4d72a0, selected by the shader of the part
// (Shader.shader_type at +0x24; only types 3..11 are drawn).
typedef enum model_render_pass {
    _model_render_pass_opaque = 0,              // environment (3) and model (4) shaders:
                                                // FUN_0052b050, or FUN_00531350 when immediate
    _model_render_pass_model_decal = 1,         // model shaders with ShaderModel flags bit 3
                                                // (alpha_blended_decal): FUN_0052b050
    _model_render_pass_transparent = 2          // types 5..11: FUN_0052b180, which builds a
                                                // transparent_geometry_group
} model_render_pass;

// ---------------------------------------------------------------------------
// model_part_group_link  (0x4d72a0 local array at esp+0x38, 32 records of 0x10)
// The first 0x0c bytes are the transparent_geometry_group_link that FUN_0052b180 writes
// through EAX (types/rasterizer.h: two int16 pointers into the new group and its group
// index). 0x4d72a0 records a link only while fewer than 32 are held, the group index is
// not -1, flag bit 0 is clear and the part has a positive prev_filthy_part_index (+0x06)
// or next_filthy_part_index (+0x07); it then stores the next_filthy_part_index of the part and
// its own part index. After each region loop it matches the linked_part_index of every record
// against the part_index of the other records and chains the two groups: this record gets
// next_group_index = other.group_index, other.previous_group_index = this group_index.
// ---------------------------------------------------------------------------
typedef struct model_part_group_link {
    uint32_t previous_group_index;  // 0x00 int16_t* written by FUN_0052b180
    uint32_t next_group_index;      // 0x04 int16_t* written by FUN_0052b180
    int16_t group_index;            // 0x08 written by FUN_0052b180, -1 when nothing was built
    int16_t linked_part_index;      // 0x0a GBXModelGeometryPart +0x07 (next_filthy_part_index),
                                    //      sign extended
    int16_t part_index;             // 0x0c index of the part inside its geometry
    int16_t unknown_0e;             // 0x0e never written or read
} model_part_group_link;            // size 0x10

// ---------------------------------------------------------------------------
// globals owned by this module
// ---------------------------------------------------------------------------
// global 0x006894b4: uint8_t animation_compressed_data_enabled   1 in .data, never written;
//                    read by 0x4d4810, 0x4d4a80, 0x4d4dd0, 0x4d4f90, 0x4d51a0, 0x4d53f0,
//                    0x4d57d0, 0x4d5c00 and nothing else
// global 0x006b7f08: float model_render_default_function_values[4]   0x4d6fc0 argument 4
//                    (object +0x134 function_out_values) when NULL; zero .bss
// global 0x006b7f18: render_model_effect model_render_default_effect  0x4d6fc0 argument 8 when
//                    NULL (types/render.h, 0x28 bytes, ends at 0x006b7f40); zero .bss
// global 0x006b7f40: uint8_t model_render_default_region_permutations[8]   0x4d6fc0 argument 2
//                    (object +0x180) when NULL; 0x20 bytes up to the next global
// global 0x006b7f60: ColorRGB model_render_default_change_colors[4]   0x4d6fc0 argument 3
//                    (object +0x1b8) when NULL; zero .bss
// global 0x007c0478: uint8_t model_render_first_person   written only by 0x4d6fc0 (1 for the
//                    first person checksum while global_scenario +0x3e bit 0 is set, cleared
//                    on every exit); read by FUN_0052b180 and 0x52b340 into
//                    transparent_geometry_group.first_person (types/rasterizer.h)
//
// Referenced, owned elsewhere:
//   0x0087bc14  tag_instance *tag_instances                 types/cache.h
//   0x00719cd0  random_seed random_seed_global              types/math.h
//   0x00719cd4  random_seed local_random_seed               types/math.h
//   0x00696664  matrix4x3_multiply function pointer         (called indirectly throughout)
//   0x007c3178  render frustum world_to_view                types/rasterizer.h; 0x4d6fc0 copies
//                                                           it into every node matrix when the
//                                                           node matrix argument is NULL
//   0x006893e8  int16 model level of detail override, -1    console debug toggle range
//                                                           0x006893e4..0x00689464 (rasterizer.h)
//   0x00746f8c  Scenario *global_scenario                   (cache)
//   0x007c1220, 0x0069c689, 0x006893f2, 0x0071d260, 0x0071d265   rasterizer state, see
//               types/rasterizer.h

#pragma pack(pop)
