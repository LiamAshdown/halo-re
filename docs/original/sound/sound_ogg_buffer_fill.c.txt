// sound_ogg_buffer_fill  (Ghidra: sound_ogg_buffer_fill, already named)
// address 0x545920, size 264 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Fills a destination buffer with decoded Ogg
//   Vorbis PCM data from a bounds-checked compressed sample pool, padding and counting
//   underruns."; reuses src/sound/sound_pcm_buffer_read.c's identical sound-cache bounds check
//   and error-buffer append; opens/reads through src/sound/sound_ogg_stream_open.c and
//   src/sound/sound_ogg_stream_read.c; sound_ogg_underrun_count is types/sound.h's own
//   "global 0x00721f14: int32_t sound_ogg_underrun_count // 0x545920".
// register convention: SoundPermutation* in param_1, destination buffer in param_2, requested
//   size in param_3, crosslap-request flag pointer in param_4; sound_stream_decoder* in ECX
//   (in_ECX), bytes-actually-filled output pointer in EAX (in_EAX).
// blam-cc: stack -> (permutation, destination, requested_size, want_crosslap), ECX -> decoder,
//   EAX -> bytes_filled_out
// The decoder arrives in ECX and is moved to ESI for sound_ogg_stream_open and to EDI for
//   sound_ogg_stream_read (checked); EAX is the bytes-filled out-parameter.
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
extern int32_t sound_ogg_underrun_count;    // 0x00721f14

extern uint8_t sound_ogg_stream_open(sound_stream_decoder *decoder, void *data, int32_t size); // 0x544eb0
extern int32_t sound_ogg_stream_read(sound_stream_decoder *decoder, char *buffer, int32_t size,
    char *want_crosslap); // 0x5451d0

// blam-cc: stack -> (permutation, destination, requested_size, want_crosslap), ECX -> decoder,
// EAX -> bytes_filled_out
// Decodes up to `requested_size` bytes of PCM from `permutation`'s cached (bounds-checked)
// compressed sample pool into `destination` via `decoder`, opening the stream on first use.
// Pads the buffer with silence and counts an underrun when the stream runs dry before the
// permutation's declared buffer_size is reached. Returns the bytes still owed for this buffer
// (never negative).
uint32_t sound_ogg_buffer_fill(SoundPermutation *permutation, void *destination, uint32_t requested_size,
    char *want_crosslap, sound_stream_decoder *decoder, uint32_t *bytes_filled_out)
{
    uint32_t sample_pointer = *(uint32_t *)&((struct SoundPermutation *)permutation)->cache_page;
    int32_t remaining;

    if (sample_pointer < (uint32_t)sound_cache_memory ||
        (uint32_t)sound_cache_size_megabytes * 0x100000 + (uint32_t)sound_cache_memory <
            permutation->samples.size + sample_pointer) {
        if (strlen(error_text_buffer) < 0xd6) {
            sprintf(error_text_buffer + strlen(error_text_buffer), "trying to queue sound but samples is null.");
        }
    } else {
        uint32_t bytes_read;

        if (decoder->open == 0) {
            sound_ogg_stream_open(decoder, (void *)sample_pointer, permutation->samples.size);
        } else {
            *want_crosslap = 0;
        }

        bytes_read = (uint32_t)sound_ogg_stream_read(decoder, (char *)destination, requested_size, want_crosslap);
        *bytes_filled_out = bytes_read;

        if (bytes_read == 0 && permutation->buffer_size != (uint32_t)decoder->decoded_bytes) {
            uint32_t pad = permutation->buffer_size - decoder->decoded_bytes;

            if (requested_size <= pad) {
                pad = requested_size;
            }
            *bytes_filled_out = pad;
            decoder->decoded_bytes += pad;

            memset(destination, 0, pad);
            sound_ogg_underrun_count += 1;
        }
    }

    remaining = (int32_t)(permutation->buffer_size - decoder->decoded_bytes);
    return (remaining < 1) ? 0 : (uint32_t)remaining;
}

#if 0
Original Ghidra decompilation (0x545920):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint sound_ogg_buffer_fill(int param_1,undefined4 *param_2,uint param_3,undefined1 *param_4)

{
  char *pcVar1;
  uint *in_EAX;
  uint uVar2;
  char *pcVar3;
  int in_ECX;
  uint uVar4;

  uVar2 = *(uint *)(param_1 + 0x30);
  if ((uVar2 < DAT_006ac554) ||
     (DAT_006869c4 * 0x100000 + DAT_006ac554 < *(int *)(param_1 + 0x40) + uVar2)) {
    pcVar1 = &DAT_006e35c8;
    do {
      pcVar3 = pcVar1;
      pcVar1 = pcVar3 + 1;
    } while (*pcVar3 != '\0');
    if (pcVar3 + -0x6e359e < &DAT_00000100) {
      pcVar1 = &DAT_006e35c8;
      do {
        pcVar3 = pcVar1;
        pcVar1 = pcVar3 + 1;
      } while (*pcVar3 != '\0');
      _sprintf(pcVar3,"trying to queue sound but samples is null.");
    }
  }
  else {
    if (*(char *)(in_ECX + 0x5a8) == '\0') {
      sound_ogg_stream_open(uVar2,*(int *)(param_1 + 0x40));
    }
    else {
      *param_4 = 0;
    }
    uVar2 = sound_ogg_stream_read(param_2,param_3,param_4);
    *in_EAX = uVar2;
    if (uVar2 == 0) {
      if (*(int *)(param_1 + 0x38) != *(int *)(in_ECX + 4)) {
        uVar2 = *(int *)(param_1 + 0x38) - *(int *)(in_ECX + 4);
        if ((int)param_3 <= (int)uVar2) {
          uVar2 = param_3;
        }
        *in_EAX = uVar2;
        *(int *)(in_ECX + 4) = *(int *)(in_ECX + 4) + uVar2;
        uVar2 = *in_EAX;
        for (uVar4 = uVar2 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
          *param_2 = 0;
          param_2 = param_2 + 1;
        }
        for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
          *(undefined1 *)param_2 = 0;
          param_2 = (undefined4 *)((int)param_2 + 1);
        }
        _DAT_00721f14 = _DAT_00721f14 + 1;
      }
    }
  }
  uVar2 = *(int *)(param_1 + 0x38) - *(int *)(in_ECX + 4);
  return uVar2 & ((int)uVar2 < 1) - 1;
}

