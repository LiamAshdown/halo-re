// ai_search_gather_obstacles  (Ghidra: ai_search_gather_obstacles, renamed)
// address 0x43c510, size 883 bytes
// name confidence: 0.45  rewrite confidence: 0.8
// REWRITTEN from objdump 0x43c510..0x43c882. Stack: list, center, radius, direction, self_object_a,
//   self_object_b. object_find_in_sphere(1, types 0xc3, self_object_a's location +0x98, center, radius, 0x100)
//   gives the candidates; the two selves, hidden objects (+0x10 bit 0), bipeds flagged +0x106 bit 2, and machines
//   (type 7) whose tag (+0x292) does not block paths or is fully open (bit 2 with +0x208 == 1.0) are skipped, as
//   are objects whose bounding sphere (+0xa0, +0xac) is out of reach and those without pathfinding spheres (the
//   collision model at tag +0x7c: +0x280 count, +0x284 array of 0x20) or with tag flag +0x02 bit 3. Each sphere
//   is placed in the world (its node's matrix, or the object's world matrix for node -1; radius scaled), dropped
//   when it lies wholly below the center (unless the direction points down, k <= -0.2) or above it (unless
//   k >= 0.2) with 0.5 slack, or out of reach (z counted twice), and appended; a biped moving along the direction
//   (d . dir > 0 and velocity . dir > 1/15) is appended with flag 1.
// blam-cc: stack -> list, center, radius, direction, self_object_a, self_object_b

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"
#include <stdint.h>

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int16_t object_find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location,
    real_point3d *center, float radius, datum_index *out_objects, int16_t max_output); // 0x4f6fe0
extern int point3d_within_radius(const real_point3d *a, const real_point3d *b, real radius); // 0x43c340, EAX, ECX, stack
extern real_matrix4x3 *object_get_world_matrix(uint32_t object_index, real_matrix4x3 *out); // 0x4f6a20, EAX, EDI
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern uint8_t ai_search_append_obstacle(ai_search_obstacle_list *list, uint16_t flags, uint32_t object_index,
    real_point2d *position, float radius); // 0x43c4b0, EDX list, ESI position, stack

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

void ai_search_gather_obstacles(ai_search_obstacle_list *list, real_point3d *center,
                                float radius, real_vector3d *direction, uint32_t self_object_a, uint32_t self_object_b)
{
    datum_index found[0x100];
    int16_t count;
    int16_t f;

    count = object_find_in_sphere(1, 0xc3, OBJECT_DATA(self_object_a) + 0x98, center, radius, found, 0x100);
    for (f = 0; f < count; f++) {
        datum_index object_index = found[f];
        uint8_t *object = OBJECT_DATA(object_index);
        uint8_t *object_tag;
        uint8_t *collision;
        real_matrix4x3 world;
        int32_t s;

        if (object_index == self_object_a || object_index == self_object_b || (object[0x10] & 1) != 0) {
            continue;
        }
        if (((struct object *)object)->type == 0 && (object[0x106] & 4) != 0) {
            continue; // 0x43c5d0: a biped flagged 4 at +0x106
        }
        if (((struct object *)object)->type == 7) {
            uint16_t machine_flags = *(uint16_t *)(TAG_DATA(*(datum_index *)object) + 0x292);

            if ((machine_flags & 1) == 0) {
                continue;
            }
            if ((machine_flags & 2) != 0 && *(float *)(object + 0x208) == 1.0f) {
                continue;
            }
        }
        if (!point3d_within_radius((real_point3d *)(object + 0xa0), center, radius + ((struct object *)object)->bounding_radius)) {
            continue;
        }
        object_tag = TAG_DATA(*(datum_index *)object);
        collision = TAG_DATA(*(datum_index *)(object_tag + 0x7c));
        if ((object_tag[2] & 8) != 0 || *(int32_t *)(collision + 0x280) <= 0) {
            continue;
        }
        object_get_world_matrix(object_index, &world);
        for (s = 0; s < *(int32_t *)(collision + 0x280); s++) {
            uint8_t *sphere = *(uint8_t **)(collision + 0x284) + s * 0x20;
            int16_t node = *(int16_t *)sphere;
            real_point3d point;
            float sphere_radius;
            float dx;
            float dy;
            float dz;
            float reach;
            uint16_t flags = 0;

            object = OBJECT_DATA(object_index);
            if (node != -1) {
                real_matrix4x3 *matrix = (real_matrix4x3 *)(object + ((struct object *)object)->nodes.offset + node * 0x34);

                matrix4x3_transform_point(&point, (real_point3d *)(sphere + 0x10), matrix);
                sphere_radius = *(float *)(sphere + 0x1c) * matrix->scale;
            } else {
                matrix4x3_transform_point(&point, (real_point3d *)(sphere + 0x10), &world);
                sphere_radius = world.scale * *(float *)(sphere + 0x1c);
            }
            // 0x43c708: spheres entirely below (unless the direction points down) or above (unless it points up) skip
            if (!(point.z + sphere_radius + 0.5f >= center->z) && direction->k > -0.2f) {
                continue;
            }
            if (point.z - sphere_radius - 0.5f > center->z && direction->k < 0.2f) {
                continue;
            }
            dx = point.x - center->x;
            dy = point.y - center->y;
            dz = point.z - center->z;
            reach = sphere_radius + radius;
            if (reach * reach < dz * dz * 4.0f + dy * dy + dx * dx) {
                continue;
            }
            if (((struct object *)object)->type == 0 && dy * direction->j + dx * direction->i + dz * direction->k > 0.0f &&
                ((struct object *)object)->velocity.k * direction->k + ((struct object *)object)->velocity.j * direction->j +
                        ((struct object *)object)->velocity.i * direction->i > 0.06666667f) {
                flags = 1; // 0x43c81b: a biped moving along the same way
            }
            ai_search_append_obstacle(list, flags, object_index, (real_point2d *)&point, sphere_radius);
        }
    }
}

