// effect_check_object_collisions  (Ghidra: FUN_00450fa0; phase 2 misnamed it
// "particle_system_check_object_collisions" -- RENAMED here for the same reason
// out/phase4/effects_types_notes.md's misattribution table renames the seven sibling
// "particle_system_*" functions: the table this walks is effect_data at 0x0087abdc with
// element stride 0xfc (matches types/effects.h effect, not the 0x158-byte particle_system),
// and the tag field it reads (Effect.maximum_damage_radius) belongs to the effe tag, not pctl)
// address 0x450fa0, size 738 bytes
// name confidence: 0.4 (Ghidra unnamed; renamed per the effect_data stride/tag evidence above)
//   rewrite confidence: 0.55
// evidence: types/effects.h effect (flags bit 3 = _effect_finished_bit, definition_index at
//   0x04, location_markers[0] at 0x5c, object_index at 0x3c, first_person_weapon_index at
//   0x4c) and effect_location_marker (marker_index at 0x02, next_marker at 0x04, transform at
//   0x08); types/tags.h Effect.maximum_damage_radius -- out/phase4/effects_types_notes.md
//   item 11 explains the byte offset directly: Ghidra's `local_20` is a `short *`, so
//   `*(float *)(local_20 + 4)` is byte offset 8, i.e. maximum_damage_radius, not the
//   loop_start_event/loop_stop_event pair at byte offset 2/4; types/objects.h object
//   (bounding_center at 0xa0, bounding_radius at 0xac, nodes at 0x1f0) and object_header
//   (size 0x0c, data at +0x08); types/game.h player.unit (datum_index at 0x34); this module's
//   own effect_resolve_marker_transform 0x453220 and effect_marker_next 0x453180, whose bodies
//   this function duplicates inline (the disassembly calls FUN_00453180 only from the
//   first-person-skip branch, exactly matching effect_marker_next's own recursive-skip shape,
//   and never calls matrix4x3_transform_point at all -- both are compiled-in duplicates of
//   those two functions' logic, not calls to them); src/math/matrix4x3_transform_point.c is
//   the exact formula the non-first-person marker branch repeats by hand.
// register convention: none -- no registers or stack parameters are read; the function takes
//   nothing and returns a bool-in-EAX.
// UNSURE: unknown_10/effect.unknown fields are not touched here, kept exactly as read; the
//   original also computes `psVar6` values purely to carry FPU comparison flag bits into unused
//   registers (dead code from converting an fcom/fnstsw sequence) -- omitted here since they are
//   never consumed, only the boolean the comparisons feed (radius != 0.0, and the final overlap
//   test) is preserved.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "game.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *effect_data;              // 0x0087abdc
extern data_array *effect_location_data;     // 0x0087abe0
extern data_array *object_data;              // 0x008603b0
extern data_array *player_data;              // 0x0087a480
extern tag_instance *tag_instances;          // 0x0087bc14
extern uint8_t *first_person_weapon_interfaces; // 0x006b2d98, stride 0x1ea0

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630
extern void *data_iterator_next(data_iterator *iterator);              // 0x4d05d0
extern effect_location_marker *effect_marker_next(effect *self, datum_index *marker,
    int32_t mode); // 0x453180, this module

