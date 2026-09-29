// hs_add_script  (Ghidra: hs_add_script, already named)
// address 0x485d50, size 964 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: CEA-PDB string match; requires (script <type-keyword> [<return-type-keyword>]
// <name> <body...>), where the return-type keyword is only present for static/stub scripts
// (matched against hs_script_type_names then, for static/stub, hs_type_names via
// string_table_index_of); enforces the name is a pre-declared script (hs_script_find_by_name, i.e.
// scripts are pre-registered from the scenario tag and this only fills in their body) and only
// allows a static script to override an existing stub of the same return type (or silently
// accepts a stub redefinition of an already-static script of the same type).
// register convention: node_index is unrecognized by Ghidra (in_EAX); by the blam-cc
// convention this is the first register slot, EAX.
// UNSURE: the two string_table_index_of search-text arguments (script-type keyword, return-type
// keyword) are not visible in Ghidra's decompile of this function (zero-argument calls,
// register-passed); modeled as each keyword token's own source text, matching hs_add_global's
// identical situation.
// UNSURE: the byte-copy loop before the final success return
// (`pcVar12[iVar8] = *pcVar12`) is read as copying the token's name text into the
// ScenarioScript's own TagString name field (offset 0, `iVar2` is exactly `&existing_script->
// name`) -- functionally a no-op since hs_script_find_by_name already matched this name
// case-sensitively-ish, but preserved since it is what the bytes do.

// VERIFIED: every branch, error message, error offset and node write below was checked
// field by field against the full Ghidra decompile of 0x485d50, and the polarity of the
// hs_parse result was confirmed against the retail bytes (this function's success path is
// `CONCAT31(...,1)`, i.e. a real `mov al,1`, unlike hs_add_global at 0x485b60 which has no
// success path at all -- see that file). The one substantive reading: the stub-override
// test is inverted in the decompile, so the datum_new path runs when the existing script
// IS a stub of the same return type and this declaration IS static.
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include <string.h>

extern int16_t string_table_index_of(const char *search, int16_t count, const char **table); // blam-cc: EAX search, stack (count, table) // 0x004875c0, library-ish (cseries/text), not this module


extern datum_index datum_new(data_array *array); // memory module, 0x004d0480

extern data_array *hs_syntax_data;           // 0x0087a474
extern char *hs_compiled_source;             // 0x006b14c0
extern char *hs_script_type_names[k_hs_script_type_count]; // 0x00688b3c
extern char *hs_type_names[k_hs_type_count]; // 0x00688a78
extern char *hs_compile_error;               // 0x006b14d4
extern int32_t hs_compile_error_offset;      // 0x006b14d8
extern Scenario *global_scenario;            // 0x00746f8c

