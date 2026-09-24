// rasterizer_light_cone_set_orientation_constants  (Ghidra: FUN_0051da20)
// address 0x51da20, size 553 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase2/results/rasterizer_01.json ("Computes and uploads the animated
// orientation and scale vertex-shader constants for a volumetric light-cone element."). `light`
// matches types/rasterizer.h rasterizer_light exactly: `in_EAX * 0x38` / `&DAT_007c1484 +
// in_EAX * 0xe` is rasterizer_lights[light_index] (stride 0x38 = 0xe dwords), and the first
// upload's vec4 (`local_94..local_88`) is `light->position` followed by `0.5 / light->radius`,
// matching rasterizer_projected_light_constants' own (position, inverse_radius) layout. The two
// vtable calls are confirmed via src/networking/network_stats_overlay_draw.c's own note on
// vtable+0x178 ("this, count, ptr, mode" -- here (StartRegister=0xd, pConstantData, Count=5), a
// SetVertexShaderConstantF shape) and by symmetry vtable+0x1b4 (index 109) is
// SetPixelShaderConstantF(StartRegister=0, pConstantData, Count=1).
// register convention: light_index in EAX (no stack parameters).
// UNSURE (function-wide): the three intermediate `matrix4x3_transform_normal`/
// `vector3d_cross_product`/`vector3d_normalize_with_length` calls are rendered by Ghidra with NO
// visible arguments at all (fully register-passed); this rewrite reconstructs them as
// transforming `light->forward` and `light->up` by the periodic-angle rotation matrix, then
// crossing and normalizing the two results, which matches the local variable count and the
// "orientation" framing of the phase2 summary, but the actual source vectors are not verified.
// `local_54/local_50/local_4c` (the fifth vec4's xyz) are literal zero constants in Ghidra's own
// output, not computed from anything -- preserved as such.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern uint8_t console_debug_toggle_6893f3;           // 0x006893f3, UNSURE meaning
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410; effect 4 (0x0069d490)
extern rasterizer_frame_time rasterizer_time;          // 0x007c1200
extern rasterizer_light rasterizer_lights[k_rasterizer_maximum_lights]; // 0x007c1484
extern void *rasterizer_device;                        // 0x0071d174

extern real periodic_function_evaluate(periodic_function_t type, double time); // 0x4cc9b0, blam-cc: AX = type
extern void matrix4x3_from_euler_angles(real_matrix4x3 *out, real yaw, real pitch, real roll); // 0x4cba10, blam-cc: EAX = out
extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cbec0, blam-cc: EAX = out, EDX = normal
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
// blam-cc: EAX -> bitmap_tag_id, CX -> bitmap_type, stack -> (stage, default_index, frame, effect_slot)
extern int16_t *rasterizer_resolve_and_cache_submap_b(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage,
                                                      int16_t default_index, int16_t frame,
                                                      rasterizer_effect_slot *effect_slot); // 0x518860

typedef int32_t (__stdcall *d3d_call4v_fn)(void *self, uint32_t start_register, const void *data, uint32_t count);

