// hs_compile_expression  (Ghidra: hs_compile_expression, already named)
// address 0x485540, size 430 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: refuses text of k_hs_maximum_expression_length (0x400) or more; appends it to
// hs_compiled_source (a fresh GlobalAlloc'd buffer if no scenario is loaded, otherwise the
// scenario's own script_string_data tail, the same reuse pattern hs_compile_postprocess uses),
// tokenizes it, and wraps the result in a synthetic "(inspect <expr>)" call node (function
// index 0x16 == _hs_function_inspect, types/hs.h) so the caller can evaluate it immediately.
// register convention: text/error_message/error_offset are recognized directly by Ghidra;
// `length` is unrecognized (in_EAX), which by the blam-cc convention is the first register
// slot, EAX.
// UNSURE: the "iVar8 = high dword of datum_new's 64-bit return" pattern Ghidra shows after each
// datum_new call is read as a leaked hs_syntax_data pointer (identical pattern in
// hs_parse_cond_recursive); modeled here as a fresh read of the actual global instead.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


extern datum_index datum_new(data_array *array); // memory module, 0x004d0480


extern datum_index global_scenario_index; // 0x0069e8d4
extern Scenario *global_scenario;         // 0x00746f8c
extern uint8_t hs_compiled_source_owned;  // 0x006b15dc
extern char *hs_compiled_source;          // 0x006b14c0
extern int32_t hs_compiled_source_length; // 0x006b14bc
extern char *hs_compile_error;            // 0x006b14d4
extern int32_t hs_compile_error_offset;   // 0x006b14d8
extern data_array *hs_syntax_data;        // 0x0087a474

// blam-cc: text length in EAX
// Tokenizes and wraps a single standalone expression string into a "(inspect <expr>)"
// syntax-node tree ready for immediate evaluation, returning its root node index or
// k_datum_index_none on failure (with *error_message/*error_offset set, the latter relative to
// `text` itself).
datum_index hs_compile_expression(char *text, uint32_t length, char **error_message, char **error_offset)
{
    int32_t start;
    uint8_t *dest;
    char *src;
    uint32_t words;
    uint32_t tail_bytes;
    char *cursor;
    datum_index expr_index;
    hs_syntax_node *expr_node;
    data_array *nodes;
    datum_index wrap_index;
    datum_index inspect_index;
    hs_syntax_node *wrap_node;
    hs_syntax_node *inspect_node;
    char ok;

    if ((int32_t)length < k_hs_maximum_expression_length) {
        if (global_scenario_index == k_datum_index_none) {
            start = 0;
            hs_compiled_source = (char *)GlobalAlloc(0, length + 1);
            hs_compiled_source_owned = 1;
        } else {
            hs_compiled_source = (char *)global_scenario->script_string_data.pointer;
            start = (int32_t)global_scenario->script_string_data.size - 0x400;
        }
        dest = (uint8_t *)hs_compiled_source + start;
        src = text;
        for (words = length >> 2; words != 0; words = words - 1) {
            *(uint32_t *)dest = *(uint32_t *)src;
            src = src + 4;
            dest = dest + 4;
        }
        for (tail_bytes = length & 3; tail_bytes != 0; tail_bytes = tail_bytes - 1) {
            *dest = *src;
            src = src + 1;
            dest = dest + 1;
        }
        hs_compiled_source_length = (int32_t)length + start;
        hs_compiled_source[hs_compiled_source_length] = 0;
        hs_compile_error = 0;
        *error_message = 0;
        *error_offset = 0;
        hs_compile_error_offset = -1;
        cursor = hs_compiled_source + start;
        skip_whitespace(&cursor);
        if (*(hs_compiled_source + start) != '\0') {
            expr_index = hs_tokenize(&cursor);
            if (hs_compile_error == 0) {
                wrap_index = datum_new(hs_syntax_data);
                inspect_index = datum_new(hs_syntax_data);
                if ((wrap_index != k_datum_index_none) && (inspect_index != k_datum_index_none)) {
                    nodes = hs_syntax_data;
                    expr_node = (hs_syntax_node *)((uint8_t *)nodes->data + (expr_index & 0xffff) * nodes->size);
                    wrap_node = (hs_syntax_node *)((uint8_t *)nodes->data + (wrap_index & 0xffff) * nodes->size);
                    inspect_node = (hs_syntax_node *)((uint8_t *)nodes->data + (inspect_index & 0xffff) * nodes->size);

                    wrap_node->data.first_child = inspect_index;
                    wrap_node->next_node = k_datum_index_none;
                    wrap_node->source_offset = expr_node->source_offset;
                    wrap_node->flags = 0;

                    inspect_node->next_node = expr_index;
                    inspect_node->source_offset = -1;
                    inspect_node->index_union = _hs_function_inspect;
                    inspect_node->flags = _hs_syntax_node_primitive_bit;
                    inspect_node->type = _hs_type_function_name;

                    ok = hs_parse(wrap_index, _hs_type_void);
                    if (ok != 0) {
                        return wrap_index;
                    }
                }
            }
            *error_message = hs_compile_error;
            if (hs_compile_error_offset != -1) {
                hs_compile_error_offset = hs_compile_error_offset - start;
                *error_offset = hs_compile_error_offset + text;
            }
        }
    }
    return k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x485540):

