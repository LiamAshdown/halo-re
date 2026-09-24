// lens_flare_unpack_direction  (Ghidra: FUN_00513490, unnamed; named speculatively -- see UNSURE)
// address 0x513490, size 34 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: thin wrapper that calls vector3d_unpack_normal_11_11_10 with no visible arguments
//   (its EAX/ECX are forwarded straight through from this function's own, unshown, register
//   inputs) and copies the unpacked 3 floats from the callee's return pointer into a second,
//   separate destination carried in unaff_ESI. lens_flare_instance (types/rasterizer.h) has
//   exactly two 11:11:10 packed fields, packed_direction and packed_up; this is one of two
//   byte-identical wrappers (see also lens_flare_unpack_up @0x5134c0) with no other
//   distinguishing evidence, so the names are a guess from that struct's field order.
// register convention: unpack scratch destination in EAX, packed value in ECX (both forwarded
//   unchanged to the callee), final destination in unaff_ESI (unresolved register read).
//   // blam-cc: EAX -> unpack_scratch, ECX -> packed, unaff_ESI -> destination
// UNSURE: which of packed_direction/packed_up this corresponds to, and why the callee is
//   invoked through an intermediate scratch buffer instead of writing unaff_ESI directly -- no
//   caller in this session's range was identified to confirm either.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern real_vector3d *vector3d_unpack_normal_11_11_10(real_vector3d *out, uint32_t packed); // 0x513400

// blam-cc: EAX -> unpack_scratch, ECX -> packed, unaff_ESI -> destination
// Unpacks a compressed 11:11:10 direction into unpack_scratch, then copies the result into
// destination.
void lens_flare_unpack_direction(real_vector3d *unpack_scratch, uint32_t packed, real_vector3d *destination)
{
    real_vector3d *unpacked;

    unpacked = vector3d_unpack_normal_11_11_10(unpack_scratch, packed);
    destination->i = unpacked->i;
    destination->j = unpacked->j;
    destination->k = unpacked->k;
}

#if 0
Original Ghidra decompilation (0x513490):

void FUN_00513490(void)

{
  undefined4 *puVar1;
  undefined4 *unaff_ESI;

  puVar1 = (undefined4 *)vector3d_unpack_normal_11_11_10();
  *unaff_ESI = *puVar1;
  unaff_ESI[1] = puVar1[1];
  unaff_ESI[2] = puVar1[2];
  return;
}
#endif
