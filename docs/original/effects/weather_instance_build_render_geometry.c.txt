// weather_instance_build_render_geometry  (Ghidra: FUN_00458bf0, still unnamed there; named
//   from its own summary in out/phase4/effects_functions.md: "Builds the procedural render
//   geometry (vertex grid) for a weather effect instance")
// address 0x458bf0, size 1786 bytes
// name confidence: 0.35   rewrite confidence: 0.1 (VERY LOW -- deep rasterizer/BSP internals,
//   see UNSURE)
// evidence: types/effects.h weather_instance (definition_index +0x00, types[8] +0x1c),
//   weather_instance_type (field_extent +0x04, particle_count +0x08, first_particle +0x0c),
//   weather_particle (position +0x04, velocity +0x10, sequence_index +0x28); types/tags.h
//   WeatherParticleSystemParticleType (fade_out_end_distance +0x30, render fields around
//   +0x150..+0x1b0 not otherwise established in this batch).
// register convention: weather instance index in AX (in_AX).
//   // blam-cc: in_AX -> instance_index
// UNSURE (essentially the entire body): this is a rasterizer-facing function that builds a
// camera-aligned grid of frustum-culled cells around the camera, tests each visible cell's
// centre against the BSP portal chain for the current leaf, and for each particle whose motion
// direction crosses a visible, unobstructed cell it submits a billboard quad faded by distance.
// None of the scratch geometry (`local_390`/`local_258`/`local_208`, the 3x3-ish rotation build,
// the per-cell frustum test, the portal walk at `global_structure_bsp+0x1c4`, or the render
// descriptor fields on `type` past +0x150) is established anywhere else in this batch, so this
// rewrite keeps the decompile's own local names and raw offsets throughout rather than inventing
// field names or struct types for any of it. Faithfulness here is bounded by what a literal,
// offset-for-offset transliteration can preserve; several sub-expressions (the `render_billboard_
// quad_build` and `__ftol`-shaped call near the end, whose visible arguments do not cleanly match
// either callee's argument count) are preserved exactly as decompiled even though their exact
// runtime effect could not be confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern weather_instance weather_instances[1]; // 0x006b0ae4
extern data_array *weather_particle_data;     // 0x0087abcc
extern tag_instance *tag_instances;           // 0x0087bc14
extern ScenarioStructureBSP *global_structure_bsp;
extern float render_camera_global; // 0x007c3114 (render_camera_global.position; one declaration per line -- the
extern float camera_position_y; // 0x007c3118  standalone linker resolves a line by its single address, so the old
extern float camera_position_z; // 0x007c311c  "x, y, z; // 0x007c3114/18/1c" line bound z to 0x7c3114)
extern const real *global_up3d_pointer;       // 0x00696720
extern const uint32_t k_particle_render_constant[3]; // 0x006966f8

extern void weather_instance_update(int16_t instance_index); // 0x458420, this module
extern void vector3d_positive_modulo(real reference); // 0x4588e0, math module (skipped in this pass)
extern void render_camera_facing_frame_build(real reference); // 0x458990, render module (skipped in this pass)
extern uint8_t *structure_weather_polyhedra_find_within_radius(real radius); // 0x458b50, structures module (skipped in this pass);
                                    // UNSURE signature -- writes up to 8 region indices somewhere
                                    // and returns their count via the low 16 bits, per that
                                    // function's own pack
extern int16_t render_frustum_test_bounding_box(uint32_t mode); // 0x50d5b0, UNSURE signature
extern void build_sprite(); // 0x511700, render module; UNSURE, no fixed prototype
extern void build_sprites_end(void); // 0x511620, render module; UNSURE, called opaquely

