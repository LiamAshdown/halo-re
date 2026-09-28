// Blam shaders module (halo.exe 1.0.10 retail, 0x53fd60..0x540295, 7 Ghidra functions).
// The runtime helpers the rasterizer asks of shader tag data, plus the numeric countdown timer
// that numeric transparent_chicago shaders display:
//   0x53fd60 chimera__shader_get_vertex_shader_permutation  shader in ECX (compared to -1, not 0),
//            returns the permutation in AX
//   0x53fde0 shader_is_decal (Ghidra FUN_0053fde0)  shader in ECX, returns the decal flag in AL
//            (types 5..7, 9, 10)
//   0x53fe30 shader_draw_before_water (Ghidra FUN_0053fe30)  shader in ECX, returns
//            draw_before_water in AL (types 5..7)
//   0x53fe50 shader_texture_animation_evaluate  ESI shader_texture_animation*, ECX
//            render_animation* (types/render.h, may be NULL), EBX / EDI the two rows of a
//            shader_texture_transform, stack: u scale, v scale, u offset, v offset, rotation
//            (degrees), time; callee pops nothing (plain ret)
//   0x540060 shader_environment_texture_scrolling_evaluate (Ghidra FUN_00540060)
//            ESI ShaderEnvironment*, stack: float *u_out, float *v_out, double time; evaluates
//            the u/v texture scrolling pair at ShaderEnvironment +0x150
//   0x5400c0 numeric_countdown_timer_get_digit (Ghidra game_timer_get_digit_pair)
//            digit index in AX, one digit returned in AX
//   0x540240 numeric_countdown_timer_update (Ghidra game_timer_update)  no arguments;
//            tail-called from game_effects_update 0x45b57f
// The C parameter order of 0x53fe50 (ECX, ESI, EBX, EDI, then the stack) and of 0x540060
// (stack, then ESI) follows the externs the rasterizer module already declares.
//
// Nearly everything these functions touch is tag data whose layout types/tags.h already
// carries. That layout is reused here, never redefined. Offsets the code uses, all consistent:
//   Shader (0x28)            +0x00 shader_flags (bit 2 transparent_lit), +0x24 shader_type
//   ShaderModel              +0x38 translucency (permutation 1 when > 0)
//   ShaderTransparentGeneric / Chicago / ChicagoExtended (types 5, 6, 7)
//                            +0x29 shader_transparent_*_flags (byte): bit 1 decal,
//                            bit 3 first_map_is_in_screenspace, bit 4 draw_before_water;
//                            +0x2a first_map_type
//   ShaderTransparentGlass   +0x28 flags low byte, bit 1 decal
//   ShaderTransparentMeter   +0x28 flags low byte, bit 0 decal
//   ShaderEnvironment        +0x150 u_animation_function, +0x154 period, +0x158 scale,
//                            +0x15c v_animation_function, +0x160 period, +0x164 scale
//   ShaderModel +0xfc, ShaderTransparentChicagoMap +0xa4, ShaderTransparentGenericMap +0x2c and
//   shader_effect +0x60 each hold one shader_texture_animation (the call sites load ESI with
//   exactly those offsets: 0x528eb3, 0x532419, 0x534335).
//
// shader_type 1 (effect) is the particle shader block embedded in Particle (+0xb0), Contrail
// (+0x84), ParticleSystemType, WeatherParticleSystem and Lightning (LightningShader). tags.h
// has it only as LightningShader and as flattened fields of the other tags, and several other
// headers already refer to it as shader_effect (types/render.h build_sprite_data.shader), so
// it is defined here under that name.
//
// Constants read by 0x53fe50: 0x00672ac0 0.0f, 0x00672ac4 1.0f, 0x00672e64 360.0f,
// 0x00672c38 0.017453292f (pi / 180).
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// module constants
// ---------------------------------------------------------------------------
typedef enum shaders_constants {
    k_numeric_countdown_timer_ticks_per_second = 30,    // 0x540240: game ticks * 1000 / 0x1e
    k_numeric_countdown_timer_milliseconds_per_second = 1000, // 0x540240 imul 0x3e8
    k_numeric_countdown_timer_digit_count = 10,          // 0x5400c0 jump table, index -1..8
    k_shader_texture_rotation_full_turn = 360            // 0x00672e64; 0 and 360 skip fsin/fcos
} shaders_constants;

// ---------------------------------------------------------------------------
// Shader.shader_flags bits (+0x00, word). Only bit 2 is read in this module (0x53fdad).
// ---------------------------------------------------------------------------
typedef enum shader_flag_bits {
    _shader_simple_parameterization_bit = 0x0001,
    _shader_ignore_normals_bit = 0x0002,
    _shader_transparent_lit_bit = 0x0004     // 0x53fd60 returns permutation 5 for types 5..7
} shader_flag_bits;

