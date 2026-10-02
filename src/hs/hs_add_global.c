// hs_add_global  (Ghidra: hs_add_global, already named)
// address 0x485b60, size 484 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: CEA-PDB string match. Requires exactly four children under the call node:
// identifier ("global"), a type keyword, a name, and an initial-value expression -- matching
// string_table_index_of(0x31, hs_type_names, <type keyword text>) validating against the same table
// hs_type_names uses, and the "less than 32 characters" / "already a variable" checks reading
// the *third* child's text as the name (not the fourth, which is the value expression parsed
// last). The "<type>" in the error message is documentation notation for a type keyword token
// (matching hs_parse_if's identical "<condition>"/"<then>" notation), not a literal
// "global<type>" single token.
// register convention: node_index is unrecognized by Ghidra (in_EAX); by the blam-cc
// convention this is the first register slot, EAX.
// RESOLVED (was UNSURE): this function always returns false, and it reports
// "i couldn't allocate space for this global." precisely when hs_parse SUCCEEDS. That reads as a
// contradiction in the decompile but is exactly what the retail bytes do. Disassembly of
// 0x485c6d..0x485cad:
//     call 0x486420            ; hs_parse
//     test al, al
//     je   0x485c9a            ; parse failed -> keep hs_parse's own error message
//     mov  [0x6b14d4], 0x6657c0 ; "i couldn't allocate space for this global."
//     mov  [0x6b14d8], <node->source_offset>
//   0x485c9a:
//     xor  al, al              ; <-- the ONLY value ever returned on this path
//     mov  [0x6b15de], al      ; hs_blocking_forbidden = 0
//     mov  [0x6b15df], al      ; hs_set_forbidden = 0
//     ret
// Every other return in the function is likewise a bare `xor al,al`, so hs_add_global has no
// success path at all. The reading that makes this consistent: retail halo.exe has no runtime
// storage to allocate a new script global into -- Scenario::globals is a fixed tag block baked by
// tool/Sapien, and hs_runtime_initialize sizes hs_globals_data from it -- so the runtime compiler
// validates a (global ...) form completely (shape, type keyword, name length, name collision, and
// the initializer expression via hs_parse) and then always fails with the allocation error.
// Globals in shipped Halo scripts therefore never go through this function; they arrive already
// compiled inside the scenario tag. This is the runtime-source path only (console / hs_doc-style
// compilation), which is why the failure is invisible in normal play.
// hs_add_script @0x485d50 is the contrast case and confirms the polarity of hs_parse's result:
// there, `test al,al / jne` takes the SUCCESS branch, which finishes the script record and
// returns CONCAT31(...,1) == true; only its failure path falls into a `xor al,al`.
// UNSURE: the type keyword's search text passed to string_table_index_of is not itself visible in
// Ghidra's decompile of this function (zero-argument call, register-passed); modeled as the
// type-keyword token's own text (hs_compiled_source + type_node->source_offset), which is what
// a direct hs_type_names lookup requires.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t string_table_index_of(const char *search, int16_t count, const char **table); // 0x004875c0, library-ish (cseries/text), not this module; blam-cc: search_text in EAX
extern hs_global_reference hs_find_global_by_name(char *name); // 0x00483480, this batch
extern char hs_parse(datum_index node_index, hs_type_t expected_type); // 0x00486420, this batch

extern data_array *hs_syntax_data;           // 0x0087a474
extern char *hs_compiled_source;             // 0x006b14c0
extern char *hs_type_names[k_hs_type_count]; // 0x00688a78
extern uint8_t hs_blocking_forbidden;        // 0x006b15de
extern uint8_t hs_set_forbidden;             // 0x006b15df
extern char *hs_compile_error;               // 0x006b14d4
extern int32_t hs_compile_error_offset;      // 0x006b14d8

// blam-cc: node index in EAX
// Validates a (global <type> <name> <value>) declaration: shape, type keyword, name length,
// name collision, and the initializer expression. Always returns false -- retail has no
// runtime storage to register the global into (see the RESOLVED note above), so a fully
// valid form still fails with "i couldn't allocate space for this global.".
char hs_add_global(datum_index node_index)
{
    data_array *nodes;
    hs_syntax_node *node;
    datum_index identifier_index;
    hs_syntax_node *identifier_node;
    datum_index type_index;
    hs_syntax_node *type_node;
    datum_index name_index;
    hs_syntax_node *name_node;
    datum_index value_index;
    hs_syntax_node *value_node;
    int16_t type_ordinal;
    char *name_text;
    size_t name_length;
    hs_global_reference existing;

    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & 0xffff) * nodes->size);
    identifier_index = node->data.first_child;
    if (identifier_index != k_datum_index_none) {
        identifier_node = (hs_syntax_node *)((uint8_t *)nodes->data + (identifier_index & 0xffff) * nodes->size);
        type_index = identifier_node->next_node;
        if (type_index != k_datum_index_none) {
            type_node = (hs_syntax_node *)((uint8_t *)nodes->data + (type_index & 0xffff) * nodes->size);
            name_index = type_node->next_node;
            if (name_index != k_datum_index_none) {
                name_node = (hs_syntax_node *)((uint8_t *)nodes->data + (name_index & 0xffff) * nodes->size);
                value_index = name_node->next_node;
                if (value_index != k_datum_index_none) {
                    value_node = (hs_syntax_node *)((uint8_t *)nodes->data + (value_index & 0xffff) * nodes->size);
                    if (value_node->next_node == k_datum_index_none) {
                        type_ordinal = string_table_index_of(hs_compiled_source + type_node->source_offset,
                                                     k_hs_type_count, (const char **)hs_type_names);
                        if ((type_ordinal < 4) || (0x30 < type_ordinal)) {
                            hs_compile_error = (char *)"this is not a valid type.";
                            hs_compile_error_offset = type_node->source_offset;
                            return 0;
                        }
                        name_text = hs_compiled_source + name_node->source_offset;
                        name_length = strlen(name_text);
                        if ((name_length != 0) && (name_length < 0x20)) {
                            existing = hs_find_global_by_name(name_text);
                            if (existing == k_hs_global_reference_none) {
                                hs_blocking_forbidden = 1;
                                hs_set_forbidden = 1;
                                if (hs_parse(value_index, type_ordinal) != 0) {
                                    // The form is completely valid -- and there is still nowhere
                                    // to put it. See the RESOLVED note at the top of the file.
                                    hs_compile_error = (char *)"i couldn't allocate space for this global.";
                                    hs_compile_error_offset = node->source_offset;
                                }
                                hs_blocking_forbidden = 0;
                                hs_set_forbidden = 0;
                                return 0;
                            }
                            hs_compile_error = (char *)"there is already a variable by this name.";
                            hs_compile_error_offset = name_node->source_offset;
                            return 0;
                        }
                        hs_compile_error = (char *)"i expected a global variable name less than 32 characters.";
                        hs_compile_error_offset = name_node->source_offset;
                        return 0;
                    }
                }
            }
        }
    }
    hs_compile_error = (char *)"i expected (global<type> <name> <initial value>)";
    hs_compile_error_offset = node->source_offset;
    return 0;
}

