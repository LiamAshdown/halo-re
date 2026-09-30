// sound_looping_find_by_owner  (Ghidra: FUN_0054e5d0, still unnamed there)
// address 0x54e5d0, size 130 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Finds a looping-sound state datum whose stored
// reference matches the given value."; types/sound.h looping_sound.owner (0x08).
// register convention: owner as the recognized parameter (param_1).
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern data_array *looping_sound_data; // 0x00724a50, "looping sounds" 0x80 x 0xe4

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module

// Linear search for the looping_sound datum whose owner field equals `owner`, or
// k_datum_index_none if there is none.
datum_index sound_looping_find_by_owner(int32_t owner)
{
    datum_index handle;
    looping_sound *state;

    handle = datum_next(-1, looping_sound_data);
    while (handle != 0xffffffff) {
        state = (looping_sound *)((uint8_t *)looping_sound_data->data + (handle & 0xffff) * sizeof(looping_sound));
        if (state->owner == owner) {
            return handle;
        }
        handle = datum_next((int16_t)handle, looping_sound_data);
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x54e5d0):

uint FUN_0054e5d0(int param_1)

{
  int iVar1;
  uint uVar2;
  short *psVar3;
  short sVar4;
  int iVar5;

  iVar1 = DAT_00724a50;
  uVar2 = datum_next();
  if (uVar2 != 0xffffffff) {
    do {
      if (*(int *)((uVar2 & 0xffff) * 0xe4 + 8 + *(int *)(iVar1 + 0x34)) == param_1) {
        return uVar2;
      }
      iVar5 = uVar2 + 1;
      uVar2 = 0xffffffff;
      sVar4 = (short)iVar5;
      if ((-1 < sVar4) && (sVar4 < *(short *)(iVar1 + 0x2e))) {
        psVar3 = (short *)((int)sVar4 * (int)*(short *)(iVar1 + 0x22) + *(int *)(iVar1 + 0x34));
        do {
          if (*psVar3 != 0) {
            uVar2 = (int)*psVar3 << 0x10 | (int)(short)iVar5;
            break;
          }
          iVar5 = iVar5 + 1;
          psVar3 = (short *)((int)psVar3 + (int)*(short *)(iVar1 + 0x22));
        } while ((short)iVar5 < *(short *)(iVar1 + 0x2e));
      }
    } while (uVar2 != 0xffffffff);
  }
  return 0xffffffff;
}

Disassembly (0x54e5d0..0x54e652, capstone; phase-4 review):

0x54e5d0: push ebx
0x54e5d1: push ebp
0x54e5d2: push esi
0x54e5d3: push edi
0x54e5d4: mov edi, dword ptr [0x724a50]
0x54e5da: or edx, 0xffffffff
0x54e5dd: call 0x4d0630
0x54e5e2: cmp eax, -1
0x54e5e5: je 0x54e64a
0x54e5e7: mov ebp, dword ptr [edi + 0x34]
0x54e5ea: lea ebx, [ebx]
0x54e5f0: mov ecx, eax
0x54e5f2: and ecx, 0xffff
0x54e5f8: imul ecx, ecx, 0xe4
0x54e5fe: mov edx, dword ptr [ecx + ebp + 8]
0x54e602: cmp edx, dword ptr [esp + 0x14]
0x54e606: je 0x54e64d
0x54e608: lea ecx, [eax + 1]
0x54e60b: or esi, 0xffffffff
0x54e60e: test cx, cx
0x54e611: jl 0x54e643
0x54e613: mov bx, word ptr [edi + 0x2e]
0x54e617: cmp cx, bx
0x54e61a: jge 0x54e643
0x54e61c: movsx edx, word ptr [edi + 0x22]
0x54e620: movsx eax, cx
0x54e623: imul eax, edx
0x54e626: add eax, ebp
0x54e628: cmp word ptr [eax], 0
0x54e62c: jne 0x54e638
0x54e62e: inc ecx
0x54e62f: add eax, edx
0x54e631: cmp cx, bx
0x54e634: jl 0x54e628
0x54e636: jmp 0x54e643
0x54e638: movsx esi, word ptr [eax]
0x54e63b: movsx eax, cx
0x54e63e: shl esi, 0x10
0x54e641: or esi, eax
0x54e643: cmp esi, -1
0x54e646: mov eax, esi
0x54e648: jne 0x54e5f0
0x54e64a: or eax, 0xffffffff
0x54e64d: pop edi
0x54e64e: pop esi
0x54e64f: pop ebp
0x54e650: pop ebx
0x54e651: ret 
#endif
