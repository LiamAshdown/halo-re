// particle_system_render  (Ghidra: particle_system_render)
// address 0x454bf0, size 1708 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// REWRITTEN (objdump 0x454bf0..0x45529b; the draft called the sprite builders with no arguments). EAX = the
//   particle system (particle_system_data 0x0087abd4, 0x158 each; its tag's types +0x5c count / +0x60, 0x80 each;
//   type states at system +0x58, 0x40 each). For every type whose state is live (+0x00 != -1) and not flagged
//   0x100 (+0x20), its particles (particle_system_particle_data 0x0087abd8, 0x80 each, chained through +0x04 from
//   the state's +0x3c) that are visible (+0x03) in a visible cluster (+0x18 against the bit vector 0x007c3350)
//   are drawn as sprites:
//   the position (+0x1c) goes through the camera matrix 0x007c3178 (matrix4x3_transform_point) and the velocity
//   (+0x34) through its rotation part. The current state (type +0x78 states, 0x178 each, particle +0x08) and, when
//   there is one (+0x0a), the next state blend by age/duration (+0x0c/+0x10, clamped): scale (+0x48/+0x64) and color
//   (+0x54..+0x60 / +0x70..+0x7c) scaled by the type state's +0x0c and +0x18..+0x24 -- or the current state alone
//   when the two share bitmap sequence and shader (+0xb8 +0x2a/+0x2e, +0x40). The sprite frame (+0x44) is picked
//   at random from the sequence's sprite count the first time (it holds -1.0), else wrapped into it. Each state
//   with a weight above 0.01 builds a group (bitmap +0x3c, shader +0xb8, max 2 sprites, flags 4, centroid from
//   the global origin): build_sprite_rotational for types with orientation 1 (+0x28; mode 3 with +0x20 bit 7,
//   else 1), else build_sprite (EBX data, AX sequence, CX frame; mode +0x2a, flags 1). Colors are multiplied by
//   the system color (+0x48..+0x50) unless the current state has +0xe2 set; the next state's sprite sits 0.001
//   further along z. The shader's +0x98 gets the current state's +0x80 and build_sprites_end (ESI data) closes it.
// blam-cc: EAX -> particle_system_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_effects.h"

extern data_array *particle_system_data;          // 0x0087abd4
extern data_array *particle_system_particle_data; // 0x0087abd8
extern tag_instance *tag_instances;                // 0x0087bc14
extern uint32_t cluster_visible_bits[];            // 0x007c3350
extern real_matrix4x3 render_camera_world_to_view;          // 0x007c3178
extern random_seed effect_random_seed;              // 0x00719cd4
extern real_point3d *global_zero_vector3d_pointer;  // 0x006966f8

extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0, EAX, EDX, stack
extern void build_sprite_rotational(build_sprite_data *data, uint32_t flags, int16_t first_sequence_index,
    int16_t sprite_index, real_point3d *origin, real_vector3d *axis, float rotation, float scale, ColorARGB *color,
    float fade); // 0x511b40, blam-cc: EAX, stack
extern void build_sprite(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index, int16_t mode,
    real_point3d *origin, real_vector3d *direction, float rotation, float scale, ColorARGB *color, float fade,
    uint32_t flags); // 0x511700, blam-cc: EBX, AX, CX, stack
extern void build_sprites_end(build_sprite_data *data); // 0x511620, blam-cc: ESI

static float particle_clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