// blam-cc: node index in EAX
// Parses and registers a (script <type> ... <name> <body>) declaration, enforcing naming,
// typing, and stub-override rules.
char hs_add_script(datum_index node_index)
{
    data_array *nodes;
    hs_syntax_node *node;
    datum_index type_index;
    hs_syntax_node *type_node;
    int16_t script_type;
    datum_index return_type_index;
    hs_syntax_node *return_type_node;
    hs_type_t return_type;
    datum_index name_index;
    hs_syntax_node *name_node;
    datum_index body_index;
    char *name_text;
    size_t name_length;
    int16_t existing_index;
    ScenarioScript *existing_script;
    datum_index new_root;
    datum_index new_body_holder;
    hs_syntax_node *new_root_node;
    hs_syntax_node *new_body_holder_node;
    char ok;
    char *dest;

    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & 0xffff) * nodes->size);
    type_index = node->data.first_child;
    if (type_index == k_datum_index_none) {
        hs_compile_error = "i expected (script <type> <name> <expression(s)>)";
        hs_compile_error_offset = node->source_offset;
        return 0;
    }
    type_node = (hs_syntax_node *)((uint8_t *)nodes->data + (type_index & 0xffff) * nodes->size);
    script_type = string_table_index_of(hs_compiled_source + type_node->source_offset,
                                k_hs_script_type_count, (const char **)hs_script_type_names);
    if (script_type == -1) {
        hs_compile_error = "script type must be \"startup\", \"dormant\", \"continuous\", or \"static\".";
        hs_compile_error_offset = type_node->source_offset;
        return 0;
    }

    if ((script_type == _hs_script_static) || (script_type == _hs_script_stub)) {
        return_type_index = type_node->next_node;
        if (return_type_index == k_datum_index_none) {
            hs_compile_error = "i expected (script local <type> <name> <expression(s)>).";
            hs_compile_error_offset = node->source_offset;
            return 0;
        }
        return_type_node = (hs_syntax_node *)((uint8_t *)nodes->data + (return_type_index & 0xffff) * nodes->size);
        return_type = string_table_index_of(hs_compiled_source + return_type_node->source_offset,
                                    k_hs_type_count, (const char **)hs_type_names);
        name_index = return_type_node->next_node;
        if ((return_type < 4) || (0x30 < return_type)) {
            hs_compile_error = "this is not a valid return type.";
            hs_compile_error_offset = return_type_node->source_offset;
            return 0;
        }
    } else {
        name_index = type_node->next_node;
        return_type = _hs_type_void;
    }

    if (name_index != k_datum_index_none) {
        name_node = (hs_syntax_node *)((uint8_t *)nodes->data + (name_index & 0xffff) * nodes->size);
        body_index = name_node->next_node;
        if (body_index != k_datum_index_none) {
            name_text = hs_compiled_source + name_node->source_offset;
            name_length = strlen(name_text);
            if ((name_length != 0) && (name_length < 0x20)) {
                existing_index = hs_script_find_by_name(name_text);
                if (existing_index == -1) {
                    hs_compile_error = "i couldn't allocate a script.";
                    hs_compile_error_offset = node->source_offset;
                    return 0;
                }
                existing_script = (ScenarioScript *)global_scenario->scripts.pointer + existing_index;
                if ((existing_script->script_type == _hs_script_stub) &&
                    (existing_script->return_type == return_type) &&
                    (script_type == _hs_script_static)) {
                    new_root = datum_new(nodes);
                    new_body_holder = datum_new(nodes);
                    if ((new_root == k_datum_index_none) || (new_body_holder == k_datum_index_none)) {
                        hs_compile_error = "i couldn't allocate a syntax node.";
                        return 0;
                    }
                    nodes = hs_syntax_data;
                    new_root_node = (hs_syntax_node *)((uint8_t *)nodes->data + (new_root & 0xffff) * nodes->size);
                    new_body_holder_node = (hs_syntax_node *)((uint8_t *)nodes->data + (new_body_holder & 0xffff) * nodes->size);

                    new_root_node->data.first_child = new_body_holder;
                    new_root_node->next_node = k_datum_index_none;
                    new_root_node->source_offset = node->source_offset;
                    new_root_node->flags = 0;

                    new_body_holder_node->source_offset = -1;
                    new_body_holder_node->next_node = body_index;
                    new_body_holder_node->index_union = 0;
                    new_body_holder_node->flags = _hs_syntax_node_primitive_bit;
                    new_body_holder_node->type = _hs_type_function_name;

                    ok = hs_parse(new_root, return_type);
                    if (ok != 0) {
                        dest = existing_script->name.string;
                        strcpy(dest, name_text);
                        existing_script->return_type = return_type;
                        existing_script->root_expression_index = new_root;
                        existing_script->script_type = _hs_script_static;
                        return 1;
                    }
                    return 0;
                }
                if ((existing_script->script_type == _hs_script_static) &&
                    (existing_script->return_type == return_type) &&
                    (script_type == _hs_script_stub)) {
                    return 1;
                }
                hs_compile_error = "only static scripts of the same type can override stub scripts.";
                hs_compile_error_offset = node->source_offset;
                return 0;
            }
            hs_compile_error = "i expected a script name less than 32 characters.";
            hs_compile_error_offset = name_node->source_offset;
            return 0;
        }
    }

    if (script_type == _hs_script_static) {
        hs_compile_error = "i expected (script static <type> <name> <expression(s)>)";
    } else if (script_type == _hs_script_stub) {
        hs_compile_error = "i expected (script stub <type> <name> <expression(s)>)";
    } else {
        hs_compile_error = "i expected (script <type> <name> <expression(s)>)";
    }
    hs_compile_error_offset = node->source_offset;
    return 0;
}