#if 0
Original Ghidra decompilation (0x485b60):

uint hs_add_global(void)

{
  char cVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  uint in_EAX;
  undefined4 uVar5;
  char *pcVar6;
  uint uVar7;
  uint uVar8;
  int iVar9;
  char *pcVar10;
  int iVar11;

  iVar3 = DAT_006b14c0;
  iVar2 = *(int *)(DAT_0087a474 + 0x34);
  uVar8 = (in_EAX & 0xffff) * 0x14;
  uVar7 = *(uint *)(iVar2 + 8 + (*(uint *)(iVar2 + 0x10 + uVar8) & 0xffff) * 0x14);
  if (uVar7 != 0xffffffff) {
    iVar11 = (uVar7 & 0xffff) * 0x14;
    uVar7 = *(uint *)(iVar2 + 8 + iVar11);
    if (uVar7 != 0xffffffff) {
      iVar9 = (uVar7 & 0xffff) * 0x14;
      uVar7 = *(uint *)(iVar2 + 8 + iVar9);
      if ((uVar7 != 0xffffffff) && (*(int *)(iVar2 + 8 + (uVar7 & 0xffff) * 0x14) == -1)) {
        uVar5 = FUN_004875c0(0x31,&PTR_s_unparsed_00688a78);
        if (((short)uVar5 < 4) || (0x30 < (short)uVar5)) {
          DAT_006b14d4 = "this is not a valid type.";
          DAT_006b14d8 = *(undefined4 *)(*(uint *)(DAT_0087a474 + 0x34) + 0xc + iVar11);
          return *(uint *)(DAT_0087a474 + 0x34) & 0xffffff00;
        }
        pcVar10 = (char *)(*(int *)(iVar2 + 0xc + iVar9) + iVar3);
        pcVar6 = pcVar10;
        do {
          cVar1 = *pcVar6;
          pcVar6 = pcVar6 + 1;
        } while (cVar1 != '\0');
        if (pcVar6 != pcVar10 + 1) {
          pcVar6 = pcVar10 + 1;
          do {
            cVar1 = *pcVar10;
            pcVar10 = pcVar10 + 1;
          } while (cVar1 != '\0');
          if ((uint)((int)pcVar10 - (int)pcVar6) < 0x20) {
            sVar4 = chimera__get_global_index();
            if (sVar4 == -1) {
              DAT_006b15de = 1;
              DAT_006b15df = 1;
              uVar7 = hs_parse(uVar7,uVar5);
              if ((char)uVar7 != '\0') {
                DAT_006b14d4 = "i couldn\'t allocate space for this global.";
                DAT_006b14d8 = *(undefined4 *)(*(int *)(DAT_0087a474 + 0x34) + 0xc + uVar8);
                uVar7 = uVar8;
              }
              DAT_006b15de = 0;
              DAT_006b15df = 0;
              return uVar7 & 0xffffff00;
            }
            DAT_006b14d4 = "there is already a variable by this name.";
            DAT_006b14d8 = *(undefined4 *)(*(uint *)(DAT_0087a474 + 0x34) + 0xc + iVar9);
            return *(uint *)(DAT_0087a474 + 0x34) & 0xffffff00;
          }
        }
        DAT_006b14d4 = "i expected a global variable name less than 32 characters.";
        DAT_006b14d8 = *(undefined4 *)(*(uint *)(DAT_0087a474 + 0x34) + 0xc + iVar9);
        return *(uint *)(DAT_0087a474 + 0x34) & 0xffffff00;
      }
    }
  }
  DAT_006b14d4 = "i expected (global<type> <name> <initial value>)";
  DAT_006b14d8 = *(undefined4 *)(*(uint *)(DAT_0087a474 + 0x34) + 0xc + uVar8);
  return *(uint *)(DAT_0087a474 + 0x34) & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
