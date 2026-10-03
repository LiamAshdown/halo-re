// hs_null_with_params_evaluate  (Ghidra: hs_null_with_params_evaluate, already named)
// address 0x483430, size 5 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: out/phase4/hs_types_notes.md: "five bytes: mov ax,[eax+4]; ret. It has no callers
// and +0x04 is equally hs_global_definition::type and hs_syntax_node::type, so which struct it
// belongs to is undecidable from this module. No type claimed." hs_functions.md documents it
// as the trivial evaluate handler for a 'null with params' syntax-node type that returns the
// node's type field, so hs_syntax_node is used here, but this is the weakest-evidence file in
// the batch.
// register convention: the pointer this reads is unrecognized by Ghidra (in_EAX); by the
// blam-cc convention this is the first register slot, EAX.
// UNSURE: whole function -- no callers exist anywhere in the binary to confirm either the
// pointed-to type or the true parameter/return convention of an "evaluate" handler that
// returns a value instead of being void.

// NOTE: the two instructions (`mov ax,[eax+4]` / `ret`) are transcribed exactly -- the rewrite
// itself is not in doubt. All of the uncertainty is in the pointee type and the calling
// convention, which is what the low name confidence and the UNSURE below record.
#include "tags.h"
#include "memory.h"
#include "hs.h"

// blam-cc: node pointer in EAX
// Returns the type field of the syntax node at `node` (offset 4, shared by hs_syntax_node and
// hs_global_definition). See UNSURE above: this function is never called anywhere in the image.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
hs_type_t hs_null_with_params_evaluate(hs_syntax_node *node)
{
    return node->type;
}

#if 0
Original Ghidra decompilation (0x483430):

undefined2 hs_null_with_params_evaluate(void)

{
  int in_EAX;

  return *(undefined2 *)(in_EAX + 4);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
