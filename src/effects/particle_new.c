// particle_new  (Ghidra: FUN_00455740, still unnamed there; named directly by types/effects.h:
//   "particle_new 0x455740 (every field)")
// address 0x455740, size 1043 bytes
// name confidence: 0.6   rewrite confidence: 0.9 (VERIFIED against objdump 0x455740..0x455b52; leaf mask added) (LOW -- see UNSURE)
// evidence: types/effects.h particle_creation_data (every field, established by this exact
//   function) and particle (every field, ditto); types/tags.h Particle (flags bit order,
//   lifespan/animation_rate/radius_animation bounds, sequence counts, physics TagDependency);
//   src/effects/effect_resolve_marker_transform.c establishes the object.nodes / first person
//   weapon globals node-transform idiom this function's two marker branches reuse;
//   src/effects/particle_system_new_at_point.c establishes object_sample_ambient_lightmap_point's
//   real signature; src/objects/effect_marker_new.c-style datum_new/tag_instances idioms.
// register convention: a particle_creation_data pointer in EDI (unaff_EDI); no other registers
//   or stack parameters are recognized by Ghidra at all.
//   // blam-cc: unaff_EDI -> creation_data
// UNSURE (heavily):
//   - The two marker-resolution branches (object marker vs first person weapon marker) call
//     matrix4x3_transform_point with only the matrix argument visible; the output pointer
//     (reconstructed as the position scratch that both other paths also write) and the input
//     point (reconstructed as creation_data->position, i.e. a marker-local offset) are guesses,
//     modeled on effect_resolve_marker_transform.c's identical node lookup arithmetic.
//   - The per-local-player visibility bitset at local_player_globals+0x58 is not named anywhere
//     in types/game.h; kept as a raw byte-offset bitset test.
//   - The world-space gravity fold reads PointPhysics+0x04 (mass_scale) and computes
//     radius^3 * mass_scale, which disagrees with types/effects.h's own prose ("scaled by the
//     current radius squared and the point_physics density at +0x04") on both the exponent and
//     the field name; the disassembly is followed here since it is the primary source.
//   - The final sequence/frame setup's arithmetic for a backwards-animating particle's starting
//     frame index is preserved exactly as decompiled without a confident semantic reading.
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)
// reconciled: R05 0x00746f90 global_globals -> ModelCollisionGeometryBSP *global_collision_bsp (ScenarioStructureBSP +0xb4; global_globals is the matg globals at 0x00746fa0)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "game.h"
#include <stdint.h> // uintptr_t only; this is a .c file, not a Ghidra-ingested header

extern data_array *particle_data;   // 0x0087abd0
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern ScenarioStructureBSP *global_structure_bsp;
extern player_globals *local_player_globals; // 0x0087a478
extern uint8_t *first_person_weapon_interfaces; // 0x006b2d98, row stride 0x1ea0
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90, passed to FUN_005013a0 in ECX
extern int32_t render_frame_index; // 0x007c3100, UNSURE: foreign module (render globals)

extern datum_index datum_new(data_array *array); // 0x4d0480, memory module
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *in, real_matrix4x3 *m); // 0x4cbde0
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point
    // 0x5013a0; blam-cc: ECX -> globals, EDX -> point, EAX -> index
extern real random_range_real(real minimum, real maximum); // 0x444af0
extern uint16_t effect_random_uint16(void); // 0x44da40, this module
extern int effect_random_int_between(int16_t minimum, int16_t maximum); // 0x44c800, this module;
    // blam-cc: ECX -> minimum, stack -> maximum
extern real particle_current_radius(datum_index particle_handle); // 0x4566f0, this module
extern uint8_t particle_next_sequence(datum_index particle_handle); // 0x455e60, this module
extern void object_sample_ambient_lightmap_point(real_point3d *point, real_vector3d *lightmap_color,
    real_vector3d *base_map_color, uint8_t wait_for_textures); // 0x4f1e60, objects module, cdecl

