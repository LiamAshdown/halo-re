// rasterizer_transparent_geometry_group_draw  (Ghidra: rasterizer_transparent_geometry_group_draw, already named)
// address 0x533850, size 5552 bytes
// name confidence: 0.7   rewrite confidence: 0.7
// evidence: rebuilt from the raw disassembly (objdump 0x533850..0x534e00); Ghidra's C is unusable
//   for most call sites (every IDirect3DDevice9 call loses its arguments, the meter constants are
//   scrambled and the glass reflection kind rewrite at 0x5345db is missing). Jump tables read from
//   the image: shader_type switch at 0x534e00 (types 1..11), blend function switch at 0x534e2c.
//   Field meanings come from types/tags.h: shader_type 9 reads ShaderTransparentGlass
//   (+0x28 flags, +0x54 background_tint_color, +0x70 background_tint_map.tag_id, +0x8a
//   reflection_type, +0x8c/+0x9c perpendicular/parallel brightness, +0xb8 reflection_map.tag_id,
//   +0xcc bump_map.tag_id, +0x164 diffuse_map.tag_id, +0x178 diffuse_detail_map.tag_id) and
//   shader_type 10 reads ShaderTransparentMeter (+0x28 flags, +0x58 map.tag_id, +0x7c..+0xb4
//   colors, +0xb8/+0xbc transparencies, +0xd8..+0xde function sources).
// Role: draws one queued transparent_geometry_group. It marks the group drawn, draws the group
//   linked before it, runs the depth-only pre-pass for a run of batched (mode 2) groups with the
//   same sort key, the stencil-free z pre-pass for mode 1 model groups, optionally captures the
//   frame for active camouflage, then dispatches on Shader.shader_type (particle effect block,
//   model, chicago, chicago extended, water, glass, meter, plasma) for one or two passes, and
//   finally draws the group linked after it and, for the last mode 1 model group of a key, the
//   secondary groups attached to that key.
// register convention: plain cdecl, both arguments on the stack.
// blam-cc: stack -> (group, attached)
// UNSURE: the particle effect block read for shader_type 1 (+0x28 flags, +0x2a
//   framebuffer_blend_function, +0x2e map flags, +0x58/+0x5c secondary map and anchor, +0x60
//   texture animation) is the shader block that particle and contrail tags embed (see
//   Contrail.shader_type in types/tags.h); the exact tag struct is not typed here.
// UNSURE: meter vertex shader variant is an uninitialised stack slot in the binary when the
//   vertex type is not 0, 2 or 4; it is zero here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t: pointer fields are held as uint32_t

extern void *rasterizer_device;                                              // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;                       // 0x007c1220
extern rasterizer_frame_time rasterizer_time;                                // 0x007c1200
extern d3d_caps9 rasterizer_caps;                                            // 0x007c10c0
extern transparent_geometry_group *transparent_geometry_groups;             // 0x0071d14c
extern transparent_geometry_group *transparent_geometry_groups_secondary;   // 0x0071d150
extern int32_t transparent_geometry_group_count;                            // 0x0071d154
extern int32_t transparent_geometry_group_secondary_count;                  // 0x0071d158
extern int16_t *transparent_geometry_group_sorted_indices;                  // 0x0071d15c
extern uint32_t transparent_geometry_group_drawn_bits[12];                  // 0x006d983c
extern int32_t transparent_geometry_group_last_drawn_key;                   // 0x006e1d58
extern uint8_t rasterizer_secondary_groups_drawn;                           // 0x0071d274 UNSURE name: set
                                                                             //   after a secondary group
                                                                             //   draws while 0x689412 is set
extern real_matrix4x3 *k_render_identity_matrix_ptr;                        // 0x0069673c
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern uint32_t rasterizer_frustum_z_values[2];                             // 0x0069c664
extern uint32_t rasterizer_depth_prepass_vertex_shader;                     // 0x0069e460 rasterizer_vertex_shaders[34].shader
extern void *rasterizer_glass_draw_procedures[3];                           // 0x007c0480 diffuse, tint, reflection
extern void *rasterizer_water_draw_procedure;                               // 0x007bf050
extern uint8_t console_debug_toggle_689422;                                 // 0x00689422
extern uint8_t console_debug_toggle_6893eb;                                 // 0x006893eb meter debug animation
extern int16_t debug_print_enabled_flag;                                 // 0x00689412 read as a word here
extern float console_debug_meter_period;                                    // 0x00689454
extern float console_debug_meter_values[4];                                 // 0x00689458 negative keeps the animated value

// blam-cc: EAX -> group
extern uint8_t transparent_geometry_group_test_drawn_bit(transparent_geometry_group *group); // 0x515310
// blam-cc: EAX -> group
extern transparent_geometry_group *transparent_geometry_group_get_next_sorted(transparent_geometry_group *group); // 0x515290
// blam-cc: stack -> upload, EDI -> nodes
extern void chimera__rasterizer_set_model_skinning(uint8_t upload, rasterizer_node_matrices *nodes); // 0x518b40
// blam-cc: EAX -> node_part_count, ESI -> node_part_indices
extern void chimera__rasterizer_set_up_node_parts(int32_t node_part_count, uint8_t *node_part_indices); // 0x526cf0
extern void rasterizer_prepare_lighting_constants(render_lighting *lighting); // 0x518ce0
// blam-cc: ECX -> group, stack -> flag
extern void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag); // 0x533660
extern void rasterizer_geometry_part_draw(transparent_geometry_group *group); // 0x533730
extern uint8_t rasterizer_transparent_decals_enabled(void); // 0x519ac0
extern void rasterizer_render_target_capture_frame(void); // 0x519b00
// blam-cc: ECX -> shader
extern uint8_t shader_is_decal(const Shader *shader); // 0x0053fde0
// blam-cc: ECX -> shader
extern uint8_t shader_draw_before_water(const void *shader); // 0x53fe30 shaders module: flags bit 12 of shader types 5..7
extern void chimera__rasterizer_set_frustum_z_func(uint32_t z_near, uint32_t z_far); // 0x518f40
extern void chimera__transparent_decal_zbias(void); // 0x519530
extern void rasterizer_clear_decal_zbias(void); // 0x519580
// blam-cc: AX -> mode
extern void rasterizer_set_shader_stage_config(int16_t mode); // 0x519200
extern void rasterizer_transparent_geometry_group_draw_active_camouflage(transparent_geometry_group *group); // 0x519f70
// blam-cc: ESI -> bitmap, stack -> stage
extern uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap); // 0x518680
// blam-cc: CX -> mode
extern void chimera__rasterizer_set_framebuffer_blend_function(int16_t mode); // 0x5185d0
// blam-cc: ECX -> function_source, ESI -> animation, EBX -> out_u, EDI -> out_v, stack -> the rest
extern void shader_texture_animation_evaluate(const void *function_source, const void *animation,
                                              float *out_u, float *out_v, float u_scale, float v_scale,
                                              float unused_z, float unused_w, float unused_5,
                                              float time); // 0x53fe50
extern void rasterizer_shader_transparent_chicago_draw(transparent_geometry_group *group, uint8_t attached); // 0x531ed0
extern void rasterizer_shader_transparent_chicago_extended_draw(transparent_geometry_group *group, uint8_t attached); // 0x532a40
// blam-cc: EBX -> group
extern void rasterizer_shader_transparent_plasma_draw(transparent_geometry_group *group); // 0x52c4a0
// blam-cc: EAX -> bitmap_tag_id, CX -> bitmap_type, stack -> (stage, default_index, frame, effect_slot)
extern int16_t *rasterizer_resolve_and_cache_submap_b(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage,
                                                      int16_t default_index, int16_t frame,
                                                      rasterizer_effect_slot *effect_slot); // 0x518860
// blam-cc: AX -> type, stack -> time
extern real periodic_function_evaluate(periodic_function_t type, double time); // 0x4cc9b0

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);
typedef void (*transparent_geometry_callback)(int32_t argument, int32_t count);
typedef void (*transparent_geometry_draw_procedure)(transparent_geometry_group *group);
typedef void (*transparent_geometry_draw_procedure2)(transparent_geometry_group *group, int16_t kind);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}
static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, sampler, type, value);
}
static void set_vertex_declaration(uint32_t declaration)
{
    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, declaration);
}
static void set_vertex_shader(uint32_t shader)
{
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, shader);
}

// ID3DXEffect Begin (+0x100) / Pass (+0x104) / End (+0x108) around one draw per pass.
static void draw_effect_passes(void *effect, transparent_geometry_group *group)
{
    uint32_t passes;
    uint32_t pass;
    void **vt = *(void ***)effect;

    ((d3dx_effect_begin_fn)vt[0x100 / 4])(effect, &passes, 3);
    for (pass = 0; pass < passes; pass++) {
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    }
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
}

