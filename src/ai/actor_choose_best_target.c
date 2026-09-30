// actor_choose_best_target  (Ghidra: actor_choose_best_target, already named)
// address 0x4203a0, size 1266 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: walks the actor's prop list (actor.first_prop, chained through prop.next_in_actor),
// rebuilds actor.tally from scratch, keeps the prop with the highest prop.desirability, and
// commits it to actor.target_unit_index -- re-rating both the old and the new target as it
// swaps -- before refreshing the combat status and the awareness level.
// register convention: actor_index is a genuine stack parameter.
// blam-cc: stack -> actor_index
// VERIFIED against disassembly 0x4203a0..0x42088d (2026-09-30): both callees take the actor index in EAX (the second is a tail
// jump); the second actor_rate_potential_target call re-reads actor.target_unit_index exactly as the original does.
// UNSURE: prop.unknown_12d, unknown_12f, unknown_135, unknown_122, unknown_9c, unknown_32 and
// unknown_38 keep their placeholder names; this function is a counter of them, not an
// explanation of them.
// UNSURE: types/ai.h previously called actor+0x23c eye_position. The 0x7b-byte zeroing run at
// the top of this function covers 0x1ec..0x266, so that name cannot be right; the region is
// now actor.tally and the old eye_position field is gone.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *prop_data;   // 0x008802c0
extern data_array *object_data; // 0x008603b0

extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index); // 0x41fd50, this module
extern void actor_update_target_combat_status(datum_index actor_index); // 0x4200d0, this module, EAX -> actor_index
extern void actor_update_awareness_level(datum_index actor_index);      // 0x420290, this module, EAX -> actor_index

