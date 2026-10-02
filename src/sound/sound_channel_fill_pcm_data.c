// sound_channel_fill_pcm_data  (Ghidra: sound_channel_fill_pcm_data, already named)
// address 0x547ab0, size 337 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: out/phase4/sound_functions.md summary "Supplies PCM audio bytes for a channel by
//   pulling from its queue of raw or Ogg Vorbis sound sources, advancing the queue as sources are
//   exhausted."; directsound_channel.source/next_source/source_started/source_crosslap/
//   next_source_crosslap/source_end_cursor/decoder/state (types/sound.h); SoundPermutation.format
//   (0x28) selects the PCM (0 / 1) or Ogg Vorbis (3) reader.
// Phase-4 review (disassembly appended below) resolves what the draft called irreconcilable:
//   the byte count to produce arrives in EAX, and the first stack slot (the channel index) is
//   reused as the readers' "bytes produced" out-parameter (EBX = &slot for
//   sound_pcm_buffer_read, EAX = &slot for sound_ogg_buffer_fill). The loop therefore advances
//   by what the reader actually produced. With a format other than 0 / 1 / 3 the binary reads
//   the slot unchanged (the channel index on the first pass), which is kept literally.
//   sound_stream_decoder_close_slot gets AL = *crosslap (the draft passed 1).
// register convention: stack -> (channel_index, destination, base_position, crosslap),
//   EAX -> byte_count.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

extern void sound_stream_decoder_close_slot(sound_stream_decoder *decoder, uint8_t crosslap); // 0x545760, blam-cc: ESI, AL
extern int32_t sound_pcm_buffer_read(uint32_t *position, SoundPermutation *permutation, uint32_t *bytes_read_out,
    uint32_t requested_size, void *destination); // 0x545860, blam-cc: stack (position, permutation, destination), ECX, EBX
extern uint32_t sound_ogg_buffer_fill(SoundPermutation *permutation, void *destination, uint32_t requested_size,
    char *want_crosslap, sound_stream_decoder *decoder, uint32_t *bytes_filled_out); // 0x545920, blam-cc: stack x4, ECX, EAX

// blam-cc: stack -> (channel_index, destination, base_position, crosslap), EAX -> byte_count
// Writes `byte_count` bytes of PCM into `destination` (one locked segment of the ring buffer
// that starts at ring offset `base_position`), walking current source -> next source as each
// is exhausted and zero-filling once no source is left.
void sound_channel_fill_pcm_data(int16_t channel_index, uint8_t *destination, int32_t base_position,
    uint8_t *crosslap, int32_t byte_count)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    uint32_t produced = (uint32_t)(int32_t)channel_index; // the reused stack slot, see file header

    while (byte_count > 0) {
        SoundPermutation *source = channel->source;

        if (source != (SoundPermutation *)0 && channel->source_started == 0) {
            channel->source_started = 1;
            if (source->format == soundformat_16_bit_pcm || source->format == soundformat_xbox_adpcm) {
                channel->decoder.position = 0;
            } else if (source->format == soundformat_ogg_vorbis) {
                sound_stream_decoder_close_slot(&channel->decoder, *crosslap);
            }
        }

        source = channel->source;
        if (source == (SoundPermutation *)0) {
            int32_t i;

            for (i = 0; i < byte_count; i++) {
                destination[i] = 0;
            }
            return;
        }

        if (source->format == soundformat_16_bit_pcm || source->format == soundformat_xbox_adpcm) {
            if (sound_pcm_buffer_read((uint32_t *)&channel->decoder.position, source, &produced,
                    (uint32_t)byte_count, destination) != 0) {
                return;
            }
        } else if (source->format == soundformat_ogg_vorbis) {
            if (sound_ogg_buffer_fill(source, destination, (uint32_t)byte_count, (char *)crosslap,
                    &channel->decoder, &produced) != 0) {
                return;
            }
        }

        destination += produced & 0xfffffffe;
        channel->source_started = 0;
        byte_count -= (int32_t)produced;
        if (channel->source_end_cursor != -1) {
            channel->state = _directsound_channel_playing;
        }
        channel->source_end_cursor = (int32_t)produced + base_position;
        channel->source = channel->next_source;
        channel->source_crosslap = channel->next_source_crosslap;
        channel->next_source_crosslap = 0;
        channel->next_source = (SoundPermutation *)0;
    }
}

