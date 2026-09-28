// rasterizer_object_shadow_begin  (Ghidra: FUN_00530ff0; the phase 3 rewrite called it
//   rasterizer_motion_sensor_hud_state_begin)
// address 0x530ff0, size 850 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: raw disassembly (phase 4 review). Only caller is 0x50f830 in the object shadow code
//   (next to shadow_cache_get_or_allocate_entry 0x50f150), which builds a matrix4x3 from a
//   forward / up pair, lerps a colour, and calls this with EAX -> matrix, EBX -> colour and the
//   stack pair (radius, out). The phase 3 file read the register arguments as a camera basis and
//   an out flag and invented a contact-list state; none of that is in the code.
// What it does: when the 3D window is active and object shadows are on, sets up the silhouette
//   pass: CCW culling, rgb writes, alpha test at 0x7f, no z, no fog, stage 0 outputs texture
//   alpha replicated as colour (COLORARG1 0x22 = TEXTURE | ALPHAREPLICATE), stage 1 disabled, textures 0..3
//   cleared; uploads the shadow projection (forward and left rows scaled by 1/radius, with
//   the position folded into w, then {0,0,0,0.5}, {0,0,0,1}, {0,0,0,0}) as c13..c17; binds
//   render target 3 with a full-surface viewport and clears it (to 0x88888888 when the debug
//   toggle 0x0068941f is on, else black); selects shader stage config 0; then records matrix,
//   colour and radius for rasterizer_object_shadow_structure_draw 0x531570 and resets the
//   per-shadow flags. Writes radius to *out (0 when shadows are off). Always returns 1.
// register convention: EAX -> projection, EBX -> color, stack -> (radius, out_radius).
// blam-cc: EAX -> projection, EBX -> color, stack -> (radius, out_radius)
// UNSURE: 0x0069e550 is also the end bound of the vertex shader table loops; here it is a
//   byte flag set once the window target is restored in 0x531570.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern void *rasterizer_device;                             // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220
extern uint8_t rasterizer_caps_flag_689;                            // 0x0069c689
extern uint8_t console_debug_toggle_6893f2;                 // 0x006893f2 object shadows enabled
extern uint8_t console_debug_toggle_68941f;                 // 0x0068941f grey shadow target clear
extern int16_t rasterizer_active_render_target;             // 0x0069d350
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern ColorRGB rasterizer_object_shadow_color;             // 0x006e1cc0
extern float rasterizer_object_shadow_radius;               // 0x006e1ccc
extern real_matrix4x3 rasterizer_object_shadow_projection;  // 0x006e1cd0
extern rasterizer_model_draw_context *rasterizer_object_shadow_model_context; // 0x0071d260
extern uint8_t rasterizer_object_shadow_prepared;           // 0x0071d264
extern uint8_t rasterizer_object_shadow_model_active;       // 0x0071d265
extern uint8_t rasterizer_object_shadow_window_restored;    // 0x0069e550

// blam-cc: AX -> mode
extern void rasterizer_set_shader_stage_config(int16_t mode); // 0x519200

typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3d_get_desc_fn)(void *surface, d3d_surface_desc *desc);
typedef int32_t (__stdcall *d3d_set_viewport_fn)(void *self, const d3d_viewport *viewport);
typedef int32_t (__stdcall *d3d_clear_fn)(void *self, uint32_t count, const void *rects, uint32_t flags, uint32_t color, float z,
                                uint32_t stencil);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}