// ---------------------------------------------------------------------------
// ShaderTransparentGenericFlags, the byte at +0x29 of shader types 5, 6 and 7 (generic,
// chicago, chicago_extended share the first 0x34 bytes of their layout).
// ---------------------------------------------------------------------------
typedef enum shader_transparent_flag_bits {
    _shader_transparent_alpha_tested_bit = 0x01,
    _shader_transparent_decal_bit = 0x02,                     // 0x53fdf9
    _shader_transparent_two_sided_bit = 0x04,
    _shader_transparent_first_map_is_in_screenspace_bit = 0x08, // 0x53fda5
    _shader_transparent_draw_before_water_bit = 0x10,         // 0x53fe45
    _shader_transparent_ignore_effect_bit = 0x20,
    _shader_transparent_scale_first_map_with_distance_bit = 0x40,
    _shader_transparent_numeric_bit = 0x80
} shader_transparent_flag_bits;

// ShaderTransparentGlassFlags (+0x28 word, read as a byte) and ShaderTransparentMeterFlags
// (+0x28 word, read as a byte): the two decal bits 0x53fde0 returns.
typedef enum shader_decal_flag_bits {
    _shader_transparent_glass_decal_bit = 0x02,   // 0x53fe01 shr al,1; and al,1
    _shader_transparent_meter_decal_bit = 0x01    // 0x53fe09 and al,1
} shader_decal_flag_bits;

// ---------------------------------------------------------------------------
// vertex shader permutation returned by 0x53fd60 (AX). The value is relative to the shader
// type, so several names share a value:
//   type 1 effect      secondary_map.tag_id != -1 ? anchor + 1 : 0   (1..3, ParticleAnchor)
//   type 4 model       translucency > 0 ? 1 : 0
//   types 5, 6, 7      first_map_type + 1 (1..4, ShaderFirstMapType), except that a 2D first
//                      map not flagged first_map_is_in_screenspace gives 0; transparent_lit in
//                      Shader.shader_flags overrides everything with 5
//   anything else, or a shader pointer of -1: 0
// ---------------------------------------------------------------------------
typedef enum shader_vertex_permutation {
    _shader_vertex_permutation_default = 0,
    _shader_vertex_permutation_effect_anchor_base = 1,     // + ParticleAnchor
    _shader_vertex_permutation_model_translucent = 1,
    _shader_vertex_permutation_first_map_base = 1,         // + ShaderFirstMapType
    _shader_vertex_permutation_transparent_lit = 5
} shader_vertex_permutation;

// ---------------------------------------------------------------------------
// shader_animation_channel  (0x10)
// One of the three periodic channels of a shader_texture_animation. 0x53fe50 reads, per
// channel c at 0x00 / 0x10 / 0x20: source (WORD, 0 means 1.0, otherwise
// render_animation.function_values[source - 1]), function (WORD, loaded into AX for
// periodic_function_evaluate 0x4cc9b0), period (0.0 is replaced by 1.0), phase (added to the
// time before the divide) and scale (multiplies the result together with the source value).
// ---------------------------------------------------------------------------
typedef struct shader_animation_channel {
    int16_t source;                 // 0x00 FunctionOut: 0 none, 1..4 = A..D out
    int16_t function;               // 0x02 WaveFunction / periodic_function (types/math.h)
    float period;                   // 0x04 seconds; 0.0 treated as 1.0
    float phase;                    // 0x08 added to the time
    float scale;                    // 0x0c
} shader_animation_channel;         // size 0x10

// ---------------------------------------------------------------------------
// shader_texture_animation  (0x38)
// The u / v / rotation animation block shared by ShaderModel (+0xfc),
// ShaderTransparentChicagoMap (+0xa4), ShaderTransparentGenericMap (+0x2c) and shader_effect
// (+0x60); the field order matches those tags.h layouts one for one. Every byte is read by
// 0x53fe50: channels at +0x00/+0x10/+0x20 as above, rotation_center at +0x30/+0x34
// (subtracted from the offsets before the rotation and added back after it).
// ---------------------------------------------------------------------------
typedef struct shader_texture_animation {
    shader_animation_channel u;          // 0x00 scroll in u (texture repeats)
    shader_animation_channel v;          // 0x10 scroll in v
    shader_animation_channel rotation;   // 0x20 degrees, added to the rotation argument
    Point2D rotation_center;             // 0x30
} shader_texture_animation;              // size 0x38

// ---------------------------------------------------------------------------
// shader_texture_transform  (0x20)
// What 0x53fe50 writes: two float[4] rows, EBX the u row and EDI the v row. Every call site
// passes them 0x10 apart in one block (0x528ebf / 0x528ec3 esp+0x4c / +0x3c, 0x53240d /
// 0x532414 esp+esi*0x20+0x80 / +0x70, 0x534339 / 0x534340), i.e. a 2x4 texture matrix handed
// to the vertex shader as two constants. With c, s the cosine and sine of the total rotation
// and du, dv the animated offsets minus rotation_center:
//   u_row = { c*u_scale, -s*v_scale, 0, c*du - s*dv + center.x }
//   v_row = { s*u_scale,  c*v_scale, 0, s*du + c*dv + center.y }
// ---------------------------------------------------------------------------
typedef struct shader_texture_transform {
    float u_row[4];                 // 0x00 EBX; [2] always 0.0
    float v_row[4];                 // 0x10 EDI; [2] always 0.0
} shader_texture_transform;         // size 0x20

