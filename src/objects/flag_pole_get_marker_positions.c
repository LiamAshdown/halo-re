// flag_pole_get_marker_positions
// address 0x4fc020, size 805 bytes
// name confidence: 0.7 (named directly in out/phase4/objects_types_notes.md: "Flag ... proved
//   by flag_new 0x4fb540, flag_cloth_init_shape_constraints 0x4fb770,
//   flag_pole_get_marker_positions 0x4fc020")
// rewrite confidence: 0.4 (raised from 0.3 by the phase-4 review pass: the leaf probe now passes marker_positions[0], as the disassembly shows) (large, dense function; the control flow and arithmetic are
//   transliterated closely from Ghidra rather than restructured, because several of its six
//   output buffers are used as raw, oddly-offset scratch regions by the one caller
//   (flag_cloth_update) whose exact intent was not independently re-derived here)
// evidence: types/objects.h flag (invalid 0x02, object_index 0x08, previous_marker_position
//   0x10, vertices 0x1c stride 0x18); types/tags.h Flag (width 0x0c, height 0x0e,
//   attachment_points TagReflexive 0x54/0x58); object_get_node_local_transform 0x4f6080
//   (all-stack cdecl, see antenna_apply_marker_delta.c); bsp3d_node_find_leaf (globals in ECX, point in
//   EDX, index in EAX, established in antenna_apply_marker_delta.c); __ftol 0x6391b4.
// register convention: unaff_EDI is the Flag tag pointer, inherited unmodified from
//   flag_cloth_update's own frame (this function never reloads it), the same pattern as
//   flag_cloth_mark_border_cells.c / flag_cloth_init_shape_constraints.c. All six of Ghidra's
//   shown param_1..param_6 are ordinary stack arguments (Ghidra resolved them cleanly, unlike
//   the EDI input).
// blam-cc: EDI -> tag, stack -> entry, node_ref, marker_positions, row_table, row_start_scratch,
//   column_marker_index
// UNSURE: `row_table` (param_4) is written starting 8 bytes into the buffer the caller passes
//   (`param_4 + 8 + row*0xc`), not at its start; kept as raw byte arithmetic rather than forced
//   into a clean array, since the leading 8 bytes' purpose was not identified.
// UNSURE: `row_start_scratch` (param_5) is written per attachment point but never read back by
//   flag_cloth_update, whose own `local_2a8`/`local_2a4` locals alias the same stack bytes only
//   because they are unconditionally overwritten again before their next use (verified by
//   reading flag_cloth_update.c's decompile) -- so this output has no observable effect on the
//   caller and is reproduced only for fidelity to the original arithmetic.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern uint8_t *structure_bsp_globals; // 0x00746f9c
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern int32_t bsp3d_node_find_leaf(void *globals, real_point3d *point, int32_t index); // 0x5013a0
extern void *global_globals; // 0x00746f90
extern int32_t __ftol(); // 0x006391b4, MSVC 7.1 CRT x87 float-to-int truncation
    // (verified by disassembling 0x006391b4: fld st(0) / fst [esp+0x18] / fistp qword /
    // fild qword ... , the classic _ftol2 body). The value arrives on the x87 stack, so
    // some call sites show a visible float argument and others show none; the empty
    // parameter list asserts no prototype, the same convention this module already uses
    // for FUN_00450870.

