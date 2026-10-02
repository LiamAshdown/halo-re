// detail_objects_update_render_list  (Ghidra: FUN_005522d0; named here)
// address 0x5522d0, size 1066 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: types/structures.h's detail objects section (detail_object_frame, detail_object_batch,
//   detail_object_layer_batches, detail_object_render_list, detail_object_globals) is derived
//   directly from this function's arithmetic; disassembly (objdump -d -M intel bin/halo.exe,
//   0x5522d0..0x5526f1 and 0x552710/0x552780) confirms: the two search calls take the detail
//   object cell array as {begin=ECX, end=EAX} register arguments (cells.pointer and
//   cells.pointer + cells.count*0x20) and a detail_object_cell_key on the stack; the key's third
//   field (cell_z) really is compared inside lower_bound/upper_bound, giving this a genuine
//   3-field lexicographic bracket, not just a 2-field (x,y) one.
// register convention: none (void); this function reads only tag data and the global camera
//   block.
// REWRITTEN 2026-09-28 against objdump 0x5522d0..0x552700: the draft's "UNSURE (major)" was a real bug --
//   0x552421 stores the camera cell z into the search key's z word before the sweep, so the first
//   lower_bound searches z-1 like every later one; the draft read an uninitialized local there and
//   could skip the first (x+1, y+1) cell's detail objects. Everything else (3x3 sweep from +1 down,
//   |dz| <= 1, 27 batches x 32 layers, 1/255 offset_z, default z reference at +0xa420) matches.
// UNSURE: rasterizer_detail_objects_begin (called with no visible arguments -- possibly a "begin detail object frame"
//   marker) is not examined.
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "structures.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern player_globals *local_player_globals; // 0x0087a478, game.h
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, physics.h/objects.h (read, not owned)
extern detail_object_globals *detail_objects; // 0x0072277c, this module
extern int16_t current_local_player_index; // 0x007c3108, this module (read, not owned)
extern real_point3d render_camera_global; // 0x007c3114, this module (read, not owned)
extern long lrint(double x); // x87 fistp under the default control word (round-half-to-even);
    // the same reading src/math/periodic_function_evaluate.c gives Ghidra's ROUND()

extern void rasterizer_detail_objects_begin(void); // 0x51b3f0, foreign render module; UNSURE, no visible arguments
extern void rasterizer_detail_objects_vertex_buffer_fill(detail_object_render_list *render_list); // 0x51b6f0, foreign, submit
extern void rasterizer_detail_objects_draw(detail_object_render_list *render_list); // 0x51b890, foreign, draw
extern ScenarioStructureBSPGlobalDetailObjectCell *detail_object_cell_lower_bound(
    ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end,
    detail_object_cell_key *key); // 0x552710, this batch; blam-cc: ECX -> begin, EAX -> end
extern ScenarioStructureBSPGlobalDetailObjectCell *detail_object_cell_upper_bound(
    ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end,
    detail_object_cell_key *key); // 0x552780, this batch; blam-cc: ECX -> begin, EAX -> end

