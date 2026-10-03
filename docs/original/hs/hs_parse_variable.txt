// hs_parse_variable  (Ghidra: hs_parse_variable, already named)
// address 0x486560, size 285 bytes
// name confidence: 0.9   rewrite confidence: 0.55
// evidence: CEA-PDB string match; resolves a bare-word token as a global reference via
// hs_find_global_by_name, checks its declared type against the node's expected type (or
// adopts it if none was expected), and sets the global-reference flag on success.
// register convention: the node index is unrecognized by Ghidra (in_EAX); by the blam-cc
// convention this is the first register slot, EAX.
// VERIFIED against disassembly 0x486560..0x48667c (2026-09-30): the error sprintf pushes (buffer, format, expected type name,
// variable name from hs_global_get_name, actual type name); the node type is re-tested after the compatibility check and always
// skips the assignment in that path; the global reference is stored sign-extended (movsx).

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern hs_global_reference hs_find_global_by_name(char *name); // 0x00483480, this batch
extern char *hs_global_get_name(hs_global_reference global); // 0x00483450, this batch
extern char hs_types_are_compatible(hs_type_t destination_type, hs_type_t source_type); // 0x0048ac90, outside this batch's assigned range

extern data_array *hs_syntax_data;           // 0x0087a474
extern char *hs_compiled_source;             // 0x006b14c0
extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count]; // 0x0068b398
extern Scenario *global_scenario;            // 0x00746f8c
extern char *hs_type_names[k_hs_type_count]; // 0x00688a78
extern char hs_compile_error_buffer[k_hs_error_buffer_size]; // 0x006b14dc
extern char *hs_compile_error;               // 0x006b14d4
extern int32_t hs_compile_error_offset;      // 0x006b14d8
extern uint8_t hs_postprocessing;            // 0x006b15e0

// blam-cc: node index in EAX
// Parses/type-checks a bare-word syntax node as a reference to a global variable (builtin or
// scenario-defined), adopting the global's type if the node had no expected type yet, or
// reporting a type mismatch against one it did.
char hs_parse_variable(datum_index node_index)
{
    hs_syntax_node *node;
    hs_global_reference global;
    hs_type_t global_type;
    hs_type_t node_type;
    char compatible;
    char *global_name;

    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * hs_syntax_data->size);
    global = hs_find_global_by_name(hs_compiled_source + node->source_offset);
    node->data.global_reference = (int16_t)global;
    if ((int16_t)global == -1) {
        if (hs_postprocessing != 0) {
            hs_compile_error = (char *)"this is not a valid variable name.";
            hs_compile_error_offset = node->source_offset;
        }
        return 0;
    }

    if ((global & k_hs_global_builtin_bit) == 0) {
        global_type = ((ScenarioGlobal *)global_scenario->globals.pointer)[global & k_hs_global_index_mask].type;
    } else {
        global_type = hs_global_definitions[global & k_hs_global_index_mask]->type;
    }

    node_type = node->type;
    if (node_type != 0) {
        compatible = hs_types_are_compatible(node_type, global_type);
        if (compatible == 0) {
            global_name = hs_global_get_name(global);
            sprintf(hs_compile_error_buffer, "i expected a value of type %s, but the variable %s has type %s",
                    hs_type_names[node_type], global_name, hs_type_names[global_type]);
            hs_compile_error = hs_compile_error_buffer;
            hs_compile_error_offset = node->source_offset;
            return 0;
        }
        /* node_type != 0 and compatible: the original re-tests "if (node_type != 0)" here,
           which is always true in this scope, so it always skips straight past the type
           assignment below (the dead re-test is folded into the else). */
    } else {
        node->type = global_type;
    }
    node->flags = node->flags | _hs_syntax_node_global_bit;
    return 1;
}

#if 0
Original Ghidra decompilation (0x486560):

undefined4 hs_parse_variable(void)

{
  int iVar1;
  short sVar2;
  char cVar3;
  ushort uVar4;
  uint in_EAX;
  undefined4 uVar5;
  short sVar6;

  iVar1 = *(int *)(DAT_0087a474 + 0x34) + (in_EAX & 0xffff) * 0x14;
  uVar4 = chimera__get_global_index();
  *(int *)(iVar1 + 0x10) = (int)(short)uVar4;
  if ((short)uVar4 == -1) {
    if (DAT_006b15e0 != '\0') {
      DAT_006b14d4 = "this is not a valid variable name.";
      DAT_006b14d8 = *(undefined4 *)(iVar1 + 0xc);
      return 0;
    }
    return 0;
  }
  if ((uVar4 & 0x8000) == 0) {
    sVar6 = *(short *)((uVar4 & 0x7fff) * 0x5c + 0x20 + *(int *)(DAT_00746f8c + 0x4ac));
  }
  else {
    sVar6 = *(short *)((&PTR_PTR_0068b398)[uVar4 & 0x7fff] + 4);
  }
  sVar2 = *(short *)(iVar1 + 4);
  if (sVar2 != 0) {
    cVar3 = hs_types_are_compatible();
    if (cVar3 == '\0') {
      uVar5 = hs_global_get_address((&PTR_s_unparsed_00688a78)[sVar6]);
      _sprintf(&DAT_006b14dc,"i expected a value of type %s, but the variable %s has type %s",
               (&PTR_s_unparsed_00688a78)[sVar2],uVar5);
      DAT_006b14d4 = &DAT_006b14dc;
      DAT_006b14d8 = *(undefined4 *)(iVar1 + 0xc);
      return 0;
    }
    if (sVar2 != 0) goto LAB_00486647;
  }
  *(short *)(iVar1 + 4) = sVar6;
LAB_00486647:
  *(byte *)(iVar1 + 6) = *(byte *)(iVar1 + 6) | 4;
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
