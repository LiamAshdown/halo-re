// actor_pick_dialogue_variant_b  (Ghidra: actor_pick_dialogue_variant_b, renamed per out/phase4 hint)
// address 0x424b80, size 160 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: phase-4 summary "Variant of actor_pick_dialogue_variant_a with a slightly
// different RNG-advance condition, used for a different dialogue category." No static
// callers recorded (out/functions.json: callers=0), same as its sibling @0x424aa0. Same
// constant roles as that function (k_random_scale_65536 the 1/65536 scale, ticks_per_second the 30.0
// ticks conversion). Confirmed the exact per-branch scale/offset pairing with objdump
// (bin/halo.exe 0x424b80..0x424c1f): category 1 and categories 2-3 use two different
// scale/offset pairs, and category 1's offset happens to reuse the same constant the
// default (category outside 1..3) path uses on its own.
// register convention: AX -> category; no stack arguments.
//   // blam-cc: EAX (low 16 bits) -> category

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint32_t random_seed_global; // 0x00719cd0

extern int32_t __ftol(double x); // FISTP-based float-to-int truncation

extern float k_real_one; // 0x00672ac4, 1.0
extern float k_random_scale_65536; // 0x00672b84, 1.5259022e-05 = 1/65536
extern float actor_dialogue_variant_scale_1b;  // 0x00672cac
extern float actor_dialogue_variant_scale_23b; // 0x00672dec
extern float k_real_point_six; // 0x00672ca8, 0.6
extern float ticks_per_second; // 0x00672ac8, 30.0

// blam-cc: AX -> category
// Rolls the shared PRNG for category 1 (scaled by actor_dialogue_variant_scale_1b, offset by
// the same constant the default case uses directly) or for categories 2-3 (a different
// scale/offset pair), otherwise falls back to the fixed default value, then converts to a
// tick count clamped to [0, 255].
int32_t actor_pick_dialogue_variant_b(int16_t category)
{
    float value = k_real_one;
    int32_t ticks;

    if (category == 1) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        value = (float)(int32_t)(random_seed_global >> 0x10) * k_random_scale_65536 * actor_dialogue_variant_scale_1b
              + k_real_one;
    } else if (category > 1 && category <= 3) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        value = (float)(int32_t)(random_seed_global >> 0x10) * k_random_scale_65536 * actor_dialogue_variant_scale_23b
              + k_real_point_six;
    }

    ticks = __ftol((double)(value * ticks_per_second));
    if (ticks > 0xff) {
        return 0xff;
    }
    return ticks;
}

#if 0
Original Ghidra decompilation (0x424b80):

int FUN_00424b80(void)

{
  short in_AX;
  short sVar1;

  if (in_AX == 1) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  }
  else if ((1 < in_AX) && (in_AX < 4)) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  }
  sVar1 = __ftol();
  if (0xff < sVar1) {
    return 0xff;
  }
  return (int)sVar1;
}

Ground truth from objdump (bin/halo.exe @ 0x424b80..0x424c1f), since the pseudocode above
elides which scale/offset pair each branch actually uses:

  424b8a: cmp eax,1 / je 0x424bcc          ; category == 1
  424b8f: jle 0x424c03                     ; category <= 0 -> default
  424b91: cmp eax,3 / jg 0x424c03          ; category > 3 -> default
  424b96: ... fmul ds:0x672dec / fadd ds:0x672ca8   ; categories 2-3
  424bcc: ... fmul ds:0x672cac / fadd ds:0x672ac4   ; category 1
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
