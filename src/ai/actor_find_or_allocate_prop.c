// actor_find_or_allocate_prop  (Ghidra: actor_find_or_allocate_prop, renamed)
// address 0x43e270, size 973 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: types/ai.h actor.first_prop(+0x50), prop.next_in_actor(+0x08)/pair_index(+0x0c)/
// object_index(+0x18)/owner_actor_index(+0x1c)/kind(+0x24)/distance(+0x11c)/is_parented(+0x12e)/
// unknown_60. phase-4 summary "finds or allocates a per-actor firing-position node of the
// requested type, reinitializing it when newly allocated or repurposed." Calls
// actor_replace_object_reference (established elsewhere), datum_new (established elsewhere),
// actor_init_prop_from_object (this rewrite's own prop initializer) and actor_unlink_prop (this rewrite's own
// prop unlink) and actor_get_current_mode_combat_grade (outside this rewrite's range).
//
// Kept close to the Ghidra decompilation for the deep eligibility-scoring section (the many
// distance thresholds and per-target-actor checks around `iVar11+0x1cc`/`+0x3a0`/`+0x6a`/
// `+0x6e`, and the object-type-definition read at `object+0x41c`) since those reach into
// actor/unit/object-type sub-fields this module does not otherwise name at these specific
// offsets.
//
// register convention: stack -> actor_index, param_2 (unused in the body), kind.
//   // blam-cc: stack -> actor_index, param_2, kind

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern data_array *object_data; // 0x008603b0
extern data_array *encounter_data; // 0x008802c8

extern void actor_replace_object_reference(datum_index actor_index); // 0x428470, see header UNSURE on arity
extern void actor_unlink_prop(void); // 0x43ea20, this rewrite's own file; called here with no visible arguments
extern void actor_init_prop_from_object(datum_index prop_index); // 0x43e640, this rewrite's own file
extern datum_index datum_new(void); // 0x4d0480
extern int16_t actor_get_current_mode_combat_grade(datum_index actor_index); // 0x40e760, outside this rewrite's range

