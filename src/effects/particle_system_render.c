// particle_system_render  (Ghidra: FUN_00454bf0, still unnamed there; named directly by
//   types/effects.h: "particle_system_render 0x454bf0 (location, position, direction, frame)")
// address 0x454bf0, size 1708 bytes
// name confidence: 0.6   rewrite confidence: 0.15 (VERY LOW -- deep renderer internals, see
//   UNSURE)
// evidence: types/effects.h particle_system (type_states[4] +0x58, particle_system_particle
//   +0x80 stride via 0x0087abd8), particle_system_type_state (state_index +0x00, first_particle
//   +0x3c), particle_system_particle (active +0x03, state_index +0x08, next_state_index +0x0a,
//   state_time_remaining +0x0c, state_duration +0x10, location +0x14, direction +0x34, frame
//   +0x44, next_particle +0x04); types/tags.h ParticleSystem.particle_types (+0x5c, count/
//   pointer), ParticleSystemType (size 0x80, particle_states reflexive +0x74/+0x78),
//   ParticleSystemTypeParticleState (size 0x178, bitmaps.tag_id +0x3c, sequence_index +0x40);
//   src/objects/glow_render.c and antenna_render_wire.c establish build_sprite
//   and build_sprites_end's "called opaquely, no fixed prototype" idiom, reused here for
//   build_sprite_rotational too.
// register convention: particle system handle in EAX (in_EAX).
//   // blam-cc: EAX -> particle_system_handle
// UNSURE (heavily, essentially the whole render-facing half of this function): kept as a
//   literal, offset-for-offset transliteration of the decompile past the point where fields are
//   confidently named above, with no invented field names or reinterpreted semantics --
//   following the same policy src/objects/object_sample_ambient_lightmap_point.c states for its
//   own renderer-adjacent body. In particular: the ParticleSystemTypeParticleState offsets past
//   +0xb8 (used both as comparison keys and as a write target) fall inside a range types/tags.h
//   documents as unread padding, so tag data appears to be written here, which cannot be right;
//   nothing in this batch identifies the real owner, so it is left as a raw offset exactly as
//   decompiled. The two render call sites' argument shapes (`local_c8`, `&local_bc`, `&local_d8`
//   / `&local_e8`) are passed through as opaque local buffers at the same relative stack
//   positions Ghidra shows, not as named structures.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include <stdint.h> // intptr_t only; this is a .c file, not a Ghidra-ingested header

extern data_array *particle_system_data;          // 0x0087abd4
extern data_array *particle_system_particle_data; // 0x0087abd8
extern tag_instance *tag_instances;                // 0x0087bc14
extern uint8_t *visible_cluster_bitset;            // 0x007c3350, UNSURE: foreign module; a
                                    // render-frame-scoped cluster visibility bitset, distinct
                                    // from local_player_globals+0x58
extern real_matrix4x3 *camera_render_basis;        // 0x007c3178, UNSURE: foreign module (render
                                    // globals); passed whole to matrix4x3_transform_point and
                                    // its 9 rotation floats reused individually right after
extern random_seed effect_random_seed;             // 0x00719cd4
extern const uint32_t k_particle_render_constant[3]; // 0x006966f8, UNSURE: 3 dwords copied
                                    // verbatim into every quad/segment descriptor

extern void matrix4x3_transform_point(real_point3d *out, real_point3d *in, real_matrix4x3 *m); // 0x4cbde0
extern void build_sprite_rotational(); // 0x511b40, render module; UNSURE, no fixed
                                    // prototype -- see file header
extern void build_sprite(); // 0x511700, render module; UNSURE, no fixed
                                    // prototype -- see src/objects/glow_render.c
extern void build_sprites_end(void); // 0x511620, render module; UNSURE, called opaquely