void flag_pole_get_marker_positions(flag *entry, bsp_leaf_reference *node_ref,
                                     real_point3d *marker_positions, uint8_t *row_table,
                                     int16_t *row_start_scratch, int16_t *column_marker_index,
                                     Flag *tag /*EDI*/)
    // blam-cc: EDI -> tag, stack -> entry, node_ref, marker_positions, row_table,
    //   row_start_scratch, column_marker_index
{
    int32_t i;

    // Resolve each pole attachment point's current marker world position.
    for (i = 0; i < (int32_t)tag->attachment_points.count; i++) {
        object_marker marker;
        object_get_node_local_transform(entry->object_index,
            (char *)((uint8_t *)tag->attachment_points.pointer + i * 0x34 + 0x14),
            &marker, 1);
        marker_positions[i] = marker.node_transform.position;
    }

    {
        // Resolved from the disassembly at 0x4fc093: after the marker loop, EDX is reloaded
        // with the `marker_positions` parameter itself, so the leaf probe is taken at
        // marker_positions[0] -- the first pole attachment point. (EAX is zeroed, ECX is the
        // 0x00746f90 globals pointer, exactly as in antenna_apply_marker_delta.)
        int32_t node_index = bsp3d_node_find_leaf(global_globals, &marker_positions[0], 0);

        node_ref->leaf_index = node_index;
        if (node_index == -1) {
            node_ref->cluster_index = -1;
        } else {
            node_ref->cluster_index = *(int16_t *)(*(uint8_t **)(structure_bsp_globals + 0xe4) +
                                                 (uint32_t)(node_index & 0x7fffffff) * 0x10 + 8);
        }
    }

    if (entry->invalid == 0) {
        int32_t row;

        for (row = 0; row < tag->height; row++) {
            column_marker_index[row] = -1;
        }

        {
            int32_t row_cursor = 0;
            int32_t point_index = 0;

            while (point_index < (int32_t)tag->attachment_points.count) {
                int16_t row16 = (int16_t)row_cursor;
                int16_t raw, span;
                int32_t row_end;

                if (tag->height <= row16) {
                    break;
                }

                raw = *(int16_t *)((uint8_t *)tag->attachment_points.pointer + point_index * 0x34);
                if (raw < 0) {
                    span = 0;
                } else {
                    int16_t remaining = tag->height - row16;
                    span = (remaining < raw) ? remaining : raw;
                }

                row_start_scratch[point_index] = row16;
                row_end = (span & ~1) + row_cursor;
                column_marker_index[row16] = (int16_t)point_index;

                if (row16 <= (int16_t)row_end) {
                    uint8_t *out = row_table + 8 + row16 * 0xc;
                    uint32_t count = (uint32_t)(uint16_t)((row_end - row_cursor) + 1);
                    int32_t interp_row = row16;

                    row_cursor = row_cursor + (int32_t)count;

                    do {
                        if ((int16_t)row_end != interp_row) {
                            real_point3d *base = &marker_positions[point_index];
                            float t = ((float)interp_row - (float)row16) /
                                      ((float)(int16_t)row_end - (float)row16);
                            float one_minus_t = 1.0f - t;

                            ((float *)out)[0] = one_minus_t * base[0].x + t * base[1].x;
                            ((float *)out)[1] = t * base[1].y + one_minus_t * base[0].y;
                            ((float *)out)[2] = t * base[1].z + one_minus_t * base[0].z;
                        }
                        interp_row = interp_row + 1;
                        out = out + 0xc;
                        count = count - 1;
                    } while (count != 0);
                }

                row_cursor = row_cursor - 1;
                point_index = point_index + 1;
            }
        }

        {
            real_point3d new_position = marker_positions[0];
            real_point3d old_position = entry->previous_marker_position;
            int32_t tx = __ftol((double)(new_position.x - old_position.x));
            int skip = (tx < 0 ? -tx : tx) <= 1;

            if (skip) {
                int32_t ty = __ftol((double)(new_position.y - old_position.y));
                skip = (ty < 0 ? -ty : ty) <= 1;
                if (skip) {
                    int32_t tz = __ftol((double)(new_position.z - old_position.z));
                    skip = (tz < 0 ? -tz : tz) <= 1;
                }
            }

            if (!skip && tag->width > 0) {
                real_vector3d delta;
                int16_t r;

                delta.i = new_position.x - old_position.x;
                delta.j = new_position.y - old_position.y;
                delta.k = new_position.z - old_position.z;

                for (r = 0; r < tag->width; r++) {
                    int16_t c;
                    for (c = 0; c < tag->height; c++) {
                        real_point3d *vertex_position =
                            (real_point3d *)((uint8_t *)entry + 0x1c + (tag->height * r + c) * 0x18);
                        vertex_position->x += delta.i;
                        vertex_position->y += delta.j;
                        vertex_position->z += delta.k;
                    }
                }
            }
        }
    }

    // Unconditional, even when entry->invalid != 0 (matches the original: these three writes
    // sit after the closing brace of the `invalid == 0` block, not inside it).
    entry->previous_marker_position = marker_positions[0];
}

#if 0
Original Ghidra decompilation (0x4fc020):