// Depth-only states shared by the two pre-passes below (colour writes off, z on and written,
// pixel shader NULL, stage 0 selecting TFACTOR for colour and alpha).
static void set_depth_prepass_states(uint32_t cull_mode, uint32_t texture_factor)
{
    set_render_state(0x16, cull_mode);  // D3DRS_CULLMODE
    set_render_state(0xa8, 0);          // D3DRS_COLORWRITEENABLE
    set_render_state(0x1b, 0);          // D3DRS_ALPHABLENDENABLE
    set_render_state(0x0f, 0);          // D3DRS_ALPHATESTENABLE
    set_render_state(0x07, 1);          // D3DRS_ZENABLE
    set_render_state(0x0e, 1);          // D3DRS_ZWRITEENABLE
    set_render_state(0x17, 4);          // D3DRS_ZFUNC LESSEQUAL
    set_render_state(0x3c, texture_factor); // D3DRS_TEXTUREFACTOR
    ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0); // SetPixelShader(NULL)
    set_texture_stage_state(0, 1, 2);   // COLOROP SELECTARG1
    set_texture_stage_state(0, 2, 3);   // COLORARG1 TFACTOR
    set_texture_stage_state(0, 4, 2);   // ALPHAOP SELECTARG1
    set_texture_stage_state(0, 5, 3);   // ALPHAARG1 TFACTOR
    set_texture_stage_state(1, 1, 1);   // stage 1 COLOROP DISABLE
    set_texture_stage_state(1, 4, 1);   // stage 1 ALPHAOP DISABLE
}

// node_matrices / node_count of a group, or the shared identity matrix with a count of 1.
static void set_group_skinning(const transparent_geometry_group *flags_group,
                               const transparent_geometry_group *source)
{
    rasterizer_node_matrices nodes;

    if (source->node_matrices != 0 && source->node_count != 0) {
        nodes.matrices = source->node_matrices;
        nodes.node_count = source->node_count;
    } else {
        nodes.matrices = (uint32_t)(uintptr_t)k_render_identity_matrix_ptr;
        nodes.node_count = 1;
    }
    chimera__rasterizer_set_model_skinning((uint8_t)(~(uint8_t)(flags_group->flags >> 8) & 1), &nodes);
}

static int16_t shader_type_of(const uint8_t *shader)
{
    return *(int16_t *)&((struct Shader *)shader)->shader_type;
}

// shader_type 1: the particle effect block. Picks one of the effects 0x5a..0x65 from the blend
// function and draws through its ID3DXEffect with the view and texture animation matrices.
static void draw_particle_effect_shader(transparent_geometry_group *group, const uint8_t *shader)
{
    uint8_t has_texture_animation;
    int16_t effect_index;
    rasterizer_effect_slot *effect;
    BitmapData *bitmap;
    float view_matrix[3][4];
    float texture_matrix[4][4];
    int i, j;

    has_texture_animation = 0;
    if (*(const int32_t *)(shader + 0x58) != -1 && *(const int16_t *)(shader + 0x5c) != 2) {
        has_texture_animation = 1;
    }
    effect_index = (shader[0x28] & 2) ? 0x5a : 0x60;
    if ((group->flags & 4) == 0) {
        switch (*(const int16_t *)(shader + 0x2a)) {
        case 0: effect_index += 2; break;
        case 1: case 5: effect_index += 4; break;
        case 2: effect_index += 3; break;
        case 3: case 4: case 6: effect_index += 1; break;
        case 7: effect_index += 5; break;
        default: break;
        }
    }
    effect = &rasterizer_effects[effect_index];
    if (effect->effect == 0) {
        return;
    }

    bitmap = (BitmapData *)(uintptr_t)group->lightmap_bitmap;
    if (bitmap != NULL && ((struct BitmapData *)bitmap)->hardware_texture != 0) {  // hardware texture
        uint8_t map_flags = shader[0x2e];
        uint32_t filter = (map_flags & 1) ? 1 : 2;                        // POINT when unfiltered

        rasterizer_bind_texture_d3d9(0, bitmap);
        set_sampler_state(0, 1, (map_flags & 2) | 1);                   // ADDRESSU WRAP or CLAMP
        set_sampler_state(0, 2, ((map_flags & 4) | 2) >> 1);            // ADDRESSV WRAP or CLAMP
        set_sampler_state(0, 5, filter);
        set_sampler_state(0, 6, filter);
        set_sampler_state(0, 7, filter);
    }
    set_render_state(0x16, 1);    // CULLMODE NONE
    set_render_state(0xa8, 7);    // COLORWRITEENABLE rgb
    set_render_state(0x1b, 1);    // ALPHABLENDENABLE
    set_render_state(0x0f, 0);    // ALPHATESTENABLE
    set_render_state(0x1c, 0);    // FOGENABLE
    chimera__rasterizer_set_framebuffer_blend_function(*(const int16_t *)(shader + 0x2a));

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            view_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            texture_matrix[i][j] = (i == j && i < 2) ? 1.0f : 0.0f;
        }
    }
    if (group->flags & 0x20) {
        // camera facing: the transposed view_to_world rotation with the camera position
        const real_matrix4x3 *view_to_world = &rasterizer_window.frustum.view_to_world;
        const real_point3d *camera = &rasterizer_window.camera.position;

        view_matrix[0][0] = view_to_world->forward.i;
        view_matrix[0][1] = view_to_world->left.i;
        view_matrix[0][2] = view_to_world->up.i;
        view_matrix[0][3] = camera->x;
        view_matrix[1][0] = view_to_world->forward.j;
        view_matrix[1][1] = view_to_world->left.j;
        view_matrix[1][2] = view_to_world->up.j;
        view_matrix[1][3] = camera->y;
        view_matrix[2][0] = view_to_world->forward.k;
        view_matrix[2][1] = view_to_world->left.k;
        view_matrix[2][2] = view_to_world->up.k;
        view_matrix[2][3] = camera->z;
    }
    if (has_texture_animation) {
        shader_texture_animation_evaluate((const void *)(uintptr_t)group->lighting_extra, shader + 0x60,
                                          texture_matrix[2], texture_matrix[3],
                                          group->base_map_u_scale, group->base_map_v_scale, 0.0f, 0.0f, 0.0f,
                                          (float)rasterizer_time.time);
    }
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0x1a, &view_matrix[0][0], 3);
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0x0d, &texture_matrix[0][0], 4);
    set_vertex_declaration(rasterizer_vertex_declarations[6].declaration);
    set_vertex_shader(rasterizer_vertex_shaders[effect->vertex_shader_index].shader);
    draw_effect_passes((void *)(uintptr_t)effect->effect, group);
}

// shader_type 9: ShaderTransparentGlass, through the three procedures of 0x007c0480.
static void draw_glass_shader(transparent_geometry_group *group, const uint8_t *shader)
{
    int16_t reflection_type = *(const int16_t *)(shader + 0x8a);

    set_render_state(0x16, (shader[0x28] & 4) ? 1 : 3); // CULLMODE: two_sided -> NONE, else CCW
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 1);
    set_render_state(0xab, 1);    // BLENDOP ADD
    set_render_state(0x18, 0);    // ALPHAREF
    set_render_state(0x1c, 0);
    set_sampler_state(0, 1, 1);
    set_sampler_state(0, 2, 1);
    set_sampler_state(0, 5, 2);
    set_sampler_state(0, 6, 2);
    set_sampler_state(0, 7, 2);

    if (reflection_type == 2 && (rasterizer_window.unknown_04 == 0 || rasterizer_window.type != 1)) {
        // dynamic mirror outside the mirror pass: only the fixed function path draws it
        if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
            ((transparent_geometry_draw_procedure2)rasterizer_glass_draw_procedures[2])(group, 2);
        }
        return;
    }
    if (*(const int32_t *)(shader + 0x70) != -1 ||
        *(const float *)(shader + 0x54) != 0.0f ||
        *(const float *)(shader + 0x58) != 0.0f ||
        *(const float *)(shader + 0x5c) != 0.0f) {
        ((transparent_geometry_draw_procedure)rasterizer_glass_draw_procedures[1])(group);
    }
    if ((*(const float *)(shader + 0x8c) > 0.0f || *(const float *)(shader + 0x9c) > 0.0f) &&
        (*(const int32_t *)(shader + 0xb8) != -1 || reflection_type == 2)) {
        if (reflection_type == 0 &&
            ((shader[0x28] & 8) != 0 || *(const int32_t *)(shader + 0xcc) == -1)) {
            reflection_type = 1;  // bumped without a usable bump map draws as a flat cube map
        }
        ((transparent_geometry_draw_procedure2)rasterizer_glass_draw_procedures[2])(group, reflection_type);
    }
    if (*(const int32_t *)(shader + 0x164) != -1 || *(const int32_t *)(shader + 0x178) != -1) {
        ((transparent_geometry_draw_procedure)rasterizer_glass_draw_procedures[0])(group);
    }
}

