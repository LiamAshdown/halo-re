// game_engine_dispatch_end_game_notification  (Ghidra: FUN_00467230; named per
// out/phase4/game_functions.md: "Validates and dispatches an incoming countdown/notification
// message to one of its three stage handlers.")
// address 0x467230, size 60 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x467230
//   --stop-address=0x467270), which shows message_delta_decode_compound_field is called with ECX pointing at a stack
//   copy of the incoming `stage` value (not with zero arguments as Ghidra's decompile shows) --
//   matching message_delta_decode_compound_field's other call site in this batch (game_engine_apply_player_profile_
//   entry.c), and that the dispatch afterward reads that same stack slot back, i.e. message_delta_decode_compound_field
//   may rewrite `stage` in place before the 1/2/3 dispatch runs. Stages 1/2/3 are this batch's
//   game_engine_end_game_sequence_stage1/2/3 (0x4670c0/0x4670f0/0x467180).
// register convention: `event` in EAX, `stage` in ECX (both callee-saved across the call, per
//   the disassembly's own push/pop ecx pair).
//   // blam-cc: EAX -> event, ECX -> stage
// UNSURE: `event`'s shape beyond one dereference; message_delta_decode_compound_field's and message_delta_decode_compound_field_staged's true
//   purpose (both outside this batch).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void game_engine_end_game_sequence_stage1(void); // 0x4670c0, this batch
extern void game_engine_end_game_sequence_stage2(void); // 0x4670f0, this batch
extern void game_engine_end_game_sequence_stage3(void); // 0x467180, this batch

extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590, blam-cc:
    // EAX -> event, ECX -> out_values; UNSURE identity (a network message-delta decode)
extern void message_delta_decode_compound_field_staged(void *event); // 0x4ec670, blam-cc: EAX -> event; UNSURE identity

// blam-cc: EAX -> event, ECX -> stage
// If `event` validates (a foreign check via message_delta_decode_compound_field, which may also rewrite `stage`),
// dispatches to the matching end-of-game countdown stage handler (1, 2 or 3); otherwise forwards
// `event` to message_delta_decode_compound_field_staged. UNSURE: see header.
// FIXED 2026-09-28 (networking call audit): the stage is the decoded value (a stack slot, 0x467230 push ecx), not
// an ECX argument -- the dispatcher's ECX there is only its own scratch.
void game_engine_dispatch_end_game_notification(void *event)
{
    int32_t stage;

    if (**(int32_t **)event != 0) {
        message_delta_decode_compound_field_staged(event);
        return;
    }

    if (message_delta_decode_compound_field(event, &stage) == 0) {
        return;
    }

    if (stage == 1) {
        game_engine_end_game_sequence_stage1();
    } else if (stage == 2) {
        game_engine_end_game_sequence_stage2();
    } else if (stage == 3) {
        game_engine_end_game_sequence_stage3();
    }
}

#if 0
Original Ghidra decompilation (0x467230), from tools/pack.py 0x467230:

void FUN_00467230(void)

{
  char cVar1;
  undefined4 *in_EAX;
  int in_ECX;

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if (cVar1 != '\0') {
      if (in_ECX == 1) {
        FUN_004670c0();
        return;
      }
      if (in_ECX == 2) {
        FUN_004670f0();
        return;
      }
      if (in_ECX == 3) {
        FUN_00467180();
        return;
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}

Raw disassembly (objdump -d -M intel --start-address=0x467230 --stop-address=0x467270):

00467230:  push ecx
00467231:  mov ecx,[eax]
00467233:  cmp dword ptr [ecx],0x0
00467236:  jne 0x467265
00467238:  lea ecx,[esp]
0046723b:  call 0x4ec590            ; FUN_004ec590(EAX=event, ECX=&stage_copy)
00467240:  test al,al
00467242:  je 0x46726a
00467244:  mov eax,[esp]            ; re-read stage_copy (possibly rewritten by FUN_004ec590)
00467247:  dec eax
00467248:  je 0x46725e              ; stage == 1
0046724a:  dec eax
0046724b:  je 0x467257              ; stage == 2
0046724d:  dec eax
0046724e:  jne 0x46726a
00467250:  call 0x467180            ; stage == 3
...
00467265:  call 0x4ec670            ; FUN_004ec670(EAX=event)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
