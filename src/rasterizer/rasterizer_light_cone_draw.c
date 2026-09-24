// rasterizer_light_cone_draw  (Ghidra: FUN_0051dc50)
// address 0x51dc50, size 660 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: the draw half of the effect 4 (0x0069d490) family whose state halves are
//   rasterizer_light_cone_set_texture_stage_states 0x51d6a0 (distance attenuation on stage 2,
//   vector normalization on stage 3) and rasterizer_light_cone_set_orientation_constants 0x51da20
//   (the light cube map on stage 1 and the light constants). It draws one shader_environment
//   surface for the current light: the bump map (none when bump_map_is_specular_mask is set) on
//   stage 0 through the same inline resolution as the other environment passes, the bump
//   transform at vertex shader c10..c12, ShaderEnvironment.material_color (+0x10c) with w 1.0 at
//   pixel shader c1, the environment declaration (type 0) with its software processing usage,
//   vertex shader 9 (0x0069e398), and one single stream draw (0x51c1c0) per effect pass. The
//   name comes from the phase 2 summary (volumetric light cone); the code draws lit
//   environment geometry, so the family is really the per light environment pass (UNSURE).
//   Spot-check fix (phase 4 review): the earlier file was a structural sketch; rewritten from the
//   raw code 0x51dc50..0x51dee3. The arguments are the environment pass list (shader, frame,
//   dynamic_index_slot, first_primitive, primitive_count, vertex_buffer).
// register convention: __cdecl, all six arguments on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"

extern void *rasterizer_device;                                     // 0x0071d174
extern rasterizer_frame_time rasterizer_time;                       // 0x007c1200
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern int16_t rasterizer_bound_bitmap_size_b[2];                   // 0x006d9870
extern uint8_t rasterizer_software_vertex_processing;               // 0x0069c680
extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893f3;                         // 0x006893f3 light pass enable
extern uint8_t console_debug_toggle_689409;                         // 0x00689409

// blam-cc: EAX -> bitmap_tag_id, DX -> index
extern BitmapData *bitmap_group_get_bitmap_data(uint32_t bitmap_tag_id, int16_t index); // 0x43f250
// blam-cc: ESI -> bitmap, EDI -> effect_slot, stack -> stage
extern uint8_t rasterizer_bind_texture_d3dx(int16_t stage, BitmapData *bitmap, rasterizer_effect_slot *effect_slot); // 0x5186c0
// blam-cc: ESI -> shader_environment
extern void shader_environment_texture_scrolling_evaluate(float *u, float *v, double time, const ShaderEnvironment *shader); // 0x540060
// blam-cc: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                       int32_t dynamic_index_slot, int32_t first_primitive); // 0x51c1c0

typedef int32_t (*d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (*d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (*d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (*d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (*d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (*d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

void rasterizer_light_cone_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive,
                                int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    const uint8_t *raw = (const uint8_t *)shader;
    rasterizer_effect_slot *effect_slot = &rasterizer_effects[4];
    void *effect = (void *)effect_slot->effect;
    uint32_t bump_map_tag;
    BitmapData *bump_bitmap = 0;
    float constants[12];
    float color[4];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f3 == 0 || effect == 0) {
        return;
    }

    // stage 0: the bump map (unless it is a specular mask), or entry 3 of the default 2D bitmap
    bump_map_tag = (raw[0x28] & 2) != 0 ? 0xffffffff : *(const uint32_t *)(raw + 0x134);
    if (console_debug_toggle_689409 != 0 && bump_map_tag != 0xffffffff) {
        Bitmap *bitmap = (Bitmap *)tag_instances[bump_map_tag & 0xffff].data;
        int32_t count = (int32_t)bitmap->bitmap_data.count;

        if (count > 0) {
            bump_bitmap = bitmap_group_get_bitmap_data(bump_map_tag, (int16_t)((int32_t)frame % count));
            if (*(int16_t *)((uint8_t *)bump_bitmap + 0xa) != 0) {
                bump_bitmap = 0;
            }
        }
    }
    if (bump_bitmap == 0) {
        uint32_t default_tag = *(uint32_t *)&rasterizer_globals_data->default_2d.tag_id;

        if (default_tag != 0xffffffff) {
            Bitmap *bitmap = (Bitmap *)tag_instances[default_tag & 0xffff].data;

            if (bitmap != 0 && (int32_t)bitmap->bitmap_data.count > 3) {
                bump_bitmap = (BitmapData *)((uint8_t *)bitmap->bitmap_data.pointer + 3 * 0x30);
            }
        }
    }
    if (bump_bitmap != 0) {
        rasterizer_bind_texture_d3dx(0, bump_bitmap, effect_slot);
        rasterizer_bound_bitmap_size_b[0] = (int16_t)bump_bitmap->width;
        rasterizer_bound_bitmap_size_b[1] = (int16_t)bump_bitmap->height;
    }

    constants[0] = *(const float *)(raw + 0x138);
    constants[1] = *(const float *)(raw + 0x13c);
    constants[2] = 1.0f;
    constants[3] = 1.0f;
    constants[4] = 1.0f;
    constants[5] = 0.0f;
    constants[6] = 0.0f;
    constants[7] = 0.0f;
    constants[8] = 0.0f;
    constants[9] = 1.0f;
    constants[10] = 0.0f;
    constants[11] = 0.0f;
    shader_environment_texture_scrolling_evaluate(&constants[7], &constants[11], rasterizer_time.time, shader);
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xa, constants, 3);

    color[0] = *(const float *)(raw + 0x10c);                   // material_color
    color[1] = *(const float *)(raw + 0x110);
    color[2] = *(const float *)(raw + 0x114);
    color[3] = 1.0f;
    ((d3d_set_constant_f_fn)device_vtable()[0x1b4 / 4])(rasterizer_device, 1, color, 1);

    ((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(rasterizer_device, (void *)rasterizer_vertex_declarations[0].declaration);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, ((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                                   rasterizer_vertex_declarations[0].usage) & 0x10);
    ((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device, (void *)rasterizer_vertex_shaders[9].shader);

    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive);
    }
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x51dc50) -- see `python tools/pack.py 0x51dc50` for the full
body; this rewrite is a low-confidence structural sketch, see file header.
#endif
