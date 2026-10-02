// hs_parse_set  (Ghidra: hs_parse_set, already named)
// address 0x484be0, size 451 bytes
// name confidence: 0.9   rewrite confidence: 0.55
// evidence: CEA-PDB string match; requires exactly two children (a variable name, a value),
// resolves the variable by name, type-checks the (set ...) expression's own expected type
// against the variable's type, then re-runs hs_parse_variable on the variable node (for its
// side effect of setting the global-reference flag) and parses the value against the
// variable's type.
// register convention: __cdecl; param_1 (function_index) is unused, matching the
// hs_function_definition::parse signature (int16_t function_index, datum_index node).
// UNSURE: the error sprintf's two %s arguments are cast-to-int raw hs_type values in Ghidra's
// view ("(int)sVar6" / "(int)*(short*)..."), not hs_type_names[...] lookups; every sibling
// message in this module indexes hs_type_names for a %s, so this is reconstructed the same way
// rather than literally passing small integers where a string pointer is expected.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <stdio.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_global_reference hs_find_global_by_name(char *name); // 0x00483480, this batch
extern hs_type_t hs_global_get_type(hs_global_reference global); // 0x00483420, this batch
extern char hs_parse(datum_index node_index, hs_type_t expected_type); // 0x00486420, this batch
extern char hs_parse_variable(datum_index node_index); // 0x00486560, this batch
extern char hs_types_are_compatible(hs_type_t destination_type, hs_type_t source_type); // 0x0048ac90, outside this batch's assigned range

extern data_array *hs_syntax_data;           // 0x0087a474
extern char *hs_compiled_source;             // 0x006b14c0
extern char hs_compile_error_buffer[k_hs_error_buffer_size]; // 0x006b14dc
extern char *hs_compile_error;               // 0x006b14d4
extern int32_t hs_compile_error_offset;      // 0x006b14d8
extern char *hs_type_names[k_hs_type_count]; // 0x00688a78

// Parses/type-checks a (set <global> <value>) special-form syntax node.
char hs_parse_set(int16_t function_index, datum_index node_index)
{
    data_array *nodes;
    hs_syntax_node *node;
    datum_index variable_index;
    hs_syntax_node *variable_node;
    datum_index value_index;
    hs_syntax_node *value_node;
    hs_syntax_node *extra_node;
    hs_global_reference global;
    hs_type_t global_type;
    char compatible;
    char ok;

    (void)function_index;
    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & 0xffff) * nodes->size);
    variable_index = node->data.first_child;
    if (variable_index == k_datum_index_none) {
        hs_compile_error = (char *)"i expected a variable to set and a value.";
        hs_compile_error_offset = node->source_offset;
        return 0;
    }
    variable_node = (hs_syntax_node *)((uint8_t *)nodes->data + (variable_index & 0xffff) * nodes->size);
    value_index = variable_node->next_node;
    if (value_index == k_datum_index_none) {
        hs_compile_error = (char *)"i expected an assignment value.";
        hs_compile_error_offset = node->source_offset;
        return 0;
    }
    value_node = (hs_syntax_node *)((uint8_t *)nodes->data + (value_index & 0xffff) * nodes->size);
    if (value_node->next_node != k_datum_index_none) {
        extra_node = (hs_syntax_node *)((uint8_t *)nodes->data + (value_node->next_node & 0xffff) * nodes->size);
        hs_compile_error = (char *)"i didn't expect this argument.";
        hs_compile_error_offset = extra_node->source_offset;
        return 0;
    }

    global = hs_find_global_by_name(hs_compiled_source + variable_node->source_offset);
    if (global == k_hs_global_reference_none) {
        hs_compile_error = (char *)"this is not a valid global variable.";
        hs_compile_error_offset = variable_node->source_offset;
        return 0;
    }
    global_type = hs_global_get_type(global);
    variable_node->type = global_type;
    if ((node->type != 0) &&
        ((compatible = hs_types_are_compatible(node->type, global_type)), compatible == 0)) {
        sprintf(hs_compile_error_buffer,
                "you cannot pass the result of this set (type %s) to a function that expects type %s.",
                hs_type_names[global_type], hs_type_names[node->type]);
        hs_compile_error = hs_compile_error_buffer;
        hs_compile_error_offset = node->source_offset;
        return 0;
    }
    hs_parse_variable(variable_index); /* side effect only: sets the global-reference flag */
    if (node->type == 0) {
        node->type = variable_node->type;
    }
    ok = hs_parse(value_index, variable_node->type);
    return ok != 0;
}

#if 0
Original Ghidra decompilation (0x484be0):

undefined4 hs_parse_set(undefined4 param_1,uint param_2)

{
  int iVar1;
  short *psVar2;
  int iVar3;
  uint uVar4;
  char cVar5;
  short sVar6;
  undefined4 uVar7;
  int iVar8;
  int iVar9;

  iVar3 = *(int *)(DAT_0087a474 + 0x34);
  iVar9 = (param_2 & 0xffff) * 0x14;
  uVar4 = *(uint *)(iVar3 + 8 + (*(uint *)(iVar3 + 0x10 + iVar9) & 0xffff) * 0x14);
  uVar7 = 0;
  if (uVar4 == 0xffffffff) {
    DAT_006b14d4 = "i expected a variable to set and a value.";
    DAT_006b14d8 = *(undefined4 *)(*(int *)(DAT_0087a474 + 0x34) + 0xc + iVar9);
    return uVar7;
  }
  iVar1 = iVar3 + (uVar4 & 0xffff) * 0x14;
  uVar4 = *(uint *)(iVar1 + 8);
  if (uVar4 == 0xffffffff) {
    DAT_006b14d4 = "i expected an assignment value.";
    DAT_006b14d8 = *(undefined4 *)(*(int *)(DAT_0087a474 + 0x34) + 0xc + iVar9);
    return uVar7;
  }
  iVar8 = (uVar4 & 0xffff) * 0x14;
  if (*(int *)(iVar8 + 8 + iVar3) != -1) {
    DAT_006b14d4 = "i didn\'t expect this argument.";
    DAT_006b14d8 = *(undefined4 *)
                    (*(int *)(DAT_0087a474 + 0x34) + 0xc +
                    (*(uint *)(*(int *)(DAT_0087a474 + 0x34) + 8 + iVar8) & 0xffff) * 0x14);
    return uVar7;
  }
  sVar6 = chimera__get_global_index();
  if (sVar6 != -1) {
    sVar6 = FUN_00483420();
    psVar2 = (short *)(iVar3 + 4 + iVar9);
    *(short *)(iVar1 + 4) = sVar6;
    if ((*psVar2 != 0) && (cVar5 = hs_types_are_compatible(), cVar5 == '\0')) {
      _sprintf(&DAT_006b14dc,
               "you cannot pass the result of this set (type %s) to a function that expects type %s."
               ,(int)sVar6,(int)*(short *)(*(int *)(DAT_0087a474 + 0x34) + 4 + iVar9));
      DAT_006b14d4 = &DAT_006b14dc;
      DAT_006b14d8 = *(undefined4 *)(*(int *)(DAT_0087a474 + 0x34) + 0xc + iVar9);
      return 0;
    }
    hs_parse_variable();
    if (*psVar2 == 0) {
      *psVar2 = *(short *)(iVar1 + 4);
    }
    cVar5 = hs_parse(uVar4,*(undefined2 *)(iVar1 + 4));
    if (cVar5 == '\0') {
      return 0;
    }
    return 1;
  }
  DAT_006b14d4 = "this is not a valid global variable.";
  DAT_006b14d8 = *(undefined4 *)(iVar1 + 0xc);
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