// Rebuilds (when the camera has moved to a new 8-world-unit detail-object cell, or is forced to)
// the per-layer batch lists of detail_objects->frames[0] from the tag's detail object cells within
// a 3x3 neighborhood of the camera's cell, then submits and draws the frame's render list. A no-op
// outside single-local-player, in-BSP rendering.
void detail_objects_update_render_list(void)
{
    ScenarioStructureBSPDetailObjectData *detail_data;
    detail_object_frame *frame = &detail_objects->frames[0];
    int16_t cell_x, cell_y, cell_z;

    if (local_player_globals->local_player_count != 1 || current_local_player_index == -1) {
        return;
    }

    detail_data = (global_structure_bsp->detail_objects.count == 0)
        ? (ScenarioStructureBSPDetailObjectData *)0
        : (ScenarioStructureBSPDetailObjectData *)global_structure_bsp->detail_objects.pointer;

    cell_x = (int16_t)(int32_t)lrint((double)(render_camera_global.x * 0.125f - 0.5f));
    cell_y = (int16_t)(int32_t)lrint((double)(render_camera_global.y * 0.125f - 0.5f));
    cell_z = (int16_t)(int32_t)lrint((double)(render_camera_global.z * 0.125f - 0.5f));

    if (detail_data->bullshit != 0) {
        rasterizer_detail_objects_begin(); // UNSURE: see file header

        if (cell_x != frame->cell_x || cell_y != frame->cell_y || cell_z != frame->cell_z ||
            frame->valid == 0 || (detail_data->bullshit & 2) != 0) {
            // --- rebuild the frame ---
            int16_t layer_batch_counts[k_maximum_detail_object_layers];
            uint32_t layers_used = 0; // OR of every visited cell's valid_layers_flags
            int32_t layer;
            int32_t x_scan, y_scan_base, y_scan;
            int32_t z_key = cell_z; // 0x552421: the key's z word (esp+0x20) starts at the camera cell's z
            int32_t dx, dy;

            for (layer = 0; layer < k_maximum_detail_object_layers; layer = layer + 1) {
                layer_batch_counts[layer] = 0;
            }

            detail_data->bullshit = 1;
            frame->cell_x = cell_x;
            frame->cell_y = cell_y;
            frame->cell_z = cell_z;
            frame->valid = 1;
            frame->unknown_520f = 0;

            x_scan = (int32_t)cell_x + 1;      // decremented each outer iteration: +1, 0, -1
            y_scan_base = (int32_t)cell_y + 1; // reloaded into y_scan each outer iteration

            for (dx = 0; dx < 3; dx = dx + 1) {
                y_scan = y_scan_base;
                for (dy = 0; dy < 3; dy = dy + 1) {
                    detail_object_cell_key key;
                    ScenarioStructureBSPGlobalDetailObjectCell *lo, *hi;

                    key.cell_x = (int16_t)x_scan;
                    key.cell_y = (int16_t)y_scan;
                    key.unknown_06 = 0;

                    key.cell_z = (int16_t)(z_key - 1);
                    lo = detail_object_cell_lower_bound(
                        (ScenarioStructureBSPGlobalDetailObjectCell *)detail_data->cells.pointer,
                        (ScenarioStructureBSPGlobalDetailObjectCell *)detail_data->cells.pointer + detail_data->cells.count,
                        &key);
                    key.cell_z = (int16_t)(z_key + 2);
                    hi = detail_object_cell_upper_bound(
                        (ScenarioStructureBSPGlobalDetailObjectCell *)detail_data->cells.pointer,
                        (ScenarioStructureBSPGlobalDetailObjectCell *)detail_data->cells.pointer + detail_data->cells.count,
                        &key);
                    z_key = cell_z; // reset for the next (x, y) search, matching the original

                    if (lo->cell_x == (int16_t)x_scan && lo->cell_y == (int16_t)y_scan &&
                        hi[-1].cell_x == (int16_t)x_scan && hi[-1].cell_y == (int16_t)y_scan &&
                        lo < hi) {
                        ScenarioStructureBSPGlobalDetailObjectCell *cell;

                        for (cell = lo; cell < hi; cell = cell + 1) {
                            int32_t z_diff = (int32_t)cell_z - (int32_t)cell->cell_z;
                            if ((z_diff < 0 ? -z_diff : z_diff) < 2) {
                                int32_t running_instance_offset = 0;
                                int32_t sub_index = 0;
                                uint32_t bit;

                                layers_used = layers_used | cell->valid_layers_flags;

                                for (layer = 0, bit = 1; layer < k_maximum_detail_object_layers;
                                     layer = layer + 1, bit = bit << 1) {
                                    if ((cell->valid_layers_flags & bit) != 0) {
                                        int16_t batch_index_in_layer = layer_batch_counts[layer];
                                        detail_object_batch *batch = &frame->batches[layer][batch_index_in_layer];

                                        batch->cell_x = cell->cell_x;
                                        layer_batch_counts[layer] = batch_index_in_layer + 1;
                                        batch->cell_y = cell->cell_y;
                                        batch->cell_z = (float)cell->cell_z + (float)cell->offset_z * 0.003921569f;
                                        batch->first_instance = (int32_t)cell->start_index + running_instance_offset;

                                        {
                                            uint16_t *counts = (uint16_t *)detail_data->counts.pointer;
                                            batch->instance_count = counts[cell->count_index + sub_index];
                                        }

                                        if (detail_data->z_reference_vectors.count == 0) {
                                            batch->z_reference = &detail_objects->default_z_reference;
                                        } else {
                                            batch->z_reference = (ScenarioStructureBSPGlobalZReferenceVector *)
                                                detail_data->z_reference_vectors.pointer + (cell->count_index + sub_index);
                                        }

                                        running_instance_offset = running_instance_offset + batch->instance_count;
                                        sub_index = sub_index + 1;
                                    }
                                }
                            }
                        }
                    }

                    y_scan = y_scan - 1;
                }
                x_scan = x_scan - 1;
            }

            // --- build the render list from every layer that received at least one batch ---
            frame->render_list.layers = &frame->layers[0];
            frame->render_list.layer_count = 0;
            for (layer = 0; layer < k_maximum_detail_object_layers; layer = layer + 1) {
                if ((layers_used & (1u << layer)) != 0 && layer_batch_counts[layer] != 0) {
                    int16_t out_index = frame->render_list.layer_count;
                    frame->layers[out_index].batches = &frame->batches[layer][0];
                    frame->layers[out_index].batch_count = layer_batch_counts[layer];
                    frame->layers[out_index].layer_index = (int16_t)layer;
                    frame->render_list.layer_count = frame->render_list.layer_count + 1;
                }
            }
            rasterizer_detail_objects_vertex_buffer_fill(&frame->render_list);
        }

        rasterizer_detail_objects_draw(&frame->render_list);
    }
}

