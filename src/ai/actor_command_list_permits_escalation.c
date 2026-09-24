// actor_command_list_permits_escalation  (Ghidra: actor_command_list_permits_escalation, renamed)
// address 0x40d580, size 132 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: phase-4 summary "checks whether the scenario's scripted command-list settings
// permit the actor to escalate to a higher alert/combat status"; despite the name, the
// gating record it reads is actor.conversation_index -> ai_conversation_data ->
// ScenarioAIConversation.flags (types/tags.h ScenarioAIConversationFlags: stop_if_damaged
// = 2, stop_if_visible_enemy = 4, stop_if_alerted_to_enemy = 8), not a ScenarioCommandList;
// the phase-4 summary's "command list" is likely a misreading and should probably say
// "conversation".
// register convention: actor_index in EAX (Ghidra's in_EAX, no declared parameter).
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;             // 0x00880360
extern data_array *ai_conversation_data;   // 0x008802d4
extern Scenario *global_scenario;              // 0x00746f8c, the Scenario tag data

// blam-cc: EAX -> actor_index
uint8_t actor_command_list_permits_escalation(datum_index actor_index)
{
    actor *self;
    ai_conversation *conv;
    ScenarioAIConversation *definition;
    ScenarioAIConversationFlags flags;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->conversation_index == (datum_index)k_datum_index_none) {
        return 1;
    }

    conv = (ai_conversation *)((uint8_t *)ai_conversation_data->data +
                                (self->conversation_index & 0xffff) * sizeof(ai_conversation));
    definition = (ScenarioAIConversation *)((TagReflexive *)((uint8_t *)global_scenario + 0x468))->pointer;
    definition = definition + conv->definition_index;
    flags = definition->flags;

    if ((((flags & 2) == 0 || self->tally.by_threat_class[8] == 0) &&
         ((flags & 4) == 0 || self->target_combat_status < 9)) &&
        ((flags & 8) == 0 || self->target_combat_status < 6)) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x40d580):

int FUN_0040d580(void)

{
  ushort uVar1;
  uint uVar2;
  uint in_EAX;
  int iVar3;
  uint3 uVar4;

  iVar3 = (in_EAX & 0xffff) * 0x724;
  uVar2 = *(uint *)(iVar3 + 0x1dc + *(int *)(DAT_00880360 + 0x34));
  iVar3 = iVar3 + *(int *)(DAT_00880360 + 0x34);
  uVar4 = (uint3)((uint)iVar3 >> 8);
  if (uVar2 == 0xffffffff) {
    return CONCAT31(uVar4,1);
  }
  uVar1 = *(ushort *)
           (*(short *)((uVar2 & 0xffff) * 100 + 2 + *(int *)(DAT_008802d4 + 0x34)) * 0x74 +
            *(int *)(global_scenario + 0x46c) + 0x20);
  if (((((uVar1 & 2) == 0) || (*(char *)(iVar3 + 0x1f6) == '\0')) &&
      (((uVar1 & 4) == 0 || (*(short *)(iVar3 + 0x268) < 9)))) &&
     (((uVar1 & 8) == 0 || (*(short *)(iVar3 + 0x268) < 6)))) {
    return (uint)uVar4 << 8;
  }
  return CONCAT31(uVar4,1);
}
#endif
