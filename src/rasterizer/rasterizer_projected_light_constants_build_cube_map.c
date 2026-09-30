// rasterizer_projected_light_constants_build_cube_map  (Ghidra: FUN_005215b0)
// address 0x5215b0, size 403 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: types/rasterizer.h documents this address directly as the cube-map-path builder of
// rasterizer_projected_light_constants ("0x005215b0 (cube map path) or FUN_00521750 (plain
// falloff) fill it for one light"), and every field write here matches that struct's own offset
// table exactly: position (0x00), inverse_radius = 0.5/(radius*Light.specular_radius_multiplier)
// (0x0c), basis[0] = negated forward (0x10), basis[1] = negated cross(forward, up) (0x20),
// basis[2] = negated up (0x30), cone_axis = forward * 1/(r - r/2) (0x40), cone_offset (0x4c);
// the trailing `DAT_006e0a64` write matches rasterizer_projected_light_cube_map immediately
// after the struct. `(&DAT_007c1484)[light_index * 0xe]` and the `* 0x38` stride both match
// rasterizer_light exactly (definition, forward, up, radius fields), and the Light tag reads at
// +0x24 (specular_radius_multiplier), +0x70/+0x88 (primary/secondary cube map tag id) match the
// header's own Light notes.
// register convention: light_index in EAX (no stack parameters).
// UNSURE: vector3d_cross_product's out/second-operand registers are elided by Ghidra; the third
// argument is assumed to be light->up by analogy with basis[2] = negated up.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"
#include "fn_math.h"

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern uint8_t console_debug_toggle_6893f6; // 0x006893f6
extern rasterizer_light rasterizer_lights[k_rasterizer_maximum_lights]; // 0x007c1484
extern rasterizer_projected_light_constants rasterizer_projected_light; // 0x006e0a10
extern uint32_t rasterizer_projected_light_cube_map;                // 0x006e0a64 bitmap tag id

extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0


// blam-cc: EAX = light_index
void rasterizer_projected_light_constants_build_cube_map(int32_t light_index)
{
    rasterizer_light *light;
    Light *definition;
    int32_t cube_map_tag_index;
    real_vector3d cross_axis;
    float radius;
    float scale;

    if (console_debug_toggle_6893f6 == 0 || rasterizer_caps.pixel_shader_version <= 0xffff0103) {
        return;
    }

    light = &rasterizer_lights[light_index];
    definition = (Light *)(uint8_t *)light->definition;

    cube_map_tag_index = *(int32_t *)&((struct Light *)definition)->primary_cube_map.tag_id;
    if (cube_map_tag_index == -1) {
        cube_map_tag_index = *(int32_t *)&((struct Light *)definition)->secondary_cube_map.tag_id;
    }

    vector3d_cross_product(&cross_axis, &light->forward, &light->up);
    vector3d_normalize_with_length(&cross_axis);

    rasterizer_projected_light.position = light->position;
    radius = ((struct Light *)definition)->specular_radius_multiplier * light->radius;
    rasterizer_projected_light.basis[0][3] = 1.0f;
    rasterizer_projected_light.basis[1][3] = 1.0f;
    rasterizer_projected_light.basis[2][3] = 1.0f;
    scale = 1.0f / (radius - radius * 0.5f);
    rasterizer_projected_light.inverse_radius = 0.5f / radius;
    rasterizer_projected_light.basis[0][0] = -light->forward.i;
    rasterizer_projected_light.basis[0][1] = -light->forward.j;
    rasterizer_projected_light.basis[0][2] = -light->forward.k;
    rasterizer_projected_light.basis[1][0] = -cross_axis.i;
    rasterizer_projected_light.basis[1][1] = -cross_axis.j;
    rasterizer_projected_light.basis[1][2] = -cross_axis.k;
    rasterizer_projected_light.basis[2][0] = -light->up.i;
    rasterizer_projected_light.basis[2][1] = -light->up.j;
    rasterizer_projected_light.basis[2][2] = -light->up.k;
    rasterizer_projected_light.cone_axis.i = light->forward.i * scale;
    rasterizer_projected_light.cone_axis.j = light->forward.j * scale;
    rasterizer_projected_light.cone_axis.k = light->forward.k * scale;
    rasterizer_projected_light.cone_offset = -(scale * radius * 0.5f);
    rasterizer_projected_light_cube_map = cube_map_tag_index;
}

#if 0
Original Ghidra decompilation (0x5215b0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_005215b0(void)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  int in_EAX;
  int iVar10;
  int iVar11;
  float local_24;
  float local_20;
  float local_1c;

  if ((DAT_006893f6 != '\0') && (0xffff0103 < DAT_007c118c)) {
    iVar10 = in_EAX * 0x38;
    iVar1 = (&DAT_007c1484)[in_EAX * 0xe];
    iVar11 = *(int *)(iVar1 + 0x70);
    if (iVar11 == -1) {
      iVar11 = *(int *)(iVar1 + 0x88);
    }
    fVar2 = *(float *)(&DAT_007c1494 + iVar10);
    fVar3 = *(float *)(&DAT_007c1498 + iVar10);
    fVar4 = *(float *)(&DAT_007c149c + iVar10);
    fVar5 = *(float *)(&DAT_007c14a0 + iVar10);
    fVar6 = *(float *)(&DAT_007c14a4 + iVar10);
    fVar7 = *(float *)(&DAT_007c14a8 + iVar10);
    vector3d_cross_product(&DAT_007c1494 + iVar10);
    vector3d_normalize_with_length();
    _DAT_006e0a10 = (&DAT_007c1488)[in_EAX * 0xe];
    fVar8 = *(float *)(iVar1 + 0x24) * *(float *)(&DAT_007c14b8 + iVar10);
    _DAT_006e0a14 = *(undefined4 *)(&DAT_007c148c + iVar10);
    _DAT_006e0a18 = *(undefined4 *)(&DAT_007c1490 + iVar10);
    _DAT_006e0a2c = 0x3f800000;
    _DAT_006e0a3c = 0x3f800000;
    _DAT_006e0a4c = 0x3f800000;
    fVar9 = 1.0 / (fVar8 - fVar8 * 0.5);
    _DAT_006e0a1c = 0.5 / fVar8;
    _DAT_006e0a20 = -fVar2;
    _DAT_006e0a24 = -fVar3;
    _DAT_006e0a28 = -fVar4;
    _DAT_006e0a30 = -local_24;
    _DAT_006e0a34 = -local_20;
    _DAT_006e0a38 = -local_1c;
    _DAT_006e0a40 = -fVar5;
    _DAT_006e0a44 = -fVar6;
    _DAT_006e0a48 = -fVar7;
    _DAT_006e0a50 = fVar2 * fVar9;
    _DAT_006e0a54 = fVar3 * fVar9;
    _DAT_006e0a58 = fVar4 * fVar9;
    _DAT_006e0a5c = -(fVar9 * fVar8 * 0.5);
    DAT_006e0a64 = iVar11;
  }
  return;
}
#endif
