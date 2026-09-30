// rasterizer_dx9_shaders_initialize  (Ghidra: rasterizer_dx9_shaders_initialize, already named)
// address 0x52fab0, size 1192 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: cea-pdb name match on the shader-constant name strings; the per-effect GlobalAlloc +
//   GetParameterByName (vtable +0x24, established in rasterizer_dx9_shaders_init_effect.c) blocks
//   line up exactly with rasterizer_effects[] slots by address arithmetic (checked with a small
//   script against every base/bound pair in the decompile): effects[0] (6 handles), effects[1..3]
//   (1 handle each), effects[32..34] and [37..39] (3 handles each), effects[40..43] (4 handles),
//   effects[106..108] (3 handles each, addressed directly rather than through a loop), effects[114]
//   (2 handles) and effects[116..121] (5 handles each).
// register convention: none -- __cdecl, no arguments.
// Phase 4 review: the return value is the byte local [esp+0xb] (mov al,[esp+0xb] at 0x52ff3a
//   and 0x52ff4d), i.e. the pool / fx.bin success flag, not the masked last handle Ghidra shows;
//   the second effects[114] handle is "c_light_enhancement" (push 0x66f05c at 0x52feb1).
// UNSURE: `DAT_00722b64 != 0x270e` selecting "ps_2_a" vs "ps_2_0" reuses
//   the same unexplained global as rasterizer_dx9_shaders_init_effect.c's `unknown_00722b60`
//   sibling (see that file); named the same way here.

#include "d3d.h"
#include "win32.h"
#include "tags.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern d3dx_macro rasterizer_effect_defines[2];                      // 0x007c0460, NULL-terminated
extern void *rasterizer_effect_pool;                                  // 0x0071d254
extern int32_t config_force_shader; // 0x00722b64 config pixel shader version (0x270e selects ps_2_a)
extern const char *rasterizer_shader_file_name;                       // 0x00722bbc
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410

extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70


typedef int32_t (__stdcall *d3dx_get_by_name_fn)(void *effect, void *parent, const char *name);

static uint32_t get_param(void *effect, const char *name)
{
    void **vt = *(void ***)effect;
    return (uint32_t)((d3dx_get_by_name_fn)vt[0x24 / 4])(effect, 0, name);
}

