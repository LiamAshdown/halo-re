// hs_parse_cond_recursive  (Ghidra: hs_parse_cond_recursive, already named)
// address 0x4848f0, size 581 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: CEA-PDB string match; recurses on the cond form's list of (condition result) pairs,
// allocating new syntax nodes and splicing/repurposing the existing ones to build a chain per
// hs_functions.md's summary ("desugars a cond special-form's list of condition/result pairs
// into a chain of nested if syntax nodes"). The base case (pair_index == none) manufactures a
// default primitive node of the cond form's own expected type with value 0.
// register convention: __cdecl, both parameters recognized directly by Ghidra.
// VERIFIED (was UNSURE): every field write in the splice below was checked instruction by
// instruction against the retail bytes at 0x4849de..0x484aa8 -- register/stack-slot map:
//   ebx = new_node, ecx = new_if_node, ebp = replacement_node, esi = nodes->data,
//   edi = pair_index*0x14, [esp+0x0c] = condition_index*0x14, [esp+0x14] =
//   &condition_node->next_node, [esp+0x10] = new_if_index, [esp+0x08] = replacement_index.
// The surprising writes are real: `pair_node->next_node = condition_index` (0x484a34) does
// repoint the pair node's sibling link at what was its own first child, because the pair node is
// being repurposed in place into the desugared `if`'s function-name node (index_union/type = 2 ==
// _hs_type_function_name, flags = primitive, source_offset = -1), and the original condition node
// becomes its first sibling. `condition_node->next_node = new_if_index` (0x484a9e) then hangs the
// nested if -- built out of new_if_node plus replacement_node -- off the condition, and
// replacement_node inherits the condition's old next_node (the pair's result expression).
// RESOLVED (was UNSURE): the guard before that branch is NOT the intended
// `next_node != k_datum_index_none`. The retail bytes at 0x4849a0 are
//     xor ecx,ecx / cmp dword [eax],ecx / sete cl / cmp ecx,-1 / je <needs-a-result>
// -- cl is 0 or 1 and is compared against -1, so the branch is never taken and the
// "this argument to cond needs a result." error is DEAD CODE in halo.exe 1.0.10. A cond pair with
// only a condition and no result therefore falls straight into the splice instead of erroring.
// Written literally below; do not "correct" it.
// UNSURE: some pointers into the syntax-node table are reused across the two datum_new calls
// without the fresh-DAT_0087a474 reload Ghidra shows for a few of the later reads/writes; both
// are preserved as separate reloads only where the original does the same, on the assumption
// (borne out by every other function in this module) that reallocation never actually moves
// hs_syntax_data's backing storage mid-compile, making the distinction inert in practice.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern datum_index datum_new(data_array *array); // memory module, 0x004d0480

extern data_array *hs_syntax_data; // 0x0087a474
extern char *hs_compile_error;     // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8

