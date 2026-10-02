// shader_environment_texture_scrolling_evaluate  (Ghidra: FUN_00540060)
// address 0x540060, size 83 bytes
// name confidence: 0.7   rewrite confidence: 0.9 (phase-4 review, objdump checked)
// evidence: out/phase4/shaders_types_notes.md names it and confirms every field: ESI is a
// ShaderEnvironment*, the two channels read u_animation_function/_period/_scale at
// 0x150/0x154/0x158 and v_animation_function/_period/_scale at 0x15c/0x160/0x164 -- exactly
// types/tags.h's ShaderEnvironment fields of the same names. The vendor Xbox decompile
// (vendor/halocea/src/blam/shaders/shader_environment_texture_animation_evaluate.c) is the same
// routine and hints only, since CEA addresses/names drift from this retail build; it is not used
// as a name here because 0x53fe50 in this module already owns the name
// "shader_texture_animation_evaluate" for a different (rotation-capable) routine, so this one is
// named "_scrolling_" to avoid collision, per out/phase4/shaders_types_notes.md.
// register convention: ShaderEnvironment* in ESI (unaff_ESI, not a stack argument -- confirmed
// against objdump: the function's only stack traffic is the two float* outputs and the double
// time; there is no ESI setup inside the function body, so it is inherited from the caller).
// Stack args, caller-cleans (plain ret, no ret N): float *u_out, float *v_out, double time.
// blam-cc: ESI -> environment, stack -> (u_out, v_out, time)
// C parameter order (u_out, v_out, time, environment) follows the existing rasterizer externs
// (src/rasterizer/rasterizer_shader_environment_*_draw.c), stack arguments first, ESI last.
// objdump (0x540060..0x5400b2) confirms three details Ghidra's decompile omits: the "push ecx" /
// "pop ecx" bracketing the body is the usual 4-byte stack reservation, but since 0x5400a8 loads
// v_out into ECX it also leaves ECX preserved for the caller (the sampled callers 0x51ddb4,
// 0x51f6d8, 0x5221d4 do not rely on it); the divide is fdivr qword, i.e. double time / float
// period in x87 precision (Ghidra's (float)param_3 cast is an artifact); and each u_animation_function/v_animation_function WORD is loaded into AX
// immediately before its periodic_function_evaluate call -- Ghidra drops that implicit argument
// because periodic_function_evaluate's AX parameter isn't part of the callee's recognized
// (stack) signature, exactly as documented for periodic_function_evaluate 0x4cc9b0 elsewhere in
// this codebase (see src/math/periodic_function_evaluate.c).

#include "tags.h"
#include "math.h"
#include "shaders.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern real periodic_function_evaluate(periodic_function_t type, double time); // 0x4cc9b0, blam-cc: AX = type

// Evaluates the ShaderEnvironment's animated base-map u/v scroll (diffuse map texture
// coordinate offset) at the given time: each axis runs its own periodic function over
// time / period, scaled by that axis's amplitude.
void shader_environment_texture_scrolling_evaluate(
    float *u_out,
    float *v_out,
    double time,
    ShaderEnvironment *environment) // ESI
{
    *u_out = (float)periodic_function_evaluate(
        environment->u_animation_function,
        time / (double)environment->u_animation_period) * environment->u_animation_scale;

    *v_out = (float)periodic_function_evaluate(
        environment->v_animation_function,
        time / (double)environment->v_animation_period) * environment->v_animation_scale;
}

#if 0
Original Ghidra decompilation (0x540060):

void FUN_00540060(float *param_1,float *param_2,double param_3)

{
  int unaff_ESI;
  float10 fVar1;

  fVar1 = (float10)periodic_function_evaluate
                             ((double)((float)param_3 / *(float *)(unaff_ESI + 0x154)));
  *param_1 = (float)(fVar1 * (float10)*(float *)(unaff_ESI + 0x158));
  fVar1 = (float10)periodic_function_evaluate
                             ((double)((float)param_3 / *(float *)(unaff_ESI + 0x160)));
  *param_2 = (float)(fVar1 * (float10)*(float *)(unaff_ESI + 0x164));
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
