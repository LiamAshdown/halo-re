// actor_find_or_create_shared_prop  (Ghidra: actor_find_or_create_shared_prop, renamed)
// address 0x43eb30, size 411 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: types/ai.h actor.first_prop(+0x50), prop.next_in_actor(+0x08)/pair_index(+0x0c)/
// has_parent(+0x14)/object_index(+0x18)/owner_actor_index(+0x1c)/kind(+0x24)/is_vault(+0x127,
// used here as `+0x126` for a related "seen while vaulted"-style flag)/unknown_30/unknown_6a
// (though the header's actor field of the same numeric names does not apply here -- these
// are prop's own); object+0x1f4/+0x1f8 (see actor_find_prop_for_object.c, same lookup
// pattern). phase-4 summary "finds or creates a shared firing-position node between an actor
// and a target object and initializes its combat-relevant fields." Calls
// actor_target_reset_combat_flags (established elsewhere) and
// actor_find_or_allocate_prop (0x43e270, this rewrite's own actor_find_or_allocate_prop.c),
// plus actor_target_data_refresh/0x41c8f0/0x41f410/0x45bd50 (outside this rewrite's range).
//
// register convention: EAX -> object_index; stack -> actor_index, create_if_missing, flag.
//   // blam-cc: EAX -> object_index, stack -> actor_index, create_if_missing, flag
//
// UNSURE: actor_target_data_refresh/0x41c8f0 are called here with fewer arguments than a canonical
// signature would likely need (a 56-byte stack scratch block, `local_38`, is shared between
// both calls); reproduced with an opaque byte buffer rather than a named type.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0
extern data_array *prop_data;   // 0x008802c0

extern void actor_target_reset_combat_flags(datum_index target_prop_index, datum_index actor_index, uint32_t unused,
    uint8_t already_noticed); // 0x41baf0, ECX, stack
extern datum_index actor_find_or_allocate_prop(datum_index actor_index, uint32_t param_2, char kind); // 0x43e270
extern void actor_target_data_refresh(datum_index actor_index, datum_index prop_index, void *scratch, uint32_t param4,
                         uint32_t flag); // 0x41c4b0, outside this rewrite's range
extern void actor_target_update_tracking_speed(datum_index actor_index, datum_index prop_index, void *scratch); // 0x41c8f0, outside this rewrite's range
extern uint8_t actor_target_has_conflicting_neighbor(datum_index actor_index, datum_index target_prop_index); // 0x41f410, EAX, stack
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, CX, DX

// blam-cc: EAX -> object_index, stack -> actor_index, create_if_missing, flag
datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
                                             char create_if_missing, uint32_t flag)
{
    datum_index result = (datum_index)0xffffffff;
    actor *self;
    uint8_t *object;
    int32_t cluster_ref;

    if (object_index == (datum_index)0xffffffff) {
        return result;
    }

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    object = *(uint8_t **)((uint8_t *)object_data->data + 8 + (object_index & 0xffff) * 0xc);
    cluster_ref = *(int32_t *)(object + 0x1f8);
    if (cluster_ref == -1) {
        cluster_ref = *(int32_t *)(object + 500);
    }

    if ((((struct object *)object)->type == 0) && ((datum_index)cluster_ref != actor_index)) {
        datum_index cur = self->first_prop;

        for (;;) {
            prop *p;
            if (cur == (datum_index)0xffffffff) {
                goto not_found;
            }
            p = (prop *)((uint8_t *)prop_data->data + (cur & 0xffff) * sizeof(prop));
            if ((p->object_index == object_index) ||
                ((p->has_parent != 0) && (p->owner_actor_index != (datum_index)0xffffffff) &&
                 ((int32_t)p->owner_actor_index == cluster_ref))) {
                break;
            }
            cur = p->next_in_actor;
        }

        {
            prop *p = (prop *)((uint8_t *)prop_data->data + (cur & 0xffff) * sizeof(prop));
            result = cur;
            if (p->pair_index != (datum_index)0xffffffff) {
                result = p->pair_index;
            }
        }

        if (result == (datum_index)0xffffffff) {
        not_found:
            if ((create_if_missing != 0) && (self->active != 0)) {
                uint8_t scratch[56];

                // 0x43ec21: the prop is allocated as an enemy when the object's team (+0xb8) is hostile to the actor's
                result = actor_find_or_allocate_prop(actor_index, object_index,
                    (char)teams_are_enemies(((struct object *)object)->owner_team, *(int16_t *)((uint8_t *)self + 0x3e)));
                if (result != (datum_index)0xffffffff) {
                    prop *p = (prop *)((uint8_t *)prop_data->data + (result & 0xffff) * sizeof(prop));

                    actor_target_data_refresh(actor_index, result, scratch, 0, flag);
                    p->unknown_6a = 0x1e;
                    p->unknown_126 = 1;

                    if ((uint8_t)flag != 0 && (actor_target_update_tracking_speed(actor_index, result, scratch), 1 < p->unknown_30)) {
                        uint8_t seen_flag = actor_target_has_conflicting_neighbor(actor_index, result); // 0x43eca0: EAX = actor
                        p->kind = 3;
                        actor_target_reset_combat_flags(result, actor_index, 0, seen_flag); // 0x43ecb4: ECX = prop
                    }
                }
            }
        }
    }

    return result;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043eb30 @ 0x43eb30) ----
