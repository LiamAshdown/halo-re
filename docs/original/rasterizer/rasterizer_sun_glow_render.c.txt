// rasterizer_sun_glow_render  (Ghidra: FUN_00525ab0, unnamed; the earlier draft called it
//   rasterizer_light_shadow_render)
// address 0x525ab0, size 3148 bytes
// VERIFIED against disassembly 0x525ab0..0x5266fc (2026-09-30): cone falloff, half-pixel screen constants, projection and rect, both alpha quads, the blur helpers and the 16 growing quads through effect 77
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: its only caller is lens_flare_render_all 0x513cf0 (0x514535), once per visible lens
//   flare instance of the current window whose LensFlare has occlusion_radius 50 or flags bit 0
//   (sun) set, with the instance in EAX. Rebuilt from the raw disassembly (Ghidra lost EAX, the
//   stack argument of 0x525320, every device call argument and the x87 stack of the cone
//   falloff and of the sixteen quad loop).
// What it does (pixel shader cards only): measures how far the flare lies inside a 45 degree cone
//   around the view direction, projects the point one occlusion radius along the flare direction
//   to the screen, clears destination alpha in a 64 x 64 pixel box around it, writes the
//   GlobalsRasterizerData glow bitmap into destination alpha depth tested at the projected depth
//   (so only unoccluded pixels keep alpha), blurs that alpha through render targets 6 and 7
//   (0x525320 twice, 0x525720), and adds sixteen growing quads of the blurred result through
//   effect 77, each at 1 / (n + 1) of the cone falloff.
// register convention: EAX -> instance.
// blam-cc: EAX -> instance
// NOTE: the three helpers keep the light_shadow names the earlier pass gave them; their role
//   here is a blur of the sun visibility mask.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_device;                             // 0x0071d174
extern d3d_caps9 rasterizer_caps;                           // 0x007c10c0
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern GlobalsRasterizerData *rasterizer_globals_data;      // 0x0071d164
extern uint8_t rasterizer_software_vertex_processing;       // 0x0069c680
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern rasterizer_dynamic_screen_vertex rasterizer_shadow_screen_quad[4]; // 0x006e1720 static quad (also used by 0x525320)

// blam-cc: ECX -> v, returns the length in ST0
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
// blam-cc: EAX -> out, ECX -> packed
extern real_vector3d *vector3d_unpack_normal_11_11_10(real_vector3d *out, uint32_t packed); // 0x513400
extern uint8_t rasterizer_sun_glow_project_point(real_point3d *point, float radius, float *out_screen,
                                                      float *out_scale); // 0x525130
// blam-cc: ESI -> rect (left, right, top, bottom), stack -> target_index
extern void rasterizer_sun_glow_capture(const float *rect, int16_t target_index); // 0x525320, ESI rect, stack target
extern int32_t rasterizer_sun_glow_blur(int16_t first_target_index, int16_t second_target_index,
                                                               uint16_t both_flag); // 0x525720
