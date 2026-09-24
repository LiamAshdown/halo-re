// ai_scan_for_recent_combat_activity  (Ghidra: ai_scan_for_recent_combat_activity; named for this rewrite)
// address 0x42c3e0, size 547 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: phase-4 summary ("scans all currently recognized objects for one that indicates
// recent nearby combat activity within a difficulty-dependent time/distance window"). Every
// float comparison below is decompiled by Ghidra as CONCAT22/CONCAT31 register-half packing
// plus NAN()-flag pseudo-expressions (the same boolean-return failure mode already resolved
// in src/math/ray_intersects_sphere_test.c); the branch structure is read directly off the
// original and the comparisons themselves rewritten as clean float tests. actor+0x270's
// target_unit_index and Unit.unit_flags' "inconsequential" bit (0x80000, the 20th name in
// tags.h's UnitFlags bitfield comment) resolve what Ghidra's iVar12 (actually the ACTOR
// pointer, not the prop) and iVar10 (a Unit tag's flags field, not an int) really are.
// register convention: plain __cdecl, one stack argument. The iterator passed to
// data_iterator_next is built as the plain 3-field types/memory.h data_iterator (this
// project's established convention, e.g. src/cache/sound_cache_dispose.c), not transcribed
// from the extra scratch bytes this call site's stack frame happens to also reserve.
// blam-cc: stack -> hard_difficulty
//
// 0x401020 is vector3d_distance_squared (src/math, EAX / ECX): the prop and its pair
// (prop.pair_index) must be within 4 world units (orphan pass 4 review, 0x42c5b0..0x42c5d5).
// UNSURE: which Object-derived tag target_object_index's definition_tag really
// points at (assumed Unit, since Unit.unit_flags sits at the base Object's end offset 0x17c).
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"
#include <stdint.h>

extern game_time_globals *game_time; // 0x006f1d6c
extern data_array *prop_data;        // 0x008802c0
extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern data_array *actor_data;       // 0x00880360

extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, src/math; blam-cc: EAX a, ECX b

