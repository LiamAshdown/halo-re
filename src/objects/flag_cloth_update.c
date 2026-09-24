// flag_cloth_update
// address 0x4fbae0, size 1340 bytes
// name confidence: 0.5 (functions.md: "Updates one timestep of the flag's cloth simulation,
//   blending each vertex toward a wind-perturbed target position relative to the flag pole's
//   current marker positions")
// rewrite confidence: 0.3 (large, dense cloth solver; transliterated closely from Ghidra rather
//   than restructured, several scratch buffers preserved as raw offsets)
// evidence: types/objects.h flag (invalid 0x02, vertices 0x1c stride 0x18); types/tags.h Flag
//   (width 0x0c, height 0x0e, cell_width 0x10, cell_height 0x14, physics TagDependency 0x28,
//   wind_noise 0x38); flag_pole_get_marker_positions 0x4fc020 (this file group);
//   point_physics_tick 0x50b530 (established in antenna_update_physics.c); math/README.md
//   sphere_point_table (0x006b7af4) / sphere_point_table_count (0x006b7af8); types/objects.h
//   globals list (widget_random_seed 0x00719cd4).
// register convention: three clean stack parameters (entry, tag data, dt); Ghidra shows no
//   in_REG/unaff_ markers for this function's own parameters (unlike its callees).
// blam-cc: stack -> entry, tag, dt
// UNSURE: `entry+4` (tested as a byte, `retracting`) and `entry+6` (the neighbour-average
//   "corner" scratch this reads and writes as a float array base, `afStack_29c`) do not
//   correspond to named fields in types/objects.h's `flag`; the struct only documents 0x00..
//   0x1c and the vertex/cell-code arrays past it, so both are kept as raw offsets.
// UNSURE: the neighbour offset table (`local_2e0`), the corner-weight table it walks, and the
//   exact selection of `mode` (1 vs 3) passed to point_physics_tick are preserved as literal
//   arithmetic; their physical meaning (which of the four grid neighbours contributes, and why
//   the "retracting" flag changes the wind scale by 0.0004 vs 0.00016) was not independently
//   re-derived.
// UNSURE: the implicit ESI velocity in/out pointer point_physics_tick reads and writes (see
//   antenna_update_physics.c's header) is not resolved for this call site; a throwaway local is
//   passed so the call shape matches, but its true target was not confirmed by disassembly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern tag_instance *tag_instances;            // 0x0087bc14
extern uint32_t widget_random_seed;            // 0x00719cd4
extern real_point3d *sphere_point_table;       // 0x006b7af4
extern int16_t sphere_point_table_count;       // 0x006b7af8
extern double sqrt(double x);

extern void flag_pole_get_marker_positions(flag *entry, bsp_leaf_reference *node_ref,
                                            real_point3d *marker_positions, uint8_t *row_table,
                                            int16_t *row_start_scratch, int16_t *column_marker_index,
                                            Flag *tag); // this module, 0x4fc020 (EDI -> tag)
extern int8_t FUN_0053ed60(int32_t *a, void *b); // out of module scope, unexamined
extern uint32_t point_physics_tick(real_vector3d *velocity /*ESI*/, uint32_t mode,
                                      void *physics_tag_data, bsp_leaf_reference *node_ref,
                                      uint32_t flags, real_point3d *position,
                                      real_vector3d *wind_direction, void *unused_c, void *unused_d,
                                      float damping_constant, float dt); // 0x50b530

