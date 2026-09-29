// hs_compile_postprocess  (Ghidra: hs_compile_postprocess, already named)
// address 0x4858c0, size 672 bytes
// name confidence: 0.9   rewrite confidence: 0.55
// evidence: CEA-PDB string match; walks every live hs_syntax_node the same way
// hs_syntax_node_garbage_collect does (datum_next inline), re-resolving each node's function
// or script reference by name/index and re-validating its type against usage. The five error
// strings and their trigger conditions match out/phase4/hs_types_notes.md's field evidence for
// hs_syntax_node, hs_function_definition and ScenarioScript.
// register convention: __cdecl, error_message/error_offset out-parameters recognized directly.
// UNSURE: `stale_script_pointer` mirrors Ghidra's `local_4 = in_ECX;` -- a value captured from
// whatever ECX held on entry, then possibly read back UNINITIALIZED-ACROSS-ITERATIONS in the
// script-index branch's short-circuited `||` (see below): if the left side of that OR is false
// without reaching the assignment, the branch reads whatever this variable held from the LAST
// time it *was* assigned (or its indeterminate initial value, on the first such node). This is
// preserved exactly rather than "fixed".
// UNSURE: hs_types_are_compatible's argument order (destination/expected type first, source/
// resolved type second) is inferred from types/hs.h's "[destination][source]" table
// description, not read directly off this call site (Ghidra shows it with no visible args).

// FIXED (verified against 0x485a4a..0x485a7a): the function-name node's own source offset (+0xc) is both the
//   one checked by hs_verify_source_offset and, added to hs_compiled_source, the name looked up; the draft used
//   the call node's offset and the name node's +0x10 as a char pointer (NULL -> _stricmp fast-fail).
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x004d0630


extern Scenario *global_scenario;              // 0x00746f8c
extern char *hs_compiled_source;               // 0x006b14c0
extern int32_t hs_compiled_source_length;      // 0x006b14bc
extern char *hs_compile_error;                 // 0x006b14d4
extern uint8_t hs_postprocessing;              // 0x006b15e0
extern data_array *hs_syntax_data;             // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern int32_t hs_compile_error_offset;        // 0x006b14d8