// blam-cc: AX -> target_index, DX -> stage
extern void *rasterizer_render_target_bind_texture_stage(int16_t target_index, int16_t stage); // 0x52cdd0
// blam-cc: EAX -> bitmap_tag_id, stack -> (stage, frame)
extern uint8_t chimera__rasterizer_set_texture_direct_d3d9(uint32_t bitmap_tag_id, int16_t stage, int16_t frame); // 0x518770
extern double floor(double x); // 0x623e40 CRT
extern double cos(double x);   // inline x87 fcos

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3d_draw_primitive_up_fn)(void *self, uint32_t type, uint32_t count, const void *data, uint32_t stride);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

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
static void set_clamped_linear_sampler(uint32_t sampler)
{
    set_sampler_state(sampler, 1, 3);  // ADDRESSU CLAMP
    set_sampler_state(sampler, 2, 3);  // ADDRESSV CLAMP
    set_sampler_state(sampler, 5, 2);  // LINEAR
    set_sampler_state(sampler, 6, 2);
    set_sampler_state(sampler, 7, 2);
}
static void draw_quad(void)
{
    ((d3d_draw_primitive_up_fn)device_vtable()[0x14c / 4])(rasterizer_device, 6, 2, rasterizer_shadow_screen_quad,
                                                           sizeof(rasterizer_dynamic_screen_vertex));
}
static void set_quad_position(float left, float top, float right, float bottom, float z)
{
    rasterizer_shadow_screen_quad[0].x = left;
    rasterizer_shadow_screen_quad[0].y = top;
    rasterizer_shadow_screen_quad[1].x = right;
    rasterizer_shadow_screen_quad[1].y = top;
    rasterizer_shadow_screen_quad[2].x = right;
    rasterizer_shadow_screen_quad[2].y = bottom;
    rasterizer_shadow_screen_quad[3].x = left;
    rasterizer_shadow_screen_quad[3].y = bottom;
    rasterizer_shadow_screen_quad[0].z = z;
    rasterizer_shadow_screen_quad[1].z = z;
    rasterizer_shadow_screen_quad[2].z = z;
    rasterizer_shadow_screen_quad[3].z = z;
}
static void set_quad_color_and_uv(uint32_t color)
{
    rasterizer_shadow_screen_quad[0].color = color;
    rasterizer_shadow_screen_quad[0].u = 0.0f;
    rasterizer_shadow_screen_quad[0].v = 0.0f;
    rasterizer_shadow_screen_quad[1].color = color;
    rasterizer_shadow_screen_quad[1].u = 1.0f;
    rasterizer_shadow_screen_quad[1].v = 0.0f;
    rasterizer_shadow_screen_quad[2].color = color;
    rasterizer_shadow_screen_quad[2].u = 1.0f;
    rasterizer_shadow_screen_quad[2].v = 1.0f;
    rasterizer_shadow_screen_quad[3].color = color;
    rasterizer_shadow_screen_quad[3].u = 0.0f;
    rasterizer_shadow_screen_quad[3].v = 1.0f;
}

// c13..c17: pixels of the render window to clip space (half pixel offset), then c15..c17 as given.
static void set_screen_constants(float c15_z, float c15_w, float c16_w, float c17_x, float c17_y)
{
    float constants[5][4];
    float inverse_width = 1.0f / (float)(int16_t)(rasterizer_window.camera.viewport_bounds.right -
                                                  rasterizer_window.camera.viewport_bounds.left);
    float inverse_height = 1.0f / (float)(int16_t)(rasterizer_window.camera.viewport_bounds.bottom -
                                                   rasterizer_window.camera.viewport_bounds.top);

    constants[0][0] = inverse_width + inverse_width;
    constants[0][1] = 0.0f;
    constants[0][2] = 0.0f;
    constants[0][3] = -1.0f - inverse_width;
    constants[1][0] = 0.0f;
    constants[1][1] = -2.0f * inverse_height;
    constants[1][2] = 0.0f;
    constants[1][3] = inverse_height + 1.0f;
    constants[2][0] = 0.0f;
    constants[2][1] = 0.0f;
    constants[2][2] = c15_z;
    constants[2][3] = c15_w;
    constants[3][0] = 0.0f;
    constants[3][1] = 0.0f;
    constants[3][2] = 0.0f;
    constants[3][3] = c16_w;
    constants[4][0] = c17_x;
    constants[4][1] = c17_y;
    constants[4][2] = 0.0f;
    constants[4][3] = 1.0f;
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &constants[0][0], 5);
}

static void set_screen_vertex_states(void)
{
    ((d3d_call1_fn)device_vtable()[0x15c / 4])(rasterizer_device, rasterizer_vertex_declarations[6].declaration);
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device,
                                               ((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[6].usage) & 0x10);
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, rasterizer_vertex_shaders[24].shader);
}