uint hs_compile_expression(undefined4 *param_1,int *param_2,undefined4 *param_3)

{
  int iVar1;
  int iVar2;
  HGLOBAL pvVar3;
  char cVar4;
  uint in_EAX;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  undefined4 extraout_EDX;
  int iVar8;
  int iVar9;
  undefined4 *puVar10;
  undefined4 *puVar11;
  undefined8 uVar12;

  if ((int)in_EAX < 0x400) {
    if (DAT_0069e8d4 == -1) {
      iVar9 = 0;
      DAT_006b14c0 = GlobalAlloc(0,in_EAX + 1);
      DAT_006b15dc = 1;
    }
    else {
      DAT_006b14c0 = *(HGLOBAL *)(DAT_00746f8c + 0x494);
      iVar9 = *(int *)(DAT_00746f8c + 0x488) + -0x400;
    }
    puVar10 = param_1;
    puVar11 = (undefined4 *)((int)DAT_006b14c0 + iVar9);
    for (uVar7 = in_EAX >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
      *puVar11 = *puVar10;
      puVar10 = puVar10 + 1;
      puVar11 = puVar11 + 1;
    }
    for (uVar7 = in_EAX & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
      *(undefined1 *)puVar11 = *(undefined1 *)puVar10;
      puVar10 = (undefined4 *)((int)puVar10 + 1);
      puVar11 = (undefined4 *)((int)puVar11 + 1);
    }
    DAT_006b14bc = in_EAX + iVar9;
    *(undefined1 *)(DAT_006b14bc + (int)DAT_006b14c0) = 0;
    pvVar3 = DAT_006b14c0;
    DAT_006b14d4 = 0;
    *param_2 = 0;
    *param_3 = 0;
    DAT_006b14d8 = -1;
    skip_whitespace();
    if (*(char *)((int)pvVar3 + iVar9) != '\0') {
      uVar7 = hs_tokenize(extraout_EDX);
      if (DAT_006b14d4 == 0) {
        uVar5 = datum_new();
        uVar12 = datum_new();
        iVar8 = (int)((ulonglong)uVar12 >> 0x20);
        uVar6 = (uint)uVar12;
        if ((uVar5 != 0xffffffff) && (uVar6 != 0xffffffff)) {
          iVar2 = *(int *)(iVar8 + 0x34);
          iVar1 = iVar2 + (uVar5 & 0xffff) * 0x14;
          *(uint *)(iVar1 + 0x10) = uVar6;
          iVar2 = iVar2 + (uVar6 & 0xffff) * 0x14;
          *(undefined4 *)(iVar1 + 8) = 0xffffffff;
          *(undefined4 *)(iVar1 + 0xc) =
               *(undefined4 *)(*(int *)(iVar8 + 0x34) + 0xc + (uVar7 & 0xffff) * 0x14);
          *(undefined2 *)(iVar1 + 6) = 0;
          *(uint *)(iVar2 + 8) = uVar7;
          *(undefined4 *)(iVar2 + 0xc) = 0xffffffff;
          *(undefined2 *)(iVar2 + 2) = 0x16;
          *(undefined2 *)(iVar2 + 6) = 1;
          *(undefined2 *)(iVar2 + 4) = 2;
          cVar4 = hs_parse(uVar5,4);
          if (cVar4 != '\0') {
            return uVar5;
          }
        }
      }
      *param_2 = DAT_006b14d4;
      if (DAT_006b14d8 != -1) {
        DAT_006b14d8 = DAT_006b14d8 - iVar9;
        *param_3 = (undefined1 *)(DAT_006b14d8 + (int)param_1);
      }
    }
  }
  return 0xffffffff;
}
#endif
