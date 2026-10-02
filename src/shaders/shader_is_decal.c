// shader_is_decal  (Ghidra: FUN_0053fde0; renamed per out/phase4/shaders_types_notes.md: the
// bits returned are the decal flag of every shader type this function handles -- transparent
// generic/chicago/chicago_extended bit 1, glass bit 1, meter bit 0 -- not two-sidedness)
// address 0x53fde0, size 47 bytes
// name confidence: 0.7 (was 0.3 as FUN_0053fde0)   rewrite confidence: 0.85 (phase-4 review, objdump checked)
// evidence: out/phase4/shaders_types_notes.md; shader_transparent_flag_bits /
//   shader_decal_flag_bits (types/shaders.h) match the tested bits exactly for every case.
// register convention: shader in ECX (compared to NULL); result in AL.
// blam-cc: ECX -> shader

#include "tags.h"
#include "shaders.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Returns the decal flag of a shader tag instance: whether it is drawn as a decal over the
// surface underneath it rather than blended into it. NULL and any shader_type this function
// does not recognize return false.
uint8_t shader_is_decal(Shader *shader)
{
    if (shader == (Shader *)0) {
        return 0;
    }

    switch (shader->shader_type) {
    case shadertype_transparent_generic:
    case shadertype_transparent_chicago:
    case shadertype_transparent_chicago_extended: {
        ShaderTransparentGeneric *transparent = (ShaderTransparentGeneric *)shader;
        return (transparent->shader_transparent_generic_flags >> 1) & 1;
    }
    case shadertype_transparent_glass: {
        ShaderTransparentGlass *glass = (ShaderTransparentGlass *)shader;
        return (uint8_t)((glass->shader_transparent_glass_flags >> 1) & 1);
    }
    case shadertype_transparent_meter: {
        ShaderTransparentMeter *meter = (ShaderTransparentMeter *)shader;
        return (uint8_t)(meter->meter_flags & 1);
    }
    default:
        return 0;
    }
}

#if 0
Original Ghidra decompilation (0x53fde0):

byte FUN_0053fde0(void)

{
  byte bVar1;
  int in_ECX;

  bVar1 = 0;
  if (in_ECX != 0) {
    switch(*(undefined2 *)(in_ECX + 0x24)) {
    case 5:
    case 6:
    case 7:
      return *(byte *)(in_ECX + 0x29) >> 1 & 1;
    case 9:
      return *(byte *)(in_ECX + 0x28) >> 1 & 1;
    case 10:
      bVar1 = *(byte *)(in_ECX + 0x28) & 1;
    }
  }
  return bVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