#if 0
Original Ghidra decompilation (0x547ab0):

void sound_channel_fill_pcm_data(uint param_1,undefined4 *param_2,int param_3,undefined4 param_4)

{
  undefined1 uVar1;
  short sVar2;
  undefined4 uVar3;
  uint in_EAX;
  int iVar4;
  uint uVar5;
  int iVar6;
  int iVar7;

  iVar6 = (int)(short)param_1;
  iVar7 = iVar6 * 0x678;
  if ((int)in_EAX < 1) {
    return;
  }
  do {
    iVar4 = (&DAT_007254b8)[iVar6 * 0x19e];
    if (iVar4 == 0) {
LAB_00547bef:
      for (uVar5 = in_EAX >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *param_2 = 0;
        param_2 = param_2 + 1;
      }
      for (uVar5 = in_EAX & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined1 *)param_2 = 0;
        param_2 = (undefined4 *)((int)param_2 + 1);
      }
      return;
    }
    if ((&DAT_007254c8)[iVar7] == '\0') {
      (&DAT_007254c8)[iVar7] = 1;
      sVar2 = *(short *)(iVar4 + 0x28);
      if ((sVar2 == 0) || (sVar2 == 1)) {
        *(undefined4 *)(&DAT_007254d0 + iVar7) = 0;
      }
      else if (sVar2 == 3) {
        FUN_00545760();
      }
    }
    iVar4 = (&DAT_007254b8)[iVar6 * 0x19e];
    if (iVar4 == 0) goto LAB_00547bef;
    sVar2 = *(short *)(iVar4 + 0x28);
    if ((sVar2 == 0) || (sVar2 == 1)) {
      iVar4 = sound_pcm_buffer_read(&DAT_007254d0 + iVar7,iVar4,param_2);
LAB_00547b83:
      if (iVar4 != 0) {
        return;
      }
    }
    else if (sVar2 == 3) {
      iVar4 = sound_ogg_buffer_fill(iVar4,param_2,in_EAX,param_4);
      goto LAB_00547b83;
    }
    param_2 = (undefined4 *)((int)param_2 + (param_1 & 0xfffffffe));
    (&DAT_007254c8)[iVar7] = 0;
    in_EAX = in_EAX - param_1;
    if (*(int *)(&DAT_007254ac + iVar7) != -1) {
      (&DAT_00725430)[iVar6 * 0x33c] = 1;
    }
    uVar3 = (&DAT_007254bc)[iVar6 * 0x19e];
    *(uint *)(&DAT_007254ac + iVar7) = param_1 + param_3;
    uVar1 = (&DAT_007254c1)[iVar7];
    (&DAT_007254b8)[iVar6 * 0x19e] = uVar3;
    (&DAT_007254c0)[iVar7] = uVar1;
    (&DAT_007254c1)[iVar7] = 0;
    (&DAT_007254bc)[iVar6 * 0x19e] = 0;
    if ((int)in_EAX < 1) {
      return;
    }
  } while( true );
}

Disassembly (0x547ab0..0x547c01, capstone; phase-4 review):