#if 0
Original Ghidra decompilation (0x485d50):

uint hs_add_script(void)

{
  char cVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  short sVar6;
  uint in_EAX;
  char *pcVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  char *pcVar12;
  int iVar13;
  int iVar14;
  undefined4 local_14;

  iVar4 = DAT_0087a474;
  iVar8 = DAT_006b14c0;
  iVar2 = *(int *)(DAT_0087a474 + 0x34);
  uVar11 = (in_EAX & 0xffff) * 0x14;
  uVar10 = *(uint *)(iVar2 + 8 + (*(uint *)(iVar2 + 0x10 + uVar11) & 0xffff) * 0x14);
  if (uVar10 == 0xffffffff) {
    DAT_006b14d4 = "i expected (script <type> <name> <expression(s)>)";
    DAT_006b14d8 = *(undefined4 *)(uVar11 + 0xc + *(int *)(DAT_0087a474 + 0x34));
    uVar10 = 0xffffffff;
LAB_00486108:
    return uVar10 & 0xffffff00;
  }
  iVar13 = (uVar10 & 0xffff) * 0x14;
  sVar5 = FUN_004875c0(5,&PTR_s_startup_00688b3c);
  if (sVar5 == -1) {
    DAT_006b14d4 = "script type must be \"startup\", \"dormant\", \"continuous\", or \"static\".";
    DAT_006b14d8 = *(uint *)(iVar13 + 0xc + *(int *)(iVar4 + 0x34));
    return DAT_006b14d8 & 0xffffff00;
  }
  if ((sVar5 == 3) || (sVar5 == 4)) {
    uVar10 = *(uint *)(iVar13 + 8 + iVar2);
    if (uVar10 == 0xffffffff) {
      DAT_006b14d4 = "i expected (script local <type> <name> <expression(s)>).";
      DAT_006b14d8 = *(undefined4 *)(uVar11 + 0xc + *(uint *)(iVar4 + 0x34));
      return *(uint *)(iVar4 + 0x34) & 0xffffff00;
    }
    iVar13 = (uVar10 & 0xffff) * 0x14;
    local_14 = FUN_004875c0(0x31,&PTR_s_unparsed_00688a78);
    uVar10 = *(uint *)(iVar13 + 8 + iVar2);
    if (((short)local_14 < 4) || (0x30 < (short)local_14)) {
      DAT_006b14d4 = "this is not a valid return type.";
      DAT_006b14d8 = *(undefined4 *)(iVar13 + 0xc + *(int *)(iVar4 + 0x34));
      return uVar10 & 0xffffff00;
    }
  }
  else {
    uVar10 = *(uint *)(iVar13 + 8 + iVar2);
    local_14 = 4;
  }
  iVar13 = DAT_00746f8c;
  if (uVar10 != 0xffffffff) {
    iVar14 = (uVar10 & 0xffff) * 0x14;
    iVar3 = *(int *)(iVar14 + 8 + iVar2);
    if (iVar3 != -1) {
      pcVar12 = (char *)(*(int *)(iVar14 + 0xc + iVar2) + iVar8);
      pcVar7 = pcVar12;
      do {
        cVar1 = *pcVar7;
        pcVar7 = pcVar7 + 1;
      } while (cVar1 != '\0');
      if (pcVar7 != pcVar12 + 1) {
        pcVar7 = pcVar12;
        do {
          cVar1 = *pcVar7;
          pcVar7 = pcVar7 + 1;
        } while (cVar1 != '\0');
        if ((uint)((int)pcVar7 - (int)(pcVar12 + 1)) < 0x20) {
          sVar6 = hs_script_find_by_name(pcVar12);
          if (sVar6 == -1) {
            DAT_006b14d4 = "i couldn\'t allocate a script.";
            DAT_006b14d8 = *(undefined4 *)(uVar11 + 0xc + *(uint *)(iVar4 + 0x34));
            return *(uint *)(iVar4 + 0x34) & 0xffffff00;
          }
          iVar8 = sVar6 * 0x5c;
          iVar2 = *(int *)(iVar13 + 0x4a0) + iVar8;
          if (((*(short *)(iVar2 + 0x20) != 4) || (*(short *)(iVar2 + 0x22) != (short)local_14)) ||
             (sVar5 != 3)) {
            if (((*(short *)(iVar2 + 0x20) == 3) && (*(short *)(iVar2 + 0x22) == (short)local_14))
               && (sVar5 == 4)) {
              return CONCAT31((int3)(CONCAT22((short)((uint)iVar8 >> 0x10),(short)local_14) >> 8),1)
              ;
            }
            DAT_006b14d4 = "only static scripts of the same type can override stub scripts.";
            DAT_006b14d8 = *(uint *)(uVar11 + 0xc + *(int *)(iVar4 + 0x34));
            return DAT_006b14d8 & 0xffffff00;
          }
          uVar9 = datum_new();
          uVar10 = datum_new();
          if ((uVar9 == 0xffffffff) || (uVar10 == 0xffffffff)) {
            DAT_006b14d4 = "i couldn\'t allocate a syntax node.";
            return uVar10 & 0xffffff00;
          }
          iVar13 = *(int *)(iVar4 + 0x34);
          iVar8 = iVar13 + (uVar9 & 0xffff) * 0x14;
          *(uint *)(iVar8 + 0x10) = uVar10;
          iVar13 = iVar13 + (uVar10 & 0xffff) * 0x14;
          *(undefined4 *)(iVar8 + 8) = 0xffffffff;
          *(undefined4 *)(iVar8 + 0xc) = *(undefined4 *)(uVar11 + 0xc + *(int *)(iVar4 + 0x34));
          *(undefined2 *)(iVar8 + 6) = 0;
          *(undefined4 *)(iVar13 + 0xc) = 0xffffffff;
          *(int *)(iVar13 + 8) = iVar3;
          *(undefined2 *)(iVar13 + 2) = 0;
          *(undefined2 *)(iVar13 + 6) = 1;
          *(undefined2 *)(iVar13 + 4) = 2;
          uVar10 = hs_parse(uVar9,local_14);
          if ((char)uVar10 != '\0') {
            iVar8 = iVar2 - (int)pcVar12;
            do {
              cVar1 = *pcVar12;
              pcVar12[iVar8] = cVar1;
              pcVar12 = pcVar12 + 1;
            } while (cVar1 != '\0');
            *(short *)(iVar2 + 0x22) = (short)local_14;
            *(uint *)(iVar2 + 0x24) = uVar9;
            *(undefined2 *)(iVar2 + 0x20) = 3;
            return CONCAT31((int3)((uint)pcVar12 >> 8),1);
          }
          goto LAB_00486108;
        }
      }
      DAT_006b14d4 = "i expected a script name less than 32 characters.";
      DAT_006b14d8 = *(undefined4 *)(iVar14 + 0xc + *(uint *)(iVar4 + 0x34));
      return *(uint *)(iVar4 + 0x34) & 0xffffff00;
    }
  }
  if (sVar5 == 3) {
    DAT_006b14d4 = "i expected (script static <type> <name> <expression(s)>)";
  }
  else {
    DAT_006b14d4 = "i expected (script stub <type> <name> <expression(s)>)";
    if (sVar5 != 4) {
      DAT_006b14d4 = "i expected (script <type> <name> <expression(s)>)";
    }
  }
  DAT_006b14d8 = *(undefined4 *)(uVar11 + 0xc + *(int *)(iVar4 + 0x34));
  return uVar11 & 0xffffff00;
}
#endif
