// hs_parse_two_numeric_arguments  (Ghidra: FUN_00485060, renamed)
// address 0x485060, size 236 bytes
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: hs_functions.md: "Type-check callback that forces a function's two arguments to
// resolve to a single shared (numeric) type." Matches the hs_function_definition::parse
// signature (int16_t function_index, datum_index node); requires exactly 2 arguments via
// hs_get_parameter_indices, parses the first with no expected type, then parses the second
// against whatever type the first resolved to (or, failing that, tries the reverse, falling
// back to _hs_type_real for the first argument if neither resolves on its own).
// register convention: function_index is recognized directly by Ghidra; node_index is not
// shown as a parameter at all (this function's own decompile has no register/stack trace for
// it), but it is required by hs_get_parameter_indices's own node argument (ECX) and by every
// call site's (function_index, node) pair used elsewhere in this module for the same
// hs_function_definition::parse slot -- modeled here as the second parameter to match.
// UNSURE: no caller of this address exists in the recovered call graph (it is reached only
// through an hs_function_definitions::parse slot that was not identified in this batch), so
// which script function(s) actually use this callback is not established.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char hs_get_parameter_indices(char *function_name, int16_t required_count, datum_index node_index, datum_index *out_indices); // 0x00484fb0, this batch
extern char hs_parse(datum_index node_index, hs_type_t expected_type); // 0x00486420, this batch

extern data_array *hs_syntax_data; // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern char *hs_compile_error; // 0x006b14d4

// Requires exactly two arguments and parses them so both resolve to the same type: the second
// argument's expected type follows whichever of the two parses first, falling back to
// _hs_type_real for the first argument if neither parses without an expected type.
char hs_parse_two_numeric_arguments(int16_t function_index, datum_index node_index)
{
    datum_index arguments[2];
    char ok;
    char result;
    hs_type_t type;

    result = 0;
    ok = hs_get_parameter_indices(hs_function_definitions[function_index]->name, 2, node_index, arguments);
    if (ok != 0) {
        ok = hs_parse(arguments[0], 0);
        if (ok == 0) {
            if (hs_compile_error == 0) {
                ok = hs_parse(arguments[1], 0);
                if (ok == 0) {
                    if (hs_compile_error != 0) {
                        return 0;
                    }
                    ok = hs_parse(arguments[0], _hs_type_real);
                    if (ok == 0) {
                        return 0;
                    }
                    type = _hs_type_real;
                    arguments[0] = arguments[1];
                } else {
                    type = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
                            (arguments[1] & 0xffff) * hs_syntax_data->size))->type;
                }
                ok = hs_parse(arguments[0], type);
                if (ok != 0) {
                    result = 1;
                }
            }
        } else {
            type = ((hs_syntax_node *)((uint8_t *)hs_syntax_data->data +
                    (arguments[0] & 0xffff) * hs_syntax_data->size))->type;
            ok = hs_parse(arguments[1], type);
            if (ok != 0) {
                return 1;
            }
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x485060):

undefined1 FUN_00485060(short param_1)

{
  char cVar1;
  int iVar2;
  undefined1 local_9;
  uint local_8;
  uint local_4;

  local_9 = 0;
  cVar1 = hs_get_parameter_indices(*(undefined4 *)((&PTR_DAT_00688b58)[param_1] + 4),2);
  if (cVar1 != '\0') {
    cVar1 = hs_parse(local_8,0);
    if (cVar1 == '\0') {
      if (DAT_006b14d4 == 0) {
        cVar1 = hs_parse(local_4,0);
        if (cVar1 == '\0') {
          if (DAT_006b14d4 != 0) {
            return 0;
          }
          cVar1 = hs_parse(local_8,6);
          if (cVar1 == '\0') {
            return 0;
          }
          iVar2 = 6;
          local_8 = local_4;
        }
        else {
          iVar2 = (int)*(short *)(*(int *)(DAT_0087a474 + 0x34) + 4 + (local_4 & 0xffff) * 0x14);
        }
        cVar1 = hs_parse(local_8,iVar2);
        if (cVar1 != '\0') {
          local_9 = 1;
        }
      }
    }
    else {
      cVar1 = hs_parse(local_4,(int)*(short *)(*(int *)(DAT_0087a474 + 0x34) + 4 +
                                              (local_8 & 0xffff) * 0x14));
      if (cVar1 != '\0') {
        return 1;
      }
    }
  }
  return local_9;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
