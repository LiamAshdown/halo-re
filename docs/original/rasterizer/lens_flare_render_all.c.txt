// lens_flare_render_all  (Ghidra: decal_render_active_list, misnamed; renamed per
// out/phase4/rasterizer_types_notes.md's lens flare misattribution table)
// address 0x513cf0, size 2147 bytes
// name confidence: 0.55  rewrite confidence: 0.85
// REWRITTEN from the disassembly (0x513cf0..0x514552), raw offsets throughout. The draft dropped
//   the reflection colour (color_channel_real_to_byte / color_pack_argb_from_real results), the
//   specular, the scale pair and the rotation of every quad, called rasterizer_lens_flare_quad_add
//   with 2 of its 5 arguments and called set_current_key / set_vertex_specular without prototypes,
//   so no lens flare (the a10 tutorial panel lights among them) drew correctly.
// Per instance of the current window with occlusion samples, a nonzero alpha byte (+0x1b) and
//   reflections:
//   d = position - camera, depth = d . forward, off = 2 (forward * depth - d) (the mirror offset
//   along which reflections are spread); depth is halved without occlusion queries.
//   brightness = visibility byte / 255 * near-fade clamp((depth - def+0x1c) / (def+0x18 -
//   def+0x1c)) (1 when def+0x1c <= 0) * alpha byte / 255.
//   rotation = lens_flare_compute_rotation(ESI flare, DI def+0x80) * def+0x84; angle =
//   atan2(d . window+0xa4, d . window+0xb0) in degrees.
//   inv = 1 / (def+0x8 - def+0xc) (0 when equal), bias = -inv * def+0xc; after normalizing d the
//   four falloff factors are 1, clamp(bias - (forward . n) inv), clamp(bias - (n . d) inv) and
//   clamp((forward . d) inv + bias), n the unpacked direction (+0x10).
//   Per reflection (0x80 bytes at def+0xc8): brightness lerp(+0x34, +0x38, intensity) *
//   falloff[+0x3c] * base (reflection 0 replaces base); radius lerp(+0x28, +0x2c); colour: the
//   instance colour with the brightness as alpha when the tint (+0x40..+0x4c) is all zero
//   (specular 1), otherwise the tint with the brightness as alpha, optionally animated by
//   periodic function +0x72 (period +0x74, phase +0x78) lerping alpha +0x50..+0x60 and colour
//   +0x54..+0x64 (specular = +0x40). Rotation +0x20 (+ the flare rotation and the def+0xa0/+0xa4
//   scale on reflection 0; flag 1 adds the angle), flag 4 scales the radius by (visibility + 1) /
//   2, flag 2 by the depth. Position = flare position + off * +0x1c. set_current_key(EAX def+0x2c,
//   ECX 0, stack +0x04) failing ends the flare; then the specular, blend mode 0x746fbc (2 for flag
//   8 on a second-window flare) and quad_add(EAX scale, EBX colour, position, radius, rotation in
//   radians).
// register convention: none -- purely global driven.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t unknown_006893ff;    // 0x006893ff lens flares enabled
extern uint8_t lens_flare_occlusion_queries_supported; // 0x006e1dc0
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern lens_flare_instance lens_flare_instances[0x400]; // 0x006ce818
extern int32_t lens_flare_instance_count;   // 0x0071d134
extern rasterizer_frame_time rasterizer_time; // 0x007c1200, the frame time as a double at +0
extern void *rasterizer_device;             // 0x0071d174
extern void *rasterizer_effect_pool_scratch; // 0x0071d278, the active effect, ended here
extern int16_t unknown_00746fbc;            // 0x00746fbc lens flare quad blend mode (0 or 2)
extern uint8_t rasterizer_caps_flag_68a;            // 0x0069c68a
extern uint8_t unknown_00689426;            // 0x00689426

