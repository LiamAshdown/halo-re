// structure_leaf_faces_for_each  (Ghidra: FUN_00552de0; named here)
// address 0x552de0, size 629 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x552de0..0x553055 (both callback argument orders, the unaligned breakable bitmask at +1).)
// evidence: disassembly of the two callers resolved here (structure_picked_polygon_draw
//   0x5528f0, and the three "debug draw surfaces in box" siblings 0x552980/0x552a60/0x552b40)
//   shows the two implicit registers Ghidra's decompilation left unnamed: ECX is the
//   *sorted, ascending* surface index list to walk (visible_surface_indices in every observed
//   caller) and EAX/AX is its element count. The outer loop is over
//   ScenarioStructureBSP.lightmaps, the inner loop over each lightmap's materials
//   (types/tags.h ScenarioStructureBSPMaterial.surfaces/surface_count), advancing through the
//   sorted index list exactly as far as each material's surface range covers -- a merge-style
//   walk, not a "leaves in a range" walk as the phase4 one-line summary guessed (corrected here).
// register convention: ECX -> surface_indices, EAX -> surface_index_count; stack -> render_context
//   (an opaque value forwarded verbatim to the material/transparent callbacks),
//   opaque_lightmap_begin, opaque_material, opaque_lightmap_end, opaque_transparent_material
//   (four optional callback pointers, called only when non-NULL).
// UNSURE: the exact argument lists of the material and transparent-material callbacks are kept as
//   Ghidra decompiled them (raw addresses/values); their true signatures belong to the render
//   module. UNSURE: 0x0072278c / runtime_decals_suppressed-style globals are not involved here;
//   do not confuse with structure_decals_update_switch_transitions.
// reconciled: R76 0x00696714 k_default_fog_plane_vector -> math.h const real_point3d *global_origin3d_pointer (points at the (0,0,0) constant 0x0065c230)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#include "cache.h"
#include "structures.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, physics.h/objects.h (read, not owned)
extern tag_instance *tag_instances; // 0x0087bc14, cache.h
extern breakable_surface_globals *breakable_surfaces; // 0x006b8d78, physics.h
extern int16_t global_structure_bsp_index;  // 0x0069e8d8: accessed as WORD // 0x0069e8d8, physics.h
extern real_vector3d fog_plane_vector; // 0x006e3ae4, this module
extern const real_point3d *global_origin3d_pointer; // 0x00696714, math.h (== 0x0065c230, the zero point)

