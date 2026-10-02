// sound_pcm_buffer_read  (Ghidra: sound_pcm_buffer_read, already named)
// address 0x545860, size 186 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Copies raw PCM audio bytes from a
//   bounds-checked sample pool into a destination buffer, logging an error if the source range is
//   invalid."; types/tags.h SoundPermutation (buffer_size 0x38, samples TagDataOffset 0x40, whose
//   first field is size) and types/sound.h's runtime note "samples_pointer 0x2c and _pad_30 0x30
//   hold the sound cache datum_index and the cached sample pointer" account for every field;
//   sound_cache_memory / sound_cache_size_megabytes (src/cache/cache_reserve_map_memory.c) are
//   the established names for the bounds-check globals; the error path's target buffer is
//   types/sound.h's own "0x006e35c8 error text buffer".
// register convention: position pointer in param_1, SoundPermutation* in param_2, requested byte
//   count in ECX (in_ECX), destination buffer in param_3, bytes-actually-read output pointer in
//   EBX (unaff_EBX).
// blam-cc: stack -> (position, permutation, destination), ECX -> requested_size, EBX -> bytes_read_out
// UNSURE: the length threshold guarding the error-buffer append (strlen(buffer) < 0xd6) is
//   preserved as a literal constant; its derivation (presumably buffer capacity minus this
//   message's own length and some margin) is not re-derived here. The original scans for the
//   buffer's NUL terminator twice (once to test the threshold, once to find the append point);
//   this rewrite computes it once via strlen with no behavioral difference.
// FIXED (overnight): the clamp is a signed compare and the cache size a signed WORD, as in the binary.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#include <string.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *sound_cache_memory;            // 0x006ac554
extern int32_t sound_cache_size_megabytes;  // 0x006869c4
extern char error_text_buffer[];            // 0x006e35c8

// blam-cc: stack -> (position, permutation, destination), ECX -> requested_size, EBX -> bytes_read_out
// Copies up to `requested_size` bytes of raw PCM from `permutation`'s cached sample pool
// (bounds-checked against the sound cache's reserved memory region) into `destination`, advancing
// `*position` and reporting the number of bytes actually copied through `*bytes_read_out`. Returns
// the number of bytes remaining in the permutation's buffer after this read. Logs an error and
// copies nothing if the cached sample pointer falls outside the sound cache.
int32_t sound_pcm_buffer_read(uint32_t *position, SoundPermutation *permutation, uint32_t *bytes_read_out,
    uint32_t requested_size, void *destination)
{
    uint32_t remaining = permutation->buffer_size - *position;
    uint32_t sample_pointer = *(uint32_t *)&((struct SoundPermutation *)permutation)->cache_page;

    if ((int32_t)requested_size <= (int32_t)remaining) { // SIGNED: cmp ecx,eax; jg (0x545871)
        remaining = requested_size;
    }
    *bytes_read_out = remaining;

    if (sample_pointer >= (uint32_t)sound_cache_memory &&
        permutation->samples.size + sample_pointer <=
            ((uint32_t)(int32_t)*(int16_t *)&sound_cache_size_megabytes << 20) + (uint32_t)sound_cache_memory) { // movsx WORD (0x545886)
        uint8_t *src = (uint8_t *)(sample_pointer + (*position & 0xfffffffeu));
        uint8_t *dst = (uint8_t *)destination;
        uint32_t new_position;

        memcpy(dst, src, remaining);

        new_position = *position + *bytes_read_out;
        *position = new_position;
        return (int32_t)(permutation->buffer_size - new_position);
    }

    if (strlen(error_text_buffer) < 0xd6) {
        sprintf(error_text_buffer + strlen(error_text_buffer), "trying to queue sound but samples is null.");
    }

    return (int32_t)(permutation->buffer_size - *position);
}

#if 0
Original Ghidra decompilation (0x545860):

int sound_pcm_buffer_read(uint *param_1,int param_2,undefined4 *param_3)

