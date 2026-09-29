// hs_evaluate_sleep  (Ghidra: hs_evaluate_sleep, already named)
// address 0x489800, size 542 bytes
// name confidence: 0.5 (out/phase4/hs_functions.md: "Implements the HS 'sleep'/'sleep_until'
//   special form, computing the game tick at which the calling thread should resume")
// rewrite confidence: 0.9
// evidence: types/hs.h hs_sleep_state (condition 0x00, ticks 0x04, timeout_ticks 0x08,
//   start_tick 0x0c, stage 0x10; scratch bumps 4+4+4+4+2 match exactly).
// register convention: unused opcode-ish param_1 (never read); thread index as the recognized
//   stack parameter (param_2); `first` as param_3.
// FIXED (re-derived from 0x489800..0x489a19). The child walk is
//   child0 = node->data.first_child   (the function-name node)
//   child1 = child0->next_node        (sleep_until's CONDITION / sleep's tick count)
//   child2 = child1->next_node        (the tick count / -1 for a two-element form)
//   child3 = child2->next_node        (the optional timeout)
// and each of the three hs_thread_push calls writes into a DIFFERENT scratch slot, which is what
// the earlier rewrite lost (it passed frame->result_address to all of them, and a placeholder
// node to the last):
//   0x489920  push child2 -> the `ticks` slot   (EBX still holds slot 2 from 0x489851)
//   0x489960  push child3 -> `timeout_ticks`    (`mov ebx,eax`, eax = slot 3 from 0x48992d)
//   0x4899ba  push child1 -> `condition`        (`mov ebx,[esp+0x14]` at 0x489973)
// So the handler evaluates its arguments one per resumption -- ticks, then timeout, then the
// condition on every wake -- rather than evaluating one node twice.
// VERIFIED: the finish path is a tail call `xor eax,eax / jmp 0x48a640` (0x489a13), so
// hs_thread_return(0, thread_index) was right, not a placeholder.
// VERIFIED: the child walk has no k_datum_index_none guards on child0/child1 -- retail
// dereferences both unconditionally, which is safe only because the compiler guarantees sleep and
// sleep_until always have at least one argument.
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "hs.h"
#include "fn_hs.h"


    // this module, 0x48a560; see hs_evaluate_random.c and the UNSURE note above


extern data_array *hs_thread_data; // 0x0087a470
extern data_array *hs_syntax_data; // 0x0087a474

// game_time_globals: defined in types/game.h (R32 replaced hs.h's partial game_time_globals)
extern game_time_globals *game_time; // 0x006f1d6c

// Evaluate handler shared by 'sleep' and 'sleep_until'. See the UNSURE note above: the overall
// shape (seed a default 30-tick sleep on the first call, then keep re-arming the thread's
// wake_tick while a condition holds and no timeout has elapsed, otherwise finish) is preserved,
// but one call's arguments could not be recovered.
void hs_evaluate_sleep(uint32_t unused_param_1, uint32_t thread_index, char first)
{
    hs_thread *thread_record;
    hs_stack_frame *frame;
    hs_syntax_node *node;
    char *condition;
    int16_t *ticks;
    int32_t *timeout_ticks;
    int32_t *start_tick;
    int16_t *stage;
    datum_index name_node;
    datum_index condition_node;
    datum_index ticks_node;
    datum_index timeout_node;
    int32_t ticks_value;
    int32_t wake_tick;
    int32_t capped;

    thread_record = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_index & 0xffff) * 0x218);
    frame = thread_record->stack;
    condition = (char *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    ticks = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    timeout_ticks = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    start_tick = (int32_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 4;

    frame = thread_record->stack;
    stage = (int16_t *)((uint8_t *)frame + 0x0e + frame->size);
    frame->size = frame->size + 2;

    frame = thread_record->stack;
    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (frame->syntax_node & 0xffff) * 0x14);
    /* no none-guards here: retail walks all three links unconditionally (0x4898c4..0x4898e4) */
    name_node = node->data.first_child;
    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (name_node & 0xffff) * 0x14);
    condition_node = node->next_node;
    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (condition_node & 0xffff) * 0x14);
    ticks_node = node->next_node;

    if (first != 0) {
        *condition = 0;
        *start_tick = game_time->game_time;
        *stage = 0;
        *ticks = 0x1e;
        *timeout_ticks = -1;
        if (ticks_node != k_datum_index_none) {
            hs_thread_push(ticks_node, thread_index, ticks);
            return;
        }
    }

    if (*stage == 0) {
        *stage = 1;
        if (ticks_node != k_datum_index_none) {
            node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
                (ticks_node & 0xffff) * 0x14);
            timeout_node = node->next_node;
            if (timeout_node != k_datum_index_none) {
                hs_thread_push(timeout_node, thread_index, timeout_ticks);
                return;
            }
        }
    } else if (*stage != 1) {
        return;
    }

    if (*condition == 0 &&
        (*timeout_ticks == -1 || game_time->game_time < *start_tick + *timeout_ticks)) {
        /* re-evaluate the condition into the condition slot on every wake */
        hs_thread_push(condition_node, thread_index, condition);
        ticks_value = *ticks;
        if (ticks_value < 1) {
            ticks_value = 1;
        }
        wake_tick = ticks_value + game_time->game_time;
        thread_record->wake_tick = wake_tick;
        if (*timeout_ticks == -1) {
            return;
        }
        capped = *timeout_ticks + *start_tick;
        if (capped <= wake_tick) {
            wake_tick = capped;
        }
        thread_record->wake_tick = wake_tick;
        return;
    }
    hs_thread_return(0, thread_index); // tail call at 0x489a19
}