// blam-cc: EAX = light_index
void rasterizer_light_cone_set_orientation_constants(int32_t light_index)
{
    void **vtable;
    d3d_call4v_fn set_vertex_shader_constant_f;
    d3d_call4v_fn set_pixel_shader_constant_f;
    rasterizer_light *light;
    real yaw, pitch, roll;
    real_matrix4x3 orientation;
    real_vector3d axis_a, axis_b, axis_c;
    float constants_vs[5][4];
    float constants_ps[1][4];

    if (console_debug_toggle_6893e4 != 0 || console_debug_toggle_6893f3 == 0 ||
        rasterizer_caps.pixel_shader_version <= 0xffff0100) {
        return;
    }

    light = &rasterizer_lights[light_index];

    // stage 1: the light primary cube map (Light +0x70), cube type, default entry 1
    if (rasterizer_effects[4].effect != 0) {
        rasterizer_resolve_and_cache_submap_b(*(uint32_t *)((uint8_t *)light->definition + 0x70), 2, 1, 1, 0,
                                              &rasterizer_effects[4]);
    }

    // the three periodic function types are passed in AX: Light +0x8e, +0x9e and +0x96
    // (read from the raw code 0x51da97..0x51dae6; Ghidra dropped them)
    yaw = periodic_function_evaluate(*(int16_t *)((uint8_t *)light->definition + 0x8e), rasterizer_time.time / *(float *)((uint8_t *)light->definition + 0x90)) * 6.2831855f;
    pitch = periodic_function_evaluate(*(int16_t *)((uint8_t *)light->definition + 0x9e), rasterizer_time.time / *(float *)((uint8_t *)light->definition + 0xa0)) * 6.2831855f;
    roll = periodic_function_evaluate(*(int16_t *)((uint8_t *)light->definition + 0x96), rasterizer_time.time / *(float *)((uint8_t *)light->definition + 0x98)) * 6.2831855f;

    matrix4x3_from_euler_angles(&orientation, yaw, pitch, roll);
    matrix4x3_transform_normal(&axis_a, &light->forward, &orientation); // UNSURE: source vector
    matrix4x3_transform_normal(&axis_b, &light->up, &orientation);      // UNSURE: source vector
    vector3d_cross_product(&axis_c, &axis_a, &axis_b);
    vector3d_normalize_with_length(&axis_c);

    constants_vs[0][0] = light->position.x;
    constants_vs[0][1] = light->position.y;
    constants_vs[0][2] = light->position.z;
    constants_vs[0][3] = 0.5f / light->radius;

    constants_vs[1][0] = -axis_a.i;
    constants_vs[1][1] = -axis_a.j;
    constants_vs[1][2] = -axis_a.k;
    constants_vs[1][3] = 1.0f;

    constants_vs[2][0] = -axis_b.i;
    constants_vs[2][1] = -axis_b.j;
    constants_vs[2][2] = -axis_b.k;
    constants_vs[2][3] = 1.0f;

    constants_vs[3][0] = -axis_c.i;
    constants_vs[3][1] = -axis_c.j;
    constants_vs[3][2] = -axis_c.k;
    constants_vs[3][3] = 1.0f;

    constants_vs[4][0] = 0.0f;
    constants_vs[4][1] = 0.0f;
    constants_vs[4][2] = 0.0f;
    constants_vs[4][3] = 1.0f;

    vtable = *(void ***)rasterizer_device;
    set_vertex_shader_constant_f = (d3d_call4v_fn)vtable[0x5e]; // +0x178
    set_vertex_shader_constant_f(rasterizer_device, 0xd, constants_vs, 5);

    constants_ps[0][0] = light->color.red;
    constants_ps[0][1] = light->color.green;
    constants_ps[0][2] = light->color.blue;
    constants_ps[0][3] = 1.0f;

    vtable = *(void ***)rasterizer_device;
    set_pixel_shader_constant_f = (d3d_call4v_fn)vtable[0x6d]; // +0x1b4
    set_pixel_shader_constant_f(rasterizer_device, 0, constants_ps, 1);
}

#if 0
Original Ghidra decompilation (0x51da20):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0051da20(void)

{
  int in_EAX;
  int iVar1;
  int *piVar2;
  float10 fVar3;
  undefined4 local_94;
  undefined4 local_90;
  undefined4 local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  undefined4 local_78;
  float local_74;
  float local_70;
  float local_6c;
  undefined4 local_68;
  float local_64;
  float local_60;
  float local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;

  if (((DAT_006893e4 == 0) && (DAT_006893f3 != '\0')) && (0xffff0100 < DAT_007c118c)) {
    iVar1 = in_EAX * 0x38;
    piVar2 = &DAT_007c1484 + in_EAX * 0xe;
    if (DAT_0069d490 != 0) {
      FUN_00518860(1,1,0,&DAT_0069d490);
    }
    fVar3 = (float10)periodic_function_evaluate
                               ((double)((float)_DAT_007c1200 / *(float *)(*piVar2 + 0x90)));
    local_8 = (float)(fVar3 * (float10)6.2831855);
    fVar3 = (float10)periodic_function_evaluate
                               ((double)((float)_DAT_007c1200 / *(float *)(*piVar2 + 0xa0)));
    local_c = (float)(fVar3 * (float10)6.2831855);
    fVar3 = (float10)periodic_function_evaluate
                               ((double)((float)_DAT_007c1200 / *(float *)(*piVar2 + 0x98)));
    matrix4x3_from_euler_angles(local_8,local_c,(float)(fVar3 * (float10)6.2831855));
    matrix4x3_transform_normal();
    matrix4x3_transform_normal();
    vector3d_cross_product();
    vector3d_normalize_with_length();
    local_94 = (&DAT_007c1488)[in_EAX * 0xe];
    local_88 = 0.5 / *(float *)(&DAT_007c14b8 + iVar1);
    local_90 = *(undefined4 *)(&DAT_007c148c + iVar1);
    local_8c = *(undefined4 *)(&DAT_007c1490 + iVar1);
    local_78 = 0x3f800000;
    local_84 = -local_30;
    local_80 = -local_2c;
    local_7c = -local_28;
    local_74 = -local_24;
    local_70 = -local_20;
    local_6c = -local_1c;
    local_64 = -local_18;
    local_60 = -local_14;
    local_68 = 0x3f800000;
    local_58 = 0x3f800000;
    local_54 = 0;
    local_5c = -local_10;
    local_50 = 0;
    local_4c = 0;
    local_48 = 0x3f800000;
    (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0xd,&local_94,5);
    local_38 = *(undefined4 *)(&DAT_007c14b4 + iVar1);
    local_3c = *(undefined4 *)(&DAT_007c14b0 + iVar1);
    local_40 = *(undefined4 *)(&DAT_007c14ac + iVar1);
    local_34 = 0x3f800000;
    (**(code **)(*DAT_0071d174 + 0x1b4))(DAT_0071d174,0,&local_40,1);
  }
  return;
}
#endif
