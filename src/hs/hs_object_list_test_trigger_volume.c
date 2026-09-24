// hs_object_list_test_trigger_volume  (Ghidra: FUN_00487820)
// address 0x487820, size 207 bytes
// name confidence: 0.3 (out/phase4/hs_functions.md: "Evaluates a boolean predicate across every
//   object in a reference list, short-circuiting as either an 'all' or an 'any' test selected by
//   a flag"; the predicate is hardcoded to trigger-volume containment)
// rewrite confidence: 0.55
// evidence: types/hs.h object_list_header (first_reference at 0x08) and object_list_reference
//   (object_index at 0x04, next at 0x08); symbols/review_queue.txt candidate evidence for
//   0x53f020 ("Indexes a 0x60-byte-stride array off DAT_00746f8c+0x364 (scenario_trigger_volumes
//   block) by type field: type 0 does an axis-aligned min/max box test...").
// register convention: trigger volume index in EAX (param_1, passed straight through to every
//   predicate call, never read locally); all-vs-any flag in the low byte of ECX (param_2, also
//   the "default" return value when the list is empty or exhausted without short-circuiting);
//   reference-list header index in ECX's other half... actually in a separate register, in_ECX
//   (a genuinely different value from param_2 despite Ghidra's naming collision -- see UNSURE).
//   // blam-cc: EAX -> trigger_volume_index (pass-through), stack -> all_mode, ECX -> header_index
// UNSURE: Ghidra names both the recognized stack parameter and the true incoming register
//   differently but both print as involving "ECX" naming (param_2 is the stack all/any flag;
//   in_ECX, used only for the list lookup, is the true header index register). This function's
//   own object_index is never given to scenario_trigger_volume_contains_point explicitly either
//   -- like hs_reposition_players_outside_trigger_volume, the per-iteration object index is
//   computed into a register the call immediately consumes, which the decompile does not show.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern char scenario_trigger_volume_contains_point(int32_t trigger_volume_index,
    datum_index object_index); // blam-cc: register args UNSURE; scenario module, 0x53f020,
                                // not yet rewritten

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

// Tests every object referenced by the list headed by `header_index` against
// `trigger_volume_index`: with `all_mode` set, returns 1 only if every object is inside the
// volume (short-circuiting to 0 on the first that is not); with `all_mode` clear, returns 1 as
// soon as any object is inside (short-circuiting to 1), otherwise 0. An empty list returns
// `all_mode` unchanged (vacuously true for "all", false for "any").
char hs_object_list_test_trigger_volume(int32_t trigger_volume_index, datum_index header_index,
    char all_mode)
{
    object_list_header *header;
    object_list_reference *reference;
    datum_index next;
    int32_t object_index;
    char inside;

    object_index = -1;
    next = header_index;
    if (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (header_index & 0xffff) * 0x0c);
        next = header->first_reference;
        if (next == k_datum_index_none) {
            object_index = -1;
            next = 0xffffffff;
        } else {
            reference = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (next & 0xffff) * 0x0c);
            object_index = reference->object_index;
            next = reference->next;
        }
    }

    if (object_index == -1) {
        return all_mode;
    }

    do {
        inside = scenario_trigger_volume_contains_point(trigger_volume_index, object_index);
        if (inside == 0) {
            if (all_mode != 0) {
                return 0;
            }
        } else if (all_mode == 0) {
            return 1;
        }
        if (next == 0xffffffff) {
            object_index = -1;
        } else {
            reference = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (next & 0xffff) * 0x0c);
            next = reference->next;
            object_index = reference->object_index;
        }
    } while (object_index != -1);
    return all_mode;
}

#if 0
Original Ghidra decompilation (0x487820):

undefined4 FUN_00487820(undefined4 param_1,uint param_2)

{
  int iVar1;
  char cVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint in_ECX;

  iVar1 = DAT_0087a468;
  iVar3 = -1;
  uVar4 = param_2;
  if (in_ECX != 0xffffffff) {
    uVar4 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    if (uVar4 == 0xffffffff) {
      iVar3 = -1;
      uVar4 = 0xffffffff;
    }
    else {
      uVar4 = uVar4 & 0xffff;
      iVar3 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar4 * 0xc + 4);
      uVar4 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar4 * 0xc);
    }
  }
  if (iVar3 == -1) {
    return CONCAT31(0xffffff,(char)param_2);
  }
  do {
    cVar2 = scenario_trigger_volume_contains_point();
    if (cVar2 == '\0') {
      if ((char)param_2 != '\0') {
        return 0;
      }
    }
    else if ((char)param_2 == '\0') {
      return 1;
    }
    if (uVar4 == 0xffffffff) {
      iVar3 = -1;
    }
    else {
      uVar5 = uVar4 & 0xffff;
      uVar4 = *(uint *)(*(int *)(iVar1 + 0x34) + 8 + uVar5 * 0xc);
      iVar3 = *(int *)(*(int *)(iVar1 + 0x34) + uVar5 * 0xc + 4);
    }
  } while (iVar3 != -1);
  return CONCAT31(0xffffff,(char)param_2);
}
#endif
