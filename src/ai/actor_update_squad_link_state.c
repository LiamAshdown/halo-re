// actor_update_squad_link_state  (Ghidra: actor_update_squad_link_state, already named)
// address 0x429270, size 438 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (checked against objdump 0x429270..0x429425)
// evidence: types/ai.h actor.swarm(0x06)/swarm_index(0x28)/unknown_4a4/unknown_78/unknown_74/
//   unknown_92/encounter_index(0x34)/keep_unit_alive(0x13)/unknown_12/target_unit_index(0x270)/
//   secondary_action(0x46c); encounter.unknown_0c; prop.is_parented(0x12e)/is_unit(0x60)/
//   is_vault(0x127)/unknown_24. Calls actor_set_units_active (0x427860) and
//   actor_delete_or_release_unit (0x4288e0), both already rewritten in this module.
//   UNSURE: this is one of the least-confident rewrites in this pass, given how many
//   branches this function has; the `activate` argument passed to actor_set_units_active and
//   the `is_dead` flag passed to actor_delete_or_release_unit are both guessed (1 and 0
//   respectively) from context rather than confirmed with objdump. actor+0x14 (a per-tick
//   staleness counter compared against 0x3b/59) has no individually established name.
// register convention: stack -> actor_index (Ghidra already resolved this as a genuine
//   parameter).
//   // blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data;     // 0x00880360
extern data_array *encounter_data; // 0x008802c8
extern data_array *prop_data;      // 0x008802c0
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

extern void actor_set_units_active(datum_index actor_index, uint8_t dormant); // 0x427860
extern void actor_delete_or_release_unit(datum_index actor_index, uint8_t is_dead); // 0x4288e0

