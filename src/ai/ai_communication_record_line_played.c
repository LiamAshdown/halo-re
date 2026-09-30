// ai_communication_record_line_played  (Ghidra: ai_communication_record_line_played, already named)
// address 0x42f9e0, size 421 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (REWRITTEN 2026-09-27 static loop from objdump 0x42f9e0..0x42fb84)
// evidence: out/phase4/ai_functions.md signature and summary ("records the current tick as
// the last-played time for a spoken line across the relevant per-unit, per-class, and
// per-line-id timestamp tables"). types/ai.h already attributes ai_globals.unknown_3f0 and
// unknown_3fa to this exact address; ai_globals.loudest_line_tick[3][2] (unknown_14..28; all zeroed by
// ai_communication_reset.c) are the six-slot, two-tier "loudest recent line" accumulator
// this function maxes into, resolved once it becomes clear that Ghidra's "iVar7" is
// reassigned to ai_globals_ptr partway through and stays that way for the rest of the
// function.
// register convention: EAX -> object_index; param_1/2/3 are genuine stack arguments.
// blam-cc: EAX -> object_index, stack -> tier, communication_line_id, conversation_line_id
//
// UNSURE: object+0x3fa/0x3f0 in Ghidra's own decompile are read relative to the OBJECT
// pointer, but types/ai.h attributes them to ai_globals; trusted the header here since it
// documents this exact call site. actor_classify_communication_object_type's early-return
// (-1) case is reproduced as an early return here too.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"

extern data_array *object_data;    // 0x008603b0
extern ai_globals *ai_globals_ptr; // 0x00880354
extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t communication_line_base; // 0x006f0c9c
extern int32_t conversation_line_base;  // 0x006f0ca4
extern real DAT_00655ab4[]; // 0x00655ab4, stride 0x28 from &DAT_00655aa0 (the communication-line table)
extern real DAT_00656b24[]; // 0x00656b24, stride 0x24 from &DAT_00656b08 (the conversation-line table)

extern void actor_recompute_grenade_eligibility(datum_index actor_index); // 0x42f260, this batch
extern int32_t actor_classify_communication_object_type(datum_index actor_index); // 0x42f9a0, this batch

// blam-cc: EAX -> object_index, stack -> tier, communication_line_id, conversation_line_id
// Refreshes the decaying "recent activity" tick (ai_globals.unknown_3f0/unknown_3fa) and,
// when object_index has a controlling actor whose communication classification is 0 or 1,
// max-accumulates the current tick into that classification's tier-1 slot (always, when
// tier < 6), tier-2 slot (when tier > 2) and tier-3 slot (when tier > 4), then stamps the
// per-line last-played tick (and, when the line's own timing tag is positive, a secondary
// timestamp) for communication_line_id and conversation_line_id.
void ai_communication_record_line_played(datum_index object_index, int16_t tier,
                                          int16_t communication_line_id, int16_t conversation_line_id)
{
    uint8_t *obj;
    datum_index actor_index;
    int32_t current_tick;
    int32_t decay;
    int32_t stamp;
    int32_t category;
    int32_t *entry;
    int32_t *slot;

    obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    actor_index = *(datum_index *)(obj + 0x1f4); // unit_data.actor_index

    // 0x42fa21..0x42fa47: the stamp is the SPEAKING UNIT's +0x3fa (minus 45, floored at 0) plus the game tick, stored
    // in the unit's +0x3f0. FIXED 2026-09-27: the draft read and wrote ai_globals +0x3fa / +0x3f0 instead.
    current_tick = game_time->game_time;
    decay = *(int16_t *)(obj + 0x3fa) - 0x2d;
    if (decay < 0) {
        decay = 0;
    }
    stamp = decay + current_tick;
    *(int32_t *)(obj + 0x3f0) = stamp;

    if (actor_index == (datum_index)k_datum_index_none) {
        return;
    }

    actor_recompute_grenade_eligibility(actor_index);
    category = actor_classify_communication_object_type(actor_index);
    if (category == -1) {
        return;
    }

    // 0x42fa99..0x42fadf: the tier slots keep the max of themselves and the STAMP (the draft used the bare tick)
    if (tier <= 5) {
        slot = &ai_globals_ptr->loudest_line_tick[0][category];
        if (*slot <= stamp) {
            *slot = stamp;
        }
        if (tier >= 3) {
            slot = &ai_globals_ptr->loudest_line_tick[1][category];
            if (*slot <= stamp) {
                *slot = stamp;
            }
        }
        if (tier >= 5) {
            slot = &ai_globals_ptr->loudest_line_tick[2][category];
            if (*slot <= stamp) {
                *slot = stamp;
            }
        }
    }

    // 0x42fae3..0x42fb2e / 0x42fb31..0x42fb7c: [0] = the tick, [1] (when the line's delay is positive) =
    // __ftol(delay * 30.0 + stamp). The draft stored the tick in [1].
    if (communication_line_id != -1) {
        float delay = DAT_00655ab4[communication_line_id * 0x28 / 4];

        entry = (int32_t *)(communication_line_base + (category + communication_line_id * 2) * 8);
        entry[0] = current_tick;
        if (delay > 0.0f) {
            entry[1] = (int32_t)(delay * 30.0f + (float)stamp);
        }
    }
    if (conversation_line_id != -1) {
        float delay = DAT_00656b24[conversation_line_id * 0x24 / 4];

        entry = (int32_t *)(conversation_line_base + (category + conversation_line_id * 2) * 8);
        entry[0] = current_tick;
        if (delay > 0.0f) {
            entry[1] = (int32_t)(delay * 30.0f + (float)stamp);
        }
    }
}