// Allocates and looks up the named vertex/pixel-shader-constant handle tables used by every
// material shader, after compiling all 122 pixel-shader effects from shaders\fx.bin.
uint8_t rasterizer_dx9_shaders_initialize(void)
{
    uint32_t saved_locale;
    int32_t hr;
    uint8_t success;
    int i;
    uint32_t *handles;

    saved_locale = GetThreadLocale();
    SetThreadLocale(0x409);

    rasterizer_effect_defines[0].name = "PS_2_0_TARGET";
    rasterizer_effect_defines[0].definition = "ps_2_a";
    if (config_force_shader != 0x270e) {
        rasterizer_effect_defines[0].definition = "ps_2_0";
    }
    rasterizer_effect_defines[1].name = 0;
    rasterizer_effect_defines[1].definition = 0;

    hr = D3DXCreateEffectPool((LPD3DXEFFECTPOOL *)&rasterizer_effect_pool);
    if (hr < 0) {
        success = 0;
    } else {
        success = rasterizer_dx9_pixel_shaders_load_all();
        if (success == 0) {
            rasterizer_shader_file_name = "shaders\\fx.bin";
            shell_display_fatal_error_dialog(0x89, 0x7e, 1);
        }
    }
    SetThreadLocale(saved_locale);
    if (success == 0) {
        shell_display_fatal_error_dialog(0x69, 0x7e, 1);
        return success;
    }

    // effects[116..121]: 5 handles each (primary_change_color / fog_color_correction_0/E/1 /
    // self_illumination_color).
    for (i = 116; i <= 121; i++) {
        void *effect = (void *)rasterizer_effects[i].effect;
        handles = (uint32_t *)GlobalAlloc(0, 0x14);
        rasterizer_effects[i].constant_handles = (uint32_t)handles;
        handles[0] = get_param(effect, "c_primary_change_color");
        handles[1] = get_param(effect, "c_fog_color_correction_0");
        handles[2] = get_param(effect, "c_fog_color_correction_E");
        handles[3] = get_param(effect, "c_fog_color_correction_1");
        handles[4] = get_param(effect, "c_self_illumination_color");
    }

    // effects[32..34]: 3 handles each (eye_forward / view_perpendicular_color / view_parallel_color).
    for (i = 32; i <= 34; i++) {
        void *effect = (void *)rasterizer_effects[i].effect;
        handles = (uint32_t *)GlobalAlloc(0, 0xc);
        rasterizer_effects[i].constant_handles = (uint32_t)handles;
        handles[0] = get_param(effect, "c_eye_forward");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
    }

    // effects[37..39]: same shape as effects[32..34].
    for (i = 37; i <= 39; i++) {
        void *effect = (void *)rasterizer_effects[i].effect;
        handles = (uint32_t *)GlobalAlloc(0, 0xc);
        rasterizer_effects[i].constant_handles = (uint32_t)handles;
        handles[0] = get_param(effect, "c_eye_forward");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
    }

    // effects[106]: 4 handles (eye_forward / view_perpendicular_color / view_parallel_color /
    // group_intensity), addressed directly rather than through a loop.
    {
        void *effect = (void *)rasterizer_effects[106].effect;
        handles = (uint32_t *)GlobalAlloc(0, 0x10);
        rasterizer_effects[106].constant_handles = (uint32_t)handles;
        handles[0] = get_param(effect, "c_eye_forward");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
        handles[3] = get_param(effect, "c_group_intensity");
    }

    // effects[107]: 3 handles (eye_forward / view_perpendicular_color / view_parallel_color).
    {
        void *effect = (void *)rasterizer_effects[107].effect;
        handles = (uint32_t *)GlobalAlloc(0, 0xc);
        rasterizer_effects[107].constant_handles = (uint32_t)handles;
        handles[0] = get_param(effect, "c_eye_forward");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
    }

    // effects[108]: same shape as effects[107].
    {
        void *effect = (void *)rasterizer_effects[108].effect;
        handles = (uint32_t *)GlobalAlloc(0, 0xc);
        rasterizer_effects[108].constant_handles = (uint32_t)handles;
        handles[0] = get_param(effect, "c_eye_forward");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
    }

    // effects[0]: 6 handles (material_color / plasma_animation / primary_color / secondary_color /
    // plasma_on_color / plasma_off_color).
    {
        void *effect = (void *)rasterizer_effects[0].effect;
        handles = (uint32_t *)GlobalAlloc(0, 0x18);
        rasterizer_effects[0].constant_handles = (uint32_t)handles;
        handles[0] = get_param(effect, "c_material_color");
        handles[1] = get_param(effect, "c_plasma_animation");
        handles[2] = get_param(effect, "c_primary_color");
        handles[3] = get_param(effect, "c_secondary_color");
        handles[4] = get_param(effect, "c_plasma_on_color");
        handles[5] = get_param(effect, "c_plasma_off_color");
    }

    // effects[1..3]: 1 handle each (material_color).
    for (i = 1; i <= 3; i++) {
        void *effect = (void *)rasterizer_effects[i].effect;
        handles = (uint32_t *)GlobalAlloc(0, 4);
        rasterizer_effects[i].constant_handles = (uint32_t)handles;
        handles[0] = get_param(effect, "c_material_color");
    }

    // effects[114]: 2 handles (desaturation_tint / light_enhancement).
    {
        void *effect = (void *)rasterizer_effects[114].effect;
        handles = (uint32_t *)GlobalAlloc(0, 8);
        rasterizer_effects[114].constant_handles = (uint32_t)handles;
        handles[0] = get_param(effect, "c_desaturation_tint");
        handles[1] = get_param(effect, "c_light_enhancement");
    }

    // effects[40..43]: 4 handles each (specular_brightness / view_perpendicular_color /
    // view_parallel_color / multiplier).
    for (i = 40; i <= 43; i++) {
        void *effect = (void *)rasterizer_effects[i].effect;
        handles = (uint32_t *)GlobalAlloc(0, 0x10);
        rasterizer_effects[i].constant_handles = (uint32_t)handles;
        handles[0] = get_param(effect, "c_specular_brightness");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
        handles[3] = get_param(effect, "c_multiplier");
    }
    return success;
}

