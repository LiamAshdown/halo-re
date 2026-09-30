// rasterizer_water_draw_pixel_shader  (Ghidra: FUN_00535fd0 (never created as a function; stored into
//   rasterizer_water_draw_procedure 0x7bf050 by rasterizer_select_hardware_codepaths when pixel shaders exist))
// address 0x535fd0, size 2970 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x535fd0..0x536b69 (DecompileAt skeleton; every device call's arguments read from
//   the disassembly). Draws one transparent geometry group with a shader_transparent_water tag (group +0x0c):
//   - skipped while byte 0x69c689 is set or byte 0x6893fe is clear;
//   - tag flag 8 (+0x28) without group flags 0x12: a plain fixed-function pass (texture stage states, no pixel
//     shader) and return;
//   - otherwise, after refreshing the ripple texture when 0x71d275 asks (0x534f80), up to three effect passes:
//     rasterizer_effects[102] pass 0 (tag flag 1: the base map on stage 0, the globals' +0x1c bitmap on stage 1,
//     opaque) and pass 1 (tag flag 2: blended), with pixel constants c0 = (+0x6c) x4 and c1 = (+0x7c) x4; then
//     rasterizer_effects[103] for the reflection/ripple pass: vertex constants c10.. = (+0xc4, +0xc4,
//     cos(+0xbc) * +0xc0 * time, sin(+0xbc) * +0xc0 * time, 0...), and either (pixel shader >= 1.1) render target
//     8 on stage 0, the ripple map (+0xa8) on stage 3 and c0 = the view-angle blend of +0x70 / +0x80 colours, or the
//     globals' +0xe8 bitmap on stage 0; every pass of the effect draws the group.
//   Z writes are on only when group flag 0x10 and tag flag 8 are both clear. The vertex type (group +0x58's first
//   word, else the dynamic slot's) picks the declaration and vertex shader 60/62 (+1 for type 4); any other type
//   leaves the original's shader index uninitialised (0 here, UNSURE).
// blam-cc: stack -> group (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include "fn_rasterizer.h"

extern uint8_t rasterizer_caps_flag_689;                 // 0x0069c689
extern uint8_t rasterizer_water_enabled;         // 0x006893fe (also gates 0x534f80)
extern uint8_t unknown_0071d275;                 // 0x0071d275, "refresh the water ripple texture"
extern void *rasterizer_device;                  // 0x0071d174
extern GlobalsRasterizerData *rasterizer_globals_data; // 0x0071d164
extern rasterizer_frame_time rasterizer_time;    // 0x007c1200
extern float rasterizer_camera_forward[3];       // 0x007c1234
extern d3d_caps9 rasterizer_caps;                // 0x007c10c0
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358


// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, bitmap_type, default_index, frame)

// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, frame)

// blam-cc: ECX -> group

extern real vector3d_length(real_vector3d *v); // 0x401960, EAX

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_call4v_fn)(void *self, uint32_t start_register, const void *data, uint32_t count);
extern double sin(double x);
extern double cos(double x);

#define DEVICE_CALL(index) ((*(void ***)rasterizer_device)[(index) / 4])

static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)DEVICE_CALL(0xe4))(rasterizer_device, state, value);
}

static void set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)DEVICE_CALL(0x114))(rasterizer_device, stage, type, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)DEVICE_CALL(0x10c))(rasterizer_device, stage, type, value);
}

// stage: filters (1, 2 [, 3]) = filter_value, then address modes 5, 6, 7 = 2
static void set_stage_samplers(uint32_t stage, uint32_t filter, uint8_t with_mip)
{
    set_sampler_state(stage, 1, filter);
    set_sampler_state(stage, 2, filter);
    if (with_mip) {
        set_sampler_state(stage, 3, filter);
    }
    set_sampler_state(stage, 5, 2);
    set_sampler_state(stage, 6, 2);
    set_sampler_state(stage, 7, 2);
}

static void effect_draw_all_passes(void *effect, transparent_geometry_group *group)
{
    void **vtable = *(void ***)effect;
    uint32_t passes = 0;
    uint32_t pass;

    ((int32_t (__stdcall *)(void *, uint32_t *, uint32_t))vtable[0x100 / 4])(effect, &passes, 3); // Begin
    for (pass = 0; pass < passes; pass++) {
        ((int32_t (__stdcall *)(void *, uint32_t))vtable[0x104 / 4])(effect, pass); // BeginPass
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    }
    ((int32_t (__stdcall *)(void *))vtable[0x108 / 4])(effect); // End
}

