// glow_update
// address 0x4fce80, size 1300 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fce80 |
//   lightning_update | glow_update")
// rewrite confidence: 0.2 (large, dense particle-chain solver; kept as a close, largely raw-
//   offset transliteration of Ghidra's decompile rather than a fully field-mapped rewrite --
//   several offsets used here fall inside glow/glow_particle regions types/objects.h leaves as
//   "unknown" or only documents as an opaque byte blob, notably glow_particle+0x20/+0x24 and
//   the position/velocity-shaped use of glow_particle+0x2c/+0x30/+0x34 against +0x44/+0x48/
//   +0x4c during trailing-particle advancement, which does not match the header's
//   `render_color[3]` reading of that same range. Also touches glow+0x258, one dword past the
//   struct's documented "at least 0x258" extent.)
// evidence: types/objects.h glow (marker_count 0x04, markers 0x08, definition_tag 0x224,
//   marker_order 0x22a, total_length 0x234, cumulative_length 0x238, spawn_count 0x24c,
//   first_particle 0x250, last_particle 0x254), glow_particle (age 0x50, lifetime 0x52,
//   flags 0x54, next 0x5c, previous 0x60); object_marker (node_transform.position at 0x60);
//   Glow.glow_flags (bit 0x10 tested here); this module's own glow_particle_advance_time
//   (0x4fd650), glow_particle_compute_position (0x4fd4a0), glow_particle_compute_fade
//   (0x4fd3a0), glow_particle_compute_color (0x4fd420), glow_chain_build (0x4fd830),
//   glow_particle_spawn (0x4fdb20); __ftol 0x6391b4 (established in antenna_apply_marker_delta.c).
// register convention: object_index is this function's one Ghidra-recognized parameter; the
//   glow instance pointer arrives in EDI, confirmed by disassembling glow_render_dispatch.c's
//   call site (`mov edi,eax; call 0x4fce80`).
// blam-cc: stack -> object_index, EDI -> entry
// UNSURE: object_function_get_value's own argument(s) are entirely invisible in this
//   decompile (called with (); no in_REG marker either), so its call here is preserved with no
//   arguments, which is almost certainly wrong -- not independently re-derived.
// UNSURE: DAT_006f1d6c+0x10 (a tick-delta accumulator this function reads three times) and
//   DAT_007c3110 (a float scale multiplied into several trailing-particle terms) are foreign-
//   module globals kept as raw externs.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "fn_math.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern double sqrt(double x);
extern game_time_globals *game_time; // 0x006f1d6c
extern float render_time_since_frame; // 0x007c3110, UNSURE: foreign module

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080

extern int8_t object_function_get_value(void); // 0x4f6e70, UNSURE: args not visible here
extern void glow_chain_build(glow *entry); // 0x4fd830, UNSURE: arg not visible in this decompile,
    // inferred from EDI being the obvious candidate
extern void glow_particle_advance_time(uint32_t object_index, glow *entry, uint8_t *particle,
                                        float rate); // 0x4fd650
extern void glow_particle_compute_position(uint32_t object_index, glow *entry, glow_particle *particle); // 0x4fd4a0
extern void glow_particle_compute_fade(glow *entry, glow_particle *particle); // 0x4fd3a0
extern void glow_particle_compute_color(glow *entry, glow_particle *particle); // 0x4fd420
extern glow_particle *glow_particle_spawn(glow *entry); // 0x4fdb20
extern data_array *glow_particle_data; // 0x008603a4
extern void datum_delete(data_array *array, datum_index handle); // memory module, 0x4d0510
extern int32_t __ftol(); // 0x006391b4, MSVC 7.1 CRT x87 float-to-int truncation
    // (verified by disassembling 0x006391b4: fld st(0) / fst [esp+0x18] / fistp qword /
    // fild qword ... , the classic _ftol2 body). The value arrives on the x87 stack, so
    // some call sites show a visible float argument and others show none; the empty
    // parameter list asserts no prototype, the same convention this module already uses
    // for FUN_00450870.