// Creates a new individual particle from a particle_creation_data block: resolves its spawn
// position (explicit world position, an object marker, or a first person weapon marker), rolls
// its lifespan/animation-rate/flags, folds in gravity for a world space particle, samples ambient
// (and optionally diffuse) lighting into its colour, and rolls its starting sequence and frame.
// Aborts without creating anything if the definition index is -1 or the spawn position's BSP
// leaf cannot be resolved, or if no local player can currently see the target cluster.
void particle_new(particle_creation_data *creation_data)
{
    Particle *tag;
    real_point3d position;
    int32_t leaf;
    int16_t cluster = -1; // UNSURE: upper 16 bits of the packed local the raw code reuses here
                                    // are documented "never read" padding (bsp_leaf_reference
                                    // .unknown_06), so this initializer is a formality
    uint32_t visible;

    if (creation_data->definition_index == (datum_index)0xffffffff) {
        return;
    }
    tag = (Particle *)tag_instances[(uint16_t)creation_data->definition_index].data;

    if (creation_data->object_index == (datum_index)0xffffffff) {
        position = creation_data->position;
    } else if (creation_data->first_person == 0) {
        object *obj = ((object_header *)object_data->data)[creation_data->object_index & 0xffff].data;
        real_matrix4x3 *marker = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset +
            creation_data->marker_index * 0x34); // UNSURE, see file header
        matrix4x3_transform_point(&position, &creation_data->position, marker); // UNSURE, see
                                    // file header
    } else {
        real_matrix4x3 *marker = (real_matrix4x3 *)(first_person_weapon_interfaces + 0x108c +
            creation_data->first_person_weapon_index * 0x1ea0 +
            (uint16_t)creation_data->marker_index * 0x34); // UNSURE, see file header
        matrix4x3_transform_point(&position, &creation_data->position, marker); // UNSURE, see
                                    // file header
    }

    leaf = bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)global_collision_bsp, &position);
    if (leaf == -1) {
        return;
    }
    cluster = *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer + ((uint32_t)leaf & 0x7fffffff) * 0x10 + 8); // 0x45581d

    visible = *(uint32_t *)((uint8_t *)local_player_globals + 0x58 + (cluster >> 5) * 4) &
        (1u << (cluster & 0x1f)); // UNSURE, see file header
    if (visible == 0) {
        return;
    }

    {
        datum_index handle = datum_new(particle_data);

        if (handle != (datum_index)0xffffffff) {
            particle *self = &((particle *)particle_data->data)[handle & 0xffff];
            real speed;

            self->flags = 0;
            if ((tag->flags & 0x1) != 0) { // can_animate_backwards
                self->flags |= (uint16_t)(effect_random_uint16() & 1);
            }
            if ((tag->flags & 0x400) != 0) { // random_horizontal_mirroring
                self->flags |= (uint16_t)(effect_random_uint16() & 4);
            }
            if ((tag->flags & 0x800) != 0) { // random_vertical_mirroring
                self->flags |= (uint16_t)(effect_random_uint16() & 8);
            }
            self->flags = (creation_data->third_person_only == 0) ? (self->flags & ~0x10) : (self->flags | 0x10);
            self->flags = (creation_data->first_person_only == 0) ? (self->flags & ~0x20) : (self->flags | 0x20);
            self->flags = (creation_data->first_person == 0) ? (self->flags & ~0x40) : (self->flags | 0x40);

            self->definition_index = creation_data->definition_index;
            self->first_person_weapon_index = creation_data->first_person_weapon_index;
            self->object_index = creation_data->object_index;
            self->marker_index = creation_data->marker_index;
            self->sequence_state = _particle_sequence_state_new;
            self->last_update_tick = render_frame_index;

            speed = random_range_real(tag->lifespan[0], tag->lifespan[1]);
            if (speed > 0.7f) {
                speed = (speed - 0.7f) / (real)local_player_globals->local_player_count + 0.7f;
            }
            self->lifespan = speed;

            self->animation_timer = -1.0f;

            if (tag->animation_rate[1] == 0.0f) {
                self->inverse_animation_period = 3.4028235e+38f;
            } else {
                self->inverse_animation_period = 1.0f / random_range_real(tag->animation_rate[0],
                                                                           tag->animation_rate[1]);
            }

            self->location.leaf_index = leaf;
            self->location.cluster_index = cluster;

            self->position = creation_data->position;
            self->direction = *(real_vector3d *)&creation_data->direction;
            self->velocity = creation_data->velocity;
            self->rotation = creation_data->rotation;

            if (self->object_index == (datum_index)0xffffffff) {
                real radius = particle_current_radius(handle);
                PointPhysics *physics = (PointPhysics *)tag_instances[tag->physics.tag_id.index].data;
                real fold = radius * physics->mass_scale * radius * radius; // UNSURE, see file header

                self->velocity.i = self->velocity.i + fold * creation_data->gravity.i;
                self->velocity.j = self->velocity.j + fold * creation_data->gravity.j;
                self->velocity.k = self->velocity.k + fold * creation_data->gravity.k;
            }

            self->angular_velocity = creation_data->angular_velocity;
            self->scale = creation_data->scale;
            self->color = creation_data->color;

            if ((tag->flags & 0x200) == 0 || (tag->flags & 0x40) != 0) { // !self_illuminated ||
                                    // tint_from_diffuse_texture
                real_vector3d ambient, incident;

                object_sample_ambient_lightmap_point(&position, &ambient, &incident, 0);
                if ((tag->flags & 0x200) == 0) {
                    self->color.red = self->color.red * ambient.i;
                    self->color.green = self->color.green * ambient.j;
                    self->color.blue = self->color.blue * ambient.k;
                }
                if ((tag->flags & 0x40) != 0) {
                    self->color.red = self->color.red * incident.i;
                    self->color.green = self->color.green * incident.j;
                    self->color.blue = self->color.blue * incident.k;
                }
            }

            if (particle_next_sequence(handle) != 0) {
                Bitmap *bitmap = (Bitmap *)tag_instances[tag->bitmap.tag_id.index].data;
                BitmapGroupSequence *sequence =
                    (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer + self->sequence_index;

                if ((tag->flags & 0x4) != 0) { // animation_starts_on_random_frame
                    int16_t roll = (int16_t)effect_random_int_between(0, (int16_t)sequence->sprites.count);
                    self->frame_index = roll + (((-(int16_t)((self->flags & 1) != 0)) & 2) - 1); // UNSURE,
                                    // see file header
                    return;
                }
                if ((self->flags & 1) != 0) { // animating backwards
                    self->frame_index = (int16_t)sequence->sprites.count;
                    return;
                }
                self->frame_index = -1;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x455740):

void FUN_00455740(void)

{
  float *pfVar1;
  uint *puVar2;
  char cVar3;
  ushort uVar4;
  short sVar5;
  int iVar6;
  int iVar7;
  uint *unaff_EDI;
  float10 fVar8;
  undefined8 uVar9;
  float fVar10;
  float local_24;
  float local_20;
  float local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  float local_c;
  float local_8;
  float local_4;

  if (*unaff_EDI != 0xffffffff) {
    puVar2 = *(uint **)((*unaff_EDI & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (unaff_EDI[1] == 0xffffffff) {
      local_18 = unaff_EDI[4];
      local_14 = unaff_EDI[5];
      local_10 = unaff_EDI[6];
    }
    else if ((char)unaff_EDI[3] == '\0') {
      iVar7 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI[1] & 0xffff) * 0xc);
      matrix4x3_transform_point((int)*(short *)(iVar7 + 0x1f2) + (short)unaff_EDI[2] * 0x34 + iVar7)
      ;
    }
    else {
      matrix4x3_transform_point
                ((short)unaff_EDI[2] * 0x34 + 0x108c +
                 *(short *)((int)unaff_EDI + 10) * 0x1ea0 + DAT_006b2d98);
    }
    local_24 = (float)FUN_005013a0();
    if (local_24 != -NAN) {
      sVar5 = *(short *)((int)local_24 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
      local_20 = (float)CONCAT22(local_20._2_2_,sVar5);
      if ((*(uint *)(DAT_0087a478 + 0x58 + ((int)sVar5 >> 5) * 4) & 1 << ((byte)sVar5 & 0x1f)) != 0)
      {
        uVar9 = datum_new();
        if ((uint)uVar9 != 0xffffffff) {
          iVar7 = ((uint)uVar9 & 0xffff) * 0x70 + *(int *)((int)((ulonglong)uVar9 >> 0x20) + 0x34);
          *(undefined2 *)(iVar7 + 2) = 0;
          if ((*puVar2 & 1) != 0) {
            uVar4 = FUN_0044da40();
            *(ushort *)(iVar7 + 2) = *(ushort *)(iVar7 + 2) | uVar4 & 1;
          }
          if ((*puVar2 & 0x400) != 0) {
            uVar4 = FUN_0044da40();
            *(ushort *)(iVar7 + 2) = *(ushort *)(iVar7 + 2) | uVar4 & 4;
          }
          if ((*puVar2 & 0x800) != 0) {
            uVar4 = FUN_0044da40();
            *(ushort *)(iVar7 + 2) = *(ushort *)(iVar7 + 2) | uVar4 & 8;
          }
          if (*(char *)((int)unaff_EDI + 0xd) == '\0') {
            *(byte *)(iVar7 + 2) = *(byte *)(iVar7 + 2) & 0xef;
          }
          else {
            *(byte *)(iVar7 + 2) = *(byte *)(iVar7 + 2) | 0x10;
          }
          if (*(char *)((int)unaff_EDI + 0xe) == '\0') {
            *(byte *)(iVar7 + 2) = *(byte *)(iVar7 + 2) & 0xdf;
          }
          else {
            *(byte *)(iVar7 + 2) = *(byte *)(iVar7 + 2) | 0x20;
          }
          if ((char)unaff_EDI[3] == '\0') {
            *(byte *)(iVar7 + 2) = *(byte *)(iVar7 + 2) & 0xbf;
          }
          else {
            *(byte *)(iVar7 + 2) = *(byte *)(iVar7 + 2) | 0x40;
          }
          *(uint *)(iVar7 + 4) = *unaff_EDI;
          *(undefined1 *)(iVar7 + 0xf) = *(undefined1 *)((int)unaff_EDI + 10);
          *(uint *)(iVar7 + 8) = unaff_EDI[1];
          *(short *)(iVar7 + 0xc) = (short)unaff_EDI[2];
          *(undefined1 *)(iVar7 + 0xe) = 0;
          *(undefined4 *)(iVar7 + 0x10) = DAT_007c3100;
          fVar10 = random_range_real((float)puVar2[0xe],(float)puVar2[0xf]);
          *(float *)(iVar7 + 0x18) = fVar10;
          if (0.7 < fVar10) {
            *(float *)(iVar7 + 0x18) =
                 (fVar10 - 0.7) / (float)(int)*(short *)(DAT_0087a478 + 0xc) + 0.7;
          }
          if ((float)puVar2[0x21] == 0.0) {
            fVar10 = 3.4028235e+38;
          }
          else {
            fVar10 = random_range_real((float)puVar2[0x20],(float)puVar2[0x21]);
            fVar10 = 1.0 / fVar10;
          }
          *(float *)(iVar7 + 0x20) = fVar10;
          *(float *)(iVar7 + 0x28) = local_24;
          *(float *)(iVar7 + 0x2c) = local_20;
          *(undefined4 *)(iVar7 + 0x1c) = 0xbf800000;
          *(uint *)(iVar7 + 0x30) = unaff_EDI[4];
          *(uint *)(iVar7 + 0x34) = unaff_EDI[5];
          *(uint *)(iVar7 + 0x38) = unaff_EDI[6];
          *(uint *)(iVar7 + 0x3c) = unaff_EDI[7];
          *(uint *)(iVar7 + 0x40) = unaff_EDI[8];
          *(uint *)(iVar7 + 0x44) = unaff_EDI[9];
          *(uint *)(iVar7 + 0x54) = unaff_EDI[0x10];
          *(uint *)(iVar7 + 0x5c) = unaff_EDI[0x12];
          *(uint *)(iVar7 + 0x60) = unaff_EDI[0x13];
          *(uint *)(iVar7 + 100) = unaff_EDI[0x14];
          *(uint *)(iVar7 + 0x68) = unaff_EDI[0x15];
          *(uint *)(iVar7 + 0x6c) = unaff_EDI[0x16];
          pfVar1 = (float *)(iVar7 + 0x48);
          *pfVar1 = (float)unaff_EDI[10];
          *(uint *)(iVar7 + 0x4c) = unaff_EDI[0xb];
          *(uint *)(iVar7 + 0x50) = unaff_EDI[0xc];
          if (*(int *)(iVar7 + 8) == -1) {
            fVar8 = (float10)FUN_004566f0();
            fVar8 = fVar8 * (float10)*(float *)(*(int *)((puVar2[8] & 0xffff) * 0x20 + 0x14 +
                                                        DAT_0087bc14) + 4) * fVar8 * fVar8;
            *pfVar1 = (float)(fVar8 * (float10)(float)unaff_EDI[0xd] + (float10)*pfVar1);
            *(float *)(iVar7 + 0x4c) =
                 (float)(fVar8 * (float10)(float)unaff_EDI[0xe] + (float10)*(float *)(iVar7 + 0x4c))
            ;
            *(float *)(iVar7 + 0x50) =
                 (float)(fVar8 * (float10)(float)unaff_EDI[0xf] + (float10)*(float *)(iVar7 + 0x50))
            ;
          }
          *(uint *)(iVar7 + 0x58) = unaff_EDI[0x11];
          if (((*puVar2 & 0x200) == 0) || ((*puVar2 & 0x40) != 0)) {
            object_sample_ambient_lightmap_point(&local_18,&local_24,&local_c,0);
            if ((*puVar2 & 0x200) == 0) {
              *(float *)(iVar7 + 100) = local_24 * *(float *)(iVar7 + 100);
              *(float *)(iVar7 + 0x68) = local_20 * *(float *)(iVar7 + 0x68);
              *(float *)(iVar7 + 0x6c) = local_1c * *(float *)(iVar7 + 0x6c);
            }
            if ((*puVar2 & 0x40) != 0) {
              *(float *)(iVar7 + 100) = local_c * *(float *)(iVar7 + 100);
              *(float *)(iVar7 + 0x68) = local_8 * *(float *)(iVar7 + 0x68);
              *(float *)(iVar7 + 0x6c) = local_4 * *(float *)(iVar7 + 0x6c);
            }
          }
          cVar3 = FUN_00455e60();
          if (cVar3 != '\0') {
            iVar6 = *(short *)(iVar7 + 0x24) * 0x40 +
                    *(int *)(*(int *)((puVar2[4] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x58);
            if ((*puVar2 & 4) != 0) {
              sVar5 = FUN_0044c800(*(undefined2 *)(iVar6 + 0x34));
              *(ushort *)(iVar7 + 0x26) =
                   sVar5 + ((-(ushort)((*(byte *)(iVar7 + 2) & 1) != 0) & 2) - 1);
              return;
            }
            if ((*(byte *)(iVar7 + 2) & 1) != 0) {
              *(undefined2 *)(iVar7 + 0x26) = *(undefined2 *)(iVar6 + 0x34);
              return;
            }
            *(undefined2 *)(iVar7 + 0x26) = 0xffff;
          }
        }
      }
    }
  }
  return;
}
#endif