// One group of sprites for one state: bitmap/shader from `state_definition`, drawn with `weight`.
static void particle_build_state_sprite(uint8_t *type, uint8_t *state_definition, uint8_t *current_state, int32_t frame,
    real_point3d *position, real_vector3d *direction, float rotation, float scale, ColorARGB *color, float weight)
{
    build_sprite_data data;
    uint32_t mode;

    data.bitmap_group_index = *(datum_index *)(state_definition + 0x3c);
    data.maximum_sprite_count = 2;
    data.shader = (uint32_t)(state_definition + 0xb8);
    data.sprite_count = 0;
    data.flags = 4;
    data.centroid = *global_zero_vector3d_pointer;
    data.group_count = 0;
    if (*(int16_t *)(type + 0x28) == 1) {
        mode = (type[0x20] & 0x80) ? 3 : 1;
        build_sprite_rotational(&data, mode, (int16_t)*(uint16_t *)(state_definition + 0x40), (int16_t)frame, position,
            direction, rotation, scale, color, weight);
    } else {
        build_sprite(&data, *(int16_t *)(state_definition + 0x40), (int16_t)frame, (int16_t)*(uint16_t *)(type + 0x2a),
            position, direction, rotation, scale, color, weight, 1);
    }
    *(uint32_t *)((uint8_t *)data.shader + 0x98) = *(uint32_t *)(current_state + 0x80);
    build_sprites_end(&data);
}

