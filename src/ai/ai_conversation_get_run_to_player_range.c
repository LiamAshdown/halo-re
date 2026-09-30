// ai_conversation_get_run_to_player_range  (Ghidra: ai_conversation_get_run_to_player_range, renamed)
// address 0x402cf0, size 128 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/ai.h ai_conversation.definition_index/unknown_10, global
//   0x008802d4 ai_conversation_data (stride 0x64, matching this function's *100 indexing);
//   types/tags.h Scenario.ai_conversations (TagReflexive at 0x468, pointer at 0x46c) and
//   ScenarioAIConversation.run_to_player_dist (offset 0x28, stride 0x74).
// MISATTRIBUTION NOTE: out/phase4/ai_functions.md describes this as reading "an actor-type
// definition table entry for a given command index", but the globals it actually touches
// (ai_conversation_data, Scenario.ai_conversations) put it squarely in the ai-conversation
// system documented in out/phase4/ai_types_notes.md's "misattributed functions" section --
// that section just did not happen to include this address. Rewritten here per what the
// code actually does, not the phase-4 summary.
// register convention: output record pointer in EDX, ai_conversation index in ESI.
//   // blam-cc: EDX -> out, ESI -> conversation_index
// TYPES (folded into types/ai.h by the review pass): the 0x14-byte output record is not defined anywhere in types/ai.h. Modelled
// locally as ai_conversation_range_lookup.
// UNSURE: the original return value is a 4-byte value whose low byte is always 1 and whose
// upper 3 bytes are leftover NaN/sign/zero classification bits from the fVar1 comparison
// (an x87/SSE flags artifact folded into the return register by the decompiler, not a real
// multi-value return). Modelled here as a plain success return of 1.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// TYPES (folded into types/ai.h by the review pass): see note above.

extern data_array *ai_conversation_data; // 0x008802d4
extern Scenario *global_scenario;        // 0x00746f8c

// Looks up the ScenarioAIConversation definition behind a live ai_conversation instance and
// reports its run_to_player_dist range (0.0 meaning the conversation does not use one) along
// with the instance's own unknown_10. Always reports success.
int32_t ai_conversation_get_run_to_player_range(ai_conversation_range_lookup *out, uint32_t conversation_index)
{
    ai_conversation *conv = &((ai_conversation *)ai_conversation_data->data)[conversation_index & 0xffff];
    ScenarioAIConversation *conversations = (ScenarioAIConversation *)global_scenario->ai_conversations.pointer;
    ScenarioAIConversation *def = &conversations[conv->definition_index];
    float distance = def->run_to_player_dist;

    out->conversation_index = 0;
    out->unknown_04 = 0;
    out->run_to_player_dist = 0.0f;
    out->player_unit_index = 0;
    out->unknown_10 = 0;

    out->conversation_index = conversation_index;
    out->run_to_player_dist = distance;
    if (distance == 0.0f) {
        out->player_unit_index = -1;
        out->unknown_10 = 0xffffffff;
        return 1;
    }
    out->player_unit_index = conv->player_unit_index;
    out->unknown_10 = 0xffffffff;
    return 1;
}

#if 0
Original Ghidra decompilation (0x402cf0):

undefined4 FUN_00402cf0(void)

{
  float fVar1;
  int iVar2;
  int iVar3;
  undefined3 uVar4;
  int iVar5;
  uint *in_EDX;
  uint unaff_ESI;

  iVar5 = (unaff_ESI & 0xffff) * 100;
  iVar2 = *(int *)(DAT_008802d4 + 0x34);
  iVar3 = *(short *)(iVar5 + 2 + iVar2) * 0x74 + *(int *)(global_scenario + 0x46c);
  *in_EDX = 0;
  in_EDX[1] = 0;
  in_EDX[2] = 0;
  in_EDX[3] = 0;
  in_EDX[4] = 0;
  *in_EDX = unaff_ESI;
  fVar1 = *(float *)(iVar3 + 0x28);
  in_EDX[2] = (uint)fVar1;
  uVar4 = (undefined3)
          (CONCAT22((short)((uint)iVar3 >> 0x10),
                    (ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 |
                    (ushort)(fVar1 == 0.0) << 0xe) >> 8);
  if (fVar1 == 0.0) {
    in_EDX[3] = 0xffffffff;
    in_EDX[4] = 0xffffffff;
    *(undefined1 *)((int)in_EDX + 5) = 0;
    return CONCAT31(uVar4,1);
  }
  in_EDX[3] = *(uint *)(iVar5 + iVar2 + 0x10);
  in_EDX[4] = 0xffffffff;
  *(undefined1 *)((int)in_EDX + 5) = 0;
  return CONCAT31(uVar4,1);
}
#endif