static void effect_draw_pass(void *effect, uint32_t pass, transparent_geometry_group *group)
{
    void **vtable = *(void ***)effect;
    uint32_t passes = 0;

    ((int32_t (__stdcall *)(void *, uint32_t *, uint32_t))vtable[0x100 / 4])(effect, &passes, 3); // Begin
    ((int32_t (__stdcall *)(void *, uint32_t))vtable[0x104 / 4])(effect, pass); // BeginPass
    rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    ((int32_t (__stdcall *)(void *))vtable[0x108 / 4])(effect); // End
}

void rasterizer_water_draw_pixel_shader(transparent_geometry_group *group)
{
    uint8_t *raw = (uint8_t *)group;
    uint8_t *water = *(uint8_t **)&((struct transparent_geometry_group *)raw)->shader;      // esi
    uint16_t frame = ((struct transparent_geometry_group *)raw)->shader_permutation;
    int16_t vertex_type = -1;                       // ebp
    int16_t shader_index = 0;                       // esp+0x10, UNSURE for other vertex types
    uint32_t declaration;
    uint8_t z_write;                                // bl
    void *effect;

    if (rasterizer_caps_flag_689 != 0 || rasterizer_water_enabled == 0) {
        return;
    }
    if (*(void **)&((struct transparent_geometry_group *)raw)->vertex_buffer != 0) {
        vertex_type = **(int16_t **)&((struct transparent_geometry_group *)raw)->vertex_buffer;
    } else if (((struct transparent_geometry_group *)raw)->dynamic_vertex_slot != -1) {
        vertex_type = rasterizer_dynamic_vertex_slots[((struct transparent_geometry_group *)raw)->dynamic_vertex_slot].vertex_type;
    }
    if (vertex_type == 0 || vertex_type == 2) {
        shader_index = 0;
    } else if (vertex_type == 4) {
        shader_index = 1;
    }
    declaration = (uint32_t)rasterizer_vertex_declarations[vertex_type].declaration;

    // 0x536040: the fixed-function pass
    if ((*(uint16_t *)(water + 0x28) & 8) != 0 && (raw[0] & 0x12) == 0) {
        set_render_state(0x16, 1);
        set_render_state(0xa8, 0);
        set_render_state(0x1b, 0);
        set_render_state(0xf, 0);
        set_render_state(7, 1);
        set_render_state(0x17, 4);
        set_render_state(0xe, 1);
        set_render_state(0x1c, 0);
        ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, declaration);
        ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, rasterizer_vertex_shaders[60 + shader_index].shader);
        ((d3d_call1_fn)DEVICE_CALL(0x1ac))(rasterizer_device, 0);
        set_render_state(0x3c, 0xffffffff);
        set_texture_stage_state(0, 1, 2);
        set_texture_stage_state(0, 2, 3);
        set_texture_stage_state(0, 4, 2);
        set_texture_stage_state(0, 5, 3);
        set_texture_stage_state(1, 1, 1);
        set_texture_stage_state(1, 4, 1);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
        return;
    }

    z_write = (uint8_t)((raw[0] & 0x10) == 0 && (*(uint16_t *)(water + 0x28) & 8) == 0);
    if (unknown_0071d275 != 0) {
        rasterizer_water_update_ripple_texture(water);
        unknown_0071d275 = 0;
    }

    effect = (void *)rasterizer_effects[102].effect;
    if (effect != 0) {
        float constants[8]; // esp+0x18: c0 = (+0x6c) x4, c1 = (+0x7c) x4

        constants[0] = constants[1] = constants[2] = constants[3] = *(float *)(water + 0x6c);
        constants[4] = constants[5] = constants[6] = constants[7] = *(float *)(water + 0x7c);
        ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, declaration);
        ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, rasterizer_vertex_shaders[60 + shader_index].shader);
        if (water[0x28] & 1) {
            set_render_state(0x16, 1);
            set_render_state(0xa8, 8);
            set_render_state(0x1b, 0);
            set_render_state(0xf, 0);
            set_render_state(7, 1);
            set_render_state(0x17, 4);
            set_render_state(0xe, z_write);
            set_render_state(0x1c, 0);
            chimera__rasterizer_set_texture(*(uint32_t *)(water + 0x58), 0, 0, 1, (int16_t)frame);
            set_stage_samplers(0, 3, 0);
            chimera__rasterizer_set_texture_direct_d3d9(*(uint32_t *)((uint8_t *)rasterizer_globals_data + 0x1c), 1, 0);
            set_stage_samplers(1, 3, 1);
            ((d3d_call4v_fn)DEVICE_CALL(0x1b4))(rasterizer_device, 0, constants, 2);
            effect_draw_pass(effect, 0, group);
        }
        if (water[0x28] & 2) {
            chimera__rasterizer_set_texture(*(uint32_t *)(water + 0x58), 0, 0, 1, (int16_t)frame);
            set_stage_samplers(0, 3, 0);
            set_render_state(0x16, 1);
            set_render_state(0xa8, 7);
            set_render_state(0x1b, 1);
            set_render_state(0x13, 1);
            set_render_state(0x14, 3);
            set_render_state(0xab, 1);
            set_render_state(0xf, 0);
            set_render_state(7, 1);
            set_render_state(0x17, 4);
            set_render_state(0xe, z_write);
            set_render_state(0x1c, (water[0x28] >> 2) & 1);
            ((d3d_call4v_fn)DEVICE_CALL(0x1b4))(rasterizer_device, 0, constants, 2);
            effect_draw_pass(effect, 1, group);
        }
    }

    effect = (void *)rasterizer_effects[103].effect;
    if (effect != 0) {
        float vertex_constants[12]; // esp+0x18: c10..c12

        set_render_state(0x16, 1);
        set_render_state(0xa8, 7);
        set_render_state(0x1b, (~(*(uint32_t *)raw >> 4)) & 1);
        set_render_state(0x13, (water[0x28] & 1) ? 7 : 2);
        set_render_state(0x14, 2);
        set_render_state(0xab, 1);
        set_render_state(0xf, 0);
        set_render_state(7, 1);
        set_render_state(0x17, 4);
        set_render_state(0xe, z_write);
        set_render_state(0x1c, (water[0x28] >> 2) & 1);
        ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, declaration);
        ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, rasterizer_vertex_shaders[62 + shader_index].shader);
        memset(vertex_constants, 0, sizeof vertex_constants);
        vertex_constants[0] = *(float *)(water + 0xc4);
        vertex_constants[1] = *(float *)(water + 0xc4);
        vertex_constants[2] = (float)(cos((double)*(float *)(water + 0xbc)) * *(float *)(water + 0xc0) * rasterizer_time.time);
        vertex_constants[3] = (float)(sin((double)*(float *)(water + 0xbc)) * *(float *)(water + 0xc0) * rasterizer_time.time);
        ((d3d_call4v_fn)DEVICE_CALL(0x178))(rasterizer_device, 10, vertex_constants, 3);

        if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
            chimera__rasterizer_set_texture_direct_d3d9(*(uint32_t *)((uint8_t *)rasterizer_globals_data + 0xe8), 0, 0);
            set_stage_samplers(0, 1, 0);
            effect_draw_all_passes(effect, group);
            return;
        }
        {
            float pixel_constant[4]; // esp+0x18
            real_vector3d *normal = (real_vector3d *)(raw + 0x88);

            ((d3d_call2_fn)DEVICE_CALL(0x104))(rasterizer_device, 0, rasterizer_render_targets[8].texture);
            set_stage_samplers(0, 1, 0);
            chimera__rasterizer_set_texture(*(uint32_t *)(water + 0xa8), 3, 2, 0, (int16_t)frame);
            set_stage_samplers(3, 3, 1);
            if (vector3d_length(normal) > 0.0f) {
                float facing = -(rasterizer_camera_forward[1] * normal->j + rasterizer_camera_forward[2] * normal->k +
                    rasterizer_camera_forward[0] * normal->i);
                float t;
                float one_minus_t;

                if (facing < 0.0f) {
                    t = 0.0f;
                } else if (!(facing <= 1.0f)) {
                    t = 1.0f;
                } else {
                    t = facing;
                }
                one_minus_t = 1.0f - t;
                pixel_constant[0] = one_minus_t * *(float *)(water + 0x80) + t * *(float *)(water + 0x70);
                pixel_constant[1] = one_minus_t * *(float *)(water + 0x84) + t * *(float *)(water + 0x74);
                pixel_constant[2] = t * *(float *)(water + 0x78) + one_minus_t * *(float *)(water + 0x88);
            } else {
                pixel_constant[0] = 1.0f;
                pixel_constant[1] = 1.0f;
                pixel_constant[2] = 1.0f;
            }
            pixel_constant[3] = 0.0f;
            ((d3d_call4v_fn)DEVICE_CALL(0x1b4))(rasterizer_device, 0, pixel_constant, 1);
            effect_draw_all_passes(effect, group);
        }
    }
}