uint8_t rasterizer_object_shadow_begin(const real_matrix4x3 *projection, const ColorRGB *color, float radius,
                                       float *out_radius)
{
    float constants[5][4];
    d3d_surface_desc desc;
    d3d_viewport viewport;
    uint32_t clear_color;
    void *surface;
    float scale;
    uint32_t stage;

    if (rasterizer_window.type != 1) {
        return 1;
    }
    if (rasterizer_caps_flag_689 != 0 || console_debug_toggle_6893f2 == 0) {
        if (out_radius != NULL) {
            *out_radius = 0.0f;
        }
        return 1;
    }
    set_render_state(0x16, 3);           // CULLMODE CCW
    set_render_state(0xa8, 7);           // COLORWRITEENABLE rgb
    set_render_state(0x1b, 0);           // ALPHABLENDENABLE
    set_render_state(0x0f, 1);           // ALPHATESTENABLE
    set_render_state(0x18, 0x7f);        // ALPHAREF
    set_render_state(0x07, 0);           // ZENABLE
    set_render_state(0x1c, 0);           // FOGENABLE
    set_texture_stage_state(0, 1, 2);    // COLOROP SELECTARG1
    set_texture_stage_state(0, 2, 0x22); // COLORARG1 texture | alpha replicate
    set_texture_stage_state(0, 4, 1);    // ALPHAOP DISABLE
    set_texture_stage_state(1, 1, 1);    // COLOROP DISABLE
    set_texture_stage_state(1, 4, 1);    // ALPHAOP DISABLE
    for (stage = 0; stage < 4; stage++) {
        ((d3d_call2_fn)device_vtable()[0x104 / 4])(rasterizer_device, stage, 0);
    }

    scale = 1.0f / radius;
    constants[0][0] = scale * projection->forward.i;
    constants[0][1] = scale * projection->forward.j;
    constants[0][2] = scale * projection->forward.k;
    constants[0][3] = -((projection->position.x * projection->forward.i + projection->position.y * projection->forward.j +
                         projection->position.z * projection->forward.k) * scale);
    constants[1][0] = scale * projection->left.i;
    constants[1][1] = scale * projection->left.j;
    constants[1][2] = scale * projection->left.k;
    constants[1][3] = -((projection->position.x * projection->left.i + projection->position.y * projection->left.j +
                         projection->position.z * projection->left.k) * scale);
    constants[2][0] = 0.0f; constants[2][1] = 0.0f; constants[2][2] = 0.0f; constants[2][3] = 0.5f;
    constants[3][0] = 0.0f; constants[3][1] = 0.0f; constants[3][2] = 0.0f; constants[3][3] = 1.0f;
    constants[4][0] = 0.0f; constants[4][1] = 0.0f; constants[4][2] = 0.0f; constants[4][3] = 0.0f;
    ((d3d_set_constant_f_fn)device_vtable()[0x178 / 4])(rasterizer_device, 0xd, &constants[0][0], 5);

    clear_color = console_debug_toggle_68941f ? 0x88888888 : 0;
    surface = (void *)(uintptr_t)rasterizer_render_targets[3].surface;
    ((d3d_call2_fn)device_vtable()[0x94 / 4])(rasterizer_device, 0, (uint32_t)(uintptr_t)surface);
    rasterizer_active_render_target = 3;
    ((d3d_get_desc_fn)(*(void ***)surface)[0x30 / 4])(surface, &desc);
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = desc.width;
    viewport.height = desc.height;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    ((d3d_set_viewport_fn)device_vtable()[0xbc / 4])(rasterizer_device, &viewport);
    ((d3d_clear_fn)device_vtable()[0xac / 4])(rasterizer_device, 0, NULL, 1, clear_color, 1.0f, 0); // TARGET
    rasterizer_set_shader_stage_config(0);

    rasterizer_object_shadow_projection = *projection;  // rep movsd, 13 dwords
    rasterizer_object_shadow_color = *color;
    rasterizer_object_shadow_radius = radius;
    if (out_radius != NULL) {
        *out_radius = radius;
    }
    rasterizer_object_shadow_model_context = NULL;
    rasterizer_object_shadow_prepared = 0;
    rasterizer_object_shadow_model_active = 0;
    rasterizer_object_shadow_window_restored = 0;
    return 1;
}

#if 0
Original Ghidra decompilation (0x530ff0): phase 3 file rasterizer_motion_sensor_hud_state_begin replaced

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00530ff0(undefined4 param_1,undefined4 *param_2)

