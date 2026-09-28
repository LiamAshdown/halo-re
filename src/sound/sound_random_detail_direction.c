// sound_random_detail_direction  (Ghidra: FUN_0054e4d0, still unnamed there)
// address 0x54e4d0, size 251 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Computes a random 3D direction vector within
// configured yaw/pitch ranges, used to place ambient/detail sounds around a listener."; field
// offsets match SoundLoopingDetail's yaw_bounds/pitch_bounds/distance_bounds exactly
// (types/tags.h, 0x50/0x58/0x60). The three inlined LCG steps are, in order, the same body as
// random_real_range_seeded (0x4cd170, already established in src/math), called for distance,
// then pitch, then yaw; this rewrite calls that function directly instead of re-inlining it.
// register convention: EDX -> detail, ESI -> out.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "sound.h"

extern random_seed effect_random_seed; // 0x00719cd4
extern const real_point3d *global_origin3d_pointer; // 0x00696714

extern real random_real_range_seeded(random_seed *seed, real min, real max); // 0x4cd170, math module
extern double cos(double x); // x87 FCOS
extern double sin(double x); // x87 FSIN

// blam-cc: EDX -> detail, ESI -> out
// Picks a random distance within detail->distance_bounds; if nonzero, also picks a random pitch
// and yaw and converts (yaw, pitch, distance) to a Cartesian direction vector. A zero distance
// returns the zero vector instead (no detail sound placement needed).
void sound_random_detail_direction(SoundLoopingDetail *detail, real_vector3d *out)
{
    float distance;
    float pitch;
    float yaw;

    distance = random_real_range_seeded(&effect_random_seed, detail->distance_bounds[0], detail->distance_bounds[1]);
    if (distance != 0.0f) {
        pitch = random_real_range_seeded(&effect_random_seed, detail->pitch_bounds[0], detail->pitch_bounds[1]);
        yaw = random_real_range_seeded(&effect_random_seed, detail->yaw_bounds[0], detail->yaw_bounds[1]);
        out->i = (float)(cos((double)yaw) * cos((double)pitch)) * distance;
        out->j = (float)(sin((double)yaw) * cos((double)pitch)) * distance;
        out->k = (float)sin((double)pitch) * distance;
    } else {
        *out = *(const real_vector3d *)global_origin3d_pointer; // the zero vector
    }
}

#if 0
Original Ghidra decompilation (0x54e4d0):

void FUN_0054e4d0(void)

{
  undefined *puVar1;
  uint uVar2;
  int in_EDX;
  float *unaff_ESI;
  float10 fVar3;
  float10 fVar4;
  float10 fVar5;
  float10 fVar6;
  float10 fVar7;

  puVar1 = PTR_DAT_00696714;
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  fVar3 = ((float10)*(float *)(in_EDX + 100) - (float10)*(float *)(in_EDX + 0x60)) *
          (float10)(DAT_00719cd4 >> 0x10) * (float10)1.5259022e-05 +
          (float10)*(float *)(in_EDX + 0x60);
  if (fVar3 != (float10)0.0) {
    uVar2 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    DAT_00719cd4 = uVar2 * 0x19660d + 0x3c6ef35f;
    fVar4 = (float10)(uVar2 >> 0x10) * (float10)1.5259022e-05 *
            ((float10)*(float *)(in_EDX + 0x5c) - (float10)*(float *)(in_EDX + 0x58)) +
            (float10)*(float *)(in_EDX + 0x58);
    fVar5 = (float10)(DAT_00719cd4 >> 0x10) * (float10)1.5259022e-05 *
            ((float10)*(float *)(in_EDX + 0x54) - (float10)*(float *)(in_EDX + 0x50)) +
            (float10)*(float *)(in_EDX + 0x50);
    fVar6 = (float10)fcos(fVar4);
    fVar7 = (float10)fcos(fVar5);
    *unaff_ESI = (float)(fVar7 * fVar6 * fVar3);
    fVar5 = (float10)fsin(fVar5);
    unaff_ESI[1] = (float)(fVar5 * fVar6 * fVar3);
    fVar4 = (float10)fsin(fVar4);
    unaff_ESI[2] = (float)(fVar4 * fVar3);
    return;
  }
  *unaff_ESI = *(float *)PTR_DAT_00696714;
  unaff_ESI[1] = *(float *)(puVar1 + 4);
  unaff_ESI[2] = *(float *)(puVar1 + 8);
  return;
}

