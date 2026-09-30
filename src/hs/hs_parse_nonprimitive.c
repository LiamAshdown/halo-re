// hs_parse_nonprimitive  (Ghidra: hs_parse_nonprimitive, already named)
// address 0x486710, size 768 bytes
// name confidence: 0.9   rewrite confidence: 0.55
// evidence: CEA-PDB string matches; `node` is the list expression, node->data.first_child is
// its head identifier. If the expected type is _hs_type_special_form (1), the head must be the
// literal keyword "global" or "script" (hs_add_global/hs_add_script); otherwise the head is
// resolved as a function or script call via hs_resolve_identifier_as_function_or_script and
// dispatched to the matched hs_function_definition::parse callback or validated as a static/
// stub ScenarioScript call.
// register convention: __cdecl, node_index is the recognized single stack parameter.
// VERIFIED against disassembly 0x486710..0x486a0c (2026-09-30): hs_add_global / hs_add_script take the node index in EAX and are
// tail-returned; the "global"/"script" test is a 7-byte compare; the redundant re-tests of the expected type are dead branches.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <stdio.h>
#include <string.h>

extern void hs_resolve_identifier_as_function_or_script(datum_index node_index); // 0x00486680, this batch
extern char hs_types_are_compatible(hs_type_t destination_type, hs_type_t source_type); // 0x0048ac90, outside this batch's assigned range
extern char hs_add_global(datum_index node_index); // 0x00485b60, this batch
extern char hs_add_script(datum_index node_index); // 0x00485d50, this batch

extern data_array *hs_syntax_data;           // 0x0087a474
extern char *hs_compiled_source;             // 0x006b14c0
extern char *hs_compile_error;               // 0x006b14d4
extern int32_t hs_compile_error_offset;      // 0x006b14d8
extern Scenario *global_scenario;            // 0x00746f8c
extern char *hs_type_names[k_hs_type_count]; // 0x00688a78
extern char hs_compile_error_buffer[k_hs_error_buffer_size]; // 0x006b14dc
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern uint8_t hs_blocking_forbidden;        // 0x006b15de
extern uint8_t hs_set_forbidden;             // 0x006b15df

