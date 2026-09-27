// actor_squad_action_status_broadcast  (Ghidra: actor_squad_action_status_broadcast, renamed)
// address 0x407140, size 249 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: types/ai.h actor.swarm (0x06)/unknown_28 (swarm_index); types/tags.h
//   Scenario.command_lists (count at 0x438, pointer at 0x43c), ScenarioCommandList.flags
//   (already-named bitfield: allow_initiative, allow_targeting, disable_looking,
//   disable_communication, ...); calls actor_clear_vocalization and this session's
//   actor_swarm_for_each_component. phase-4 summary "records a squad member's updated
//   status bits and broadcasts the change to the rest of the squad" (again this session's
//   swarm/squad terminology mismatch: the broadcast is via swarm_for_each_component).
// UNSURE: the byte-shuffling around a temporary the original calls uStack_4 is, after
// working through the CONCAT operations by hand, equivalent to the plain bitfield reads
// below (both branches of the `if (allow_look == 0)` produce the same value for the
// "allow_communication" byte); simplified here rather than reproduced literally.
//   UNSURE: the callback address (`&LAB_00406dd0`) is an internal jump-table-style label
//   inside this function's own compiled range, not a function Ghidra's analysis recognized
//   (`python tools/pack.py 0x406dd0` reports no function there). Its real behaviour is
//   unknown; declared as an opaque extern callback matching actor_swarm_for_each_component's
//   callback signature.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include <stdint.h>

extern data_array *actor_data;    // 0x00880360
extern Scenario *global_scenario; // 0x00746f8c
extern uint16_t global_structure_bsp_index; // 0x0069e8d8, UNSURE: compared against ScenarioCommandList.precomputed_bsp_index

extern void actor_swarm_for_each_component(uint32_t actor_index, char reset_first, actor_swarm_member_callback callback, uint32_t callback_extra, uint16_t *caller_record); // 0x407040, this session
extern void actor_clear_vocalization(uint32_t actor_index); // 0x414560
extern void actor_command_list_reset_record(uint32_t actor_index, datum_index unit_index, uint16_t extra, void *component_record, int32_t secondary_record, uint32_t callback_extra); // 0x406dd0

// FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original never reads EAX; actor_index arrive(s) on the stack (2 stack argument(s)).
// blam-cc: ESI -> record, stack -> actor_index, command_list_index
int32_t actor_squad_action_status_broadcast(uint32_t actor_index, int16_t command_list_index, int16_t *record)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    int16_t *zero_cursor = record;
    int32_t i;

    for (i = 0x21; i != 0; i--) {
        zero_cursor[0] = 0;
        zero_cursor[1] = 0;
        zero_cursor += 2;
    }

    if (command_list_index < 0 || command_list_index >= (int32_t)global_scenario->command_lists.count) {
        return 0;
    }

    {
        ScenarioCommandList *lists = (ScenarioCommandList *)global_scenario->command_lists.pointer;
        ScenarioCommandList *list = &lists[command_list_index];

        if (a->swarm == 0 || a->swarm_index != (datum_index)k_datum_index_none) {
            if (list->precomputed_bsp_index == 0xffff || list->precomputed_bsp_index == global_structure_bsp_index) {
                uint8_t allow_initiative = (uint8_t)(list->flags & 1);
                uint8_t allow_look = (uint8_t)(~(list->flags >> 2)) & 1;
                uint8_t allow_communication = (uint8_t)(~(list->flags >> 3)) & 1;

                record[0] = command_list_index;
                if (allow_look == 0) {
                    actor_clear_vocalization(actor_index);
                }
                *((uint8_t *)record + 4) = allow_communication;
                *((uint8_t *)record + 2) = allow_initiative;
                *((uint8_t *)record + 3) = allow_look;
                // 0x4071cc..0x40722a: the callback's extra is a local byte holding flag bit 1, not a record field
                uint8_t flag_bit_1 = (uint8_t)((list->flags >> 1) & 1);

                actor_swarm_for_each_component(actor_index, 1, actor_command_list_reset_record,
                    (uint32_t)(uintptr_t)&flag_bit_1, (uint16_t *)record);
                return 1;
            }
        } else if (a->active == 0) {
            a->unknown_90 = command_list_index;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x407140):

undefined4 FUN_00407140(uint param_1,short param_2)

{
  byte bVar1;
  uint uVar2;
  byte bVar3;
  int iVar4;
  undefined3 in_ECX;
  int iVar5;
  int iVar6;
  byte bVar7;
  short *unaff_ESI;
  short *psVar8;
  undefined4 uStack_4;

  iVar4 = global_scenario;
  iVar6 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  psVar8 = unaff_ESI;
  for (iVar5 = 0x21; iVar5 != 0; iVar5 = iVar5 + -1) {
    psVar8[0] = 0;
    psVar8[1] = 0;
    psVar8 = psVar8 + 2;
  }
  if ((-1 < param_2) && ((int)param_2 < *(int *)(iVar4 + 0x438))) {
    iVar4 = param_2 * 0x60 + *(int *)(iVar4 + 0x43c);
    if ((*(char *)(iVar6 + 6) == '\0') || (*(int *)(iVar6 + 0x28) != -1)) {
      if ((*(short *)(iVar4 + 0x2e) == -1) || (*(short *)(iVar4 + 0x2e) == DAT_0069e8d8)) {
        *unaff_ESI = param_2;
        bVar1 = *(byte *)(iVar4 + 0x20);
        uVar2 = *(uint *)(iVar4 + 0x20);
        bVar3 = ~(byte)(uVar2 >> 3);
        bVar7 = ~(byte)(uVar2 >> 2) & 1;
        uStack_4 = CONCAT13((char)(uVar2 >> 1),in_ECX) & 0x1ffffff;
        uStack_4 = CONCAT13(uStack_4._3_1_,CONCAT12(bVar3,(undefined2)uStack_4)) & 0xff01ffff;
        bVar3 = bVar3 & 1;
        if (bVar7 == 0) {
          actor_clear_vocalization();
          bVar3 = uStack_4._2_1_;
        }
        *(byte *)(unaff_ESI + 2) = bVar3;
        *(byte *)(unaff_ESI + 1) = bVar1 & 1;
        *(byte *)((int)unaff_ESI + 3) = bVar7;
        FUN_00407040(param_1,1,&LAB_00406dd0,(int)&uStack_4 + 3);
        return 1;
      }
    }
    else if (*(char *)(iVar6 + 8) == '\0') {
      *(short *)(iVar6 + 0x90) = param_2;
    }
  }
  return 0;
}
#endif
