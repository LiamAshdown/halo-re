// hs_parse_object_name  (Ghidra: hs_parse_object_name, already named; confirmed by the CEA
// prototype's string overlap on both error messages)
// address 0x487230, size 191 bytes
// name confidence: 0.9   rewrite confidence: 0.75
// evidence: types/tags.h Scenario::object_names (TagReflexive, ScenarioObjectName, size 0x24,
//   object_type at 0x20) matches out/phase4/hs_types_notes.md's documented 0x204/0x208 count/
//   pointer pair with stride 0x24; hs.h's hs_object_type_masks (0x00657538, biased by
//   _hs_type_object_name == 0x2b, which is exactly DAT_006574e2's bias in this function).
// register convention: none (void); node index is the recognized stack parameter (param_1).
// note: the object-name lookup helper (0x0053ebb0, symbols/review_queue.txt candidate name
//   scenario_object_name_find_index, confidence 0.45) belongs to the objects module and has not
//   been rewritten yet; only its prototype is declared here. Its ECX argument is inferred from
//   the immediately preceding `iVar2 = DAT_00746f8c` load, which Ghidra placed after the call
//   because iVar2 (== ECX at the call) is read again afterward for object_names.pointer.
//   // blam-cc: ECX -> scenario, stack -> name

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"

extern int16_t scenario_object_name_find_index(Scenario *scenario, char *name);
    // blam-cc: ECX -> scenario, stack -> name; objects module, 0x0053ebb0, not yet rewritten

extern data_array *hs_syntax_data;                 // 0x0087a474
extern char *hs_compiled_source;                    // 0x006b14c0
extern Scenario *global_scenario;                   // 0x00746f8c
extern char *hs_compile_error;                      // 0x006b14d4
extern int32_t hs_compile_error_offset;             // 0x006b14d8
extern char hs_compile_error_buffer[k_hs_error_buffer_size]; // 0x006b14dc
extern char *hs_type_names[k_hs_type_count];        // 0x00688a78
extern uint16_t hs_object_type_masks[6];            // 0x00657538, biased by _hs_type_object_name

// Parses an object-name token: resolves it against Scenario::object_names, then checks that the
// matched entry's object_type is a member of the object family the node's expected type demands
// (e.g. a "vehicle_name" node requires the vehicle bit). Stores the matched index on success.
char hs_parse_object_name(datum_index node_index)
{
    hs_syntax_node *node;
    Scenario *scenario;
    ScenarioObjectName *object_names;
    int16_t match_index;

    scenario = global_scenario;
    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * 0x14);

    match_index = scenario_object_name_find_index(scenario, hs_compiled_source + node->source_offset);
    if (match_index == -1) {
        hs_compile_error = (char *)"this is not a valid object name.";
        hs_compile_error_offset = node->source_offset;
        return 0;
    }

    object_names = (ScenarioObjectName *)scenario->object_names.pointer;
    if (hs_object_type_masks[node->type - _hs_type_object_name] &
        (1 << (object_names[match_index].object_type & 0x1f))) {
        node->data.short_value = match_index;
        return 1;
    }

    sprintf(hs_compile_error_buffer, "this is not an object of type %s.", hs_type_names[node->type]);
    hs_compile_error = hs_compile_error_buffer;
    hs_compile_error_offset = node->source_offset;
    return 0;
}

#if 0
Original Ghidra decompilation (0x487230):

undefined4 hs_parse_object_name(uint param_1)

{
  int iVar1;
  int iVar2;
  short sVar3;

  iVar2 = DAT_00746f8c;
  iVar1 = *(int *)(DAT_0087a474 + 0x34) + (param_1 & 0xffff) * 0x14;
  sVar3 = FUN_0053ebb0(*(int *)(iVar1 + 0xc) + DAT_006b14c0);
  if (sVar3 == -1) {
    DAT_006b14d4 = "this is not a valid object name.";
    DAT_006b14d8 = *(undefined4 *)(iVar1 + 0xc);
    return 0;
  }
  if (((int)*(short *)(&DAT_006574e2 + *(short *)(iVar1 + 4) * 2) &
      1 << (*(byte *)(*(int *)(iVar2 + 0x208) + 0x20 + sVar3 * 0x24) & 0x1f)) != 0) {
    *(short *)(iVar1 + 0x10) = sVar3;
    return 1;
  }
  _sprintf(&DAT_006b14dc,"this is not an object of type %s.",
           (&PTR_s_unparsed_00688a78)[*(short *)(iVar1 + 4)]);
  DAT_006b14d4 = &DAT_006b14dc;
  DAT_006b14d8 = *(undefined4 *)(iVar1 + 0xc);
  return 0;
}
#endif
