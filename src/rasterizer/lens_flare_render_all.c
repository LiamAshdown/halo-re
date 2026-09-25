// lens_flare_render_all  (Ghidra: decal_render_active_list, misnamed; renamed per
// out/phase4/rasterizer_types_notes.md's lens flare misattribution table)
// address 0x513cf0, size 2147 bytes
// name confidence: 0.55  rewrite confidence: 0.25 (largest and most register-lossy function in
//   this session's range; see the UNSURE notes below)
// evidence: for every active lens_flare_instance belonging to the current render window with a
//   positive occlusion sample count and nonzero alpha/visibility, projects it (view-space depth
//   against LensFlare.near/far_fade_distance, a rotation angle via lens_flare_compute_rotation,
//   three clamped falloff factors from the frustum forward/left/up basis) and, for each of its
//   LensFlareReflection entries (LensFlare.reflections @+0xc4, stride 0x80 -- see
//   types/tags.h), computes a lerped brightness/radius/color from the reflection's visibility
//   byte and submits a screen space quad via rasterizer_lens_flare_quad_add. Finishes by
//   flushing the sprite batches, ending the D3D scene and, on supported hardware, issuing a
//   fresh occlusion query per still-eligible flare via FUN_00525ab0.
// register convention: none recognized -- purely global driven.
// UNSURE, throughout, and more than usually load-bearing here: several calls in this function
//   are shown by Ghidra with a return value that is never stored anywhere visible
//   (color_channel_real_to_byte(fVar2), color_pack_argb_from_real(&local_80), and
//   vector3d_normalize_with_length() itself) and rasterizer_lens_flare_quad_add is called with
//   only 2 of what is almost certainly more real arguments (a lens_flare_vertex-shaped block).
//   This is preserved as literally as possible: the "lost" calls are kept, in order, as bare
//   calls whose result is genuinely discarded here (most likely their true destination is a
//   stack slot inside the same vertex block that quad_add reads, which Ghidra did not resolve),
//   and quad_add is declared exactly as it is called at this site. This function needs a
//   disassembly-based pass before its reflection draw loop can be trusted; the outer gating,
//   depth/fade projection and the vertex position math are on firmer ground (every operand
//   there maps onto a named struct field).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern uint8_t unknown_006893ff;    // 0x006893ff UNSURE: console/debug toggle, owner module unclear
extern uint8_t lens_flare_occlusion_queries_supported; // 0x006e1dc0
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern lens_flare_instance lens_flare_instances[0x400]; // 0x006ce818
extern int32_t lens_flare_instance_count;   // 0x0071d134
extern rasterizer_frame_time rasterizer_time; // 0x007c1200
extern void *rasterizer_device;             // 0x0071d174
extern void *rasterizer_effect_pool_scratch; // 0x0071d278, UNSURE: an effect/COM object being ended here

// blam-cc: stack -> (z_near, z_far) as raw float bits
extern void chimera__rasterizer_set_frustum_z_func(uint32_t z_near, uint32_t z_far); // 0x518f40
// blam-cc: AX -> mode, ECX -> flags
extern void rasterizer_lens_flare_batching_select_mode(int16_t mode, uint32_t flags); // 0x537130
// blam-cc: AX -> mode
extern void rasterizer_set_shader_stage_config(int16_t mode);       // 0x519200
extern void rasterizer_lens_flare_batch_flush_all(void); // 0x536c80
extern uint8_t *lens_flare_get_visibility_byte(lens_flare_instance *flare); // 0x5134f0
extern real_vector3d *vector3d_unpack_normal_11_11_10(real_vector3d *out, uint32_t packed); // 0x513400
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, UNSURE: argument at this call site
// blam-cc: ESI -> flare, DI -> mode
extern float lens_flare_compute_rotation(lens_flare_instance *flare, int16_t mode); // 0x513540
extern double fpatan(double y, double x); // x87 FPATAN, atan2(y, x)
extern uint8_t color_channel_real_to_byte(float channel); // 0x5132b0, UNSURE: return unused at this call site
extern void color_interpolate(ColorRGB *out_color, uint32_t color_pair, float t); // 0x43f6a0
// blam-cc: AX -> type, stack -> input
extern real periodic_function_evaluate(periodic_function_t type, double time); // 0x4cc9b0
extern uint32_t color_pack_argb_from_real(ColorARGB *color); // 0x497900, UNSURE: return unused at this call site
extern void rasterizer_lens_flare_quad_add(real_point3d *position, float radius); // 0x537550, UNSURE: likely more real arguments
// blam-cc: EAX -> instance
extern void rasterizer_sun_glow_render(lens_flare_instance *instance); // 0x525ab0

