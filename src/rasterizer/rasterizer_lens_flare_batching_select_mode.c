// rasterizer_lens_flare_batching_select_mode  (Ghidra: FUN_00537130)
// address 0x537130, size 1054 bytes
// VERIFIED against disassembly 0x537130..0x53754e (2026-09-30): both modes: every render/texture-stage/sampler state, the effect begin/pass branch, and the batch key reset
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: functions.md summary ("Selects a rendering technique/mode (values 5 and 6 observed)
//   and, for mode 5, resets all per-frame state used by the screen-space sprite (lens-flare/decal)
//   batching system"). Ghidra's decompile of every SetRenderState/SetTextureStageState/
//   SetSamplerState call in this function is unusable (the same denormalized-float /
//   embedded-return-address corruption as rasterizer_screen_flash_render.c); this rewrite is
//   built directly from the raw disassembly (0x537130..0x53754d) instead, which also revealed
//   that mode 5's code is laid out physically before mode 6's despite Ghidra showing the "mode==5"
//   branch first (a pure reordering, not a semantic difference).
// register convention: AX -> mode, ECX -> flags (in_ECX; bit0 selects ZENABLE, bit1 selects
//   SHADEMODE, both only for mode 5).
// blam-cc: AX -> mode, ECX -> flags
// NOTE: `unknown_0069da10` is read once through a dead compare against the literal address
//   0x69da10 (always false, omitted here as in the other files in this range) before the real
//   null check; and 0x0071d278 (rasterizer_effect_pool_scratch, per lens_flare_render_all.c) is
//   set to `&unknown_0069da10` itself (a pointer to the variable, not its value), matching that
//   file's own "an effect/COM object being ended here" note -- the double indirection is
//   preserved exactly even though its purpose is not otherwise explained here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device; // 0x0071d174
extern uint8_t console_debug_toggle_689425; // 0x00689425
extern uint8_t lens_flare_occlusion_queries_supported; // 0x006e1dc0
extern void *unknown_0069da10; // 0x0069da10 an ID3DXEffect-shaped object, see file header
extern void *rasterizer_effect_pool_scratch; // 0x0071d278
extern d3d_caps9 rasterizer_caps; // 0x007c10c0
extern uint32_t lens_flare_batch_clock; // 0x00746fa8
extern lens_flare_batch_key lens_flare_current_key; // 0x00746fb0
extern lens_flare_batch_key lens_flare_applied_key; // 0x007bf040
extern uint32_t lens_flare_vertex_specular; // 0x0069e708

extern void rasterizer_set_shader_stage_config(int16_t mode); // 0x519200

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *pass_count, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);

static void **device_vtable(void) { return *(void ***)rasterizer_device; }
static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}
static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, stage, type, value);
}
static void set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, stage, type, value);
}

// Selects a rendering technique/mode (values 5 and 6 observed) for the screen-space sprite
// (lens-flare/decal) batching system and, for mode 5, resets all of its per-frame state.
void rasterizer_lens_flare_batching_select_mode(int16_t mode, uint32_t flags)
{
    if (mode == 6) {
        set_render_state(0x16, 3); // CULLMODE = CCW
        set_render_state(0xa8, (console_debug_toggle_689425 != 0) ? 7u : 0u); // COLORWRITEENABLE
        set_render_state(0x1b, 0); // ALPHABLENDENABLE = FALSE
        set_render_state(0xf, 0);  // ALPHATESTENABLE = FALSE
        set_render_state(0x7, 1);  // ZENABLE = TRUE
        set_render_state(0x17, 4); // ZFUNC = LESSEQUAL
        set_render_state(0xe, console_debug_toggle_689425); // SHADEMODE
        set_render_state(0x1c, 0); // FOGENABLE = FALSE
        set_render_state(0x3c, 0xffff0000); // TEXTUREFACTOR

        set_texture_stage_state(0, 1, 2);
        set_texture_stage_state(0, 2, 3);
        set_texture_stage_state(0, 4, 2);
        set_texture_stage_state(0, 5, 1);
        set_texture_stage_state(1, 1, 1);
        set_texture_stage_state(1, 4, 1);

        ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, 0); // SetVertexShader(NULL)
        ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0); // SetPixelShader(NULL)
        ((d3d_call1_fn)device_vtable()[0x164 / 4])(rasterizer_device, 0x144); // SetFVF
        return;
    }
    if (mode != 5) {
        return;
    }

    {
        uint32_t z_enable = (lens_flare_occlusion_queries_supported == 0) ? 1u : (flags & 1);

        set_render_state(0x16, 3);  // CULLMODE = CCW
        set_render_state(0xa8, 7);  // COLORWRITEENABLE = RGBA
        set_render_state(0x1b, 1);  // ALPHABLENDENABLE = TRUE
        set_render_state(0x13, 5);  // SRCBLEND = SRCALPHA
        set_render_state(0x14, 2);  // DESTBLEND
        set_render_state(0xab, 1);  // BLENDOP = ADD
        set_render_state(0xf, 0);   // ALPHATESTENABLE = FALSE
        set_render_state(0x7, z_enable); // ZENABLE
        set_render_state(0x17, 4);  // ZFUNC = LESSEQUAL
        set_render_state(0xe, (flags >> 1) & 1); // SHADEMODE
        set_render_state(0x1c, 0);  // FOGENABLE = FALSE
    }

    rasterizer_effect_pool_scratch = &unknown_0069da10; // see file header note
    if (unknown_0069da10 != 0) {
        void **effect_vt = *(void ***)unknown_0069da10;
        uint32_t pass_count = 0;
        ((d3dx_effect_begin_fn)effect_vt[0x100 / 4])(unknown_0069da10, &pass_count, 3);
        // Re-reads the effect through rasterizer_effect_pool_scratch, exactly as the original does.
        effect_vt = *(void ***)(*(void **)rasterizer_effect_pool_scratch);
        ((d3dx_effect_pass_fn)effect_vt[0x104 / 4])(*(void **)rasterizer_effect_pool_scratch, 0);
    } else {
        set_texture_stage_state(0, 1, 4);
        set_texture_stage_state(0, 2, 2);
        set_texture_stage_state(0, 3, 0);
        set_texture_stage_state(0, 4, 2);
        set_texture_stage_state(0, 5, 0); // 0x537425: ALPHAARG1 = D3DTA_DIFFUSE (push ebx = 0)
        set_texture_stage_state(1, 1, 1);
        set_texture_stage_state(1, 4, 1);
        ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0); // SetPixelShader(NULL)
    }

    {
        uint32_t address_mode = (rasterizer_caps.texture_address_caps & 8) ? 4u : 3u; // BORDER : CLAMP
        set_sampler_state(0, 1, address_mode); // ADDRESSU
        set_sampler_state(0, 2, address_mode); // ADDRESSV
    }
    set_sampler_state(0, 5, 2); // MAGFILTER = LINEAR
    set_sampler_state(0, 6, 2); // MINFILTER = LINEAR
    set_sampler_state(0, 7, 2); // MIPFILTER = LINEAR
    ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, 0); // SetVertexShader(NULL)
    ((d3d_call1_fn)device_vtable()[0x164 / 4])(rasterizer_device, 0x1c4); // SetFVF

    rasterizer_set_shader_stage_config(0);

    lens_flare_batch_clock = 0;
    lens_flare_current_key.bitmap_tag_index = 0;
    lens_flare_current_key.second_bitmap_tag_index = 0;
    lens_flare_current_key.bitmap_index = 0;
    lens_flare_current_key.shader_stage_config = 0;
    lens_flare_applied_key.bitmap_tag_index = 0;
    lens_flare_applied_key.second_bitmap_tag_index = 0;
    lens_flare_applied_key.bitmap_index = 0;
    lens_flare_applied_key.shader_stage_config = 0;
    lens_flare_vertex_specular = 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x537130):

