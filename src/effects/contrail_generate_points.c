// contrail_generate_points  (Ghidra: contrail_generate_points, already named)
// address 0x44d020, size 1094 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: types/effects.h contrail / contrail_point (every field), types/tags.h Contrail
// (point_velocity[2] 0x08, point_velocity_cone_angle 0x10, inherited_velocity_fraction 0x14,
// ContrailScaleFlags bits 1-4), Object.attachments / ObjectAttachment.marker (established by
// contrail_new.c); types/objects.h object.parent_object (0x11c, root-object walk), object.velocity
// (0x68); src/objects/antenna_apply_marker_delta.c establishes the FUN_005013a0(globals, point,
// index) and structure_bsp_globals+0xe4 leaf/cluster idioms this function also uses.
// register convention: contrail handle in EAX (in_EAX, not a pointer); point_count and force are
// the two stack arguments Ghidra already recognises (param_1, param_2).
//   // blam-cc: EAX -> contrail_handle, stack -> (point_count, force)
// UNSURE: the `direction` argument (EAX) of the vector3d_randomize_direction call is fully
// elided by Ghidra along with `out` (EBX) and `seed` (EDI); only the two stack arguments (lo,
// hi) survive in the decompile as the literal call `vector3d_randomize_direction(0,local_1f0)`.
// lo=0.0 and hi=the (possibly scaled) point_velocity_cone_angle are confident; the reference
// direction the random velocity is spread around is not recoverable from the decompiled C, and
// is reconstructed here as the attachment marker's "up" axis, which is the only nearby vector
// already in scope with a plausible physical reading (a trail sprayed outward from the marker) --
// flagged as a guess, not evidence.
// UNSURE: the interpolation branch (i < count) recomputes the new point's BSP leaf/cluster a
// second time, against the SAME still-unmodified position the first probe already used a few
// lines above. That duplicate call is preserved verbatim (same call order) even though its
// result cannot differ from the first.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *contrail_data;       // 0x0087abec
extern data_array *contrail_point_data; // 0x0087abe8
extern data_array *object_data;         // 0x008603b0
extern tag_instance *tag_instances;     // 0x0087bc14
extern void *global_globals;            // 0x00746f90, passed to FUN_005013a0 in ECX
extern uint8_t *structure_bsp_globals;  // 0x00746f9c; +0xe4 is the per-leaf lookup table
extern random_seed effect_random_seed;  // 0x00719cd4

extern datum_index datum_new(data_array *array); // 0x4d0480, memory module; blam-cc: array in EDX
extern real effect_random_scaled_range(uint32_t flags, real scale, real base_min, real base_max,
    uint8_t bit_index); // 0x44c840, this module; blam-cc: EDX -> flags, stack -> the rest