#if 0
Original Ghidra decompilation (0x489800):

void hs_evaluate_sleep(undefined4 param_1,uint param_2,char param_3)

{
  int iVar1;
  char *pcVar2;
  short *psVar3;
  int *piVar4;
  int *piVar5;
  short *psVar6;
  short sVar7;
  uint uVar8;
  int iVar9;
  int iVar10;
  int iVar11;

  iVar11 = DAT_0087a470;
  iVar9 = (param_2 & 0xffff) * 0x218;
  iVar1 = *(int *)(DAT_0087a470 + 0x34) + iVar9;
  iVar10 = *(int *)(iVar1 + 0x10);
  pcVar2 = (char *)(*(short *)(iVar10 + 0xc) + 0xe + iVar10);
  *(short *)(iVar10 + 0xc) = *(short *)(iVar10 + 0xc) + 4;
  iVar10 = *(int *)(*(int *)(iVar11 + 0x34) + 0x10 + iVar9);
  psVar3 = (short *)(*(short *)(iVar10 + 0xc) + 0xe + iVar10);
  *(short *)(iVar10 + 0xc) = *(short *)(iVar10 + 0xc) + 4;
  iVar10 = *(int *)(*(int *)(iVar11 + 0x34) + 0x10 + iVar9);
  piVar4 = (int *)(*(short *)(iVar10 + 0xc) + 0xe + iVar10);
  *(short *)(iVar10 + 0xc) = *(short *)(iVar10 + 0xc) + 4;
  iVar10 = *(int *)(*(int *)(iVar11 + 0x34) + 0x10 + iVar9);
  piVar5 = (int *)(*(short *)(iVar10 + 0xc) + 0xe + iVar10);
  *(short *)(iVar10 + 0xc) = *(short *)(iVar10 + 0xc) + 4;
  iVar10 = *(int *)(*(int *)(iVar11 + 0x34) + 0x10 + iVar9);
  psVar6 = (short *)(*(short *)(iVar10 + 0xc) + 0xe + iVar10);
  *(short *)(iVar10 + 0xc) = *(short *)(iVar10 + 0xc) + 2;
  iVar11 = DAT_0087a474;
  iVar10 = *(int *)(DAT_0087a474 + 0x34);
  uVar8 = *(uint *)(iVar10 + 8 +
                   (*(uint *)(iVar10 + 8 +
                             (*(uint *)(iVar10 + 0x10 +
                                       (*(uint *)(*(int *)(iVar1 + 0x10) + 4) & 0xffff) * 0x14) &
                             0xffff) * 0x14) & 0xffff) * 0x14);
  if (param_3 != '\0') {
    *pcVar2 = '\0';
    *piVar5 = *(int *)(DAT_006f1d6c + 0xc);
    *psVar6 = 0;
    *psVar3 = 0x1e;
    *piVar4 = -1;
    if (uVar8 != 0xffffffff) {
      FUN_0048a560();
      return;
    }
  }
  if (*psVar6 == 0) {
    *psVar6 = 1;
    if ((uVar8 != 0xffffffff) &&
       (*(int *)(*(int *)(iVar11 + 0x34) + 8 + (uVar8 & 0xffff) * 0x14) != -1)) {
      FUN_0048a560();
      return;
    }
  }
  else if (*psVar6 != 1) {
    return;
  }
  if ((*pcVar2 == '\0') && ((*piVar4 == -1 || (*(int *)(DAT_006f1d6c + 0xc) < *piVar5 + *piVar4))))
  {
    FUN_0048a560();
    sVar7 = *psVar3;
    if (sVar7 < 1) {
      iVar10 = 1;
    }
    else {
      iVar10 = (int)sVar7;
    }
    iVar10 = iVar10 + *(int *)(DAT_006f1d6c + 0xc);
    *(int *)(iVar1 + 8) = iVar10;
    if (*piVar4 == -1) {
      return;
    }
    iVar11 = *piVar4 + *piVar5;
    if (iVar11 <= iVar10) {
      iVar10 = iVar11;
    }
    *(int *)(iVar1 + 8) = iVar10;
    return;
  }
  FUN_0048a640();
  return;
}
#endif