// Parses a non-primitive expression token: either the "global"/"script" special forms (when
// the node's expected type is _hs_type_special_form) or a function/script call, resolving and
// type-checking it against hs_function_definitions or the scenario's own scripts.
char hs_parse_nonprimitive(datum_index node_index)
{
    data_array *nodes;
    hs_syntax_node *node;
    hs_syntax_node *identifier_node;
    char *message;
    hs_type_t expected_type;
    int16_t resolved_index;
    hs_function_definition *function_def;
    hs_type_t actual_type;
    char compatible;
    char (*parse)(int16_t, datum_index);
    ScenarioScript *script;
    char *identifier_text;

    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & 0xffff) * nodes->size);
    identifier_node = (hs_syntax_node *)((uint8_t *)nodes->data + (node->data.first_child & 0xffff) * nodes->size);

    if ((identifier_node->flags & _hs_syntax_node_primitive_bit) == 0) {
        message = "\"script\" or \"global\"";
        if (node->type != _hs_type_special_form) {
            message = "a function name";
        }
        sprintf(hs_compile_error_buffer, "i expected %s, but i got an expression.", message);
        hs_compile_error = hs_compile_error_buffer;
        hs_compile_error_offset = identifier_node->source_offset;
        return 0;
    }

    if (node->type != _hs_type_special_form) {
        hs_resolve_identifier_as_function_or_script(node_index);
        resolved_index = node->index_union;
        if (resolved_index == -1) {
            hs_compile_error = "this is not a valid function or script name.";
            hs_compile_error_offset = identifier_node->source_offset;
            return 0;
        }

        if ((node->flags & _hs_syntax_node_script_call_bit) == 0) {
            /* function call */
            function_def = hs_function_definitions[resolved_index];
            expected_type = node->type;
            if (expected_type != 0) {
                actual_type = function_def->return_type;
                compatible = hs_types_are_compatible(expected_type, actual_type);
                if (compatible == 0) {
                    sprintf(hs_compile_error_buffer, "i expected a %s, but this function returns a %s.",
                            hs_type_names[expected_type], hs_type_names[actual_type]);
                    hs_compile_error = hs_compile_error_buffer;
                    hs_compile_error_offset = node->source_offset;
                    return 0;
                }
            }
            if ((hs_blocking_forbidden != 0) && ((resolved_index == _hs_function_sleep) || (resolved_index == _hs_function_sleep_until))) {
                hs_compile_error = "it is illegal to block in this context.";
                hs_compile_error_offset = node->source_offset;
                return 0;
            }
            if ((hs_set_forbidden != 0) && (resolved_index == _hs_function_set)) {
                hs_compile_error = "it is illegal to set the value of variables in this context.";
                hs_compile_error_offset = node->source_offset;
                return 0;
            }
            if ((expected_type == 0) && (function_def->return_type != 3)) {
                node->type = function_def->return_type;
            }
            parse = (char (*)(int16_t, datum_index))function_def->parse;
            return parse(resolved_index, node_index);
        }

        /* script call */
        script = (ScenarioScript *)global_scenario->scripts.pointer + resolved_index;
        if ((script->script_type != _hs_script_static) && (script->script_type != _hs_script_stub)) {
            hs_compile_error = "this is not a static script.";
            hs_compile_error_offset = node->source_offset;
            return 0;
        }
        expected_type = node->type;
        if (expected_type != 0) {
            actual_type = script->return_type;
            compatible = hs_types_are_compatible(expected_type, actual_type);
            if (compatible == 0) {
                sprintf(hs_compile_error_buffer, "i expected a %s, but this script returns a %s.",
                        hs_type_names[expected_type], hs_type_names[actual_type]);
                hs_compile_error = hs_compile_error_buffer;
                hs_compile_error_offset = node->source_offset;
                return 0;
            }
            /* expected_type != 0 here always: original re-tests the same condition and
               returns success without touching node->type -- equivalent to returning here. */
            return 1;
        }
        node->type = script->return_type;
        return 1;
    }

    /* node->type == _hs_type_special_form: the head identifier must be "global" or "script" */
    identifier_text = hs_compiled_source + identifier_node->source_offset;
    if (strncmp(identifier_text, "global", 7) == 0) {
        return hs_add_global(node_index);
    }
    if (strncmp(identifier_text, "script", 7) == 0) {
        return hs_add_script(node_index);
    }
    hs_compile_error = "i expected \"script\" or \"global\".";
    hs_compile_error_offset = identifier_node->source_offset;
    return 0;
}

#if 0
Original Ghidra decompilation (0x486710):

undefined4 hs_parse_nonprimitive(uint param_1)