// Recursively desugars a cond special-form's list of condition/result pairs into a chain of
// nested if syntax nodes; see UNSURE notes above for the exact node-splicing story.
datum_index hs_parse_cond_recursive(datum_index cond_node_index, datum_index pair_index)
{
    data_array *nodes;
    datum_index new_index;
    hs_syntax_node *new_node;
    hs_syntax_node *cond_node;
    hs_syntax_node *pair_node;
    datum_index condition_index;
    hs_syntax_node *condition_node;
    datum_index new_if_index;
    datum_index replacement_index;
    hs_syntax_node *new_if_node;
    hs_syntax_node *replacement_node;
    datum_index inner_result;
    datum_index result_index;

    nodes = hs_syntax_data;
    new_index = datum_new(nodes);
    cond_node = (hs_syntax_node *)((uint8_t *)nodes->data + (cond_node_index & 0xffff) * nodes->size);
    if (new_index == k_datum_index_none) {
        hs_compile_error = (char *)"i couldn't allocate a syntax node.";
        hs_compile_error_offset = cond_node->source_offset;
        return k_datum_index_none;
    }
    new_node = (hs_syntax_node *)((uint8_t *)nodes->data + (new_index & 0xffff) * nodes->size);
    new_node->source_offset = cond_node->source_offset;
    new_node->flags = 0;
    new_node->next_node = k_datum_index_none;

    if (pair_index == k_datum_index_none) {
        /* base case: a default primitive value of the cond form's own expected type */
        new_node->flags = _hs_syntax_node_primitive_bit;
        new_node->index_union = cond_node->type;
        new_node->type = cond_node->type;
        new_node->data.long_value = 0;
        return new_index;
    }

    pair_node = (hs_syntax_node *)((uint8_t *)nodes->data + (pair_index & 0xffff) * nodes->size);
    if ((pair_node->flags & _hs_syntax_node_primitive_bit) != 0) {
        hs_compile_error = (char *)"this argument to cond should be a condition/result pair";
        hs_compile_error_offset = pair_node->source_offset;
        return k_datum_index_none;
    }
    condition_index = pair_node->data.first_child;
    condition_node = (hs_syntax_node *)((uint8_t *)nodes->data + (condition_index & 0xffff) * nodes->size);

    /* 0x4849a0: sete cl over (next_node == 0), then cmp ecx,-1 -- always false.
       See the RESOLVED note at the top of the file: the else branch is unreachable. */
    if ((uint32_t)(condition_node->next_node == 0) != (uint32_t)k_datum_index_none) {
        new_if_index = datum_new(nodes);
        replacement_index = datum_new(nodes);
        if ((new_if_index == k_datum_index_none) || (replacement_index == k_datum_index_none)) {
            nodes = hs_syntax_data;
            cond_node = (hs_syntax_node *)((uint8_t *)nodes->data + (cond_node_index & 0xffff) * nodes->size);
            hs_compile_error = (char *)"i couldn't allocate a syntax node.";
            hs_compile_error_offset = cond_node->source_offset;
            return k_datum_index_none;
        }
        nodes = hs_syntax_data;
        new_if_node = (hs_syntax_node *)((uint8_t *)nodes->data + (new_if_index & 0xffff) * nodes->size);
        replacement_node = (hs_syntax_node *)((uint8_t *)nodes->data + (replacement_index & 0xffff) * nodes->size);

        inner_result = hs_parse_cond_recursive(cond_node_index, pair_node->next_node);
        new_if_node->next_node = inner_result;
        if (inner_result == k_datum_index_none) {
            return k_datum_index_none;
        }

        new_node->data.first_child = pair_index;
        new_node->index_union = _hs_type_function_name;

        pair_node->next_node = condition_index;
        pair_node->index_union = _hs_type_function_name;
        pair_node->type = _hs_type_function_name;
        pair_node->data.long_value = 0;
        pair_node->flags = _hs_syntax_node_primitive_bit;
        pair_node->source_offset = -1;

        new_if_node->data.first_child = replacement_index;
        new_if_node->flags = 0;
        new_if_node->source_offset = new_node->source_offset;

        nodes = hs_syntax_data;
        replacement_node->data.long_value = 0;
        replacement_node->index_union = 0;
        replacement_node->flags = _hs_syntax_node_primitive_bit;
        result_index = condition_node->next_node;
        replacement_node->type = _hs_type_function_name;
        replacement_node->source_offset = -1;
        replacement_node->next_node = result_index;
        condition_node->next_node = new_if_index;
        return new_index;
    }
    /* unreachable in retail -- see above */
    hs_compile_error = (char *)"this argument to cond needs a result.";
    hs_compile_error_offset = condition_node->source_offset;
    return k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x4848f0):

uint hs_parse_cond_recursive(uint param_1,uint param_2)