// Renders every live particle of every particle type slot of one particle system. See the file
// header: this is a low confidence, offset-for-offset transliteration past the point where
// fields are solidly named.
void particle_system_render(datum_index particle_system_handle)
{
    particle_system *system =
        &((particle_system *)particle_system_data->data)[(uint16_t)particle_system_handle];
    ParticleSystem *system_tag = (ParticleSystem *)tag_instances[(uint16_t)system->definition_index].data;
    int32_t type_index;

    for (type_index = 0; type_index < (int32_t)system_tag->particle_types.count; type_index++) {
        particle_system_type_state *type_state = &system->type_states[type_index];
        ParticleSystemType *type_tag =
            (ParticleSystemType *)system_tag->particle_types.pointer + type_index;
        uint16_t particle_index;

        if (type_state->state_index == -1 || (type_tag->flags & 0x100) != 0) {
            continue;
        }

        particle_index = (uint16_t)type_state->first_particle;
        while (particle_index != 0xffff) {
            particle_system_particle *p =
                &((particle_system_particle *)particle_system_particle_data->data)[particle_index];

            if (p->active != 0 &&
                (visible_cluster_bitset[p->location.leaf_index >> 5] &
                    (1u << (p->location.leaf_index & 0x1f))) != 0) {
                uint8_t *state = (uint8_t *)((ParticleSystemTypeParticleState *)
                    type_tag->particle_states.pointer + p->state_index);
                uint8_t *next_state = (uint8_t *)0;
                real local_bc, local_b8, local_b4;   // camera-space direction (see below)
                real local_120, local_11c;           // state / next-state blend fraction
                real local_110, local_104, local_100, local_fc, local_f8; // radius, r, g, b, a
                real local_c8[2]; // opaque 8-byte block passed to the render calls verbatim
                int32_t local_118; // resolved frame index within the sequence
                real fVar2 = p->direction.i, fVar3 = p->direction.j, fVar4 = p->direction.k;

                matrix4x3_transform_point((real_point3d *)0, (real_point3d *)0, camera_render_basis);
                    // UNSURE: out/in dropped by Ghidra; this call's own result is not visibly
                    // used again, see file header. camera_render_basis is a real_matrix4x3
                    // (scale at +0x00 is never referenced, matching the manual re-derivation of
                    // its rotation below directly from forward/left/up).
                local_bc = fVar2 * camera_render_basis->forward.i + fVar3 * camera_render_basis->left.i +
                    fVar4 * camera_render_basis->up.i;
                local_b8 = fVar2 * camera_render_basis->forward.j + fVar3 * camera_render_basis->left.j +
                    fVar4 * camera_render_basis->up.j;
                local_b4 = fVar2 * camera_render_basis->forward.k + fVar3 * camera_render_basis->left.k +
                    fVar4 * camera_render_basis->up.k;

                if (p->next_state_index == -1) {
                    local_110 = *(real *)(state + 0x48) * type_state->scale;   // UNSURE field
                    local_104 = *(real *)(state + 0x54) * type_state->color.red;
                    local_100 = *(real *)(state + 0x58) * type_state->color.green;
                    local_fc = *(real *)(state + 0x5c) * type_state->color.blue;
                    local_f8 = *(real *)(state + 0x60) * type_state->color.alpha;
                    local_120 = 1.0f;
                    local_11c = 0.0f;
                } else {
                    local_120 = p->state_time_remaining / p->state_duration;
                    next_state = (uint8_t *)((ParticleSystemTypeParticleState *)
                        type_tag->particle_states.pointer + p->next_state_index);

                    if (local_120 < 0.0f) {
                        local_120 = 0.0f;
                    } else if (local_120 > 1.0f) {
                        local_120 = 1.0f;
                    }
                    local_11c = 1.0f - local_120;

                    local_110 = (local_120 * *(real *)(state + 0x48) +
                        local_11c * *(real *)(next_state + 0x64)) * type_state->scale;
                    local_104 = (local_120 * *(real *)(state + 0x54) +
                        local_11c * *(real *)(next_state + 0x70)) * type_state->color.red;
                    local_100 = (local_120 * *(real *)(state + 0x58) +
                        local_11c * *(real *)(next_state + 0x74)) * type_state->color.green;
                    local_fc = (local_120 * *(real *)(state + 0x5c) +
                        local_11c * *(real *)(next_state + 0x78)) * type_state->color.blue;
                    local_f8 = (local_120 * *(real *)(state + 0x60) +
                        local_11c * *(real *)(next_state + 0x7c)) * type_state->color.alpha;

                    if ((intptr_t)state != -0xb8 && next_state != (uint8_t *)0 &&
                        // UNSURE: original compares the raw pointer-shaped int against the
                        // literal -0xb8, which reads as a sentinel/underflow guard rather than a
                        // meaningful pointer value; preserved exactly as decompiled
                        *(int16_t *)(state + 0xe2) == *(int16_t *)(next_state + 0xe2) &&
                        *(int16_t *)(state + 0xe6) == *(int16_t *)(next_state + 0xe6) &&
                        *(int16_t *)(state + 0x40) == *(int16_t *)(next_state + 0x40)) {
                        local_120 = 1.0f;
                        local_11c = 0.0f;
                    }
                }

                {
                    int16_t sequence_index = *(int16_t *)(state + 0x40);

                    if (type_tag->complex_sprite_render_mode == 1) {
                        sequence_index = sequence_index + 1;
                    }

                    {
                        Bitmap *bitmap = (Bitmap *)
                            tag_instances[(*(uint32_t *)(state + 0x3c)) & 0xffff].data;
                        BitmapGroupSequence *sequence =
                            (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer + sequence_index;
                        int32_t sprite_count = sequence->sprites.count;

                        if (*(int32_t *)(state + 0x44) == -0x40800000 /* -1.0f bit pattern */) {
                            effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
                            *(real *)(state + 0x44) =
                                (real)(int16_t)((int32_t)sprite_count * (int32_t)(effect_random_seed >> 16) >> 16);
                            local_118 = (int32_t)*(real *)(state + 0x44);
                        } else {
                            int32_t rotation_as_int = (int32_t)*(real *)(state + 0x44);
                            local_118 = rotation_as_int % sprite_count;
                            if (local_118 < 0) {
                                local_118 = local_118 + sprite_count;
                            }
                        }
                    }
                }

                if (local_120 > 0.01f) {
                    real local_d8, local_d4, local_d0, local_cc;
                    uint32_t quad_mode;

                    local_d8 = local_104;
                    local_d0 = local_fc;
                    local_d4 = local_100;
                    local_cc = local_f8;
                    if (*(int16_t *)(state + 0xe2) == 0) {
                        local_d4 = local_100 * system->ambient_color.red;
                        local_d0 = local_fc * system->ambient_color.green;
                        local_cc = local_f8 * system->ambient_color.blue;
                    }

                    quad_mode = 1;
                    if (type_tag->complex_sprite_render_mode == 1) {
                        if (*(int8_t *)((uint8_t *)type_tag + 0x20) < 0) {
                            quad_mode = 3;
                        }
                        build_sprite_rotational(quad_mode, *(int16_t *)(state + 0x40),
                            local_118, local_c8, &local_bc, *(uint32_t *)((uint8_t *)p + 0x40),
                            local_110, &local_d8, local_120);
                    } else {
                        build_sprite(*(int16_t *)((uint8_t *)type_tag + 0x2a),
                            local_c8, &local_bc, *(uint32_t *)((uint8_t *)p + 0x40), local_110,
                            &local_d8, local_120, 1);
                    }
                    *(uint32_t *)(state + 0x150) = *(uint32_t *)(state + 0x80); // UNSURE, see
                                    // file header -- lands inside ParticleSystemTypeParticleState's
                                    // own documented padding, both source and destination keyed
                                    // off the CURRENT state regardless of branch
                    build_sprites_end();
                }

                if (local_11c > 0.01f && next_state != (uint8_t *)0) {
                    real local_e8, local_e4, local_e0, local_dc;
                    uint32_t quad_mode;

                    local_e4 = local_100;
                    local_dc = local_f8;
                    local_e8 = local_104;
                    local_e0 = local_fc;
                    if (*(int16_t *)(state + 0xe2) == 0) {
                        local_e4 = local_100 * system->ambient_color.red;
                        local_e0 = local_fc * system->ambient_color.green;
                        local_dc = local_f8 * system->ambient_color.blue;
                    }

                    quad_mode = 1;
                    if (type_tag->complex_sprite_render_mode == 1) {
                        if ((*(uint8_t *)((uint8_t *)type_tag + 0x20) & 0x80) != 0) {
                            quad_mode = 3;
                        }
                        build_sprite_rotational(quad_mode, *(int16_t *)(next_state + 0x40),
                            local_118, local_c8, &local_bc, *(uint32_t *)((uint8_t *)p + 0x40),
                            local_110, &local_e8, local_11c);
                    } else {
                        build_sprite(*(int16_t *)((uint8_t *)type_tag + 0x2a),
                            local_c8, &local_bc, *(uint32_t *)((uint8_t *)p + 0x40), local_110,
                            &local_e8, local_11c, 1);
                    }
                    *(uint32_t *)(next_state + 0x150) = *(uint32_t *)(state + 0x80); // UNSURE, see
                                    // file header -- destination is the NEXT state's cache here,
                                    // source is still the CURRENT state's, matching the decompile
                    build_sprites_end();
                }
            }

            particle_index = (uint16_t)p->next_particle;
        }
    }
}

#if 0
Original Ghidra decompilation (0x454bf0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00454bf0(void)

{
  short *psVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  ushort uVar5;
  uint uVar6;
  short sVar7;
  uint in_EAX;
  int iVar8;
  int iVar9;
  undefined4 uVar10;
  short sVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  float local_120;
  float local_11c;
  undefined4 local_118;
  int local_114;
  float local_110;
  int local_10c;
  float local_104;
  float local_100;
  float local_fc;
  float local_f8;
  float local_e8;
  float local_e4;
  float local_e0;
  float local_dc;
  float local_d8;
  float local_d4;
  float local_d0;
  float local_cc;
  undefined1 local_c8 [8];
  float local_c0;
  float local_bc;
  float local_b8;
  float local_b4;
  int local_b0;
  undefined4 local_ac;
  undefined2 local_a8;
  int local_a4;
  undefined2 local_a0;
  undefined4 local_9c;
  undefined4 local_98;
  undefined4 local_94;
  undefined4 local_90;
  undefined2 local_8c;

  iVar8 = (in_EAX & 0xffff) * 0x158;
  iVar9 = iVar8 + *(int *)(DAT_0087abd4 + 0x34);
  iVar8 = *(int *)((*(uint *)(iVar8 + 8 + *(int *)(DAT_0087abd4 + 0x34)) & 0xffff) * 0x20 + 0x14 +
                  DAT_0087bc14);
  iVar13 = 0;
  sVar11 = 0;
  local_b0 = iVar8;
  if (0 < *(int *)(iVar8 + 0x5c)) {
    do {
      psVar1 = (short *)(iVar13 * 0x40 + 0x58 + iVar9);
      iVar13 = iVar13 * 0x80 + *(int *)(iVar8 + 0x60);
      if ((*psVar1 != -1) && ((*(uint *)(iVar13 + 0x20) & 0x100) == 0)) {
        uVar5 = psVar1[0x1e];
        while (uVar5 != 0xffff) {
          iVar8 = (uint)uVar5 * 0x80 + *(int *)(DAT_0087abd8 + 0x34);
          if ((*(char *)((uint)uVar5 * 0x80 + 3 + *(int *)(DAT_0087abd8 + 0x34)) != '\0') &&
             (((&DAT_007c3350)[(int)*(short *)(iVar8 + 0x18) >> 5] &
              1 << ((byte)*(short *)(iVar8 + 0x18) & 0x1f)) != 0)) {
            iVar12 = *(short *)(iVar8 + 8) * 0x178 + *(int *)(iVar13 + 0x78);
            local_114 = 0;
            matrix4x3_transform_point(&DAT_007c3178);
            fVar2 = *(float *)(iVar8 + 0x34);
            fVar3 = *(float *)(iVar8 + 0x38);
            fVar4 = *(float *)(iVar8 + 0x3c);
            local_bc = fVar4 * _DAT_007c3194 + fVar3 * _DAT_007c3188 + fVar2 * DAT_007c317c;
            local_b8 = fVar4 * _DAT_007c3198 + fVar3 * _DAT_007c318c + fVar2 * _DAT_007c3180;
            local_b4 = fVar4 * _DAT_007c319c + fVar3 * _DAT_007c3190 + fVar2 * _DAT_007c3184;
            if (*(short *)(iVar8 + 10) == -1) {
              local_10c = 0;
              local_110 = *(float *)(iVar8 + 0x48) * *(float *)(psVar1 + 6);
              local_104 = *(float *)(iVar8 + 0x54) * *(float *)(psVar1 + 0xc);
              local_100 = *(float *)(iVar8 + 0x58) * *(float *)(psVar1 + 0xe);
              local_fc = *(float *)(iVar8 + 0x5c) * *(float *)(psVar1 + 0x10);
              local_f8 = *(float *)(iVar8 + 0x60) * *(float *)(psVar1 + 0x12);
LAB_00454ea9:
              local_120 = 1.0;
              local_11c = 0.0;
            }
            else {
              local_120 = *(float *)(iVar8 + 0xc) / *(float *)(iVar8 + 0x10);
              local_10c = *(short *)(iVar8 + 10) * 0x178 + *(int *)(iVar13 + 0x78);
              local_114 = local_10c + 0xb8;
              if (0.0 <= local_120) {
                if (1.0 < local_120) {
                  local_120 = 1.0;
                }
              }
              else {
                local_120 = 0.0;
              }
              local_11c = 1.0 - local_120;
              local_110 = (local_120 * *(float *)(iVar8 + 0x48) +
                          local_11c * *(float *)(iVar8 + 100)) * *(float *)(psVar1 + 6);
              local_104 = (local_120 * *(float *)(iVar8 + 0x54) +
                          local_11c * *(float *)(iVar8 + 0x70)) * *(float *)(psVar1 + 0xc);
              local_100 = (local_120 * *(float *)(iVar8 + 0x58) +
                          local_11c * *(float *)(iVar8 + 0x74)) * *(float *)(psVar1 + 0xe);
              local_fc = (local_120 * *(float *)(iVar8 + 0x5c) +
                         local_11c * *(float *)(iVar8 + 0x78)) * *(float *)(psVar1 + 0x10);
              local_f8 = (local_120 * *(float *)(iVar8 + 0x60) +
                         local_11c * *(float *)(iVar8 + 0x7c)) * *(float *)(psVar1 + 0x12);
              if ((((iVar12 != -0xb8) && (local_114 != 0)) &&
                  (*(short *)(iVar12 + 0xe2) == *(short *)(local_10c + 0xe2))) &&
                 ((*(short *)(iVar12 + 0xe6) == *(short *)(local_10c + 0xe6) &&
                  (*(short *)(iVar12 + 0x40) == *(short *)(local_10c + 0x40))))) goto LAB_00454ea9;
            }
            sVar7 = *(short *)(iVar12 + 0x40);
            if (*(short *)(iVar13 + 0x28) == 1) {
              sVar7 = sVar7 + 1;
            }
            iVar14 = sVar7 * 0x40 +
                     *(int *)(*(int *)((*(uint *)(iVar12 + 0x3c) & 0xffff) * 0x20 + 0x14 +
                                      DAT_0087bc14) + 0x58);
            if (*(int *)(iVar8 + 0x44) == -0x40800000) {
              DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
              *(float *)(iVar8 + 0x44) =
                   (float)(int)(short)((int)*(short *)(iVar14 + 0x34) * (DAT_00719cd4 >> 0x10) >>
                                      0x10);
              local_118 = __ftol();
            }
            else {
              sVar7 = __ftol();
              local_118 = (int)sVar7 % *(int *)(iVar14 + 0x34);
              sVar7 = (short)local_118;
              if (sVar7 < 0) {
                uVar6 = (uint)local_118 >> 0x10;
                local_118 = CONCAT22((short)uVar6,sVar7 + *(short *)(iVar14 + 0x34));
              }
            }
            if (0.01 < local_120) {
              local_d8 = local_104;
              local_d0 = local_fc;
              local_d4 = local_100;
              local_cc = local_f8;
              if (*(short *)(iVar12 + 0xe2) == 0) {
                local_d4 = local_100 * *(float *)(iVar9 + 0x48);
                local_d0 = local_fc * *(float *)(iVar9 + 0x4c);
                local_cc = local_f8 * *(float *)(iVar9 + 0x50);
              }
              local_ac = *(undefined4 *)(iVar12 + 0x3c);
              local_8c = 0;
              local_a0 = 0;
              local_a4 = iVar12 + 0xb8;
              local_98 = *(undefined4 *)PTR_DAT_006966f8;
              local_94 = *(undefined4 *)(PTR_DAT_006966f8 + 4);
              local_90 = *(undefined4 *)(PTR_DAT_006966f8 + 8);
              uVar10 = 1;
              local_a8 = 2;
              local_9c = 4;
              if (*(short *)(iVar13 + 0x28) == 1) {
                if (*(char *)(iVar13 + 0x20) < '\0') {
                  uVar10 = 3;
                }
                contrail_draw_segment_blended
                          (uVar10,*(undefined2 *)(iVar12 + 0x40),local_118,local_c8,&local_bc,
                           *(undefined4 *)(iVar8 + 0x40),local_110,&local_d8,local_120);
              }
              else {
                render_billboard_quad_build
                          (*(undefined2 *)(iVar13 + 0x2a),local_c8,&local_bc,
                           *(undefined4 *)(iVar8 + 0x40),local_110,&local_d8,local_120,1);
              }
              *(undefined4 *)(local_a4 + 0x98) = *(undefined4 *)(iVar12 + 0x80);
              FUN_00511620();
            }
            if (0.01 < local_11c) {
              local_e4 = local_100;
              local_dc = local_f8;
              local_e8 = local_104;
              local_e0 = local_fc;
              if (*(short *)(iVar12 + 0xe2) == 0) {
                local_e4 = local_100 * *(float *)(iVar9 + 0x48);
                local_e0 = local_fc * *(float *)(iVar9 + 0x4c);
                local_dc = local_f8 * *(float *)(iVar9 + 0x50);
              }
              local_ac = *(undefined4 *)(local_10c + 0x3c);
              local_c0 = local_c0 + 0.001;
              local_8c = 0;
              local_a0 = 0;
              local_a4 = local_114;
              local_98 = *(undefined4 *)PTR_DAT_006966f8;
              local_94 = *(undefined4 *)(PTR_DAT_006966f8 + 4);
              local_90 = *(undefined4 *)(PTR_DAT_006966f8 + 8);
              uVar10 = 1;
              local_a8 = 2;
              local_9c = 4;
              if (*(short *)(iVar13 + 0x28) == 1) {
                if ((*(byte *)(iVar13 + 0x20) & 0x80) != 0) {
                  uVar10 = 3;
                }
                contrail_draw_segment_blended
                          (uVar10,*(undefined2 *)(local_10c + 0x40),local_118,local_c8,&local_bc,
                           *(undefined4 *)(iVar8 + 0x40),local_110,&local_e8,local_11c);
              }
              else {
                render_billboard_quad_build
                          (*(undefined2 *)(iVar13 + 0x2a),local_c8,&local_bc,
                           *(undefined4 *)(iVar8 + 0x40),local_110,&local_e8,local_11c,1);
              }
              *(undefined4 *)(local_a4 + 0x98) = *(undefined4 *)(iVar12 + 0x80);
              FUN_00511620();
            }
          }
          uVar5 = *(ushort *)(iVar8 + 4);
          iVar8 = local_b0;
        }
      }
      sVar11 = sVar11 + 1;
      iVar13 = (int)sVar11;
    } while (iVar13 < *(int *)(iVar8 + 0x5c));
  }
  return;
}
#endif