{
  short sVar1;
  short sVar2;
  short sVar3;
  short *psVar4;
  char cVar5;
  char *pcVar6;
  undefined4 uVar7;
  int iVar8;
  char *pcVar9;
  char *pcVar10;
  int iVar11;
  bool bVar12;

  iVar8 = *(int *)(DAT_0087a474 + 0x34) + (param_1 & 0xffff) * 0x14;
  iVar11 = *(int *)(DAT_0087a474 + 0x34) + (*(uint *)(iVar8 + 0x10) & 0xffff) * 0x14;
  if ((*(byte *)(iVar11 + 6) & 1) == 0) {
    pcVar6 = "\"script\" or \"global\"";
    if (*(short *)(iVar8 + 4) != 1) {
      pcVar6 = "a function name";
    }
    _sprintf(&DAT_006b14dc,"i expected %s, but i got an expression.",pcVar6);
    DAT_006b14d4 = &DAT_006b14dc;
    DAT_006b14d8 = *(undefined4 *)(iVar11 + 0xc);
    return 0;
  }
  if (*(short *)(iVar8 + 4) != 1) {
    hs_resolve_identifier_as_function_or_script();
    sVar1 = *(short *)(iVar8 + 2);
    if (sVar1 == -1) {
      DAT_006b14d4 = "this is not a valid function or script name.";
      DAT_006b14d8 = *(undefined4 *)(iVar11 + 0xc);
      return 0;
    }
    if ((*(byte *)(iVar8 + 6) & 2) == 0) {
      psVar4 = (short *)(&PTR_DAT_00688b58)[sVar1];
      sVar2 = *(short *)(iVar8 + 4);
      if (sVar2 != 0) {
        sVar3 = *psVar4;
        cVar5 = hs_types_are_compatible();
        if (cVar5 == '\0') {
          _sprintf(&DAT_006b14dc,"i expected a %s, but this function returns a %s.",
                   (&PTR_s_unparsed_00688a78)[sVar2],(&PTR_s_unparsed_00688a78)[sVar3]);
          DAT_006b14d4 = &DAT_006b14dc;
          DAT_006b14d8 = *(undefined4 *)(iVar8 + 0xc);
          return 0;
        }
      }
      if ((DAT_006b15de != '\0') && ((sVar1 == 0x13 || (sVar1 == 0x14)))) {
        DAT_006b14d4 = "it is illegal to block in this context.";
        DAT_006b14d8 = *(undefined4 *)(iVar8 + 0xc);
        return 0;
      }
      if ((DAT_006b15df != '\0') && (sVar1 == 4)) {
        DAT_006b14d4 = "it is illegal to set the value of variables in this context.";
        DAT_006b14d8 = *(undefined4 *)(iVar8 + 0xc);
        return 0;
      }
      if ((sVar2 == 0) && (*psVar4 != 3)) {
        *(short *)(iVar8 + 4) = *psVar4;
      }
      uVar7 = (**(code **)(psVar4 + 4))(sVar1,param_1);
      return uVar7;
    }
    sVar2 = *(short *)(sVar1 * 0x5c + 0x20 + *(int *)(DAT_00746f8c + 0x4a0));
    iVar11 = sVar1 * 0x5c + *(int *)(DAT_00746f8c + 0x4a0);
    if ((sVar2 != 3) && (sVar2 != 4)) {
      DAT_006b14d4 = "this is not a static script.";
      DAT_006b14d8 = *(undefined4 *)(iVar8 + 0xc);
      return 0;
    }
    sVar1 = *(short *)(iVar8 + 4);
    if (sVar1 != 0) {
      sVar2 = *(short *)(iVar11 + 0x22);
      cVar5 = hs_types_are_compatible();
      if (cVar5 == '\0') {
        _sprintf(&DAT_006b14dc,"i expected a %s, but this script returns a %s.",
                 (&PTR_s_unparsed_00688a78)[sVar1],(&PTR_s_unparsed_00688a78)[sVar2]);
        DAT_006b14d4 = &DAT_006b14dc;
        DAT_006b14d8 = *(undefined4 *)(iVar8 + 0xc);
        return 0;
      }
      if (sVar1 != 0) {
        return 1;
      }
    }
    *(undefined2 *)(iVar8 + 4) = *(undefined2 *)(iVar11 + 0x22);
    return 1;
  }
  pcVar6 = (char *)(DAT_006b14c0 + *(int *)(iVar11 + 0xc));
  iVar8 = 7;
  bVar12 = true;
  pcVar9 = pcVar6;
  pcVar10 = "global";
  do {
    if (iVar8 == 0) break;
    iVar8 = iVar8 + -1;
    bVar12 = *pcVar9 == *pcVar10;
    pcVar9 = pcVar9 + 1;
    pcVar10 = pcVar10 + 1;
  } while (bVar12);
  if (bVar12) {
    uVar7 = hs_add_global();
    return uVar7;
  }
  iVar8 = 7;
  bVar12 = true;
  pcVar9 = "script";
  do {
    if (iVar8 == 0) break;
    iVar8 = iVar8 + -1;
    bVar12 = *pcVar6 == *pcVar9;
    pcVar6 = pcVar6 + 1;
    pcVar9 = pcVar9 + 1;
  } while (bVar12);
  if (bVar12) {
    uVar7 = hs_add_script();
    return uVar7;
  }
  DAT_006b14d4 = "i expected \"script\" or \"global\".";
  DAT_006b14d8 = *(undefined4 *)(iVar11 + 0xc);
  return 0;
}
#endif