// ---------------------------------------------------------------------------
// shader_effect  (0xb4)
// The particle shader block (shader_type 1). Layout from the tags.h tag definitions
// (LightningShader is this block alone; Particle +0xb0, Contrail +0x84 embed it), confirmed
// by 0x53fd60 (+0x24, +0x58 secondary_map.tag_id, +0x5c anchor) and by the 0x534335 call site
// of 0x53fe50 (texture_animation at +0x60). types/render.h notes that render_particles stores
// an average radius at +0x98 and that the sprite builder reads framebuffer_fade_mode at +0x2c.
// ---------------------------------------------------------------------------
typedef struct shader_effect {
    uint8_t unknown_00[0x24];       // 0x00 tag padding; nothing in this module reads it
    int16_t shader_type;            // 0x24 ShaderType, 1 (effect)
    int16_t unknown_26;             // 0x26 tag padding
    uint16_t flags;                 // 0x28 ParticleShaderFlags
    int16_t framebuffer_blend_function; // 0x2a FramebufferBlendFunction
    int16_t framebuffer_fade_mode;  // 0x2c FramebufferFadeMode
    uint16_t map_flags;             // 0x2e IsUnfilteredFlag
    uint8_t unknown_30[0x1c];       // 0x30 tag padding
    TagDependency secondary_map;    // 0x4c bitmap; tag_id at 0x58 (-1 means none)
    int16_t anchor;                 // 0x5c ParticleAnchor
    uint16_t secondary_map_flags;   // 0x5e IsUnfilteredFlag
    shader_texture_animation texture_animation; // 0x60
    float average_particle_radius;  // 0x98 tag padding reused at runtime: render_particles stores the
                                    //      average radius of the particles it drew (types/render.h)
    float zsprite_radius_scale;     // 0x9c
    uint8_t unknown_a0[0x14];       // 0xa0 tag padding
} shader_effect;                    // size 0xb4

// ---------------------------------------------------------------------------
// numeric countdown timer
// Driven by the script functions numeric_countdown_timer_set (long milliseconds, boolean
// start; evaluate 0x47ae10 copies them to 0x00721e50 / 0x00721e54), _get (short digit ->
// short; 0x47ae60 calls 0x5400c0), _stop (0x47aec0 clears the running byte) and _restart
// (0x47aee0 sets it); the hs definitions are at 0x00657d74..0x00657dd0. 0x540240 counts it
// down once per update from the game time (0x006f1d6c +0x0c, ticks) converted to ms, clamps at
// 0, and remembers the conversion in last_update_ms; while stopped last_update_ms is left
// untouched. The numeric transparent_chicago draws 0x531ed0 / 0x532a40 read digits through
// 0x5400c0.
// ---------------------------------------------------------------------------
typedef enum numeric_countdown_timer_digit {
    _numeric_countdown_timer_raw = -1,                 // low 16 bits of the millisecond count
    _numeric_countdown_timer_millisecond_ones = 0,     // ms % 10
    _numeric_countdown_timer_millisecond_tens = 1,     // ms / 10 % 10
    _numeric_countdown_timer_millisecond_hundreds = 2, // ms / 100 % 10
    _numeric_countdown_timer_second_ones = 3,          // ms / 1000 % 10
    _numeric_countdown_timer_second_tens = 4,          // ms / 10000 % 6
    _numeric_countdown_timer_minute_ones = 5,          // ms / 60000 % 10
    _numeric_countdown_timer_minute_tens = 6,          // ms / 600000 % 6
    _numeric_countdown_timer_hour_ones = 7,            // ms / 3600000 % 10
    _numeric_countdown_timer_hour_tens = 8             // ms / 36000000 % 10
} numeric_countdown_timer_digit;

// global 0x00721e50: int32_t numeric_countdown_timer_remaining_ms  0x5400c0, 0x540240, set by 0x47ae41
// global 0x00721e54: uint8_t numeric_countdown_timer_running       0x540240; 0x47ae49 / 0x47aec4 / 0x47aee4
// global 0x00721e55: uint8_t numeric_countdown_timer_unknown_55[3] alignment, never referenced
// global 0x00721e58: int32_t numeric_countdown_timer_last_update_ms 0x540240 only (game ticks * 1000 / 30)
// Not owned here: 0x00721e4c (before, read by 0x508060 / 0x53e7e8 / 0x56f23f) and 0x00721e5c
// onwards (shell module, 0x540632..).

#pragma pack(pop)