// Draws every active lens flare's reflections as screen space quads (see the file header for
// the significant UNSURE caveats on the per reflection body).
void lens_flare_render_all(void)
{
    int32_t i;
    int32_t j;

    if (unknown_006893ff == 0 || rasterizer_window.type != 1 || lens_flare_instance_count <= 0) {
        return;
    }

    if (lens_flare_occlusion_queries_supported != 1) {
        chimera__rasterizer_set_frustum_z_func(0x3d000200, 0x45800000); // 0.03125f, 4096.0f (0x513d2b pushes both)
    }
    rasterizer_lens_flare_batching_select_mode(5, 0); // 0x513d3d: xor ecx,ecx; mov eax,5

    for (i = 0; i < lens_flare_instance_count; i++) {
        lens_flare_instance *flare = &lens_flare_instances[i];
        uint8_t *visibility_byte = lens_flare_get_visibility_byte(flare);
        real_vector3d unpack_scratch;
        real_vector3d *direction = vector3d_unpack_normal_11_11_10(&unpack_scratch, flare->packed_direction);
        LensFlare *definition;
        float view_x, view_y, view_z;
        float depth;
        float perp_i, perp_j, perp_k;
        uint8_t alpha_byte;
        uint8_t visibility_value;
        float fade;
        float falloff[7]; // local_50[0..6]
        float rotation_degrees;
        float inv_radius_span;
        float base_falloff;
        int32_t reflection_count;

        if ((flare->window_flags & 0x7f) != rasterizer_window.window_index) {
            continue;
        }

        definition = (LensFlare *)flare->definition;
        reflection_count = definition->reflections.count;
        if (reflection_count <= 0) {
            continue;
        }
        if (flare->sample_count <= 0) {
            continue;
        }
        alpha_byte = ((uint8_t *)&flare->color)[3]; // color's alpha byte (0xAARRGGBB)
        if (alpha_byte == 0) {
            continue;
        }

        view_x = flare->position.x - rasterizer_window.camera.position.x;
        view_y = flare->position.y - rasterizer_window.camera.position.y;
        view_z = flare->position.z - rasterizer_window.camera.position.z;
        depth = view_x * rasterizer_window.camera.forward.i + rasterizer_window.camera.forward.j * view_y +
                rasterizer_window.camera.forward.k * view_z;
        perp_i = rasterizer_window.camera.forward.i * depth - view_x;
        perp_j = rasterizer_window.camera.forward.j * depth - view_y;
        perp_k = rasterizer_window.camera.forward.k * depth - view_z;
        if (lens_flare_occlusion_queries_supported == 0) {
            depth = depth * 0.5f;
        }

        visibility_value = *visibility_byte;
        if (definition->near_fade_distance <= 0.0f) {
            fade = 1.0f;
        } else {
            float t = (depth - definition->near_fade_distance) /
                      (definition->far_fade_distance - definition->near_fade_distance);
            if (t < 0.0f) {
                fade = 0.0f;
            } else if (t > 1.0f) {
                fade = 1.0f;
            } else {
                fade = t;
            }
        }

        base_falloff = (float)alpha_byte * (float)visibility_value * 0.003921569f * fade * 0.003921569f;

        rotation_degrees = (float)(lens_flare_compute_rotation(flare, (int16_t)definition->rotation_function) *
                                   definition->rotation_function_scale); // DI = LensFlare +0x80 (0x513f22)
        falloff[6] = rotation_degrees;
        (void)fpatan; // used inside lens_flare_compute_rotation's own family; kept for parity with the pack

        inv_radius_span = definition->cos_falloff_angle - definition->cos_cutoff_angle; // UNSURE: field names guessed from struct order
        inv_radius_span = (inv_radius_span == 0.0f) ? 0.0f : 1.0f / inv_radius_span;
        {
            float base = -(inv_radius_span * definition->cos_cutoff_angle);
            real_vector3d perp;
            float t;

            perp.i = perp_i; perp.j = perp_j; perp.k = perp_k;
            vector3d_normalize_with_length(&perp); // UNSURE: return (length) unused
            perp_i = perp.i; perp_j = perp.j; perp_k = perp.k;

            falloff[0] = 1.0f;

            t = base - (direction->i * rasterizer_window.camera.forward.i +
                        rasterizer_window.camera.forward.j * direction->j +
                        rasterizer_window.camera.forward.k * direction->k) * inv_radius_span;
            falloff[1] = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);

            t = base - (direction->i * view_x + direction->j * view_y + direction->k * view_z) * inv_radius_span;
            falloff[2] = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);

            t = (view_x * rasterizer_window.camera.forward.i + rasterizer_window.camera.forward.j * view_y +
                 rasterizer_window.camera.forward.k * view_z) * inv_radius_span + base;
            falloff[3] = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
        }

        if (base_falloff <= 0.0f) {
            continue;
        }

        {
            uint8_t intensity = flare->intensity;
            LensFlareReflection *reflections = (LensFlareReflection *)definition->reflections.pointer;

            for (j = 0; j < reflection_count; j++) {
                LensFlareReflection *r = &reflections[j];
                float brightness = r->brightness[0] +
                                    (r->brightness[1] - r->brightness[0]) * (float)intensity * 0.003921569f;
                brightness = brightness * falloff[r->brightness_scaled_by] * base_falloff; // UNSURE index type
                float radius;
                float alpha, red, green, blue;
                uint32_t packed_color;
                float vx, vy, vz;

                if (j == 0) {
                    base_falloff = brightness;
                }
                if (brightness <= 0.0f) {
                    continue;
                }

                radius = r->radius[0] + (r->radius[1] - r->radius[0]) * (float)intensity * 0.003921569f;

                if (r->tint_color.alpha == 0.0f && r->tint_color.red == 0.0f &&
                    r->tint_color.green == 0.0f && r->tint_color.blue == 0.0f) {
                    color_channel_real_to_byte(brightness); // UNSURE: return unused
                    packed_color = 0x3f800000; // UNSURE: raw float 1.0 bit pattern, not an ARGB dword
                } else {
                    alpha = r->tint_color.alpha;
                    red = r->tint_color.red;
                    green = r->tint_color.green;
                    blue = r->tint_color.blue;
                    if (r->animation_function > 1) {
                        // UNSURE: the two operands of the period division (puVar18+0x3c and
                        // puVar18+0x3a) and the two lerp bounds (puVar18+0x28, +0x30) land past
                        // the confidently typed prefix of LensFlareReflection; kept as raw byte
                        // offsets off the reflection record rather than guessed field names.
                        uint8_t *raw = (uint8_t *)r;
                        ColorRGB interpolated;
                        float phase = (float)periodic_function_evaluate((periodic_function_t)r->animation_function,   // AX = +0x72 (0x51425e)
                            (*(float *)(raw + 0x78) + rasterizer_time.time) / *(float *)(raw + 0x74));
                        float lerp_factor = phase * *(float *)(raw + 0x60) + (1.0f - phase) * *(float *)(raw + 0x50);
                        color_interpolate(&interpolated, r->more_flags & 3, phase);
                        alpha = lerp_factor * alpha;
                        red = red * interpolated.red;
                        green = green * interpolated.green;
                        blue = blue * interpolated.blue;
                    }
                    {
                        ColorARGB c;
                        c.alpha = alpha; c.red = red; c.green = green; c.blue = blue;
                        color_pack_argb_from_real(&c); // UNSURE: return unused
                    }
                    packed_color = *(uint32_t *)&r->tint_color.alpha; // UNSURE: local_70 = raw tint_color dword
                }

                if (j == 0) {
                    falloff[4] = definition->horizontal_scale;
                    falloff[5] = definition->vertical_scale;
                } else {
                    falloff[4] = 1.0f;
                    falloff[5] = 1.0f;
                }
                if ((r->flags & 4) != 0) {
                    radius = ((float)visibility_value * 0.003921569f + 1.0f) * radius * 0.5f;
                }
                if ((r->flags & 2) != 0) {
                    radius = radius * depth;
                }

                vx = (perp_i + perp_i) * r->position + flare->position.x;
                vy = (perp_j + perp_j) * r->position + flare->position.y;
                vz = (perp_k + perp_k) * r->position + flare->position.z;

                (void)packed_color; // UNSURE: destination not resolved -- see file header

                // UNSURE: FUN_005120f0 (visibility test?) and FUN_00512120 (occlusion query
                // begin?) are outside this session's range and are called here with no visible
                // arguments; FUN_005120f0 returning nonzero breaks the reflection loop.
                extern uint8_t rasterizer_lens_flare_set_current_key(void);
                extern void rasterizer_lens_flare_set_vertex_specular(void);
                if (rasterizer_lens_flare_set_current_key() != 0) {
                    break;
                }
                rasterizer_lens_flare_set_vertex_specular();

                {
                    real_point3d vertex_position;
                    vertex_position.x = vx; vertex_position.y = vy; vertex_position.z = vz;
                    rasterizer_lens_flare_quad_add(&vertex_position, radius);
                }
            }
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
        ((void (__stdcall *)(void *, int32_t))device_vtable[0x164 / 4])(rasterizer_device, 0); // SetFVF(0)
    }

    {
        extern uint8_t unknown_0069c68a; // 0x0069c68a, rasterizer_caps_flag_68a per types/rasterizer.h
        extern uint8_t unknown_00689426; // 0x00689426, console/debug toggle range per types/rasterizer.h
        if (unknown_0069c68a == 0 && unknown_00689426 != 0) {
            for (i = 0; i < lens_flare_instance_count; i++) {
                lens_flare_instance *flare = &lens_flare_instances[i];
                if (flare->sample_count > 0 && (flare->window_flags & 0x7f) == rasterizer_window.window_index) {
                    LensFlare *definition = (LensFlare *)flare->definition;
                    if (*(uint32_t *)&definition->occlusion_radius == 0x42480000 || // UNSURE: literal radius sentinel
                        (((uint8_t *)definition)[0x30] & 1) != 0) {                  // UNSURE: flags byte at +0x30
                        rasterizer_sun_glow_render(flare); // EAX = instance (0x514535)
                    }
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