0x547ab0: push ebp
0x547ab1: movsx ebp, word ptr [esp + 8]
0x547ab6: imul ebp, ebp, 0x678
0x547abc: push edi
0x547abd: mov edi, eax
0x547abf: xor edx, edx
0x547ac1: add ebp, 0x725430
0x547ac7: cmp edi, edx
0x547ac9: jle 0x547c07
0x547acf: push ebx
0x547ad0: mov ebx, dword ptr [esp + 0x10]
0x547ad4: push esi
0x547ad5: jmp 0x547ae0
0x547ad7: lea esp, [esp]
0x547ade: mov edi, edi
0x547ae0: mov eax, dword ptr [ebp + 0x88]
0x547ae6: cmp eax, edx
0x547ae8: je 0x547bef
0x547aee: mov cl, byte ptr [ebp + 0x98]
0x547af4: test cl, cl
0x547af6: jne 0x547b28
0x547af8: mov byte ptr [ebp + 0x98], 1
0x547aff: movsx eax, word ptr [eax + 0x28]
0x547b03: sub eax, edx
0x547b05: je 0x547b22
0x547b07: dec eax
0x547b08: je 0x547b22
0x547b0a: sub eax, 2
0x547b0d: jne 0x547b28
0x547b0f: mov eax, dword ptr [esp + 0x20]
0x547b13: mov al, byte ptr [eax]
0x547b15: lea esi, [ebp + 0xa0]
0x547b1b: call 0x545760
0x547b20: jmp 0x547b28
0x547b22: mov dword ptr [ebp + 0xa0], edx
0x547b28: mov eax, dword ptr [ebp + 0x88]
0x547b2e: xor edx, edx
0x547b30: cmp eax, edx
0x547b32: je 0x547bef
0x547b38: movsx ecx, word ptr [eax + 0x28]
0x547b3c: sub ecx, edx
0x547b3e: je 0x547b68
0x547b40: dec ecx
0x547b41: je 0x547b68
0x547b43: sub ecx, 2
0x547b46: jne 0x547b8d
0x547b48: mov ecx, dword ptr [esp + 0x20]
0x547b4c: mov edx, dword ptr [esp + 0x18]
0x547b50: push ecx
0x547b51: push edi
0x547b52: push edx
0x547b53: push eax
0x547b54: lea ecx, [ebp + 0xa0]
0x547b5a: lea eax, [esp + 0x24]
0x547b5e: call 0x545920
0x547b63: add esp, 0x10
0x547b66: jmp 0x547b83
0x547b68: mov ecx, dword ptr [esp + 0x18]
0x547b6c: push ecx
0x547b6d: lea edx, [ebp + 0xa0]
0x547b73: push eax
0x547b74: mov ecx, edi
0x547b76: lea ebx, [esp + 0x1c]
0x547b7a: push edx
0x547b7b: call 0x545860
0x547b80: add esp, 0xc
0x547b83: xor edx, edx
0x547b85: cmp eax, edx
0x547b87: jne 0x547c05
0x547b89: mov ebx, dword ptr [esp + 0x14]
0x547b8d: mov ecx, dword ptr [esp + 0x18]
0x547b91: mov eax, ebx
0x547b93: shr eax, 1
0x547b95: lea eax, [ecx + eax*2]
0x547b98: mov byte ptr [ebp + 0x98], 0
0x547b9f: mov dword ptr [esp + 0x18], eax
0x547ba3: mov eax, dword ptr [ebp + 0x7c]
0x547ba6: sub edi, ebx
0x547ba8: cmp eax, -1
0x547bab: je 0x547bb3
0x547bad: mov word ptr [ebp], 1
0x547bb3: cmp edi, edx
0x547bb5: mov ecx, dword ptr [esp + 0x1c]
0x547bb9: lea eax, [ebx + ecx]
0x547bbc: mov ecx, dword ptr [ebp + 0x8c]
0x547bc2: mov dword ptr [ebp + 0x7c], eax
0x547bc5: mov al, byte ptr [ebp + 0x91]
0x547bcb: mov dword ptr [ebp + 0x88], ecx
0x547bd1: mov byte ptr [ebp + 0x90], al
0x547bd7: mov byte ptr [ebp + 0x91], 0
0x547bde: mov dword ptr [ebp + 0x8c], edx
0x547be4: jg 0x547ae0
0x547bea: pop esi
0x547beb: pop ebx
0x547bec: pop edi
0x547bed: pop ebp
0x547bee: ret 
0x547bef: mov ecx, edi
0x547bf1: mov edi, dword ptr [esp + 0x18]
0x547bf5: mov edx, ecx
0x547bf7: shr ecx, 2
0x547bfa: xor eax, eax
0x547bfc: rep stosd dword ptr es:[edi], eax
0x547bfe: mov ecx, edx
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