#if 0
Original Ghidra decompilation (0x42f9e0):

void ai_communication_record_line_played(short param_1,short param_2,short param_3)

{
  uint uVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  uint in_EAX;
  int iVar5;
  int iVar6;
  int iVar7;
  short sVar8;

  iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (*(uint *)(iVar6 + 500) == 0xffffffff) {
    iVar7 = 0;
  }
  else {
    iVar7 = (*(uint *)(iVar6 + 500) & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  }
  iVar4 = *(int *)(DAT_006f1d6c + 0xc);
  uVar1 = (int)*(short *)(iVar6 + 0x3fa) - 0x2d;
  iVar2 = (((int)uVar1 < 0) - 1 & uVar1) + iVar4;
  *(int *)(iVar6 + 0x3f0) = iVar2;
  if (iVar7 != 0) {
    FUN_0042f260();
    iVar7 = DAT_00880354;
    if ((*(ushort *)
          ((&PTR_PTR_006853b8)
           [*(short *)((*(uint *)(iVar6 + 500) & 0xffff) * 0x724 + 4 + *(int *)(DAT_00880360 + 0x34)
                      )] + 4) & 2) == 0) {
      if ((*(ushort *)
            ((&PTR_PTR_006853b8)
             [*(short *)((*(uint *)(iVar6 + 500) & 0xffff) * 0x724 + 4 +
                        *(int *)(DAT_00880360 + 0x34))] + 4) & 4) == 0) {
        return;
      }
      sVar8 = 1;
    }
    else {
      sVar8 = 0;
    }
    if (param_1 < 6) {
      iVar5 = (int)sVar8;
      iVar6 = *(int *)(DAT_00880354 + 0x14 + iVar5 * 4);
      if (iVar6 <= iVar2) {
        iVar6 = iVar2;
      }
      *(int *)(DAT_00880354 + 0x14 + iVar5 * 4) = iVar6;
      if (2 < param_1) {
        iVar6 = *(int *)(iVar7 + 0x1c + iVar5 * 4);
        if (iVar6 <= iVar2) {
          iVar6 = iVar2;
        }
        *(int *)(iVar7 + 0x1c + iVar5 * 4) = iVar6;
      }
      if (4 < param_1) {
        iVar6 = *(int *)(iVar7 + 0x24 + iVar5 * 4);
        if (iVar6 <= iVar2) {
          iVar6 = iVar2;
        }
        *(int *)(iVar7 + 0x24 + iVar5 * 4) = iVar6;
      }
    }
    if (param_2 != -1) {
      piVar3 = (int *)(DAT_006f0c9c + ((int)sVar8 + param_2 * 2) * 8);
      *piVar3 = iVar4;
      if (0.0 < *(float *)(&DAT_00655ab4 + param_2 * 0x28)) {
        iVar6 = __ftol();
        piVar3[1] = iVar6;
      }
    }
    if (param_3 != -1) {
      piVar3 = (int *)(DAT_006f0ca4 + ((int)sVar8 + param_3 * 2) * 8);
      *piVar3 = iVar4;
      if (0.0 < *(float *)(&DAT_00656b24 + param_3 * 0x24)) {
        iVar6 = __ftol();
        piVar3[1] = iVar6;
      }
    }
  }
  return;
}
#endif