#if 0
// ---- original Ghidra decompilation (ai_search_gather_obstacles @ 0x43c510) ----
void ai_search_gather_obstacles
               (undefined4 param_1,float *param_2,float param_3,float *param_4,uint param_5,
               uint param_6)

{
  uint uVar1;
  uint *puVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  char cVar8;
  ushort uVar9;
  short sVar10;
  int iVar11;
  short *psVar12;
  undefined4 uVar13;
  int iVar14;
  float *pfVar15;
  int iVar16;
  uint *local_46c;
  uint local_454;
  float local_44c;
  float local_448;
  float local_444;
  float local_440 [14];
  uint local_408 [257];

  uVar9 = object_find_in_sphere
                    (1,0xc3,*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_5 & 0xffff) * 0xc) +
                            0x98,param_2,param_3,local_408,0x100);
  if (0 < (short)uVar9) {
    local_454 = (uint)uVar9;
    local_46c = local_408;
    iVar16 = DAT_0087bc14;
    do {
      uVar1 = *local_46c;
      iVar14 = (uVar1 & 0xffff) * 0xc;
      puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar14);
      if ((((((uVar1 != param_5) && (uVar1 != param_6)) && ((puVar2[4] & 1) == 0)) &&
           ((((short)puVar2[0x2d] != 0 || ((*(byte *)((int)puVar2 + 0x106) & 4) == 0)) &&
            (((short)puVar2[0x2d] != 7 ||
             ((uVar9 = *(ushort *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + iVar16) + 0x292),
              (uVar9 & 1) != 0 && (((uVar9 & 2) == 0 || (puVar2[0x82] != 0x3f800000)))))))))) &&
          (cVar8 = FUN_0043c340(param_3 + (float)puVar2[0x2b]), cVar8 != '\0')) &&
         ((iVar11 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + iVar16),
          iVar3 = *(int *)((*(uint *)(iVar11 + 0x7c) & 0xffff) * 0x20 + 0x14 + iVar16),
          (*(byte *)(iVar11 + 2) & 8) == 0 && (0 < *(int *)(iVar3 + 0x280))))) {
        object_get_world_matrix();
        iVar11 = 0;
        sVar10 = 0;
        iVar16 = DAT_0087bc14;
        if (0 < *(int *)(iVar3 + 0x280)) {
          do {
            psVar12 = (short *)(iVar11 * 0x20 + *(int *)(iVar3 + 0x284));
            if (*psVar12 == -1) {
              matrix4x3_transform_point(local_440);
              fVar4 = local_440[0] * *(float *)(psVar12 + 0xe);
            }
            else {
              iVar16 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar14);
              pfVar15 = (float *)((int)*(short *)(iVar16 + 0x1f2) + *psVar12 * 0x34 + iVar16);
              matrix4x3_transform_point(pfVar15);
              fVar4 = *(float *)(psVar12 + 0xe) * *pfVar15;
            }
            if (((param_2[2] <= local_444 + fVar4 + 0.5) || (param_4[2] <= -0.2)) &&
               (((local_444 - fVar4) - 0.5 <= param_2[2] || (0.2 <= param_4[2])))) {
              fVar5 = local_44c - *param_2;
              fVar6 = local_448 - param_2[1];
              fVar7 = local_444 - param_2[2];
              if (fVar5 * fVar5 + fVar6 * fVar6 + fVar7 * fVar7 * 4.0 <=
                  (fVar4 + param_3) * (fVar4 + param_3)) {
                uVar13 = 0;
                if ((((short)puVar2[0x2d] == 0) &&
                    (0.0 < fVar7 * param_4[2] + fVar5 * *param_4 + fVar6 * param_4[1])) &&
                   (0.06666667 <
                    (float)puVar2[0x1a] * *param_4 +
                    (float)puVar2[0x1b] * param_4[1] + (float)puVar2[0x1c] * param_4[2])) {
                  uVar13 = 1;
                }
                FUN_0043c4b0(uVar1,uVar13,fVar4);
              }
            }
            sVar10 = sVar10 + 1;
            iVar11 = (int)sVar10;
            iVar16 = DAT_0087bc14;
          } while (iVar11 < *(int *)(iVar3 + 0x280));
        }
      }
      local_46c = local_46c + 1;
      local_454 = local_454 - 1;
    } while (local_454 != 0);
  }
  return;
}
#endif