extern int32_t object_get_node_local_transform(uint32_t object_index, const char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080, established
extern int32_t FUN_005013a0(void *globals, real_point3d *point, int32_t index);
    // 0x5013a0; blam-cc: ECX -> globals, EDX -> point, EAX -> index
extern real_vector3d *vector3d_randomize_direction(real_point3d *direction, real_vector3d *out,
    random_seed *seed, real lo, real hi); // 0x4cd1b0, math module;
    // blam-cc: EAX -> direction, EBX -> out, EDI -> seed, stack -> (lo, hi)

// Subdivides a contrail's marker attachment(s): for a list that has no points yet, adds a single
// fresh point at the marker; for a list whose head point has fallen behind the marker's current
// position (or when `force` is set), adds `point_count` new points interpolated between the old
// head and the marker.
void contrail_generate_points(datum_index contrail_handle, int16_t point_count, uint8_t force)
{
    contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_handle];
    Contrail *tag = (Contrail *)tag_instances[(uint16_t)self->definition_index].data;

    if (point_count == 0) {
        return;
    }

    {
        object *owner = ((object_header *)object_data->data)[(uint16_t)self->object_index].data;
        Object *owner_tag = (Object *)tag_instances[(uint16_t)owner->definition_tag].data;
        ObjectAttachment *attachment = (ObjectAttachment *)owner_tag->attachments.pointer +
            self->attachment_index;
        object_marker markers[4];
        int16_t marker_count = (int16_t)object_get_node_local_transform(self->object_index,
            attachment->marker.string, markers, 4);

        if (marker_count > 0) {
            real velocity_magnitude = effect_random_scaled_range(tag->scale_flags, self->scale,
                tag->point_velocity[0], tag->point_velocity[1], 1);
            real cone_angle = tag->point_velocity_cone_angle;
            real inherited_fraction = tag->inherited_velocity_fraction;
            int list;

            if ((tag->scale_flags & (1u << 3)) != 0) {
                cone_angle = cone_angle * self->scale;
            }
            if ((tag->scale_flags & (1u << 4)) != 0) {
                inherited_fraction = inherited_fraction * self->scale;
            }

            for (list = 0; list < marker_count; list++) {
                object_marker *marker = &markers[list];
                datum_index *head = &self->first_point[list];
                contrail_point *previous = 0;
                int16_t count;

                if (*head == k_datum_index_none) {
                    count = 1;
                } else {
                    contrail_point *current = &((contrail_point *)contrail_point_data->data)[(uint16_t)*head];
                    int unchanged = current->position.x == marker->node_transform.position.x &&
                                    current->position.y == marker->node_transform.position.y &&
                                    current->position.z == marker->node_transform.position.z;

                    if (unchanged && force == 0) {
                        continue;
                    }
                    previous = current;
                    count = point_count;
                }

                {
                    int i;
                    for (i = 1; i <= count; i++) {
                        datum_index new_handle = datum_new(contrail_point_data);

                        if (new_handle != k_datum_index_none) {
                            contrail_point *point =
                                &((contrail_point *)contrail_point_data->data)[(uint16_t)new_handle];
                            real_vector3d direction;

                            point->age = 0.0f;
                            point->inverse_duration = 0.0f;
                            point->flags = _contrail_point_skip_render_bit | _contrail_point_in_transition_bit;
                            point->state_index = -1;
                            point->scale = self->scale;

                            vector3d_randomize_direction((real_point3d *)&marker->node_transform.up,
                                &direction, &effect_random_seed, 0.0f, cone_angle);

                            point->position = marker->node_transform.position;

                            {
                                int32_t leaf = FUN_005013a0(global_globals, &point->position, 0);
                                point->location.leaf_index = leaf;
                                point->location.cluster_index = (leaf == -1) ? -1 :
                                    *(int16_t *)(*(uint8_t **)(structure_bsp_globals + 0xe4) +
                                        (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);
                            }

                            {
                                object *root = ((object_header *)object_data->data)[(uint16_t)self->object_index].data;
                                while (root->parent_object != k_datum_index_none) {
                                    root = ((object_header *)object_data->data)[(uint16_t)root->parent_object].data;
                                }
                                point->velocity.i = direction.i * velocity_magnitude + inherited_fraction * root->velocity.i;
                                point->velocity.j = direction.j * velocity_magnitude + inherited_fraction * root->velocity.j;
                                point->velocity.k = direction.k * velocity_magnitude + inherited_fraction * root->velocity.k;
                            }

                            if (i < count) {
                                real fraction = (real)i / (real)count;
                                real inverse_fraction = 1.0f - fraction;
                                real_point3d sampled_position = point->position;
                                real_vector3d sampled_velocity = point->velocity;
                                int32_t leaf;

                                point->scale = fraction * point->scale + inverse_fraction * previous->scale;

                                leaf = FUN_005013a0(global_globals, &point->position, 0);
                                point->location.leaf_index = leaf;
                                point->location.cluster_index = (leaf == -1) ? -1 :
                                    *(int16_t *)(*(uint8_t **)(structure_bsp_globals + 0xe4) +
                                        (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);

                                point->position.x = fraction * sampled_position.x + inverse_fraction * previous->position.x;
                                point->position.y = fraction * sampled_position.y + inverse_fraction * previous->position.y;
                                point->position.z = fraction * sampled_position.z + inverse_fraction * previous->position.z;
                                point->velocity.i = fraction * sampled_velocity.i + inverse_fraction * previous->velocity.i;
                                point->velocity.j = fraction * sampled_velocity.j + inverse_fraction * previous->velocity.j;
                                point->velocity.k = fraction * sampled_velocity.k + inverse_fraction * previous->velocity.k;
                            }

                            point->next_point = *head;
                            self->point_count[list] = self->point_count[list] + 1;
                            *head = new_handle;
                        }
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x44d020):

void contrail_generate_points(short param_1,char param_2)

{
  float *pfVar1;
  short *psVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  uint uVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  short sVar13;
  ushort uVar14;
  undefined2 uVar15;
  short sVar16;
  uint in_EAX;
  int iVar17;
  int iVar18;
  uint uVar19;
  int iVar20;
  int *piVar21;
  int iVar22;
  int *piVar23;
  bool bVar24;
  float10 fVar25;
  undefined8 uVar26;
  float local_204;
  undefined1 *local_200;
  uint *local_1f8;
  float local_1f0;
  int local_1ec;
  int local_1e4;
  uint local_1e0;
  float local_1bc;
  float local_1b8;
  float local_1b4;
  undefined1 local_1b0 [432];

  iVar17 = (in_EAX & 0xffff) * 0x44 + *(int *)(DAT_0087abec + 0x34);
  iVar18 = *(int *)((*(uint *)(iVar17 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((param_1 != 0) &&
     (uVar14 = object_get_node_local_transform
                         (*(uint *)(iVar17 + 8),
                          *(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                                        (*(uint *)(iVar17 + 8) & 0xffff) * 0xc) &
                                            0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x144) + 0x10 +
                          *(short *)(iVar17 + 0xc) * 0x48,local_1b0,4), 0 < (short)uVar14)) {
    fVar25 = (float10)FUN_0044c840(*(undefined4 *)(iVar17 + 0x10),*(undefined4 *)(iVar18 + 8),
                                   *(undefined4 *)(iVar18 + 0xc),1);
    local_1f0 = *(float *)(iVar18 + 0x10);
    fVar3 = (float)fVar25;
    if ((*(ushort *)(iVar18 + 2) & 8) != 0) {
      local_1f0 = local_1f0 * *(float *)(iVar17 + 0x10);
    }
    local_204 = *(float *)(iVar18 + 0x14);
    if ((*(ushort *)(iVar18 + 2) & 0x10) != 0) {
      local_204 = local_204 * *(float *)(iVar17 + 0x10);
    }
    if (0 < (short)uVar14) {
      local_1e0 = (uint)uVar14;
      local_200 = local_1b0;
      local_1f8 = (uint *)(iVar17 + 0x34);
      local_1e4 = 0;
      do {
        if (*local_1f8 == 0xffffffff) {
          iVar18 = 0;
          sVar13 = 1;
LAB_0044d18c:
          sVar16 = 1;
          if (0 < sVar13) {
            local_1ec = 1;
            do {
              uVar26 = datum_new();
              uVar19 = (uint)uVar26;
              if (uVar19 != 0xffffffff) {
                iVar22 = (uVar19 & 0xffff) * 0x38 +
                         *(int *)((int)((ulonglong)uVar26 >> 0x20) + 0x34);
                *(undefined4 *)(iVar22 + 4) = 0;
                *(undefined4 *)(iVar22 + 8) = 0;
                *(undefined1 *)(iVar22 + 2) = 3;
                *(undefined1 *)(iVar22 + 3) = 0xff;
                *(undefined4 *)(iVar22 + 0xc) = *(undefined4 *)(iVar17 + 0x10);
                vector3d_randomize_direction(0,local_1f0);
                pfVar1 = (float *)(iVar22 + 0x1c);
                *pfVar1 = *(float *)(local_200 + 0x60);
                *(undefined4 *)(iVar22 + 0x20) = *(undefined4 *)(local_200 + 100);
                *(undefined4 *)(iVar22 + 0x24) = *(undefined4 *)(local_200 + 0x68);
                iVar20 = FUN_005013a0();
                *(int *)(iVar22 + 0x14) = iVar20;
                if (iVar20 == -1) {
                  uVar15 = 0xffff;
                }
                else {
                  uVar15 = *(undefined2 *)(iVar20 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
                }
                *(undefined2 *)(iVar22 + 0x18) = uVar15;
                iVar20 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                 (*(uint *)(iVar17 + 8) & 0xffff) * 0xc);
                uVar8 = *(uint *)(iVar20 + 0x11c);
                while (uVar8 != 0xffffffff) {
                  iVar20 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar8 & 0xffff) * 0xc);
                  uVar8 = *(uint *)(iVar20 + 0x11c);
                }
                fVar9 = *(float *)(iVar20 + 0x6c);
                fVar10 = *(float *)(iVar20 + 0x70);
                *(float *)(iVar22 + 0x28) =
                     local_1bc * fVar3 + local_204 * *(float *)(iVar20 + 0x68);
                *(float *)(iVar22 + 0x2c) = local_1b8 * fVar3 + local_204 * fVar9;
                *(float *)(iVar22 + 0x30) = local_1b4 * fVar3 + local_204 * fVar10;
                if (sVar16 < sVar13) {
                  fVar11 = (float)local_1ec / (float)(int)sVar13;
                  fVar12 = 1.0 - fVar11;
                  *(float *)(iVar22 + 0xc) =
                       fVar11 * *(float *)(iVar22 + 0xc) + fVar12 * *(float *)(iVar18 + 0xc);
                  fVar9 = *(float *)(iVar18 + 0x1c);
                  fVar10 = *pfVar1;
                  fVar4 = *(float *)(iVar18 + 0x20);
                  fVar5 = *(float *)(iVar22 + 0x20);
                  fVar6 = *(float *)(iVar18 + 0x24);
                  fVar7 = *(float *)(iVar22 + 0x24);
                  iVar20 = FUN_005013a0();
                  *(int *)(iVar22 + 0x14) = iVar20;
                  if (iVar20 == -1) {
                    uVar15 = 0xffff;
                  }
                  else {
                    uVar15 = *(undefined2 *)(iVar20 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
                  }
                  *(undefined2 *)(iVar22 + 0x18) = uVar15;
                  *pfVar1 = fVar11 * fVar10 + fVar12 * fVar9;
                  *(float *)(iVar22 + 0x20) = fVar11 * fVar5 + fVar12 * fVar4;
                  *(float *)(iVar22 + 0x24) = fVar11 * fVar7 + fVar12 * fVar6;
                  *(float *)(iVar22 + 0x28) =
                       fVar11 * *(float *)(iVar22 + 0x28) + fVar12 * *(float *)(iVar18 + 0x28);
                  *(float *)(iVar22 + 0x2c) =
                       fVar11 * *(float *)(iVar22 + 0x2c) + fVar12 * *(float *)(iVar18 + 0x2c);
                  *(float *)(iVar22 + 0x30) =
                       fVar11 * *(float *)(iVar22 + 0x30) + fVar12 * *(float *)(iVar18 + 0x30);
                }
                *(uint *)(iVar22 + 0x34) = *local_1f8;
                psVar2 = (short *)(iVar17 + 0x2c + local_1e4 * 2);
                *psVar2 = *psVar2 + 1;
                *local_1f8 = uVar19;
              }
              sVar16 = sVar16 + 1;
              local_1ec = local_1ec + 1;
            } while (sVar16 <= sVar13);
          }
        }
        else {
          iVar18 = (*local_1f8 & 0xffff) * 0x38 + *(int *)(DAT_0087abe8 + 0x34);
          sVar13 = param_1;
          if (iVar18 == 0) goto LAB_0044d18c;
          iVar20 = 3;
          bVar24 = true;
          piVar21 = (int *)(local_200 + 0x60);
          piVar23 = (int *)(iVar18 + 0x1c);
          do {
            if (iVar20 == 0) break;
            iVar20 = iVar20 + -1;
            bVar24 = *piVar21 == *piVar23;
            piVar21 = piVar21 + 1;
            piVar23 = piVar23 + 1;
          } while (bVar24);
          if ((!bVar24) || (param_2 != '\0')) goto LAB_0044d18c;
        }
        local_1e4 = local_1e4 + 1;
        local_1f8 = local_1f8 + 1;
        local_200 = local_200 + 0x6c;
        local_1e0 = local_1e0 - 1;
      } while (local_1e0 != 0);
    }
  }
  return;
}
#endif