Disassembly (0x545920..0x545a28, capstone; phase-4 review):

0x545920: mov edx, dword ptr [esp + 4]
0x545924: push ebx
0x545925: push ebp
0x545926: mov ebp, dword ptr [esp + 0x14]
0x54592a: push esi
0x54592b: mov ebx, eax
0x54592d: mov eax, dword ptr [edx + 0x30]
0x545930: mov esi, ecx
0x545932: mov ecx, dword ptr [0x6ac554]
0x545938: cmp eax, ecx
0x54593a: push edi
0x54593b: jb 0x5459cc
0x545941: movsx edi, word ptr [0x6869c4]
0x545948: mov edx, dword ptr [edx + 0x40]
0x54594b: shl edi, 0x14
0x54594e: add edi, ecx
0x545950: lea ecx, [edx + eax]
0x545953: cmp ecx, edi
0x545955: ja 0x5459cc
0x545957: mov cl, byte ptr [esi + 0x5a8]
0x54595d: test cl, cl
0x54595f: jne 0x54596d
0x545961: push edx
0x545962: push eax
0x545963: call 0x544eb0
0x545968: add esp, 8
0x54596b: jmp 0x545974
0x54596d: mov edx, dword ptr [esp + 0x20]
0x545971: mov byte ptr [edx], 0
0x545974: mov eax, dword ptr [esp + 0x20]
0x545978: mov ecx, dword ptr [esp + 0x18]
0x54597c: push eax
0x54597d: push ebp
0x54597e: push ecx
0x54597f: mov edi, esi
0x545981: call 0x5451d0
0x545986: add esp, 0xc
0x545989: test eax, eax
0x54598b: mov dword ptr [ebx], eax
0x54598d: jne 0x545a0d
0x545993: mov edx, dword ptr [esp + 0x14]
0x545997: mov eax, dword ptr [edx + 0x38]
0x54599a: mov ecx, dword ptr [esi + 4]
0x54599d: cmp eax, ecx
0x54599f: je 0x545a0d
0x5459a1: sub eax, ecx
0x5459a3: cmp eax, ebp
0x5459a5: jl 0x5459a9
0x5459a7: mov eax, ebp
0x5459a9: mov edi, dword ptr [esp + 0x18]
0x5459ad: mov dword ptr [ebx], eax
0x5459af: add dword ptr [esi + 4], eax
0x5459b2: mov ecx, dword ptr [ebx]
0x5459b4: mov edx, ecx
0x5459b6: shr ecx, 2
0x5459b9: xor eax, eax
0x5459bb: rep stosd dword ptr es:[edi], eax
0x5459bd: mov ecx, edx
0x5459bf: and ecx, 3
0x5459c2: rep stosb byte ptr es:[edi], al
0x5459c4: inc dword ptr [0x721f14]
0x5459ca: jmp 0x545a0d
0x5459cc: mov eax, 0x6e35c8
0x5459d1: lea edx, [eax + 1]
0x5459d4: mov cl, byte ptr [eax]
0x5459d6: inc eax
0x5459d7: test cl, cl
0x5459d9: jne 0x5459d4
0x5459db: sub eax, edx
0x5459dd: add eax, 0x2a
0x5459e0: cmp eax, 0x100
0x5459e5: jae 0x545a0d
0x5459e7: mov eax, 0x6e35c8
0x5459ec: lea edx, [eax + 1]
0x5459ef: nop 
0x5459f0: mov cl, byte ptr [eax]
0x5459f2: inc eax
0x5459f3: test cl, cl
0x5459f5: jne 0x5459f0
0x5459f7: sub eax, edx
0x5459f9: lea eax, [eax + 0x6e35c8]
0x5459ff: push 0x67196c
0x545a04: push eax
0x545a05: call 0x623693
0x545a0a: add esp, 8
0x545a0d: mov ecx, dword ptr [esp + 0x14]
0x545a11: mov ebx, dword ptr [esi + 4]
0x545a14: mov eax, dword ptr [ecx + 0x38]
0x545a17: sub eax, ebx
0x545a19: xor edx, edx
0x545a1b: test eax, eax
0x545a1d: setle dl
0x545a20: pop edi
0x545a21: pop esi
0x545a22: pop ebp
0x545a23: pop ebx
0x545a24: dec edx
0x545a25: and eax, edx
0x545a27: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
