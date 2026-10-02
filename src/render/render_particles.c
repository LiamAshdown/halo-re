// render_particles  (Ghidra: contrail_render_all_active, phase-2 name, wrong: it never touches
// contrails; CEA render_particles(void), hint only; renamed)
// address 0x50fd90, size 1645 bytes
// VERIFIED against disassembly 0x50fd90..0x5103fd (2026-09-30)
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: objdump -d -M intel 0x50fd90..0x510403 (__chkstk 0x24f4 frame), every stack slot
//   followed. Types: types/effects.h particle (0x70), types/render.h rendered_particle_datum,
//   build_sprite_data, types/tags.h Particle (bitmap id +0x10, fade_in_time +0x40,
//   fade_out_time +0x44, minimum_size +0x68, radius_animation +0x74 / +0x78, fade_end_size
//   +0x90, sprite_size +0xa8, orientation +0xac, the shader block +0xb0).
//   - gated on the particle toggle 0x0069c565. The viewer is current_local_player_index
//     (0x007c3108), or 1 when that is -1 or render_local_player_gunner_seat_visible 0x50fcd0
//     (EAX = the index, AL result) says no.
//   - collect: every live particle (datum_next 0x4d0630, then its inlined copy) whose cluster
//     bit is set in the visible cluster bitset 0x007c3350, skipping flag 0x10 particles owned
//     by the viewer (first_person_weapon_index +0x0f == viewer) and flag 0x20 particles that
//     are not; the record is {index, definition low word, cluster, owned && flag 0x20}.
//   - sort with the std::sort instantiation 0x510410 (first, last, count, predicate slot; the
//     predicate is an empty object and the pushed dword is whatever ECX held, the viewer).
//   - group equal (definition, cluster, first person) runs into at most 0x200 counts. The
//     "current count" pointer starts as the viewer value (mov edi,[esp+0x24] at 0x50ff00), so
//     a first record equal to the initial key (0xffff, 0xffff, 0) would increment through a
//     non pointer; that cannot happen for a live particle and is modelled as NULL here.
//   - per group, an inlined build_sprites_begin: bitmap = Particle bitmap, maximum = group size,
//     shader = Particle + 0xb0, flags = 4 | (first person ? 2 : 0), centroid = *global_zero_vector3d_pointer.
//   - per particle: radius = lerp(radius_animation, age / lifespan) * scale; the position and
//     direction come straight from the particle for a world particle (object_index -1, which is
//     then stored again), through the first person weapon node matrix
//     (0x006b2d98 + weapon * 0x1ea0 + 0x108c + marker * 0x34) for flag 0x40, or through the
//     object node matrix (object + object.nodes.offset (+0x1f2) + marker * 0x34) after an
//     inlined salt checked datum_get of the object; a dead object deletes the particle
//     (datum_delete 0x4d0510, EAX array, EDX index). The matrix scale is applied to the
//     position only when it is not exactly 1.0 (compared as bits).
//   - screen size = projection_world_to_screen.j (0x007c32f0) / max(|view z|, 0.1) * 2 radius,
//     view z from world_to_view (0x007c3184 / 0x007c3190 / 0x007c319c / 0x007c31a8); drawn only
//     above fade_end_size; scale = 2 radius * sprite_size, grown to minimum_size; fade by age
//     against fade_in_time and by remaining life against fade_out_time; mirror flags from
//     particle flags 0x04 / 0x08. build_sprite 0x511700 (EBX data, AX sequence +0x24, CX frame
//     +0x26, stack (orientation, &position, &direction, +0x54 rotation, scale, &color +0x60,
//     fade, flags)); then particle.last_update_tick = render_frame_index (0x007c3100).
//   - the average radius of the drawn particles goes to shader + 0x98, then build_sprites_end
//     0x511620 (ESI data).
// register convention: none (void).
//   // blam-cc: void
// UNSURE: particle +0x3c is used as the sprite direction (effects.h calls it unknown_3c) and
//   +0x54 as the rotation (unknown_54).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "effects.h"
#include "interface.h"
#include "render.h"
#include "shaders.h"
#include <stdint.h> // uintptr_t, for the 32 bit pointer fields of build_sprite_data

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t particle_spawn_debug_mode;          // 0x0069c565 particles enabled
extern int16_t current_local_player_index;            // 0x007c3108, this module
extern data_array *particle_data;                    // 0x0087abd0, effects module
extern uint32_t cluster_visible_bits[0x10];          // 0x007c3350, structures module
extern tag_instance *tag_instances;                  // 0x0087bc14
extern real_point3d *global_zero_vector3d_pointer;                // 0x006966f8 -> {0,0,0}
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98
extern data_array *object_data;                      // 0x008603b0, objects module
extern render_frustum render_frustum_global;         // 0x007c3168, this module
extern int32_t render_frame_index;                   // 0x007c3100, this module

