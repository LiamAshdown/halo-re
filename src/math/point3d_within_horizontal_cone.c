// point3d_within_horizontal_cone  (Ghidra: FUN_00414910, renamed)
// address 0x414910, size 128 bytes
// review fix (phase 4 math gate): the first draft returned 1 on dot == threshold and on a NaN
//   dot, and let a NaN length through; the binary's `test ah,0x41 / jne` exits on <= and on
//   unordered, so both late tests are now written as !(x > y).
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: out/functions.csv / out/phase4/math_functions.md do not cover this address (it is
//   one of the four large-function-session leftovers this pass was assigned); out/phase2/ai/
//   01.md's decompile (module=ai (0.8), but the body touches no actor state whatsoever -- two
//   read-only real_point3d pointers, a stack float threshold, and three .rdata float/double
//   literals). Already declared and called from two sibling ai-module files that this session
//   does not modify: src/ai/actor_select_facing_target_prop.c and
//   src/ai/actor_update_look_target.c, both as
//   `uint8_t FUN_00414910(real_point3d *to_point, real_point3d *reference, float
//   min_cos_threshold)` with "EAX->to_point, EDX->reference, stack->threshold"; this file
//   supplies the definition those externs point at, and matches their parameter names/order
//   so a future pass can drop this file's name in without touching the call sites. Its sibling
//   at 0x414990, actor_point_in_directional_lane (src/ai/actor_point_in_directional_lane.c),
//   is the same test generalized to per-side thresholds and a second reference vector; this is
//   the plain single-cone version.
// register convention, confirmed against objdump 0x414910..0x41498f: `mov ecx,[eax]` /
//   `mov eax,[eax+0x4]` read the incoming EAX pointer's x/y (no read of +0x8, so z is never
//   touched -- a horizontal/2D test despite the real_point3d parameter type, exactly like
//   0x414990's to_point/forward); the single stack slot referenced is `[esp+0xc]` after
//   `sub esp,0x8`, i.e. the caller's pre-sub [esp+0x4], one stack float. EDX is read only
//   after the length gate ([edx]/[edx+0x4]), never before, so it is the second (register)
//   argument in blam-cc order.
//   // blam-cc: EAX -> to_point, EDX -> reference, stack -> min_cos_threshold
// UNSURE: unlike 0x414990 (which normalizes its EDX operand too, if only as an unused side
//   effect), this function uses `reference` completely raw -- it is never normalized here or
//   observably by either caller before the call. The comparison is therefore
//   dot(normalize(to_point), reference) > min_cos_threshold, not a true cosine-of-angle test
//   unless the caller already passes a unit-length reference; both known call sites do (they
//   pass position_cache_a/_b, which 0x414990's header already establishes get normalized
//   in-place elsewhere in the same actor-update tick). Preserved literally either way.
// UNSURE: the tolerance/one/zero literals below are read once as .rdata evidence (see the
//   constant table at the top of this file) rather than re-derived from types/math.h's own
//   0.0001f note, since this instance is compiled as an 8-byte double literal, not the 4-byte
//   float literal that note documents for the rest of the module.

#include "tags.h"
#include "math.h"

extern double sqrt(double x); // FSQRT
extern double fabs(double x); // FABS

// blam-cc: EAX -> to_point, EDX -> reference, stack -> min_cos_threshold
// Horizontal (x,y-only) cone test: normalizes a local copy of to_point (to_point itself is
// not modified) and returns whether its dot product with the raw reference vector is strictly
// greater than min_cos_threshold. Returns 0 if to_point's horizontal length is below the module's 0.0001
// tolerance.
uint8_t point3d_within_horizontal_cone(const real_point3d *to_point, const real_point3d *reference,
                                        real min_cos_threshold)
{
    real x, y;
    real length;
    real dot;

    x = to_point->x;
    y = to_point->y;
    length = (real)sqrt((double)(y * y + x * x));
    if ((real)fabs((double)length) < 0.0001f) {
        return 0;
    }

    x = x * (1.0f / length);
    y = y * (1.0f / length);

    // UNSURE: re-checks length (the same sqrt result) with `test ah,0x41 / jne`, i.e. length
    // <= 0.0 or unordered returns 0. Only a NaN length can reach here and fail it (a NaN passes
    // the 0.0001 gate above, whose `test ah,0x5 / jnp` only exits on a strict less-than).
    // Mirrors the check in actor_point_in_directional_lane @0x414990.
    if (!(length > 0.0f)) {
        return 0;
    }

    dot = x * reference->x + y * reference->y;
    // `test ah,0x41 / jne`: C0 or C3 set -> dot <= threshold (or unordered) returns 0, so the
    // test is strictly greater-than and a NaN dot fails it.
    if (!(dot > min_cos_threshold)) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x414910):

undefined4 FUN_00414910(float param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float *in_EAX;
  undefined4 uVar5;
  float *in_EDX;
  undefined2 uVar6;

  fVar1 = *in_EAX;
  fVar2 = in_EAX[1];
  fVar3 = SQRT(fVar2 * fVar2 + fVar1 * fVar1);
  fVar4 = ABS(fVar3);
  uVar6 = (undefined2)((uint)fVar2 >> 0x10);
  uVar5 = CONCAT22(uVar6,(ushort)(fVar4 < 0.0001) << 8 | (ushort)NAN(fVar4) << 10 |
                         (ushort)(fVar4 == 0.0001) << 0xe);
  if (fVar4 >= 0.0001) {
    uVar5 = CONCAT22(uVar6,(ushort)(fVar3 < 0.0) << 8 | (ushort)NAN(fVar3) << 10 |
                           (ushort)(fVar3 == 0.0) << 0xe);
    if (fVar3 >= 0.0 && (fVar3 == 0.0) == 0) {
      fVar1 = fVar1 * (1.0 / fVar3) * *in_EDX + (1.0 / fVar3) * fVar2 * in_EDX[1];
      uVar5 = CONCAT22(uVar6,(ushort)(fVar1 < param_1) << 8 |
                             (ushort)(NAN(fVar1) || NAN(param_1)) << 10 |
                             (ushort)(fVar1 == param_1) << 0xe);
      if (fVar1 >= param_1 && (fVar1 == param_1) == 0) {
        return CONCAT31((int3)((uint)uVar5 >> 8),1);
      }
    }
  }
  return uVar5;
}

Disassembly cross-check (objdump -d -M intel bin/halo.exe, 0x414910..0x41498f):

00414910: sub esp,0x8
00414913: mov ecx,[eax]        ; to_point->x
00414915: mov eax,[eax+0x4]    ; to_point->y
00414918: mov [esp],ecx
0041491b: fld DWORD PTR [esp]
0041491e: fmul DWORD PTR [esp]
00414921: mov [esp+0x4],eax
00414925: fld DWORD PTR [esp+0x4]
00414929: xor cl,cl                       ; default return = 0
0041492b: fmul DWORD PTR [esp+0x4]
0041492f: faddp st(1),st
00414931: fsqrt
00414933: fld st(0)
00414935: fabs
00414937: fcomp QWORD PTR ds:0x672bd8     ; 9.999999747378752e-05 (0.0001f as a double literal)
0041493d: fnstsw ax
0041493f: test ah,0x5
00414942: jnp 0x414988                    ; |length| < tolerance -> return 0
00414944: fld DWORD PTR ds:0x672ac4       ; 1.0f
0041494a: fdiv st,st(1)
0041494c: fld DWORD PTR [esp]
0041494f: fmul st,st(1)
00414951: fstp DWORD PTR [esp]            ; x/length
00414954: fmul DWORD PTR [esp+0x4]
00414958: fstp DWORD PTR [esp+0x4]        ; y/length
0041495c: fcomp DWORD PTR ds:0x672ac0     ; 0.0f
00414962: fnstsw ax
00414964: test ah,0x41
00414967: jne 0x41498a                    ; length <= 0.0 or NaN -> return 0
00414969: fld DWORD PTR [esp+0x4]
0041496d: fmul DWORD PTR [edx+0x4]        ; (y/length) * reference->y
00414970: fld DWORD PTR [esp]
00414973: fmul DWORD PTR [edx]            ; (x/length) * reference->x
00414975: faddp st(1),st
00414977: fcomp DWORD PTR [esp+0xc]       ; min_cos_threshold (callers stack slot)
0041497b: fnstsw ax
0041497d: test ah,0x41
00414980: jne 0x41498a                    ; dot <= min_cos_threshold or NaN -> return 0
00414982: mov al,0x1
00414984: add esp,0x8
00414987: ret
00414988: fstp st(0)
0041498a: mov al,cl                       ; al = 0
0041498c: add esp,0x8
0041498f: ret
#endif
