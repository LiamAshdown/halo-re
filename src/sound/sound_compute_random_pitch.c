// sound_compute_random_pitch  (Ghidra: FUN_0054aec0, still unnamed there)
// address 0x54aec0, size 73 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: sole caller is 0x549af0 (sound_play_new, other half of this module), which stores
// the result straight into sound.pitch (0xb0-struct offset 0x88, see types/sound.h "pitch 0x88
// random pitch from 0x54aec0 / random_range_real"); its five arguments there are
// Sound.random_pitch_bounds[0]/[1] (tag offsets 0x14/0x18), Sound.zero_pitch_modifier (0x44),
// Sound.one_pitch_modifier (0x5c) and a sound_location's scale field (location + 2 shorts ==
// location->scale, 0x04). out/phase4/sound_functions.md's summary ("randomized playback gain")
// is superseded by this cross-reference; the field is pitch, not gain.
// register convention: five plain stack floats (Ghidra recognized all five parameters directly;
// no register-passed arguments here).
//
// The inlined "seed = seed*0x19660d + 0x3c6ef35f; (seed>>16)*1.5259022e-05" step is the same
// LCG body as random_real_range_seeded (src/math/random_real_range_seeded.c, 0x4cd170) run over
// the same global (types/math.h "0x00719cd4 local_random_seed"); this rewrite calls that
// established function instead of re-inlining its body, which reads the identical state and
// produces the identical sequence of values.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "sound.h"

extern random_seed local_random_seed; // 0x00719cd4

extern real random_real_range_seeded(random_seed *seed, real min, real max); // 0x4cd170, math module

// Randomized pitch multiplier: a random value within the tag's random_pitch_bounds, scaled by
// the zero_pitch_modifier..one_pitch_modifier blend at the sound's current distance fraction.
float sound_compute_random_pitch(float pitch_bounds_min, float pitch_bounds_max,
    float zero_pitch_modifier, float one_pitch_modifier, float distance_scale)
{
    float distance_modifier;
    float random_pitch;

    distance_modifier = (one_pitch_modifier - zero_pitch_modifier) * distance_scale + zero_pitch_modifier;
    random_pitch = random_real_range_seeded(&local_random_seed, pitch_bounds_min, pitch_bounds_max);
    return distance_modifier * random_pitch;
}

#if 0
Original Ghidra decompilation (0x54aec0):

float10 FUN_0054aec0(float param_1,float param_2,float param_3,float param_4,float param_5)

{
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  return (((float10)param_4 - (float10)param_3) * (float10)param_5 + (float10)param_3) *
         (((float10)param_2 - (float10)param_1) *
          (float10)(DAT_00719cd4 >> 0x10) * (float10)1.5259022e-05 + (float10)param_1);
}

Disassembly (0x54aec0..0x54af09, capstone; phase-4 review):

0x54aec0: push ecx
0x54aec1: mov eax, dword ptr [0x719cd4]
0x54aec6: imul eax, eax, 0x19660d
0x54aecc: add eax, 0x3c6ef35f
0x54aed1: mov ecx, eax
0x54aed3: shr ecx, 0x10
0x54aed6: mov dword ptr [esp], ecx
0x54aed9: mov dword ptr [0x719cd4], eax
0x54aede: fild dword ptr [esp]
0x54aee1: fmul dword ptr [0x672b84]
0x54aee7: fld dword ptr [esp + 0xc]
0x54aeeb: fsub dword ptr [esp + 8]
0x54aeef: fmulp st(1)
0x54aef1: fadd dword ptr [esp + 8]
0x54aef5: fld dword ptr [esp + 0x14]
0x54aef9: fsub dword ptr [esp + 0x10]
0x54aefd: fmul dword ptr [esp + 0x18]
0x54af01: fadd dword ptr [esp + 0x10]
0x54af05: fmulp st(1)
0x54af07: pop ecx
0x54af08: ret 
#endif