// blam-cc: stack -> actor_index, param_2, kind
datum_index actor_find_or_allocate_prop(datum_index actor_index, uint32_t param_2, char kind)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    datum_index cur = self->first_prop;
    datum_index best = (datum_index)0xffffffff;
    datum_index second_best = (datum_index)0xffffffff;
    float best_distance = 3.4028235e+38f;
    float second_best_distance = 3.4028235e+38f;
    int16_t match_count = 0;

    (void)param_2;

    for (;;) {
        prop *p;
        float distance2;
        actor *target = 0;
        uint8_t is_candidate;

        while (cur != (datum_index)0xffffffff) {
            p = (prop *)((uint8_t *)prop_data->data + (cur & 0xffff) * sizeof(prop));
            if (((3 < p->kind) && (p->kind < 6)) || (p->pair_index != (datum_index)0xffffffff)) {
                cur = p->next_in_actor;
                continue;
            }
            break;
        }

        if (cur == (datum_index)0xffffffff) {
            if ((best == (datum_index)0xffffffff) &&
                ((second_best == (datum_index)0xffffffff) || (match_count < (int16_t)((kind != 0) * 2 + 4)) ||
                 ((best = second_best), second_best == (datum_index)0xffffffff))) {
                best = datum_new();
            } else {
                prop *reuse = (prop *)((uint8_t *)prop_data->data + (best & 0xffff) * sizeof(prop));
                int16_t identifier = reuse->identifier;
                uint32_t *clear;
                int32_t n;

                actor_replace_object_reference(actor_index);
                actor_unlink_prop();

                clear = (uint32_t *)reuse;
                for (n = 0x4e; n != 0; n = n - 1) {
                    *clear = 0;
                    clear = clear + 1;
                }
                reuse->identifier = identifier;
            }
            actor_init_prop_from_object(best);
            return best;
        }

        distance2 = p->distance * p->distance;
        if (p->owner_actor_index != (datum_index)0xffffffff) {
            target = (actor *)((uint8_t *)actor_data->data + (p->owner_actor_index & 0xffff) * sizeof(actor));
        }
        is_candidate = 0;

        if (p->is_parented == 0) {
            /* fall through to eligibility scoring below */
            if (target == 0 || (target->active != 0 && target->keep_unit_alive == 0)) {
                if ((*((uint8_t *)p + 99) != 0) || (0 < *(int16_t *)((uint8_t *)p + 0x6a))) {
                    goto tally;
                }
                if (distance2 <= 1600.0f) {
                    if (p->is_vault == 0) {
                        if (p->is_unit == 0) {
                            if (target != 0 && target->unknown_6e < 4) {
                                if ((target->unknown_1cc != 0) || (is_candidate = 1, distance2 <= 16.0f)) {
                                    is_candidate = 0;
                                }
                            } else if (target != 0) {
                                is_candidate = 1;
                            }
                            if (225.0f <= distance2) {
                                goto score;
                            }
                        } else if (distance2 <= 36.0f) {
                            is_candidate = 0;
                        } else {
                            is_candidate = 1;
                        }
                        goto tally;
                    } else {
                        uint8_t within_reach = 1;
                        if ((target == 0) || (target->encounter_index == (datum_index)0xffffffff)) {
                        no_encounter:
                            if (0.0f < p->unknown_20) {
                                goto tally;
                            }
                            {
                                uint8_t unknown_60 = p->is_unit;
                                if (((unknown_60 == 0) || (*(int16_t *)((uint8_t *)p + 0x76) < 0x97)) &&
                                    (actor_get_current_mode_combat_grade(actor_index) < 2)) {
                                    float threshold = 16.0f;
                                    if ((unknown_60 == 0) && (target != 0) && (target->awareness_level < 3)) {
                                        threshold = 64.0f;
                                    }
                                    if (distance2 < threshold) {
                                        goto tally;
                                    }
                                }
                            }
                        } else {
                            encounter *enc = (encounter *)((uint8_t *)encounter_data->data +
                                                           (target->encounter_index & 0xffff) * sizeof(encounter));
                            int32_t limit = (enc->unknown_58 <= target->unknown_3a0) ? target->unknown_3a0 : enc->unknown_58;
                            uint8_t no_encounter_flags;

                            if (limit != -1) {
                                object *obj = *(object **)((uint8_t *)object_data->data + 8 +
                                                           (p->object_index & 0xffff) * 0xc); // UNSURE, see header
                                int32_t type_field = *(int32_t *)((uint8_t *)obj + 0x41c);
                                if ((type_field == -1) || (type_field < limit)) {
                                    within_reach = 0;
                                }
                            }
                            no_encounter_flags = (enc->unknown_45 == 0) && (enc->unknown_44 == 0) && (enc->unknown_42 == 0);

                            if (within_reach) {
                                if (!no_encounter_flags) {
                                    goto no_encounter;
                                }
                                if (distance2 < 225.0f) {
                                    goto tally;
                                }
                            }
                        }
                    }
                }
            }
        }

    score:
        if (p->distance < second_best_distance) {
            second_best = cur;
            second_best_distance = p->distance;
        }
        cur = p->next_in_actor;
        continue;

    tally:
        if ((p->is_unit == kind) && (match_count = match_count + 1, is_candidate)) {
            if (p->distance < best_distance) {
                best = cur;
                best_distance = p->distance;
            }
        }
        cur = p->next_in_actor;
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043e270 @ 0x43e270) ----
uint FUN_0043e270(uint param_1,undefined4 param_2,char param_3)

