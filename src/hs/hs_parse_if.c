// hs_parse_if  (Ghidra: hs_parse_if, already named)
// address 0x484780, size 352 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: CEA-PDB string match ("i expected (if <condition> <then> [<else>])."); requires
// 2 or 3 children (condition, then, optional else), parses the condition as boolean, then
// unifies the type of "then" and "else" -- trying "then" first and falling back to parsing
// "else" first (with no constraint) if "then" alone can't resolve a type.
// register convention: __cdecl; param_1 (function_index) is unused, matching the
// hs_function_definition::parse signature (int16_t function_index, datum_index node).

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


extern data_array *hs_syntax_data;      // 0x0087a474
extern char *hs_compile_error;          // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8

// Parses/type-checks an (if <condition> <then> [<else>]) special-form syntax node.
char hs_parse_if(int16_t function_index, datum_index node_index)
{
    data_array *nodes;
    hs_syntax_node *node;
    hs_syntax_node *condition_node;
    hs_syntax_node *then_node;
    datum_index then_index;
    datum_index else_index;
    hs_syntax_node *else_node;
    char ok;
    hs_type_t resolved_type;

    (void)function_index;
    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & 0xffff) * nodes->size);

    condition_node = 0;
    then_index = k_datum_index_none;
    else_index = k_datum_index_none;
    if (node->data.first_child != k_datum_index_none) {
        condition_node = (hs_syntax_node *)((uint8_t *)nodes->data + (node->data.first_child & 0xffff) * nodes->size);
        then_index = condition_node->next_node;
    }
    if ((condition_node != 0) && (then_index != k_datum_index_none)) {
        then_node = (hs_syntax_node *)((uint8_t *)nodes->data + (then_index & 0xffff) * nodes->size);
        else_index = then_node->next_node;
        if ((else_index == k_datum_index_none) ||
            (((hs_syntax_node *)((uint8_t *)nodes->data + (else_index & 0xffff) * nodes->size))->next_node == k_datum_index_none)) {
            ok = hs_parse(node->data.first_child, _hs_type_boolean);
            if (ok == 0) {
                return 0;
            }
            ok = hs_parse(then_index, node->type);
            if (ok != 0) {
                if (node->type == 0) {
                    node->type = then_node->type;
                }
                if (else_index != k_datum_index_none) {
                    ok = hs_parse(else_index, node->type);
                    if (ok == 0) {
                        return 0;
                    }
                }
                return 1;
            }
            if (hs_compile_error != 0) {
                return 0;
            }
            if (node->type != 0) {
                return 0;
            }
            if (else_index == k_datum_index_none) {
                return 0;
            }
            ok = hs_parse(else_index, 0);
            if (ok == 0) {
                return 0;
            }
            else_node = (hs_syntax_node *)((uint8_t *)nodes->data + (else_index & 0xffff) * nodes->size);
            resolved_type = else_node->type;
            node->type = resolved_type;
            return hs_parse(then_index, resolved_type);
        }
    }
    hs_compile_error = "i expected (if <condition> <then> [<else>]).";
    hs_compile_error_offset = node->source_offset;
    return 0;
}

#if 0
Original Ghidra decompilation (0x484780):

undefined1 hs_parse_if(undefined4 param_1,uint param_2)

{
  short *psVar1;
  short sVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  char cVar7;
  undefined1 uVar8;
  int iVar9;
  int iVar10;

  iVar3 = *(int *)(DAT_0087a474 + 0x34);
  iVar10 = (param_2 & 0xffff) * 0x14;
  uVar4 = *(uint *)(iVar3 + 8 + (*(uint *)(iVar3 + 0x10 + iVar10) & 0xffff) * 0x14);
  if ((uVar4 != 0xffffffff) &&
     (uVar5 = *(uint *)(iVar3 + 8 + (uVar4 & 0xffff) * 0x14), uVar5 != 0xffffffff)) {
    iVar9 = (uVar5 & 0xffff) * 0x14;
    uVar6 = *(uint *)(iVar9 + 8 + iVar3);
    if ((uVar6 == 0xffffffff) || (*(int *)(iVar3 + 8 + (uVar6 & 0xffff) * 0x14) == -1)) {
      cVar7 = hs_parse(uVar4,5);
      if (cVar7 == '\0') {
        return 0;
      }
      psVar1 = (short *)(iVar3 + 4 + iVar10);
      cVar7 = hs_parse(uVar5,*psVar1);
      if (cVar7 != '\0') {
        if (*psVar1 == 0) {
          *psVar1 = *(short *)(*(int *)(DAT_0087a474 + 0x34) + 4 + iVar9);
        }
        if ((uVar6 != 0xffffffff) && (cVar7 = hs_parse(uVar6,*psVar1), cVar7 == '\0')) {
          return 0;
        }
        return 1;
      }
      if (DAT_006b14d4 != (char *)0x0) {
        return 0;
      }
      if (*psVar1 != 0) {
        return 0;
      }
      if (uVar6 == 0xffffffff) {
        return 0;
      }
      cVar7 = hs_parse(uVar6,0);
      if (cVar7 == '\0') {
        return 0;
      }
      sVar2 = *(short *)(*(int *)(DAT_0087a474 + 0x34) + 4 + (uVar6 & 0xffff) * 0x14);
      *psVar1 = sVar2;
      uVar8 = hs_parse(uVar5,(int)sVar2);
      return uVar8;
    }
  }
  DAT_006b14d4 = "i expected (if <condition> <then> [<else>]).";
  DAT_006b14d8 = *(undefined4 *)(*(int *)(DAT_0087a474 + 0x34) + 0xc + iVar10);
  return 0;
}
#endif