#if 0
Original Ghidra decompilation (0x52fab0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint __cdecl rasterizer_dx9_shaders_initialize(void)

{
  int *piVar1;
  undefined4 *puVar2;
  LCID Locale;
  int iVar3;
  HGLOBAL pvVar4;
  undefined4 uVar5;
  uint uVar6;
  undefined4 *puVar7;
  char local_1;

  Locale = GetThreadLocale();
  SetThreadLocale(0x409);
  _DAT_007c0460 = "PS_2_0_TARGET";
  _DAT_007c0464 = "ps_2_a";
  if (DAT_00722b64 != 0x270e) {
    _DAT_007c0464 = "ps_2_0";
  }
  _DAT_007c0468 = 0;
  _DAT_007c046c = 0;
  iVar3 = FUN_00583ddc(&DAT_0071d254);
  if (iVar3 < 0) {
    local_1 = '\0';
  }
  else {
    uVar6 = rasterizer_dx9_pixel_shaders_load_all();
    local_1 = (char)uVar6;
    if (local_1 == '\0') {
      DAT_00722bbc = "shaders\\fx.bin";
      shell_display_fatal_error_dialog(0x89,0x7e,1);
    }
  }
  SetThreadLocale(Locale);
  if (local_1 != '\0') {
    puVar7 = &DAT_0069e290;
    do {
      pvVar4 = GlobalAlloc(0,0x14);
      puVar7[6] = pvVar4;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_primary_change_color");
      *(undefined4 *)puVar7[6] = uVar5;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_fog_color_correction_0");
      *(undefined4 *)(puVar7[6] + 4) = uVar5;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_fog_color_correction_E");
      *(undefined4 *)(puVar7[6] + 8) = uVar5;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_fog_color_correction_1");
      *(undefined4 *)(puVar7[6] + 0xc) = uVar5;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_self_illumination_color");
      piVar1 = puVar7 + 6;
      puVar7 = puVar7 + 8;
      *(undefined4 *)(*piVar1 + 0x10) = uVar5;
    } while ((int)puVar7 < 0x69e331);
    puVar7 = &DAT_0069d810;
    do {
      pvVar4 = GlobalAlloc(0,0xc);
      puVar7[6] = pvVar4;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_eye_forward");
      *(undefined4 *)puVar7[6] = uVar5;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_view_perpendicular_color");
      *(undefined4 *)(puVar7[6] + 4) = uVar5;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_view_parallel_color");
      piVar1 = puVar7 + 6;
      puVar7 = puVar7 + 8;
      *(undefined4 *)(*piVar1 + 8) = uVar5;
    } while ((int)puVar7 < 0x69d851);
    puVar7 = &DAT_0069d8b0;
    do {
      pvVar4 = GlobalAlloc(0,0xc);
      puVar7[6] = pvVar4;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_eye_forward");
      *(undefined4 *)puVar7[6] = uVar5;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_view_perpendicular_color");
      *(undefined4 *)(puVar7[6] + 4) = uVar5;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_view_parallel_color");
      piVar1 = puVar7 + 6;
      puVar7 = puVar7 + 8;
      *(undefined4 *)(*piVar1 + 8) = uVar5;
    } while ((int)puVar7 < 0x69d8f1);
    DAT_0069e168 = GlobalAlloc(0,0x10);
    uVar5 = (**(code **)(*DAT_0069e150 + 0x24))(DAT_0069e150,0,"c_eye_forward");
    *DAT_0069e168 = uVar5;
    uVar5 = (**(code **)(*DAT_0069e150 + 0x24))(DAT_0069e150,0,"c_view_perpendicular_color");
    DAT_0069e168[1] = uVar5;
    uVar5 = (**(code **)(*DAT_0069e150 + 0x24))(DAT_0069e150,0,"c_view_parallel_color");
    DAT_0069e168[2] = uVar5;
    uVar5 = (**(code **)(*DAT_0069e150 + 0x24))(DAT_0069e150,0,"c_group_intensity");
    DAT_0069e168[3] = uVar5;
    DAT_0069e188 = GlobalAlloc(0,0xc);
    uVar5 = (**(code **)(*DAT_0069e170 + 0x24))(DAT_0069e170,0,"c_eye_forward");
    *DAT_0069e188 = uVar5;
    uVar5 = (**(code **)(*DAT_0069e170 + 0x24))(DAT_0069e170,0,"c_view_perpendicular_color");
    DAT_0069e188[1] = uVar5;
    uVar5 = (**(code **)(*DAT_0069e170 + 0x24))(DAT_0069e170,0,"c_view_parallel_color");
    DAT_0069e188[2] = uVar5;
    DAT_0069e1a8 = GlobalAlloc(0,0xc);
    uVar5 = (**(code **)(*DAT_0069e190 + 0x24))(DAT_0069e190,0,"c_eye_forward");
    *DAT_0069e1a8 = uVar5;
    uVar5 = (**(code **)(*DAT_0069e190 + 0x24))(DAT_0069e190,0,"c_view_perpendicular_color");
    DAT_0069e1a8[1] = uVar5;
    uVar5 = (**(code **)(*DAT_0069e190 + 0x24))(DAT_0069e190,0,"c_view_parallel_color");
    DAT_0069e1a8[2] = uVar5;
    DAT_0069d428 = GlobalAlloc(0,0x18);
    uVar5 = (**(code **)(*DAT_0069d410 + 0x24))(DAT_0069d410,0,"c_material_color");
    *DAT_0069d428 = uVar5;
    uVar5 = (**(code **)(*DAT_0069d410 + 0x24))(DAT_0069d410,0,"c_plasma_animation");
    DAT_0069d428[1] = uVar5;
    uVar5 = (**(code **)(*DAT_0069d410 + 0x24))(DAT_0069d410,0,"c_primary_color");
    DAT_0069d428[2] = uVar5;
    uVar5 = (**(code **)(*DAT_0069d410 + 0x24))(DAT_0069d410,0,"c_secondary_color");
    DAT_0069d428[3] = uVar5;
    uVar5 = (**(code **)(*DAT_0069d410 + 0x24))(DAT_0069d410,0,"c_plasma_on_color");
    DAT_0069d428[4] = uVar5;
    uVar5 = (**(code **)(*DAT_0069d410 + 0x24))(DAT_0069d410,0,"c_plasma_off_color");
    DAT_0069d428[5] = uVar5;
    puVar7 = &DAT_0069d448;
    do {
      pvVar4 = GlobalAlloc(0,4);
      *puVar7 = pvVar4;
      uVar5 = (**(code **)(*(int *)puVar7[-6] + 0x24))((int *)puVar7[-6],0,"c_material_color");
      puVar2 = (undefined4 *)*puVar7;
      puVar7 = puVar7 + 8;
      *puVar2 = uVar5;
    } while ((int)puVar7 < 0x69d489);
    DAT_0069e268 = GlobalAlloc(0,8);
    uVar5 = (**(code **)(*DAT_0069e250 + 0x24))(DAT_0069e250,0,"c_desaturation_tint");
    *DAT_0069e268 = uVar5;
    uVar5 = (**(code **)(*DAT_0069e250 + 0x24))(DAT_0069e250,0);
    DAT_0069e268[1] = uVar5;
    puVar7 = &DAT_0069d910;
    do {
      pvVar4 = GlobalAlloc(0,0x10);
      puVar7[6] = pvVar4;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_specular_brightness");
      *(undefined4 *)puVar7[6] = uVar5;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_view_perpendicular_color");
      *(undefined4 *)(puVar7[6] + 4) = uVar5;
      uVar5 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_view_parallel_color");
      *(undefined4 *)(puVar7[6] + 8) = uVar5;
      uVar6 = (**(code **)(*(int *)*puVar7 + 0x24))((int *)*puVar7,0,"c_multiplier");
      piVar1 = puVar7 + 6;
      puVar7 = puVar7 + 8;
      *(uint *)(*piVar1 + 0xc) = uVar6;
    } while ((int)puVar7 < 0x69d971);
    return uVar6 & 0xffffff00;
  }
  uVar6 = shell_display_fatal_error_dialog(0x69,0x7e,1);
  return uVar6 & 0xffffff00;
}
#endif
