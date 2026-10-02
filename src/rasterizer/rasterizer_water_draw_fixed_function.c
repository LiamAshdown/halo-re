// rasterizer_water_draw_fixed_function  (Ghidra: LAB_005358b0, never created as a function; the water draw procedure
//   rasterizer_select_hardware_codepaths installs when the device has no pixel shaders)
// address 0x5358b0, size 1816 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x5358b0..0x535fc7. The fixed-function twin of
//   rasterizer_water_draw_pixel_shader (0x535fd0): same gates (only the 0x6893fe toggle), the same tag flag 8
//   pass, and the same rasterizer_effects[102] pass 0 (tag flag 1, base map on stage 0 only) / pass 1 (tag flag 2,
//   blended) and rasterizer_effects[103] passes, but always with vertex shader 0 (fixed function), no shader
//   constants, no ripple texture refresh, and the rasterizer globals' +0xe8 bitmap on stage 0 for effect 103.
// blam-cc: stack -> group (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t rasterizer_water_enabled;         // 0x006893fe
extern void *rasterizer_device;                  // 0x0071d174
extern GlobalsRasterizerData *rasterizer_globals_data; // 0x0071d164
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

extern int16_t *chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type,
    int16_t default_index, int16_t frame); // 0x518960, EAX bitmap
extern uint8_t chimera__rasterizer_set_texture_direct_d3d9(uint32_t bitmap_tag_id, int16_t stage, int16_t frame); // 0x518770, EAX bitmap
extern void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag); // 0x00533660, ECX

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);

#define DEVICE_CALL(index) ((*(void ***)rasterizer_device)[(index) / 4])

static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)DEVICE_CALL(0xe4))(rasterizer_device, state, value);
}

static void set_stage0_samplers(uint32_t filter)
{
    d3d_call3_fn set_sampler_state = (d3d_call3_fn)DEVICE_CALL(0x114);

    set_sampler_state(rasterizer_device, 0, 1, filter);
    set_sampler_state(rasterizer_device, 0, 2, filter);
    set_sampler_state(rasterizer_device, 0, 5, 2);
    set_sampler_state(rasterizer_device, 0, 6, 2);
    set_sampler_state(rasterizer_device, 0, 7, 2);
}

static void effect_draw(void *effect, int32_t only_pass, transparent_geometry_group *group)
{
    void **vtable = *(void ***)effect;
    uint32_t passes = 0;
    uint32_t pass;

    ((int32_t (__stdcall *)(void *, uint32_t *, uint32_t))vtable[0x100 / 4])(effect, &passes, 3); // Begin
    if (only_pass >= 0) {
        ((int32_t (__stdcall *)(void *, uint32_t))vtable[0x104 / 4])(effect, (uint32_t)only_pass);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
    } else {
        for (pass = 0; pass < passes; pass++) {
            ((int32_t (__stdcall *)(void *, uint32_t))vtable[0x104 / 4])(effect, pass);
            rasterizer_transparent_geometry_group_draw_vertices(group, 0);
        }
    }
    ((int32_t (__stdcall *)(void *))vtable[0x108 / 4])(effect); // End
}

void rasterizer_water_draw_fixed_function(transparent_geometry_group *group)
{
    uint8_t *raw = (uint8_t *)group;
    uint8_t *water = *(uint8_t **)&((struct transparent_geometry_group *)raw)->shader;   // edi
    uint16_t frame = ((struct transparent_geometry_group *)raw)->shader_permutation;
    int16_t vertex_type = -1;                    // bp
    uint32_t declaration;
    uint8_t z_write;                             // bl
    void *effect;

    if (rasterizer_water_enabled == 0) {
        return;
    }
    if (*(void **)&((struct transparent_geometry_group *)raw)->vertex_buffer != 0) {
        vertex_type = **(int16_t **)&((struct transparent_geometry_group *)raw)->vertex_buffer;
    } else if (((struct transparent_geometry_group *)raw)->dynamic_vertex_slot != -1) {
        vertex_type = rasterizer_dynamic_vertex_slots[((struct transparent_geometry_group *)raw)->dynamic_vertex_slot].vertex_type;
    }
    declaration = (uint32_t)rasterizer_vertex_declarations[vertex_type].declaration;

    if ((*(uint16_t *)(water + 0x28) & 8) != 0 && (raw[0] & 0x12) == 0) {
        d3d_call3_fn set_texture_stage_state;

        set_render_state(0x16, 1);
        set_render_state(0xa8, 0);
        set_render_state(0x1b, 0);
        set_render_state(0xf, 0);
        set_render_state(7, 1);
        set_render_state(0x17, 4);
        set_render_state(0xe, 1);
        set_render_state(0x1c, 0);
        ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, declaration);
        ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, 0);
        ((d3d_call1_fn)DEVICE_CALL(0x1ac))(rasterizer_device, 0);
        set_render_state(0x3c, 0xffffffff);
        set_texture_stage_state = (d3d_call3_fn)DEVICE_CALL(0x10c);
        set_texture_stage_state(rasterizer_device, 0, 1, 2);
        set_texture_stage_state(rasterizer_device, 0, 2, 3);
        set_texture_stage_state(rasterizer_device, 0, 4, 2);
        set_texture_stage_state(rasterizer_device, 0, 5, 3);
        set_texture_stage_state(rasterizer_device, 1, 1, 1);
        set_texture_stage_state(rasterizer_device, 1, 4, 1);
        rasterizer_transparent_geometry_group_draw_vertices(group, 0);
        return;
    }

    z_write = (uint8_t)((raw[0] & 0x10) == 0 && (*(uint16_t *)(water + 0x28) & 8) == 0);

    effect = (void *)rasterizer_effects[102].effect;
    if (effect != 0) {
        ((d3d_call1_fn)DEVICE_CALL(0x15c))(rasterizer_device, declaration);
        ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, 0);
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
            set_stage0_samplers(3);
            effect_draw(effect, 0, group);
        }
        if (water[0x28] & 2) {
            chimera__rasterizer_set_texture(*(uint32_t *)(water + 0x58), 0, 0, 1, (int16_t)frame);
            set_stage0_samplers(3);
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
            effect_draw(effect, 1, group);
        }
    }

    effect = (void *)rasterizer_effects[103].effect;
    if (effect != 0) {
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
        ((d3d_call1_fn)DEVICE_CALL(0x170))(rasterizer_device, 0);
        chimera__rasterizer_set_texture_direct_d3d9(*(uint32_t *)((uint8_t *)rasterizer_globals_data + 0xe8), 0, 0);
        set_stage0_samplers(1);
        effect_draw(effect, -1, group);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