// blam-cc: stack -> hard_difficulty
// Walks every recognized-object prop looking for one that indicates recent nearby combat:
// its tracked object must be parented, classified as hostile (is_unit) and player-controlled;
// its owning actor's linked unit (direct, or via its swarm cluster) must not be flagged
// "inconsequential" further than 4 world units away, and -- when hard_difficulty is set and
// the actor is not already alert or in a vehicle -- must be within 15 units. Beyond that, a
// prop of an unusual kind seen within the last 90 ticks, or close (kind outside 4..5, within
// 4 units), or -- when the actor has no target -- of kind 2/3, or of kind 4/5 that is either
// already noticed or (kind 4, within 12 units) wins a random roll, counts as activity found.
int32_t ai_scan_for_recent_combat_activity(uint8_t hard_difficulty)
{
    data_iterator iterator;
    prop *p;
    int32_t current_tick;
    object *tracked_object;
    actor *a;
    datum_index linked_unit_index;
    object *linked_object;
    Unit *linked_unit_tag;
    uint8_t skip_close_check;
    int16_t kind;

    current_tick = game_time->game_time;

    iterator.data = prop_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = data_iterator_next(&iterator);

    while (p != 0) {
        if (p->is_parented && p->is_unit) {
            tracked_object = ((object_header *)object_data->data)[p->object_index & 0xffff].data;
            if (((unit_data *)((uint8_t *)tracked_object + k_unit_data_offset))->controlling_player !=
                (datum_index)k_datum_index_none) {
                a = &((actor *)actor_data->data)[p->actor_index & 0xffff];
                linked_unit_index = a->swarm ? a->cluster_unit_index : a->unit_index;

                linked_object = ((object_header *)object_data->data)[linked_unit_index & 0xffff].data;
                linked_unit_tag = (Unit *)tag_instances[linked_object->definition_tag & 0xffff].data;

                skip_close_check = 0;
                if ((linked_unit_tag->unit_flags & 0x80000) != 0) { // "inconsequential"
                    // Strictly greater: the original's NAN()-packed form is
                    // `!(d < 4.0) && !(d == 4.0)`, so a distance of exactly 4.0 does NOT set
                    // this flag. An earlier draft used `4.0f <= d`.
                    if (4.0f < p->distance) {
                        skip_close_check = 1;
                    }
                }

                if (hard_difficulty != 0 && a->unknown_5f2 == 0 && a->mode != _actor_mode_vehicle) {
                    // Strictly greater, for the same reason as the 4.0 test above:
                    // `!(d < 15.0) && !(d == 15.0)`.
                    if (15.0f < p->distance) {
                        goto next_prop;
                    }
                }

                if (!skip_close_check) {
                    kind = p->kind;
                    if ((kind < 4 || 5 < kind) && p->unknown_8c != -1 &&
                        current_tick <= p->unknown_8c + 0x5a) {
                        return 1;
                    }
                    if ((kind < 4 || 5 < kind) && p->distance < 4.0f) {
                        return 1;
                    }
                    if (a->target_unit_index == (datum_index)k_datum_index_none) {
                        if (1 < kind && kind < 4) {
                            return 1;
                        }
                        if (3 < kind && kind < 6) {
                            if (p->unknown_b8 != 0) {
                                return 1;
                            }
                            if (kind == 4 && p->distance < 12.0f) {
                                // 0x42c5b0..0x42c5d5: EAX = &p->last_known_position, ECX = the paired
                                // prop's (p->pair_index) last_known_position
                                prop *pair = &((prop *)prop_data->data)[p->pair_index & 0xffff];
                                if (vector3d_distance_squared(&p->last_known_position, &pair->last_known_position) < 16.0f) {
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
next_prop:
        p = data_iterator_next(&iterator);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x42c3e0):

undefined4 FUN_0042c3e0(char param_1)

{
  float fVar1;
  short sVar2;
  int iVar3;
  float10 fVar4;
  bool bVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  int iVar11;
  undefined2 extraout_var;
  int iVar12;
  float10 fVar13;

  iVar3 = *(int *)(DAT_006f1d6c + 0xc);
  iVar8 = data_iterator_next();
  iVar7 = DAT_0087bc14;
  iVar6 = DAT_008603b0;
  do {
    if (iVar8 == 0) {
      return 0;
    }
    if (((*(char *)(iVar8 + 0x12e) != '\0') && (*(char *)(iVar8 + 0x60) != '\0')) &&
       (*(int *)(*(int *)(*(int *)(iVar6 + 0x34) + 8 + (*(uint *)(iVar8 + 0x18) & 0xffff) * 0xc) +
                0x218) != -1)) {
      iVar12 = (*(uint *)(iVar8 + 4) & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      if (*(char *)(iVar12 + 6) == '\0') {
        uVar9 = *(uint *)(iVar12 + 0x18);
      }
      else {
        uVar9 = *(uint *)(iVar12 + 0x24);
      }
      iVar10 = *(int *)((**(uint **)(*(int *)(iVar6 + 0x34) + 8 + (uVar9 & 0xffff) * 0xc) & 0xffff)
                        * 0x20 + 0x14 + iVar7);
      bVar5 = false;
      if ((*(uint *)(iVar10 + 0x17c) & 0x80000) != 0) {
        fVar1 = *(float *)(iVar8 + 0x11c);
        iVar10 = CONCAT22((short)((uint)iVar10 >> 0x10),
                          (ushort)(fVar1 < 4.0) << 8 | (ushort)NAN(fVar1) << 10 |
                          (ushort)(fVar1 == 4.0) << 0xe);
        if (fVar1 < 4.0 == 0 && (fVar1 == 4.0) == 0) {
          bVar5 = true;
        }
      }
      iVar11 = CONCAT31((int3)((uint)iVar10 >> 8),param_1);
      if (((param_1 != '\0') && (*(short *)(iVar12 + 0x5f2) == 0)) &&
         (*(short *)(iVar12 + 0x6c) != 10)) {
        fVar1 = *(float *)(iVar8 + 0x11c);
        iVar11 = CONCAT22((short)((uint)iVar10 >> 0x10),
                          (ushort)(fVar1 < 15.0) << 8 | (ushort)NAN(fVar1) << 10 |
                          (ushort)(fVar1 == 15.0) << 0xe);
        if (fVar1 < 15.0 == 0 && (fVar1 == 15.0) == 0) goto LAB_0042c5e7;
      }
      if (!bVar5) {
        sVar2 = *(short *)(iVar8 + 0x24);
        if (((sVar2 < 4) || (5 < sVar2)) &&
           ((iVar11 = *(int *)(iVar8 + 0x8c), iVar11 != -1 &&
            (iVar11 = iVar11 + 0x5a, iVar3 <= iVar11)))) {
LAB_0042c604:
          return CONCAT31((int3)((uint)iVar11 >> 8),1);
        }
        if ((sVar2 < 4) || (5 < sVar2)) {
          fVar1 = *(float *)(iVar8 + 0x11c);
          iVar11 = CONCAT22((short)((uint)iVar11 >> 0x10),
                            (ushort)(fVar1 < 4.0) << 8 | (ushort)NAN(fVar1) << 10 |
                            (ushort)(fVar1 == 4.0) << 0xe);
          if (fVar1 < 4.0) goto LAB_0042c604;
        }
        if (*(int *)(iVar12 + 0x270) == -1) {
          if ((1 < sVar2) && (sVar2 < 4)) goto LAB_0042c604;
          if ((3 < sVar2) && (sVar2 < 6)) {
            iVar11 = CONCAT31((int3)((uint)iVar11 >> 8),*(char *)(iVar8 + 0xb8));
            if (*(char *)(iVar8 + 0xb8) != '\0') goto LAB_0042c604;
            if ((sVar2 == 4) && (*(float *)(iVar8 + 0x11c) < 12.0)) {
              fVar13 = (float10)FUN_00401020();
              fVar4 = (float10)16.0;
              iVar11 = CONCAT22(extraout_var,
                                (ushort)(fVar13 < fVar4) << 8 |
                                (ushort)(NAN(fVar13) || NAN(fVar4)) << 10 |
                                (ushort)(fVar13 == fVar4) << 0xe);
              if (fVar13 < fVar4) goto LAB_0042c604;
            }
          }
        }
      }
    }
LAB_0042c5e7:
    iVar8 = data_iterator_next();
  } while( true );
}
#endif