void particle_system_render(datum_index particle_system_handle)
{
    uint8_t *system = (uint8_t *)particle_system_data->data + (particle_system_handle & 0xffff) * 0x158;
    uint8_t *definition = (uint8_t *)tag_instances[((particle_system *)system)->definition_index & 0xffff].data;
    int16_t type_index;

    for (type_index = 0; type_index < *(int32_t *)(definition + 0x5c); type_index++) {
        uint8_t *type = *(uint8_t **)(definition + 0x60) + type_index * 0x80;
        uint8_t *type_state = system + 0x58 + type_index * 0x40;
        uint16_t particle_index;

        if (*(int16_t *)type_state == -1 || (*(uint32_t *)(type + 0x20) & 0x100)) {
            continue;
        }
        for (particle_index = *(uint16_t *)(type_state + 0x3c); particle_index != 0xffff; ) {
            uint8_t *particle = (uint8_t *)particle_system_particle_data->data + particle_index * 0x80;
            int16_t cluster = *(int16_t *)(particle + 0x18);

            if (particle[3] && (cluster_visible_bits[cluster >> 5] & (1u << (cluster & 0x1f)))) {
                uint8_t *states = *(uint8_t **)(type + 0x78);
                uint8_t *current = states + *(int16_t *)(particle + 8) * 0x178;
                uint8_t *next = 0;
                real_point3d position;
                real_vector3d direction;
                float fraction = 1.0f;
                float inverse = 0.0f;
                float scale;
                float color[4];
                float drawn[4];
                uint8_t *bitmap;
                uint8_t *sequence;
                int16_t sequence_index;
                int32_t frame;
                float vx = *(float *)(particle + 0x34);
                float vy = *(float *)(particle + 0x38);
                float vz = *(float *)(particle + 0x3c);
                float *m = (float *)&render_camera_world_to_view;

                matrix4x3_transform_point(&position, (real_point3d *)(particle + 0x1c), &render_camera_world_to_view);
                direction.i = vx * m[1] + vy * m[4] + vz * m[7];
                direction.j = vx * m[2] + vy * m[5] + vz * m[8];
                direction.k = vx * m[3] + vy * m[6] + vz * m[9];

                if (*(int16_t *)(particle + 0xa) == -1) {
                    scale = *(float *)(particle + 0x48) * *(float *)(type_state + 0xc);
                    color[0] = *(float *)(particle + 0x54) * *(float *)(type_state + 0x18);
                    color[1] = *(float *)(particle + 0x58) * *(float *)(type_state + 0x1c);
                    color[2] = *(float *)(particle + 0x5c) * *(float *)(type_state + 0x20);
                    color[3] = *(float *)(particle + 0x60) * *(float *)(type_state + 0x24);
                } else {
                    next = states + *(int16_t *)(particle + 0xa) * 0x178;
                    fraction = particle_clamp01(*(float *)(particle + 0xc) / *(float *)(particle + 0x10));
                    inverse = 1.0f - fraction;
                    scale = (inverse * *(float *)(particle + 0x64) + fraction * *(float *)(particle + 0x48)) *
                        *(float *)(type_state + 0xc);
                    color[0] = (inverse * *(float *)(particle + 0x70) + fraction * *(float *)(particle + 0x54)) *
                        *(float *)(type_state + 0x18);
                    color[1] = (inverse * *(float *)(particle + 0x74) + fraction * *(float *)(particle + 0x58)) *
                        *(float *)(type_state + 0x1c);
                    color[2] = (inverse * *(float *)(particle + 0x78) + fraction * *(float *)(particle + 0x5c)) *
                        *(float *)(type_state + 0x20);
                    color[3] = (inverse * *(float *)(particle + 0x7c) + fraction * *(float *)(particle + 0x60)) *
                        *(float *)(type_state + 0x24);
                    if (*(int16_t *)(current + 0xb8 + 0x2a) == *(int16_t *)(next + 0xb8 + 0x2a) &&
                        *(int16_t *)(current + 0xb8 + 0x2e) == *(int16_t *)(next + 0xb8 + 0x2e) &&
                        *(int16_t *)(current + 0x40) == *(int16_t *)(next + 0x40)) {
                        fraction = 1.0f;
                        inverse = 0.0f;
                    }
                }

                bitmap = (uint8_t *)tag_instances[*(datum_index *)(current + 0x3c) & 0xffff].data;
                sequence_index = *(int16_t *)(current + 0x40);
                if (*(int16_t *)(type + 0x28) == 1) {
                    sequence_index++;
                }
                sequence = *(uint8_t **)(bitmap + 0x58) + sequence_index * 0x40;
                if (*(uint32_t *)(particle + 0x44) == 0xbf800000) {
                    int16_t count = *(int16_t *)(sequence + 0x34);
                    int16_t picked;

                    effect_random_seed = effect_random_seed * 0x19660d + 0x3c6ef35f;
                    picked = (int16_t)(((uint32_t)count * (effect_random_seed >> 0x10)) >> 0x10);
                    *(float *)(particle + 0x44) = (float)picked;
                    frame = picked;
                } else {
                    int32_t value = (int16_t)(int32_t)*(float *)(particle + 0x44);
                    int32_t remainder = value % *(int32_t *)(sequence + 0x34);

                    frame = remainder;
                    if ((int16_t)remainder < 0) {
                        frame = (int32_t)(((uint32_t)remainder & 0xffff0000u) |
                            (uint16_t)((int16_t)remainder + *(int16_t *)(sequence + 0x34)));
                    }
                }

                if (fraction > 0.01f) {
                    drawn[0] = color[0];
                    drawn[1] = color[1];
                    drawn[2] = color[2];
                    drawn[3] = color[3];
                    if (*(int16_t *)(current + 0xe2) == 0) {
                        drawn[1] *= *(float *)&((particle_system *)system)->ambient_color;
                        drawn[2] *= *(float *)(system + 0x4c);
                        drawn[3] *= *(float *)(system + 0x50);
                    }
                    particle_build_state_sprite(type, current, current, frame, &position, &direction,
                        *(float *)(particle + 0x40), scale, (ColorARGB *)drawn, fraction);
                }
                if (inverse > 0.01f) {
                    drawn[0] = color[0];
                    drawn[1] = color[1];
                    drawn[2] = color[2];
                    drawn[3] = color[3];
                    if (*(int16_t *)(current + 0xe2) == 0) {
                        drawn[1] *= *(float *)&((particle_system *)system)->ambient_color;
                        drawn[2] *= *(float *)(system + 0x4c);
                        drawn[3] *= *(float *)(system + 0x50);
                    }
                    position.z += 0.001f;
                    particle_build_state_sprite(type, next, current, frame, &position, &direction,
                        *(float *)(particle + 0x40), scale, (ColorARGB *)drawn, inverse);
                }
            }
            particle_index = *(uint16_t *)(particle + 4);
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