void rasterizer_sun_glow_render(lens_flare_instance *instance)
{
    real_vector3d to_flare;
    real_vector3d direction;
    real_point3d point;
    float screen[3];                  // x, y in pixels, z depth
    float scale;
    float rect[4];                    // left, right, top, bottom
    float falloff;
    float cone_cosine;
    float radius;
    void *effect;
    int32_t target;
    int32_t quad;

    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        return;
    }
    set_clamped_linear_sampler(0);

    // how far inside a 45 degree cone around the view direction the flare lies
    to_flare.i = instance->position.x - rasterizer_window.camera.position.x;
    to_flare.j = instance->position.y - rasterizer_window.camera.position.y;
    to_flare.k = instance->position.z - rasterizer_window.camera.position.z;
    vector3d_normalize_with_length(&to_flare);
    cone_cosine = (float)cos(0.7853981852531433);
    falloff = (to_flare.k * rasterizer_window.camera.forward.k + to_flare.j * rasterizer_window.camera.forward.j +
               to_flare.i * rasterizer_window.camera.forward.i - cone_cosine) / (1.0f - cone_cosine);
    if (falloff < 0.0f) {
        falloff = 0.0f;
    } else if (falloff > 1.0f) {
        falloff = 1.0f;
    }
    set_screen_constants(1.0f, 0.0f, 1.0f, 0.0f, 0.0f);

    // the point one occlusion radius out along the flare direction, on screen
    vector3d_unpack_normal_11_11_10(&direction, instance->packed_direction);
    radius = *(const float *)((const uint8_t *)(uintptr_t)instance->definition + 0x10); // LensFlare.occlusion_radius
    point.x = direction.i * radius + instance->position.x;
    point.y = direction.j * radius + instance->position.y;
    point.z = direction.k * radius + instance->position.z;
    if (!rasterizer_sun_glow_project_point(&point, radius, screen, &scale)) {
        ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
        return;
    }
    screen[0] = (float)floor(screen[0] + 0.5f);
    screen[1] = (float)floor(screen[1] + 0.5f);
    rect[0] = screen[0] - 32.0f;
    rect[2] = screen[1] - 32.0f;
    rect[1] = screen[0] + 32.0f;
    rect[3] = screen[1] + 32.0f;

    set_clamped_linear_sampler(1);
    set_clamped_linear_sampler(2);
    set_clamped_linear_sampler(3);
    set_screen_vertex_states();
    ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);  // SetPixelShader(NULL)

    // clear destination alpha in the box
    set_render_state(0x16, 3);
    set_render_state(0xa8, 8);        // COLORWRITEENABLE alpha only
    set_render_state(0x1b, 0);
    set_render_state(0x0f, 0);
    set_render_state(0x07, 0);
    set_render_state(0x1c, 0);
    set_quad_position(rect[0], rect[2], rect[1], rect[3], 0.0f);
    set_render_state(0x3c, 0);        // TEXTUREFACTOR 0
    set_texture_stage_state(0, 1, 1); // COLOROP DISABLE
    set_texture_stage_state(0, 4, 2); // ALPHAOP SELECTARG1
    set_texture_stage_state(0, 5, 3); // ALPHAARG1 TFACTOR
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);
    draw_quad();

    // write the glow bitmap into alpha where the flare depth is visible
    chimera__rasterizer_set_texture_direct_d3d9(*(uint32_t *)((uint8_t *)rasterizer_globals_data + 0x6c), 0, 0); // glow
    set_render_state(0x16, 3);
    set_render_state(0xa8, 8);
    set_render_state(0x1b, 0);
    set_render_state(0x0f, 0);
    set_render_state(0x07, 1);        // ZENABLE
    set_render_state(0x17, 4);        // ZFUNC LESSEQUAL
    set_render_state(0x0e, 0);        // ZWRITEENABLE off
    set_render_state(0x1c, 0);
    set_quad_position(rect[0], rect[2], rect[1], rect[3], screen[2]);
    set_quad_color_and_uv(0xffffffff);
    set_render_state(0x3c, 0xffffffff);
    set_texture_stage_state(0, 1, 2);    // COLOROP SELECTARG1
    set_texture_stage_state(0, 2, 0x22); // COLORARG1 TEXTURE | ALPHAREPLICATE
    set_texture_stage_state(0, 4, 6);    // ALPHAOP MODULATE4X
    set_texture_stage_state(0, 5, 2);    // ALPHAARG1 TEXTURE
    set_texture_stage_state(0, 6, 3);    // ALPHAARG2 TFACTOR
    set_texture_stage_state(1, 1, 1);
    set_texture_stage_state(1, 4, 1);
    draw_quad();

    effect = (void *)(uintptr_t)rasterizer_effects[77].effect;
    if (effect != NULL) {
        uint32_t passes;
        uint32_t pass;

        // blur the visibility alpha through targets 6 and 7
        rasterizer_sun_glow_capture(rect, 6);
        rasterizer_sun_glow_capture(rect, 7);
        target = rasterizer_sun_glow_blur(6, 7, 4);
        set_screen_constants(0.0f, 0.5f, 1.0f, 1.0f, 1.0f);
        set_screen_vertex_states();
        rasterizer_render_target_bind_texture_stage((int16_t)target, 0);
        set_sampler_state(0, 7, 2);
        set_render_state(0x16, 3);
        set_render_state(0xa8, 7);
        set_render_state(0x1b, 1);
        set_render_state(0x13, 5);    // SRCBLEND SRCALPHA
        set_render_state(0x14, 2);    // DESTBLEND ONE
        set_render_state(0xab, 1);
        set_render_state(0x0f, 0);
        set_render_state(0x07, 0);
        set_render_state(0x1c, 0);

        // sixteen quads growing by 5 pixels, each 1 / (n + 1) as bright
        for (quad = 0; quad < 16; quad++) {
            float grow = (float)quad * 0.0625f * 80.0f - 4.0f;
            uint32_t alpha = (uint32_t)(int32_t)(falloff / (float)(quad + 1) * 255.0f);  // __ftol

            set_quad_color_and_uv((alpha << 24) | 0x00ffffff);
            set_quad_position(rect[0] - grow, rect[2] - grow, rect[1] + grow, rect[3] + grow, 0.0f);
            effect = (void *)(uintptr_t)rasterizer_effects[77].effect;
            ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &passes, 3);
            for (pass = 0; pass < passes; pass++) {
                effect = (void *)(uintptr_t)rasterizer_effects[77].effect;
                ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
                draw_quad();
            }
            effect = (void *)(uintptr_t)rasterizer_effects[77].effect;
            ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
        }
    }
    ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
}