// shader_type 10: ShaderTransparentMeter, pixel shader path only (effect 111, vertex shaders
// 57/58).
static void draw_meter_shader(transparent_geometry_group *group, const uint8_t *shader, int16_t vertex_type)
{
    int16_t variant = 0;
    float meter_brightness, flash_brightness, value, gradient;
    float flash[3];
    float scaled_gradient;
    float pixel_constants[6][4];
    float vertex_constants[3][4];
    void *effect;
    int i;

    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        return;
    }
    if (vertex_type == 0 || vertex_type == 2) {
        variant = 0;
    } else if (vertex_type == 4) {
        variant = 1;
    }
    if (rasterizer_effects[111].effect == 0) {
        return;
    }
    meter_brightness = 1.0f;
    flash_brightness = 1.0f;
    value = 1.0f;
    gradient = 1.0f;
    set_vertex_declaration(rasterizer_vertex_declarations[vertex_type].declaration);
    set_vertex_shader(rasterizer_vertex_shaders[57 + variant].shader);

    if (group->lighting_extra != 0) {
        const float *function_values = *(const float **)(uintptr_t)(group->lighting_extra + 4);

        if (function_values != NULL) {
            int16_t source;

            source = *(const int16_t *)(shader + 0xd8);
            if (source >= 1 && source <= 4) meter_brightness = function_values[source - 1];
            source = *(const int16_t *)(shader + 0xda);
            if (source >= 1 && source <= 4) flash_brightness = function_values[source - 1];
            source = *(const int16_t *)(shader + 0xdc);
            if (source >= 1 && source <= 4) value = function_values[source - 1];
            source = *(const int16_t *)(shader + 0xde);
            if (source >= 1 && source <= 4) gradient = function_values[source - 1];
        }
    }
    if (console_debug_toggle_6893eb) {
        float animated = (float)periodic_function_evaluate(2, rasterizer_time.time / console_debug_meter_period);

        meter_brightness = (console_debug_meter_values[0] < 0.0f) ? animated : console_debug_meter_values[0];
        flash_brightness = (console_debug_meter_values[1] < 0.0f) ? animated : console_debug_meter_values[1];
        value = (console_debug_meter_values[2] < 0.0f) ? animated : console_debug_meter_values[2];
        gradient = (console_debug_meter_values[3] < 0.0f) ? animated : console_debug_meter_values[3];
    }

    flash[0] = flash_brightness * *(const float *)(shader + 0xa0);
    flash[1] = flash_brightness * *(const float *)(shader + 0xa4);
    flash[2] = flash_brightness * *(const float *)(shader + 0xa8);
    scaled_gradient = gradient * 8.0f;
    if (!(scaled_gradient > 1.0f)) {
        scaled_gradient = 1.0f;
    }

    pixel_constants[0][0] = flash[0];
    pixel_constants[0][1] = flash[1];
    pixel_constants[0][2] = flash[2];
    pixel_constants[0][3] = 1.0f;
    pixel_constants[1][0] = *(const float *)(shader + 0x88);     // gradient_max_color
    pixel_constants[1][1] = *(const float *)(shader + 0x8c);
    pixel_constants[1][2] = *(const float *)(shader + 0x90);
    pixel_constants[1][3] = 1.0f / scaled_gradient;
    pixel_constants[2][0] = *(const float *)(shader + 0x7c);     // gradient_min_color
    pixel_constants[2][1] = *(const float *)(shader + 0x80);
    pixel_constants[2][2] = *(const float *)(shader + 0x84);
    pixel_constants[2][3] = value;
    pixel_constants[3][0] = *(const float *)(shader + 0x94);     // background_color
    pixel_constants[3][1] = *(const float *)(shader + 0x98);
    pixel_constants[3][2] = *(const float *)(shader + 0x9c);
    pixel_constants[3][3] = 1.0f;
    pixel_constants[4][0] = *(const float *)(shader + 0xac);     // meter_tint_color
    pixel_constants[4][1] = *(const float *)(shader + 0xb0);
    pixel_constants[4][2] = *(const float *)(shader + 0xb4);
    pixel_constants[4][3] = 1.0f;
    if (shader[0x28] & 4) {                                      // flash_color_is_negative
        pixel_constants[5][0] = -flash[0];
        pixel_constants[5][1] = -flash[1];
        pixel_constants[5][2] = -flash[2];
        pixel_constants[5][3] = -1.0f;
    } else {
        pixel_constants[5][0] = flash[0];
        pixel_constants[5][1] = flash[1];
        pixel_constants[5][2] = flash[2];
        pixel_constants[5][3] = 1.0f;
    }
    if (shader[0x28] & 8) {                                      // tint_mode_2
        pixel_constants[0][3] = *(const float *)(shader + 0xb8); // meter_transparency
        pixel_constants[3][3] = *(const float *)(shader + 0xbc); // background_transparency
        pixel_constants[4][0] *= meter_brightness;
        pixel_constants[4][1] *= meter_brightness;
        pixel_constants[4][2] *= meter_brightness;
        pixel_constants[4][3] = *(const float *)(shader + 0xb8);
    } else {
        pixel_constants[0][3] = meter_brightness;
        pixel_constants[3][3] = 0.0f;
        pixel_constants[4][3] = meter_brightness;
    }

    rasterizer_resolve_and_cache_submap_b(*(const uint32_t *)(shader + 0x58), 0, 0, 1,
                                          (int16_t)group->shader_permutation, &rasterizer_effects[111]);
    set_sampler_state(0, 1, 1);
    set_sampler_state(0, 2, 1);
    set_sampler_state(0, 5, (shader[0x28] & 0x10) ? 1 : 2);     // unfiltered
    set_sampler_state(0, 6, (shader[0x28] & 0x10) ? 1 : 2);
    set_sampler_state(0, 7, (shader[0x28] & 0x10) ? 1 : 2);
    set_render_state(0x16, (*(const uint16_t *)(shader + 0x28) & 2) ? 1 : 3); // two_sided
    set_render_state(0xa8, 7);
    set_render_state(0x1b, 1);
    set_render_state(0x13, 2);    // SRCBLEND ONE
    set_render_state(0x14, 2);    // DESTBLEND ONE
    set_render_state(0xab, 1);    // BLENDOP ADD
    set_render_state(0x0f, 0);
    set_render_state(0x1c, 0);

    for (i = 0; i < 4; i++) {
        vertex_constants[0][i] = 1.0f;
    }
    vertex_constants[1][0] = group->base_map_u_scale;
    vertex_constants[1][1] = 0.0f;
    vertex_constants[1][2] = 0.0f;
    vertex_constants[1][3] = 0.0f;
    vertex_constants[2][0] = 0.0f;
    vertex_constants[2][1] = group->base_map_v_scale;
    vertex_constants[2][2] = 0.0f;
    vertex_constants[2][3] = 0.0f;
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 10, &vertex_constants[0][0], 3);
    if (console_debug_toggle_6893eb && debug_print_enabled_flag != 0) {
        set_render_state(0x1b, 0);
    }
    ((d3d_set_constant_f_fn)device_vtable()[0x1b4 / 4])(rasterizer_device, 0, &pixel_constants[0][0], 6);
    effect = (void *)(uintptr_t)rasterizer_effects[111].effect;
    draw_effect_passes(effect, group);
}