uint FUN_0043eb30(uint param_1,char param_2,undefined4 param_3)

{
  int iVar1;
  uint uVar2;
  undefined1 uVar3;
  uint in_EAX;
  uint uVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  undefined4 local_44;
  undefined1 local_38 [56];

  uVar7 = 0xffffffff;
  if (in_EAX != 0xffffffff) {
    iVar1 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    uVar4 = *(uint *)(iVar5 + 0x1f8);
    if (uVar4 == 0xffffffff) {
      uVar4 = *(uint *)(iVar5 + 500);
    }
    if ((*(short *)(iVar5 + 0xb4) == 0) && (uVar4 != param_1)) {
      uVar2 = *(uint *)(iVar1 + 0x50);
      do {
        uVar6 = uVar2;
        if (uVar6 == 0xffffffff) goto LAB_0043ec06;
        iVar5 = (uVar6 & 0xffff) * 0x138;
        uVar2 = *(uint *)(iVar5 + 8 + *(int *)(DAT_008802c0 + 0x34));
        iVar5 = iVar5 + *(int *)(DAT_008802c0 + 0x34);
      } while ((*(uint *)(iVar5 + 0x18) != in_EAX) &&
              (((*(char *)(iVar5 + 0x14) == '\0' || (*(uint *)(iVar5 + 0x1c) == 0xffffffff)) ||
               (*(uint *)(iVar5 + 0x1c) != uVar4))));
      uVar7 = uVar6;
      if (*(uint *)(iVar5 + 0xc) != 0xffffffff) {
        uVar7 = *(uint *)(iVar5 + 0xc);
      }
      if (uVar7 == 0xffffffff) {
LAB_0043ec06:
        if ((param_2 != '\0') && (*(char *)(iVar1 + 8) != '\0')) {
          FUN_0045bd50();
          uVar7 = FUN_0043e270(param_1);
          if (uVar7 != 0xffffffff) {
            iVar5 = (uVar7 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
            FUN_0041c4b0(param_1,uVar7,local_38,0,param_3);
            *(undefined2 *)(iVar5 + 0x6a) = 0x1e;
            *(undefined1 *)(iVar5 + 0x126) = 1;
            if (((char)param_3 != '\0') &&
               (FUN_0041c8f0(param_1,uVar7,local_38), 1 < *(short *)(iVar5 + 0x30))) {
              uVar3 = FUN_0041f410(uVar7);
              local_44 = CONCAT31((int3)((uint)iVar1 >> 8),uVar3);
              *(undefined2 *)(iVar5 + 0x24) = 3;
              actor_target_reset_combat_flags(param_1,0,local_44);
            }
          }
        }
      }
    }
  }
  return uVar7;
}
#endif