// Second compilation pass: for every live hs_syntax_node, re-resolves function/script/global
// references by name or index (as loaded from a possibly stale scenario tag) and re-validates
// its type against how it is used, reporting the first inconsistency found.
char hs_compile_postprocess(char **error_message, int32_t *error_offset)
{
    data_array *nodes;
    char success;
    datum_index current;
    hs_syntax_node *node;
    hs_type_t node_type;
    int32_t stale_script_pointer; // UNSURE: see file header
    int32_t next_start;
    int16_t next_index;
    hs_syntax_node *scan;
    char valid_offset;
    hs_syntax_node *function_name_node;
    int16_t function_index;

    nodes = hs_syntax_data;
    hs_compiled_source = (char *)global_scenario->script_string_data.pointer;
    hs_compiled_source_length = (int32_t)global_scenario->script_string_data.size - 0x400;
    success = 1;
    hs_compile_error = 0;
    hs_postprocessing = 1;
    *error_message = 0;
    *error_offset = 0;
    current = datum_next(-1, nodes);

    do {
        if (current == k_datum_index_none) {
            if (success != 0) {
                hs_compiled_source = 0;
                hs_compile_error = 0;
                hs_postprocessing = 0;
                return success;
            }
            break;
        }
        node = (hs_syntax_node *)((uint8_t *)nodes->data + (current & 0xffff) * nodes->size);
        node_type = node->type;

        if ((node_type < 4) || (0x30 < node_type)) {
            if (node_type != 2) {
                hs_compile_error = "missing type (you need to recompile scripts.)";
                goto fail;
            }
            goto advance; /* _hs_function_name: nothing more to postprocess on this node */
        }

        if ((node->flags & _hs_syntax_node_primitive_bit) == 0) {
            /* non-primitive: a function call or a script call */
            if ((node->flags & _hs_syntax_node_script_call_bit) != 0) {
                node_type = node->index_union;
                if ((((-1 < node_type) && (node_type < (int32_t)global_scenario->scripts.count)) &&
                     (stale_script_pointer = node_type * 0x5c + (int32_t)global_scenario->scripts.pointer,
                      *(int16_t *)(stale_script_pointer + 0x20) == _hs_script_static)) ||
                    (*(int16_t *)(stale_script_pointer + 0x20) == _hs_script_stub)) {
                    node_type = *(int16_t *)(stale_script_pointer + 0x22);
                    goto resolved;
                }
                hs_compile_error = "bad script index (you need to recompile.)";
                goto fail;
            }
            /* function call */
            if (node->data.first_child == k_datum_index_none) {
                hs_compile_error = "corrupt syntax tree (you need to recompile scripts.)";
                goto fail;
            }
            function_name_node = (hs_syntax_node *)((uint8_t *)nodes->data + (node->data.first_child & 0xffff) * nodes->size);
            if (function_name_node->type != 2) {
                hs_compile_error = "corrupt syntax tree (you need to recompile scripts.)";
                goto fail;
            }
            valid_offset = hs_verify_source_offset(function_name_node->source_offset); // 0x485a65: ECX = name node +0xc
            if (valid_offset == 0) {
                goto fail;
            }
            function_index = hs_find_function_by_name(hs_compiled_source + function_name_node->source_offset);
                // 0x485a71: EDX = name node's source offset + hs_compiled_source
            nodes = hs_syntax_data;
            if (function_index == -1) {
                hs_compile_error = "missing function (you need to recompile scripts.)";
                goto fail;
            }
            node->index_union = function_index;
            node_type = hs_function_definitions[function_index]->return_type;
            goto resolved;
        }

        /* primitive node */
        if ((node_type < 9) && ((node->flags & _hs_syntax_node_global_bit) == 0)) {
            goto recheck_global;
        }
        valid_offset = 1;
        if ((node->source_offset < 0) || (hs_compiled_source_length <= node->source_offset)) {
            hs_compile_error = "bad source offset (you need to recompile.)";
            valid_offset = 0;
        }
        success = 0;
        if (valid_offset != 0) {
            success = hs_parse_primitive(current);
            nodes = hs_syntax_data;
            goto recheck_global;
        }
        goto use_index_union;

    recheck_global:
        if ((success == 0) || ((node->flags & _hs_syntax_node_global_bit) == 0)) {
            goto use_index_union;
        }
        node_type = hs_global_get_type(node->data.global_reference);
        goto resolved;

    use_index_union:
        node_type = node->index_union;

    resolved:
        if (success != 0) {
            if ((((node_type < 4) || (0x30 < node_type)) && (node_type != 3)) ||
                ((success = hs_types_are_compatible(node->type, node_type)), success == 0)) {
                hs_compile_error = "type is inconsistent with usage (you need to recompile scripts.)";
                goto fail;
            }
            success = 1;
        }
        goto advance;

    fail:
        success = 0;

    advance:
        next_start = (int32_t)(current & 0xffff) + 1;
        current = k_datum_index_none;
        next_index = (int16_t)next_start;
        if (0 <= next_index && next_index < nodes->last_index) {
            scan = (hs_syntax_node *)((uint8_t *)nodes->data + (int32_t)next_index * nodes->size);
            for (;;) {
                if (scan->identifier != 0) {
                    current = ((uint32_t)(uint16_t)scan->identifier << 16) | (uint16_t)next_index;
                    break;
                }
                next_start = next_start + 1;
                scan = (hs_syntax_node *)((uint8_t *)scan + nodes->size);
                next_index = (int16_t)next_start;
                if (nodes->last_index <= next_index) {
                    break;
                }
            }
        }
    } while (success != 0);

    *error_message = hs_compile_error;
    if (hs_compile_error_offset != -1) {
        *error_offset = hs_compile_error_offset + (int32_t)hs_compiled_source;
    }
    hs_compiled_source = 0;
    hs_compile_error = 0;
    hs_postprocessing = 0;
    return 0;
}

#if 0
Original Ghidra decompilation (0x4858c0):

char hs_compile_postprocess(undefined4 *param_1,int *param_2)

