// animation_quaternion48_decode  (Ghidra: model_vertex_unpack_compressed_normal, wrong name;
// renamed per out/phase4/models_types_notes.md "Misnamed or misattributed functions" table --
// this decodes compressed animation rotation data, not a vertex normal)
// address 0x4d6380, size 182 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/models_types_notes.md animation_quaternion48 section, which spells out
//   this exact bit layout (verified against the Ghidra output, which matches term for term).
//   Each 12-bit field is packed into the high bits of a 16-bit half before the *3.051851e-05
//   scale (the same 1/32767 constant as animation_quaternion16_decode), so the low 4 bits of
//   each decoded component are always zero -- a real property of the codec, not a rewrite
//   artifact. This function does not call quaternion_normalize; the types notes' mention of
//   normalize describes the caller (animation_node_get_rotation, 0x4d6b60), not this decoder.
// register convention: packed source in ECX (in_ECX), output real_quaternion in ESI
//   (unaff_ESI).
//   // blam-cc: ECX -> source, ESI -> out

#include "tags.h"
#include "math.h"
#include "models.h"

// Unpacks the three 16-bit words of a 48-bit compressed animation rotation into four 12-bit
// signed fields (i, j, k, w), each left-justified into a 16-bit half and scaled by 1/32767.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void animation_quaternion48_decode(animation_quaternion48 *source, real_quaternion *out)
{
    uint16_t w0, w1, w2;

    w0 = source->packed[0];
    w1 = source->packed[1];
    w2 = source->packed[2];

    out->i = (real)(int16_t)((w0 >> 0xc) | (w0 & 0xfff0)) * 3.051851e-05f;
    out->j = (real)(int16_t)(((w1 >> 4) & 0xff0) | (w0 & 0xf) | (w0 << 0xc)) * 3.051851e-05f;
    out->k = (real)(int16_t)((((w2 >> 4) & 0xf00) | (w1 & 0xf0)) >> 4 | (w1 << 8)) * 3.051851e-05f;
    out->w = (real)(int16_t)((w2 >> 8 & 0xf) | (w2 << 4)) * 3.051851e-05f;
}

#if 0
Original Ghidra decompilation (0x4d6380):

void model_vertex_unpack_compressed_normal(void)

{
  ushort uVar1;
  ushort uVar2;
  ushort uVar3;
  ushort *in_ECX;
  float *unaff_ESI;

  uVar1 = in_ECX[1];
  uVar2 = *in_ECX;
  uVar3 = in_ECX[2];
  *unaff_ESI = (float)(int)(short)(uVar2 >> 0xc | uVar2 & 0xfff0) * 3.051851e-05;
  unaff_ESI[1] = (float)(int)(short)(uVar1 >> 4 & 0xff0 | uVar2 & 0xf | uVar2 << 0xc) * 3.051851e-05
  ;
  unaff_ESI[2] = (float)(int)(short)((uVar3 >> 4 & 0xf00 | uVar1 & 0xf0) >> 4 | uVar1 << 8) *
                 3.051851e-05;
  unaff_ESI[3] = (float)(int)(short)(uVar3 >> 8 & 0xf | uVar3 << 4) * 3.051851e-05;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