// blam-cc: stack -> (z_near, z_far) as raw float bits
extern void chimera__rasterizer_set_frustum_z_func(uint32_t z_near, uint32_t z_far); // 0x518f40
// blam-cc: AX -> mode, ECX -> flags
extern void rasterizer_lens_flare_batching_select_mode(int16_t mode, uint32_t flags); // 0x537130
// blam-cc: AX -> mode
extern void rasterizer_set_shader_stage_config(int16_t mode);       // 0x519200
extern void rasterizer_lens_flare_batch_flush_all(void); // 0x536c80
extern uint8_t *lens_flare_get_visibility_byte(lens_flare_instance *flare); // 0x5134f0, ECX
extern real_vector3d *vector3d_unpack_normal_11_11_10(real_vector3d *out, uint32_t packed); // 0x513400, EAX out, ECX packed
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
// blam-cc: ESI -> flare, DI -> mode
extern float lens_flare_compute_rotation(lens_flare_instance *flare, int16_t mode); // 0x513540
extern double fpatan(double y, double x); // x87 FPATAN, atan2(y, x) (harness/x87_shims.c)
extern uint8_t color_channel_real_to_byte(float channel); // 0x5132b0, cdecl
extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t); // 0x43f6a0, EAX color1, ECX color0
// blam-cc: AX -> type, stack -> input
extern real periodic_function_evaluate(periodic_function_t type, double time); // 0x4cc9b0
extern uint32_t color_pack_argb_from_real(ColorARGB *color); // 0x497900
extern uint8_t rasterizer_lens_flare_set_current_key(int32_t second_bitmap_tag_index, int16_t bitmap_tag_index,
    int16_t bitmap_index); // 0x5120f0, EAX, ECX, stack
extern void rasterizer_lens_flare_set_vertex_specular(float intensity); // 0x512120, stack
extern void rasterizer_lens_flare_quad_add(const float *scale, uint32_t diffuse, const real_point3d *position,
    float radius, float rotation_radians); // 0x537550, EAX scale, EBX diffuse, stack
// blam-cc: EAX -> instance
extern void rasterizer_sun_glow_render(lens_flare_instance *instance); // 0x525ab0

static float lens_flare_clamp01(float x)
{
    if (!(x >= 0.0f)) {
        return 0.0f;
    }
    if (x > 1.0f) {
        return 1.0f;
    }
    return x;
}