void rasterizer_transparent_geometry_group_draw(transparent_geometry_group *group, uint8_t attached)
{
    transparent_geometry_group *pool;
    transparent_geometry_group *cursor;
    uint8_t *shader;
    uint8_t draw_secondary_groups = 0;
    uint32_t immediate;
    int16_t vertex_type;
    int16_t pass;
    int32_t key;

    if (group->parent_sort_key != 0 && !attached) {
        return;
    }
    if (!transparent_geometry_group_test_drawn_bit(group)) {
        return;
    }
    pool = transparent_geometry_groups;
    if (group >= pool && group < pool + transparent_geometry_group_count) {
        int16_t index = (int16_t)(group - pool);

        if (index != -1) {
            transparent_geometry_group_drawn_bits[index >> 5] |= 1u << (index & 0x1f);
        }
    }
    if (group->previous_group_index != -1) {
        rasterizer_transparent_geometry_group_draw(&pool[group->previous_group_index], attached);
    }

    // mode 2: one depth-only pre-pass over the whole run of groups sharing this sort key
    if (group->parameters.mode == 2) {
        key = group->sort_key;
        if (key != transparent_geometry_group_last_drawn_key && !attached) {
            set_depth_prepass_states(1, 0);
            set_vertex_declaration(rasterizer_vertex_declarations[4].declaration);
            set_vertex_shader(rasterizer_depth_prepass_vertex_shader);
            cursor = group;
            do {
                int16_t next;

                if (cursor->sort_key != key || cursor->parameters.mode != 2) {
                    break;
                }
                shader = (uint8_t *)(uintptr_t)cursor->shader;
                if (shader == NULL ||
                    !((shader_type_of(shader) == 5 || shader_type_of(shader) == 6 || shader_type_of(shader) == 7) &&
                      ((shader[0x29] >> 5) & 1) != 0)) {
                    // the flags tested are the head group's, the matrices and lighting the cursor's
                    set_group_skinning(group, cursor);
                    if (group->flags & 0x100) {
                        chimera__rasterizer_set_up_node_parts(cursor->node_part_count,
                                                              (uint8_t *)(uintptr_t)cursor->node_part_indices);
                    }
                    if (group->lighting != 0) {
                        rasterizer_prepare_lighting_constants((render_lighting *)(uintptr_t)cursor->lighting);
                    }
                    rasterizer_transparent_geometry_group_draw_vertices(cursor, 0);
                }
                next = (int16_t)((int16_t)cursor->sorted_index + 1);
                if (next < transparent_geometry_group_count) {
                    cursor = &transparent_geometry_groups[transparent_geometry_group_sorted_indices[next]];
                } else {
                    cursor = NULL;
                }
            } while (cursor != NULL);
        }
    }

    if (rasterizer_transparent_decals_enabled()) {
        // mode 1 model groups: z pre-pass through the geometry part drawer
        if ((group->flags & 2) == 0 && group->parameters.mode == 1) {
            key = group->sort_key;
            shader = (uint8_t *)(uintptr_t)group->shader;
            if (key != transparent_geometry_group_last_drawn_key && shader != NULL &&
                shader_type_of(shader) == 4 && !attached) {
                set_depth_prepass_states(3, 0xffffffff);
                cursor = group;
                do {
                    if (cursor->sort_key != key || cursor->parameters.mode != 1) {
                        break;
                    }
                    if (cursor->shader != 0) {
                        rasterizer_geometry_part_draw(cursor);
                    }
                    cursor = transparent_geometry_group_get_next_sorted(cursor);
                } while (cursor != NULL);
                set_render_state(0xa8, 7);
            }
        }
    }

    // frame capture for active camouflage
    if ((group->flags & 2) == 0 && rasterizer_window.type == 1 && !attached) {
        shader = (uint8_t *)(uintptr_t)group->shader;
        if (console_debug_toggle_689422) {
            if (shader != NULL && shader_type_of(shader) == 4 && group->parameters.mode == 1 &&
                group->sort_key != transparent_geometry_group_last_drawn_key) {
                rasterizer_render_target_capture_frame();
            }
        } else if (shader == NULL || (shader_type_of(shader) != 8 && !shader_draw_before_water(shader))) {
            rasterizer_render_target_capture_frame();
        }
    }

    // the last mode 1 model group of a key also draws the secondary groups attached to the key
    immediate = group->flags & 2;
    if (immediate == 0 && rasterizer_window.type == 1 && group->parameters.mode == 1) {
        shader = (uint8_t *)(uintptr_t)group->shader;
        if (shader != NULL && shader_type_of(shader) == 4 && !attached) {
            transparent_geometry_group *next = transparent_geometry_group_get_next_sorted(group);

            if (next == NULL || next->parameters.mode != 1 || next->sort_key != group->sort_key ||
                next->shader == 0 || shader_type_of((uint8_t *)(uintptr_t)next->shader) != 4) {
                draw_secondary_groups = 1;
            }
        }
    }

    if (group->shader == 0) {
        ((transparent_geometry_callback)(uintptr_t)group->index_buffer)(group->first_index, group->primitive_count);
        goto finish;
    }

    vertex_type = -1;
    if (group->vertex_buffer != 0) {
        vertex_type = *(int16_t *)(uintptr_t)group->vertex_buffer;  // rasterizer_vertex_buffer.type
    } else if (group->dynamic_vertex_slot != -1) {
        vertex_type = rasterizer_dynamic_vertex_slots[group->dynamic_vertex_slot].vertex_type;
    }
    if (immediate == 0) {
        set_group_skinning(group, group);
        if (group->flags & 0x100) {
            chimera__rasterizer_set_up_node_parts(group->node_part_count,
                                                  (uint8_t *)(uintptr_t)group->node_part_indices);
        }
        if (group->lighting != 0) {
            rasterizer_prepare_lighting_constants((render_lighting *)(uintptr_t)group->lighting);
        }
    }
    if (group->flags & 8) {
        if (rasterizer_window.type == 1) {
            chimera__rasterizer_set_frustum_z_func(0x3b800000, 0x45800000); // 1/256, 4096
        }
        set_render_state(0x07, 0);    // ZENABLE off
    } else {
        set_render_state(0x07, 1);
        set_render_state(0x0e, 0);    // ZWRITEENABLE off
        set_render_state(0x17, 4);    // ZFUNC LESSEQUAL
        if (shader_is_decal((void *)(uintptr_t)group->shader)) {
            chimera__transparent_decal_zbias();
        } else {
            rasterizer_clear_decal_zbias();
        }
    }

    for (pass = 0; pass < 2; pass++) {
        if ((int8_t)group->flags < 0) {
            // first person groups: mode 1 once with the first person z range, the rest in two
            // passes (stage config 3, then 2 with z off)
            if (group->parameters.mode == 1) {
                if (pass > 0) {
                    break;
                }
                chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
            } else if (pass == 0) {
                shader = (uint8_t *)(uintptr_t)group->shader;
                if (shader != NULL && shader_type_of(shader) == 1 && (shader[0x28] & 4) != 0) {
                    continue;
                }
                rasterizer_set_shader_stage_config(3);
            } else {
                rasterizer_set_shader_stage_config(2);
                set_render_state(0x07, 0);
            }
        } else if (pass > 0) {
            break;
        }

        shader = (uint8_t *)(uintptr_t)group->shader;
        switch (shader_type_of(shader)) {
        case 1:
            draw_particle_effect_shader(group, shader);
            break;
        case 4:
            if (group->parameters.mode == 1) {
                if (rasterizer_secondary_groups_drawn) {
                    return;  // leaves without the state restores below, as the binary does
                }
                rasterizer_transparent_geometry_group_draw_active_camouflage(group);
            }
            break;
        case 6:
            rasterizer_shader_transparent_chicago_draw(group, attached);
            break;
        case 7:
            rasterizer_shader_transparent_chicago_extended_draw(group, attached);
            break;
        case 8:
            ((transparent_geometry_draw_procedure)rasterizer_water_draw_procedure)(group);
            break;
        case 9:
            draw_glass_shader(group, shader);
            break;
        case 10:
            draw_meter_shader(group, shader, vertex_type);
            break;
        case 11:
            rasterizer_shader_transparent_plasma_draw(group);
            break;
        default:
            break;
        }
    }

    if ((group->flags & 8) && rasterizer_window.type == 1) {
        chimera__rasterizer_set_frustum_z_func(0, 0);
    }
    if ((int8_t)group->flags < 0 && group->parameters.mode == 1) {
        chimera__rasterizer_set_frustum_z_func(0, 0);
    }
    if (rasterizer_caps.raster_caps & 0x04000000) {
        set_render_state(0xc3, 0);    // DEPTHBIAS
    }
    if (rasterizer_caps.raster_caps & 0x02000000) {
        set_render_state(0xaf, 0);    // SLOPESCALEDEPTHBIAS
    }

finish:
    if (!attached) {
        transparent_geometry_group_last_drawn_key = group->sort_key;
    }
    if (group->next_group_index != -1) {
        rasterizer_transparent_geometry_group_draw(&transparent_geometry_groups[group->next_group_index], attached);
    }
    if (draw_secondary_groups && (int16_t)transparent_geometry_group_secondary_count > 0) {
        uint16_t remaining = (uint16_t)transparent_geometry_group_secondary_count;
        transparent_geometry_group *secondary = transparent_geometry_groups_secondary;

        do {
            if (secondary->parent_sort_key == group->sort_key && secondary->parameters.mode == 1) {
                rasterizer_transparent_geometry_group_draw(secondary, 1);
                if (debug_print_enabled_flag != 0) {
                    rasterizer_secondary_groups_drawn = 1;
                }
            }
            secondary++;
        } while (--remaining != 0);
    }
}

#if 0
Original Ghidra decompilation (0x533850): (most device calls lost their arguments; the rewrite follows the disassembly)

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void rasterizer_transparent_geometry_group_draw(uint *param_1,char param_2)