void flag_cloth_update(flag *entry, Flag *tag, float dt) // blam-cc: stack -> entry, tag, dt
{
    int retracting = (*((uint8_t *)entry + 4) == 0); // UNSURE, see file header
    bsp_leaf_reference node_ref;
    uint32_t physics_a = 0, physics_b = 0; // local_2c0/local_2c4 pair Ghidra never showed a
                                            // producer for beyond FUN_004fc020's node_ref write;
                                            // preserved as opaque scratch. UNSURE.
    real_point3d marker_positions[8];    // local_26c, sized generously for attachment_points.count
    uint8_t row_table[488];              // local_1e0, 120 floats
    int16_t row_start_scratch[8];        // local_2a8-and-beyond in the original, unread by us
    int16_t column_marker_index[40];     // local_230
    int8_t moving;

    flag_pole_get_marker_positions(entry, &node_ref, marker_positions, row_table,
                                    row_start_scratch, column_marker_index, tag);
    moving = FUN_0053ed60((int32_t *)&physics_a, (void *)&physics_b);

    if (entry->invalid == 0) {
        // The three grid neighbours this solver samples (delta-column, delta-row), read out of
        // Ghidra's flat int16[6] `local_2e0` as three (first,second) pairs: (-1,0), (0,1),
        // (0,-1) -- the previous column, next row and previous row (never the next column,
        // which this column-by-column sweep has not computed yet).
        int16_t neighbour_dcol[3] = { -1, 0, 0 };
        int16_t neighbour_drow[3] = { 0, 1, -1 };
        float corner_length[3];
        int32_t n0;

        for (n0 = 0; n0 < 3; n0++) {
            float fx = (float)neighbour_dcol[n0] * tag->cell_width;
            float fy = (float)neighbour_drow[n0] * tag->cell_height;
            corner_length[n0] = (float)sqrt(fx * fx + fy * fy);
        }

        if (tag->width > 0) {
            int32_t row_step = ((int16_t)retracting != 0) ? 1 : -1;
            int16_t col = 0;

            do {
                int16_t row_cursor;
                int16_t bound;

                row_cursor = (col == 0) ? (int16_t)(tag->height - 1) : 0;

                for (;;) {
                    real_point3d *vertex;
                    real_point3d target;
                    int32_t contributor_count;
                    uint32_t mode;
                    float wind_scale;
                    real_vector3d wind_dir;

                    if (col == 0) {
                        if (!(0 < row_cursor)) break;
                    } else {
                        if (!(row_cursor < tag->height)) break;
                    }

                    vertex = (real_point3d *)((uint8_t *)entry + 0x1c + (tag->height * col + row_cursor) * 0x18);
                    contributor_count = 0;
                    mode = 1;

                    if (!moving) {
                        wind_scale = *(float *)((uint8_t *)tag_instances[tag->physics.tag_id.index].data + 0x24) *
                                     tag->wind_noise * 0.0004f;
                    } else {
                        mode = 3;
                        wind_scale = *(float *)((uint8_t *)tag_instances[tag->physics.tag_id.index].data + 0x28) *
                                     tag->wind_noise * 0.00016f;
                    }

                    widget_random_seed = widget_random_seed * 0x19660dU + 0x3c6ef35fU;
                    {
                        int16_t idx = (int16_t)(((widget_random_seed >> 16) *
                                                  (uint32_t)sphere_point_table_count) >> 16);
                        real_point3d *dir = &sphere_point_table[idx];
                        wind_dir.i = dir->x * wind_scale;
                        wind_dir.j = dir->y * wind_scale;
                        wind_dir.k = dir->z * wind_scale;
                    }

                    target = *vertex;

                    point_physics_tick(0 /*ESI, UNSURE*/, mode,
                        tag_instances[tag->physics.tag_id.index].data, &node_ref,
                        physics_b, &target, &wind_dir, 0, 0, 0.02f, dt);

                    if (col == 0 && column_marker_index[row_cursor] != -1) {
                        int32_t m = column_marker_index[row_cursor];
                        target = marker_positions[m];
                    } else {
                        real_point3d contributions[3];
                        int32_t n;

                        for (n = 0; n < 3; n++) {
                            int16_t ncol = neighbour_dcol[n] + col;
                            int16_t nrow = neighbour_drow[n] + row_cursor;

                            if (ncol >= 0 && ncol < tag->width && nrow >= 0 && nrow < tag->height) {
                                real_point3d *nvp = (real_point3d *)((uint8_t *)entry + 0x1c +
                                                                      (tag->height * ncol + nrow) * 0x18);
                                float dx = target.x - nvp->x;
                                float dy = target.y - nvp->y;
                                float dz = target.z - nvp->z;
                                float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

                                if (dist >= 0.0001f || dist <= -0.0001f) {
                                    float inv = 1.0f / dist;
                                    dx *= inv; dy *= inv; dz *= inv;
                                }

                                contributions[contributor_count].x = dx * corner_length[n] + nvp->x;
                                contributions[contributor_count].y = dy * corner_length[n] + nvp->y;
                                contributions[contributor_count].z = dz * corner_length[n] + nvp->z;
                                contributor_count++;
                            }
                        }

                        {
                            float sum_x = 0.0f, sum_y = 0.0f, sum_z = 0.0f, weight_total = 0.0f;
                            int32_t idx2;

                            for (idx2 = 0; idx2 < contributor_count; idx2++) {
                                float w = (col == 0 && idx2 == 0) ? 4.0f : 1.0f;
                                sum_x += w * contributions[idx2].x;
                                sum_y += w * contributions[idx2].y;
                                sum_z += w * contributions[idx2].z;
                                weight_total += w;
                            }
                            if (col == 0) {
                                float *row_positions = (float *)row_table;
                                sum_x += row_positions[row_cursor * 3 + 0] * 4.0f;
                                sum_y += row_positions[row_cursor * 3 + 1] * 4.0f;
                                sum_z += row_positions[row_cursor * 3 + 2] * 4.0f;
                                weight_total += 4.0f;
                            }
                            weight_total = 1.0f / weight_total;
                            target.x = weight_total * sum_x;
                            target.y = weight_total * sum_y;
                            target.z = weight_total * sum_z;
                        }
                    }

                    {
                        float inv_dt = 1.0f / dt;
                        real_vector3d *velocity_slot = (real_vector3d *)((uint8_t *)vertex + 0x0c);
                        velocity_slot->i = (target.x - vertex->x) * inv_dt;
                        velocity_slot->j = (target.y - vertex->y) * inv_dt;
                        velocity_slot->k = (target.z - vertex->z) * inv_dt;
                        *vertex = target;
                    }

                    row_cursor = (int16_t)(row_cursor + row_step);
                }

                col = col + 1;
            } while (col < tag->width);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4fbae0):

void flag_cloth_update(int param_1,int param_2,float param_3)

{
  float *pfVar1;
  float *pfVar2;
  bool bVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  char cVar7;
  int iVar8;
  short sVar9;
  short sVar10;
  int iVar11;
  undefined4 uVar12;
  short sVar13;
  short sVar14;
  uint uVar15;
  float afStackY_60290 [44];
  float afStackY_601e0 [98218];
  uint local_300;
  int local_2f8;
  float local_2f0;
  float local_2ec;
  float local_2e8;
  int local_2e4;
  short local_2e0 [6];
  float local_2d4;
  float local_2d0;
  float local_2cc;
  uint local_2c8;
  undefined4 local_2c4;
  undefined4 local_2c0;
  undefined4 local_2bc;
  int local_2b8;
  float local_2ac;
  undefined4 local_2a8;
  undefined4 local_2a4;
  float afStack_29c [12];
  float local_26c [15];
  short local_230 [40];
  float local_1e0 [120];

  local_2c8 = (uint)(*(char *)(param_1 + 4) == '\0');
  FUN_004fc020(param_1,&local_2c0,local_26c,local_1e0,&local_2a8,local_230);
  cVar7 = FUN_0053ed60(&local_2c0,&local_2c4);
  if (*(char *)(param_1 + 2) == '\0') {
    local_2e0[0] = -1;
    local_2e0[5] = 0xffff;
    local_2e0[1] = 0;
    local_2e0[2] = 0;
    local_2e0[3] = 1;
    local_2e0[4] = 0;
    iVar11 = 3;
    iVar8 = 0;
    do {
      iVar11 = iVar11 + -1;
      fVar4 = (float)(int)*(short *)((int)local_2e0 + iVar8) * *(float *)(param_2 + 0x10);
      fVar5 = (float)(int)*(short *)((int)local_2e0 + iVar8 + 2) * *(float *)(param_2 + 0x14);
      *(float *)((int)afStack_29c + iVar8) = SQRT(fVar4 * fVar4 + fVar5 * fVar5);
      iVar8 = iVar8 + 4;
    } while (iVar11 != 0);
    if (0 < *(short *)(param_2 + 0xc)) {
      local_2b8 = (uint)((short)local_2c8 != 0) * 2 + -1;
      sVar14 = 0;
      uVar15 = local_2c8;
      do {
        if ((short)uVar15 == 0) {
          local_300 = (uint)(ushort)(*(short *)(param_2 + 0xe) - 1);
        }
        else {
          local_300 = 0;
        }
        while( true ) {
          sVar9 = (short)local_300;
          if ((short)uVar15 == 0) {
            bVar3 = 0 < sVar9;
          }
          else {
            bVar3 = sVar9 < *(short *)(param_2 + 0xe);
          }
          if (!bVar3) break;
          pfVar2 = (float *)(param_1 + 0x1c +
                            ((int)*(short *)(param_2 + 0xe) * (int)sVar14 + (int)sVar9) * 0x18);
          local_2e4 = 0;
          uVar12 = 1;
          if (cVar7 == '\0') {
            local_2cc = *(float *)(*(int *)((*(uint *)(param_2 + 0x34) & 0xffff) * 0x20 + 0x14 +
                                           DAT_0087bc14) + 0x24) * *(float *)(param_2 + 0x38) *
                        0.0004;
          }
          else {
            uVar12 = 3;
            local_2cc = *(float *)(*(int *)((*(uint *)(param_2 + 0x34) & 0xffff) * 0x20 + 0x14 +
                                           DAT_0087bc14) + 0x28) * *(float *)(param_2 + 0x38) *
                        0.00016;
          }
          DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
          pfVar1 = (float *)(DAT_006b7af4 +
                            (short)((DAT_00719cd4 >> 0x10) * (int)DAT_006b7af8 >> 0x10) * 0xc);
          local_2d4 = *pfVar1 * local_2cc;
          local_2f0 = *pfVar2;
          local_2ec = pfVar2[1];
          local_2d0 = pfVar1[1] * local_2cc;
          local_2e8 = pfVar2[2];
          local_2a8 = local_2c0;
          local_2cc = pfVar1[2] * local_2cc;
          local_2a4 = local_2bc;
          FUN_0050b530(uVar12,*(undefined4 *)
                               ((*(uint *)(param_2 + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
                       &local_2a8,local_2c4,&local_2f0,&local_2d4,0,0,0x3ca3d70a,param_3);
          if ((sVar14 == 0) && (local_230[sVar9] != -1)) {
            iVar8 = (int)local_230[sVar9];
            local_2f0 = local_26c[iVar8 * 3];
            local_2ec = local_26c[iVar8 * 3 + 1];
            local_2e8 = local_26c[iVar8 * 3 + 2];
          }
          else {
            iVar8 = 0;
            local_2f8 = 3;
            do {
              sVar13 = *(short *)((int)local_2e0 + iVar8 + 2) + sVar9;
              sVar10 = *(short *)((int)local_2e0 + iVar8) + sVar14;
              if ((((-1 < sVar10) && (sVar10 < *(short *)(param_2 + 0xc))) && (-1 < sVar13)) &&
                 (sVar13 < *(short *)(param_2 + 0xe))) {
                pfVar1 = (float *)(param_1 + 0x1c +
                                  ((int)*(short *)(param_2 + 0xe) * (int)sVar10 + (int)sVar13) *
                                  0x18);
                fVar4 = local_2f0 - *pfVar1;
                fVar6 = local_2ec - pfVar1[1];
                local_2ac = local_2e8 - pfVar1[2];
                fVar5 = SQRT(fVar4 * fVar4 + fVar6 * fVar6 + local_2ac * local_2ac);
                if (0.0001 <= ABS(fVar5)) {
                  fVar5 = 1.0 / fVar5;
                  fVar4 = fVar5 * fVar4;
                  fVar6 = fVar5 * fVar6;
                  local_2ac = fVar5 * local_2ac;
                }
                iVar11 = (int)(short)local_2e4;
                local_2e4 = local_2e4 + 1;
                afStack_29c[iVar11 * 3 + 3] = fVar4 * *(float *)((int)afStack_29c + iVar8) + *pfVar1
                ;
                afStack_29c[iVar11 * 3 + 4] =
                     fVar6 * *(float *)((int)afStack_29c + iVar8) + pfVar1[1];
                afStack_29c[iVar11 * 3 + 5] =
                     local_2ac * *(float *)((int)afStack_29c + iVar8) + pfVar1[2];
              }
              iVar8 = iVar8 + 4;
              local_2f8 = local_2f8 + -1;
            } while (local_2f8 != 0);
            sVar10 = 0;
            fVar4 = 0.0;
            local_2ec = 0.0;
            local_2f0 = 0.0;
            local_2e8 = 0.0;
            if (0 < (short)local_2e4) {
              do {
                if ((sVar14 == 0) || (sVar10 != 0)) {
                  fVar5 = 1.0;
                }
                else {
                  fVar5 = 4.0;
                }
                iVar8 = (int)sVar10;
                sVar10 = sVar10 + 1;
                local_2f0 = fVar5 * afStack_29c[iVar8 * 3 + 3] + local_2f0;
                local_2ec = fVar5 * afStack_29c[iVar8 * 3 + 4] + local_2ec;
                fVar4 = fVar5 * afStack_29c[iVar8 * 3 + 5] + fVar4;
                local_2e8 = fVar5 + local_2e8;
              } while (sVar10 < (short)local_2e4);
            }
            if (sVar14 == 0) {
              iVar8 = (int)sVar9;
              local_2f0 = local_1e0[iVar8 * 3] * 4.0 + local_2f0;
              local_2ec = local_1e0[iVar8 * 3 + 1] * 4.0 + local_2ec;
              fVar4 = local_1e0[iVar8 * 3 + 2] * 4.0 + fVar4;
              local_2e8 = local_2e8 + 4.0;
            }
            local_2e8 = 1.0 / local_2e8;
            local_2f0 = local_2e8 * local_2f0;
            local_2ec = local_2e8 * local_2ec;
            local_2e8 = local_2e8 * fVar4;
          }
          fVar4 = 1.0 / param_3;
          pfVar2[3] = (local_2f0 - *pfVar2) * fVar4;
          pfVar2[4] = (local_2ec - pfVar2[1]) * fVar4;
          pfVar2[5] = (local_2e8 - pfVar2[2]) * fVar4;
          *pfVar2 = local_2f0;
          pfVar2[1] = local_2ec;
          pfVar2[2] = local_2e8;
          local_300 = local_300 + local_2b8;
          uVar15 = local_2c8;
        }
        sVar14 = sVar14 + 1;
      } while (sVar14 < *(short *)(param_2 + 0xc));
    }
  }
  return;
}
#endif