// Walks every lightmap's materials, and for each material whose surface range still overlaps the
// unconsumed head of `surface_indices` (a sorted, ascending list of `surface_index_count` global
// surface indices), invokes the material callback (or the transparent-material callback for
// environment-shaded materials) once per material, passing the count of indices that fell in its
// range and advancing past them. Calls the lightmap-begin/-end callbacks once per lightmap that
// still has unconsumed indices ahead of it.
void structure_leaf_faces_for_each(int32_t render_context, structure_lightmap_begin_callback lightmap_begin,
    structure_material_callback material_cb, structure_lightmap_end_callback lightmap_end,
    structure_transparent_material_callback transparent_material_cb,
    int32_t *surface_indices, int16_t surface_index_count)
    // blam-cc: ECX -> surface_indices, EAX -> surface_index_count
{
    int32_t *end = surface_indices + surface_index_count;
    int32_t surface_offset = 0;
    int16_t lightmap_index;

    for (lightmap_index = 0; lightmap_index < global_structure_bsp->lightmaps.count; lightmap_index = lightmap_index + 1) {
        ScenarioStructureBSPLightmap *lightmap =
            (ScenarioStructureBSPLightmap *)global_structure_bsp->lightmaps.pointer + lightmap_index;
        ScenarioStructureBSPMaterial *materials =
            (ScenarioStructureBSPMaterial *)lightmap->materials.pointer;
        int32_t material_count = lightmap->materials.count;

        if (surface_indices >= end) {
            return;
        }

        if (*surface_indices < materials[material_count - 1].surfaces + materials[material_count - 1].surface_count) {
            void *bitmap_data = 0;

            if (global_structure_bsp->lightmaps_bitmap.tag_id.index != 0xffff) {
                uint16_t bitmap_index = lightmap->bitmap;
                Bitmap *bitmap = (Bitmap *)tag_instances[global_structure_bsp->lightmaps_bitmap.tag_id.index].data;
                if (bitmap != 0 && bitmap_index < bitmap->bitmap_data.count) {
                    bitmap_data = (uint8_t *)bitmap->bitmap_data.pointer + bitmap_index * 0x30;
                }
            }
            if (lightmap_begin != 0) {
                lightmap_begin(bitmap_data);
            }

            {
                int16_t material_index;
                for (material_index = 0; material_index < material_count; material_index = material_index + 1) {
                    ScenarioStructureBSPMaterial *material = &materials[material_index];
                    int32_t material_end = material->surfaces + material->surface_count;

                    if (surface_indices >= end) {
                        break;
                    }
                    if (*surface_indices < material_end) {
                        Shader *shader = (Shader *)tag_instances[material->shader.tag_id.index].data;
                        int32_t *scan = surface_indices;
                        int16_t consumed;

                        do {
                            scan = scan + 1;
                            if (scan >= end) {
                                break;
                            }
                        } while (*scan < material_end);
                        consumed = (int16_t)(scan - surface_indices);

                        if (material->breakable_surface == (uint16_t)-1 ||
                            (breakable_surfaces->active[global_structure_bsp_index][material->breakable_surface >> 5] &
                             (1u << (material->breakable_surface & 0x1f))) != 0) {
                            if (shader->shader_type == 1 || (shader->shader_type > 4 && shader->shader_type < 0xc)) {
                                if (transparent_material_cb != 0) {
                                    // Default is the live fog plane vector; falls back to the
                                    // fixed default fog plane pointer when material.flags bit 1
                                    // (coplanar/fog) is clear.
                                    void *coplanar_vector = ((uint16_t)material->flags & 2) != 0
                                        ? (void *)&fog_plane_vector
                                        : (void *)global_origin3d_pointer;
                                    void *lightmap_vertices = ((uint16_t)material->flags & 1) != 0
                                        ? (void *)((uint8_t *)material + 0x9c)
                                        : (void *)0;
                                    transparent_material_cb(shader, material->shader_permutation, bitmap_data,
                                        render_context, surface_offset, consumed, (uint8_t *)material + 0xb0,
                                        (uint8_t *)material + 0x1c, lightmap_vertices, coplanar_vector,
                                        (uint8_t *)material + 0x28, 0);
                                }
                            } else if (material_cb != 0) {
                                material_cb(shader, material->shader_permutation, render_context, surface_offset,
                                    consumed, (uint8_t *)material + 0xb0);
                            }
                        }

                        surface_offset = surface_offset + consumed;
                        surface_indices = scan;
                    }
                }
            }

            if (lightmap_end != 0) {
                lightmap_end();
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x552de0):

void FUN_00552de0(undefined4 param_1,code *param_2,code *param_3,code *param_4,code *param_5)

{
  int *piVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  short in_AX;
  short sVar5;
  int *in_ECX;
  int iVar6;
  int iVar7;
  short sVar8;
  int iVar9;
  undefined *puVar10;
  int *piVar11;
  short *psVar12;
  short sVar13;
  int local_1c;

  iVar4 = DAT_00746f9c;
  piVar1 = in_ECX + in_AX;
  local_1c = 0;
  sVar5 = 0;
  if (0 < *(int *)(DAT_00746f9c + 0x104)) {
    do {
      if (piVar1 <= in_ECX) {
        return;
      }
      psVar12 = (short *)(sVar5 * 0x20 + *(int *)(iVar4 + 0x108));
      iVar9 = *(int *)(sVar5 * 0x20 + 0x14 + *(int *)(iVar4 + 0x108)) * 0x100;
      piVar11 = in_ECX;
      if (*in_ECX < *(int *)(iVar9 + -0xe8 + *(int *)(psVar12 + 0xc)) +
                    *(int *)(*(int *)(psVar12 + 0xc) + -0xec + iVar9)) {
        if (*(uint *)(iVar4 + 0xc) == 0xffffffff) {
          iVar6 = 0;
        }
        else {
          sVar8 = *psVar12;
          iVar9 = *(int *)((*(uint *)(iVar4 + 0xc) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          iVar6 = 0;
          if (((iVar9 != 0) && (-1 < sVar8)) && ((int)sVar8 < *(int *)(iVar9 + 0x60))) {
            iVar6 = sVar8 * 0x30 + *(int *)(iVar9 + 100);
          }
        }
        if (param_2 != (code *)0x0) {
          (*param_2)(iVar6);
        }
        sVar8 = 0;
        if (0 < *(int *)(psVar12 + 10)) {
          do {
            piVar11 = in_ECX;
            if (piVar1 <= in_ECX) break;
            iVar9 = sVar8 * 0x100 + *(int *)(psVar12 + 0xc);
            iVar7 = *(int *)(iVar9 + 0x18) +
                    *(int *)(sVar8 * 0x100 + 0x14 + *(int *)(psVar12 + 0xc));
            if (*in_ECX < iVar7) {
              iVar3 = *(int *)((*(uint *)(iVar9 + 0xc) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
              do {
                piVar11 = piVar11 + 1;
                if (piVar1 <= piVar11) break;
              } while (*piVar11 < iVar7);
              sVar2 = *(short *)(iVar9 + 0xac);
              sVar13 = (short)((int)piVar11 - (int)in_ECX >> 2);
              if ((sVar2 == -1) ||
                 ((*(uint *)(DAT_006b8d78 + 1 + (((int)sVar2 >> 5) + DAT_0069e8d8 * 8) * 4) &
                  1 << ((byte)sVar2 & 0x1f)) != 0)) {
                sVar2 = *(short *)(iVar3 + 0x24);
                if ((sVar2 == 1) || ((4 < sVar2 && (sVar2 < 0xc)))) {
                  if (param_5 != (code *)0x0) {
                    puVar10 = &DAT_006e3ae4;
                    if ((*(ushort *)(iVar9 + 0x12) & 2) == 0) {
                      puVar10 = PTR_DAT_00696714;
                    }
                    if ((*(ushort *)(iVar9 + 0x12) & 1) == 0) {
                      iVar7 = 0;
                    }
                    else {
                      iVar7 = iVar9 + 0x9c;
                    }
                    (*param_5)(iVar3,(int)*(short *)(iVar9 + 0x10),iVar6,param_1,local_1c,
                               (int)sVar13,iVar9 + 0xb0,iVar9 + 0x1c,iVar7,puVar10,iVar9 + 0x28,0);
                  }
                }
                else if (param_3 != (code *)0x0) {
                  (*param_3)(iVar3,(int)*(short *)(iVar9 + 0x10),param_1,local_1c,(int)sVar13,
                             iVar9 + 0xb0);
                }
              }
              local_1c = local_1c + sVar13;
            }
            sVar8 = sVar8 + 1;
            in_ECX = piVar11;
          } while ((int)sVar8 < *(int *)(psVar12 + 10));
        }
        if (param_4 != (code *)0x0) {
          (*param_4)();
        }
      }
      sVar5 = sVar5 + 1;
      in_ECX = piVar11;
    } while ((int)sVar5 < *(int *)(iVar4 + 0x104));
  }
  return;
}
#endif