#if 0
Original Ghidra decompilation (0x525ab0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00525ab0(void)

{
  char cVar1;
  int *in_EAX;
  float *pfVar2;
  int iVar3;
  int *piVar4;
  int iVar5;
  float10 fVar6;
  float fVar7;
  undefined4 uVar8;
  int *piVar9;
  int *piVar10;
  undefined4 uStack_d8;
  int *piStack_d4;
  undefined4 uStack_d0;
  float fStack_cc;
  float fStack_c8;
  int *piStack_c4;
  float fStack_c0;
  float fStack_bc;
  float fStack_b8;
  int *piStack_b4;
  float afStack_b0 [3];
  int *piStack_a4;
  float fStack_a0;
  float fStack_9c;
  undefined4 uStack_98;
  int *piStack_94;
  undefined4 uStack_90;
  float fStack_8c;
  undefined4 uStack_88;
  
  if (0xffff0100 < DAT_007c118c) {
    uStack_88 = 3;
    fStack_8c = 1.4013e-45;
    uStack_90 = 0;
    piStack_94 = DAT_0071d174;
    uStack_98 = 0x525add;
    (**(code **)(*DAT_0071d174 + 0x114))();
    uStack_98 = 3;
    fStack_9c = 2.8026e-45;
    fStack_a0 = 0.0;
    piStack_a4 = DAT_0071d174;
    afStack_b0[2] = 7.563134e-39;
    (**(code **)(*DAT_0071d174 + 0x114))();
    afStack_b0[2] = 2.8026e-45;
    afStack_b0[1] = 7.00649e-45;
    afStack_b0[0] = 0.0;
    piStack_b4 = DAT_0071d174;
    fStack_b8 = 7.563162e-39;
    (**(code **)(*DAT_0071d174 + 0x114))();
    fStack_b8 = 2.8026e-45;
    fStack_bc = 8.40779e-45;
    fStack_c0 = 0.0;
    piStack_c4 = DAT_0071d174;
    fStack_c8 = 7.56319e-39;
    (**(code **)(*DAT_0071d174 + 0x114))();
    fStack_c8 = 2.8026e-45;
    fStack_cc = 9.80909e-45;
    uStack_d0 = 0;
    piStack_d4 = DAT_0071d174;
    uStack_d8 = 0x525b2d;
    (**(code **)(*DAT_0071d174 + 0x114))();
    fStack_bc = (float)in_EAX[1] - DAT_007c1228;
    fStack_b8 = (float)in_EAX[2] - DAT_007c122c;
    piStack_b4 = (int *)((float)in_EAX[3] - DAT_007c1230);
    uStack_d8 = 0x525b5d;
    vector3d_normalize_with_length();
    fVar6 = (float10)fcos((float10)0.7853981852531433);
    fVar6 = (((float10)fStack_bc * (float10)DAT_007c1234 +
             (float10)fStack_b8 * (float10)DAT_007c1238 +
             (float10)(float)piStack_b4 * (float10)DAT_007c123c) - fVar6) / ((float10)1.0 - fVar6);
    if ((float10)0.0 <= fVar6) {
      if (fVar6 <= (float10)1.0) {
        fStack_cc = (float)fVar6;
      }
      else {
        fStack_cc = 1.0;
      }
    }
    else {
      fStack_cc = 0.0;
    }
    fStack_c8 = (float)(int)(short)((short)DAT_007c1258 - (short)DAT_007c1254);
    fVar7 = 1.0 / (float)(int)(short)(DAT_007c1258._2_2_ - DAT_007c1254._2_2_);
    uStack_d8 = 5;
    fStack_9c = 0.0;
    uStack_98 = 0;
    uStack_90 = 0;
    uStack_88 = 0;
    fStack_a0 = fVar7 + fVar7;
    piStack_94 = (int *)(-1.0 - fVar7);
    fStack_8c = (1.0 / (float)(int)fStack_c8) * -2.0;
    piVar9 = DAT_0071d174;
    (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,&fStack_a0);
    pfVar2 = (float *)vector3d_unpack_normal_11_11_10();
    fStack_cc = *pfVar2;
    fStack_c8 = pfVar2[1];
    piStack_c4 = (int *)pfVar2[2];
    fVar7 = *(float *)(*in_EAX + 0x10);
    fStack_c0 = fStack_cc * fVar7 + (float)in_EAX[1];
    fStack_bc = fStack_c8 * fVar7 + (float)in_EAX[2];
    fStack_b8 = (float)piStack_c4 * fVar7 + (float)in_EAX[3];
    cVar1 = FUN_00525130(&fStack_c0,*(undefined4 *)(*in_EAX + 0x10),&fStack_cc,&piStack_d4);
    if (cVar1 != '\0') {
      fVar6 = (float10)FUN_00623e40((double)(fStack_cc + 0.5));
      fStack_cc = (float)fVar6;
      fVar6 = (float10)FUN_00623e40((double)(fStack_c8 + 0.5));
      fStack_c0 = fStack_cc - 32.0;
      fStack_b8 = (float)(fVar6 - (float10)32.0);
      fStack_bc = fStack_cc + 32.0;
      piStack_b4 = (int *)(float)(fVar6 + (float10)32.0);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,1,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,2,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,6,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,1,7,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,1,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,2,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,6,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,2,7,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,1,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,2,3);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,5,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,6,2);
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,3,7,2);
      (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,DAT_006e1ad8);
      (**(code **)(*DAT_0071d174 + 0x134))
                (DAT_0071d174,-(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1ae0 & 0x10);
      (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,DAT_0069e410);
      (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
      uVar8 = 0xa8;
      piVar10 = DAT_0071d174;
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,8);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,0);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
      _DAT_006e1754 = (float)uVar8;
      _DAT_006e1724 = piVar10;
      _DAT_006e1720 = 3.78351e-44;
      _DAT_006e1738 = 0.0;
      _DAT_006e1750 = 0.0;
      _DAT_006e1768 = 3.78351e-44;
      _DAT_006e1770 = (int *)0x0;
      _DAT_006e1758 = (int *)0x0;
      _DAT_006e1740 = (int *)0x0;
      _DAT_006e1728 = (int *)0x0;
      _DAT_006e173c = _DAT_006e1724;
      _DAT_006e176c = _DAT_006e1754;
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3c,0);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,3);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
      (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&DAT_006e1720,0x18);
      chimera__rasterizer_set_texture_direct_d3d9(0,0);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,8);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,0);
      fVar7 = 2.10195e-44;
      piVar4 = DAT_0071d174;
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
      piVar10 = DAT_0071d174;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x17,4);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xe,0);
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
      _DAT_006e1754 = fVar7;
      _DAT_006e1724 = piVar4;
      _DAT_006e172c = 0xffffffff;
      _DAT_006e1730 = 0;
      _DAT_006e1734 = 0;
      _DAT_006e1720 = 9.80909e-45;
      _DAT_006e1728 = piVar10;
      _DAT_006e1738 = 1.4013e-45;
      _DAT_006e1740 = piVar10;
      _DAT_006e1750 = 1.4013e-45;
      _DAT_006e1758 = piVar10;
      _DAT_006e1768 = 9.80909e-45;
      _DAT_006e1744 = 0xffffffff;
      _DAT_006e1748 = 0x3f800000;
      _DAT_006e174c = 0;
      _DAT_006e175c = 0xffffffff;
      _DAT_006e1760 = 0x3f800000;
      _DAT_006e1764 = 0x3f800000;
      _DAT_006e1774 = 0xffffffff;
      _DAT_006e1778 = 0;
      _DAT_006e177c = 0x3f800000;
      _DAT_006e1770 = piVar10;
      _DAT_006e173c = _DAT_006e1724;
      _DAT_006e176c = _DAT_006e1754;
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3c,0xffffffff);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,0x22);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,6);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,6,3);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
      (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&DAT_006e1720,0x18);
      if (DAT_0069ddb0 != (int *)0x0) {
        FUN_00525320(6);
        FUN_00525320(7);
        FUN_00525720(6,7,4);
        afStack_b0[1] = 0.0;
        fVar7 = 1.0 / (float)(int)(short)(DAT_007c1258._2_2_ - DAT_007c1254._2_2_);
        afStack_b0[2] = 0.0;
        fStack_a0 = 0.0;
        uStack_98 = 0;
        uStack_90 = 0;
        fStack_8c = 0.0;
        uStack_88 = 0;
        afStack_b0[0] = fVar7 + fVar7;
        piStack_a4 = (int *)(-1.0 - fVar7);
        fVar7 = 1.0 / (float)(int)(short)((short)DAT_007c1258 - (short)DAT_007c1254);
        fStack_9c = fVar7 * -2.0;
        piStack_94 = (int *)(fVar7 + 1.0);
        (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,afStack_b0,5);
        (**(code **)(*DAT_0071d174 + 0x15c))(DAT_0071d174,DAT_006e1ad8);
        (**(code **)(*DAT_0071d174 + 0x134))
                  (DAT_0071d174,-(uint)(DAT_0069c680 != '\0') & 0x10 | DAT_006e1ae0 & 0x10);
        (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,DAT_0069e410);
        FUN_0052cdd0();
        (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,2);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,7);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,5);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,2);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,0);
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
        piVar10 = (int *)0x0;
        iVar5 = 0x10;
        do {
          fVar7 = (float)(int)piVar10;
          piVar10 = (int *)((int)piVar10 + 1);
          fVar7 = fVar7 * 0.0625 * 80.0 - 4.0;
          piStack_d4 = piVar10;
          iVar3 = __ftol();
          _DAT_006e1720 = fStack_c0 - fVar7;
          _DAT_006e172c = iVar3 << 0x18 | 0xffffff;
          _DAT_006e1724 = (int *)(fStack_b8 - fVar7);
          _DAT_006e1738 = fVar7 + fStack_bc;
          _DAT_006e1730 = 0;
          _DAT_006e1734 = 0;
          _DAT_006e1748 = 0x3f800000;
          _DAT_006e174c = 0;
          _DAT_006e1760 = 0x3f800000;
          _DAT_006e1764 = 0x3f800000;
          _DAT_006e1778 = 0;
          _DAT_006e177c = 0x3f800000;
          _DAT_006e1770 = (int *)0x0;
          _DAT_006e1754 = fVar7 + (float)piStack_b4;
          _DAT_006e1758 = (int *)0x0;
          _DAT_006e1740 = (int *)0x0;
          _DAT_006e1728 = (int *)0x0;
          _DAT_006e173c = _DAT_006e1724;
          _DAT_006e1744 = _DAT_006e172c;
          _DAT_006e1750 = _DAT_006e1738;
          _DAT_006e175c = _DAT_006e172c;
          _DAT_006e1768 = _DAT_006e1720;
          _DAT_006e176c = _DAT_006e1754;
          _DAT_006e1774 = _DAT_006e172c;
          (**(code **)(*DAT_0069ddb0 + 0x100))(DAT_0069ddb0,&uStack_d8,3);
          piVar4 = (int *)0x0;
          if (piVar9 != (int *)0x0) {
            do {
              (**(code **)(*DAT_0069ddb0 + 0x104))(DAT_0069ddb0,piVar4);
              (**(code **)(*DAT_0071d174 + 0x14c))(DAT_0071d174,6,2,&DAT_006e1720,0x18);
              piVar4 = (int *)((int)piVar4 + 1);
            } while (piVar4 < piVar9);
          }
          (**(code **)(*DAT_0069ddb0 + 0x108))(DAT_0069ddb0);
          iVar5 = iVar5 + -1;
        } while (iVar5 != 0);
      }
    }
    (**(code **)(*DAT_0071d174 + 0x134))();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