// blam-cc: stack -> actor_index
void actor_choose_best_target(datum_index actor_index)
{
    actor *self;
    actor *target_actor;
    prop *p;
    object *tracked_object;
    datum_index prop_index;
    datum_index best;
    datum_index previous;
    datum_index owning_actor_index;
    float best_desirability;
    uint32_t *clear;
    int32_t i;
    int16_t actor_type_slot;
    int16_t threat_class;
    uint8_t in_group_a;
    uint8_t in_group_b;
    uint8_t in_group_c;
    uint8_t low_priority_kind;
    uint8_t suppress_close_bonus;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    best_desirability = 0.0f;
    best = (datum_index)k_datum_index_none;

    suppress_close_bonus = 0;
    if (self->berserking != 0 || self->mode == 10) {
        suppress_close_bonus = 1;
    }

    clear = (uint32_t *)&self->tally;
    for (i = 0x1e; i != 0; i--) {
        *clear = 0;
        clear++;
    }
    *(int16_t *)clear = 0;
    *((uint8_t *)clear + 2) = 0;

    for (prop_index = self->first_prop; prop_index != (datum_index)k_datum_index_none;
         prop_index = p->next_in_actor) {
        p = &((prop *)prop_data->data)[prop_index & 0xffff];

        if (p->state > 1 && p->state < 4 && p->dead == 0) {
            if (p->enemy == 0) {
                tracked_object = ((object_header *)object_data->data)[p->object_index & 0xffff].data;
                owning_actor_index = *(datum_index *)((uint8_t *)tracked_object + 0x1f4);
                target_actor = (actor *)0;
                if (owning_actor_index != (datum_index)k_datum_index_none) {
                    target_actor = (actor *)((uint8_t *)actor_data->data +
                                             (owning_actor_index & 0xffff) * sizeof(actor));
                }

                if (*(int32_t *)((uint8_t *)tracked_object + 0x218) != -1) {
                    actor_type_slot = 6;
                } else if (target_actor == (actor *)0) {
                    actor_type_slot = 14;
                } else {
                    actor_type_slot = target_actor->type;
                }

                in_group_a = 0;
                in_group_b = 0;
                if (p->distance >= 8.0f) {
                    if (p->unknown_12d != 0 && target_actor != (actor *)0 &&
                        self->target_unit_index != (datum_index)k_datum_index_none &&
                        target_actor->target_unit_index != (datum_index)k_datum_index_none &&
                        ((prop *)prop_data->data)[self->target_unit_index & 0xffff].object_index ==
                        ((prop *)prop_data->data)[target_actor->target_unit_index & 0xffff].object_index) {
                        in_group_a = 1;
                    }
                } else {
                    in_group_a = 1;
                }

                in_group_c = 0;
                if (p->obstruction == 0 || p->obstruction == 1) {
                    in_group_b = 1;
                    in_group_c = (uint8_t)(p->distance < 3.0f);
                }

                if (in_group_a) {
                    self->tally.group_a_total = (uint8_t)(self->tally.group_a_total + 1);
                    if (p->unknown_12d != 0) {
                        self->tally.group_a_marked = (uint8_t)(self->tally.group_a_marked + 1);
                        if (p->unknown_12d != 0 && p->is_vehicle_gunner != 0) {
                            self->tally.group_a_marked_135 =
                                (uint8_t)(self->tally.group_a_marked_135 + 1);
                        }
                    }
                    self->tally.group_a_by_actor_type[actor_type_slot] =
                        (uint8_t)(self->tally.group_a_by_actor_type[actor_type_slot] + 1);
                    if (p->unknown_12d != 0) {
                        self->tally.group_a_marked_by_actor_type[actor_type_slot] =
                            (uint8_t)(self->tally.group_a_marked_by_actor_type[actor_type_slot] + 1);
                    }
                }
                if (in_group_b) {
                    self->tally.group_b_total = (uint8_t)(self->tally.group_b_total + 1);
                    if (p->unknown_12d != 0) {
                        self->tally.group_b_marked = (uint8_t)(self->tally.group_b_marked + 1);
                    }
                    self->tally.group_b_by_actor_type[actor_type_slot] =
                        (uint8_t)(self->tally.group_b_by_actor_type[actor_type_slot] + 1);
                    if (p->unknown_12d != 0) {
                        self->tally.group_b_marked_by_actor_type[actor_type_slot] =
                            (uint8_t)(self->tally.group_b_marked_by_actor_type[actor_type_slot] + 1);
                    }
                }
                if (in_group_c) {
                    self->tally.group_c_total = (uint8_t)(self->tally.group_c_total + 1);
                    if (p->unknown_12d != 0) {
                        self->tally.group_c_marked = (uint8_t)(self->tally.group_c_marked + 1);
                    }
                    self->tally.group_c_by_actor_type[actor_type_slot] =
                        (uint8_t)(self->tally.group_c_by_actor_type[actor_type_slot] + 1);
                    if (p->unknown_12d != 0) {
                        self->tally.group_c_marked_by_actor_type[actor_type_slot] =
                            (uint8_t)(self->tally.group_c_marked_by_actor_type[actor_type_slot] + 1);
                    }
                }
            } else {
                low_priority_kind = (uint8_t)(p->visual_perception < 2);
                threat_class = 0;
                self->tally.unit_props = (uint8_t)(self->tally.unit_props + 1);

                if (low_priority_kind && (p->shooting == 0 || p->obstruction != 0)) {
                    // the whole classification below is skipped
                } else {
                    if (!low_priority_kind) {
                        if (p->engaged_ticks == 0) {
                            self->tally.unit_props_unseen =
                                (uint8_t)(self->tally.unit_props_unseen + 1);
                        }
                        self->tally.threat_class_ge_1 =
                            (uint8_t)(self->tally.threat_class_ge_1 + 1);
                        threat_class = 1;
                    }
                    if (p->seen != 0) {
                        self->tally.threat_class_8 = (uint8_t)(self->tally.threat_class_8 + 1);
                        if (threat_class < 9) {
                            threat_class = 8;
                        }
                    }
                    if (p->shooting != 0) {
                        self->tally.threat_class_4 = (uint8_t)(self->tally.threat_class_4 + 1);
                        if (threat_class < 5) {
                            threat_class = 4;
                        }
                    }
                    if ((int8_t)p->aiming_at_actor_class < 3) {
                        if (!low_priority_kind) {
                            self->tally.threat_class_2 = (uint8_t)(self->tally.threat_class_2 + 1);
                            if (threat_class < 3) {
                                threat_class = 2;
                            }
                            if (suppress_close_bonus == 0 && p->distance < 2.0f) {
                                self->tally.threat_class_7 =
                                    (uint8_t)(self->tally.threat_class_7 + 1);
                                if (threat_class < 8) {
                                    threat_class = 7;
                                }
                            }
                        }
                        if ((int8_t)p->aiming_at_actor_class < 2) {
                            if (p->shooting != 0) {
                                self->tally.threat_class_5 =
                                    (uint8_t)(self->tally.threat_class_5 + 1);
                                if (threat_class < 6) {
                                    threat_class = 5;
                                }
                            }
                            if ((int8_t)p->aiming_at_actor_class < 1) {
                                if (!low_priority_kind) {
                                    self->tally.threat_class_3 =
                                        (uint8_t)(self->tally.threat_class_3 + 1);
                                    if (threat_class < 4) {
                                        threat_class = 3;
                                    }
                                }
                                if (p->shooting != 0) {
                                    self->tally.threat_class_6 =
                                        (uint8_t)(self->tally.threat_class_6 + 1);
                                    if (threat_class < 7) {
                                        threat_class = 6;
                                    }
                                }
                            }
                        }
                    }
                }
                self->tally.by_threat_class[threat_class] =
                    (uint8_t)(self->tally.by_threat_class[threat_class] + 1);
            }
        }

        if (best_desirability < p->desirability) {
            best = prop_index;
            best_desirability = p->desirability;
        }
    }

    previous = self->target_unit_index;
    if (best != previous) {
        self->target_combat_status = 0;
        self->target_unit_index = best;
        self->target_last_seen_time = (datum_index)k_datum_index_none;
        if (previous != (datum_index)k_datum_index_none) {
            ((prop *)prop_data->data)[previous & 0xffff].desirability =
                actor_rate_potential_target(actor_index, previous);
        }
        if (best != (datum_index)k_datum_index_none) {
            ((prop *)prop_data->data)[best & 0xffff].desirability =
                actor_rate_potential_target(actor_index, self->target_unit_index);
        }
    }
    actor_update_target_combat_status(actor_index);
    actor_update_awareness_level(actor_index);
}