#if 0
Original Ghidra decompilation (0x5522d0):

void FUN_005522d0(void)

{
  int *piVar1;
  short sVar2;
  short *psVar3;
  short *psVar4;
  byte bVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  short sVar9;
  int iVar10;
  short sVar11;
  undefined4 *puVar12;
  int local_90;
  int local_8c;
  short *local_88;
  short local_84;
  short local_82;
  short local_80;
  undefined2 local_7e;
  int *local_7c;
  short local_78;
  short sStack_76;
  short sStack_74;
  undefined2 local_72;
  uint local_70;
  int local_6c;
  int local_68;
  int local_64;
  int local_60;
  int local_5c;
  int local_58;
  int local_54;
  int local_50;
  int local_4c;
  short local_48;
  undefined4 local_46 [16];

  iVar10 = DAT_0072277c;
  if ((*(short *)(DAT_0087a478 + 0xc) == 1) && (DAT_007c3108 != -1)) {
    if (*(int *)(DAT_00746f9c + 0x24c) == 0) {
      iVar8 = 0;
    }
    else {
      iVar8 = *(int *)(DAT_00746f9c + 0x250);
    }
    local_5c = DAT_0072277c;
    local_90._0_2_ = (short)(int)ROUND(DAT_007c3114 * 0.125 - 0.5);
    sVar9 = (short)local_90;
    local_78 = (short)local_90;
    local_90._0_2_ = (short)(int)ROUND(DAT_007c3118 * 0.125 - 0.5);
    sStack_76 = (short)local_90;
    local_90._0_2_ = (short)(int)ROUND(DAT_007c311c * 0.125 - 0.5);
    sStack_74 = (short)local_90;
    local_72 = 0;
    if (*(char *)(iVar8 + 0x30) != '\0') {
      FUN_0051b3f0();
      if ((((sVar9 != *(short *)(iVar10 + 21000)) || (sStack_76 != *(short *)(iVar10 + 0x520a))) ||
          ((short)local_90 != *(short *)(iVar10 + 0x520c))) ||
         ((*(char *)(iVar10 + 0x520e) == '\0' || ((*(byte *)(iVar8 + 0x30) & 2) != 0)))) {
        local_48 = 0;
        local_70 = 0;
        puVar12 = local_46;
        for (iVar6 = 0xf; iVar6 != 0; iVar6 = iVar6 + -1) {
          *puVar12 = 0;
          puVar12 = puVar12 + 1;
        }
        *(undefined2 *)puVar12 = 0;
        *(undefined1 *)(iVar8 + 0x30) = 1;
        local_4c = CONCAT22(sStack_74,sStack_76) + 1;
        *(int *)(iVar10 + 21000) = CONCAT22(sStack_76,local_78);
        local_88 = (short *)(CONCAT22(sStack_76,local_78) + 1);
        *(uint *)(iVar10 + 0x520c) = CONCAT22(local_72,sStack_74);
        *(undefined1 *)(iVar10 + 0x520e) = 1;
        local_80 = (short)local_90;
        local_8c = 3;
        do {
          local_60 = local_4c;
          local_90 = 3;
          do {
            local_80 = local_80 + -1;
            local_84 = (short)local_88;
            local_82 = (short)local_60;
            local_7e = 0;
            psVar3 = (short *)FUN_00552710(&local_84);
            local_80 = local_80 + 3;
            psVar4 = (short *)FUN_00552780(&local_84);
            local_80 = sStack_74;
            if (((*psVar3 == local_84) && (psVar3[1] == local_82)) &&
               ((psVar4[-0x10] == local_84 && ((psVar4[-0xf] == local_82 && (psVar3 < psVar4)))))) {
              local_54 = ((uint)((int)psVar4 + (-1 - (int)psVar3)) >> 5) + 1;
              psVar3 = psVar3 + 2;
              do {
                uVar7 = (int)sStack_74 - (int)*psVar3 >> 0x1f;
                if ((int)(((int)sStack_74 - (int)*psVar3 ^ uVar7) - uVar7) < 2) {
                  local_70 = local_70 | *(uint *)(psVar3 + 2);
                  local_68 = 0;
                  local_6c = 0;
                  local_50 = 0;
                  local_64 = 0;
                  psVar4 = &local_48;
                  local_58 = 0x20;
                  do {
                    if ((*(uint *)(psVar3 + 2) & 1 << ((byte)local_50 & 0x1f)) != 0) {
                      sVar9 = *psVar4;
                      piVar1 = (int *)(local_5c + (sVar9 + local_64) * 0x18);
                      *(short *)(piVar1 + 2) = psVar3[-2];
                      *psVar4 = sVar9 + 1;
                      *(short *)((int)piVar1 + 10) = psVar3[-1];
                      local_7c = (int *)(int)*psVar3;
                      piVar1[3] = (int)((float)(int)psVar3[1] * 0.003921569 + (float)(int)local_7c);
                      *piVar1 = *(int *)(psVar3 + 4) + local_68;
                      piVar1[1] = (uint)*(ushort *)
                                         (*(int *)(iVar8 + 0x1c) +
                                         (*(int *)(psVar3 + 6) + (int)(short)local_6c) * 2);
                      if (*(int *)(iVar8 + 0x24) == 0) {
                        iVar10 = DAT_0072277c + 0xa420;
                      }
                      else {
                        iVar10 = (*(int *)(psVar3 + 6) + (int)(short)local_6c) * 0x10 +
                                 *(int *)(iVar8 + 0x28);
                      }
                      piVar1[5] = iVar10;
                      local_68 = local_68 + piVar1[1];
                      local_6c = local_6c + 1;
                    }
                    local_50 = local_50 + 1;
                    local_64 = local_64 + 0x1b;
                    psVar4 = psVar4 + 1;
                    local_58 = local_58 + -1;
                  } while (local_58 != 0);
                }
                psVar3 = psVar3 + 0x10;
                local_54 = local_54 + -1;
              } while (local_54 != 0);
            }
            local_60 = local_60 + -1;
            local_90 = local_90 + -1;
          } while (local_90 != 0);
          local_88 = (short *)((int)local_88 + -1);
          local_8c = local_8c + -1;
        } while (local_8c != 0);
        local_7c = (int *)(local_5c + 0x5200);
        *local_7c = local_5c + 0x5100;
        sVar11 = 0;
        local_88 = &local_48;
        sVar9 = 0;
        *(undefined2 *)(local_5c + 0x5204) = 0;
        bVar5 = 0;
        iVar10 = local_5c;
        do {
          if (((local_70 & 1 << (bVar5 & 0x1f)) != 0) && (sVar2 = *local_88, sVar2 != 0)) {
            piVar1 = (int *)(local_5c + 0x5100 + sVar9 * 8);
            sVar9 = sVar9 + 1;
            *piVar1 = iVar10;
            *(short *)(piVar1 + 1) = sVar2;
            *(short *)((int)piVar1 + 6) = sVar11;
            *(short *)(local_5c + 0x5204) = *(short *)(local_5c + 0x5204) + 1;
          }
          sVar11 = sVar11 + 1;
          local_88 = local_88 + 1;
          bVar5 = bVar5 + 1;
          iVar10 = iVar10 + 0x288;
        } while (sVar11 < 0x20);
        FUN_0051b6f0(local_7c);
        iVar10 = local_5c;
      }
      FUN_0051b890(iVar10 + 0x5200);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