{
  float fVar1;
  int *piVar2;
  undefined4 *in_EAX;
  undefined4 *unaff_EBX;
  int iVar3;
  undefined4 *puVar4;
  bool bVar5;
  undefined4 uStack_150;
  int *piStack_14c;
  int *piStack_148;
  int *piStack_144;
  float *pfStack_140;
  undefined4 uStack_13c;
  int *piStack_138;
  undefined4 uStack_134;
  undefined4 uStack_130;
  undefined4 uStack_12c;
  int *piStack_128;
  undefined4 uStack_124;
  undefined4 uStack_120;
  undefined4 uStack_11c;
  int *piStack_118;
  float fStack_114;
  float fStack_110;
  float fStack_10c;
  int *piStack_108;
  float fStack_104;
  float fStack_100;
  float fStack_fc;
  int *piStack_f8;
  undefined4 uStack_f4;
  undefined4 uStack_f0;
  undefined4 uStack_ec;
  int *piStack_e8;
  undefined4 *puStack_e4;
  undefined4 uStack_e0;
  int *piStack_dc;
  undefined4 uStack_d8;
  undefined4 uStack_d4;
  int *piStack_d0;
  undefined4 uStack_cc;
  undefined4 uStack_c8;
  int *piStack_c4;
  undefined4 uStack_c0;
  undefined4 uStack_bc;
  int *piStack_b8;
  undefined4 uStack_b4;
  undefined4 uStack_b0;
  int *piStack_ac;
  undefined4 uStack_a8;
  undefined4 uStack_a4;
  int *piStack_a0;
  undefined4 uStack_9c;
  undefined4 uStack_98;
  
  if ((short)DAT_007c1220 == 1) {
    if ((DAT_0069c689 == '\0') && (DAT_006893f2 != '\0')) {
      uStack_98 = 3;
      uStack_9c = 0x16;
      piStack_a0 = DAT_0071d174;
      uStack_a4 = 0x531035;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_a4 = 7;
      uStack_a8 = 0xa8;
      piStack_ac = DAT_0071d174;
      uStack_b0 = 0x53104a;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_b0 = 0;
      uStack_b4 = 0x1b;
      piStack_b8 = DAT_0071d174;
      uStack_bc = 0x53105c;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_bc = 1;
      uStack_c0 = 0xf;
      piStack_c4 = DAT_0071d174;
      uStack_c8 = 0x53106e;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_c8 = 0x7f;
      uStack_cc = 0x18;
      piStack_d0 = DAT_0071d174;
      uStack_d4 = 0x531080;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_d4 = 0;
      uStack_d8 = 7;
      piStack_dc = DAT_0071d174;
      uStack_e0 = 0x531092;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_e0 = 0;
      puStack_e4 = (undefined4 *)0x1c;
      piStack_e8 = DAT_0071d174;
      uStack_ec = 0x5310a4;
      (**(code **)(*DAT_0071d174 + 0xe4))();
      uStack_ec = 2;
      uStack_f0 = 1;
      uStack_f4 = 0;
      piStack_f8 = DAT_0071d174;
      fStack_fc = 7.628344e-39;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      fStack_fc = 4.76441e-44;
      fStack_100 = 2.8026e-45;
      fStack_104 = 0.0;
      piStack_108 = DAT_0071d174;
      fStack_10c = 7.628372e-39;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      fStack_10c = 1.4013e-45;
      fStack_110 = 5.60519e-45;
      fStack_114 = 0.0;
      piStack_118 = DAT_0071d174;
      uStack_11c = 0x5310e0;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_11c = 1;
      uStack_120 = 1;
      uStack_124 = 1;
      piStack_128 = DAT_0071d174;
      uStack_12c = 0x5310f4;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      uStack_12c = 1;
      uStack_130 = 4;
      uStack_134 = 1;
      piStack_138 = DAT_0071d174;
      uStack_13c = 0x531108;
      (**(code **)(*DAT_0071d174 + 0x10c))();
      iVar3 = 0;
      do {
        uStack_13c = 0;
        piStack_144 = DAT_0071d174;
        piStack_148 = (int *)0x531121;
        pfStack_140 = (float *)iVar3;
        (**(code **)(*DAT_0071d174 + 0x104))();
        iVar3 = iVar3 + 1;
      } while (iVar3 < 4);
      fVar1 = 1.0 / (float)piStack_a0;
      uStack_13c = 5;
      pfStack_140 = &fStack_114;
      piStack_144 = (int *)0xd;
      uStack_f4 = 0;
      uStack_f0 = 0;
      uStack_ec = 0;
      piStack_e8 = (int *)0x3f000000;
      puStack_e4 = (undefined4 *)0x0;
      uStack_e0 = 0;
      piStack_dc = (int *)0x0;
      uStack_d8 = 0x3f800000;
      uStack_d4 = 0;
      piStack_d0 = (int *)0x0;
      uStack_cc = 0;
      uStack_c8 = 0;
      piStack_148 = DAT_0071d174;
      fStack_114 = fVar1 * (float)in_EAX[1];
      fStack_110 = fVar1 * (float)in_EAX[2];
      fStack_10c = fVar1 * (float)in_EAX[3];
      piStack_108 = (int *)-(((float)in_EAX[10] * (float)in_EAX[1] +
                             (float)in_EAX[0xb] * (float)in_EAX[2] +
                             (float)in_EAX[0xc] * (float)in_EAX[3]) * fVar1);
      fStack_104 = fVar1 * (float)in_EAX[4];
      fStack_100 = fVar1 * (float)in_EAX[5];
      fStack_fc = fVar1 * (float)in_EAX[6];
      piStack_f8 = (int *)-(((float)in_EAX[10] * (float)in_EAX[4] +
                            (float)in_EAX[0xb] * (float)in_EAX[5] +
                            (float)in_EAX[0xc] * (float)in_EAX[6]) * fVar1);
      piStack_14c = (int *)0x53121f;
      (**(code **)(*DAT_0071d174 + 0x178))();
      piVar2 = DAT_0069d3a0;
      bVar5 = DAT_0068941f != '\0';
      piStack_14c = DAT_0069d3a0;
      uStack_150 = 0;
      (**(code **)(*DAT_0071d174 + 0x94))(DAT_0071d174);
      _DAT_0069d350 = 3;
      (**(code **)(*piVar2 + 0x30))(piVar2,&uStack_e0);
      piStack_144 = (int *)uStack_cc;
      uStack_150 = 0;
      piStack_14c = (int *)0x0;
      piStack_148 = piStack_d0;
      pfStack_140 = (float *)0x0;
      uStack_13c = 0x3f800000;
      (**(code **)(*DAT_0071d174 + 0xbc))(DAT_0071d174,&uStack_150);
      (**(code **)(*DAT_0071d174 + 0xac))(DAT_0071d174,0,0,1,-(uint)bVar5 & 0x88888888,0x3f800000,0)
      ;
      rasterizer_set_shader_stage_config();
      puVar4 = &DAT_006e1cd0;
      for (iVar3 = 0xd; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar4 = *in_EAX;
        in_EAX = in_EAX + 1;
        puVar4 = puVar4 + 1;
      }
      _DAT_006e1cc4 = unaff_EBX[1];
      _DAT_006e1cc0 = *unaff_EBX;
      _DAT_006e1cc8 = unaff_EBX[2];
      _DAT_006e1ccc = piStack_e8;
      if (puStack_e4 != (undefined4 *)0x0) {
        *puStack_e4 = piStack_e8;
      }
      DAT_0071d260 = 0;
      DAT_0071d264 = 0;
      DAT_0071d265 = 0;
      DAT_0069e550 = 0;
      return 1;
    }
    if (param_2 != (undefined4 *)0x0) {
      *param_2 = 0;
    }
  }
  return 1;
}
#endif
