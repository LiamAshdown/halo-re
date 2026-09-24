// object_lights_gather_nearest
// address 0x4f2df0, size 503 bytes
// name confidence: 0.75 (still FUN_004f2df0 in Ghidra; types/objects.h's light.radius comment
//   names this function directly: "the object_lights_gather_nearest falloff denominator";
//   matches functions.md's summary)
// rewrite confidence: 0.4
// evidence: types/objects.h light (owner_object 0x2c, position 0x30, creation_tick 0x0c, radius
//   0x54); global 0x00860b20 light_cluster_first, 0x00860b24 light_cluster_references, 0x00860b14
//   light_data, 0x008607c4 light_frame_counter; types/tags.h Object.flags bit 2
//   ("brighter_than_it_should_be" per its bitfield comment); types/cache.h tag_instance.
// register convention: cluster/hash index in AX (in_AX), then the eight stack parameters shown
//   by Ghidra unchanged.
// UNSURE: light+0x08 (tested != -1 as a validity gate) and light+0x14/0x18/0x1c (read here as
//   an R/G/B intensity triple weighted by the standard 0.299/0.587/0.114 luma coefficients) are
//   not named fields in types/objects.h, which currently folds 0x14..0x2b into "unknown_14";
//   this function's own arithmetic is the evidence for the luma read, kept as raw offsets since
//   the header is not redefined here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern datum_index *light_cluster_first; // 0x00860b20
extern data_array *light_cluster_references; // 0x00860b24
extern data_array *light_data; // 0x00860b14
extern int32_t light_frame_counter; // 0x008607c4
extern tag_instance *tag_instances; // 0x0087bc14
extern double sqrt(double x); // a single x87 FSQRT instruction in the original (Ghidra's SQRT())

