// actor_target_get_priority_class  (Ghidra: actor_target_get_priority_class; named from out/phase2/results/ai_02.json)
// address 0x41be10, size 189 bytes
// name confidence: 0.45   rewrite confidence: 0.5
// evidence: out/phase2/results/ai_02.json -- classifies a target-data record into a small
//   integer (0-3) based on its combat state (kind 2..3 or seat state 1/2 forces 3) and
//   awareness flags at target-data+0x60/+0x127, falling back to actor posture (+0x6a/+0x6e)
//   when in_ECX is -1. Fields match prop.kind/is_unit/is_vault/unknown_66/pair_index and
//   actor.awareness_level/combat_status in types/ai.h.
// register convention: EAX -> actor_index, ECX -> target_prop_index (or k_datum_index_none).
//   // blam-cc: EAX -> actor_index, ECX -> target_prop_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

// blam-cc: EAX -> actor_index, ECX -> target_prop_index
// Computes a priority/urgency class (0-3) for a given target relative to the actor, used to
// rank target handling. A target in an active-combat kind, occupying a seat, or an exposed
// non-unit target the actor is alert enough to notice, always ranks 3. Otherwise, a paired
// prop (e.g. a vehicle and its rider) inherits a class of 2 or 3 from the pair's object type.
// With no target supplied, the result falls back to the actor's own posture: 2 while its
// vitality grade is still low, otherwise 0 or 1 depending on awareness level.
uint16_t actor_target_get_priority_class(datum_index actor_index, datum_index target_prop_index)
{
    actor *self;
    prop *target;
    prop *paired;
    uint16_t paired_class;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (target_prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));

        if (((1 < target->state && target->state < 4) || target->stimulus_type == 1 || target->stimulus_type == 2) ||
            (target->enemy == 0 && (target->dead == 0 || self->awareness_level > 2))) {
            return 3;
        }

        if (target->pair_index != k_datum_index_none) {
            paired = (prop *)((uint8_t *)prop_data->data + (target->pair_index & 0xffff) * sizeof(prop));
            paired_class = (uint16_t)((paired->has_current_information != 0) + 2);
            // UNSURE: Ghidra's `if (uVar3 != 0xffff) return uVar3;` is dead code as written --
            // paired_class is always 2 or 3 -- preserved exactly rather than simplified.
            if (paired_class != 0xffff) {
                return paired_class;
            }
        }
    }

    if (self->combat_status < 2) {
        return (uint16_t)(self->awareness_level > 2);
    }
    return 2;
}

#if 0
Original Ghidra decompilation (0x41be10):

ushort FUN_0041be10(void)

{
  short sVar1;
  int iVar2;
  ushort uVar3;
  uint in_EAX;
  int iVar4;
  uint in_ECX;
  int iVar5;

  iVar4 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (in_ECX != 0xffffffff) {
    iVar2 = *(int *)(DAT_008802c0 + 0x34);
    iVar5 = (in_ECX & 0xffff) * 0x138;
    sVar1 = *(short *)(iVar5 + 0x24 + iVar2);
    iVar5 = iVar5 + iVar2;
    if (((((1 < sVar1) && (sVar1 < 4)) || (*(short *)(iVar5 + 0x66) == 1)) ||
        (*(short *)(iVar5 + 0x66) == 2)) ||
       ((*(char *)(iVar5 + 0x60) == '\0' &&
        ((*(char *)(iVar5 + 0x127) == '\0' || (2 < *(short *)(iVar4 + 0x6a))))))) {
      return 3;
    }
    if ((*(uint *)(iVar5 + 0xc) != 0xffffffff) &&
       (uVar3 = (*(char *)((*(uint *)(iVar5 + 0xc) & 0xffff) * 0x138 + 0xb8 + iVar2) != '\0') + 2,
       uVar3 != 0xffff)) {
      return uVar3;
    }
  }
  if (*(short *)(iVar4 + 0x6e) < 2) {
    return (ushort)(2 < *(short *)(iVar4 + 0x6a));
  }
  return 2;
}
#endif
