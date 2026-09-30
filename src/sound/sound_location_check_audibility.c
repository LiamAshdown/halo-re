// sound_location_check_audibility  (Ghidra: FUN_0054bb20, still unnamed there)
// address 0x54bb20, size 171 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: `in_EAX + 0x1e` (a `short *`, so byte offset 0x3c) matches sound_location.occlusion
// exactly, and types/sound.h's own struct comment attributes that exact check to this address:
// "occlusion 0x3c 1.0 disables the channel (0x54bb20)". Calls sound_location_distance_squared
// (0x54bbd0) with listener_index -1 for the listener-relative case and 0 for the absolute case,
// matching sound_listeners' single entry.
// register convention: EAX -> location, stack -> max_distance.
// Phase-4 review: checked against the disassembly appended below; the body matched except the
// obstruction call, which takes the location in EBX and the chosen listener in AX besides the
// stack distance. Note the listener-relative case compares the SQUARED distance with the
// unsquared max_distance (binary behaviour, kept). Returns AX.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern sound_listener sound_listeners[1]; // 0x00725218


extern double sqrt(double x); // FSQRT, Ghidra's SQRT() pseudo-function

// blam-cc: EAX -> location, stack -> max_distance
// Checks whether `location` is close enough to be audible (within max_distance of a listener,
// and not fully occluded), and picks which listener it is closest to. Returns 0 (listener 0) if
// audible, or -1 if `location` is unspatialized, too far, or fully occluded (occlusion == 1.0).
int16_t sound_location_check_audibility(sound_location *location, float max_distance)
{
    float nearest_distance_squared;
    int16_t listener_index;

    listener_index = -1;

    if (location->type == _sound_location_none) {
        return 0;
    }

    if (location->type == _sound_location_listener_relative) {
        if (sound_location_distance_squared(-1, location) < max_distance) {
            return 0;
        }
    } else {
        nearest_distance_squared = 3.4028235e+38f;
        if (sound_listeners[0].valid) {
            float distance_squared = sound_location_distance_squared(0, location);
            if (distance_squared < 3.4028235e+38f) {
                nearest_distance_squared = distance_squared;
                listener_index = 0;
            }
        }
        if (listener_index != -1) {
            sound_compute_obstruction_occlusion(location, listener_index, (float)sqrt(nearest_distance_squared));
        }
        if (max_distance * max_distance < nearest_distance_squared || *(uint32_t *)&location->occlusion == 0x3f800000) {
            return -1;
        }
    }

    return listener_index;
}

#if 0
Original Ghidra decompilation (0x54bb20):

undefined4 FUN_0054bb20(float param_1)

{
  short *in_EAX;
  undefined2 uVar1;
  undefined2 extraout_var;
  undefined2 extraout_var_00;
  undefined2 extraout_var_01;
  short sVar2;
  float10 fVar3;
  float local_4;

  uVar1 = (undefined2)((uint)in_EAX >> 0x10);
  sVar2 = -1;
  if (*in_EAX == 0) {
    return 0;
  }
  if (*in_EAX == 2) {
    fVar3 = (float10)FUN_0054bbd0(0xffffffff);
    uVar1 = extraout_var;
    if (fVar3 < (float10)param_1) {
      return 0;
    }
  }
  else {
    local_4 = 3.4028235e+38;
    if ((DAT_00725218 != '\0') &&
       (fVar3 = (float10)FUN_0054bbd0(0), uVar1 = extraout_var_00, fVar3 < (float10)3.4028235e+38))
    {
      local_4 = (float)fVar3;
      sVar2 = 0;
    }
    if (sVar2 != -1) {
      FUN_00544aa0(SQRT(local_4));
      uVar1 = extraout_var_01;
    }
    if ((param_1 * param_1 < local_4) || (*(int *)(in_EAX + 0x1e) == 0x3f800000)) {
      return 0xffffffff;
    }
  }
  return CONCAT22(uVar1,sVar2);
}

Disassembly (0x54bb20..0x54bbcb, capstone; phase-4 review):

0x54bb20: push ecx
0x54bb21: push ebx
0x54bb22: mov ebx, eax
0x54bb24: mov ax, word ptr [ebx]
0x54bb27: push esi
0x54bb28: or esi, 0xffffffff
0x54bb2b: test ax, ax
0x54bb2e: je 0x54bb4d
0x54bb30: cmp ax, 2
0x54bb34: jne 0x54bb53
0x54bb36: push -1
0x54bb38: mov ecx, ebx
0x54bb3a: call 0x54bbd0
0x54bb3f: fcomp dword ptr [esp + 0x14]
0x54bb43: add esp, 4
0x54bb46: fnstsw ax
0x54bb48: test ah, 5
0x54bb4b: jp 0x54bbc4
0x54bb4d: pop esi
0x54bb4e: xor eax, eax
0x54bb50: pop ebx
0x54bb51: pop ecx
0x54bb52: ret 
0x54bb53: mov al, byte ptr [0x725218]
0x54bb58: test al, al
0x54bb5a: mov dword ptr [esp + 8], 0x7f7fffff
0x54bb62: je 0x54bb87
0x54bb64: push 0
0x54bb66: mov ecx, ebx
0x54bb68: call 0x54bbd0
0x54bb6d: fcom dword ptr [0x672be0]
0x54bb73: add esp, 4
0x54bb76: fnstsw ax
0x54bb78: test ah, 5
0x54bb7b: jp 0x54bb85
0x54bb7d: fstp dword ptr [esp + 8]
0x54bb81: xor esi, esi
0x54bb83: jmp 0x54bb87
0x54bb85: fstp st(0)
0x54bb87: cmp si, -1
0x54bb8b: je 0x54bba1
0x54bb8d: fld dword ptr [esp + 8]
0x54bb91: push ecx
0x54bb92: fsqrt 
0x54bb94: mov eax, esi
0x54bb96: fstp dword ptr [esp]
0x54bb99: call 0x544aa0
0x54bb9e: add esp, 4
0x54bba1: fld dword ptr [esp + 0x10]
0x54bba5: fmul dword ptr [esp + 0x10]
0x54bba9: fcomp dword ptr [esp + 8]
0x54bbad: fnstsw ax
0x54bbaf: test ah, 5
0x54bbb2: jnp 0x54bbbd
0x54bbb4: cmp dword ptr [ebx + 0x3c], 0x3f800000
0x54bbbb: jne 0x54bbc4
0x54bbbd: pop esi
0x54bbbe: or eax, 0xffffffff
0x54bbc1: pop ebx
0x54bbc2: pop ecx
0x54bbc3: ret 
0x54bbc4: mov ax, si
0x54bbc7: pop esi
0x54bbc8: pop ebx
0x54bbc9: pop ecx
0x54bbca: ret 
#endif
