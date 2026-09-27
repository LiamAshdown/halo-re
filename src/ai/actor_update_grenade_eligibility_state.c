// actor_update_grenade_eligibility_state  (Ghidra: actor_update_grenade_eligibility_state; named for this rewrite)
// address 0x42f370, size 261 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (verified against objdump 0x42f370..0x42f474)
// evidence: phase-4 summary ("periodically re-evaluates an actor's grenade-eligibility state
// and, once a countdown expires, queues a corresponding communication/behavior event").
// register convention: EAX -> actor_index (in_EAX, the only register Ghidra's own decompile
// shows).
// blam-cc: EAX -> actor_index
//
// UNSURE, substantially: the 0x30-byte local buffer this function builds and hands to
// ai_communication_target_result_reset (this batch's ai_communication_target_result_reset) is reproduced as raw
// bytes rather than a named struct, since only 8 of its bytes are ever explicitly written
// and ai_communication_target_result_reset itself immediately overwrites much of it; unit_animation_change_priority_check/unit_commit_speech's
// real argument shapes are not established (a second, differently-shaped call to
// unit_animation_change_priority_check already exists in this batch's ai_select_target-adjacent code at 0x42eee0).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data;     // 0x00880360
extern ai_globals *ai_globals_ptr; // 0x00880354

extern void actor_recompute_grenade_eligibility(datum_index actor_index); // 0x42f260, this batch
extern int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback,
    int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index,
    int32_t *chain_value); // 0x560d00, EAX, DL, stack
extern int32_t unit_commit_speech(uint32_t unit_index, const void *source, int16_t mode); // 0x560f20, EAX, ECX, DX
extern void ai_communication_target_result_reset(void *record); // 0x42d310, this batch

// blam-cc: EAX -> actor_index
// If the actor's awareness is above 1 and communication is valid, refreshes its cached
// grenade-eligibility whenever that cache is stale or has never been rolled, and counts down
// its recheck timer; when the timer reaches zero, attempts to queue a communication event
// reflecting the (possibly just-recomputed) eligibility state.
void actor_update_grenade_eligibility_state(datum_index actor_index)
{
    actor *self;
    uint8_t eligible;
    uint32_t buffer[12];
    int16_t out_a;
    int32_t out_b;
    int16_t result;

    self = &((actor *)actor_data->data)[actor_index & 0xffff];
    if (self->awareness_level <= 1 || !ai_globals_ptr->communication_valid) {
        return;
    }

    eligible = (self->awareness_level == 3 && self->unknown_72 < self->unknown_6e);

    if (self->grenade_recheck_ticks == 0 || self->grenade_eligible != eligible) {
        actor_recompute_grenade_eligibility(actor_index);
    }

    if (0 < self->grenade_recheck_ticks) {
        self->grenade_recheck_ticks = self->grenade_recheck_ticks - 1;
        if (self->grenade_recheck_ticks == 0) {
            // 0x42f3f9: the unit (actor +0x18) says line index "eligible" at priority 1
            out_a = (int16_t)(eligible != 0);
            out_b = -1;
            result = (int16_t)unit_animation_change_priority_check(self->unit_index, 1, 1, 0, 0, &out_a, &out_b);
            if (0 < result) {
                int i;
                for (i = 0; i < 12; i++) {
                    buffer[i] = 0;
                }
                *(int16_t *)((uint8_t *)buffer + 2) = (int16_t)out_a;
                *((uint32_t *)((uint8_t *)buffer + 4)) = out_b;
                *(int16_t *)buffer = 1;
                ai_communication_target_result_reset((uint8_t *)buffer + 0x10); // 0x42f44c: EAX = speech + 0x10
                unit_commit_speech(self->unit_index, buffer, result);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x42f370):

void FUN_0042f370(void)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  char cVar4;
  undefined4 *puVar5;
  uint local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;

  iVar2 = (in_EAX & 0xffff) * 0x724;
  iVar3 = iVar2 + *(int *)(DAT_00880360 + 0x34);
  if ((1 < *(short *)(iVar2 + 0x6a + *(int *)(DAT_00880360 + 0x34))) &&
     (*(char *)(DAT_00880354 + 0x10) != '\0')) {
    if ((*(short *)(iVar3 + 0x6a) == 3) && (*(short *)(iVar3 + 0x72) < *(short *)(iVar3 + 0x6e))) {
      cVar4 = '\x01';
    }
    else {
      cVar4 = '\0';
    }
    if ((*(short *)(iVar3 + 0x6ce) == 0) || (*(char *)(iVar3 + 0x6cc) != cVar4)) {
      FUN_0042f260();
    }
    if ((0 < *(short *)(iVar3 + 0x6ce)) &&
       (sVar1 = *(short *)(iVar3 + 0x6ce) + -1, *(short *)(iVar3 + 0x6ce) = sVar1, sVar1 == 0)) {
      local_38 = (uint)(cVar4 != '\0');
      local_34 = 0xffffffff;
      sVar1 = FUN_00560d00(1,0,0,&local_38,&local_34);
      if (0 < sVar1) {
        puVar5 = &local_30;
        for (iVar3 = 0xc; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar5 = 0;
          puVar5 = puVar5 + 1;
        }
        local_30._2_2_ = (undefined2)local_38;
        local_2c = local_34;
        local_30._0_2_ = 1;
        FUN_0042d310();
        FUN_00560f20();
      }
    }
  }
  return;
}
#endif
