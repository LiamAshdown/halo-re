// sound_ogg_seek_callback  (Ghidra: sound_ogg_seek_callback, already named)
// address 0x544e00, size 176 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: out/phase4/sound_functions.md summary "Implements a fseek-style (SEEK_SET/CUR/END)
//   position update with bounds checking, used as a custom seek callback for the Ogg Vorbis
//   stream reader."; matches libvorbisfile's ov_callbacks seek_func signature exactly
//   (int (*)(void *datasource, ogg_int64_t offset, int whence)), which on cdecl x86 passes the
//   64-bit offset as two 32-bit values (param_1 low, param_2 high) -- the manual 32-bit
//   carry/sign-extension arithmetic Ghidra shows in every branch is the compiler's own emulation
//   of 64-bit comparisons against that split value; this rewrite reassembles it into a single
//   int64_t and performs the equivalent 64-bit arithmetic directly (provably the same result for
//   every input, since each branch's bounds check reduces to 0 <= new_position <= size). The
//   struct is types/sound.h sound_ogg_memory_file (position 0x00, size 0x08, end_of_file 0x0c).
// register convention: ESI -> file, EAX -> whence, stack -> (offset_low, offset_high).
// Phase-4 review (disassembly appended below): this is not the ov_callbacks entry itself; the
//   seek thunk at 0x544da0 (not a Ghidra function) moves datasource into ESI and whence into EAX
//   and calls it. Body confirmed; returns 0, or -1 when the target is outside [0, size]; an
//   unknown whence returns 0 without moving.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "sound.h"
#include "fn_sound.h"

// blam-cc: ESI -> file, EAX -> whence, stack -> (offset_low, offset_high)
// fseek-style SEEK_SET(0)/SEEK_CUR(1)/SEEK_END(2) position update on a bounds-checked in-memory
// Ogg Vorbis data source. Returns 0 on success, -1 if the resulting position would fall outside
// [0, size], 0 for an unrecognized whence value.
int32_t sound_ogg_seek_callback(sound_ogg_memory_file *file, uint32_t offset_low, int32_t offset_high, int32_t whence)
{
    int64_t offset = ((int64_t)offset_high << 32) | offset_low;
    int64_t new_position;

    file->end_of_file = 0;

    switch (whence) {
    case 0: // SEEK_SET
        new_position = offset;
        break;
    case 1: // SEEK_CUR
        new_position = (int64_t)file->position + offset;
        break;
    case 2: // SEEK_END
        new_position = (int64_t)file->size + offset;
        break;
    default:
        return 0;
    }

    if (new_position >= 0 && new_position <= (int64_t)file->size) {
        file->position = (int32_t)new_position;
        return 0;
    }

    return -1;
}

#if 0
Original Ghidra decompilation (0x544e00):

undefined4 sound_ogg_seek_callback(uint param_1,int param_2)

{
  int in_EAX;
  uint uVar1;
  int iVar2;
  int iVar3;
  uint *unaff_ESI;

  *(undefined1 *)(unaff_ESI + 3) = 0;
  if (in_EAX == 0) {
    if (-1 < param_2) {
      iVar2 = (int)unaff_ESI[2] >> 0x1f;
      if ((param_2 <= iVar2) && ((param_2 < iVar2 || (param_1 <= unaff_ESI[2])))) {
        *unaff_ESI = param_1;
        return 0;
      }
    }
    return 0xffffffff;
  }
  if (in_EAX == 1) {
    uVar1 = *unaff_ESI;
    iVar2 = ((int)uVar1 >> 0x1f) + param_2 + (uint)CARRY4(uVar1,param_1);
    iVar3 = (int)unaff_ESI[2] >> 0x1f;
    if ((iVar2 <= iVar3) && (((iVar2 < iVar3 || (uVar1 + param_1 <= unaff_ESI[2])) && (-1 < iVar2)))
       ) {
      *unaff_ESI = uVar1 + param_1;
      return 0;
    }
  }
  else {
    if (in_EAX != 2) {
      return 0;
    }
    if ((param_2 < 1) && ((param_2 < 0 || (param_1 == 0)))) {
      uVar1 = -unaff_ESI[2];
      iVar2 = (int)uVar1 >> 0x1f;
      if ((iVar2 <= param_2) && ((iVar2 < param_2 || (uVar1 <= param_1)))) {
        *unaff_ESI = unaff_ESI[2] + param_1;
        return 0;
      }
    }
  }
  return 0xffffffff;
}