void object_lights_gather_nearest(int16_t cluster_index, uint32_t self_object_index,
                                   real_point3d *probe_point, float search_margin,
                                   uint32_t *out_indices, float *out_intensities,
                                   uint32_t out_falloffs, int16_t *count, int16_t max_count)
    // blam-cc: AX -> cluster_index, stack -> self_object_index, probe_point, search_margin,
    //          out_indices, out_intensities, out_falloffs, count, max_count
{
    datum_index next_ref = light_cluster_first[cluster_index];
    uint32_t chain_index;

    if (next_ref == k_datum_index_none) {
        chain_index = k_datum_index_none;
    } else {
        object_cluster_reference *ref =
            (object_cluster_reference *)light_cluster_references->data + (next_ref & 0xffff);
        chain_index = ref->object_index;
        next_ref = ref->next_reference;
    }

    while (chain_index != k_datum_index_none) {
        light *entry = (light *)light_data->data + (chain_index & 0xffff);

        if (entry->creation_tick != light_frame_counter) {
            if (*(int32_t *)((uint8_t *)entry + 8) != -1) {
                int eligible;
                if (entry->owner_object != self_object_index) {
                    eligible = 1;
                } else {
                    Object *owner_tag = (Object *)tag_instances[*(uint32_t *)((uint8_t *)entry + 4) & 0xffff].data;
                    eligible = (owner_tag->flags & 4) == 0;
                }

                if (eligible) {
                    float dx = probe_point->x - entry->position.x;
                    float dy = probe_point->y - entry->position.y;
                    float dz = probe_point->z - entry->position.z;
                    float distance = (float)sqrt((double)(dy * dy + dx * dx + dz * dz));
                    if (distance < search_margin + entry->radius) {
                        int16_t used = *count;
                        float attenuation = 1.0f - (distance * distance) / (entry->radius * entry->radius);
                        float luminance = (*(float *)((uint8_t *)entry + 0x14) * 0.299f +
                                           *(float *)((uint8_t *)entry + 0x18) * 0.587f +
                                           *(float *)((uint8_t *)entry + 0x1c) * 0.114f) * attenuation;
                        int16_t slot;

                        if (used < max_count) {
                            *count = used + 1;
                            slot = used;
                        } else {
                            // Replace the weakest current entry, but only if the new light is
                            // actually stronger; otherwise slot ends up == *count (== max_count)
                            // and the bounds check below skips the insert, exactly mirroring the
                            // original's post-loop sVar7 value.
                            float weakest = 3.4028235e+38f;
                            int16_t weakest_slot = -1;
                            slot = 0;
                            if (used > 0) {
                                int16_t j;
                                for (j = 0; j < *count; j++) {
                                    if (out_intensities[j] < weakest) {
                                        weakest = out_intensities[j];
                                        weakest_slot = j;
                                    }
                                }
                                slot = j;
                            }
                            if (weakest < luminance) {
                                slot = weakest_slot;
                            }
                        }

                        if (slot < max_count) {
                            out_indices[slot] = chain_index;
                            out_intensities[slot] = luminance;
                            *(float *)((uint8_t *)out_falloffs + slot * 4) = attenuation;
                        }
                    }
                }
            }
            if (entry->creation_tick != light_frame_counter) {
                entry->creation_tick = light_frame_counter;
            }
        }

        if (next_ref == k_datum_index_none) {
            chain_index = k_datum_index_none;
        } else {
            object_cluster_reference *ref =
                (object_cluster_reference *)light_cluster_references->data + (next_ref & 0xffff);
            chain_index = ref->object_index;
            next_ref = ref->next_reference;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f2df0):

void FUN_004f2df0(int param_1,float *param_2,float param_3,int param_4,float *param_5,int param_6,
                 short *param_7,short param_8)

{
  float fVar1;
  short sVar2;
  float fVar3;
  float fVar4;
  uint uVar5;
  short in_AX;
  uint uVar6;
  short sVar7;
  int iVar8;
  float *pfVar9;
  short sVar10;
  int iVar11;
  uint uVar12;

  uVar12 = *(uint *)(DAT_00860b20 + in_AX * 4);
  if (uVar12 == 0xffffffff) {
    uVar6 = 0xffffffff;
    uVar12 = 0xffffffff;
    uVar5 = uVar12;
  }
  else {
    iVar8 = *(int *)(DAT_00860b24 + 0x34) + (uVar12 & 0xffff) * 0xc;
    uVar12 = *(uint *)(iVar8 + 8);
    uVar6 = *(uint *)(iVar8 + 4);
    uVar5 = uVar12;
  }
  while (uVar6 != 0xffffffff) {
    iVar11 = (uVar6 & 0xffff) * 0x7c;
    iVar8 = *(int *)(DAT_00860b14 + 0x34) + iVar11;
    if (*(int *)(*(int *)(DAT_00860b14 + 0x34) + 0xc + iVar11) != DAT_008607c4) {
      if ((*(int *)(iVar8 + 8) != -1) &&
         (((*(int *)(iVar8 + 0x2c) != param_1 ||
           ((**(byte **)((*(uint *)(iVar8 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) & 4) == 0))
          && (fVar3 = *param_2 - *(float *)(iVar8 + 0x30),
             fVar4 = param_2[1] - *(float *)(iVar8 + 0x34),
             fVar1 = param_2[2] - *(float *)(iVar8 + 0x38),
             fVar3 = SQRT(fVar4 * fVar4 + fVar3 * fVar3 + fVar1 * fVar1),
             fVar3 < param_3 + *(float *)(iVar8 + 0x54))))) {
        sVar2 = *param_7;
        fVar3 = 1.0 - (fVar3 * fVar3) / (*(float *)(iVar8 + 0x54) * *(float *)(iVar8 + 0x54));
        fVar4 = (*(float *)(iVar8 + 0x14) * 0.299 +
                *(float *)(iVar8 + 0x18) * 0.587 + *(float *)(iVar8 + 0x1c) * 0.114) * fVar3;
        if (sVar2 < param_8) {
          *param_7 = sVar2 + 1;
          sVar7 = sVar2;
        }
        else {
          fVar1 = 3.4028235e+38;
          sVar10 = -1;
          sVar7 = 0;
          if (0 < sVar2) {
            pfVar9 = param_5;
            sVar7 = 0;
            do {
              if (*pfVar9 < fVar1) {
                fVar1 = *pfVar9;
                sVar10 = sVar7;
              }
              sVar7 = sVar7 + 1;
              pfVar9 = pfVar9 + 1;
              uVar12 = uVar5;
            } while (sVar7 < *param_7);
          }
          if (fVar1 < fVar4) {
            sVar7 = sVar10;
          }
        }
        if (sVar7 < param_8) {
          iVar8 = sVar7 * 4;
          *(uint *)(iVar8 + param_4) = uVar6;
          param_5[sVar7] = fVar4;
          *(float *)(iVar8 + param_6) = fVar3;
        }
      }
      if (*(int *)(*(int *)(DAT_00860b14 + 0x34) + 0xc + iVar11) != DAT_008607c4) {
        *(int *)(*(int *)(DAT_00860b14 + 0x34) + iVar11 + 0xc) = DAT_008607c4;
      }
    }
    if (uVar12 == 0xffffffff) {
      uVar6 = 0xffffffff;
    }
    else {
      uVar6 = uVar12 & 0xffff;
      uVar12 = *(uint *)(*(int *)(DAT_00860b24 + 0x34) + 8 + uVar6 * 0xc);
      uVar6 = *(uint *)(*(int *)(DAT_00860b24 + 0x34) + uVar6 * 0xc + 4);
      uVar5 = uVar12;
    }
  }
  return;
}
#endif