extern int16_t render_local_player_gunner_seat_visible(int16_t local_player_index);
    // 0x50fcd0, this module; blam-cc: EAX -> local_player_index; the caller tests AL only
extern datum_index datum_next(int16_t after_index, data_array *array);
    // 0x4d0630, memory module; blam-cc: DX=after_index, EDI=array
extern void datum_delete(data_array *array, datum_index index);
    // 0x4d0510, memory module; blam-cc: EAX -> array, EDX -> index (sign extended word here)
extern void sort_introsort_loop(rendered_particle_datum *first, rendered_particle_datum *last,
                                int32_t ideal, int32_t predicate);
    // 0x510410, MSVC STL std::sort instantiation (rewritten in src/render/sort_introsort_loop.c); cdecl
extern void build_sprite(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index,
                         int16_t mode, real_point3d *origin, real_vector3d *direction,
                         float rotation, float scale, ColorARGB *color, float fade, uint32_t flags);
    // 0x511700, this module; blam-cc: EBX=data, AX=sequence_index, CX=sprite_index, stack=rest
extern void build_sprites_end(build_sprite_data *data); // 0x511620, this module; ESI

// Draws every visible particle: collects them, sorts them into runs that share a definition,
// cluster and first person state, and builds one sprite batch per run.
void render_particles(void)
{
    rendered_particle_datum records[k_maximum_rendered_particles];
    uint16_t group_counts[k_maximum_rendered_particle_groups];
    build_sprite_data data;
    int16_t record_count = 0;
    int16_t viewer;
    int32_t viewer_value;
    datum_index index;

    if (!particle_spawn_debug_mode) {
        return;
    }
    viewer = current_local_player_index;
    if (viewer == -1 || !(uint8_t)render_local_player_gunner_seat_visible(viewer)) {
        viewer = 1;
    }
    viewer_value = (int32_t)viewer;

    for (index = datum_next(-1, particle_data); index != 0xffffffff;
         index = datum_next((int16_t)index, particle_data)) {
        particle *p = &((particle *)particle_data->data)[(uint16_t)index];
        int32_t cluster = (int32_t)p->location.cluster_index;
        uint8_t owned = (int32_t)p->first_person_weapon_index == viewer_value;

        if ((cluster_visible_bits[cluster >> 5] & (1u << (cluster & 0x1f))) == 0) {
            continue;
        }
        if ((p->flags & 0x10) != 0 && owned) {
            continue;
        }
        if ((p->flags & 0x20) != 0 && !owned) {
            continue;
        }
        records[record_count].particle_index = (uint16_t)index;
        records[record_count].definition_index = (uint16_t)p->definition_index;
        records[record_count].cluster_index = p->location.cluster_index;
        records[record_count].first_person = (owned && (p->flags & 0x20) != 0) ? 1 : 0;
        record_count++;
    }

    if (record_count <= 0) {
        return;
    }
    sort_introsort_loop(records, records + record_count, record_count, viewer_value);

    {
        int16_t group_count = 0;
        int16_t remaining = record_count;
        uint16_t key_definition = 0xffff;
        uint16_t key_cluster = 0xffff;
        uint8_t key_first_person = 0;
        uint16_t *current_count = 0; // see the header: the binary holds the viewer value here
        rendered_particle_datum *record = records;
        rendered_particle_datum *group_first;
        int16_t group;

        do {
            remaining--;
            if (record->definition_index == key_definition &&
                (uint16_t)record->cluster_index == key_cluster &&
                record->first_person == key_first_person) {
                (*current_count)++;
            } else {
                if (group_count >= k_maximum_rendered_particle_groups) {
                    break;
                }
                key_cluster = (uint16_t)record->cluster_index;
                key_first_person = record->first_person;
                current_count = &group_counts[group_count];
                group_count++;
                *current_count = 1;
                key_definition = record->definition_index;
            }
            record++;
        } while (remaining > 0);

        group_first = records;
        for (group = 0; group < group_count; group++) {
            Particle *definition = (Particle *)tag_instances[group_first->definition_index].data;
            int16_t in_group = (int16_t)group_counts[group];
            int16_t drawn = 0;
            float radius_sum = 0.0f;
            float average;
            int16_t k;

            data.bitmap_group_index = *(datum_index *)&definition->bitmap.tag_id;
            data.maximum_sprite_count = in_group;
            data.shader = (uint32_t)(uintptr_t)((uint8_t *)definition + 0xb0);
            data.centroid = *global_zero_vector3d_pointer;
            data.flags = (group_first->first_person ? _build_sprite_data_flag_1_bit : 0) |
                         _build_sprite_data_flag_2_bit;
            data.sprite_count = 0;
            data.group_count = 0;

            for (k = 0; k < in_group; k++, group_first++) {
                uint16_t particle_index = group_first->particle_index;
                particle *p = &((particle *)particle_data->data)[particle_index];
                Particle *pd = (Particle *)tag_instances[(uint16_t)p->definition_index].data;
                float radius = ((pd->radius_animation[1] - pd->radius_animation[0]) *
                                (p->age / p->lifespan) + pd->radius_animation[0]) * p->scale;
                real_point3d origin;
                real_vector3d direction;
                float view_z;
                float pixels;

                if (p->object_index == 0xffffffff) {
                    origin = p->position;
                    direction = p->direction;
                    p->object_index = 0xffffffff;
                } else {
                    real_matrix4x3 *m = 0;
                    real_point3d pos;
                    real_vector3d dir;

                    if ((p->flags & _particle_first_person_bit) != 0) {
                        m = (real_matrix4x3 *)((uint8_t *)&first_person_weapon_interfaces[
                                p->first_person_weapon_index] + 0x108c +
                                (int32_t)p->marker_index * 0x34);
                    } else {
                        int16_t object_slot = (int16_t)p->object_index;
                        int16_t salt = (int16_t)(p->object_index >> 16);

                        if (object_slot >= 0 && object_slot < object_data->maximum_count) {
                            object_header *header = (object_header *)((uint8_t *)object_data->data +
                                (int32_t)object_data->size * (int32_t)object_slot);

                            if (header->identifier != 0 &&
                                (salt == 0 || header->identifier == salt) &&
                                (1 << (header->type & 0x1f)) != 0 && header->data != 0) {
                                object *o = ((object_header *)object_data->data)[
                                    (uint16_t)p->object_index].data;

                                m = (real_matrix4x3 *)((uint8_t *)o + o->nodes.offset +
                                                       (int32_t)p->marker_index * 0x34);
                            }
                        }
                        if (m == 0) {
                            datum_delete(particle_data, (datum_index)(int32_t)(int16_t)particle_index);
                            continue;
                        }
                    }

                    pos = p->position;
                    if (*(uint32_t *)&m->scale != 0x3f800000) {
                        pos.x = pos.x * m->scale;
                        pos.y = pos.y * m->scale;
                        pos.z = pos.z * m->scale;
                    }
                    origin.x = pos.z * m->up.i + pos.y * m->left.i + pos.x * m->forward.i + m->position.x;
                    origin.y = pos.x * m->forward.j + pos.z * m->up.j + pos.y * m->left.j + m->position.y;
                    origin.z = pos.z * m->up.k + pos.y * m->left.k + pos.x * m->forward.k + m->position.z;
                    dir = p->direction;
                    direction.i = dir.k * m->up.i + dir.j * m->left.i + dir.i * m->forward.i;
                    direction.j = dir.k * m->up.j + dir.j * m->left.j + dir.i * m->forward.j;
                    direction.k = dir.k * m->up.k + dir.j * m->left.k + dir.i * m->forward.k;
                }

                view_z = origin.z * render_frustum_global.world_to_view.up.k +
                         origin.y * render_frustum_global.world_to_view.left.k +
                         origin.x * render_frustum_global.world_to_view.forward.k +
                         render_frustum_global.world_to_view.position.z;
                if (view_z < 0.0f) {
                    view_z = -view_z;
                }
                if (view_z <= 0.1f) {
                    view_z = 0.1f;
                }
                pixels = render_frustum_global.projection_world_to_screen.j / view_z * radius;
                pixels = pixels + pixels;

                if (pixels > definition->fade_end_size) {
                    float fade = 1.0f;
                    float scale = (radius + radius) * definition->sprite_size;
                    float remaining_life = p->lifespan - p->age;
                    uint32_t flags;

                    drawn++;
                    radius_sum = radius + radius_sum;
                    if (pixels < definition->minimum_size) {
                        scale = (definition->minimum_size / pixels) * scale;
                    }
                    if (definition->fade_in_time > 0.0f && p->age < definition->fade_in_time) {
                        fade = p->age / definition->fade_in_time;
                    }
                    if (definition->fade_out_time > 0.0f && remaining_life < definition->fade_out_time) {
                        fade = (remaining_life / definition->fade_out_time) * fade;
                    }
                    flags = ((uint32_t)(uint8_t)p->flags >> 1) & _build_sprite_mirror_u_bit;
                    if ((p->flags & _particle_mirror_vertical_bit) != 0) {
                        flags |= _build_sprite_mirror_v_bit;
                    } else {
                        flags &= ~(uint32_t)_build_sprite_mirror_v_bit;
                    }
                    build_sprite(&data, p->sequence_index, p->frame_index,
                                 (int16_t)(uint16_t)definition->orientation, &origin, &direction,
                                 p->rotation, scale, &p->color, fade, flags);
                    p->last_update_tick = render_frame_index;
                }
            }

            if (drawn != 0) {
                average = radius_sum / (float)drawn;
            } else {
                average = 0.0f;
            }
            ((shader_effect *)(uintptr_t)data.shader)->average_particle_radius = average;
            build_sprites_end(&data);
        }
    }
}

