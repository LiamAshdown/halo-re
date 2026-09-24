// sound_location_distance_squared  (Ghidra: FUN_0054bbd0, still unnamed there)
// address 0x54bbd0, size 115 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md's summary ("nearest spatial cluster/portal position")
// is superseded here: the stride used (0x44, sound_listener's own size) and base address
// (0x00725244 == sound_listeners[0].position.x, see types/sound.h) show `param_1` indexes
// sound_listeners, not a cluster/portal table. Location field offsets (type 0x00, position
// 0x0c/0x10/0x14) match sound_location exactly (in_ECX is `short *`, so "+6/+8/+10" are byte
// offsets 0xc/0x10/0x14).
// register convention: ECX -> location, stack -> listener_index (the draft said DX).
// UNSURE: the type-not-in-{0,1,2} fallback (`(float10)(float)in_ECX`, converting the pointer's
// own bit pattern to a float) is preserved literally; sound_location_type only has 3 members so
// this path should be unreachable.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern sound_listener sound_listeners[1]; // 0x00725218

// blam-cc: ECX -> location, stack -> listener_index
// Squared distance from `location` to `sound_listeners[listener_index]`'s position (type
// _sound_location_absolute), or the squared length of `location`'s own position (type
// _sound_location_listener_relative, already listener-space), or 0 for _sound_location_none.
float sound_location_distance_squared(int16_t listener_index, sound_location *location)
{
    sound_listener *listener;

    if (location->type == _sound_location_none) {
        return 0.0f;
    }
    if (location->type == _sound_location_listener_relative) {
        return location->position.z * location->position.z + location->position.y * location->position.y +
               location->position.x * location->position.x;
    }
    if (location->type != _sound_location_absolute) {
        { union { sound_location *p; float f; } bits; bits.p = location; return bits.f; } // not a valid type: the original returns the stack slot holding the saved ECX (this pointer) as a float (0x54bcc2: fld [esp])
    }

    listener = &sound_listeners[listener_index];
    return (listener->position.y - location->position.y) * (listener->position.y - location->position.y) +
           (listener->position.x - location->position.x) * (listener->position.x - location->position.x) +
           (listener->position.z - location->position.z) * (listener->position.z - location->position.z);
}

#if 0
Original Ghidra decompilation (0x54bbd0):

float10 FUN_0054bbd0(short param_1)

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
      return (float10)*(float *)(in_ECX + 10) * (float10)*(float *)(in_ECX + 10) +
             (float10)*(float *)(in_ECX + 8) * (float10)*(float *)(in_ECX + 8) +
             (float10)*(float *)(in_ECX + 6) * (float10)*(float *)(in_ECX + 6);
    }
    return (float10)(float)in_ECX;
  }
  iVar2 = param_1 * 0x44;
  return ((float10)*(float *)(&DAT_00725248 + iVar2) - (float10)*(float *)(in_ECX + 8)) *
         ((float10)*(float *)(&DAT_00725248 + iVar2) - (float10)*(float *)(in_ECX + 8)) +
         ((float10)*(float *)(&DAT_00725244 + iVar2) - (float10)*(float *)(in_ECX + 6)) *
         ((float10)*(float *)(&DAT_00725244 + iVar2) - (float10)*(float *)(in_ECX + 6)) +
         ((float10)*(float *)(&DAT_0072524c + iVar2) - (float10)*(float *)(in_ECX + 10)) *
         ((float10)*(float *)(&DAT_0072524c + iVar2) - (float10)*(float *)(in_ECX + 10));
}

Disassembly (0x54bbd0..0x54bc43, capstone; phase-4 review):

0x54bbd0: push ecx
0x54bbd1: movsx eax, word ptr [ecx]
0x54bbd4: sub eax, 0
0x54bbd7: je 0x54bc36
0x54bbd9: dec eax
0x54bbda: je 0x54bc00
0x54bbdc: dec eax
0x54bbdd: jne 0x54bc3e
0x54bbdf: fld dword ptr [ecx + 0x14]
0x54bbe2: fld dword ptr [ecx + 0x10]
0x54bbe5: fld dword ptr [ecx + 0xc]
0x54bbe8: fld st(0)
0x54bbea: fmul st(1)
0x54bbec: fld st(2)
0x54bbee: fmul st(3)
0x54bbf0: faddp st(1)
0x54bbf2: fld st(3)
0x54bbf4: fmul st(4)
0x54bbf6: faddp st(1)
0x54bbf8: fstp st(3)
0x54bbfa: fstp st(0)
0x54bbfc: fstp st(0)
0x54bbfe: pop ecx
0x54bbff: ret 
0x54bc00: movsx eax, word ptr [esp + 8]
0x54bc05: imul eax, eax, 0x44
0x54bc08: add eax, 0x725244
0x54bc0d: fld dword ptr [eax]
0x54bc0f: fsub dword ptr [ecx + 0xc]
0x54bc12: fld dword ptr [eax + 4]
0x54bc15: fsub dword ptr [ecx + 0x10]
0x54bc18: fld dword ptr [eax + 8]
0x54bc1b: fsub dword ptr [ecx + 0x14]
0x54bc1e: fld st(0)
0x54bc20: fmul st(1)
0x54bc22: fld st(3)
0x54bc24: fmul st(4)
0x54bc26: faddp st(1)
0x54bc28: fld st(2)
0x54bc2a: fmul st(3)
0x54bc2c: faddp st(1)
0x54bc2e: fstp st(3)
0x54bc30: fstp st(0)
0x54bc32: fstp st(0)
0x54bc34: pop ecx
0x54bc35: ret 
0x54bc36: fld dword ptr [0x672ac0]
0x54bc3c: pop ecx
0x54bc3d: ret 
0x54bc3e: fld dword ptr [esp]
0x54bc41: pop ecx
0x54bc42: ret 
#endif