#if 0
Original Ghidra decompilation (0x4203a0):

void actor_choose_best_target(uint param_1)

{
  char *pcVar1;
  float fVar2;
  uint uVar3;
  uint uVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  bool bVar8;
  int iVar9;
  int iVar10;
  ushort uVar11;
  int iVar12;
  short sVar13;
  int iVar14;
  undefined4 *puVar15;
  float10 fVar16;
  uint local_c;
  uint local_4;

  iVar10 = DAT_00880360;
  fVar2 = 0.0;
  iVar12 = (param_1 & 0xffff) * 0x724;
  iVar14 = *(int *)(DAT_00880360 + 0x34) + iVar12;
  local_c = 0xffffffff;
  if ((*(char *)(*(int *)(DAT_00880360 + 0x34) + 0x378 + iVar12) != '\0') ||
     (bVar7 = false, *(short *)(iVar14 + 0x6c) == 10)) {
    bVar7 = true;
  }
  puVar15 = (undefined4 *)(iVar14 + 0x1ec);
  for (iVar9 = 0x1e; iVar9 != 0; iVar9 = iVar9 + -1) {
    *puVar15 = 0;
    puVar15 = puVar15 + 1;
  }
  *(undefined2 *)puVar15 = 0;
  *(undefined1 *)((int)puVar15 + 2) = 0;
  uVar3 = *(uint *)(iVar12 + 0x50 + *(int *)(iVar10 + 0x34));
  do {
    local_4 = uVar3;
    if (local_4 == 0xffffffff) {
      uVar3 = *(uint *)(iVar14 + 0x270);
      if (local_c != uVar3) {
        *(undefined2 *)(iVar14 + 0x268) = 0;
        *(uint *)(iVar14 + 0x270) = local_c;
        *(undefined4 *)(iVar14 + 0x26c) = 0xffffffff;
        if (uVar3 != 0xffffffff) {
          iVar10 = *(int *)(DAT_008802c0 + 0x34);
          fVar16 = (float10)actor_rate_potential_target(param_1,uVar3);
          *(float *)((uVar3 & 0xffff) * 0x138 + iVar10 + 0x50) = (float)fVar16;
        }
        if (local_c != 0xffffffff) {
          iVar10 = *(int *)(DAT_008802c0 + 0x34);
          fVar16 = (float10)actor_rate_potential_target(param_1,*(undefined4 *)(iVar14 + 0x270));
          *(float *)((local_c & 0xffff) * 0x138 + iVar10 + 0x50) = (float)fVar16;
        }
      }
      actor_update_target_combat_status();
      actor_update_awareness_level();
      return;
    }
    iVar10 = (local_4 & 0xffff) * 0x138;
    uVar3 = *(uint *)(iVar10 + 8 + *(int *)(DAT_008802c0 + 0x34));
    iVar10 = iVar10 + *(int *)(DAT_008802c0 + 0x34);
    if (((1 < *(short *)(iVar10 + 0x24)) && (*(short *)(iVar10 + 0x24) < 4)) &&
       (*(char *)(iVar10 + 0x127) == '\0')) {
      if (*(char *)(iVar10 + 0x60) == '\0') {
        iVar12 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                         (*(uint *)(iVar10 + 0x18) & 0xffff) * 0xc);
        uVar4 = *(uint *)(iVar12 + 500);
        if (uVar4 == 0xffffffff) {
          iVar9 = 0;
        }
        else {
          iVar9 = (uVar4 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
        }
        if (*(int *)(iVar12 + 0x218) == -1) {
          if (iVar9 == 0) {
            sVar13 = 0xe;
          }
          else {
            sVar13 = *(short *)(iVar9 + 4);
          }
        }
        else {
          sVar13 = 6;
        }
        bVar5 = false;
        bVar8 = false;
        if (8.0 <= *(float *)(iVar10 + 0x11c)) {
          if (((*(char *)(iVar10 + 0x12d) != '\0') && (iVar9 != 0)) &&
             ((*(uint *)(iVar14 + 0x270) != 0xffffffff &&
              ((*(uint *)(iVar9 + 0x270) != 0xffffffff &&
               (*(int *)((*(uint *)(iVar14 + 0x270) & 0xffff) * 0x138 + 0x18 +
                        *(int *)(DAT_008802c0 + 0x34)) ==
                *(int *)((*(uint *)(iVar9 + 0x270) & 0xffff) * 0x138 + 0x18 +
                        *(int *)(DAT_008802c0 + 0x34)))))))) {
            bVar5 = true;
          }
        }
        else {
          bVar5 = true;
        }
        bVar6 = false;
        if ((*(short *)(iVar10 + 0x38) == 0) || (*(short *)(iVar10 + 0x38) == 1)) {
          bVar8 = true;
          if (3.0 <= *(float *)(iVar10 + 0x11c)) {
            bVar6 = false;
          }
          else {
            bVar6 = true;
          }
        }
        if (bVar5) {
          *(char *)(iVar14 + 0x200) = *(char *)(iVar14 + 0x200) + '\x01';
          if (((*(char *)(iVar10 + 0x12d) != '\0') &&
              (*(char *)(iVar14 + 0x201) = *(char *)(iVar14 + 0x201) + '\x01',
              *(char *)(iVar10 + 0x12d) != '\0')) && (*(char *)(iVar10 + 0x135) != '\0')) {
            *(char *)(iVar14 + 0x202) = *(char *)(iVar14 + 0x202) + '\x01';
          }
          iVar12 = sVar13 + iVar14;
          *(char *)(iVar12 + 0x203) = *(char *)(sVar13 + 0x203 + iVar14) + '\x01';
          if (*(char *)(iVar10 + 0x12d) != '\0') {
            pcVar1 = (char *)(iVar12 + 0x213);
            *pcVar1 = *pcVar1 + '\x01';
          }
        }
        if (bVar8) {
          *(char *)(iVar14 + 0x223) = *(char *)(iVar14 + 0x223) + '\x01';
          if (*(char *)(iVar10 + 0x12d) != '\0') {
            *(char *)(iVar14 + 0x224) = *(char *)(iVar14 + 0x224) + '\x01';
          }
          iVar12 = sVar13 + iVar14;
          *(char *)(iVar12 + 0x225) = *(char *)(sVar13 + 0x225 + iVar14) + '\x01';
          if (*(char *)(iVar10 + 0x12d) != '\0') {
            pcVar1 = (char *)(iVar12 + 0x235);
            *pcVar1 = *pcVar1 + '\x01';
          }
        }
        if (bVar6) {
          *(char *)(iVar14 + 0x245) = *(char *)(iVar14 + 0x245) + '\x01';
          if (*(char *)(iVar10 + 0x12d) != '\0') {
            *(char *)(iVar14 + 0x246) = *(char *)(iVar14 + 0x246) + '\x01';
          }
          iVar12 = sVar13 + iVar14;
          *(char *)(iVar12 + 0x247) = *(char *)(sVar13 + 0x247 + iVar14) + '\x01';
          if (*(char *)(iVar10 + 0x12d) != '\0') {
            pcVar1 = (char *)(iVar12 + 599);
            *pcVar1 = *pcVar1 + '\x01';
          }
        }
      }
      else {
        bVar5 = *(short *)(iVar10 + 0x32) < 2;
        uVar11 = 0;
        *(char *)(iVar14 + 0x1ec) = *(char *)(iVar14 + 0x1ec) + '\x01';
        if (bVar5) {
          if ((*(char *)(iVar10 + 0x12f) != '\0') && (*(short *)(iVar10 + 0x38) == 0))
          goto LAB_004204ab;
        }
        else {
          if (*(short *)(iVar10 + 0x9c) == 0) {
            *(char *)(iVar14 + 0x1ed) = *(char *)(iVar14 + 0x1ed) + '\x01';
          }
          *(char *)(iVar14 + 0x1f8) = *(char *)(iVar14 + 0x1f8) + '\x01';
          uVar11 = 1;
LAB_004204ab:
          if ((*(char *)(iVar10 + 0x74) != '\0') &&
             (*(char *)(iVar14 + 0x1ff) = *(char *)(iVar14 + 0x1ff) + '\x01', uVar11 < 9)) {
            uVar11 = 8;
          }
          if ((*(char *)(iVar10 + 0x12f) != '\0') &&
             (*(char *)(iVar14 + 0x1fb) = *(char *)(iVar14 + 0x1fb) + '\x01', uVar11 < 5)) {
            uVar11 = 4;
          }
          if (*(char *)(iVar10 + 0x122) < '\x03') {
            if (!bVar5) {
              *(char *)(iVar14 + 0x1f9) = *(char *)(iVar14 + 0x1f9) + '\x01';
              if (uVar11 < 3) {
                uVar11 = 2;
              }
              if (((!bVar7) && (*(float *)(iVar10 + 0x11c) < 2.0)) &&
                 (*(char *)(iVar14 + 0x1fe) = *(char *)(iVar14 + 0x1fe) + '\x01', uVar11 < 8)) {
                uVar11 = 7;
              }
            }
            if (*(char *)(iVar10 + 0x122) < '\x02') {
              if ((*(char *)(iVar10 + 0x12f) != '\0') &&
                 (*(char *)(iVar14 + 0x1fc) = *(char *)(iVar14 + 0x1fc) + '\x01', uVar11 < 6)) {
                uVar11 = 5;
              }
              if (*(char *)(iVar10 + 0x122) < '\x01') {
                if ((!bVar5) &&
                   (*(char *)(iVar14 + 0x1fa) = *(char *)(iVar14 + 0x1fa) + '\x01', uVar11 < 4)) {
                  uVar11 = 3;
                }
                if ((*(char *)(iVar10 + 0x12f) != '\0') &&
                   (*(char *)(iVar14 + 0x1fd) = *(char *)(iVar14 + 0x1fd) + '\x01', uVar11 < 7)) {
                  uVar11 = 6;
                }
              }
            }
          }
        }
        pcVar1 = (char *)((short)uVar11 + 0x1ee + iVar14);
        *pcVar1 = *pcVar1 + '\x01';
      }
    }
    if (fVar2 < *(float *)(iVar10 + 0x50)) {
      local_c = local_4;
      fVar2 = *(float *)(iVar10 + 0x50);
    }
  } while( true );
}
#endif