{
  int iVar1;
  bool bVar2;
  char cVar3;
  char cVar4;
  short sVar5;
  uint uVar6;
  short *psVar7;
  int in_ECX;
  int iVar8;
  int iVar9;
  int local_4;

  iVar9 = DAT_0087a474;
  DAT_006b14c0 = *(int *)(DAT_00746f8c + 0x494);
  DAT_006b14bc = *(int *)(DAT_00746f8c + 0x488) + -0x400;
  cVar3 = '\x01';
  DAT_006b14d4 = (char *)0x0;
  DAT_006b15e0 = 1;
  *param_1 = 0;
  *param_2 = 0;
  uVar6 = FUN_004d0630();
  local_4 = in_ECX;
  do {
    if (uVar6 == 0xffffffff) {
      if (cVar3 != '\0') {
        DAT_006b14c0 = 0;
        DAT_006b14d4 = (char *)0x0;
        DAT_006b15e0 = 0;
        return cVar3;
      }
      break;
    }
    iVar1 = *(int *)(iVar9 + 0x34);
    sVar5 = *(short *)(iVar1 + 4 + (uVar6 & 0xffff) * 0x14);
    iVar8 = iVar1 + (uVar6 & 0xffff) * 0x14;
    if ((sVar5 < 4) || (0x30 < sVar5)) {
      if (sVar5 != 2) {
        DAT_006b14d4 = "missing type (you need to recompile scripts.)";
        goto LAB_00485ad5;
      }
    }
    else if ((*(byte *)(iVar8 + 6) & 1) == 0) {
      if ((*(byte *)(iVar8 + 6) & 2) == 0) {
        if ((*(uint *)(iVar8 + 0x10) == 0xffffffff) ||
           (*(short *)(iVar1 + 4 + (*(uint *)(iVar8 + 0x10) & 0xffff) * 0x14) != 2)) {
          DAT_006b14d4 = "corrupt syntax tree (you need to recompile scripts.)";
        }
        else {
          cVar4 = hs_verify_source_offset();
          if (cVar4 != '\0') {
            sVar5 = hs_find_function_by_name();
            iVar9 = DAT_0087a474;
            if (sVar5 != -1) {
              *(short *)(iVar8 + 2) = sVar5;
              sVar5 = *(short *)(&PTR_DAT_00688b58)[sVar5];
              goto LAB_004859b1;
            }
            DAT_006b14d4 = "missing function (you need to recompile scripts.)";
          }
        }
      }
      else {
        sVar5 = *(short *)(iVar8 + 2);
        if ((((-1 < sVar5) && ((int)sVar5 < *(int *)(DAT_00746f8c + 0x49c))) &&
            (local_4 = sVar5 * 0x5c + *(int *)(DAT_00746f8c + 0x4a0),
            *(short *)(local_4 + 0x20) == 3)) || (*(short *)(local_4 + 0x20) == 4)) {
          sVar5 = *(short *)(local_4 + 0x22);
          goto LAB_004859b1;
        }
        DAT_006b14d4 = "bad script index (you need to recompile.)";
      }
LAB_00485ad5:
      cVar3 = '\0';
    }
    else {
      if ((sVar5 < 9) && ((*(byte *)(iVar8 + 6) & 4) == 0)) {
LAB_0048599c:
        if ((cVar3 == '\0') || ((*(byte *)(iVar8 + 6) & 4) == 0)) goto LAB_004859e7;
        sVar5 = FUN_00483420();
      }
      else {
        bVar2 = true;
        if ((*(int *)(iVar8 + 0xc) < 0) || (DAT_006b14bc <= *(int *)(iVar8 + 0xc))) {
          DAT_006b14d4 = "bad source offset (you need to recompile.)";
          bVar2 = false;
        }
        cVar3 = '\0';
        if (bVar2) {
          cVar3 = hs_parse_primitive();
          iVar9 = DAT_0087a474;
          goto LAB_0048599c;
        }
LAB_004859e7:
        sVar5 = *(short *)(iVar8 + 2);
      }
LAB_004859b1:
      if (cVar3 != '\0') {
        if ((((sVar5 < 4) || (0x30 < sVar5)) && (sVar5 != 3)) ||
           (cVar3 = hs_types_are_compatible(), cVar3 == '\0')) {
          DAT_006b14d4 = "type is inconsistent with usage (you need to recompile scripts.)";
          goto LAB_00485ad5;
        }
        cVar3 = '\x01';
      }
    }
    iVar8 = uVar6 + 1;
    uVar6 = 0xffffffff;
    sVar5 = (short)iVar8;
    if ((-1 < sVar5) && (sVar5 < *(short *)(iVar9 + 0x2e))) {
      psVar7 = (short *)((int)sVar5 * (int)*(short *)(iVar9 + 0x22) + *(int *)(iVar9 + 0x34));
      do {
        if (*psVar7 != 0) {
          uVar6 = (int)*psVar7 << 0x10 | (int)(short)iVar8;
          break;
        }
        iVar8 = iVar8 + 1;
        psVar7 = (short *)((int)psVar7 + (int)*(short *)(iVar9 + 0x22));
      } while ((short)iVar8 < *(short *)(iVar9 + 0x2e));
    }
  } while (cVar3 != '\0');
  *param_1 = DAT_006b14d4;
  if (DAT_006b14d8 != -1) {
    *param_2 = DAT_006b14d8 + DAT_006b14c0;
  }
  DAT_006b14c0 = 0;
  DAT_006b14d4 = (char *)0x0;
  DAT_006b15e0 = 0;
  return '\0';
}
#endif