{
  int iVar1;
  uint *puVar2;
  undefined2 uVar3;
  int iVar4;
  uint uVar5;
  undefined4 uVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  undefined8 uVar15;

  uVar15 = datum_new();
  iVar11 = (int)((ulonglong)uVar15 >> 0x20);
  uVar7 = (uint)uVar15;
  if (uVar7 == 0xffffffff) {
    DAT_006b14d4 = "i couldn\'t allocate a syntax node.";
    DAT_006b14d8 = *(undefined4 *)(*(int *)(iVar11 + 0x34) + 0xc + (param_1 & 0xffff) * 0x14);
    return 0xffffffff;
  }
  iVar1 = *(int *)(iVar11 + 0x34) + (uVar7 & 0xffff) * 0x14;
  iVar13 = (param_1 & 0xffff) * 0x14;
  *(undefined4 *)(iVar1 + 0xc) = *(undefined4 *)(iVar13 + 0xc + *(int *)(iVar11 + 0x34));
  *(undefined2 *)(iVar1 + 6) = 0;
  *(undefined4 *)(iVar1 + 8) = 0xffffffff;
  if (param_2 == 0xffffffff) {
    *(undefined2 *)(iVar1 + 6) = 1;
    uVar3 = *(undefined2 *)(*(int *)(iVar11 + 0x34) + 4 + iVar13);
    *(undefined2 *)(iVar1 + 2) = uVar3;
    *(undefined2 *)(iVar1 + 4) = uVar3;
    *(undefined4 *)(iVar1 + 0x10) = 0;
    return uVar7;
  }
  iVar4 = *(int *)(iVar11 + 0x34);
  iVar14 = (param_2 & 0xffff) * 0x14;
  if ((*(byte *)(iVar4 + 6 + iVar14) & 1) != 0) {
    DAT_006b14d4 = "this argument to cond should be a condition/result pair";
    DAT_006b14d8 = *(undefined4 *)(*(int *)(iVar11 + 0x34) + 0xc + iVar14);
    return 0xffffffff;
  }
  uVar5 = *(uint *)(iVar14 + 0x10 + iVar4);
  iVar12 = (uVar5 & 0xffff) * 0x14;
  puVar2 = (uint *)(iVar4 + 8 + iVar12);
  if ((*puVar2 == 0) != 0xffffffff) {
    uVar8 = datum_new();
    uVar15 = datum_new();
    iVar11 = (int)((ulonglong)uVar15 >> 0x20);
    uVar9 = (uint)uVar15;
    if ((uVar8 == 0xffffffff) || (uVar9 == 0xffffffff)) {
      DAT_006b14d4 = "i couldn\'t allocate a syntax node.";
      DAT_006b14d8 = *(undefined4 *)(*(int *)(iVar11 + 0x34) + 0xc + iVar13);
    }
    else {
      iVar4 = *(int *)(iVar11 + 0x34);
      iVar11 = iVar4 + (uVar8 & 0xffff) * 0x14;
      iVar13 = iVar4 + (uVar9 & 0xffff) * 0x14;
      iVar10 = hs_parse_cond_recursive(param_1,*(undefined4 *)(iVar14 + 8 + iVar4));
      *(int *)(iVar11 + 8) = iVar10;
      if (iVar10 != -1) {
        *(uint *)(iVar1 + 0x10) = param_2;
        *(undefined2 *)(iVar1 + 2) = 2;
        *(uint *)(iVar14 + 8 + iVar4) = uVar5;
        *(undefined2 *)(iVar14 + 2 + iVar4) = 2;
        *(undefined2 *)(iVar14 + 4 + iVar4) = 2;
        *(undefined4 *)(iVar14 + 0x10 + iVar4) = 0;
        *(undefined2 *)(iVar14 + 6 + iVar4) = 1;
        *(undefined4 *)(iVar14 + 0xc + iVar4) = 0xffffffff;
        *(uint *)(iVar11 + 0x10) = uVar9;
        *(undefined2 *)(iVar11 + 6) = 0;
        *(undefined4 *)(iVar11 + 0xc) = *(undefined4 *)(iVar1 + 0xc);
        iVar11 = DAT_0087a474;
        *(undefined4 *)(iVar13 + 0x10) = 0;
        *(undefined2 *)(iVar13 + 2) = 0;
        *(undefined2 *)(iVar13 + 6) = 1;
        uVar6 = *(undefined4 *)(*(int *)(iVar11 + 0x34) + 8 + iVar12);
        *(undefined2 *)(iVar13 + 4) = 2;
        *(undefined4 *)(iVar13 + 0xc) = 0xffffffff;
        *(undefined4 *)(iVar13 + 8) = uVar6;
        *puVar2 = uVar8;
        return uVar7;
      }
    }
    return 0xffffffff;
  }
  DAT_006b14d4 = "this argument to cond needs a result.";
  DAT_006b14d8 = *(undefined4 *)(*(int *)(iVar11 + 0x34) + 0xc + iVar12);
  return 0xffffffff;
}
#endif
