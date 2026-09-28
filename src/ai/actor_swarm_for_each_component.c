// actor_swarm_for_each_component  (Ghidra: actor_swarm_for_each_component, renamed)
// address 0x407040, size 244 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (checked against objdump 0x407040..0x407133)
// evidence: types/ai.h actor.swarm (0x06)/swarm_index (0x28)/unit_index (0x18);
//   swarm.component_count (0x02)/unit_index[16] (0x18)/component_index[16] (0x58);
//   swarm_component (size 0x40). phase-4's "for every member of the actor's squad" is this
//   session's swarm/squad terminology mismatch (see types/ai.h's own module header note on
//   "swarm" vs the scripted "squad"): this function clearly walks swarm_data/
//   swarm_component_data, not any squad/encounter structure, so it is named for what it does.
// register convention: actor index in EAX, a "reset first" flag and the callback in the
//   recognized stack parameters, plus a caller-owned record pointer in EDI (unaff_EDI) that
//   is forwarded into the callback largely unexamined.
//   // blam-cc: EAX -> actor_index, stack -> reset_first/callback/callback_extra,
//   //          EDI -> caller_record
// TYPES-GAP/UNSURE: `caller_record`'s shape is not established (only offsets 0, 8, 0x2c are
// read); the callback's third and fourth parameters are forwarded through unexamined.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>
#include <string.h>
#include "objects.h"
#include "units.h"

extern data_array *actor_data;           // 0x00880360
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358


// If the actor is not a swarm, invokes callback once for the actor's own unit. Otherwise,
// for every live component of the actor's swarm: optionally (reset_first) zeroes the
// component's scratch region (swarm_component+0x1c..0x3f) and marks it bit 3 "active" in its
// flags (+2), then, if that active bit is set, invokes callback for that component's unit.
void actor_swarm_for_each_component(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback, uint32_t callback_extra, uint16_t *caller_record)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (a->swarm == 0) {
        // UNSURE: the original's 5th callback argument is caller_record+0x16 here (a pointer)
        // but a plain 0 in the swarm branch below; the callback typedef fixes that slot to
        // int32_t to match the more common case, so this call carries the pointer's bit
        // pattern through the cast instead.
        callback(actor_index, a->unit_index, *caller_record, caller_record + 4, (int32_t)(intptr_t)(caller_record + 0x16), callback_extra);
        return;
    }

    {
        swarm *sw = &((swarm *)swarm_data->data)[a->swarm_index & 0xffff];
        int16_t i;

        for (i = 0; i < sw->component_count; i++) {
            swarm_component *comp = &((swarm_component *)swarm_component_data->data)[sw->component_index[i] & 0xffff];
            uint8_t *comp_base = (uint8_t *)comp;

            if (reset_first != 0) {
                memset(comp_base + 0x1c, 0, 0x24);
                *(uint16_t *)&((struct swarm_component *)comp_base)->flags = (*(uint16_t *)&((struct swarm_component *)comp_base)->flags & 0xfffb) | 8;
            }
            if ((comp_base[2] & 8) != 0) {
                callback(actor_index, sw->unit_index[i], *caller_record, comp_base + 0x1c, 0, callback_extra);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x407040):

void FUN_00407040(uint param_1,char param_2,code *param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  short sVar3;
  undefined2 *unaff_EDI;

  iVar1 = (param_1 & 0xffff) * 0x724;
  iVar2 = iVar1 + *(int *)(DAT_00880360 + 0x34);
  if (*(char *)(iVar1 + 6 + *(int *)(DAT_00880360 + 0x34)) != '\0') {
    iVar1 = (*(uint *)(iVar2 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
    sVar3 = 0;
    if (0 < *(short *)(iVar1 + 2)) {
      do {
        iVar2 = (*(uint *)(iVar1 + 0x58 + sVar3 * 4) & 0xffff) * 0x40 +
                *(int *)(DAT_00880358 + 0x34);
        if (param_2 != '\0') {
          *(undefined4 *)(iVar2 + 0x1c) = 0;
          *(undefined4 *)(iVar2 + 0x20) = 0;
          *(undefined4 *)(iVar2 + 0x24) = 0;
          *(undefined4 *)(iVar2 + 0x28) = 0;
          *(undefined4 *)(iVar2 + 0x2c) = 0;
          *(undefined4 *)(iVar2 + 0x30) = 0;
          *(undefined4 *)(iVar2 + 0x34) = 0;
          *(undefined4 *)(iVar2 + 0x38) = 0;
          *(undefined4 *)(iVar2 + 0x3c) = 0;
          *(ushort *)(iVar2 + 2) = *(ushort *)(iVar2 + 2) & 0xfffb | 8;
        }
        if ((*(byte *)(iVar2 + 2) & 8) != 0) {
          (*param_3)(param_1,*(undefined4 *)(iVar1 + 0x18 + sVar3 * 4),*unaff_EDI,iVar2 + 0x1c,0,
                     param_4);
        }
        sVar3 = sVar3 + 1;
      } while (sVar3 < *(short *)(iVar1 + 2));
    }
    return;
  }
  (*param_3)(param_1,*(undefined4 *)(iVar2 + 0x18),*unaff_EDI,unaff_EDI + 4,unaff_EDI + 0x16,param_4
            );
  return;
}
#endif
