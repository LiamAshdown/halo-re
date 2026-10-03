// sound_decode_dispatch  (Ghidra: FUN_0054e830, still unnamed there)
// address 0x54e830, size 137 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: only caller sound_cache_decode_permutation (0x443dd4) passes ECX = channel count,
//   EBX = the decode buffer (0x006f17ec) and on the stack the permutation's cached compressed
//   samples (+0x30) and their size (+0x40); a 0 return means success. The arithmetic is the
//   Xbox ADPCM block layout: a block is 36 bytes per channel (4-byte header plus 64 4-bit
//   samples) and decodes to 64 16-bit samples per channel (128 bytes per channel); with
//   EBX == NULL the function only returns the decoded size. The decoder itself is
//   k_sound_decode_procs[channel_count] (0x0065e640: 0x54e920 mono, 0x54ea60 stereo; slot 0 holds
//   0x7fff), stored in sound_decode_proc (0x00724a48) and called with seven arguments.
// Phase-4 review: rewritten from the disassembly appended below; the draft mixed up the
//   register and stack arguments and called the decoder with one argument.
// register convention: ECX -> channel_count, EBX -> destination, stack -> (source, source_size).

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern sound_decode_block_proc k_sound_decode_procs[3]; // 0x0065e640
extern sound_decode_block_proc sound_decode_proc;       // 0x00724a48

// blam-cc: ECX -> channel_count, EBX -> destination, stack -> (source, source_size)
// Decodes `source_size` bytes of Xbox ADPCM into `destination`: returns 0 on success, -1 when the
// decoder fails; with no destination, returns the number of bytes the decode would produce.
int32_t sound_decode_dispatch(int16_t channel_count, void *destination, void *source, int32_t source_size)
{
    uint16_t nibble_bytes = (uint16_t)(((uint16_t)(((uint16_t)((uint16_t)(channel_count * 0xfc) + 7) >> 3) + 7)) >> 3);
    uint16_t block_size = (uint16_t)((channel_count + nibble_bytes * 2) << 2); // 36 bytes per channel
    int32_t block_count = source_size / (int32_t)block_size;
    int32_t decoded_size = (((int32_t)channel_count << 10) / 8) * block_count;  // 128 bytes per channel per block
    int32_t out_0;
    int32_t out_1;

    if (destination == (void *)0) {
        return decoded_size;
    }

    sound_decode_proc = k_sound_decode_procs[channel_count];
    out_0 = 0;
    out_1 = 0;
    if (sound_decode_proc(source, destination, block_count, (int32_t)block_size, 0x40, &out_0, &out_1) == 0) {
        return -1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x54e830):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int FUN_0054e830(undefined4 param_1,int param_2)

{
  int iVar1;
  int in_ECX;
  int unaff_EBX;

  iVar1 = ((int)(in_ECX * 0x400 + (in_ECX * 0x400 >> 0x1f & 7U)) >> 3) *
          (param_2 /
          (int)((in_ECX + (uint)((ushort)(((ushort)((short)(in_ECX * 0xfc) + 7U) >> 3) + 7) >> 3) *
                          2 & 0x3fff) << 2));
  if (unaff_EBX != 0) {
    _DAT_00724a48 = *(code **)(&DAT_0065e640 + in_ECX * 4);
    iVar1 = (*_DAT_00724a48)(param_1);
    if (iVar1 == 0) {
      return -1;
    }
    iVar1 = 0;
  }
  return iVar1;
}

Disassembly (0x54e830..0x54e8b9, capstone; phase-4 review):

0x54e830: push ecx
0x54e831: mov eax, ecx
0x54e833: imul eax, eax, 0xfc
0x54e839: add ax, 7
0x54e83d: shr ax, 3
0x54e841: add ax, 7
0x54e845: shr ax, 3
0x54e849: push esi
0x54e84a: push edi
0x54e84b: lea edx, [ecx + eax*2]
0x54e84e: mov eax, dword ptr [esp + 0x14]
0x54e852: shl edx, 2
0x54e855: movzx esi, dx
0x54e858: cdq 
0x54e859: idiv esi
0x54e85b: mov edi, eax
0x54e85d: mov eax, ecx
0x54e85f: shl eax, 0xa
0x54e862: cdq 
0x54e863: and edx, 7
0x54e866: add eax, edx
0x54e868: sar eax, 3
0x54e86b: imul eax, edi
0x54e86e: test ebx, ebx
0x54e870: je 0x54e8b5
0x54e872: mov ecx, dword ptr [ecx*4 + 0x65e640]
0x54e879: lea eax, [esp + 0x14]
0x54e87d: push eax
0x54e87e: mov eax, dword ptr [esp + 0x14]
0x54e882: lea edx, [esp + 0xc]
0x54e886: push edx
0x54e887: push 0x40
0x54e889: push esi
0x54e88a: push edi
0x54e88b: push ebx
0x54e88c: push eax
0x54e88d: mov dword ptr [0x724a48], ecx
0x54e893: mov dword ptr [esp + 0x30], 0
0x54e89b: mov dword ptr [esp + 0x24], 0
0x54e8a3: call ecx
0x54e8a5: add esp, 0x1c
0x54e8a8: test eax, eax
0x54e8aa: jne 0x54e8b3
0x54e8ac: pop edi
0x54e8ad: or eax, 0xffffffff
0x54e8b0: pop esi
0x54e8b1: pop ecx
0x54e8b2: ret 
0x54e8b3: xor eax, eax
0x54e8b5: pop edi
0x54e8b6: pop esi
0x54e8b7: pop ecx
0x54e8b8: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