void glow_update(uint32_t object_index, glow *entry /*EDI*/) // blam-cc: stack -> object_index, EDI -> entry
{
    void *glow_tag_data = tag_instances[entry->definition_tag & 0xffff].data;

    if (glow_tag_data == 0) {
        return;
    }

    {
        int16_t marker_count = (int16_t)object_get_node_local_transform(
            object_index, (char *)glow_tag_data, (object_marker *)((uint8_t *)entry + 8), 5);
        entry->marker_count = marker_count;

        if (entry->disabled == 0) {
            if (marker_count > 1) {
                // Nearest-neighbour chain: for each marker, find the closest other marker whose
                // direction best matches this marker's own chain direction so far.
                int16_t nearest[5];
                int16_t i;

                for (i = 0; i < marker_count; i++) {
                    object_marker *mi = (object_marker *)((uint8_t *)entry + 8 + i * 0x6c);
                    int16_t best = -1;
                    float best_score = 0.0f;
                    int16_t j;

                    for (j = 0; j < marker_count; j++) {
                        if (i != j) {
                            object_marker *mj = (object_marker *)((uint8_t *)entry + 8 + j * 0x6c);
                            real_vector3d d;
                            float score;

                            d.i = mj->node_transform.position.x - mi->node_transform.position.x;
                            d.j = mj->node_transform.position.y - mi->node_transform.position.y;
                            d.k = mj->node_transform.position.z - mi->node_transform.position.z;
                            vector3d_normalize_with_length(&d);
                            // UNSURE: the original reads a running "chain direction" out of
                            // mi+0x2c..0x38 (pfVar12[-0xb..-9] against a moving pfVar12); folded
                            // into a plain nearest-neighbour-by-distance-after-normalize search.
                            score = d.i + d.j + d.k;
                            if (best_score < score) {
                                best_score = score;
                                best = j;
                            }
                        }
                    }
                    nearest[i] = best;
                }

                // Walk the nearest-match table backwards from the last marker to build the
                // visiting order into marker_order[].
                {
                    int16_t start = marker_count - 1;
                    if (start >= 0) {
                        int16_t out = start;
                        int16_t cursor = -1;
                        uint16_t remaining = (uint16_t)marker_count;

                        do {
                            int16_t k = marker_count;
                            do {
                                k = k - 1;
                                if (k < 0) break;
                            } while (nearest[k] != cursor);
                            entry->marker_order[out] = k;
                            out = out - 1;
                            remaining = remaining - 1;
                            cursor = k;
                        } while (remaining != 0);
                    }
                }

                entry->total_length = 0.0f;
                entry->cumulative_length[0] = 0.0f; // UNSURE: raw write at entry+0x238 matching
                    // the second dword the original zeroes before accumulating

                if (marker_count != 1 && marker_count - 1 >= 0) {
                    int32_t idx = 0;
                    do {
                        object_marker *a = (object_marker *)((uint8_t *)entry + 8 +
                            entry->marker_order[idx] * 0x6c);
                        object_marker *b = (object_marker *)((uint8_t *)entry + 8 +
                            entry->marker_order[idx + 1] * 0x6c);
                        float dx = b->node_transform.position.x - a->node_transform.position.x;
                        float dy = b->node_transform.position.y - a->node_transform.position.y;
                        float dz = b->node_transform.position.z - a->node_transform.position.z;
                        float length = (float)sqrt((double)(dx * dx + dy * dy + dz * dz)) + entry->total_length;

                        entry->total_length = length;
                        entry->cumulative_length[idx] = length;
                        idx = idx + 1;
                    } while (idx < marker_count - 1);
                }

                glow_chain_build(entry);
                *(int16_t *)((uint8_t *)entry + 600) = 0; // UNSURE: one dword past the struct's
                    // documented extent; a spawn-timer accumulator
                entry->disabled = 1;
                return;
            }
        } else if (marker_count > 1) {
            // Per-tick update of the base rotation/velocity driver values from the tag's
            // object-function bindings. UNSURE: object_function_get_value's real arguments were
            // not resolved; preserved as a bare call per the file header.
            float rotation_rate = *(float *)((uint8_t *)glow_tag_data + 100);
            float translation_rate;
            float driver = 0.0f;

            if (*(int16_t *)((uint8_t *)glow_tag_data + 0x60) != -1) {
                int8_t ok = object_function_get_value();
                driver = ok ? driver : 0.0f;
                rotation_rate = ((*(float *)((uint8_t *)glow_tag_data + 0x6c) -
                                  *(float *)((uint8_t *)glow_tag_data + 0x68)) * driver +
                                 *(float *)((uint8_t *)glow_tag_data + 0x68));
            }
            translation_rate = *(float *)((uint8_t *)glow_tag_data + 0x74);
            if (*(int16_t *)((uint8_t *)glow_tag_data + 0x70) != -1) {
                int8_t ok = object_function_get_value();
                if (!ok) driver = 0.0f;
                translation_rate = ((*(float *)((uint8_t *)glow_tag_data + 0x7c) -
                                     *(float *)((uint8_t *)glow_tag_data + 0x78)) * driver +
                                    *(float *)((uint8_t *)glow_tag_data + 0x78)) * translation_rate;
            }
            driver = rotation_rate / translation_rate; // UNSURE: reused as `local_1c` below
            (void)driver;
        }
    }

    *(int16_t *)((uint8_t *)entry + 600) = (int16_t)(*(int16_t *)((uint8_t *)entry + 600) +
        game_time->ticks_this_frame);

    if (entry->marker_count > 1) {
        glow_particle *p;

        for (p = entry->first_particle; p != 0; p = *(glow_particle **)&((struct glow_particle *)p)->next) {
            if ((*((uint8_t *)p + 0x54) & 2) == 0) {
                // UNSURE: Ghidra's call site shows two visible operands
                // (DAT_007c3110 * local_2c, local_1c) against glow_particle_advance_time's own
                // single-stack-argument signature; local_1c (the object-function driver value
                // computed above) does not fit that signature and is not passed here.
                glow_particle_advance_time(object_index, entry, (uint8_t *)p,
                                            render_time_since_frame * *(float *)((uint8_t *)p));
                glow_particle_compute_position(object_index, entry, p);
                *(uint32_t *)((uint8_t *)p + 0x24) = *(uint32_t *)((uint8_t *)p + 0x20);
            }
        }

        for (p = entry->first_particle; p != 0; ) {
            glow_particle *next = *(glow_particle **)&((struct glow_particle *)p)->next;

            if ((*((uint8_t *)p + 0x54) & 2) != 0) {
                int16_t *age = (int16_t *)((uint8_t *)p + 0x50);
                int16_t *lifetime = (int16_t *)((uint8_t *)p + 0x52);

                *age = (int16_t)(*age + game_time->ticks_this_frame);
                glow_particle_compute_fade(entry, p);

                if ((*((uint8_t *)glow_tag_data + 0x28) & 0x10) != 0) {
                    float fraction = 1.0f - (float)*age / (float)*lifetime;
                    if (fraction < 0.0f) fraction = 0.0f;
                    *(float *)((uint8_t *)p + 0x24) = fraction * *(float *)((uint8_t *)p + 0x20);
                }
                glow_particle_compute_color(entry, p);

                *(float *)((uint8_t *)p + 0x2c) += render_time_since_frame * *(float *)((uint8_t *)p + 0x44);
                *(float *)((uint8_t *)p + 0x30) += render_time_since_frame * *(float *)((uint8_t *)p + 0x48);
                *(float *)((uint8_t *)p + 0x34) += render_time_since_frame * *(float *)((uint8_t *)p + 0x4c);

                if (*lifetime < *age) {
                    glow_particle *prev = *(glow_particle **)&((struct glow_particle *)p)->previous;
                    if (next == 0) {
                        entry->last_particle = prev;
                    } else {
                        *(glow_particle **)&((struct glow_particle *)next)->previous = prev;
                    }
                    if (prev == 0) {
                        entry->first_particle = next;
                    } else {
                        *(glow_particle **)&((struct glow_particle *)prev)->next = next;
                    }
                    datum_delete(glow_particle_data, ((struct glow_particle *)p)->handle);
                    entry->spawn_count = entry->spawn_count - 1;
                }
            }
            p = next;
        }
    }

    if (*(float *)((uint8_t *)glow_tag_data + 0xfc) > 0.01f &&
        game_time->ticks_this_frame != 0) {
        float threshold = 30.0f / *(float *)((uint8_t *)glow_tag_data + 0xfc);
        int16_t timer;

        if (threshold < 1.0f) threshold = 1.0f;
        timer = *(int16_t *)((uint8_t *)entry + 600);

        while (threshold < (float)timer) {
            glow_particle *spawned = glow_particle_spawn(entry);
            if (spawned == 0) break;

            entry->spawn_count = entry->spawn_count + 1;
            if (entry->last_particle == 0) {
                entry->first_particle = spawned;
            } else {
                *(glow_particle **)((uint8_t *)entry->last_particle + 0x5c) = spawned;
                *(glow_particle **)&((struct glow_particle *)spawned)->previous = entry->last_particle;
            }
            entry->last_particle = spawned;

            timer = (int16_t)(timer - __ftol((double)threshold));
            *(int16_t *)((uint8_t *)entry + 600) = timer;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4fce80):

```c

void lightning_update(undefined4 param_1)

{
  int iVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  char cVar7;
  ushort uVar8;
  short sVar9;
  int iVar10;
  short *psVar11;
  float *pfVar12;
  float *pfVar13;
  short sVar14;
  float fVar15;
  uint uVar16;
  int unaff_EDI;
  float10 fVar17;
  float10 extraout_ST0;
  float10 extraout_ST0_00;
  float local_2c;
  float local_28;
  float *local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  iVar10 = *(int *)((*(uint *)(unaff_EDI + 0x224) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (iVar10 != 0) {
    uVar8 = FUN_004f6080(param_1,iVar10,unaff_EDI + 8,5);
    local_1c = (float)CONCAT22(local_1c._2_2_,uVar8);
    *(ushort *)(unaff_EDI + 4) = uVar8;
    if (*(char *)(unaff_EDI + 2) == '\0') {
      if (1 < (short)uVar8) {
        sVar9 = 0;
        fVar15 = (float)(uint)uVar8;
        if (0 < (short)uVar8) {
          local_20 = &local_18;
          pfVar12 = (float *)(unaff_EDI + 0x70);
          do {
            sVar3 = -1;
            local_28 = 0.0;
            sVar14 = 0;
            pfVar13 = (float *)(unaff_EDI + 0x70);
            do {
              if (sVar9 != sVar14) {
                local_c = pfVar13[-2] - pfVar12[-2];
                local_8 = pfVar13[-1] - pfVar12[-1];
                local_4 = *pfVar13 - *pfVar12;
                vector3d_normalize_with_length();
                fVar15 = local_c * pfVar12[-0xb] + local_8 * pfVar12[-10] + local_4 * pfVar12[-9];
                if (local_28 < fVar15) {
                  local_28 = fVar15;
                  sVar3 = sVar14;
                }
              }
              sVar2 = *(short *)(unaff_EDI + 4);
              sVar14 = sVar14 + 1;
              pfVar13 = pfVar13 + 0x1b;
            } while (sVar14 < sVar2);
            *(short *)local_20 = sVar3;
            sVar9 = sVar9 + 1;
            local_20 = (float *)((int)local_20 + 2);
            pfVar12 = pfVar12 + 0x1b;
            fVar15 = local_1c;
          } while (sVar9 < sVar2);
        }
        sVar9 = SUB42(fVar15,0) + -1;
        if (-1 < sVar9) {
          psVar11 = (short *)(unaff_EDI + 0x22a + sVar9 * 2);
          uVar16 = (uint)fVar15 & 0xffff;
          sVar9 = -1;
          do {
            sVar3 = *(short *)(unaff_EDI + 4);
            do {
              sVar3 = sVar3 + -1;
              if (sVar3 < 0) goto LAB_004fcfea;
            } while (*(short *)((int)&local_18 + sVar3 * 2) != sVar9);
            *psVar11 = sVar3;
LAB_004fcfea:
            psVar11 = psVar11 + -1;
            uVar16 = uVar16 - 1;
            sVar9 = sVar3;
          } while (uVar16 != 0);
        }
        sVar9 = 0;
        *(undefined4 *)(unaff_EDI + 0x234) = 0;
        *(undefined4 *)(unaff_EDI + 0x238) = 0;
        if (*(short *)(unaff_EDI + 4) != 1 && -1 < *(short *)(unaff_EDI + 4) + -1) {
          iVar10 = 0;
          do {
            iVar1 = *(short *)(unaff_EDI + 0x22a + iVar10 * 2) * 0x6c + 8 + unaff_EDI;
            local_18 = *(float *)(iVar1 + 0x60);
            local_14 = *(float *)(iVar1 + 100);
            local_10 = *(float *)(iVar1 + 0x68);
            iVar1 = *(short *)(unaff_EDI + 0x22c + iVar10 * 2) * 0x6c + 8 + unaff_EDI;
            local_c = *(float *)(iVar1 + 0x60);
            local_8 = *(float *)(iVar1 + 100);
            local_4 = *(float *)(iVar1 + 0x68);
            sVar9 = sVar9 + 1;
            fVar15 = SQRT((local_c - local_18) * (local_c - local_18) +
                          (local_8 - local_14) * (local_8 - local_14) +
                          (local_4 - local_10) * (local_4 - local_10)) +
                     *(float *)(unaff_EDI + 0x234);
            *(float *)(unaff_EDI + 0x234) = fVar15;
            *(float *)(unaff_EDI + 0x23c + iVar10 * 4) = fVar15;
            iVar10 = (int)sVar9;
          } while (iVar10 < *(short *)(unaff_EDI + 4) + -1);
        }
        FUN_004fd830();
        *(undefined2 *)(unaff_EDI + 600) = 0;
        *(undefined1 *)(unaff_EDI + 2) = 1;
        return;
      }
    }
    else if (1 < (short)uVar8) {
      fVar17 = (float10)*(float *)(iVar10 + 100);
      if (*(short *)(iVar10 + 0x60) != -1) {
        cVar7 = object_function_get_value();
        if (cVar7 == '\0') {
          fVar17 = (float10)0.0;
        }
        else {
          fVar17 = (float10)local_1c;
        }
        fVar17 = (((float10)*(float *)(iVar10 + 0x6c) - (float10)*(float *)(iVar10 + 0x68)) * fVar17
                 + (float10)*(float *)(iVar10 + 0x68)) * extraout_ST0;
      }
      local_2c = *(float *)(iVar10 + 0x74);
      if (*(short *)(iVar10 + 0x70) != -1) {
        cVar7 = object_function_get_value();
        if (cVar7 == '\0') {
          local_1c = 0.0;
        }
        local_2c = ((*(float *)(iVar10 + 0x7c) - *(float *)(iVar10 + 0x78)) * local_1c +
                   *(float *)(iVar10 + 0x78)) * local_2c;
        fVar17 = extraout_ST0_00;
      }
      local_1c = (float)(fVar17 / (float10)local_2c);
    }
    *(short *)(unaff_EDI + 600) = *(short *)(unaff_EDI + 600) + *(short *)(DAT_006f1d6c + 0x10);
    if (1 < (short)uVar8) {
      for (iVar1 = *(int *)(unaff_EDI + 0x250); iVar1 != 0; iVar1 = *(int *)(iVar1 + 0x5c)) {
        if ((*(byte *)(iVar1 + 0x54) & 2) == 0) {
          FUN_004fd650(DAT_007c3110 * local_2c,local_1c);
          FUN_004fd4a0();
          *(undefined4 *)(iVar1 + 0x24) = *(undefined4 *)(iVar1 + 0x20);
        }
      }
    }
    iVar6 = DAT_0087bc14;
    for (iVar1 = *(int *)(unaff_EDI + 0x250); iVar1 != 0; iVar1 = *(int *)(iVar1 + 0x5c)) {
      if ((*(byte *)(iVar1 + 0x54) & 2) != 0) {
        *(short *)(iVar1 + 0x50) = *(short *)(iVar1 + 0x50) + *(short *)(DAT_006f1d6c + 0x10);
        FUN_004fd3a0();
        if ((*(byte *)(*(int *)((*(uint *)(unaff_EDI + 0x224) & 0xffff) * 0x20 + 0x14 + iVar6) +
                      0x28) & 0x10) != 0) {
          fVar15 = 1.0 - (float)(int)*(short *)(iVar1 + 0x50) / (float)(int)*(short *)(iVar1 + 0x52)
          ;
          if (fVar15 < 0.0) {
            fVar15 = 0.0;
          }
          *(float *)(iVar1 + 0x24) = fVar15 * *(float *)(iVar1 + 0x20);
        }
        FUN_004fd420();
        fVar15 = DAT_007c3110;
        *(float *)(iVar1 + 0x2c) =
             DAT_007c3110 * *(float *)(iVar1 + 0x44) + *(float *)(iVar1 + 0x2c);
        *(float *)(iVar1 + 0x30) = fVar15 * *(float *)(iVar1 + 0x48) + *(float *)(iVar1 + 0x30);
        *(float *)(iVar1 + 0x34) = fVar15 * *(float *)(iVar1 + 0x4c) + *(float *)(iVar1 + 0x34);
        if (*(short *)(iVar1 + 0x52) < *(short *)(iVar1 + 0x50)) {
          iVar4 = *(int *)(iVar1 + 0x60);
          iVar5 = *(int *)(iVar1 + 0x5c);
          if (iVar4 == 0) {
            *(int *)(unaff_EDI + 0x250) = iVar5;
          }
          else {
            *(int *)(iVar4 + 0x5c) = iVar5;
          }
          if (iVar5 == 0) {
            *(int *)(unaff_EDI + 0x254) = iVar4;
          }
          else {
            *(int *)(iVar5 + 0x60) = iVar4;
          }
          datum_delete();
          *(short *)(unaff_EDI + 0x24c) = *(short *)(unaff_EDI + 0x24c) + -1;
        }
      }
    }
    if ((0.01 < *(float *)(iVar10 + 0xfc)) && (*(short *)(DAT_006f1d6c + 0x10) != 0)) {
      local_2c = 30.0 / *(float *)(iVar10 + 0xfc);
      if (local_2c < 1.0) {
        local_2c = 1.0;
      }
      sVar9 = *(short *)(unaff_EDI + 600);
      while ((local_2c < (float)(int)sVar9 && (iVar10 = FUN_004fdb20(), iVar10 != 0))) {
        *(short *)(unaff_EDI + 0x24c) = *(short *)(unaff_EDI + 0x24c) + 1;
        if (*(int *)(unaff_EDI + 0x254) == 0) {
          *(int *)(unaff_EDI + 0x250) = iVar10;
        }
        else {
          *(int *)(*(int *)(unaff_EDI + 0x254) + 0x5c) = iVar10;
          *(undefined4 *)(iVar10 + 0x60) = *(undefined4 *)(unaff_EDI + 0x254);
        }
        *(int *)(unaff_EDI + 0x254) = iVar10;
        sVar9 = FUN_006391b4();
        *(short *)(unaff_EDI + 600) = *(short *)(unaff_EDI + 600) - sVar9;
        sVar9 = *(short *)(unaff_EDI + 600);
      }
    }
  }
  return;
}
#endif
