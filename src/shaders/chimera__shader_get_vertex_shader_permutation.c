// chimera__shader_get_vertex_shader_permutation  (Ghidra: chimera__shader_get_vertex_shader_permutation,
// already named; CEA/Chimera-derived, hint only)
// address 0x53fd60, size 91 bytes
// name confidence: 0.7   rewrite confidence: 0.85 (phase-4 review, objdump checked)
// evidence: out/phase2/results/shaders_*.json and out/phase4/shaders_types_notes.md. Three
//   callers (0x51eeda, 0x531eeb, 0x532a5f) each pass a tag data pointer in ECX, tested here
//   against -1 (not NULL) before any field is read. shader_type - 1 switches on
//   Shader.shader_type (+0x24): case 0 is shader_type 1 (effect, shader_effect), case 3 is
//   shader_type 4 (model, ShaderModel), cases 4/5/6 are shader_type 5/6/7 (generic, chicago,
//   chicago_extended), which all share the same +0x29 flags byte / +0x2a first_map_type layout
//   (ShaderTransparentGeneric here stands in for all three -- the read is byte-for-byte
//   identical on ShaderTransparentChicago and ShaderTransparentChicagoExtended). Every other
//   shader_type, or a shader pointer of -1, returns 0.
// register convention: shader in ECX (compared to -1, not NULL); result in AX.
// blam-cc: ECX -> shader
// Only EAX and the flags are written (objdump 0x53fd60..0x53fdba). LTCG relies on that: the
//   callers 0x531eeb and 0x532a5f read EDX again right after the call (mov ecx,[edx+0x58])
//   without reloading it, so a hook must preserve EDX (and ECX).
// UNSURE: why the caller's "no shader" sentinel is -1 rather than NULL; out/phase4/shaders_types_notes.md
//   notes this is unexplained from this module alone.

#include "tags.h"
#include "shaders.h"

// Picks the vertex shader permutation index the rasterizer selects for one shader tag
// instance, per shader_vertex_permutation (types/shaders.h).
int16_t chimera__shader_get_vertex_shader_permutation(Shader *shader)
{
    if (shader == (Shader *)-1) {
        return _shader_vertex_permutation_default;
    }

    switch (shader->shader_type - 1) {
    case 0: { // shader_type 1: effect
        shader_effect *effect = (shader_effect *)shader;
        if (*(int32_t *)&effect->secondary_map.tag_id != -1) {
            return (int16_t)(effect->anchor + 1);
        }
        break;
    }
    case 3: { // shader_type 4: model
        ShaderModel *model = (ShaderModel *)shader;
        if (0.0f < model->translucency) {
            return _shader_vertex_permutation_model_translucent;
        }
        break;
    }
    case 4: // shader_type 5: generic
    case 5: // shader_type 6: chicago
    case 6: { // shader_type 7: chicago_extended
        ShaderTransparentGeneric *transparent = (ShaderTransparentGeneric *)shader;
        int16_t permutation = (int16_t)(transparent->first_map_type + 1);

        if (permutation == 1 &&
            (transparent->shader_transparent_generic_flags &
             _shader_transparent_first_map_is_in_screenspace_bit) == 0) {
            permutation = _shader_vertex_permutation_default;
        }
        if (shader->shader_flags & _shader_transparent_lit_bit) {
            return _shader_vertex_permutation_transparent_lit;
        }
        return permutation;
    }
    }
    return _shader_vertex_permutation_default;
}

#if 0
Original Ghidra decompilation (0x53fd60):

undefined4 chimera__shader_get_vertex_shader_permutation(void)

{
  undefined2 uVar2;
  undefined4 uVar1;
  byte *in_ECX;

  if (in_ECX != (byte *)0xffffffff) {
    uVar2 = (undefined2)((uint)(*(short *)(in_ECX + 0x24) + -1) >> 0x10);
    switch(*(short *)(in_ECX + 0x24) + -1) {
    case 0:
      if (*(int *)(in_ECX + 0x58) != -1) {
        return CONCAT22(uVar2,*(short *)(in_ECX + 0x5c) + 1);
      }
      break;
    case 3:
      if (0.0 < *(float *)(in_ECX + 0x38)) {
        return 1;
      }
      break;
    case 4:
    case 5:
    case 6:
      uVar1 = CONCAT22(uVar2,*(short *)(in_ECX + 0x2a) + 1);
      if (((short)(*(short *)(in_ECX + 0x2a) + 1) == 1) && ((in_ECX[0x29] & 8) == 0)) {
        uVar1 = 0;
      }
      if ((*in_ECX & 4) == 0) {
        return uVar1;
      }
      return 5;
    }
  }
  return 0;
}
#endif
