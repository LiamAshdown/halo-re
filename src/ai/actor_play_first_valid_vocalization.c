// actor_play_first_valid_vocalization  (Ghidra: actor_play_first_valid_vocalization, renamed)
// address 0x40e260, size 275 bytes
// name confidence: 0.45  rewrite confidence: 0.6
// evidence: the tail is actor_set_mode(actor_index, 9, block), and types/ai.h records mode
//   9 as _actor_mode_vocalize with the comment that 0x40e260 is what plays a line through
//   it; the block it hands over is filled by actor_build_order_investigate_encounter_point
//   @0x408a30 and is therefore in mode_data shape.
// register convention: the candidate list is in EAX (Ghidra in_EAX); actor_index, the
//   table, the category pointer and the count are the four stack parameters. Passing a
//   null list makes the function build its own list through 0x56a310, which is what both
//   call sites in actor_squad_action_execute do.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern uint8_t unit_seat_index_is_valid(void); // 0x565150, not yet rewritten (a different module): validates one line id
extern int16_t unit_find_seats_matching_name_and_flags(void); // 0x56a310, not yet rewritten (a different module): fills a candidate list

extern uint8_t actor_build_order_investigate_encounter_point(uint32_t actor_index, uint32_t object_index, int16_t kind, uint32_t *order); // 0x408a30, this module
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0, this module

// blam-cc: EAX -> candidates, stack -> actor_index, table, category, count
// Walks a list of candidate vocalization line ids and plays the first one that is still
// valid and for which a mode-9 data block can be built. The chosen entry is struck out of
// the list with -1 so a later pass does not pick it again. Returns 1 when a line was
// started.
uint8_t actor_play_first_valid_vocalization(int16_t *candidates, datum_index actor_index,
                                            const void *table, void *category, int16_t count)
{
    int16_t local_candidates[16]; // 32 bytes of stack the original reserves for the built list
    uint8_t mode_block[132];      // handed to actor_set_mode as the mode 9 data
    int16_t *list;
    int16_t list_count;
    int16_t i;

    if (candidates == (int16_t *)0) {
        list = local_candidates;
        // UNSURE: Ghidra shows a bare unit_find_seats_matching_name_and_flags(). table, category and the local buffer
        // are the only live values at the call, so they must be its arguments; it returns
        // how many entries it wrote.
        list_count = unit_find_seats_matching_name_and_flags();
    } else {
        list_count = count;
        list = candidates;
    }

    if (list_count < 1) {
        return 0;
    }

    for (i = 0; i < list_count; i++) {
        if (list[i] == -1) {
            continue;
        }
        // UNSURE: unit_seat_index_is_valid is invoked with no visible arguments; list[i] is the only
        // value that can be being validated.
        if (unit_seat_index_is_valid() == 0) {
            continue;
        }
        if (actor_build_order_investigate_encounter_point(actor_index, list[i], 0, (uint32_t *)mode_block) == 0) {
            continue;
        }
        actor_set_mode(actor_index, 9, mode_block);
        list[i] = -1;
        return 1;
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x40e260):

undefined1 FUN_0040e260(undefined4 param_1,undefined4 param_2,undefined4 param_3,short param_4)

{
  short sVar1;
  char cVar2;
  undefined1 *in_EAX;
  short sVar3;
  undefined1 *local_ac;
  short local_a8;
  undefined1 local_a4 [32];
  undefined1 local_84 [132];

  if (in_EAX == (undefined1 *)0x0) {
    local_ac = local_a4;
    local_a8 = FUN_0056a310();
  }
  else {
    local_a8 = param_4;
    local_ac = in_EAX;
  }
  sVar3 = 0;
  if (local_a8 < 1) {
    return 0;
  }
  while (((sVar1 = *(short *)(local_ac + sVar3 * 2), sVar1 == -1 ||
          (cVar2 = FUN_00565150(), cVar2 == '\0')) ||
         (cVar2 = actor_build_order_investigate_encounter_point(param_1,sVar1,local_84),
         cVar2 == '\0'))) {
    sVar3 = sVar3 + 1;
    if (local_a8 <= sVar3) {
      return 0;
    }
  }
  actor_set_mode(param_1,9,local_84);
  *(undefined2 *)(local_ac + sVar3 * 2) = 0xffff;
  return 1;
}
#endif
