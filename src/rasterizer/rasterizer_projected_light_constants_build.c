// rasterizer_projected_light_constants_build  (Ghidra: FUN_00521750)
// address 0x521750, size 418 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: types/rasterizer.h documents this address as the "plain falloff" builder of
// rasterizer_projected_light_constants, alternative to the cube-map path at 0x5215b0. When the
// light has a real falloff cone (cos_falloff_angle != -1.0, the bit pattern -0x40800000) and at
// least one cube map tag, it computes the NTSC luminance (0.299/0.587/0.114 weights, matching
// rasterizer_light.color's own "NTSC luminance taken in 0x521750" note) and delegates to
// rasterizer_projected_light_constants_build_cube_map; otherwise it fills the constants with an
// identity-ish falloff (zero basis except the homogeneous w, matching a plain point light with
// no cone) and reads a default cube map tag from GlobalsRasterizerData+0xc.
// register convention: light_index in EAX (no stack parameters).
// UNSURE: DAT_0069c6fc has no documented owner in types/rasterizer.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893f6; // 0x006893f6
extern rasterizer_light rasterizer_lights[k_rasterizer_maximum_lights]; // 0x007c1484
extern int16_t rasterizer_projected_light_shader_variant;           // 0x0069c6fc 0 or 1, set by 0x521750
extern float rasterizer_projected_light_luminance; // 0x0071d1d8
extern rasterizer_projected_light_constants rasterizer_projected_light; // 0x006e0a10
extern uint8_t rasterizer_projected_light_has_cube_map; // 0x006e0a60
extern uint32_t rasterizer_projected_light_cube_map;                // 0x006e0a64 bitmap tag id
extern GlobalsRasterizerData *rasterizer_globals_data;              // 0x0071d164

extern void rasterizer_projected_light_constants_build_cube_map(int32_t light_index); // 0x5215b0

// blam-cc: EAX = light_index
void rasterizer_projected_light_constants_build(int32_t light_index)
{
    rasterizer_light *light;
    Light *definition;

    if (console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f6 == 0 ||
        rasterizer_caps.pixel_shader_version <= 0xffff0103) {
        return;
    }

    light = &rasterizer_lights[light_index];
    definition = (Light *)(uint8_t *)light->definition;

    if (*(int32_t *)&((struct Light *)definition)->cos_falloff_angle != (int32_t)0xbf800000 &&
        (*(int32_t *)&((struct Light *)definition)->primary_cube_map.tag_id != -1 ||
         *(int32_t *)&((struct Light *)definition)->secondary_cube_map.tag_id != -1)) {
        rasterizer_projected_light_shader_variant = 1;
        rasterizer_projected_light_luminance =
            light->color.red * 0.299f + light->color.green * 0.587f + light->color.blue * 0.114f;
        rasterizer_projected_light_constants_build_cube_map(light_index);
        rasterizer_projected_light_has_cube_map = 1;
        return;
    }

    rasterizer_projected_light.position = light->position;
    rasterizer_projected_light_shader_variant = 0;
    rasterizer_projected_light_has_cube_map = 0;
    rasterizer_projected_light_luminance =
        light->color.red * 0.299f + light->color.green * 0.587f + light->color.blue * 0.114f;

    rasterizer_projected_light.inverse_radius =
        1.0f / (((struct Light *)definition)->specular_radius_multiplier * light->radius) * 0.5f;

    rasterizer_projected_light.basis[0][0] = 0.0f;
    rasterizer_projected_light.basis[0][1] = 0.0f;
    rasterizer_projected_light.basis[0][2] = 0.0f;
    rasterizer_projected_light.basis[0][3] = 1.0f;
    rasterizer_projected_light.basis[1][0] = 0.0f;
    rasterizer_projected_light.basis[1][1] = 0.0f;
    rasterizer_projected_light.basis[1][2] = 0.0f;
    rasterizer_projected_light.basis[1][3] = 1.0f;
    rasterizer_projected_light.basis[2][0] = 0.0f;
    rasterizer_projected_light.basis[2][1] = 0.0f;
    rasterizer_projected_light.basis[2][2] = 0.0f;
    rasterizer_projected_light.basis[2][3] = 1.0f;
    rasterizer_projected_light.cone_axis.i = 0.0f;
    rasterizer_projected_light.cone_axis.j = 0.0f;
    rasterizer_projected_light.cone_axis.k = 0.0f;
    rasterizer_projected_light.cone_offset = 1.0f;

    rasterizer_projected_light_cube_map = *(int32_t *)((uint8_t *)rasterizer_globals_data + 0xc);
}

#if 0
Original Ghidra decompilation (0x521750):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00521750(void)

{
  int iVar1;
  int in_EAX;
  int iVar2;

  if (((DAT_006893e4 == 0) && (DAT_006893f6 != '\0')) && (0xffff0103 < DAT_007c118c)) {
    iVar2 = in_EAX * 0x38;
    iVar1 = (&DAT_007c1484)[in_EAX * 0xe];
    if ((*(int *)(iVar1 + 0x1c) != -0x40800000) &&
       ((*(int *)(iVar1 + 0x70) != -1 || (*(int *)(iVar1 + 0x88) != -1)))) {
      DAT_0069c6fc = 1;
      _DAT_0071d1d8 =
           *(float *)(&DAT_007c14ac + iVar2) * 0.299 +
           *(float *)(&DAT_007c14b0 + iVar2) * 0.587 + *(float *)(&DAT_007c14b4 + iVar2) * 0.114;
      FUN_005215b0();
      DAT_006e0a60 = 1;
      return;
    }
    _DAT_006e0a10 = (&DAT_007c1488)[in_EAX * 0xe];
    DAT_0069c6fc = 0;
    DAT_006e0a60 = 0;
    _DAT_0071d1d8 =
         *(float *)(&DAT_007c14ac + iVar2) * 0.299 +
         *(float *)(&DAT_007c14b0 + iVar2) * 0.587 + *(float *)(&DAT_007c14b4 + iVar2) * 0.114;
    _DAT_006e0a14 = *(undefined4 *)(&DAT_007c148c + iVar2);
    _DAT_006e0a18 = *(undefined4 *)(&DAT_007c1490 + iVar2);
    _DAT_006e0a20 = 0;
    _DAT_006e0a24 = 0;
    _DAT_006e0a28 = 0;
    _DAT_006e0a2c = 0x3f800000;
    _DAT_006e0a30 = 0;
    _DAT_006e0a34 = 0;
    _DAT_006e0a38 = 0;
    _DAT_006e0a3c = 0x3f800000;
    _DAT_006e0a40 = 0;
    _DAT_006e0a44 = 0;
    _DAT_006e0a48 = 0;
    _DAT_006e0a4c = 0x3f800000;
    _DAT_006e0a50 = 0;
    _DAT_006e0a54 = 0;
    _DAT_006e0a58 = 0;
    _DAT_006e0a5c = 0x3f800000;
    _DAT_006e0a1c = (1.0 / (*(float *)(iVar1 + 0x24) * *(float *)(&DAT_007c14b8 + iVar2))) * 0.5;
    DAT_006e0a64 = *(undefined4 *)(DAT_0071d164 + 0xc);
  }
  return;
}
#endif
