// shader_draw_before_water  (Ghidra: FUN_0053fe30; renamed per out/phase4/shaders_types_notes.md:
// the bit returned is draw_before_water of the transparent generic/chicago/chicago_extended
// flags, not alpha testing)
// address 0x53fe30, size 30 bytes
// name confidence: 0.7 (was 0.3 as FUN_0053fe30)   rewrite confidence: 0.9 (phase-4 review, objdump checked)
// evidence: out/phase4/shaders_types_notes.md; shader_transparent_flag_bits (types/shaders.h)
//   bit 4 is _shader_transparent_draw_before_water_bit, matching the tested bit exactly.
// register convention: shader in ECX (compared to NULL); result in AL.
// blam-cc: ECX -> shader

#include "tags.h"
#include "shaders.h"

// Returns whether a transparent generic/chicago/chicago_extended shader draws before the
// water plane. NULL and every other shader_type return false.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t shader_draw_before_water(Shader *shader)
{
    ShaderTransparentGeneric *transparent;

    if (shader == (Shader *)0) {
        return 0;
    }
    if (shader->shader_type != shadertype_transparent_generic &&
        shader->shader_type != shadertype_transparent_chicago &&
        shader->shader_type != shadertype_transparent_chicago_extended) {
        return 0;
    }

    transparent = (ShaderTransparentGeneric *)shader;
    return (transparent->shader_transparent_generic_flags >> 4) & 1;
}

#if 0
Original Ghidra decompilation (0x53fe30):

byte FUN_0053fe30(void)

{
  short sVar1;
  byte bVar2;
  int in_ECX;

  bVar2 = 0;
  if ((in_ECX != 0) &&
     (((sVar1 = *(short *)(in_ECX + 0x24), sVar1 == 5 || (sVar1 == 6)) || (sVar1 == 7)))) {
    bVar2 = *(byte *)(in_ECX + 0x29) >> 4 & 1;
  }
  return bVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
