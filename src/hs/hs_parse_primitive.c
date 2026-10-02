// hs_parse_primitive  (Ghidra: hs_parse_primitive, already named)
// address 0x486480, size 218 bytes
// name confidence: 0.9   rewrite confidence: 0.65
// evidence: CEA-PDB string match; rejects _hs_type_special_form and _hs_type_void expected
// types outright, otherwise tries hs_parse_variable first and falls back to
// hs_parse_primitive_procedures[type] (types/hs.h), reporting "currently unsupported" for a
// type with no parser registered.
// register convention: the node index is unrecognized by Ghidra (unaff_EDI); by the blam-cc
// convention this is the sixth register slot, EDI.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <stdio.h>

extern char hs_parse_variable(datum_index node_index); // 0x00486560, this batch

extern data_array *hs_syntax_data;           // 0x0087a474
extern char *hs_compile_error;               // 0x006b14d4
extern int32_t hs_compile_error_offset;      // 0x006b14d8
extern uint8_t hs_postprocessing;            // 0x006b15e0
extern void *hs_parse_primitive_procedures[k_hs_type_count]; // 0x0065b668
extern char *hs_type_names[k_hs_type_count]; // 0x00688a78
extern char hs_compile_error_buffer[k_hs_error_buffer_size]; // 0x006b14dc

// blam-cc: node index in EDI
// Parses/type-checks a primitive (non-list) syntax node as either a variable reference
// (tried first, unless postprocessing a non-global) or a typed constant via
// hs_parse_primitive_procedures.
char hs_parse_primitive(datum_index node_index)
{
    hs_syntax_node *node;
    hs_type_t node_type;
    char result;
    char (*procedure)(datum_index);

    node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * hs_syntax_data->size);
    node_type = node->type;
    result = 0;

    if (node_type == _hs_type_special_form) {
        hs_compile_error = (char *)"i expected a script or variable definition.";
        hs_compile_error_offset = node->source_offset;
        return result;
    }
    if (node_type == _hs_type_void) {
        hs_compile_error = (char *)"the value of this expression (in a <void> slot) can never be used.";
        hs_compile_error_offset = node->source_offset;
        return result;
    }

    if ((hs_postprocessing == 0) || ((node->flags & _hs_syntax_node_global_bit) != 0)) {
        result = hs_parse_variable(node_index);
        if (result != 0) {
            return result;
        }
    }
    node_type = node->type;
    if ((node_type != 0) && (hs_compile_error == 0) &&
        ((hs_postprocessing == 0) || ((node->flags & _hs_syntax_node_global_bit) == 0))) {
        procedure = (char (*)(datum_index))hs_parse_primitive_procedures[node_type];
        if (procedure != 0) {
            result = procedure(node_index);
            return result;
        }
        sprintf(hs_compile_error_buffer, "expressions of type %s are currently unsupported.", hs_type_names[node_type]);
        hs_compile_error = hs_compile_error_buffer;
        hs_compile_error_offset = node->source_offset;
        result = 0;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x486480):

uint hs_parse_primitive(void)

{
  int iVar1;
  short sVar2;
  uint uVar3;
  uint unaff_EDI;

  uVar3 = unaff_EDI & 0xffff;
  sVar2 = *(short *)(*(int *)(DAT_0087a474 + 0x34) + 4 + uVar3 * 0x14);
  iVar1 = *(int *)(DAT_0087a474 + 0x34) + uVar3 * 0x14;
  uVar3 = uVar3 * 5 & 0xffffff00;
  if (sVar2 == 1) {
    DAT_006b14d4 = "i expected a script or variable definition.";
    DAT_006b14d8 = *(undefined4 *)(iVar1 + 0xc);
    return uVar3;
  }
  if (sVar2 != 4) {
    if (((DAT_006b15e0 == '\0') || ((*(byte *)(iVar1 + 6) & 4) != 0)) &&
       (uVar3 = hs_parse_variable(), (char)uVar3 != '\0')) {
      return uVar3;
    }
    sVar2 = *(short *)(iVar1 + 4);
    if (((sVar2 != 0) && (DAT_006b14d4 == (undefined1 *)0x0)) &&
       ((DAT_006b15e0 == '\0' || ((*(byte *)(iVar1 + 6) & 4) == 0)))) {
      if (*(code **)(&DAT_0065b668 + sVar2 * 4) != (code *)0x0) {
        uVar3 = (**(code **)(&DAT_0065b668 + sVar2 * 4))();
        return uVar3;
      }
      uVar3 = _sprintf(&DAT_006b14dc,"expressions of type %s are currently unsupported.",
                       (&PTR_s_unparsed_00688a78)[sVar2]);
      DAT_006b14d4 = &DAT_006b14dc;
      DAT_006b14d8 = *(undefined4 *)(iVar1 + 0xc);
      uVar3 = uVar3 & 0xffffff00;
    }
    return uVar3;
  }
  DAT_006b14d4 = "the value of this expression (in a <void> slot) can never be used.";
  DAT_006b14d8 = *(undefined4 *)(iVar1 + 0xc);
  return uVar3;
}
#endif