#if 0
Original Ghidra decompilation (0x50fd90):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void contrail_render_all_active(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  float fVar5;
  char cVar6;
  undefined1 uVar7;
  byte bVar8;
  short sVar9;
  uint uVar10;
  ushort *puVar11;
  short *psVar12;
  ushort *puVar13;
  float *pfVar14;
  int iVar15;
  ushort uVar16;
  int iVar17;
  int iVar18;
  int iVar19;
  ushort uVar20;
  ushort uVar21;
  short sVar22;
  short sVar23;
  ushort uVar24;
  bool bVar25;
  ushort *local_24f4;
  uint local_24f0;
  float local_24e8;
  uint local_24e4;
  ushort *local_24e0;
  float local_24dc;
  float local_24c8;
  float local_24c4;
  float local_24c0;
  float local_24bc;
  float local_24b8;
  float local_24b4;
  float local_24b0;
  undefined4 local_24ac;
  ushort local_24a8;
  int local_24a4;
  undefined2 local_24a0;
  uint local_249c;
  undefined4 local_2498;
  undefined4 local_2494;
  undefined4 local_2490;
  undefined2 local_248c;
  ushort local_2404 [512];
  ushort local_2004 [3];
  undefined1 local_1ffe [8186];
  undefined4 uStack_4;
  
  uStack_4 = 0x50fd9a;
  sVar23 = 0;
  if (DAT_0069c565 != '\0') {
    sVar9 = (short)_DAT_007c3108;
    if ((sVar9 == -1) || (cVar6 = FUN_0050fcd0(), cVar6 == '\0')) {
      sVar9 = 1;
    }
    iVar4 = DAT_0087abd0;
    uVar10 = datum_next();
    if (uVar10 != 0xffffffff) {
      puVar11 = (ushort *)(int)sVar9;
      do {
        iVar17 = (uVar10 & 0xffff) * 0x70;
        sVar9 = *(short *)(iVar17 + 0x2c + *(int *)(iVar4 + 0x34));
        iVar17 = iVar17 + *(int *)(iVar4 + 0x34);
        bVar25 = (ushort *)(uint)*(byte *)(iVar17 + 0xf) != puVar11;
        if (((((&DAT_007c3350)[(int)sVar9 >> 5] & 1 << ((byte)sVar9 & 0x1f)) != 0) &&
            (((*(ushort *)(iVar17 + 2) & 0x10) == 0 || (bVar25)))) &&
           (((*(ushort *)(iVar17 + 2) & 0x20) == 0 || (!bVar25)))) {
          iVar15 = (int)sVar23;
          sVar23 = sVar23 + 1;
          local_2004[iVar15 * 4] = (ushort)uVar10;
          local_2004[iVar15 * 4 + 1] = *(ushort *)(iVar17 + 4);
          *(undefined2 *)(local_1ffe + iVar15 * 8 + -2) = *(undefined2 *)(iVar17 + 0x2c);
          if ((bVar25) || ((*(byte *)(iVar17 + 2) & 0x20) == 0)) {
            uVar7 = 0;
          }
          else {
            uVar7 = 1;
          }
          local_1ffe[iVar15 * 8] = uVar7;
        }
        iVar17 = uVar10 + 1;
        uVar10 = 0xffffffff;
        sVar9 = (short)iVar17;
        if ((-1 < sVar9) && (sVar9 < *(short *)(iVar4 + 0x2e))) {
          psVar12 = (short *)((int)sVar9 * (int)*(short *)(iVar4 + 0x22) + *(int *)(iVar4 + 0x34));
          do {
            if (*psVar12 != 0) {
              uVar10 = (int)*psVar12 << 0x10 | (int)(short)iVar17;
              break;
            }
            iVar17 = iVar17 + 1;
            psVar12 = (short *)((int)psVar12 + (int)*(short *)(iVar4 + 0x22));
          } while ((short)iVar17 < *(short *)(iVar4 + 0x2e));
        }
      } while (uVar10 != 0xffffffff);
      if (0 < sVar23) {
        uVar24 = 0;
        sort_introsort_loop(local_2004,local_2004 + sVar23 * 4,
                            (int)(local_2004 + sVar23 * 4) - (int)local_2004 >> 3,puVar11);
        uVar16 = 0xffff;
        cVar6 = '\0';
        puVar13 = local_2004 + 2;
        uVar20 = 0xffff;
        do {
          sVar23 = sVar23 + -1;
          uVar21 = puVar13[-1];
          if (((uVar21 == uVar20) && (*puVar13 == uVar16)) && ((char)puVar13[1] == cVar6)) {
            *puVar11 = *puVar11 + 1;
            uVar21 = uVar20;
          }
          else {
            if (0x1ff < (short)uVar24) break;
            uVar16 = *puVar13;
            cVar6 = (char)puVar13[1];
            puVar11 = local_2404 + (short)uVar24;
            uVar24 = uVar24 + 1;
            *puVar11 = 1;
          }
          puVar13 = puVar13 + 4;
          uVar20 = uVar21;
        } while (0 < sVar23);
        local_24f4 = local_2004;
        if (0 < (short)uVar24) {
          local_24f0 = (uint)uVar24;
          local_24e0 = local_2404;
          do {
            iVar4 = *(int *)((uint)local_24f4[1] * 0x20 + 0x14 + DAT_0087bc14);
            local_24ac = *(undefined4 *)(iVar4 + 0x10);
            local_24a8 = *local_24e0;
            local_24a4 = iVar4 + 0xb0;
            local_2498 = *(undefined4 *)PTR_DAT_006966f8;
            local_2494 = *(undefined4 *)(PTR_DAT_006966f8 + 4);
            local_2490 = *(undefined4 *)(PTR_DAT_006966f8 + 8);
            local_249c = -(uint)((char)local_24f4[3] != '\0') & 2 | 4;
            local_24c8 = 0.0;
            sVar23 = 0;
            local_248c = 0;
            local_24a0 = 0;
            if ((short)local_24a8 < 1) {
LAB_005103c6:
              local_24c8 = 0.0;
            }
            else {
              local_24e4 = (uint)local_24a8;
              do {
                iVar15 = *(int *)(DAT_0087abd0 + 0x34);
                iVar18 = (uint)*local_24f4 * 0x70;
                iVar17 = iVar18 + iVar15;
                iVar19 = *(int *)((*(uint *)(iVar17 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
                uVar10 = *(uint *)(iVar17 + 8);
                fVar5 = ((*(float *)(iVar19 + 0x78) - *(float *)(iVar19 + 0x74)) *
                         (*(float *)(iVar18 + 0x14 + iVar15) / *(float *)(iVar18 + 0x18 + iVar15)) +
                        *(float *)(iVar19 + 0x74)) * *(float *)(iVar17 + 0x5c);
                if (uVar10 == 0xffffffff) {
                  local_24c4 = *(float *)(iVar17 + 0x30);
                  local_24c0 = *(float *)(iVar17 + 0x34);
                  local_24bc = *(float *)(iVar17 + 0x38);
                  local_24b8 = *(float *)(iVar17 + 0x3c);
                  local_24b4 = *(float *)(iVar17 + 0x40);
                  local_24b0 = *(float *)(iVar17 + 0x44);
                  *(undefined4 *)(iVar17 + 8) = 0xffffffff;
LAB_00510222:
                  fVar1 = local_24c4 * _DAT_007c3184 +
                          local_24c0 * _DAT_007c3190 + local_24bc * _DAT_007c319c + _DAT_007c31a8;
                  if (fVar1 < 0.0) {
                    fVar1 = -fVar1;
                  }
                  if (fVar1 <= 0.1) {
                    fVar1 = 0.1;
                  }
                  fVar1 = (_DAT_007c32f0 / fVar1) * fVar5;
                  fVar1 = fVar1 + fVar1;
                  if (*(float *)(iVar4 + 0x90) < fVar1) {
                    sVar23 = sVar23 + 1;
                    local_24e8 = 1.0;
                    local_24dc = (fVar5 + fVar5) * *(float *)(iVar4 + 0xa8);
                    fVar2 = *(float *)(iVar17 + 0x18) - *(float *)(iVar17 + 0x14);
                    local_24c8 = fVar5 + local_24c8;
                    if (fVar1 < *(float *)(iVar4 + 0x68)) {
                      local_24dc = (*(float *)(iVar4 + 0x68) / fVar1) * local_24dc;
                    }
                    if ((0.0 < *(float *)(iVar4 + 0x40)) &&
                       (*(float *)(iVar17 + 0x14) < *(float *)(iVar4 + 0x40))) {
                      local_24e8 = *(float *)(iVar17 + 0x14) / *(float *)(iVar4 + 0x40);
                    }
                    if ((0.0 < *(float *)(iVar4 + 0x44)) && (fVar2 < *(float *)(iVar4 + 0x44))) {
                      local_24e8 = (fVar2 / *(float *)(iVar4 + 0x44)) * local_24e8;
                    }
                    bVar8 = (byte)*(ushort *)(iVar17 + 2) >> 1 & 2;
                    if ((*(ushort *)(iVar17 + 2) & 8) != 0) {
                      bVar8 = bVar8 | 4;
                    }
                    render_billboard_quad_build
                              (*(undefined2 *)(iVar4 + 0xac),&local_24c4,&local_24b8,
                               *(undefined4 *)(iVar17 + 0x54),local_24dc,iVar17 + 0x60,local_24e8,
                               bVar8);
                    *(undefined4 *)(iVar17 + 0x10) = DAT_007c3100;
                  }
                }
                else {
                  if ((*(byte *)(iVar17 + 2) & 0x40) != 0) {
                    pfVar14 = (float *)(*(short *)(iVar17 + 0xc) * 0x34 + 0x108c +
                                       (uint)*(byte *)(iVar17 + 0xf) * 0x1ea0 + DAT_006b2d98);
LAB_0051011c:
                    fVar1 = *(float *)(iVar17 + 0x30);
                    fVar2 = *(float *)(iVar17 + 0x34);
                    fVar3 = *(float *)(iVar17 + 0x38);
                    if (*pfVar14 != 1.0) {
                      fVar1 = fVar1 * *pfVar14;
                      fVar2 = fVar2 * *pfVar14;
                      fVar3 = fVar3 * *pfVar14;
                    }
                    local_24c4 = fVar1 * pfVar14[1] + fVar2 * pfVar14[4] + fVar3 * pfVar14[7] +
                                 pfVar14[10];
                    local_24c0 = fVar2 * pfVar14[5] + fVar3 * pfVar14[8] + fVar1 * pfVar14[2] +
                                 pfVar14[0xb];
                    local_24bc = fVar1 * pfVar14[3] + fVar2 * pfVar14[6] + fVar3 * pfVar14[9] +
                                 pfVar14[0xc];
                    fVar1 = *(float *)(iVar17 + 0x3c);
                    fVar2 = *(float *)(iVar17 + 0x40);
                    fVar3 = *(float *)(iVar17 + 0x44);
                    local_24b8 = fVar1 * pfVar14[1] + fVar2 * pfVar14[4] + fVar3 * pfVar14[7];
                    local_24b4 = fVar1 * pfVar14[2] + fVar2 * pfVar14[5] + fVar3 * pfVar14[8];
                    local_24b0 = fVar1 * pfVar14[3] + fVar2 * pfVar14[6] + fVar3 * pfVar14[9];
                    goto LAB_00510222;
                  }
                  sVar9 = (short)uVar10;
                  if ((-1 < sVar9) && (sVar9 < *(short *)(DAT_008603b0 + 0x20))) {
                    iVar15 = *(int *)(DAT_008603b0 + 0x34);
                    iVar19 = (int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar9;
                    sVar9 = *(short *)(iVar19 + iVar15);
                    iVar19 = iVar19 + iVar15;
                    if ((((sVar9 != 0) &&
                         ((sVar22 = (short)(uVar10 >> 0x10), sVar22 == 0 || (sVar9 == sVar22)))) &&
                        (1 << (*(byte *)(iVar19 + 3) & 0x1f) != 0)) && (*(int *)(iVar19 + 8) != 0))
                    {
                      iVar15 = *(int *)(iVar15 + 8 + (uVar10 & 0xffff) * 0xc);
                      pfVar14 = (float *)((int)*(short *)(iVar15 + 0x1f2) +
                                         *(short *)(iVar17 + 0xc) * 0x34 + iVar15);
                      goto LAB_0051011c;
                    }
                  }
                  datum_delete();
                }
                local_24f4 = local_24f4 + 4;
                local_24e4 = local_24e4 - 1;
              } while (local_24e4 != 0);
              if (sVar23 == 0) goto LAB_005103c6;
              local_24c8 = local_24c8 / (float)(int)sVar23;
            }
            *(float *)(local_24a4 + 0x98) = local_24c8;
            FUN_00511620();
            local_24e0 = local_24e0 + 1;
            local_24f0 = local_24f0 - 1;
          } while (local_24f0 != 0);
        }
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
