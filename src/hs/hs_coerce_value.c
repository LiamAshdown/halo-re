// hs_coerce_value  (Ghidra: FUN_0048ad10)
// address 0x48ad10, size 75 bytes
// name confidence: 0.4 (out/phase4/hs_functions.md: "Coerces a raw HS global's value to a
//   requested type using the shared per-type-pair conversion dispatch table" -- used more
//   generally than just for globals, see hs_thread_push.c)
// rewrite confidence: 0.85 (verified instruction by instruction against 0x48ad10;
//   the EAX/value binding is confirmed by hs_thread_push's two call sites and by
//   hs_thread_return's inlined copy of this exact code at 0x48a6ac..0x48a6f0)
// evidence: near-identical logic to the inline type-coercion block in hs_thread_return.c (this
//   module, 0x48a640), which this function does NOT call, so the two independently confirm the
//   same hs_type_conversion_procedures / object-name-family shape.
// register convention: value in EAX (fully register-implicit -- Ghidra shows zero parameters of
//   any kind); destination type in CX (in_CX); source type in DX (in_DX).
//   // blam-cc: EAX -> value, ECX -> dest_type, EDX -> source_type
// VERIFIED (was UNSURE): the EAX/value binding is real. At 0x48ad4f the function does `push eax`
// before `call dword [ecx*4 + 0x68bc10]`, so EAX is the single cdecl argument handed to the
// conversion procedure, and the no-conversion path simply `ret`s with EAX untouched. The
// object-name fallback at 0x48ad3f is a `jmp 0x4f73c0`, i.e. a tail call that returns that
// function's own result.

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t object_lookup_table_get(int32_t value); // UNSURE args; module unknown, 0x4f73c0
extern int32_t (*hs_type_conversion_procedures[k_hs_type_count][k_hs_type_count])(int32_t value);
    // 0x0068bc10, indexed [destination_type][source_type]

// Coerces `value` from `source_type` to `dest_type`, unless they already match, source_type is
// "passthrough", or dest_type is in the object-name family (0x2b..0x30, not handled here).
// Ordinary types go through hs_type_conversion_procedures; a source_type in the object-name
// family instead goes through object_lookup_table_get (the same object-name resolution fallback
// hs_thread_return uses). Returns `value` unchanged if none of that applies.
int32_t hs_coerce_value(int32_t value, hs_type_t dest_type, hs_type_t source_type)
{
    if (source_type != dest_type && source_type != _hs_type_passthrough &&
        (dest_type < 0x2b || 0x30 < dest_type)) {
        if (dest_type < 0x25 || 0x2a < dest_type) {
            // Called unconditionally: retail does `call dword [ecx*4 + 0x68bc10]` with no
            // NULL check (0x48ad4f). 2331 of the 2401 slots are NULL, so reaching this with an
            // incompatible pair is a jump through NULL -- the compiler's hs_types_are_compatible
            // pass is what keeps it from happening.
            value = hs_type_conversion_procedures[dest_type][source_type](value);
        } else if (0x2a < source_type && source_type < 0x31) {
            return object_lookup_table_get(value);
        }
    }
    return value;
}

#if 0
Original Ghidra decompilation (0x48ad10):

void FUN_0048ad10(void)

{
  short in_CX;
  short in_DX;

  if (((in_DX != in_CX) && (in_DX != 3)) && ((in_CX < 0x2b || (0x30 < in_CX)))) {
    if ((in_CX < 0x25) || (0x2a < in_CX)) {
      (**(code **)(&DAT_0068bc10 + (in_CX * 0x31 + (int)in_DX) * 4))();
    }
    else if ((0x2a < in_DX) && (in_DX < 0x31)) {
      FUN_004f73c0();
      return;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
