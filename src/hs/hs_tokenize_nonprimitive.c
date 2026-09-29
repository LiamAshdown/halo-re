// hs_tokenize_nonprimitive  (Ghidra: hs_tokenize_nonprimitive, already named)
// address 0x486290, size 190 bytes
// name confidence: 0.9   rewrite confidence: 0.65
// evidence: CEA-PDB string match ("this left parenthesis is unmatched.",
// "this expression is empty."); recursively tokenizes each child element of a parenthesized
// list, threading them through node->data (first child) then each child's own next_node, until
// the closing ')'; NUL-terminates each bare-word child in place right after skip_whitespace
// finds its end (hs_tokenize_primitive leaves bare words un-terminated for exactly this).
// register convention: the node index is unrecognized by Ghidra (in_EAX) and the cursor
// pointer is unrecognized (unaff_ESI); by the blam-cc convention these are the first and fifth
// register slots, EAX and ESI.
// UNSURE: `node` itself is computed once from hs_syntax_data as captured on entry and never
// reloaded (matching the original, which never re-reads DAT_0087a474 for iVar2), while the
// child-link slot IS recomputed from a freshly-read hs_syntax_data after every hs_tokenize
// call -- this asymmetry is preserved exactly as Ghidra shows it.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


extern data_array *hs_syntax_data;      // 0x0087a474
extern char *hs_compiled_source;        // 0x006b14c0
extern char *hs_compile_error;          // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8

// blam-cc: node index in EAX, cursor pointer in ESI
// Tokenizes a parenthesized list expression, recursively tokenizing each child element (via
// hs_tokenize) until the matching ')', threading them through node->data/next_node.
void hs_tokenize_nonprimitive(datum_index node_index, char **cursor)
{
    data_array *nodes;
    hs_syntax_node *node;
    void *first_child_slot;
    void *child_slot;
    char *prev_cursor;
    datum_index child_index;

    nodes = hs_syntax_data;
    node = (hs_syntax_node *)((uint8_t *)nodes->data + (node_index & 0xffff) * nodes->size);
    node->source_offset = (int32_t)(*cursor - hs_compiled_source);
    *cursor = *cursor + 1;
    first_child_slot = &node->data;
    child_slot = first_child_slot;
    for (;;) {
        if (hs_compile_error != 0) {
            goto empty_check;
        }
        prev_cursor = *cursor;
        skip_whitespace(cursor);
        if (*cursor != prev_cursor) {
            *prev_cursor = '\0';
        }
        if (**cursor == '\0') {
            hs_compile_error = "this left parenthesis is unmatched.";
            hs_compile_error_offset = node->source_offset;
            goto empty_check;
        }
        if (**cursor == ')') {
            **cursor = '\0';
            *cursor = *cursor + 1;
            goto empty_check;
        }
        child_index = hs_tokenize(cursor);
        *(datum_index *)child_slot = child_index;
        if (child_index != k_datum_index_none) {
            child_slot = (uint8_t *)hs_syntax_data->data + 8 + (child_index & 0xffff) * hs_syntax_data->size;
        }
        continue;
    empty_check:
        if ((child_slot == first_child_slot) && (hs_compile_error == 0)) {
            hs_compile_error = "this expression is empty.";
            hs_compile_error_offset = node->source_offset;
        }
        return;
    }
}

#if 0
Original Ghidra decompilation (0x486290):

void hs_tokenize_nonprimitive(void)

{
  uint *puVar1;
  int iVar2;
  undefined1 *puVar3;
  uint in_EAX;
  uint uVar4;
  int *unaff_ESI;

  iVar2 = *(int *)(DAT_0087a474 + 0x34) + (in_EAX & 0xffff) * 0x14;
  *(int *)(iVar2 + 0xc) = *unaff_ESI - DAT_006b14c0;
  *unaff_ESI = *unaff_ESI + 1;
  puVar1 = (uint *)(iVar2 + 0x10);
  do {
    if (DAT_006b14d4 != (char *)0x0) {
LAB_00486328:
      if ((puVar1 == (uint *)(iVar2 + 0x10)) && (DAT_006b14d4 == (char *)0x0)) {
        DAT_006b14d4 = "this expression is empty.";
        DAT_006b14d8 = *(undefined4 *)(iVar2 + 0xc);
      }
      return;
    }
    puVar3 = (undefined1 *)*unaff_ESI;
    skip_whitespace();
    if ((undefined1 *)*unaff_ESI != puVar3) {
      *puVar3 = 0;
    }
    if (*(char *)*unaff_ESI == '\0') {
      DAT_006b14d4 = "this left parenthesis is unmatched.";
      DAT_006b14d8 = *(undefined4 *)(iVar2 + 0xc);
      goto LAB_00486328;
    }
    if (*(char *)*unaff_ESI == ')') {
      *(undefined1 *)*unaff_ESI = 0;
      *unaff_ESI = *unaff_ESI + 1;
      goto LAB_00486328;
    }
    uVar4 = hs_tokenize();
    *puVar1 = uVar4;
    if (uVar4 != 0xffffffff) {
      puVar1 = (uint *)(*(int *)(DAT_0087a474 + 0x34) + 8 + (uVar4 & 0xffff) * 0x14);
    }
  } while( true );
}
#endif