Disassembly (0x54e4d0..0x54e5cb, capstone; phase-4 review):

0x54e4d0: push ecx
0x54e4d1: mov ecx, dword ptr [0x719cd4]
0x54e4d7: fld dword ptr [edx + 0x64]
0x54e4da: fld dword ptr [edx + 0x60]
0x54e4dd: imul ecx, ecx, 0x19660d
0x54e4e3: add ecx, 0x3c6ef35f
0x54e4e9: mov eax, ecx
0x54e4eb: shr eax, 0x10
0x54e4ee: mov dword ptr [esp], eax
0x54e4f1: mov dword ptr [0x719cd4], ecx
0x54e4f7: fild dword ptr [esp]
0x54e4fa: fmul dword ptr [0x672b84]
0x54e500: fxch st(2)
0x54e502: fsub st(1)
0x54e504: fmulp st(2)
0x54e506: fxch st(1)
0x54e508: fadd st(1)
0x54e50a: fxch st(1)
0x54e50c: fstp st(0)
0x54e50e: fld dword ptr [0x672ac0]
0x54e514: fld st(1)
0x54e516: fucompp 
0x54e518: fnstsw ax
0x54e51a: test ah, 0x44
0x54e51d: jnp 0x54e5b1
0x54e523: fld dword ptr [edx + 0x5c]
0x54e526: imul ecx, ecx, 0x19660d
0x54e52c: fld dword ptr [edx + 0x58]
0x54e52f: fxch st(1)
0x54e531: fsub st(1)
0x54e533: add ecx, 0x3c6ef35f
0x54e539: mov eax, ecx
0x54e53b: shr eax, 0x10
0x54e53e: mov dword ptr [0x719cd4], ecx
0x54e544: imul ecx, ecx, 0x19660d
0x54e54a: mov dword ptr [esp], eax
0x54e54d: add ecx, 0x3c6ef35f
0x54e553: fild dword ptr [esp]
0x54e556: fmul dword ptr [0x672b84]
0x54e55c: fmulp st(1)
0x54e55e: fadd st(1)
0x54e560: fxch st(1)
0x54e562: fstp st(0)
0x54e564: fld dword ptr [edx + 0x54]
0x54e567: fld dword ptr [edx + 0x50]
0x54e56a: mov dword ptr [0x719cd4], ecx
0x54e570: fxch st(1)
0x54e572: shr ecx, 0x10
0x54e575: fsub st(1)
0x54e577: mov dword ptr [esp], ecx
0x54e57a: fild dword ptr [esp]
0x54e57d: fmul dword ptr [0x672b84]
0x54e583: fmulp st(1)
0x54e585: fadd st(1)
0x54e587: fxch st(1)
0x54e589: fstp st(0)
0x54e58b: fld st(1)
0x54e58d: fcos 
0x54e58f: fld st(1)
0x54e591: fcos 
0x54e593: fmul st(1)
0x54e595: fmul st(4)
0x54e597: fstp dword ptr [esi]
0x54e599: fxch st(1)
0x54e59b: fsin 
0x54e59d: fmul st(1)
0x54e59f: fmul st(3)
0x54e5a1: fstp dword ptr [esi + 4]
0x54e5a4: fstp st(0)
0x54e5a6: fsin 
0x54e5a8: fmul st(1)
0x54e5aa: fstp dword ptr [esi + 8]
0x54e5ad: fstp st(0)
0x54e5af: pop ecx
0x54e5b0: ret 
0x54e5b1: mov edx, dword ptr [0x696714]
0x54e5b7: fstp st(0)
0x54e5b9: mov eax, dword ptr [edx]
0x54e5bb: mov dword ptr [esi], eax
0x54e5bd: mov ecx, dword ptr [edx + 4]
0x54e5c0: mov dword ptr [esi + 4], ecx
0x54e5c3: mov edx, dword ptr [edx + 8]
0x54e5c6: mov dword ptr [esi + 8], edx
0x54e5c9: pop ecx
0x54e5ca: ret 
#endif