void FUN_00537130(void)

{
  short in_AX;
  uint in_ECX;
  uint uVar1;
  undefined4 uVar2;
  int *piStack_88;
  undefined4 uStack_84;
  uint uStack_80;
  int *piStack_7c;
  int *piStack_78;
  undefined4 uStack_74;
  int *piStack_70;
  int *piStack_6c;
  uint uStack_68;
  int *piStack_64;
  int *piStack_60;
  undefined4 uStack_5c;
  int *piStack_58;
  int *piStack_54;
  undefined4 uStack_50;
  int *piStack_4c;
  int *piStack_48;
  undefined4 uStack_44;
  int *piStack_40;
  int *piStack_3c;
  undefined4 uStack_38;
  int *piStack_34;
  int *piStack_30;
  undefined4 uStack_2c;
  int *piStack_28;
  int *piStack_24;
  undefined4 uStack_20;
  int *piStack_1c;
  int *piStack_18;
  undefined4 uStack_14;

  if (in_AX == 5) {
    if (DAT_006e1dc0 == '\0') {
      uVar1 = 1;
    }
    else {
      uVar1 = in_ECX & 1;
    }
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,7);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,5);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,2);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x7,uVar1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x17,4);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xe,in_ECX >> 1 & 1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
    DAT_0071d278 = &DAT_0069da10;
    if (DAT_0069da10 == (int *)0x0) {
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,4);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,3,0);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,0);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
      (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
      (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
    }
    else {
      (**(code **)(*DAT_0069da10 + 0x100))(DAT_0069da10,&piStack_88,3);
      (**(code **)(*(int *)*DAT_0071d278 + 0x104))((int *)*DAT_0071d278,0);
    }
    if ((DAT_007c110c & 8) == 0) {
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,3);
      uVar2 = 3;
    }
    else {
      (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,4);
      uVar2 = 4;
    }
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,2,uVar2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,5,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,2);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,2);
    (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,0);
    (**(code **)(*DAT_0071d174 + 0x164))(DAT_0071d174,0x1c4);
    rasterizer_set_shader_stage_config();
    DAT_00746fa8 = 0;
    DAT_00746fb0 = 0;
    DAT_00746fb4 = 0;
    DAT_00746fb8 = 0;
    DAT_00746fbc._0_2_ = 0;
    DAT_007bf040 = 0;
    DAT_007bf044 = 0;
    DAT_007bf048 = 0;
    DAT_007bf04c = 0;
    DAT_0069e708 = 0xffffffff;
  }
  else if (in_AX == 6) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
    (**(code **)(*DAT_0071d174 + 0xe4))
              (DAT_0071d174,0xa8,-(uint)(DAT_00689425 != 0) & 7);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x7,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x17,4);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xe,(uint)DAT_00689425);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x3c,0xffff0000);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,3);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
    (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,0);
    (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
    (**(code **)(*DAT_0071d174 + 0x164))(DAT_0071d174,0x144);
    return;
  }
  return;
}
#endif
