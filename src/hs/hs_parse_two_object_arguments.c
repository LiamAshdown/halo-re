// hs_parse_two_object_arguments  (Ghidra: FUN_00485150, renamed)
// address 0x485150, size 291 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: hs_functions.md: "Type-check callback that forces a function's two arguments to
// resolve to a compatible object/unit-family type." Sibling of hs_parse_two_numeric_arguments
// (0x485060): same hs_get_parameter_indices(name, 2) setup, but instead of accepting any type
// from the first successfully-parsed argument, it requires that type to fall in one of two
// ranges before reusing it for the other argument.
// register convention: same as hs_parse_two_numeric_arguments -- function_index recognized,
// node_index modeled as the second parameter to match the hs_function_definition::parse slot.
// UNSURE: no caller exists in the recovered call graph, same as 0x485060. The two accepted
// type ranges are preserved exactly as literal bounds (0x20..0x24 and 6..8) rather than named,
// since types/hs.h documents 0x20..0x24 as the enum types (difficulty/team/ai_default_state/
// actor_type/hud_corner) and 6..8 as real/short/long -- an odd combination for an "object/
// unit-family" check, so the hs_functions.md description may not be precise; the condition
// itself is not in doubt, only its best English label.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern char hs_get_parameter_indices(char *function_name, int16_t required_count, datum_index node_index, datum_index *out_indices); // 0x00484fb0, this batch
extern char hs_parse(datum_index node_index, hs_type_t expected_type); // 0x00486420, this batch

extern data_array *hs_syntax_data; // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern char *hs_compile_error; // 0x006b14d4

// Requires exactly two arguments; if the first parses to a type in [0x20,0x25) or [6,9), the
// second is parsed against that same type, and vice versa if only the second qualifies;
// otherwise both are parsed against _hs_type_real.
char hs_parse_two_object_arguments(int16_t function_index, datum_index node_index)
{
    datum_index arguments[2];
    char ok;
    hs_type_t type;

    ok = hs_get_parameter_indices(hs_function_definitions[function_index]->name, 2, node_index, arguments);
    if (ok == 0) {
        return 0;
    }
    ok = hs_parse(arguments[0], 0);
    if (ok != 0) {
        type = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
                (arguments[0] & 0xffff) * hs_syntax_data->size))->type;
        if (((0x1f < type) && (type < 0x25)) || ((5 < type) && (type < 9))) {
            ok = hs_parse(arguments[1], type);
            return ok != 0;
        }
    }
    if (hs_compile_error != 0) {
        return 0;
    }
    ok = hs_parse(arguments[1], 0);
    if (ok != 0) {
        type = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
                (arguments[1] & 0xffff) * hs_syntax_data->size))->type;
        if (((0x1f < type) && (type < 0x25)) || ((5 < type) && (type < 9))) {
            goto second_pass;
        }
    }
    if (hs_compile_error != 0) {
        return 0;
    }
    ok = hs_parse(arguments[0], _hs_type_real);
    if (ok == 0) {
        return 0;
    }
    type = _hs_type_real;
    arguments[0] = arguments[1];
second_pass:
    ok = hs_parse(arguments[0], type);
    return ok != 0;
}

#if 0
Original Ghidra decompilation (0x485150):

bool FUN_00485150(short param_1)

{
  short sVar1;
  char cVar2;
  undefined4 uVar3;
  uint local_8;
  uint local_4;

  cVar2 = hs_get_parameter_indices(*(undefined4 *)((&PTR_DAT_00688b58)[param_1] + 4),2);
  if (cVar2 == '\0') {
    return false;
  }
  cVar2 = hs_parse(local_8,0);
  if (cVar2 != '\0') {
    sVar1 = *(short *)(*(int *)(DAT_0087a474 + 0x34) + 4 + (local_8 & 0xffff) * 0x14);
    if (((0x1f < sVar1) && (sVar1 < 0x25)) || ((5 < sVar1 && (sVar1 < 9)))) {
      cVar2 = hs_parse(local_4,CONCAT22((short)((local_8 & 0xffff) * 5 >> 0x10),sVar1));
      if (cVar2 == '\0') {
        return false;
      }
      return true;
    }
  }
  if (DAT_006b14d4 != 0) {
    return false;
  }
  cVar2 = hs_parse(local_4,0);
  if (cVar2 != '\0') {
    sVar1 = *(short *)(*(int *)(DAT_0087a474 + 0x34) + 4 + (local_4 & 0xffff) * 0x14);
    uVar3 = CONCAT22((short)((uint)*(int *)(DAT_0087a474 + 0x34) >> 0x10),sVar1);
    if (((0x1f < sVar1) && (sVar1 < 0x25)) || ((5 < sVar1 && (sVar1 < 9)))) goto LAB_00485258;
  }
  if (DAT_006b14d4 != 0) {
    return false;
  }
  cVar2 = hs_parse(local_8,6);
  if (cVar2 == '\0') {
    return false;
  }
  uVar3 = 6;
  local_8 = local_4;
LAB_00485258:
  cVar2 = hs_parse(local_8,uVar3);
  return cVar2 != '\0';
}
#endif
