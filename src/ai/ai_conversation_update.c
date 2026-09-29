// ai_conversation_update  (Ghidra: ai_conversation_update; renamed per ai_types_notes.md)
// address 0x430a70, size 490 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: out/phase4/ai_types_notes.md's misattribution table: "the per-tick conversation
// update", not "per-tick update of every squad instance". squad_despawn renamed to
// ai_conversation_stop, and ai_conversation_activate_next_participant/ai_conversation_current_line_is_ready keep their corrected participant-
// activation/readiness roles from the same table (both outside this batch's address range).
// register convention: plain __cdecl, no parameters (drains the ai_conversation data_array).
// blam-cc: (no arguments)
//
// UNSURE, substantially: the instance handle Ghidra's own decompile calls "squad_instance_index"
// is initialized to -1 and never visibly reassigned before use, which would make every call
// into it a no-op; the data_iterator's own `index` field (the handle of the instance the loop
// is currently on) is used here instead, matching the same pattern already resolved in
// ai_conversation_stop_all.c. The per-participant leader/reference resolution in the middle
// of the loop (actor.conversation_participant selection against the line's two indices and
// its flag bits) is reproduced structurally but not independently verified.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "ai.h"
#include "fn_ai.h"
#include <stdint.h>

extern game_time_globals *game_time; // 0x006f1d6c
extern data_array *ai_conversation_data; // 0x008802d4
extern Scenario *global_scenario;    // 0x00746f8c
extern data_array *actor_data;       // 0x00880360

extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0, EDI

extern int8_t ai_conversation_resolve_participants(datum_index instance_index, uint8_t *out_flag); // 0x430fc0