{
  float fVar1;
  float fVar2;
  char cVar3;
  short sVar4;
  ushort uVar5;
  uint *puVar6;
  int iVar7;
  uint uVar8;
  float unaff_EBX;
  uint uVar9;
  short *psVar10;
  undefined4 unaff_EDI;
  uint *puVar11;
  float10 fVar12;
  undefined8 uVar13;
  int *piVar14;
  uint *puStack_208;
  uint *puStack_204;
  uint uStack_200;
  undefined4 uStack_1fc;
  uint *puStack_1f8;
  undefined4 uStack_1f4;
  undefined4 uStack_1f0;
  uint *puStack_1ec;
  uint *puStack_1e8;
  undefined4 uStack_1e4;
  uint *puStack_1e0;
  undefined4 uStack_1dc;
  uint *puStack_1d8;
  uint *puStack_1d4;
  int *piStack_1d0;
  uint *puStack_1a8;
  uint *puStack_1a4;
  uint *puStack_1a0;
  uint *puStack_19c;
  uint *puStack_198;
  float *pfStack_194;
  uint *puStack_190;
  uint *puStack_18c;
  undefined4 uStack_188;
  int iStack_184;
  uint *puStack_180;
  undefined4 uStack_17c;
  uint *puStack_178;
  uint *puStack_174;
  uint *puStack_170;
  undefined4 uStack_16c;
  undefined8 uStack_168;
  uint *puStack_160;
  uint *puStack_15c;
  uint *puStack_158;
  float fStack_144;
  undefined4 uStack_140;
  uint *local_13c;
  uint local_138;
  undefined4 uStack_134;
  undefined *puStack_130;
  float fStack_12c;
  float fStack_128;
  int iStack_124;
  float fStack_120;
  undefined4 uStack_11c;
  undefined4 uStack_118;
  undefined4 uStack_114;
  undefined4 uStack_110;
  undefined4 uStack_10c;
  undefined4 uStack_f8;
  undefined4 uStack_f4;
  undefined4 uStack_f0;
  undefined4 uStack_ec;
  undefined *puStack_e8;
  float fStack_e4;
  float fStack_e0;
  float fStack_dc;
  undefined4 uStack_d8;
  uint uStack_d4;
  undefined4 uStack_d0;
  float fStack_cc;
  undefined4 uStack_c8;
  uint uStack_c4;
  undefined4 uStack_c0;
  float fStack_bc;
  undefined4 uStack_b8;
  undefined4 uStack_b4;
  undefined4 uStack_b0;
  undefined4 uStack_ac;
  float fStack_a8;
  float fStack_a4;
  float fStack_a0;
  float fStack_9c;
  undefined *puStack_98;
  float fStack_94;
  float fStack_90;
  undefined4 uStack_8c;
  undefined4 uStack_88;
  undefined4 uStack_84;
  undefined4 uStack_80;
  undefined4 uStack_7c;
  undefined4 uStack_78;
  
  uStack_140._0_3_ = (uint3)(ushort)uStack_140;
  if ((param_1[0x28] != 0) && (param_2 == '\0')) {
    return;
  }
  cVar3 = FUN_00515310();
  puVar6 = DAT_0071d14c;
  if (cVar3 == '\0') {
    return;
  }
  if ((DAT_0071d14c <= param_1) && (param_1 < DAT_0071d14c + DAT_0071d154 * 0x2a)) {
    iVar7 = (int)param_1 - (int)DAT_0071d14c;
    sVar4 = ((short)(iVar7 / 0xa8) + (short)(iVar7 >> 0x1f)) -
            (short)((longlong)iVar7 * 0x30c30c31 >> 0x3f);
    if (sVar4 != -1) {
      (&DAT_006d983c)[(int)sVar4 >> 5] =
           (&DAT_006d983c)[(int)sVar4 >> 5] | 1 << ((byte)sVar4 & 0x1f);
    }
  }
  if ((short)param_1[0x27] != -1) {
    puStack_158 = puVar6 + (short)param_1[0x27] * 0x2a;
    puStack_15c = (uint *)0x5338fe;
    rasterizer_transparent_geometry_group_draw();
  }
  if ((((short)param_1[5] == 2) && (uVar8 = param_1[2], local_138 = uVar8, uVar8 != DAT_006e1d58))
     && (param_2 == '\0')) {
    puStack_158 = (uint *)&DAT_00000016;
    puStack_15c = DAT_0071d174;
    local_13c = param_1;
    puStack_160 = (uint *)0x533942;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    puStack_160 = (uint *)0x0;
    uStack_168 = (double)CONCAT44(0xa8,DAT_0071d174);
    uStack_16c = 0x533957;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    uStack_16c = 0;
    puStack_170 = (uint *)0x1b;
    puStack_174 = DAT_0071d174;
    puStack_178 = (uint *)0x533969;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    puStack_178 = (uint *)0x0;
    uStack_17c = 0xf;
    puStack_180 = DAT_0071d174;
    iStack_184 = 0x53397b;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    iStack_184 = 1;
    uStack_188 = 7;
    puStack_18c = DAT_0071d174;
    puStack_190 = (uint *)0x53398d;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    puStack_190 = (uint *)0x1;
    pfStack_194 = (float *)&DAT_0000000e;
    puStack_198 = DAT_0071d174;
    puStack_19c = (uint *)0x53399f;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    puStack_19c = (uint *)&DAT_00000004;
    puStack_1a0 = (uint *)&DAT_00000017;
    puStack_1a4 = DAT_0071d174;
    puStack_1a8 = (uint *)0x5339b1;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    puStack_1a8 = (uint *)0x0;
    (**(code **)(*DAT_0071d174 + 0xe4))();
    (**(code **)(*DAT_0071d174 + 0x1ac))();
    (**(code **)(*DAT_0071d174 + 0x10c))();
    piStack_1d0 = (int *)0x2;
    puStack_1d4 = (uint *)0x0;
    puStack_1d8 = DAT_0071d174;
    uStack_1dc = 0x5339fb;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    uStack_1dc = 2;
    puStack_1e0 = (uint *)&DAT_00000004;
    uStack_1e4 = 0;
    puStack_1e8 = DAT_0071d174;
    puStack_1ec = (uint *)0x533a0f;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    puStack_1ec = (uint *)0x3;
    uStack_1f0 = 5;
    uStack_1f4 = 0;
    puStack_1f8 = DAT_0071d174;
    uStack_1fc = 0x533a23;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    uStack_1fc = 1;
    uStack_200 = 1;
    puStack_204 = (uint *)0x1;
    puStack_208 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0x10c))();
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
    (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,DAT_006e1ac0);
    (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,DAT_0069e460);
    puVar6 = param_1;
    do {
      if ((puVar6[2] != uVar8) || ((short)puVar6[5] != 2)) break;
      uVar9 = puVar6[3];
      if ((uVar9 == 0) ||
         ((((sVar4 = *(short *)(uVar9 + 0x24), sVar4 != 5 && (sVar4 != 6)) && (sVar4 != 7)) ||
          ((*(byte *)(uVar9 + 0x29) >> 5 & 1) == 0)))) {
        puStack_130 = (undefined *)puVar6[0x18];
        if ((puStack_130 == (undefined *)0x0) || ((short)puVar6[0x19] == 0)) {
          fStack_12c = (float)CONCAT22(fStack_12c._2_2_,1);
          puStack_130 = PTR_DAT_0069673c;
        }
        else {
          fStack_12c = (float)CONCAT22(fStack_12c._2_2_,(short)puVar6[0x19]);
        }
        puStack_158 = (uint *)0x533af1;
        chimera__rasterizer_set_model_skinning();
        if ((*param_1 & 0x100) != 0) {
          chimera__rasterizer_set_up_node_parts();
          puVar6 = local_13c;
        }
        if (param_1[0x1c] != 0) {
          puStack_158 = (uint *)0x533b1e;
          FUN_00518ce0();
        }
        puStack_158 = (uint *)0x533b2a;
        rasterizer_transparent_geometry_group_draw_vertices(puVar6,(void *)0x0,(char)unaff_EDI);
        uVar8 = local_138;
      }
      iVar7 = (int)(short)((short)puVar6[0x26] + 1);
      if (iVar7 < DAT_0071d154) {
        puVar6 = DAT_0071d14c + *(short *)(DAT_0071d15c + iVar7 * 2) * 0x2a;
      }
      else {
        puVar6 = (uint *)0x0;
      }
      local_13c = puVar6;
    } while (puVar6 != (uint *)0x0);
  }
  iVar7 = rasterizer_transparent_decals_enabled();
  if ((char)iVar7 != '\0') {
    if ((*param_1 & 2) != 0) goto LAB_00533d81;
    if (((((short)param_1[5] == 1) && (local_138 = param_1[2], local_138 != DAT_006e1d58)) &&
        (param_1[3] != 0)) && ((*(short *)(param_1[3] + 0x24) == 4 && (param_2 == '\0')))) {
      puStack_158 = (uint *)&DAT_00000016;
      puStack_15c = DAT_0071d174;
      puStack_160 = (uint *)0x533bd7;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      puStack_160 = (uint *)0x0;
      uStack_168 = (double)CONCAT44(0xa8,DAT_0071d174);
      uStack_16c = 0x533bec;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_16c = 0;
      puStack_170 = (uint *)0x1b;
      puStack_174 = DAT_0071d174;
      puStack_178 = (uint *)0x533bfe;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      puStack_178 = (uint *)0x0;
      uStack_17c = 0xf;
      puStack_180 = DAT_0071d174;
      iStack_184 = 0x533c10;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      iStack_184 = 1;
      uStack_188 = 7;
      puStack_18c = DAT_0071d174;
      puStack_190 = (uint *)0x533c26;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      puStack_190 = (uint *)0x1;
      pfStack_194 = (float *)&DAT_0000000e;
      puStack_198 = DAT_0071d174;
      puStack_19c = (uint *)0x533c37;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      puStack_19c = (uint *)&DAT_00000004;
      puStack_1a0 = (uint *)&DAT_00000017;
      puStack_1a4 = DAT_0071d174;
      puStack_1a8 = (uint *)0x533c49;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      puStack_1a8 = (uint *)0xffffffff;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      (**(code **)(*DAT_0071d174 + 0x1ac))();
      (**(code **)(*DAT_0071d174 + 0x10c))();
      piStack_1d0 = (int *)0x2;
      puStack_1d4 = (uint *)0x0;
      puStack_1d8 = DAT_0071d174;
      uStack_1dc = 0x533c92;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_1dc = 2;
      puStack_1e0 = (uint *)&DAT_00000004;
      uStack_1e4 = 0;
      puStack_1e8 = DAT_0071d174;
      puStack_1ec = (uint *)0x533ca6;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      puStack_1ec = (uint *)0x3;
      uStack_1f0 = 5;
      uStack_1f4 = 0;
      puStack_1f8 = DAT_0071d174;
      uStack_1fc = 0x533cba;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_1fc = 1;
      uStack_200 = 1;
      puStack_204 = (uint *)0x1;
      puStack_208 = DAT_0071d174;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
      puVar6 = param_1;
      do {
        if ((puVar6[2] != uStack_200) || ((short)puVar6[5] != 1)) break;
        if (puVar6[3] != 0) {
          rasterizer_geometry_part_draw(puVar6);
        }
        puVar6 = (uint *)FUN_00515290();
      } while (puVar6 != (uint *)0x0);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,7);
    }
  }
  if ((((*param_1 & 2) == 0) && ((short)DAT_007c1220 == 1)) && (param_2 == '\0')) {
    if (DAT_00689422 == '\0') {
      if ((param_1[3] != 0) &&
         ((*(short *)(param_1[3] + 0x24) == 8 || (cVar3 = FUN_0053fe30(), cVar3 != '\0'))))
      goto LAB_00533d81;
    }
    else if ((param_1[3] == 0) ||
            (((*(short *)(param_1[3] + 0x24) != 4 || ((short)param_1[5] != 1)) ||
             (param_1[2] == DAT_006e1d58)))) goto LAB_00533d81;
    FUN_00519b00();
  }