// Scans every effect that has not finished playing and whose Effect tag carries a nonzero
// maximum_damage_radius, testing each of them against every player's controlled unit: for the
// first (non first-person) location marker of the effect that resolves to a world point, if the
// unit's bounding sphere is within (unit.bounding_radius + effect.maximum_damage_radius) of that
// point the function returns true immediately. Returns false once every effect/player/marker
// combination has been checked with no overlap.
uint32_t effect_check_object_collisions(void)
{
    datum_index effect_handle;

    for (effect_handle = datum_next((int16_t)k_datum_index_none, effect_data);
         effect_handle != k_datum_index_none;
         effect_handle = datum_next((int16_t)effect_handle, effect_data)) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)effect_handle];

        if ((self->flags & _effect_finished_bit) != 0) {
            continue;
        }

        Effect *definition = (Effect *)tag_instances[(uint16_t)self->definition_index].data;
        float damage_radius = definition->maximum_damage_radius;
        if (damage_radius == 0.0f) {
            continue;
        }

        {
            data_iterator player_iterator;
            player *p;

            player_iterator.data = player_data;
            player_iterator.next_index = 0;
            player_iterator.index = k_datum_index_none;
            player_iterator.signature = (uint32_t)(uintptr_t)player_iterator.data ^ k_data_iterator_signature;

            for (p = (player *)data_iterator_next(&player_iterator); p != (player *)0;
                 p = (player *)data_iterator_next(&player_iterator)) {
                if (p->unit == k_datum_index_none) {
                    continue;
                }

                object *unit = ((object_header *)object_data->data)[(uint16_t)p->unit].data;
                datum_index marker_handle = self->location_markers[0];

                while (marker_handle != k_datum_index_none) {
                    effect_location_marker *entry =
                        &((effect_location_marker *)effect_location_data->data)[(uint16_t)marker_handle];
                    marker_handle = entry->next_marker;

                    if (entry->marker_index != 0xffff && (entry->marker_index & 0x8000) != 0) {
                        // this entry is a first-person-weapon marker: skip it exactly as
                        // effect_marker_next(self, &marker_handle, 0) would.
                        entry = effect_marker_next(self, &marker_handle, 0);
                    }
                    if (entry == (effect_location_marker *)0) {
                        break;
                    }

                    real_point3d point;
                    if (entry->marker_index == 0xffff) {
                        point = entry->transform.position;
                    } else {
                        real_matrix4x3 *node;
                        uint16_t node_index = entry->marker_index & 0x7fff;

                        if ((entry->marker_index & 0x8000) != 0) {
                            node = (real_matrix4x3 *)(first_person_weapon_interfaces + 0x108c +
                                self->first_person_weapon_index * 0x1ea0 + node_index * 0x34);
                        } else {
                            object *owner =
                                ((object_header *)object_data->data)[(uint16_t)self->object_index].data;
                            node = (real_matrix4x3 *)((uint8_t *)owner + owner->nodes.offset +
                                node_index * 0x34);
                        }

                        // duplicated matrix4x3_transform_point(&point, &entry->transform.position, node)
                        float x = entry->transform.position.x;
                        float y = entry->transform.position.y;
                        float z = entry->transform.position.z;
                        if (node->scale != 1.0f) {
                            x = x * node->scale;
                            y = y * node->scale;
                            z = z * node->scale;
                        }
                        point.x = x * node->forward.i + y * node->left.i + z * node->up.i +
                            node->position.x;
                        point.y = x * node->forward.j + y * node->left.j + z * node->up.j +
                            node->position.y;
                        point.z = x * node->forward.k + y * node->left.k + z * node->up.k +
                            node->position.z;
                    }

                    float combined_radius = unit->bounding_radius + damage_radius;
                    float dx = point.x - unit->bounding_center.x;
                    float dy = point.y - unit->bounding_center.y;
                    float dz = point.z - unit->bounding_center.z;
                    float distance_squared = dx * dx + dy * dy + dz * dz;

                    if (combined_radius * combined_radius >= distance_squared) {
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x450fa0):

uint FUN_00450fa0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  ushort uVar4;
  float fVar5;
  short *psVar6;
  int iVar7;
  int iVar8;
  float fVar9;
  short sVar10;
  float *pfVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  short *psVar15;
  uint local_28;
  short *local_24;
  short *local_20;
  float local_1c;
  float local_18;
  float local_14;
  uint local_10;
  undefined2 local_c;
  undefined4 local_8;
  uint local_4;

  iVar13 = DAT_0087abdc;
  psVar6 = (short *)datum_next();
  local_24 = psVar6;
  iVar12 = DAT_008603b0;
  do {
    do {
      if (local_24 == (short *)0xffffffff) {
        return (uint)psVar6 & 0xffffff00;
      }
      iVar14 = ((uint)local_24 & 0xffff) * 0xfc + *(int *)(iVar13 + 0x34);
      local_20 = *(short **)((*(uint *)(iVar14 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      psVar6 = local_20;
      if ((*(byte *)(iVar14 + 2) & 8) == 0) {
        fVar9 = *(float *)(local_20 + 4);
        psVar6 = (short *)CONCAT22((short)((uint)local_20 >> 0x10),
                                   (ushort)(fVar9 < 0.0) << 8 | (ushort)NAN(fVar9) << 10 |
                                   (ushort)(fVar9 == 0.0) << 0xe);
        if (fVar9 != 0.0) {
          local_4 = DAT_0087a480 ^ 0x69746572;
          local_c = 0;
          local_8 = 0xffffffff;
          local_10 = DAT_0087a480;
          iVar7 = data_iterator_next();
          while (psVar6 = (short *)0x0, iVar7 != 0) {
            if (*(uint *)(iVar7 + 0x34) != 0xffffffff) {
              local_28 = *(uint *)(iVar14 + 0x5c);
              iVar7 = *(int *)(*(int *)(iVar12 + 0x34) + 8 +
                              (*(uint *)(iVar7 + 0x34) & 0xffff) * 0xc);
              while (iVar13 = DAT_0087abdc, local_28 != 0xffffffff) {
                iVar8 = (local_28 & 0xffff) * 0x3c + *(int *)(DAT_0087abe0 + 0x34);
                local_28 = *(uint *)(iVar8 + 4);
                if ((*(short *)(iVar8 + 2) != -1) && (*(short *)(iVar8 + 2) < 0)) {
                  iVar8 = FUN_00453180(iVar14,&local_28,0);
                }
                iVar13 = DAT_0087abdc;
                if (iVar8 == 0) break;
                uVar4 = *(ushort *)(iVar8 + 2);
                if (uVar4 == 0xffff) {
                  local_1c = *(float *)(iVar8 + 0x30);
                  local_18 = *(float *)(iVar8 + 0x34);
                  fVar9 = *(float *)(iVar8 + 0x38);
                  local_14 = fVar9;
                }
                else {
                  if ((short)uVar4 < 0) {
                    pfVar11 = (float *)((short)(uVar4 & 0x7fff) * 0x34 + 0x108c +
                                       *(short *)(iVar14 + 0x4c) * 0x1ea0 + DAT_006b2d98);
                  }
                  else {
                    iVar12 = *(int *)(*(int *)(iVar12 + 0x34) + 8 +
                                     (*(uint *)(iVar14 + 0x3c) & 0xffff) * 0xc);
                    pfVar11 = (float *)((int)*(short *)(iVar12 + 0x1f2) +
                                       (short)(uVar4 & 0x7fff) * 0x34 + iVar12);
                  }
                  fVar1 = *(float *)(iVar8 + 0x30);
                  fVar2 = *(float *)(iVar8 + 0x34);
                  fVar3 = *(float *)(iVar8 + 0x38);
                  fVar9 = *pfVar11;
                  if (fVar9 != 1.0) {
                    fVar1 = fVar1 * *pfVar11;
                    fVar2 = fVar2 * *pfVar11;
                    fVar3 = fVar3 * *pfVar11;
                  }
                  local_1c = fVar1 * pfVar11[1] + fVar2 * pfVar11[4] + fVar3 * pfVar11[7] +
                             pfVar11[10];
                  local_18 = fVar1 * pfVar11[2] + fVar2 * pfVar11[5] + fVar3 * pfVar11[8] +
                             pfVar11[0xb];
                  iVar12 = DAT_008603b0;
                  local_14 = fVar1 * pfVar11[3] + fVar2 * pfVar11[6] + fVar3 * pfVar11[9] +
                             pfVar11[0xc];
                }
                fVar1 = *(float *)(iVar7 + 0xac) + *(float *)(local_20 + 4);
                fVar5 = local_1c - *(float *)(iVar7 + 0xa0);
                fVar3 = local_18 - *(float *)(iVar7 + 0xa4);
                fVar2 = local_14 - *(float *)(iVar7 + 0xa8);
                fVar2 = fVar5 * fVar5 + fVar3 * fVar3 + fVar2 * fVar2;
                fVar1 = fVar1 * fVar1;
                if (fVar1 < fVar2 == 0) {
                  return CONCAT31((int3)(CONCAT22((short)((uint)fVar9 >> 0x10),
                                                  (ushort)(fVar1 < fVar2) << 8 |
                                                  (ushort)(NAN(fVar1) || NAN(fVar2)) << 10 |
                                                  (ushort)(fVar1 == fVar2) << 0xe) >> 8),1);
                }
              }
            }
            iVar7 = data_iterator_next();
          }
        }
      }
      psVar15 = (short *)0xffffffff;
      iVar14 = (int)local_24 + 1;
      sVar10 = (short)iVar14;
      local_24 = psVar15;
    } while ((sVar10 < 0) || (*(short *)(iVar13 + 0x2e) <= sVar10));
    psVar6 = (short *)((int)sVar10 * (int)*(short *)(iVar13 + 0x22) + *(int *)(iVar13 + 0x34));
    do {
      if (*psVar6 != 0) {
        local_24 = (short *)((int)*psVar6 << 0x10 | (int)(short)iVar14);
        break;
      }
      iVar14 = iVar14 + 1;
      psVar6 = (short *)((int)psVar6 + (int)*(short *)(iVar13 + 0x22));
    } while ((short)iVar14 < *(short *)(iVar13 + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