{
  uint uVar1;
  char *pcVar2;
  uint uVar3;
  char *pcVar4;
  uint in_ECX;
  uint uVar5;
  uint *unaff_EBX;
  undefined4 *puVar6;

  uVar3 = *(int *)(param_2 + 0x38) - *param_1;
  if ((int)in_ECX <= (int)uVar3) {
    uVar3 = in_ECX;
  }
  *unaff_EBX = uVar3;
  uVar1 = *(uint *)(param_2 + 0x30);
  if ((DAT_006ac554 <= uVar1) &&
     (*(int *)(param_2 + 0x40) + uVar1 <= DAT_006869c4 * 0x100000 + DAT_006ac554)) {
    puVar6 = (undefined4 *)(uVar1 + (*param_1 & 0xfffffffe));
    for (uVar5 = uVar3 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
      *param_3 = *puVar6;
      puVar6 = puVar6 + 1;
      param_3 = param_3 + 1;
    }
    for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
      *(undefined1 *)param_3 = *(undefined1 *)puVar6;
      puVar6 = (undefined4 *)((int)puVar6 + 1);
      param_3 = (undefined4 *)((int)param_3 + 1);
    }
    uVar3 = *unaff_EBX;
    uVar1 = *param_1;
    *param_1 = uVar1 + uVar3;
    return *(int *)(param_2 + 0x38) - (uVar1 + uVar3);
  }
  pcVar2 = &DAT_006e35c8;
  do {
    pcVar4 = pcVar2;
    pcVar2 = pcVar4 + 1;
  } while (*pcVar4 != '\0');
  if (pcVar4 + -0x6e359e < &DAT_00000100) {
    pcVar2 = &DAT_006e35c8;
    do {
      pcVar4 = pcVar2;
      pcVar2 = pcVar4 + 1;
    } while (*pcVar4 != '\0');
    _sprintf(pcVar4,"trying to queue sound but samples is null.");
  }
  return *(int *)(param_2 + 0x38) - *param_1;
}

Disassembly (0x545860..0x54591a, capstone; phase-4 review):

0x545860: push ebp
0x545861: mov ebp, dword ptr [esp + 8]
0x545865: push esi
0x545866: mov esi, dword ptr [esp + 0x10]
0x54586a: mov eax, dword ptr [esi + 0x38]
0x54586d: push edi
0x54586e: sub eax, dword ptr [ebp]
0x545871: cmp ecx, eax
0x545873: jg 0x545877
0x545875: mov eax, ecx
0x545877: mov dword ptr [ebx], eax
0x545879: mov edx, dword ptr [esi + 0x30]
0x54587c: mov ecx, dword ptr [0x6ac554]
0x545882: cmp edx, ecx
0x545884: jb 0x5458ce
0x545886: movsx edi, word ptr [0x6869c4]
0x54588d: shl edi, 0x14
0x545890: add edi, ecx
0x545892: mov ecx, dword ptr [esi + 0x40]
0x545895: add ecx, edx
0x545897: cmp ecx, edi
0x545899: ja 0x5458ce
0x54589b: mov edi, dword ptr [esp + 0x18]
0x54589f: mov ecx, eax
0x5458a1: mov eax, dword ptr [ebp]
0x5458a4: shr eax, 1
0x5458a6: lea esi, [edx + eax*2]
0x5458a9: mov edx, ecx
0x5458ab: shr ecx, 2
0x5458ae: rep movsd dword ptr es:[edi], dword ptr [esi]
0x5458b0: mov ecx, edx
0x5458b2: and ecx, 3
0x5458b5: rep movsb byte ptr es:[edi], byte ptr [esi]
0x5458b7: mov eax, dword ptr [ebx]
0x5458b9: mov ecx, dword ptr [ebp]
0x5458bc: mov esi, dword ptr [esp + 0x14]
0x5458c0: add ecx, eax
0x5458c2: mov dword ptr [ebp], ecx
0x5458c5: mov eax, dword ptr [esi + 0x38]
0x5458c8: pop edi
0x5458c9: pop esi
0x5458ca: sub eax, ecx
0x5458cc: pop ebp
0x5458cd: ret 
0x5458ce: mov eax, 0x6e35c8
0x5458d3: lea edx, [eax + 1]
0x5458d6: mov cl, byte ptr [eax]
0x5458d8: inc eax
0x5458d9: test cl, cl
0x5458db: jne 0x5458d6
0x5458dd: sub eax, edx
0x5458df: add eax, 0x2a
0x5458e2: cmp eax, 0x100
0x5458e7: jae 0x54590e
0x5458e9: mov eax, 0x6e35c8
0x5458ee: lea edx, [eax + 1]
0x5458f1: mov cl, byte ptr [eax]
0x5458f3: inc eax
0x5458f4: test cl, cl
0x5458f6: jne 0x5458f1
0x5458f8: sub eax, edx
0x5458fa: lea ecx, [eax + 0x6e35c8]
0x545900: push 0x67196c
0x545905: push ecx
0x545906: call 0x623693
0x54590b: add esp, 8
0x54590e: mov eax, dword ptr [esi + 0x38]
0x545911: mov ecx, dword ptr [ebp]
0x545914: pop edi
0x545915: pop esi
0x545916: sub eax, ecx
0x545918: pop ebp
0x545919: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