Disassembly (0x544e00..0x544eb0, capstone; phase-4 review):

0x544e00: push ebp
0x544e01: xor ebp, ebp
0x544e03: test eax, eax
0x544e05: push edi
0x544e06: mov byte ptr [esi + 0xc], 0
0x544e0a: jne 0x544e39
0x544e0c: mov edi, dword ptr [esp + 0x10]
0x544e10: test edi, edi
0x544e12: jl 0x544e33
0x544e14: mov ecx, dword ptr [esp + 0xc]
0x544e18: jg 0x544e1e
0x544e1a: test ecx, ecx
0x544e1c: jb 0x544e33
0x544e1e: mov eax, dword ptr [esi + 8]
0x544e21: cdq 
0x544e22: cmp edi, edx
0x544e24: jg 0x544e33
0x544e26: jl 0x544e2c
0x544e28: cmp ecx, eax
0x544e2a: ja 0x544e33
0x544e2c: pop edi
0x544e2d: mov eax, ebp
0x544e2f: mov dword ptr [esi], ecx
0x544e31: pop ebp
0x544e32: ret 
0x544e33: pop edi
0x544e34: or eax, 0xffffffff
0x544e37: pop ebp
0x544e38: ret 
0x544e39: cmp eax, 1
0x544e3c: push ebx
0x544e3d: jne 0x544e7d
0x544e3f: mov ebx, dword ptr [esi]
0x544e41: mov ebp, dword ptr [esp + 0x10]
0x544e45: mov eax, ebx
0x544e47: cdq 
0x544e48: mov edi, eax
0x544e4a: mov eax, dword ptr [esp + 0x14]
0x544e4e: add edi, ebp
0x544e50: mov ecx, edx
0x544e52: adc ecx, eax
0x544e54: mov eax, dword ptr [esi + 8]
0x544e57: cdq 
0x544e58: cmp ecx, edx
0x544e5a: jg 0x544e76
0x544e5c: jl 0x544e62
0x544e5e: cmp edi, eax
0x544e60: ja 0x544e76
0x544e62: test ecx, ecx
0x544e64: jl 0x544e76
0x544e66: jg 0x544e6c
0x544e68: test edi, edi
0x544e6a: jb 0x544e76
0x544e6c: add ebx, ebp
0x544e6e: mov dword ptr [esi], ebx
0x544e70: pop ebx
0x544e71: pop edi
0x544e72: xor eax, eax
0x544e74: pop ebp
0x544e75: ret 
0x544e76: pop ebx
0x544e77: pop edi
0x544e78: or eax, 0xffffffff
0x544e7b: pop ebp
0x544e7c: ret 
0x544e7d: cmp eax, 2
0x544e80: jne 0x544eaa
0x544e82: mov ebx, dword ptr [esp + 0x14]
0x544e86: test ebx, ebx
0x544e88: jg 0x544e76
0x544e8a: mov edi, dword ptr [esp + 0x10]
0x544e8e: jl 0x544e94
0x544e90: test edi, edi
0x544e92: ja 0x544e76
0x544e94: mov ecx, dword ptr [esi + 8]
0x544e97: mov eax, ecx
0x544e99: neg eax
0x544e9b: cdq 
0x544e9c: cmp ebx, edx
0x544e9e: jl 0x544e76
0x544ea0: jg 0x544ea6
0x544ea2: cmp edi, eax
0x544ea4: jb 0x544e76
0x544ea6: add ecx, edi
0x544ea8: mov dword ptr [esi], ecx
0x544eaa: pop ebx
0x544eab: pop edi
0x544eac: mov eax, ebp
0x544eae: pop ebp
0x544eaf: ret 
#endif
