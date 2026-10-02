// sound_location_distance  (Ghidra: FUN_0054bc50, still unnamed there)
// address 0x54bc50, size 119 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: same field/stride evidence as sound_location_distance_squared.c (0x54bbd0), just
// with a sqrt() around each result; out/phase4/sound_functions.md's summary agrees ("real
// distance" vs "squared distance").
// register convention: ECX -> location, stack -> listener_index (the draft said DX).
// UNSURE: same unreachable pointer-to-float fallback as 0x54bbd0, preserved literally.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern sound_listener sound_listeners[1]; // 0x00725218

extern double sqrt(double x); // FSQRT, Ghidra's SQRT() pseudo-function

// blam-cc: ECX -> location, stack -> listener_index
// Real distance from `location` to `sound_listeners[listener_index]`'s position (type
// _sound_location_absolute), or the length of `location`'s own position (type
// _sound_location_listener_relative), or 0 for _sound_location_none.
float sound_location_distance(int16_t listener_index, sound_location *location)
{
    sound_listener *listener;

    if (location->type == _sound_location_none) {
        return 0.0f;
    }
    if (location->type == _sound_location_listener_relative) {
        return sqrt(location->position.z * location->position.z + location->position.y * location->position.y +
                      location->position.x * location->position.x);
    }
    if (location->type != _sound_location_absolute) {
        { union { sound_location *p; float f; } bits; bits.p = location; return bits.f; } // not a valid type: the original returns the stack slot holding the saved ECX (this pointer) as a float (0x54bcc2: fld [esp])
    }

    listener = &sound_listeners[listener_index];
    return sqrt((listener->position.y - location->position.y) * (listener->position.y - location->position.y) +
                 (listener->position.x - location->position.x) * (listener->position.x - location->position.x) +
                 (listener->position.z - location->position.z) * (listener->position.z - location->position.z));
}

#if 0
Original Ghidra decompilation (0x54bc50):

float10 FUN_0054bc50(short param_1)

{
  short sVar1;
  int iVar2;
  short *in_ECX;

  sVar1 = *in_ECX;
  if (sVar1 == 0) {
    return (float10)0.0;
  }
  if (sVar1 != 1) {
    if (sVar1 == 2) {
      return SQRT((float10)*(float *)(in_ECX + 10) * (float10)*(float *)(in_ECX + 10) +
                  (float10)*(float *)(in_ECX + 8) * (float10)*(float *)(in_ECX + 8) +
                  (float10)*(float *)(in_ECX + 6) * (float10)*(float *)(in_ECX + 6));
    }
    return (float10)(float)in_ECX;
  }
  iVar2 = param_1 * 0x44;
  return SQRT(((float10)*(float *)(&DAT_00725248 + iVar2) - (float10)*(float *)(in_ECX + 8)) *
              ((float10)*(float *)(&DAT_00725248 + iVar2) - (float10)*(float *)(in_ECX + 8)) +
              ((float10)*(float *)(&DAT_00725244 + iVar2) - (float10)*(float *)(in_ECX + 6)) *
              ((float10)*(float *)(&DAT_00725244 + iVar2) - (float10)*(float *)(in_ECX + 6)) +
              ((float10)*(float *)(&DAT_0072524c + iVar2) - (float10)*(float *)(in_ECX + 10)) *
              ((float10)*(float *)(&DAT_0072524c + iVar2) - (float10)*(float *)(in_ECX + 10)));
}

Disassembly (0x54bc50..0x54bcc7, capstone; phase-4 review):

0x54bc50: push ecx
0x54bc51: movsx eax, word ptr [ecx]
0x54bc54: sub eax, 0
0x54bc57: je 0x54bcba
0x54bc59: dec eax
0x54bc5a: je 0x54bc82
0x54bc5c: dec eax
0x54bc5d: jne 0x54bcc2
0x54bc5f: fld dword ptr [ecx + 0x14]
0x54bc62: fld dword ptr [ecx + 0x10]
0x54bc65: fld dword ptr [ecx + 0xc]
0x54bc68: fld st(0)
0x54bc6a: fmul st(1)
0x54bc6c: fld st(2)
0x54bc6e: fmul st(3)
0x54bc70: faddp st(1)
0x54bc72: fld st(3)
0x54bc74: fmul st(4)
0x54bc76: faddp st(1)
0x54bc78: fsqrt 
0x54bc7a: fstp st(3)
0x54bc7c: fstp st(0)
0x54bc7e: fstp st(0)
0x54bc80: pop ecx
0x54bc81: ret 
0x54bc82: movsx eax, word ptr [esp + 8]
0x54bc87: imul eax, eax, 0x44
0x54bc8a: add eax, 0x725244
0x54bc8f: fld dword ptr [eax]
0x54bc91: fsub dword ptr [ecx + 0xc]
0x54bc94: fld dword ptr [eax + 4]
0x54bc97: fsub dword ptr [ecx + 0x10]
0x54bc9a: fld dword ptr [eax + 8]
0x54bc9d: fsub dword ptr [ecx + 0x14]
0x54bca0: fld st(0)
0x54bca2: fmul st(1)
0x54bca4: fld st(3)
0x54bca6: fmul st(4)
0x54bca8: faddp st(1)
0x54bcaa: fld st(2)
0x54bcac: fmul st(3)
0x54bcae: faddp st(1)
0x54bcb0: fsqrt 
0x54bcb2: fstp st(3)
0x54bcb4: fstp st(0)
0x54bcb6: fstp st(0)
0x54bcb8: pop ecx
0x54bcb9: ret 
0x54bcba: fld dword ptr [0x672ac0]
0x54bcc0: pop ecx
0x54bcc1: ret 
0x54bcc2: fld dword ptr [esp]
0x54bcc5: pop ecx
0x54bcc6: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
