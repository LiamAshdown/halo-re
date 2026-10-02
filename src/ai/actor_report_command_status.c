// actor_report_command_status  (Ghidra: actor_report_command_status, renamed)
// address 0x4048b0, size 243 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop: objdump 0x4048b0..0x4049a2 incl. the jump table at 0x4049a4; prop offsets probed)
// evidence: types/ai.h actor.unit_index (0x18)/unknown_1e4 (0x1e4, the same command/category
//   field read by actor_build_order_search_wait.c); prop.object_index (0x18)/is_unit (0x60);
//   phase-4 summary "reports the actor's scripted command-list status as a chatter/status
//   event keyed by command index and target shield state, once per actor".
// register convention: actor index in EAX, the sole real parameter.
//   // blam-cc: EAX -> actor_index
// UNSURE: actor+0xa2 (a "reported already" byte) and actor+0xd8 (a datum, presumably a prop
//   index given the 0x138 stride below) both fall inside actor.mode_data.raw (a per-mode
//   union, see types/ai.h); not independently confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
// 0x42d340, not yet rewritten (this module). Always seven stack arguments: every call
// site in the binary cleans up 0x1c bytes, so the shorter forms Ghidra recovers at some
// sites are artefacts, not a reduced-arity overload.
extern uint8_t team_pair_flag_test(int16_t team_a, int16_t team_b); // 0x45bdb0, ECX, EDX

// If this actor has not already reported for its current scripted command, maps
// actor.unknown_1e4 (1..10, skipping 6) to a chatter event code and broadcasts it along with
// the actor's controlled unit and, if it has a tracked target prop, that prop's object and a
// shield-state code (2 = no shield, 3/4 = shield down/up per team_pair_flag_test). Marks the actor
// as having reported either way, except for command 6 (reported but nothing is broadcast).
int32_t actor_report_command_status(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t *already_reported = (uint8_t *)a->mode_data.raw + (0xa2 - 0x9c);
    uint32_t event_code = 0;

    if (*already_reported != 0) {
        return 0;
    }

    switch (a->post_combat_action) {
    case 1: event_code = 0x30; break;
    case 2: event_code = 0x31; break;
    case 3: event_code = 0x32; break;
    case 4: event_code = 0x33; break;
    case 5: event_code = 0x34; break;
    case 6:
        *already_reported = 1;
        return 0;
    case 7: event_code = 0x35; break;
    case 8: event_code = 0x36; break;
    case 9: event_code = 0x37; break;
    case 10: event_code = 0x38; break;
    default:
        *already_reported = 1;
        return event_code;
    }

    {
        datum_index target_prop_index = *(datum_index *)((uint8_t *)a->mode_data.raw + (0xd8 - 0x9c));
        datum_index target_object = (datum_index)k_datum_index_none;
        int32_t target_state = -1;

        if (target_prop_index != (datum_index)k_datum_index_none) {
            prop *p = &((prop *)prop_data->data)[target_prop_index & 0xffff];

            target_object = p->object_index;
            if (p->enemy == 0) {
                target_state = 2;
            } else {
                target_state = (team_pair_flag_test(((actor *)a)->team, ((struct prop *)p)->team) != 0) + 3; // 0x404962: ECX actor team, EDX prop team
            }
        }
        ai_communication_broadcast(event_code, a->unit_index, target_object, target_state, 0xffffffff, 0xffffffff, 0);
    }
    *already_reported = 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4048b0):

undefined4 FUN_004048b0(void)

{
  char cVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 uVar5;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar4 = 0;
  if (*(char *)(iVar2 + 0xa2) != '\0') {
    return 0;
  }
  switch(*(undefined2 *)(iVar2 + 0x1e4)) {
  case 1:
    uVar4 = 0x30;
    break;
  case 2:
    uVar4 = 0x31;
    break;
  case 3:
    uVar4 = 0x32;
    break;
  case 4:
    uVar4 = 0x33;
    break;
  case 5:
    uVar4 = 0x34;
    break;
  case 6:
    goto switchD_004048eb_caseD_6;
  case 7:
    uVar4 = 0x35;
    break;
  case 8:
    uVar4 = 0x36;
    break;
  case 9:
    uVar4 = 0x37;
    break;
  case 10:
    uVar4 = 0x38;
    break;
  default:
switchD_004048eb_default:
    *(undefined1 *)(iVar2 + 0xa2) = 1;
    return uVar4;
  }
  uVar5 = 0xffffffff;
  iVar3 = -1;
  if (*(uint *)(iVar2 + 0xd8) != 0xffffffff) {
    iVar3 = (*(uint *)(iVar2 + 0xd8) & 0xffff) * 0x138;
    uVar5 = *(undefined4 *)(iVar3 + *(int *)(DAT_008802c0 + 0x34) + 0x18);
    if (*(char *)(iVar3 + 0x60 + *(int *)(DAT_008802c0 + 0x34)) == '\0') {
      iVar3 = 2;
    }
    else {
      cVar1 = FUN_0045bdb0();
      iVar3 = (cVar1 != '\0') + 3;
    }
  }
  ai_communication_broadcast
            (uVar4,*(undefined4 *)(iVar2 + 0x18),uVar5,iVar3,0xffffffff,0xffffffff,0);
  uVar4 = 1;
switchD_004048eb_caseD_6:
  goto switchD_004048eb_default;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