// REWRITTEN from objdump 0x430a70..0x430c5f. Per live conversation (definition: Scenario +0x46c, 0x74 each):
//   - not yet active (+6): every 30 ticks since +0xc re-resolve its participants; stop (1, 0) when that fails;
//   - active and not finished (+7): while the current line (+0x48) is ready, advance and activate the next
//     participant (whose result says whether the new line must be waited on); past the last line mark it finished;
//   - finished: stop (0, 1);
//   - still active: every masked participant actor (+0x14 bits, +0x28 actors) gets the conversation (+0x1dc) and its
//     partner (+0x1e0) from the two units +0x54 / +0x58 and the flags +0x4e.
//   The draft called the line helpers without the conversation, never stopped a finished one and only assigned
//   participants once finished.
void ai_conversation_update(void)
{
    int32_t now = game_time->game_time;
    data_iterator iterator;
    uint8_t *inst;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    for (inst = (uint8_t *)data_iterator_next(&iterator); inst != 0; inst = (uint8_t *)data_iterator_next(&iterator)) {
        datum_index handle = iterator.index;                                             // ebx
        uint8_t *definition = *(uint8_t **)((uint8_t *)global_scenario + 0x46c) + *(int16_t *)(inst + 0x2) * 0x74;
        int32_t line_count = *(int32_t *)(definition + 0x5c);

        if (!inst[0x6]) {
            uint8_t ok = 1;                                                              // [esp+0x13]

            if ((now - *(int32_t *)(inst + 0xc)) % 0x1e == 0) {
                ai_conversation_resolve_participants(handle, &ok);
            }
            if (!inst[0x6]) {
                if (!ok) {
                    ai_conversation_stop(handle, 1, 0);
                }
                if (!inst[0x6]) {
                    goto finished_check;
                }
            }
        }
        if (!inst[0x7]) {
            int16_t line = *(int16_t *)(inst + 0x48);
            uint8_t pending = (line >= 0 && (int32_t)line < line_count);

            for (;;) {
                if (pending && !ai_conversation_current_line_is_ready(handle)) {
                    break;
                }
                *(int16_t *)(inst + 0x48) = (int16_t)(*(int16_t *)(inst + 0x48) + 1);
                if ((int32_t)*(int16_t *)(inst + 0x48) >= line_count) {
                    inst[0x7] = 1;
                    break;
                }
                pending = ai_conversation_activate_next_participant(handle);
            }
        }
finished_check:                                                                          // 0x430b79
        if (inst[0x7]) {
            ai_conversation_stop(handle, 0, 1);
            continue;
        }
        if (inst[0x6]) {
            int16_t i;

            for (i = 0; (int32_t)i < *(int32_t *)(definition + 0x50); i++) {
                datum_index actor_index = *(datum_index *)(inst + 0x28 + i * 4);
                uint8_t *a;
                datum_index unit;
                uint16_t flags = *(uint16_t *)(inst + 0x4e);

                if (!(*(uint32_t *)(inst + 0x14) & (1u << i)) || actor_index == k_datum_index_none) {
                    continue;
                }
                a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
                unit = ((actor *)a)->unit_index;
                ((actor *)a)->conversation_index = handle;
                ((actor *)a)->conversation_participant = k_datum_index_none;
                if (unit == *(datum_index *)(inst + 0x54)) {
                    ((actor *)a)->conversation_participant = *(datum_index *)(inst + 0x58);
                } else if (unit == *(datum_index *)(inst + 0x58) && (flags & 1)) {
                    ((actor *)a)->conversation_participant = *(datum_index *)(inst + 0x54);
                } else if (flags & 2) {
                    ((actor *)a)->conversation_participant = *(datum_index *)(inst + 0x54);
                } else if (flags & 4) {
                    ((actor *)a)->conversation_participant = *(datum_index *)(inst + 0x58);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x430a70):

void FUN_00430a70(void)

{
  uint uVar1;
  int iVar2;
  uint squad_instance_index;
  char cVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  char local_19;
  int local_18;
  int local_14;
  uint local_10;
  undefined2 local_c;
  uint local_8;
  uint local_4;

  local_14 = *(int *)(DAT_006f1d6c + 0xc);
  local_10 = DAT_008802d4;
  local_4 = DAT_008802d4 ^ 0x69746572;
  local_c = 0;
  local_8 = 0xffffffff;
  iVar4 = data_iterator_next();
  squad_instance_index = local_8;
  do {
    if (iVar4 == 0) {
      return;
    }
    iVar7 = *(short *)(iVar4 + 2) * 0x74 + *(int *)(global_scenario + 0x46c);
    local_8 = squad_instance_index;
    if (*(char *)(iVar4 + 6) == '\0') {
      local_19 = '\x01';
      if ((local_14 - *(int *)(iVar4 + 0xc)) % 0x1e == 0) {
        FUN_00430fc0(squad_instance_index,&local_19);
      }
      if (*(char *)(iVar4 + 6) != '\0') goto LAB_00430b2b;
      if (local_19 == '\0') {
        squad_despawn(squad_instance_index,'\x01','\0');
      }
      if (*(char *)(iVar4 + 6) != '\0') goto LAB_00430b2b;
LAB_00430b79:
      if (*(char *)(iVar4 + 7) != '\0') goto LAB_00430b80;
      if (*(char *)(iVar4 + 6) != '\0') {
        iVar6 = 0;
        local_18 = 0;
        if (0 < *(int *)(iVar7 + 0x50)) {
          do {
            if (((*(uint *)(iVar4 + 0x14) & 1 << ((byte)iVar6 & 0x1f)) != 0) &&
               (uVar1 = *(uint *)(iVar4 + 0x28 + iVar6 * 4), uVar1 != 0xffffffff)) {
              iVar5 = (uVar1 & 0xffff) * 0x724;
              iVar6 = *(int *)(iVar5 + 0x18 + *(int *)(DAT_00880360 + 0x34));
              iVar5 = iVar5 + *(int *)(DAT_00880360 + 0x34);
              *(uint *)(iVar5 + 0x1dc) = squad_instance_index;
              *(undefined4 *)(iVar5 + 0x1e0) = 0xffffffff;
              iVar2 = *(int *)(iVar4 + 0x54);
              if (iVar6 == iVar2) {
                *(undefined4 *)(iVar5 + 0x1e0) = *(undefined4 *)(iVar4 + 0x58);
              }
              else if ((iVar6 == *(int *)(iVar4 + 0x58)) && ((*(byte *)(iVar4 + 0x4e) & 1) != 0)) {
                *(int *)(iVar5 + 0x1e0) = iVar2;
              }
              else if ((*(ushort *)(iVar4 + 0x4e) & 2) == 0) {
                if ((*(ushort *)(iVar4 + 0x4e) & 4) != 0) {
                  *(int *)(iVar5 + 0x1e0) = *(int *)(iVar4 + 0x58);
                }
              }
              else {
                *(int *)(iVar5 + 0x1e0) = iVar2;
              }
            }
            local_18 = local_18 + 1;
            iVar6 = (int)(short)local_18;
          } while (iVar6 < *(int *)(iVar7 + 0x50));
        }
      }
    }
    else {
LAB_00430b2b:
      if (*(char *)(iVar4 + 7) == '\0') {
        if ((*(short *)(iVar4 + 0x48) < 0) ||
           (*(int *)(iVar7 + 0x5c) <= (int)*(short *)(iVar4 + 0x48))) {
          cVar3 = '\0';
        }
        else {
          cVar3 = '\x01';
        }
        while( true ) {
          if ((cVar3 != '\0') && (cVar3 = FUN_00431e70(), cVar3 == '\0')) goto LAB_00430b79;
          *(short *)(iVar4 + 0x48) = *(short *)(iVar4 + 0x48) + 1;
          if (*(int *)(iVar7 + 0x5c) <= (int)*(short *)(iVar4 + 0x48)) break;
          cVar3 = FUN_00431d10();
        }
        *(undefined1 *)(iVar4 + 7) = 1;
        goto LAB_00430b79;
      }
LAB_00430b80:
      squad_despawn(squad_instance_index,'\0','\x01');
    }
    iVar4 = data_iterator_next();
    squad_instance_index = local_8;
  } while( true );
}
#endif
