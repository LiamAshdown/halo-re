// compute_sha1_hash_first_qword  (Ghidra: FUN_0057f330; renamed per
//   out/phase4/shell_types_notes.md: "0x57f330: data in EDX, length in ECX, 8-byte output in
//   ESI.")
// address 0x57f330, size 44 bytes
// name confidence: 0.55  rewrite confidence: 0.8
// evidence: matches out/phase4/shell_functions.md summary: "Computes a hash via FUN_0057f2a0
//   and copies an 8-byte fragment of the result out through a register-passed output pointer."
//   Used by shell_build_product_id_string 0x57f3f0 to reduce the 20-byte SHA-1 digest of the
//   hashed product id key down to the 8-byte value it prints as a %19.19I64d field.
// register convention: out/phase4/shell_types_notes.md pins data in EDX, length in ECX, the
//   8-byte output pointer in ESI.
// blam-cc: data in EDX, length in ECX, output in ESI.
// UNSURE: Ghidra's own decompile of this function has no return statement (it falls off the
//   end), but its caller (shell_build_product_id_string 0x57f3f0) reads EAX as a bool result
//   afterward ("iVar4 = FUN_0057f330(); if (iVar4 != 0) ..."); that is simply compute_sha1_hash's
//   own success flag left in EAX by the tail call, so it is modeled here as an explicit return.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern uint8_t compute_sha1_hash(const uint8_t *data, uint32_t length, uint8_t *digest_out); // 0x57f2a0

// Computes the SHA-1 digest of `length` bytes at `data` and, if that succeeds, copies its first
// 8 bytes to `*output` (when `output` is non-NULL). The remaining 12 digest bytes are discarded.
// Returns compute_sha1_hash's own success flag (see UNSURE above).
uint8_t compute_sha1_hash_first_qword(const uint8_t *data, uint32_t length, uint32_t *output)
{
    uint32_t digest[5]; // 20-byte SHA-1 digest; only the first 8 bytes are ever used
    uint8_t ok;

    ok = compute_sha1_hash(data, length, (uint8_t *)digest);
    if (ok != 0 && output != 0) {
        output[0] = digest[0];
        output[1] = digest[1];
    }
    return ok;
}

#if 0
Original Ghidra decompilation (0x57f330):

void FUN_0057f330(void)

{
  uint uVar1;
  DWORD in_ECX;
  BYTE *in_EDX;
  undefined4 *unaff_ESI;
  undefined4 local_14;
  undefined4 local_10;

  uVar1 = compute_sha1_hash(in_EDX,in_ECX,(BYTE *)&local_14);
  if (((uVar1 & 0xff) != 0) && (unaff_ESI != (undefined4 *)0x0)) {
    *unaff_ESI = local_14;
    unaff_ESI[1] = local_10;
  }
  return;
}
#endif