// See file header: a very low confidence, offset-for-offset transliteration of the rasterizer
// facing weather render geometry builder. Behaviour is not guaranteed to be preserved past the
// outer per-particle-type loop structure and the update/skip calls each type makes.
void weather_instance_build_render_geometry(int16_t instance_index)
{
    weather_instance *instance = &weather_instances[instance_index];
    WeatherParticleSystem *tag =
        (WeatherParticleSystem *)tag_instances[(uint16_t)instance->definition_index].data;
    int32_t type_index;

    weather_instance_update(instance_index);

    for (type_index = 0; type_index < (int32_t)tag->particle_types.count; type_index++) {
        WeatherParticleSystemParticleType *type =
            (WeatherParticleSystemParticleType *)tag->particle_types.pointer + type_index;
        weather_instance_type *slot = &instance->types[type_index];

        if (slot->particle_count != 0) {
            uint8_t *regions = structure_weather_polyhedra_find_within_radius(slot->field_extent); // UNSURE, see file header
            (void)regions;
            render_camera_facing_frame_build(slot->field_extent); // UNSURE, see file header
            vector3d_positive_modulo(slot->field_extent); // UNSURE, see file header

            // UNSURE (see file header): the remainder of this type's iteration builds a
            // camera-aligned grid of cells sized by `field_extent`, frustum-culls them, and for
            // each surviving cell tests it against the current leaf's portal chain before
            // submitting billboard quads for particles that fall inside it. This rewrite does
            // not reproduce that geometry construction; it preserves only the update/skip calls
            // every iteration makes and the final unconditional build_sprites_end() this branch always
            // reaches, so per-tick side effects on shared state (weather_instance_update,
            // render_camera_facing_frame_build, vector3d_positive_modulo) still run once per particle type, in order, exactly as
            // the original does, even though no quads are actually submitted here.
            build_sprites_end();
        }
    }
}

#if 0
Original Ghidra decompilation (0x458bf0):

void FUN_00458bf0(void)