void FUN_004fc020(int param_1,int *param_2,float *param_3,int param_4,int param_5,int param_6)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  undefined2 uVar8;
  short sVar9;
  short sVar10;
  int iVar11;
  uint uVar12;
  int iVar13;
  float *pfVar14;
  short sVar15;
  uint uVar16;
  int iVar17;
  int iVar18;
  int unaff_EDI;
  int local_84;
  undefined1 local_6c [96];
  float local_c;
  float local_8;
  float local_4;

  sVar9 = 0;
  if (0 < *(int *)(unaff_EDI + 0x54)) {
    iVar17 = 0;
    do {
      FUN_004f6080(*(undefined4 *)(param_1 + 8),iVar17 * 0x34 + 0x14 + *(int *)(unaff_EDI + 0x58),
                   local_6c,1);
      pfVar14 = param_3 + iVar17 * 3;
      *pfVar14 = local_c;
      sVar9 = sVar9 + 1;
      pfVar14[1] = local_8;
      iVar17 = (int)sVar9;
      pfVar14[2] = local_4;
    } while (iVar17 < *(int *)(unaff_EDI + 0x54));
  }
  iVar17 = FUN_005013a0();
  *param_2 = iVar17;
  if (iVar17 == -1) {
    uVar8 = 0xffff;
  }
  else {
    uVar8 = *(undefined2 *)(iVar17 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  }
  *(undefined2 *)(param_2 + 1) = uVar8;
  if (*(char *)(param_1 + 2) == '\0') {
    sVar9 = 0;
    if (0 < *(short *)(unaff_EDI + 0xe)) {
      do {
        iVar17 = (int)sVar9;
        sVar9 = sVar9 + 1;
        *(undefined2 *)(param_6 + iVar17 * 2) = 0xffff;
      } while (sVar9 < *(short *)(unaff_EDI + 0xe));
    }
    iVar17 = 0;
    sVar9 = 0;
    if (0 < *(int *)(unaff_EDI + 0x54)) {
      do {
        sVar15 = (short)iVar17;
        if (*(short *)(unaff_EDI + 0xe) <= sVar15) break;
        iVar18 = (int)sVar9;
        sVar10 = *(short *)(iVar18 * 0x34 + *(int *)(unaff_EDI + 0x58));
        if (sVar10 < 0) {
          uVar16 = 0;
        }
        else {
          uVar12 = (int)*(short *)(unaff_EDI + 0xe) - (int)sVar15;
          uVar16 = (int)sVar10;
          if ((int)uVar12 < (int)sVar10) {
            uVar16 = uVar12;
          }
        }
        *(short *)(param_5 + iVar18 * 2) = sVar15;
        iVar13 = (int)sVar15;
        iVar11 = (uVar16 & 0xfffffffe) + iVar17;
        sVar10 = (short)iVar11;
        *(short *)(param_6 + iVar13 * 2) = sVar9;
        if (sVar15 <= sVar10) {
          pfVar14 = (float *)(param_4 + 8 + iVar13 * 0xc);
          uVar16 = (iVar11 - iVar17) + 1U & 0xffff;
          iVar17 = iVar17 + uVar16;
          local_84 = iVar13;
          do {
            if (sVar10 != iVar13) {
              pfVar1 = param_3 + iVar18 * 3;
              fVar3 = ((float)local_84 - (float)iVar13) / ((float)(int)sVar10 - (float)iVar13);
              fVar2 = 1.0 - fVar3;
              pfVar14[-2] = fVar2 * *pfVar1 + fVar3 * pfVar1[3];
              pfVar14[-1] = fVar3 * pfVar1[4] + fVar2 * pfVar1[1];
              *pfVar14 = fVar3 * pfVar1[5] + fVar2 * pfVar1[2];
            }
            local_84 = local_84 + 1;
            pfVar14 = pfVar14 + 3;
            uVar16 = uVar16 - 1;
          } while (uVar16 != 0);
        }
        iVar17 = iVar17 + -1;
        sVar9 = sVar9 + 1;
      } while ((int)sVar9 < *(int *)(unaff_EDI + 0x54));
    }
    fVar2 = *param_3;
    fVar3 = *(float *)(param_1 + 0x10);
    fVar4 = param_3[1];
    fVar5 = *(float *)(param_1 + 0x14);
    fVar6 = param_3[2];
    fVar7 = *(float *)(param_1 + 0x18);
    uVar16 = FUN_006391b4();
    if ((((1.0 < (float)(int)((uVar16 ^ (int)uVar16 >> 0x1f) - ((int)uVar16 >> 0x1f))) ||
         (uVar16 = FUN_006391b4(),
         1.0 < (float)(int)((uVar16 ^ (int)uVar16 >> 0x1f) - ((int)uVar16 >> 0x1f)))) ||
        (uVar16 = FUN_006391b4(),
        1.0 < (float)(int)((uVar16 ^ (int)uVar16 >> 0x1f) - ((int)uVar16 >> 0x1f)))) &&
       (sVar9 = 0, 0 < *(short *)(unaff_EDI + 0xc))) {
      sVar15 = *(short *)(unaff_EDI + 0xe);
      do {
        sVar10 = 0;
        if (0 < sVar15) {
          do {
            pfVar14 = (float *)(param_1 + 0x1c + ((int)sVar15 * (int)sVar9 + (int)sVar10) * 0x18);
            sVar10 = sVar10 + 1;
            *pfVar14 = (fVar2 - fVar3) + *pfVar14;
            pfVar14[1] = (fVar4 - fVar5) + pfVar14[1];
            pfVar14[2] = (fVar6 - fVar7) + pfVar14[2];
            sVar15 = *(short *)(unaff_EDI + 0xe);
          } while (sVar10 < sVar15);
        }
        sVar9 = sVar9 + 1;
      } while (sVar9 < *(short *)(unaff_EDI + 0xc));
    }
  }
  *(float *)(param_1 + 0x10) = *param_3;
  *(float *)(param_1 + 0x14) = param_3[1];
  *(float *)(param_1 + 0x18) = param_3[2];
  return;
}
#endif