void lens_flare_render_all(void)
{
    uint8_t *window = (uint8_t *)&rasterizer_window;
    const float *camera = (const float *)(window + 0x08);  // 0x7c1228
    const float *forward = (const float *)(window + 0x14); // 0x7c1234
    const float *axis_a = (const float *)(window + 0xa4);  // 0x7c12c4
    const float *axis_b = (const float *)(window + 0xb0);  // 0x7c12d0
    int16_t i;

    if (unknown_006893ff == 0 || *(int16_t *)window != 1 || lens_flare_instance_count <= 0) {
        return;
    }
    if (lens_flare_occlusion_queries_supported != 1) {
        chimera__rasterizer_set_frustum_z_func(0x3d000200, 0x45800000);
    }
    rasterizer_lens_flare_batching_select_mode(5, 0);

    for (i = 0; i < lens_flare_instance_count; i++) {
        uint8_t *instance = (uint8_t *)&lens_flare_instances[i];
        uint8_t *visibility_byte = lens_flare_get_visibility_byte((lens_flare_instance *)instance);
        real_vector3d unpacked;
        real_vector3d normal;
        uint8_t *definition;
        uint8_t alpha;
        real_point3d position;
        real_vector3d d;
        float depth, off[3], visibility, fade, base, rotation, angle, span, inv, bias, falloff[4];
        float intensity;
        int16_t j;

        normal = *vector3d_unpack_normal_11_11_10(&unpacked, ((struct lens_flare_instance *)instance)->packed_direction);
        if ((int16_t)(instance[0x22] & 0x7f) != *(int16_t *)(window + 2)) {
            continue;
        }
        definition = *(uint8_t **)instance;
        if (((struct lens_flare_instance *)instance)->sample_count <= 0) {
            continue;
        }
        alpha = instance[0x1b];
        if (alpha == 0 || *(int32_t *)(definition + 0xc4) <= 0) {
            continue;
        }

        position = *(real_point3d *)&((struct lens_flare_instance *)instance)->position.x;
        d.i = position.x - camera[0];
        d.j = position.y - camera[1];
        d.k = position.z - camera[2];
        depth = forward[2] * d.k + forward[1] * d.j + d.i * forward[0];
        off[0] = (forward[0] * depth - d.i) * 2.0f;
        off[1] = (forward[1] * depth - d.j) * 2.0f;
        off[2] = (forward[2] * depth - d.k) * 2.0f;
        if (lens_flare_occlusion_queries_supported == 0) {
            depth = depth * 0.5f;
        }

        visibility = (float)*visibility_byte * 0.003921569f;
        {
            float near_distance = *(float *)(definition + 0x1c);
            float far_distance = *(float *)(definition + 0x18);

            if (!(near_distance > 0.0f)) {
                fade = 1.0f;
            } else {
                float t = (depth - near_distance) / (far_distance - near_distance);

                if (!(t >= 0.0f)) {
                    fade = 0.0f;
                } else if (t > 1.0f) {
                    fade = 1.0f;
                } else {
                    fade = t;
                }
            }
        }
        base = visibility * fade * (float)alpha * 0.003921569f;

        rotation = lens_flare_compute_rotation((lens_flare_instance *)instance, *(int16_t *)(definition + 0x80)) *
                   *(float *)(definition + 0x84);
        angle = (float)fpatan((double)(axis_a[2] * d.k + axis_a[1] * d.j + d.i * axis_a[0]),
                              (double)(axis_b[2] * d.k + axis_b[1] * d.j + d.i * axis_b[0])) * 57.29578f;

        span = *(float *)(definition + 0x08) - *(float *)(definition + 0x0c);
        inv = (span != 0.0f) ? 1.0f / span : 0.0f;
        bias = -(inv * *(float *)(definition + 0x0c));
        vector3d_normalize_with_length(&d);
        falloff[0] = 1.0f;
        falloff[1] = lens_flare_clamp01(bias - (forward[2] * normal.k + forward[1] * normal.j + normal.i * forward[0]) * inv);
        falloff[2] = lens_flare_clamp01(bias - (normal.k * d.k + normal.j * d.j + normal.i * d.i) * inv);
        falloff[3] = lens_flare_clamp01((forward[2] * d.k + forward[1] * d.j + d.i * forward[0]) * inv + bias);

        if (!(base > 0.0f)) {
            continue;
        }
        intensity = (float)instance[0x23] * 0.003921569f;

        for (j = 0; j < *(int32_t *)(definition + 0xc4); j++) {
            uint8_t *reflection = *(uint8_t **)(definition + 0xc8) + (int32_t)j * 0x80;
            float r34 = *(float *)(reflection + 0x34);
            float brightness = ((*(float *)(reflection + 0x38) - r34) * intensity + r34) *
                               falloff[*(int16_t *)(reflection + 0x3c)] * base;
            float r28, radius, specular, reflection_rotation, scale[2];
            uint32_t colour;
            real_point3d vertex;
            uint16_t flags;

            if (j == 0) {
                base = brightness;
            }
            if (!(brightness > 0.0f)) {
                continue;
            }
            r28 = *(float *)(reflection + 0x28);
            radius = (*(float *)(reflection + 0x2c) - r28) * intensity + r28;

            if (*(float *)(reflection + 0x40) == 0.0f && *(float *)(reflection + 0x44) == 0.0f &&
                *(float *)(reflection + 0x48) == 0.0f && *(float *)(reflection + 0x4c) == 0.0f) {
                colour = ((uint32_t)color_channel_real_to_byte(brightness) << 24) |
                         (((struct lens_flare_instance *)instance)->color & 0xffffff);
                specular = 1.0f;
            } else {
                ColorARGB tint;

                tint.alpha = brightness;
                tint.red = *(float *)(reflection + 0x44);
                tint.green = *(float *)(reflection + 0x48);
                tint.blue = *(float *)(reflection + 0x4c);
                if (*(int16_t *)(reflection + 0x72) > 1) {
                    ColorRGB animated;
                    float t = (float)periodic_function_evaluate(
                        (periodic_function_t)*(int16_t *)(reflection + 0x72),
                        ((double)*(float *)(reflection + 0x78) + *(double *)&rasterizer_time) /
                            (double)*(float *)(reflection + 0x74));

                    color_interpolate((ColorRGB *)(reflection + 0x64), (ColorRGB *)(reflection + 0x54), &animated,
                                      reflection[0x70] & 3, t);
                    tint.alpha = ((1.0f - t) * *(float *)(reflection + 0x50) + t * *(float *)(reflection + 0x60)) *
                                 tint.alpha;
                    tint.red = tint.red * animated.red;
                    tint.green = tint.green * animated.green;
                    tint.blue = tint.blue * animated.blue;
                }
                colour = color_pack_argb_from_real(&tint);
                specular = *(float *)(reflection + 0x40);
            }

            if (j == 0) {
                reflection_rotation = rotation + *(float *)(reflection + 0x20);
                scale[0] = *(float *)(definition + 0xa0);
                scale[1] = *(float *)(definition + 0xa4);
            } else {
                reflection_rotation = *(float *)(reflection + 0x20);
                scale[0] = 1.0f;
                scale[1] = 1.0f;
            }
            flags = *(uint16_t *)reflection;
            if ((flags & 1) != 0) {
                reflection_rotation = reflection_rotation + angle;
            }
            if ((flags & 4) != 0) {
                radius = (visibility + 1.0f) * radius * 0.5f;
            }
            if ((flags & 2) != 0) {
                radius = radius * depth;
            }
            {
                float along = *(float *)(reflection + 0x1c);

                vertex.x = off[0] * along + position.x;
                vertex.y = off[1] * along + position.y;
                vertex.z = off[2] * along + position.z;
            }

            if (rasterizer_lens_flare_set_current_key(*(int32_t *)(definition + 0x2c), 0,
                                                      (int16_t)*(uint16_t *)(reflection + 0x04)) != 0) {
                break; // 0x5143fa: the rest of this flare is skipped
            }
            rasterizer_lens_flare_set_vertex_specular(specular);
            unknown_00746fbc = ((flags & 8) != 0 && (instance[0x22] & 0x80) != 0) ? 2 : 0;
            rasterizer_lens_flare_quad_add(scale, colour, &vertex, radius, reflection_rotation * 0.017453292f);
        }
    }

    rasterizer_set_shader_stage_config(0);
    if (lens_flare_occlusion_queries_supported != 1) {
        chimera__rasterizer_set_frustum_z_func(0, 0);
    }
    rasterizer_lens_flare_batch_flush_all();

    if (rasterizer_effect_pool_scratch != 0 && *(void **)rasterizer_effect_pool_scratch != 0) {
        void *obj = *(void **)rasterizer_effect_pool_scratch;
        void **vtable = *(void ***)obj;
        ((void (__stdcall *)(void *))vtable[0x108 / 4])(obj); // ID3DXEffect::End
    }
    rasterizer_effect_pool_scratch = 0;
    {
        void **device_vtable = *(void ***)rasterizer_device;
        ((void (__stdcall *)(void *, int32_t))device_vtable[0x164 / 4])(rasterizer_device, 0);
    }

    if (rasterizer_caps_flag_68a == 0 && unknown_00689426 != 0) {
        for (i = 0; i < lens_flare_instance_count; i++) {
            uint8_t *instance = (uint8_t *)&lens_flare_instances[i];

            if (((struct lens_flare_instance *)instance)->sample_count > 0 && (int16_t)(instance[0x22] & 0x7f) == *(int16_t *)(window + 2)) {
                uint8_t *definition = *(uint8_t **)instance;

                if (*(uint32_t *)(definition + 0x10) == 0x42480000 || (definition[0x30] & 1) != 0) {
                    rasterizer_sun_glow_render((lens_flare_instance *)instance); // EAX = instance (0x514535)
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x513cf0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void decal_render_active_list(void)

{
  int iVar1;
  float fVar2;
  byte bVar3;
  byte bVar4;
  int iVar5;
  int iVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  char cVar13;
  int iVar14;
  byte *pbVar15;
  float *pfVar16;
  short sVar17;
  ushort *puVar18;
  float10 fVar19;
  float local_b8;
  float local_a4;
  float local_a0;
  float local_94;
  float local_80;
  float local_7c;
  float local_78;
  float local_74;
  undefined4 local_70;
  float local_6c;
  float local_68;
  float local_64;
  int local_60;
  float local_5c;
  float local_58;
  float local_54;
  float local_50 [7];
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;

  if (((DAT_006893ff != '\0') && ((short)DAT_007c1220 == 1)) && (0 < DAT_0071d134)) {
    if (DAT_006e1dc0 != '\x01') {
      chimera__rasterizer_set_frustum_z_func(0x3d000200);
    }
    FUN_00537130();
    local_60 = 0;
    if (0 < DAT_0071d134) {
      iVar14 = 0;
      do {
        iVar1 = iVar14 * 0x28;
        pbVar15 = (byte *)FUN_005134f0();
        pfVar16 = (float *)vector3d_unpack_normal_11_11_10();
        local_5c = *pfVar16;
        local_58 = pfVar16[1];
        local_54 = pfVar16[2];
        if (((((byte)(&DAT_006ce83a)[iVar1] & 0xff7f) == DAT_007c1220._2_2_) &&
            (iVar5 = (&DAT_006ce818)[iVar14 * 10], 0 < (int)(&DAT_006ce83c)[iVar14 * 10])) &&
           ((bVar3 = *(byte *)((int)&DAT_006ce830 + iVar1 + 3), bVar3 != 0 &&
            (0 < *(int *)(iVar5 + 0xc4))))) {
          local_6c = (float)(&DAT_006ce81c)[iVar14 * 10];
          fVar2 = local_6c - DAT_007c1228;
          local_68 = (float)(&DAT_006ce820)[iVar14 * 10];
          local_64 = (float)(&DAT_006ce824)[iVar14 * 10];
          fVar9 = local_68 - DAT_007c122c;
          fVar10 = local_64 - DAT_007c1230;
          local_b8 = fVar2 * DAT_007c1234 + DAT_007c1238 * fVar9 + DAT_007c123c * fVar10;
          fVar11 = DAT_007c1234 * local_b8 - fVar2;
          fVar12 = DAT_007c1238 * local_b8 - fVar9;
          fVar7 = DAT_007c123c * local_b8 - fVar10;
          if (DAT_006e1dc0 == '\0') {
            local_b8 = local_b8 * 0.5;
          }
          bVar4 = *pbVar15;
          if (*(float *)(iVar5 + 0x1c) <= 0.0) {
LAB_00513f18:
            fVar8 = 1.0;
          }
          else if (0.0 <= (local_b8 - *(float *)(iVar5 + 0x1c)) /
                          (*(float *)(iVar5 + 0x18) - *(float *)(iVar5 + 0x1c))) {
            if (1.0 < (local_b8 - *(float *)(iVar5 + 0x1c)) /
                      (*(float *)(iVar5 + 0x18) - *(float *)(iVar5 + 0x1c))) goto LAB_00513f18;
            fVar8 = (local_b8 - *(float *)(iVar5 + 0x1c)) /
                    (*(float *)(iVar5 + 0x18) - *(float *)(iVar5 + 0x1c));
          }
          else {
            fVar8 = 0.0;
          }
          local_94 = (float)bVar3 * (float)bVar4 * 0.003921569 * fVar8 * 0.003921569;
          fVar19 = (float10)FUN_00513540();
          local_50[6] = (float)(fVar19 * (float10)*(float *)(iVar5 + 0x84));
          fVar19 = (float10)fpatan((float10)fVar2 * (float10)DAT_007c12c4 +
                                   (float10)DAT_007c12c8 * (float10)fVar9 +
                                   (float10)DAT_007c12cc * (float10)fVar10,
                                   (float10)fVar2 * (float10)DAT_007c12d0 +
                                   (float10)DAT_007c12d4 * (float10)fVar9 +
                                   (float10)DAT_007c12d8 * (float10)fVar10);
          local_34 = (float)(fVar19 * (float10)57.29578);
          local_a0 = *(float *)(iVar5 + 8) - *(float *)(iVar5 + 0xc);
          if (local_a0 == 0.0) {
            local_a0 = 0.0;
          }
          else {
            local_a0 = 1.0 / local_a0;
          }
          fVar8 = -(local_a0 * *(float *)(iVar5 + 0xc));
          vector3d_normalize_with_length();
          local_50[0] = 1.0;
          local_50[1] = fVar8 - (local_5c * DAT_007c1234 +
                                DAT_007c1238 * local_58 + DAT_007c123c * local_54) * local_a0;
          if (0.0 <= local_50[1]) {
            if (local_50[1] <= 1.0) {
            }
            else {
              local_50[1] = 1.0;
            }
          }
          else {
            local_50[1] = 0.0;
          }
          local_50[2] = fVar8 - (local_5c * fVar2 + local_58 * fVar9 + local_54 * fVar10) * local_a0
          ;
          if (0.0 <= local_50[2]) {
            if (local_50[2] <= 1.0) {
            }
            else {
              local_50[2] = 1.0;
            }
          }
          else {
            local_50[2] = 0.0;
          }
          local_50[3] = (fVar2 * DAT_007c1234 + DAT_007c1238 * fVar9 + DAT_007c123c * fVar10) *
                        local_a0 + fVar8;
          if (0.0 <= local_50[3]) {
            if (local_50[3] <= 1.0) {
            }
            else {
              local_50[3] = 1.0;
            }
          }
          else {
            local_50[3] = 0.0;
          }
          if (0.0 < local_94) {
            bVar3 = (&DAT_006ce83b)[iVar1];
            sVar17 = 0;
            if (0 < *(int *)(iVar5 + 0xc4)) {
              iVar14 = 0;
              do {
                iVar6 = *(int *)(iVar5 + 200);
                iVar14 = iVar14 * 0x80;
                fVar2 = *(float *)(iVar14 + 0x34 + iVar6);
                puVar18 = (ushort *)(iVar14 + iVar6);
                fVar2 = ((*(float *)(iVar14 + 0x38 + iVar6) - fVar2) * (float)bVar3 * 0.003921569 +
                        fVar2) * local_50[*(short *)(iVar14 + 0x3c + iVar6)] * local_94;
                if (sVar17 == 0) {
                  local_94 = fVar2;
                }
                if (0.0 < fVar2) {
                  local_a4 = (*(float *)(puVar18 + 0x16) - *(float *)(puVar18 + 0x14)) *
                             (float)bVar3 * 0.003921569 + *(float *)(puVar18 + 0x14);
                  if (((*(float *)(puVar18 + 0x20) == 0.0) && (*(float *)(puVar18 + 0x22) == 0.0))
                     && ((*(float *)(puVar18 + 0x24) == 0.0 && (*(float *)(puVar18 + 0x26) == 0.0)))
                     ) {
                    color_channel_real_to_byte(fVar2);
                    local_70 = 0x3f800000;
                  }
                  else {
                    local_7c = *(float *)(puVar18 + 0x22);
                    local_78 = *(float *)(puVar18 + 0x24);
                    local_74 = *(float *)(puVar18 + 0x26);
                    local_80 = fVar2;
                    if (1 < (short)puVar18[0x39]) {
                      fVar19 = (float10)periodic_function_evaluate
                                                  ((double)((*(float *)(puVar18 + 0x3c) +
                                                            (float)_DAT_007c1200) /
                                                           *(float *)(puVar18 + 0x3a)));
                      fVar2 = (float)fVar19;
                      color_interpolate(&local_2c,(byte)puVar18[0x38] & 3,fVar2);
                      local_30 = fVar2 * *(float *)(puVar18 + 0x30) +
                                 (1.0 - fVar2) * *(float *)(puVar18 + 0x28);
                      local_80 = local_30 * local_80;
                      local_7c = local_7c * local_2c;
                      local_78 = local_78 * local_28;
                      local_74 = local_74 * local_24;
                    }
                    color_pack_argb_from_real(&local_80);
                    local_70 = *(undefined4 *)(puVar18 + 0x20);
                  }
                  if (sVar17 == 0) {
                    local_50[4] = *(float *)(iVar5 + 0xa0);
                    local_50[5] = *(float *)(iVar5 + 0xa4);
                  }
                  else {
                    local_50[5] = 1.0;
                    local_50[4] = 1.0;
                  }
                  if ((*puVar18 & 4) != 0) {
                    local_a4 = ((float)bVar4 * 0.003921569 + 1.0) * local_a4 * 0.5;
                  }
                  if ((*puVar18 & 2) != 0) {
                    local_a4 = local_a4 * local_b8;
                  }
                  fVar2 = *(float *)(puVar18 + 0xe);
                  local_20 = (fVar11 + fVar11) * fVar2 + local_6c;
                  local_1c = (fVar12 + fVar12) * fVar2 + local_68;
                  local_18 = (fVar7 + fVar7) * fVar2 + local_64;
                  cVar13 = FUN_005120f0();
                  if (cVar13 != '\0') break;
                  FUN_00512120();
                  if (((*puVar18 & 8) == 0) ||
                     (DAT_00746fbc._0_2_ = 2, -1 < (char)(&DAT_006ce83a)[iVar1])) {
                    DAT_00746fbc._0_2_ = 0;
                  }
                  rasterizer_lens_flare_quad_add(&local_20,local_a4);
                }
                sVar17 = sVar17 + 1;
                iVar14 = (int)sVar17;
              } while (iVar14 < *(int *)(iVar5 + 0xc4));
            }
          }
        }
        local_60 = local_60 + 1;
        iVar14 = (int)(short)local_60;
      } while (iVar14 < DAT_0071d134);
    }
    rasterizer_set_shader_stage_config();
    if (DAT_006e1dc0 != '\x01') {
      chimera__rasterizer_set_frustum_z_func(0,0);
    }
    rasterizer_lens_flare_batch_flush_all();
    if ((DAT_0071d278 != (int *)0x0) && ((int *)*DAT_0071d278 != (int *)0x0)) {
      (**(code **)(*(int *)*DAT_0071d278 + 0x108))();
    }
    DAT_0071d278 = (int *)0x0;
    (**(code **)(*DAT_0071d174 + 0x164))(DAT_0071d174,0);
    if (((DAT_0069c68a == '\0') && (DAT_00689426 != '\0')) && (sVar17 = 0, 0 < DAT_0071d134)) {
      iVar14 = 0;
      do {
        if (((0 < (int)(&DAT_006ce83c)[iVar14 * 10]) &&
            (((byte)(&DAT_006ce83a)[iVar14 * 0x28] & 0xff7f) == DAT_007c1220._2_2_)) &&
           ((*(int *)((&DAT_006ce818)[iVar14 * 10] + 0x10) == 0x42480000 ||
            ((*(byte *)((&DAT_006ce818)[iVar14 * 10] + 0x30) & 1) != 0)))) {
          FUN_00525ab0();
        }
        sVar17 = sVar17 + 1;
        iVar14 = (int)sVar17;
      } while (iVar14 < DAT_0071d134);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
