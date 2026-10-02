// actor_pick_dialogue_variant_a  (Ghidra: actor_pick_dialogue_variant_a, renamed per out/phase4 hint)
// address 0x424aa0, size 212 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: phase-4 summary "Advances the actor's RNG for certain dialogue categories and
// returns a clamped random dialogue-variant index." No static callers are recorded by Ghidra
// (out/functions.json: callers=0); ai_types_notes.md does not attribute this to any dialogue
// family directly, so it is likely reached only through a jump/dispatch table this module
// does not otherwise model. Confirmed the hidden __ftol operand and the exact per-category
// scale/offset pairing with objdump (bin/halo.exe 0x424aa0..0x424b73): every branch converges
// on `ftol((random_scaled * 30.0))` after computing `random_scaled` differently per category
// (1, 2, 3, or the DAT_00672ac4 default for anything else), and the k_random_scale_65536 constant
// matches the 1/65536 scale used throughout this module's other inline PRNG rolls.
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

// UNSURE: none of these constants have an established name; read straight out of
// bin/halo.exe. DAT_00672ac4 is the default (category outside 1..3), k_random_scale_65536 is the
// shared 1/65536 PRNG-to-[0,1) scale, and ticks_per_second is the usual 30.0 ticks-per-second
// conversion also used by every dialogue-timing function in this range.
extern float k_real_one; // 0x00672ac4, 1.0
extern float k_random_scale_65536; // 0x00672b84, 1.5259022e-05 = 1/65536
extern float actor_dialogue_variant_offset_1a; // 0x00672c28, category 1 (no extra scale)
extern float actor_dialogue_variant_scale_2a;  // 0x00672df0, category 2
extern float actor_dialogue_variant_offset_2a; // 0x00672be4
extern float k_real_point_six; // 0x00672ca8, 0.6
extern float actor_dialogue_variant_offset_3a; // 0x00672bc8
extern float ticks_per_second; // 0x00672ac8, 30.0

// blam-cc: AX -> category
// Rolls the shared PRNG for categories 1-3 (each with its own scale/offset, category 1
// having no extra scale factor), or falls back to a fixed default value for anything else,
// then converts the result to a tick count clamped to [0, 255].
int32_t actor_pick_dialogue_variant_a(int16_t category)
{
    float value = k_real_one;
    int32_t ticks;

    if (category == 1) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        value = (float)(int32_t)(random_seed_global >> 0x10) * k_random_scale_65536
              + actor_dialogue_variant_offset_1a;
    } else if (category == 2) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        value = (float)(int32_t)(random_seed_global >> 0x10) * k_random_scale_65536 * actor_dialogue_variant_scale_2a
              + actor_dialogue_variant_offset_2a;
    } else if (category == 3) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        value = (float)(int32_t)(random_seed_global >> 0x10) * k_random_scale_65536 * k_real_point_six
              + actor_dialogue_variant_offset_3a;
    }

    ticks = __ftol((double)(value * ticks_per_second));
    if (ticks > 0xff) {
        return 0xff;
    }
    return ticks;
}

#if 0
Original Ghidra decompilation (0x424aa0):

int FUN_00424aa0(void)

{
  short in_AX;
  short sVar1;

  if (in_AX == 1) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  }
  else if (in_AX == 2) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  }
  else if (in_AX == 3) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  }
  sVar1 = __ftol();
  if (0xff < sVar1) {
    return 0xff;
  }
  return (int)sVar1;
}

Ground truth from objdump (bin/halo.exe @ 0x424aa0..0x424b73), since the pseudocode above
elides which scale/offset pair each branch actually uses (and category 1 turned out to have
no second fmul at all):

  424aaa: dec eax / je 0x424b26           ; category == 1 -> fmul 0x672b84 only, fadd 0x672c28
  424aad: dec eax / je 0x424aed           ; category == 2 -> fmul 0x672b84, fmul 0x672df0, fadd 0x672be4
  424ab0: dec eax / jne 0x424b57          ; category != 3 -> default (falls straight to the shared tail)
  424ab7: (category == 3) fmul 0x672b84, fmul 0x672ca8, fadd 0x672bc8
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