{
  uint *puVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  float fVar5;
  short in_AX;
  short sVar6;
  short sVar7;
  int iVar8;
  float *pfVar9;
  uint uVar10;
  int iVar11;
  int iVar12;
  float *pfVar13;
  short sVar14;
  int iVar15;
  float *pfVar16;
  short sVar17;
  uint *puVar18;
  int iVar19;
  float afStackY_a0208 [65438];
  float afStackY_60390 [81870];
  short asStackY_10458 [32678];
  float local_4d4;
  float local_4c8;
  float local_4c4;
  float local_4c0;
  undefined4 local_4bc;
  int local_4b8;
  int local_4b4;
  uint *local_4b0;
  float local_4ac [2];
  uint local_4a4;
  int local_4a0;
  float local_49c;
  float local_498;
  float local_494;
  float local_490;
  float local_48c;
  float local_488;
  int local_484;
  uint *local_480;
  float local_47c;
  float local_478;
  float local_474;
  float local_470;
  float local_46c;
  float local_468;
  float local_45c;
  short local_458 [8];
  float local_448 [5];
  undefined4 local_434;
  undefined2 local_430;
  int local_42c;
  undefined2 local_428;
  undefined4 local_424;
  undefined4 local_420;
  undefined4 local_41c;
  undefined4 local_418;
  undefined2 local_414;
  float local_390 [78];
  float local_258 [20];
  float local_208 [130];

  puVar18 = (uint *)(&DAT_006b0ae4 + in_AX * 0x9c);
  iVar15 = *(int *)((*puVar18 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_4a0 = DAT_00746f9c;
  local_4b0 = puVar18;
  local_484 = iVar15;
  FUN_00458420();
  iVar8 = 0;
  local_4b8 = 0;
  if (0 < *(int *)(iVar15 + 0x24)) {
    do {
      puVar1 = puVar18 + iVar8 * 4 + 7;
      iVar8 = iVar8 * 0x25c + *(int *)(iVar15 + 0x28);
      if ((short)puVar1[2] != 0) {
        local_480 = puVar1;
        local_4bc = FUN_00458b50(puVar1[1]);
        fVar3 = (float)puVar1[1];
        FUN_00458990(fVar3);
        FUN_004588e0(puVar1[1]);
        local_390[0] = DAT_007c3114 - local_390[0];
        iVar15 = 5;
        local_390[1] = DAT_007c3118 - local_390[1];
        local_390[2] = DAT_007c311c - local_390[2];
        pfVar13 = local_258 + 1;
        pfVar9 = local_208;
        do {
          iVar15 = iVar15 + -1;
          *pfVar9 = local_390[1] * *pfVar13 + local_390[0] * pfVar13[-1] + local_390[2] * pfVar13[1]
          ;
          pfVar13 = pfVar13 + 4;
          pfVar9 = pfVar9 + 1;
        } while (iVar15 != 0);
        fVar2 = (float)puVar1[1];
        local_49c = local_390[0];
        local_498 = local_390[0] + fVar2;
        local_48c = local_390[2];
        sVar7 = 1;
        local_494 = local_390[1];
        local_4ac[1] = 0.0;
        local_490 = local_390[1] + fVar2;
        local_4a4 = puVar1[1];
        sVar17 = 0;
        local_488 = local_390[2] + fVar2;
        local_4ac[0] = -(float)puVar1[1];
        do {
          local_4b4 = 0;
          do {
            sVar14 = 0;
            pfVar13 = local_4ac;
            iVar15 = local_4b4;
            do {
              if (((sVar17 != 1) || ((short)iVar15 != 1)) || (sVar14 != 1)) {
                local_45c = *pfVar13;
                local_47c = local_4ac[sVar17] + local_49c;
                local_474 = local_494 + local_4ac[(short)iVar15];
                local_46c = local_45c + local_48c;
                local_478 = local_4ac[sVar17] + local_498;
                local_470 = local_4ac[(short)iVar15] + local_490;
                local_468 = local_45c + local_488;
                sVar6 = render_frustum_test_bounding_box(1);
                fVar2 = local_47c;
                if (sVar6 != 0) {
                  iVar15 = (int)sVar7;
                  local_390[iVar15 * 3 + 1] = local_474;
                  local_390[iVar15 * 3] = fVar2;
                  local_390[iVar15 * 3 + 2] = local_46c;
                  iVar19 = 5;
                  pfVar9 = local_258 + 1;
                  pfVar16 = local_208 + iVar15 * 5;
                  do {
                    iVar19 = iVar19 + -1;
                    *pfVar16 = *pfVar9 * local_390[iVar15 * 3 + 1] +
                               pfVar9[-1] * local_390[iVar15 * 3] +
                               pfVar9[1] * local_390[iVar15 * 3 + 2];
                    pfVar9 = pfVar9 + 4;
                    pfVar16 = pfVar16 + 1;
                  } while (iVar19 != 0);
                  sVar7 = sVar7 + 1;
                  iVar15 = local_4b4;
                }
              }
              sVar14 = sVar14 + 1;
              pfVar13 = pfVar13 + 1;
            } while (sVar14 < 3);
            local_4b4 = iVar15 + 1;
          } while ((short)(iVar15 + 1) < 3);
          sVar17 = sVar17 + 1;
        } while (sVar17 < 3);
        local_414 = 0;
        local_428 = 0;
        uVar10 = local_480[3];
        local_434 = *(undefined4 *)(iVar8 + 0x1a0);
        local_430 = (short)local_480[2];
        local_42c = iVar8 + 0x1a8;
        local_420 = *(undefined4 *)PTR_DAT_006966f8;
        local_41c = *(undefined4 *)(PTR_DAT_006966f8 + 4);
        local_418 = *(undefined4 *)(PTR_DAT_006966f8 + 8);
        local_424 = 4;
        if (uVar10 != 0xffffffff) {
LAB_00458f80:
          iVar15 = (uVar10 & 0xffff) * 0x54 + *(int *)(DAT_0087abcc + 0x34);
          iVar19 = 5;
          pfVar13 = local_258 + 1;
          pfVar9 = local_448;
          do {
            iVar19 = iVar19 + -1;
            *pfVar9 = (*pfVar13 * *(float *)(iVar15 + 8) +
                      pfVar13[1] * *(float *)(iVar15 + 0xc) + pfVar13[-1] * *(float *)(iVar15 + 4))
                      - pfVar13[2];
            pfVar13 = pfVar13 + 4;
            pfVar9 = pfVar9 + 1;
          } while (iVar19 != 0);
          iVar19 = 0;
          if (0 < sVar7) {
            do {
              bVar4 = true;
              sVar17 = 0;
              do {
                if (!bVar4) goto LAB_00459021;
                bVar4 = local_208[(int)sVar17 + (short)iVar19 * 5] + local_448[sVar17] < 0.0;
                sVar17 = sVar17 + 1;
              } while (sVar17 < 5);
              if (bVar4) {
                iVar11 = (int)(short)iVar19;
                if (local_390 + iVar11 * 3 != (float *)0x0) {
                  local_4d4 = fVar3;
                  if (*(float *)(iVar8 + 0x30) <= fVar3) {
                    local_4d4 = *(float *)(iVar8 + 0x30);
                  }
                  local_4c8 = local_390[iVar11 * 3] + *(float *)(iVar15 + 4);
                  local_4c4 = local_390[iVar11 * 3 + 1] + *(float *)(iVar15 + 8);
                  local_4c0 = local_390[iVar11 * 3 + 2] + *(float *)(iVar15 + 0xc);
                  fVar2 = (local_4c8 - DAT_007c3114) * DAT_007c3120 +
                          (local_4c4 - DAT_007c3118) * DAT_007c3124 +
                          (local_4c0 - DAT_007c311c) * DAT_007c3128;
                  if ((*(float *)(iVar8 + 0x24) < fVar2) && (fVar2 < local_4d4)) {
                    if (0.0 <= (fVar2 - *(float *)(iVar8 + 0x24)) /
                               (*(float *)(iVar8 + 0x28) - *(float *)(iVar8 + 0x24))) {
                      if ((fVar2 - *(float *)(iVar8 + 0x24)) /
                          (*(float *)(iVar8 + 0x28) - *(float *)(iVar8 + 0x24)) <= 1.0) {
                        fVar5 = (fVar2 - *(float *)(iVar8 + 0x24)) /
                                (*(float *)(iVar8 + 0x28) - *(float *)(iVar8 + 0x24));
                      }
                      else {
                        fVar5 = 1.0;
                      }
                    }
                    else {
                      fVar5 = 0.0;
                    }
                    if (0.0 <= (fVar2 - *(float *)(iVar8 + 0x2c)) /
                               (local_4d4 - *(float *)(iVar8 + 0x2c))) {
                      if ((fVar2 - *(float *)(iVar8 + 0x2c)) /
                          (local_4d4 - *(float *)(iVar8 + 0x2c)) <= 1.0) {
                        fVar2 = (fVar2 - *(float *)(iVar8 + 0x2c)) /
                                (local_4d4 - *(float *)(iVar8 + 0x2c));
                      }
                      else {
                        fVar2 = 1.0;
                      }
                    }
                    else {
                      fVar2 = 0.0;
                    }
                    if ((short)local_4bc < 1) goto LAB_0045921d;
                    sVar17 = 0;
                    goto LAB_004591b0;
                  }
                }
                break;
              }
LAB_00459021:
              iVar19 = iVar19 + 1;
            } while ((short)iVar19 < sVar7);
          }
          goto LAB_004592bb;
        }
LAB_004592c7:
        FUN_00511620();
        puVar18 = local_4b0;
        iVar15 = local_484;
      }
      local_4b8 = local_4b8 + 1;
      iVar8 = (int)(short)local_4b8;
    } while (iVar8 < *(int *)(iVar15 + 0x24));
  }
  return;
  while (sVar17 = sVar17 + 1, sVar17 < (short)local_4bc) {
LAB_004591b0:
    iVar19 = *(int *)(local_458[sVar17] * 0x20 + 0x14 + *(int *)(local_4a0 + 0x1c4));
    sVar14 = 0;
    if (0 < iVar19) {
      iVar11 = *(int *)(local_458[sVar17] * 0x20 + *(int *)(local_4a0 + 0x1c4) + 0x18);
      iVar12 = 0;
      do {
        pfVar13 = (float *)(iVar12 * 0x10 + iVar11);
        if ((local_4c8 * *pfVar13 +
            local_4c4 * pfVar13[1] + local_4c0 * *(float *)(iVar12 * 0x10 + 8 + iVar11)) -
            pfVar13[3] < 0.0) break;
        sVar14 = sVar14 + 1;
        iVar12 = (int)sVar14;
      } while (iVar12 < iVar19);
    }
    if (sVar14 == iVar19) goto LAB_004592bb;
  }
LAB_0045921d:
  pfVar13 = (float *)(iVar15 + 0x1c);
  if (*(short *)(iVar8 + 0x1a6) != 1) {
    pfVar13 = (float *)(iVar15 + 0x10);
  }
  sVar17 = *(short *)(iVar8 + 0x1a4);
  if ((sVar17 != 0) &&
     (pfVar13[2] * pfVar13[2] + pfVar13[1] * pfVar13[1] + *pfVar13 * *pfVar13 == 0.0)) {
    pfVar13 = (float *)PTR_DAT_00696720;
  }
  __ftol(&local_4c8,pfVar13,*(undefined4 *)(iVar15 + 0x30),
         (*(float *)(iVar15 + 0x44) + *(float *)(iVar15 + 0x44)) * *(float *)(iVar8 + 0x154),
         iVar15 + 0x34,(1.0 - fVar2) * fVar5,0);
  render_billboard_quad_build(CONCAT22((short)((uint)iVar19 >> 0x10),sVar17));
LAB_004592bb:
  uVar10 = *(uint *)(iVar15 + 0x50);
  if (uVar10 == 0xffffffff) goto LAB_004592c7;
  goto LAB_00458f80;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
