// actor_update_path_if_needed  (Ghidra: actor_update_path_if_needed, renamed)
// address 0x4017b0, size 156 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// REWRITTEN from objdump 0x4017b0..0x40184b: the "avoid" mode's +0x0c process. While the actor wants a path (+0x4c)
//   it picks a firing position with goal kind 6 (actor_select_firing_position: EBX query, EDI candidate, stack
//   actor, &previous owner, path context, &path ok) and claims it (actor_claim_firing_position: CX index, AL path
//   ok). Returns whether the danger (+0x280) is gone.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>

extern data_array *actor_data; // 0x00880360

extern int16_t actor_select_firing_position(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context,
    uint8_t *out_path_ok); // 0x413e50, stack, EBX query, EDI candidate
extern int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner,
    path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok); // 0x414060, stack, CX, AL

uint8_t actor_update_path_if_needed(datum_index actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    if (actor[0x4c] != 0) {
        static actor_firing_position_query query;
        static path_find_context path_context;
        actor_firing_position_candidate candidate;
        uint32_t previous_owner = 0xffffffff;
        uint8_t path_ok = 0;
        int16_t selected;

        memset(&query, 0, sizeof(query));
        memset(&candidate, 0, sizeof(candidate));
        query.goal_kind = 6;
        selected = actor_select_firing_position(actor_index, &query, &candidate, &previous_owner, &path_context,
            &path_ok);
        actor_claim_firing_position(actor_index, previous_owner, &path_context, selected, path_ok);
        actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    }
    return *(int16_t *)(actor + 0x280) == 0;
}

#if 0
Original Ghidra decompilation (0x4017b0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

bool FUN_004017b0(uint param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined1 uStack_10741;
  undefined4 auStack_10740 [16];
  undefined4 uStack_10700;
  undefined2 uStack_106fc;
  undefined1 auStack_10098 [65684];

  iVar1 = *(int *)(DAT_00880360 + 0x34);
  iVar3 = (param_1 & 0xffff) * 0x724;
  if (*(char *)(iVar3 + 0x4c + iVar1) != '\0') {
    puVar4 = &uStack_10700;
    for (iVar2 = 0x199; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    uStack_106fc = 6;
    FUN_00413e50(param_1,auStack_10740,auStack_10098,&uStack_10741);
    FUN_00414060(param_1,auStack_10740[0],auStack_10098);
  }
  return *(short *)(iVar3 + iVar1 + 0x280) == 0;
}
#endif
