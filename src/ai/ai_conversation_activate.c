// ai_conversation_activate  (Ghidra: ai_conversation_activate; named for this rewrite)
// address 0x4307c0, size 105 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/ai_types_notes.md's misattribution table: the 0x430830..0x431e70
// block operates on Scenario.ai_conversations (TagReflexive at Scenario+0x468), not squads;
// this function is immediately adjacent, reads that same field directly, and calls three
// functions from that block under their corrected names (ai_conversation_new,
// ai_conversation_stop, and ai_conversation_resolve_participants -- deferred in this batch, "a conversation-driven
// member mode change" per the same table). Renamed here from the phase-4 guess
// ("activates the squad associated with a starting-location index") accordingly.
// register convention: AX -> conversation_definition_index (in_AX, unresolved in Ghidra's
// own decompile); param_1 is a genuine stack argument.
// blam-cc: AX -> conversation_definition_index, stack -> allow_eviction

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario; // 0x00746f8c

extern datum_index ai_conversation_new(int16_t conversation_definition_index, uint8_t allow_eviction); // 0x431590, this batch
extern int8_t ai_conversation_resolve_participants(datum_index instance_index, uint8_t *out_flag); // 0x430fc0, deferred in this batch; UNSURE signature
extern void ai_conversation_stop(datum_index instance_handle, uint8_t reason_a, uint8_t reason_b); // 0x430ea0, this batch

// blam-cc: AX -> conversation_definition_index, stack -> allow_eviction
// Creates a new ai_conversation instance for conversation_definition_index (when it is a
// valid index into Scenario.ai_conversations) and immediately drives its first update; if
// that update reports the conversation is neither active nor ready, tears the instance back
// down. Returns 1 if the conversation is now running.
uint8_t ai_conversation_activate(int16_t conversation_definition_index, uint8_t allow_eviction)
{
    datum_index instance;
    uint8_t out_flag;
    int8_t active;

    if (conversation_definition_index < 0) {
        return 0;
    }
    if (conversation_definition_index < global_scenario->ai_conversations.count) {
        instance = ai_conversation_new(conversation_definition_index, allow_eviction);
        if (instance != (datum_index)k_datum_index_none) {
            out_flag = 0;
            active = ai_conversation_resolve_participants(instance, &out_flag);
            if (active != 0 || out_flag != 0) {
                return 1;
            }
            ai_conversation_stop(instance, 1, 0);
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4307c0):

undefined4 FUN_004307c0(char param_1)

{
  char cVar1;
  ushort in_AX;
  uint squad_instance_index;
  undefined4 uStack_4;

  if ((short)in_AX < 0) {
    return 0;
  }
  if ((int)(short)in_AX < *(int *)(global_scenario + 0x468)) {
    squad_instance_index = squad_new_instance(in_AX,param_1);
    if (squad_instance_index != 0xffffffff) {
      uStack_4 = uStack_4 & 0xffffff;
      cVar1 = FUN_00430fc0(squad_instance_index,(int)&uStack_4 + 3);
      if ((cVar1 != '\0') || (uStack_4._3_1_ != '\0')) {
        return 1;
      }
      squad_despawn(squad_instance_index,'\x01','\0');
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