{
  char cVar1;
  undefined2 uVar2;
  uint uVar3;
  int iVar4;
  float fVar5;
  bool bVar6;
  float fVar7;
  bool bVar8;
  bool bVar9;
  short sVar10;
  int iVar11;
  int iVar12;
  short sVar13;
  int iVar14;
  int iVar15;
  undefined4 *puVar16;
  int iVar17;
  undefined4 *puVar18;
  float10 fVar19;
  float10 extraout_ST0;
  float local_1c;
  uint local_18;
  uint local_14;
  uint local_4;

  fVar19 = (float10)3.4028235e+38;
  iVar14 = (param_1 & 0xffff) * 0x724;
  local_18 = 0xffffffff;
  local_14 = 0xffffffff;
  iVar15 = *(int *)(DAT_00880360 + 0x34);
  iVar11 = iVar15 + iVar14;
  local_1c = 3.4028235e+38;
  sVar13 = 0;
  uVar3 = *(uint *)(iVar15 + 0x50 + iVar14);
  do {
    while( true ) {
      do {
        local_4 = uVar3;
        if (local_4 == 0xffffffff) {
          if ((local_18 == 0xffffffff) &&
             (((local_14 == 0xffffffff || (sVar13 < (short)((ushort)(param_3 != '\0') * 2 + 4))) ||
              (local_18 = local_14, local_14 == 0xffffffff)))) {
            local_18 = datum_new();
          }
          else {
            puVar16 = (undefined4 *)((local_18 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34));
            actor_replace_object_reference(param_1);
            FUN_0043ea20();
            uVar2 = *(undefined2 *)puVar16;
            puVar18 = puVar16;
            for (iVar15 = 0x4e; iVar15 != 0; iVar15 = iVar15 + -1) {
              *puVar18 = 0;
              puVar18 = puVar18 + 1;
            }
            *(undefined2 *)puVar16 = uVar2;
          }
          FUN_0043e640(local_18);
          return local_18;
        }
        iVar14 = *(int *)(DAT_008802c0 + 0x34);
        iVar17 = (local_4 & 0xffff) * 0x138;
        sVar10 = *(short *)(iVar17 + 0x24 + iVar14);
        uVar3 = *(uint *)(iVar17 + 8 + iVar14);
        iVar17 = iVar17 + iVar14;
      } while (((3 < sVar10) && (sVar10 < 6)) || (*(int *)(iVar17 + 0xc) != -1));
      fVar5 = *(float *)(iVar17 + 0x11c) * *(float *)(iVar17 + 0x11c);
      if (*(uint *)(iVar17 + 0x1c) == 0xffffffff) {
        iVar14 = 0;
      }
      else {
        iVar14 = (*(uint *)(iVar17 + 0x1c) & 0xffff) * 0x724 + iVar15;
      }
      bVar9 = false;
      if (*(char *)(iVar17 + 0x12e) == '\0') break;
LAB_0043e445:
      if (((*(char *)(iVar17 + 0x60) == param_3) && (sVar13 = sVar13 + 1, bVar9)) &&
         ((float10)*(float *)(iVar17 + 0x11c) < fVar19)) {
        local_14 = local_4;
        fVar19 = (float10)*(float *)(iVar17 + 0x11c);
      }
    }
    if ((iVar14 == 0) || ((*(char *)(iVar14 + 8) != '\0' && (*(char *)(iVar14 + 0x13) == '\0')))) {
      if ((*(char *)(iVar17 + 99) != '\0') || (0 < *(short *)(iVar17 + 0x6a))) goto LAB_0043e445;
      if (fVar5 <= 1600.0) {
        if (*(char *)(iVar17 + 0x127) == '\0') {
          if (*(char *)(iVar17 + 0x60) == '\0') {
            if (*(short *)(iVar11 + 0x6e) < 4) {
              if ((*(char *)(iVar11 + 0x1cc) != '\0') || (bVar9 = true, fVar5 <= 16.0)) {
                bVar9 = false;
              }
            }
            else {
              bVar9 = true;
            }
            if (225.0 <= fVar5) goto LAB_0043e573;
          }
          else if (fVar5 <= 36.0) {
            bVar9 = false;
          }
          else {
            bVar9 = true;
          }
          goto LAB_0043e445;
        }
        bVar6 = true;
        if (*(uint *)(iVar11 + 0x34) == 0xffffffff) {
LAB_0043e48d:
          if (0.0 < *(float *)(iVar17 + 0x20)) goto LAB_0043e445;
          cVar1 = *(char *)(iVar17 + 0x60);
          if (((cVar1 == '\0') || (*(short *)(iVar17 + 0x76) < 0x97)) &&
             (sVar10 = FUN_0040e760(), fVar19 = extraout_ST0, sVar10 < 2)) {
            fVar7 = 16.0;
            if ((cVar1 == '\0') && (*(short *)(iVar11 + 0x6a) < 3)) {
              fVar7 = 64.0;
            }
            if (fVar5 < fVar7) goto LAB_0043e445;
          }
        }
        else {
          iVar12 = (*(uint *)(iVar11 + 0x34) & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
          iVar14 = *(int *)(iVar12 + 0x58);
          if (*(int *)(iVar12 + 0x58) <= *(int *)(iVar11 + 0x3a0)) {
            iVar14 = *(int *)(iVar11 + 0x3a0);
          }
          if ((iVar14 != -1) &&
             ((iVar4 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                        (*(uint *)(iVar17 + 0x18) & 0xffff) * 0xc) + 0x41c),
              iVar4 == -1 || (iVar4 < iVar14)))) {
            bVar6 = false;
          }
          if (((*(char *)(iVar12 + 0x45) == '\0') && (*(char *)(iVar12 + 0x44) == '\0')) &&
             (*(char *)(iVar12 + 0x42) == '\0')) {
            bVar8 = true;
          }
          else {
            bVar8 = false;
          }
          if (bVar6) {
            if (!bVar8) goto LAB_0043e48d;
            if (fVar5 < 225.0) goto LAB_0043e445;
          }
        }
      }
    }
LAB_0043e573:
    if (*(float *)(iVar17 + 0x11c) < local_1c) {
      local_18 = local_4;
      local_1c = *(float *)(iVar17 + 0x11c);
    }
  } while( true );
}
#endif