LAB_00533d81:
  uVar8 = *param_1 & 2;
  if (((((uVar8 == 0) && ((short)DAT_007c1220 == 1)) && ((short)param_1[5] == 1)) &&
      ((param_1[3] != 0 && (*(short *)(param_1[3] + 0x24) == 4)))) && (param_2 == '\0')) {
    uVar13 = FUN_00515290();
    uVar8 = (uint)((ulonglong)uVar13 >> 0x20);
    iVar7 = (int)uVar13;
    if (((iVar7 == 0) || (*(short *)(iVar7 + 0x14) != 1)) ||
       ((*(uint *)(iVar7 + 8) != param_1[2] ||
        ((*(int *)(iVar7 + 0xc) == 0 || (*(short *)(*(int *)(iVar7 + 0xc) + 0x24) != 4)))))) {
      uStack_140._0_3_ = CONCAT12(1,(ushort)uStack_140);
    }
  }
  if (param_1[3] == 0) {
    puStack_158 = (uint *)param_1[0x13];
    puStack_15c = (uint *)0x533df1;
    (*(code *)param_1[0x12])();
  }
  else {
    uStack_114 = 0xffffffff;
    if ((undefined2 *)param_1[0x16] == (undefined2 *)0x0) {
      if (param_1[0x15] != 0xffffffff) {
        uStack_114 = CONCAT22(0xffff,*(undefined2 *)(&DAT_006d99d8 + param_1[0x15] * 0x10));
      }
    }
    else {
      uStack_114 = CONCAT22(0xffff,*(undefined2 *)param_1[0x16]);
    }
    if (uVar8 == 0) {
      if (((undefined *)param_1[0x18] == (undefined *)0x0) || ((short)param_1[0x19] == 0)) {
        fStack_12c = (float)CONCAT22(fStack_12c._2_2_,1);
        puStack_130 = PTR_DAT_0069673c;
      }
      else {
        fStack_12c = (float)CONCAT22(fStack_12c._2_2_,(short)param_1[0x19]);
        puStack_130 = (undefined *)param_1[0x18];
      }
      puStack_158 = (uint *)0x533e6c;
      chimera__rasterizer_set_model_skinning();
      if ((*param_1 & 0x100) != 0) {
        chimera__rasterizer_set_up_node_parts();
      }
      if (param_1[0x1c] != 0) {
        puStack_158 = (uint *)0x533e8e;
        FUN_00518ce0();
      }
    }
    if ((*param_1 & 8) == 0) {
      puStack_158 = (uint *)0x7;
      puStack_15c = DAT_0071d174;
      puStack_160 = (uint *)0x533ede;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      puStack_160 = (uint *)0x0;
      uStack_168 = (double)CONCAT44(0xe,DAT_0071d174);
      uStack_16c = 0x533ef0;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_16c = 4;
      puStack_170 = (uint *)&DAT_00000017;
      puStack_174 = DAT_0071d174;
      puStack_178 = (uint *)0x533f02;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      cVar3 = FUN_0053fde0();
      if (cVar3 == '\0') {
        rasterizer_clear_decal_zbias();
        iStack_124 = 0;
      }
      else {
        chimera__transparent_decal_zbias();
        iStack_124 = 0;
      }
    }
    else {
      if ((short)DAT_007c1220 == 1) {
        puStack_158 = (uint *)0x3b800000;
        puStack_15c = (uint *)0x533eaf;
        chimera__rasterizer_set_frustum_z_func();
      }
      puStack_158 = (uint *)0x7;
      puStack_15c = DAT_0071d174;
      puStack_160 = (uint *)0x533ec4;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      iStack_124 = 0;
    }
    do {
      sVar4 = (short)iStack_124;
      if ((char)(byte)*param_1 < '\0') {
        if ((short)param_1[5] == 1) {
          if (sVar4 < 1) {
            puStack_158 = DAT_0069c664;
            puStack_15c = (uint *)0x533f57;
            chimera__rasterizer_set_frustum_z_func();
            goto LAB_00533fac;
          }
          break;
        }
        if (sVar4 != 0) {
          rasterizer_set_shader_stage_config();
          puStack_158 = (uint *)0x7;
          puStack_15c = DAT_0071d174;
          puStack_160 = (uint *)0x533fa1;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          goto LAB_00533fac;
        }
        uVar8 = param_1[3];
        if (((uVar8 == 0) || (*(short *)(uVar8 + 0x24) != 1)) ||
           ((*(byte *)(uVar8 + 0x28) & 4) == 0)) {
          rasterizer_set_shader_stage_config();
          goto LAB_00533fac;
        }
      }
      else {
        if (0 < sVar4) break;
LAB_00533fac:
        uVar8 = param_1[3];
        switch(*(undefined2 *)(uVar8 + 0x24)) {
        case 1:
          if (*(int *)(uVar8 + 0x58) == -1) {
LAB_00533ffc:
            uStack_140 = (float)((uint)uStack_140 & 0xffffff);
          }
          else {
            uStack_140 = (float)CONCAT13(1,(uint3)uStack_140);
            if (*(short *)(uVar8 + 0x5c) == 2) goto LAB_00533ffc;
          }
          uVar5 = -(ushort)((*(byte *)(uVar8 + 0x28) >> 1 & 1) != 0) & 0xfffa;
          sVar4 = uVar5 + 0x60;
          if ((*param_1 & 4) == 0) {
            switch(*(undefined2 *)(uVar8 + 0x2a)) {
            case 0:
              sVar4 = uVar5 + 0x62;
              break;
            case 1:
            case 5:
              sVar4 = uVar5 + 100;
              break;
            case 2:
              sVar4 = uVar5 + 99;
              break;
            case 3:
            case 4:
            case 6:
              sVar4 = uVar5 + 0x61;
              break;
            case 7:
              sVar4 = uVar5 + 0x65;
            }
          }
          local_13c = &DAT_0069d410 + sVar4 * 8;
          if ((local_13c != (uint *)0x0) && (*local_13c != 0)) {
            if ((param_1[0x17] != 0) && (*(int *)(param_1[0x17] + 0x28) != 0)) {
              puStack_158 = (uint *)0x53407c;
              FUN_00518680();
              puStack_158 = (uint *)0x1;
              puStack_15c = (uint *)0x0;
              puStack_160 = DAT_0071d174;
              uStack_168 = (double)CONCAT44(0x53409d,(undefined4)uStack_168);
              (**(code **)(*DAT_0071d174 + 0x114))();
              uStack_168 = (double)CONCAT44((*(byte *)(uVar8 + 0x2e) & 4 | 2) >> 1,2);
              uStack_16c = 0;
              puStack_170 = DAT_0071d174;
              puStack_174 = (uint *)0x5340bd;
              (**(code **)(*DAT_0071d174 + 0x114))();
              puStack_174 = (uint *)(2 - (uint)((*(byte *)(uVar8 + 0x2e) & 1) != 0));
              puStack_178 = (uint *)&DAT_00000005;
              uStack_17c = 0;
              puStack_180 = DAT_0071d174;
              iStack_184 = 0x5340dd;
              (**(code **)(*DAT_0071d174 + 0x114))();
              iStack_184 = 2 - (uint)((*(byte *)(uVar8 + 0x2e) & 1) != 0);
              uStack_188 = 6;
              puStack_18c = (uint *)0x0;
              puStack_190 = DAT_0071d174;
              pfStack_194 = (float *)0x5340fd;
              (**(code **)(*DAT_0071d174 + 0x114))();
              pfStack_194 = (float *)(2 - (uint)((*(byte *)(uVar8 + 0x2e) & 1) != 0));
              puStack_198 = (uint *)0x7;
              puStack_19c = (uint *)0x0;
              puStack_1a0 = DAT_0071d174;
              puStack_1a4 = (uint *)0x53411d;
              (**(code **)(*DAT_0071d174 + 0x114))();
            }
            puVar6 = local_13c;
            puStack_158 = (uint *)&DAT_00000016;
            puStack_15c = DAT_0071d174;
            puStack_160 = (uint *)0x534133;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            puStack_160 = (uint *)0x7;
            uStack_168 = (double)CONCAT44(0xa8,DAT_0071d174);
            uStack_16c = 0x534148;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            uStack_16c = 1;
            puStack_170 = (uint *)0x1b;
            puStack_174 = DAT_0071d174;
            puStack_178 = (uint *)0x53415a;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            puStack_178 = (uint *)0x0;
            uStack_17c = 0xf;
            puStack_180 = DAT_0071d174;
            iStack_184 = 0x53416c;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            iStack_184 = 0;
            uStack_188 = 0x1c;
            puStack_18c = DAT_0071d174;
            puStack_190 = (uint *)0x53417e;
            (**(code **)(*DAT_0071d174 + 0xe4))();
            puStack_190 = (uint *)0x534187;
            chimera__rasterizer_set_framebuffer_blend_function();
            fStack_144 = 1.0;
            uStack_140 = 0.0;
            local_13c = (uint *)0x0;
            local_138 = 0;
            uStack_134 = 0;
            puStack_130 = (undefined *)0x3f800000;
            fStack_12c = 0.0;
            fStack_128 = 0.0;
            iStack_124 = 0;
            fStack_120 = 0.0;
            uStack_11c = 0x3f800000;
            uStack_118 = 0;
            uStack_b4 = 0x3f800000;
            uStack_b0 = 0;
            uStack_ac = 0;
            fStack_a8 = 0.0;
            fStack_a4 = 0.0;
            fStack_a0 = 1.0;
            fStack_9c = 0.0;
            puStack_98 = (undefined *)0x0;
            fStack_94 = 0.0;
            fStack_90 = 0.0;
            uStack_8c = 0;
            uStack_88 = 0;
            uStack_84 = 0;
            uStack_80 = 0;
            uStack_7c = 0;
            uStack_78 = 0;
            if ((*param_1 & 0x20) != 0) {
              fStack_144 = DAT_007c12c4;
              uStack_140 = DAT_007c12d0;
              local_13c = DAT_007c12dc;
              local_138 = DAT_007c1228;
              uStack_134 = DAT_007c12c8;
              puStack_130 = DAT_007c12d4;
              fStack_12c = DAT_007c12e0;
              fStack_128 = DAT_007c122c;
              iStack_124 = DAT_007c12cc;
              fStack_120 = DAT_007c12d8;
              uStack_11c = DAT_007c12e4;
              uStack_118 = DAT_007c1230;
            }
            if (uStack_17c._3_1_ != '\0') {
              puStack_190 = (uint *)(float)_DAT_007c1200;
              puStack_1a0 = (uint *)param_1[0x10];
              puStack_1a4 = (uint *)param_1[0xf];
              pfStack_194 = (float *)0x0;
              puStack_198 = (uint *)0x0;
              puStack_19c = (uint *)0x0;
              puStack_1a8 = (uint *)0x53434c;
              shader_texture_animation_evaluate();
              puVar6 = puStack_178;
            }
            puStack_190 = (uint *)0x3;
            pfStack_194 = &fStack_144;
            puStack_198 = (uint *)&DAT_0000001a;
            puStack_19c = DAT_0071d174;
            puStack_1a0 = (uint *)0x53436d;
            (**(code **)(*DAT_0071d174 + 0x178))();
            puStack_1a0 = (uint *)&DAT_00000004;
            puStack_1a4 = &uStack_c4;
            puStack_1a8 = (uint *)0xd;
            (**(code **)(*DAT_0071d174 + 0x178))();
            (**(code **)(*DAT_0071d174 + 0x15c))();
            (**(code **)(*DAT_0071d174 + 0x170))();
            piVar14 = (int *)*puVar6;
            (**(code **)(*piVar14 + 0x100))();
            puVar11 = (uint *)0x0;
            if (puStack_1a0 != (uint *)0x0) {
              do {
                piStack_1d0 = (int *)*puVar6;
                puStack_1d4 = (uint *)0x5343dd;
                (**(code **)(*piStack_1d0 + 0x104))();
                piStack_1d0 = (int *)0x5343e6;
                rasterizer_transparent_geometry_group_draw_vertices
                          (param_1,(void *)0x0,(char)piVar14);
                puVar11 = (uint *)((int)puVar11 + 1);
              } while (puVar11 < puStack_1a0);
            }
            piStack_1d0 = (int *)0x5343fd;
            (**(code **)(*(int *)*puVar6 + 0x108))();
          }
          break;
        case 4:
          if ((short)param_1[5] == 1) {
            if (DAT_0071d274 != '\0') {
              return;
            }
            puStack_158 = (uint *)0x533fe2;
            FUN_00519f70();
          }
          break;
        case 6:
          puStack_15c = (uint *)0x53440c;
          puStack_158 = param_1;
          FUN_00531ed0();
          break;
        case 7:
          puStack_15c = (uint *)0x53441e;
          puStack_158 = param_1;
          FUN_00532a40();
          break;
        case 8:
          puStack_158 = (uint *)0x53442d;
          (*DAT_007bf050)();
          break;
        case 9:
          sVar4 = *(short *)(uVar8 + 0x8a);
          puStack_158 = (uint *)&DAT_00000016;
          puStack_15c = DAT_0071d174;
          puStack_160 = (uint *)0x53445c;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          puStack_160 = (uint *)0x7;
          uStack_168 = (double)CONCAT44(0xa8,DAT_0071d174);
          uStack_16c = 0x534471;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          uStack_16c = 1;
          puStack_170 = (uint *)0x1b;
          puStack_174 = DAT_0071d174;
          puStack_178 = (uint *)0x534483;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          puStack_178 = (uint *)0x1;
          uStack_17c = 0xab;
          puStack_180 = DAT_0071d174;
          iStack_184 = 0x534498;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          iStack_184 = 0;
          uStack_188 = 0x18;
          puStack_18c = DAT_0071d174;
          puStack_190 = (uint *)0x5344aa;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          puStack_190 = (uint *)0x0;
          pfStack_194 = (float *)0x1c;
          puStack_198 = DAT_0071d174;
          puStack_19c = (uint *)0x5344bc;
          (**(code **)(*DAT_0071d174 + 0xe4))();
          puStack_19c = (uint *)0x1;
          puStack_1a0 = (uint *)0x1;
          puStack_1a4 = (uint *)0x0;
          puStack_1a8 = DAT_0071d174;
          (**(code **)(*DAT_0071d174 + 0x114))();
          (**(code **)(*DAT_0071d174 + 0x114))();
          (**(code **)(*DAT_0071d174 + 0x114))();
          piStack_1d0 = (int *)0x6;
          puStack_1d4 = (uint *)0x0;
          puStack_1d8 = DAT_0071d174;
          uStack_1dc = 0x53450c;
          (**(code **)(*DAT_0071d174 + 0x114))();
          uStack_1dc = 2;
          puStack_1e0 = (uint *)0x7;
          uStack_1e4 = 0;
          puStack_1e8 = DAT_0071d174;
          puStack_1ec = (uint *)0x534520;
          (**(code **)(*DAT_0071d174 + 0x114))();
          if ((sVar4 == 2) && ((DAT_007c1224 == '\0' || ((short)DAT_007c1220 != 1)))) {
            if (DAT_007c118c < 0xffff0101) {
              puStack_15c = (uint *)0x534552;
              puStack_158 = param_1;
              (*DAT_007c0488)();
            }
          }
          else {
            if ((*(int *)(uVar8 + 0x70) != -1) ||
               (((*(float *)(uVar8 + 0x54) != 0.0 || (*(float *)(uVar8 + 0x58) != 0.0)) ||
                (*(float *)(uVar8 + 0x5c) != 0.0)))) {
              puStack_158 = (uint *)0x5345a1;
              (*DAT_007c0484)();
            }
            if (((0.0 < *(float *)(uVar8 + 0x8c)) || (0.0 < *(float *)(uVar8 + 0x9c))) &&
               ((*(int *)(uVar8 + 0xb8) != -1 || (sVar4 == 2)))) {
              puStack_15c = (uint *)0x5345fb;
              puStack_158 = param_1;
              (*DAT_007c0488)();
            }
            if ((*(int *)(uVar8 + 0x164) != -1) || (*(int *)(uVar8 + 0x178) != -1)) {
              puStack_158 = (uint *)0x53461c;
              (*DAT_007c0480)();
            }
          }
          break;
        case 10:
          if (0xffff0100 < DAT_007c118c) {
            if (((short)uStack_114 == 0) || ((short)uStack_114 == 2)) {
              uStack_10c = 0;
            }
            else if ((short)uStack_114 == 4) {
              uStack_10c = 1;
            }
            if (DAT_0069e1f0 != (int *)0x0) {
              puStack_158 = DAT_0071d174;
              uStack_134 = 0x3f800000;
              local_138 = 0x3f800000;
              uStack_110 = 0x3f800000;
              puStack_130 = (undefined *)0x3f800000;
              puStack_15c = (uint *)0x5346b2;
              (**(code **)(*DAT_0071d174 + 0x15c))();
              puStack_15c = *(uint **)(&DAT_0069e518 + (short)uStack_114 * 8);
              puStack_160 = DAT_0071d174;
              uStack_168 = (double)CONCAT44(0x5346cd,(undefined4)uStack_168);
              (**(code **)(*DAT_0071d174 + 0x170))();
              fVar2 = uStack_140;
              fVar1 = fStack_120;
              if ((param_1[0x1d] != 0) && (iVar7 = *(int *)(param_1[0x1d] + 4), iVar7 != 0)) {
                sVar4 = *(short *)(uVar8 + 0xd8);
                if ((0 < sVar4) && (sVar4 < 5)) {
                  fStack_144 = *(float *)(iVar7 + -4 + sVar4 * 4);
                }
                sVar4 = *(short *)(uVar8 + 0xda);
                if ((0 < sVar4) && (sVar4 < 5)) {
                  unaff_EBX = *(float *)(iVar7 + -4 + sVar4 * 4);
                }
                sVar4 = *(short *)(uVar8 + 0xdc);
                if ((0 < sVar4) && (sVar4 < 5)) {
                  fVar1 = *(float *)(iVar7 + -4 + sVar4 * 4);
                }
                sVar4 = *(short *)(uVar8 + 0xde);
                if ((0 < sVar4) && (sVar4 < 5)) {
                  fVar2 = *(float *)(iVar7 + -4 + sVar4 * 4);
                }
              }
              if (DAT_006893e8._3_1_ != '\0') {
                uStack_168 = (double)((float)_DAT_007c1200 / _DAT_00689454);
                uStack_16c = 0x53478c;
                fVar12 = (float10)periodic_function_evaluate();
                fVar2 = (float)fVar12;
                fStack_144 = fVar2;
                if (0.0 <= DAT_00689458) {
                  fStack_144 = DAT_00689458;
                }
                unaff_EBX = fVar2;
                if (0.0 <= DAT_0068945c) {
                  unaff_EBX = DAT_0068945c;
                }
                fVar1 = fVar2;
                if (0.0 <= _DAT_00689460) {
                  fVar1 = _DAT_00689460;
                }
                if (0.0 <= _DAT_00689464) {
                  fVar2 = _DAT_00689464;
                }
              }
              fStack_bc = fVar1;
              puStack_130 = (undefined *)(unaff_EBX * *(float *)(uVar8 + 0xa0));
              fStack_12c = unaff_EBX * *(float *)(uVar8 + 0xa4);
              fStack_128 = unaff_EBX * *(float *)(uVar8 + 0xa8);
              fVar2 = fVar2 * 8.0;
              if (fVar2 <= 1.0) {
                fVar2 = 1.0;
              }
              fStack_cc = 1.0 / fVar2;
              uStack_d8 = *(undefined4 *)(uVar8 + 0x88);
              uStack_d4 = *(undefined4 *)(uVar8 + 0x8c);
              uStack_d0 = *(undefined4 *)(uVar8 + 0x90);
              uStack_c8 = *(undefined4 *)(uVar8 + 0x7c);
              uStack_c4 = *(uint *)(uVar8 + 0x80);
              uStack_c0 = *(undefined4 *)(uVar8 + 0x84);
              uStack_b8 = *(undefined4 *)(uVar8 + 0x94);
              uStack_b4 = *(undefined4 *)(uVar8 + 0x98);
              uStack_b0 = *(undefined4 *)(uVar8 + 0x9c);
              if ((*(byte *)(uVar8 + 0x28) & 4) == 0) {
                uStack_8c = 0x3f800000;
                puStack_98 = puStack_130;
                fStack_94 = fStack_12c;
                fStack_90 = fStack_128;
              }
              else {
                uStack_8c = 0xbf800000;
                puStack_98 = (undefined *)-(float)puStack_130;
                fStack_94 = -fStack_12c;
                fStack_90 = -fStack_128;
              }
              if ((*(byte *)(uVar8 + 0x28) & 8) == 0) {
                fStack_dc = fStack_144;
                uStack_ac = 0;
                fStack_a8 = *(float *)(uVar8 + 0xac);
                fStack_a4 = *(float *)(uVar8 + 0xb0);
                fStack_a0 = *(float *)(uVar8 + 0xb4);
                fStack_9c = fStack_144;
              }
              else {
                fStack_dc = *(float *)(uVar8 + 0xb8);
                fStack_a8 = *(float *)(uVar8 + 0xac) * fStack_144;
                uStack_ac = *(undefined4 *)(uVar8 + 0xbc);
                fStack_a4 = *(float *)(uVar8 + 0xb0) * fStack_144;
                fStack_a0 = *(float *)(uVar8 + 0xb4) * fStack_144;
                fStack_9c = *(float *)(uVar8 + 0xb8);
              }
              uStack_168 = (double)CONCAT44(&DAT_0069e1f0,(uint)(ushort)param_1[4]);
              uStack_16c = 1;
              puStack_170 = (uint *)0x0;
              puStack_174 = (uint *)0x534a7d;
              puStack_e8 = puStack_130;
              fStack_e4 = fStack_12c;
              fStack_e0 = fStack_128;
              FUN_00518860();
              uStack_168 = 2.12199579145934e-314;
              uStack_16c = 0;
              puStack_170 = DAT_0071d174;
              puStack_174 = (uint *)0x534a94;
              (**(code **)(*DAT_0071d174 + 0x114))();
              puStack_174 = (uint *)0x1;
              puStack_178 = (uint *)0x2;
              uStack_17c = 0;
              puStack_180 = DAT_0071d174;
              iStack_184 = 0x534aa8;
              (**(code **)(*DAT_0071d174 + 0x114))();
              iStack_184 = 2 - (uint)((*(byte *)(uVar8 + 0x28) & 0x10) != 0);
              uStack_188 = 5;
              puStack_18c = (uint *)0x0;
              puStack_190 = DAT_0071d174;
              pfStack_194 = (float *)0x534ac8;
              (**(code **)(*DAT_0071d174 + 0x114))();
              pfStack_194 = (float *)(2 - (uint)((*(byte *)(uVar8 + 0x28) & 0x10) != 0));
              puStack_198 = (uint *)&DAT_00000006;
              puStack_19c = (uint *)0x0;
              puStack_1a0 = DAT_0071d174;
              puStack_1a4 = (uint *)0x534ae8;
              (**(code **)(*DAT_0071d174 + 0x114))();
              puStack_1a4 = (uint *)(2 - (uint)((*(byte *)(uVar8 + 0x28) & 0x10) != 0));
              puStack_1a8 = (uint *)0x7;
              (**(code **)(*DAT_0071d174 + 0x114))();
              (**(code **)(*DAT_0071d174 + 0xe4))();
              (**(code **)(*DAT_0071d174 + 0xe4))();
              piStack_1d0 = (int *)0x1b;
              puStack_1d4 = DAT_0071d174;
              puStack_1d8 = (uint *)0x534b4e;
              (**(code **)(*DAT_0071d174 + 0xe4))();
              puStack_1d8 = (uint *)0x2;
              uStack_1dc = 0x13;
              puStack_1e0 = DAT_0071d174;
              uStack_1e4 = 0x534b60;
              (**(code **)(*DAT_0071d174 + 0xe4))();
              uStack_1e4 = 2;
              puStack_1e8 = (uint *)&DAT_00000014;
              puStack_1ec = DAT_0071d174;
              uStack_1f0 = 0x534b72;
              (**(code **)(*DAT_0071d174 + 0xe4))();
              uStack_1f0 = 1;
              uStack_1f4 = 0xab;
              puStack_1f8 = DAT_0071d174;
              uStack_1fc = 0x534b87;
              (**(code **)(*DAT_0071d174 + 0xe4))();
              uStack_1fc = 0;
              uStack_200 = 0xf;
              puStack_204 = DAT_0071d174;
              puStack_208 = (uint *)0x534b99;
              (**(code **)(*DAT_0071d174 + 0xe4))();
              puStack_208 = (uint *)0x0;
              (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c);
              puStack_e8 = (undefined *)param_1[0xf];
              uStack_d4 = param_1[0x10];
              uVar8 = 3;
              uStack_f8 = 0x3f800000;
              uStack_f4 = 0x3f800000;
              uStack_f0 = 0x3f800000;
              uStack_ec = 0x3f800000;
              fStack_e4 = 0.0;
              fStack_e0 = 0.0;
              fStack_dc = 0.0;
              uStack_d8 = 0;
              uStack_d0 = 0;
              fStack_cc = 0.0;
              (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,10,&uStack_f8);
              if ((DAT_006893e8._3_1_ != '\0') && (DAT_00689412 != 0)) {
                (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,0);
              }
              (**(code **)(*DAT_0071d174 + 0x1b4))(DAT_0071d174,0,&puStack_1a8,6);
              piVar14 = DAT_0069e1f0;
              (**(code **)(*DAT_0069e1f0 + 0x100))(DAT_0069e1f0,&puStack_208,3);
              uVar9 = 0;
              if (uVar8 != 0) {
                do {
                  (**(code **)(*DAT_0069e1f0 + 0x104))(DAT_0069e1f0,uVar9);
                  rasterizer_transparent_geometry_group_draw_vertices
                            (param_1,(void *)0x0,(char)piVar14);
                  uVar9 = uVar9 + 1;
                } while (uVar9 < uVar8);
              }
              (**(code **)(*DAT_0069e1f0 + 0x108))(DAT_0069e1f0);
            }
          }
          break;
        case 0xb:
          FUN_0052c4a0();
        }
      }
      iStack_124 = iStack_124 + 1;
    } while ((short)iStack_124 < 2);
    if (((*param_1 & 8) != 0) && ((short)DAT_007c1220 == 1)) {
      puStack_158 = (uint *)0x0;
      puStack_15c = (uint *)0x534d09;
      chimera__rasterizer_set_frustum_z_func();
    }
    if (((char)(byte)*param_1 < '\0') && ((short)param_1[5] == 1)) {
      puStack_158 = (uint *)0x0;
      puStack_15c = (uint *)0x534d21;
      chimera__rasterizer_set_frustum_z_func();
    }
    if ((_DAT_007c10e4 & 0x4000000) != 0) {
      puStack_158 = (uint *)0xc3;
      puStack_15c = DAT_0071d174;
      puStack_160 = (uint *)0x534d45;
      (**(code **)(*DAT_0071d174 + 0xe4))();
    }
    if ((_DAT_007c10e4 & 0x2000000) != 0) {
      puStack_158 = (uint *)0xaf;
      puStack_15c = DAT_0071d174;
      puStack_160 = (uint *)0x534d66;
      (**(code **)(*DAT_0071d174 + 0xe4))();
    }
  }
  if (param_2 == '\0') {
    DAT_006e1d58 = param_1[2];
  }
  if (*(short *)((int)param_1 + 0x9e) != -1) {
    puStack_158 = DAT_0071d14c + *(short *)((int)param_1 + 0x9e) * 0x2a;
    puStack_15c = (uint *)0x534d9e;
    rasterizer_transparent_geometry_group_draw();
  }
  if ((uStack_140._2_1_ != '\0') && (0 < (short)DAT_0071d158)) {
    psVar10 = (short *)(DAT_0071d150 + 0x14);
    uVar8 = DAT_0071d158 & 0xffff;
    do {
      if ((*(uint *)(psVar10 + 0x46) == param_1[2]) && (*psVar10 == 1)) {
        puStack_158 = (uint *)(psVar10 + -10);
        puStack_15c = (uint *)0x534ddc;
        rasterizer_transparent_geometry_group_draw();
        if (DAT_00689412 != 0) {
          DAT_0071d274 = '\x01';
        }
      }
      psVar10 = psVar10 + 0x54;
      uVar8 = uVar8 - 1;
    } while (uVar8 != 0);
  }
  return;
}
#endif
