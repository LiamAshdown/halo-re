// lens_flare_unpack_up  (Ghidra: FUN_005134c0, unnamed; named speculatively -- see UNSURE)
// address 0x5134c0, size 33 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: byte-identical wrapper to lens_flare_unpack_direction @0x513490 (see that file for
//   the full argument analysis); named as the "up" counterpart from
//   lens_flare_instance.packed_up (types/rasterizer.h).
// register convention: unpack scratch destination in EAX, packed value in ECX (both forwarded
//   unchanged to the callee), final destination in unaff_ESI (unresolved register read).
//   // blam-cc: EAX -> unpack_scratch, ECX -> packed, unaff_ESI -> destination
// UNSURE: same as lens_flare_unpack_direction.c -- which packed field this corresponds to is a
//   guess from struct field order, not a traced caller.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern real_vector3d *vector3d_unpack_normal_11_11_10(real_vector3d *out, uint32_t packed); // 0x513400

// blam-cc: EAX -> unpack_scratch, ECX -> packed, unaff_ESI -> destination
// Unpacks a compressed 11:11:10 direction into unpack_scratch, then copies the result into
// destination.
void lens_flare_unpack_up(real_vector3d *unpack_scratch, uint32_t packed, real_vector3d *destination)
{
    real_vector3d *unpacked;

    unpacked = vector3d_unpack_normal_11_11_10(unpack_scratch, packed);
    destination->i = unpacked->i;
    destination->j = unpacked->j;
    destination->k = unpacked->k;
}

#if 0
Original Ghidra decompilation (0x5134c0):

void FUN_005134c0(void)

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