// blam-cc: stack -> actor_index
// Per-tick housekeeping for an actor's link to its squad/encounter: releases a swarm actor
// that has lost its swarm outright; ages a couple of timers; reactivates the actor's units
// when it or its encounter requests it; otherwise, once its current target has become
// invalid/stale for long enough (or after a ~60-tick staleness counter expires), reactivates
// it anyway so it can pick a new target. Returns false only when the actor was deleted.
uint8_t actor_update_squad_link_state(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    encounter *enc = 0;
    uint8_t combined_flag;

    if (self->swarm != 0 && self->swarm_index == (datum_index)k_datum_index_none) {
        actor_delete_or_release_unit(actor_index, 0);
        return 0;
    }

    self->unknown_4a4 = 0;

    if (self->suspicion_timer > 0) {
        self->suspicion_timer = self->suspicion_timer - 1;
        if (self->suspicion_timer == 0) {
            self->suspicion_status = 0;
        }
    }
    if (self->unknown_92 > 0) {
        self->unknown_92 = self->unknown_92 - 1;
    }

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        enc = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
    }

    combined_flag = self->unknown_0a;
    if (enc != 0) {
        combined_flag |= enc->unknown_0c;
    }

    if (self->unknown_12 == 0 || combined_flag != 0) {
        actor_set_units_active(actor_index, 0); // 0x429414: BL = 0 (wake)
    } else if (self->keep_unit_alive == 0) {
        int16_t combat_grade = actor_mode_definitions[self->mode].combat_grade;
        int stale = 1;

        if (combat_grade != 2) {
            if (self->target_unit_index != (datum_index)k_datum_index_none) {
                prop *target = &((prop *)prop_data->data)[self->target_unit_index & 0xffff];
                if (target->is_parented != 0 && target->is_unit != 0 && target->is_vault == 0) {
                    int16_t kind = target->kind;
                    // 0x429387: kinds 2..3 always count, kinds 4..5 only for a grade 3 (combat) mode
                    if ((kind >= 2 && kind <= 3) || (kind >= 4 && kind <= 5 && combat_grade == 3)) {
                        stale = 0;
                    }
                }
            }
        } else {
            stale = 0;
        }

        if (stale) {
            uint8_t movement_done = self->movement_action_complete;
            if (movement_done != 0) {
                // 0x4293b7: the ACTIVE movement action's type (+0x46c), not the secondary action (+0x418)
                if (self->active_movement.type == 3) {
                    if (self->mode == 6 && enc != 0 && ((struct encounter *)enc)->unknown_62 == 1) {
                        return 1;
                    }
                } else if (self->active_movement.type == 5) {
                    // 0x4293c8: +0x470, the first dword after the type, holds the prop for this type
                    prop *p = &((prop *)prop_data->data)[*(datum_index *)&((struct actor *)self)->active_movement.destination.x & 0xffff];
                    if (p->is_parented != 0) {
                        return 1;
                    }
                }
            }
            *(int16_t *)((uint8_t *)self + 0x14) = *(int16_t *)((uint8_t *)self + 0x14) + 1; // UNSURE, see file header
            if (*(int16_t *)((uint8_t *)self + 0x14) > 0x3b) {
                actor_set_units_active(actor_index, 1);
                return 1;
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x429270):

undefined4 actor_update_squad_link_state(uint param_1)

{
  short sVar1;
  byte bVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;

  iVar5 = DAT_00880360;
  iVar4 = (param_1 & 0xffff) * 0x724;
  iVar3 = *(int *)(DAT_00880360 + 0x34) + iVar4;
  if ((*(char *)(*(int *)(DAT_00880360 + 0x34) + 6 + iVar4) != '\0') &&
     (*(int *)(iVar3 + 0x28) == -1)) {
    actor_delete_or_release_unit(param_1);
    return 0;
  }
  iVar6 = 0;
  *(undefined1 *)(iVar3 + 0x4a4) = 0;
  if ((0 < *(int *)(iVar3 + 0x78)) &&
     (iVar7 = *(int *)(iVar3 + 0x78) + -1, *(int *)(iVar3 + 0x78) = iVar7, iVar7 == 0)) {
    *(undefined2 *)(iVar3 + 0x74) = 0;
  }
  if (0 < *(short *)(iVar3 + 0x92)) {
    *(short *)(iVar3 + 0x92) = *(short *)(iVar3 + 0x92) + -1;
  }
  if (*(uint *)(iVar3 + 0x34) != 0xffffffff) {
    iVar6 = (*(uint *)(iVar3 + 0x34) & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  }
  bVar2 = *(byte *)(iVar3 + 10);
  if (iVar6 != 0) {
    bVar2 = bVar2 | *(byte *)(iVar6 + 0xc);
  }
  if ((*(char *)(iVar3 + 0x12) == '\0') || (bVar2 != 0)) {
    actor_set_units_active();
  }
  else if (*(char *)(iVar3 + 0x13) == '\0') {
    iVar4 = *(int *)(iVar5 + 0x34) + iVar4;
    if ((*(short *)(&DAT_00655258 + *(short *)(iVar4 + 0x6c) * 0x38) != 2) &&
       (((((*(uint *)(iVar3 + 0x270) == 0xffffffff ||
           (iVar5 = (*(uint *)(iVar3 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34),
           *(char *)(iVar5 + 0x12e) == '\0')) || (*(char *)(iVar5 + 0x60) == '\0')) ||
         (*(char *)(iVar5 + 0x127) != '\0')) ||
        (((sVar1 = *(short *)(iVar5 + 0x24), sVar1 < 2 || (3 < sVar1)) &&
         ((sVar1 < 4 ||
          ((5 < sVar1 || (*(short *)(&DAT_00655258 + *(short *)(iVar4 + 0x6c) * 0x38) != 3))))))))))
    {
      if (*(char *)(iVar4 + 0x4a8) != '\0') {
        if (*(short *)(iVar3 + 0x46c) == 3) {
          if ((*(short *)(iVar3 + 0x6c) == 6) && (*(short *)(iVar6 + 0x62) == 1)) {
            return 1;
          }
        }
        else if ((*(short *)(iVar3 + 0x46c) == 5) &&
                (*(char *)((*(uint *)(iVar3 + 0x470) & 0xffff) * 0x138 + 0x12e +
                          *(int *)(DAT_008802c0 + 0x34)) != '\0')) {
          return 1;
        }
      }
      *(short *)(iVar3 + 0x14) = *(short *)(iVar3 + 0x14) + 1;
      if (0x3b < *(short *)(iVar3 + 0x14)) {
        actor_set_units_active();
        return 1;
      }
    }
  }
  return 1;
}
#endif
