// sound_stream_decoder_close_slot  (Ghidra: FUN_00545760)
// address 0x545760, size 252 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Closes and clears one of the two
//   double-buffered Ogg Vorbis decode streams, swapping which buffer slot is considered active.";
//   types/sound.h's own header note "0x545760 zeroes 0xb4 dwords at +0x08 and +0x2d8 (two 0x2d0
//   OggVorbis_File) and the dword groups at +0x5ac and +0x5bc" matches every offset used here
//   exactly (ogg_vorbis_file[0]/[1] at decoder+0x008/+0x2d8, memory_files[0]/[1] at +0x5ac/+0x5bc,
//   open at +0x5a8, active_file at +0x5a9).
// register convention: sound_stream_decoder* in ESI (unaff_ESI); a mode flag in AL (in_AL).
// blam-cc: ESI -> decoder, AL -> smart_toggle
// UNSURE: in_AL's exact meaning is inferred from its effect: false always resets active_file to 1
//   and closes slot 0; true instead toggles/derives active_file from which memory_files slot(s)
//   currently have a nonzero size, then closes whichever slot ends up *not* active. Named
//   "smart_toggle" for that behavior, not from any recovered caller-side evidence.
// Phase-4 review (disassembly appended below): the slot-in-use tests read the memory files'
// data pointers (+0x5b0 / +0x5c0), not their sizes as the draft had it; otherwise confirmed.
// AL = crosslap flag from sound_channel_fill_pcm_data.

#include "vorbisfile.h"
#include "tags.h"
#include "memory.h"
#include "sound.h"


#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
static void sound_stream_decoder_clear_ogg_vorbis_file(void *vorbis_file)
{
    uint32_t *words = (uint32_t *)vorbis_file;
    int32_t i;

    ov_clear(vorbis_file);
    for (i = 0; i < (int32_t)(k_ogg_vorbis_file_size / 4); i++) {
        words[i] = 0;
    }
}

// blam-cc: ESI -> decoder, AL -> smart_toggle
// Resets a sound_stream_decoder's decode position, then closes and zeroes one of its two
// double-buffered Ogg Vorbis slots, choosing which one per `smart_toggle` (see file header).
void sound_stream_decoder_close_slot(sound_stream_decoder *decoder, uint8_t smart_toggle)
{
    decoder->decoded_bytes = 0;
    decoder->position = 0;

    if (smart_toggle == 0) {
        decoder->active_file = 1;
        if (decoder->memory_files[1].data != 0) {
            sound_stream_decoder_clear_ogg_vorbis_file(decoder->ogg_vorbis_file[1]);
            decoder->memory_files[1].position = 0;
            decoder->memory_files[1].data = (void *)0;
            decoder->memory_files[1].size = 0;
            decoder->memory_files[1].end_of_file = 0;
        }
    } else {
        if (decoder->memory_files[0].data == 0) {
            if (decoder->memory_files[1].data == 0) {
                decoder->active_file = decoder->active_file == 0;
            } else {
                decoder->active_file = 1;
            }
        } else if (decoder->memory_files[1].data != 0) {
            decoder->active_file = decoder->active_file == 0;
        } else {
            decoder->active_file = 0;
        }

        if (decoder->active_file == 0) {
            sound_stream_decoder_clear_ogg_vorbis_file(decoder->ogg_vorbis_file[1]);
            decoder->memory_files[1].position = 0;
            decoder->memory_files[1].data = (void *)0;
            decoder->memory_files[1].size = 0;
            decoder->memory_files[1].end_of_file = 0;
            decoder->open = 0;
            return;
        }
    }

    sound_stream_decoder_clear_ogg_vorbis_file(decoder->ogg_vorbis_file[0]);
    decoder->memory_files[0].position = 0;
    decoder->memory_files[0].data = (void *)0;
    decoder->memory_files[0].size = 0;
    decoder->memory_files[0].end_of_file = 0;
    decoder->open = 0;
}

#if 0
Original Ghidra decompilation (0x545760):

void FUN_00545760(void)

{
  char in_AL;
  int iVar1;
  undefined4 *unaff_ESI;
  undefined4 *puVar2;

  unaff_ESI[1] = 0;
  *unaff_ESI = 0;
  if (in_AL == '\0') {
    *(undefined1 *)((int)unaff_ESI + 0x5a9) = 1;
    if (unaff_ESI[0x170] != 0) {
      puVar2 = unaff_ESI + 0xb6;
      if (puVar2 != (undefined4 *)0x0) {
        ov_clear(puVar2);
      }
      for (iVar1 = 0xb4; iVar1 != 0; iVar1 = iVar1 + -1) {
        *puVar2 = 0;
        puVar2 = puVar2 + 1;
      }
      unaff_ESI[0x16f] = 0;
      unaff_ESI[0x170] = 0;
      unaff_ESI[0x171] = 0;
      unaff_ESI[0x172] = 0;
    }
    goto LAB_00545827;
  }
  if (unaff_ESI[0x16c] == 0) {
    if (unaff_ESI[0x170] == 0) {
LAB_0054579c:
      *(bool *)((int)unaff_ESI + 0x5a9) = *(char *)((int)unaff_ESI + 0x5a9) == '\0';
    }
    else {
      *(undefined1 *)((int)unaff_ESI + 0x5a9) = 1;
    }
  }
  else {
    if (unaff_ESI[0x170] != 0) goto LAB_0054579c;
    *(undefined1 *)((int)unaff_ESI + 0x5a9) = 0;
  }
  if (*(char *)((int)unaff_ESI + 0x5a9) == '\0') {
    puVar2 = unaff_ESI + 0xb6;
    if (puVar2 != (undefined4 *)0x0) {
      ov_clear(puVar2);
    }
    for (iVar1 = 0xb4; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    unaff_ESI[0x16f] = 0;
    unaff_ESI[0x170] = 0;
    unaff_ESI[0x171] = 0;
    unaff_ESI[0x172] = 0;
    *(undefined1 *)(unaff_ESI + 0x16a) = 0;
    return;
  }
LAB_00545827:
  puVar2 = unaff_ESI + 2;
  if (puVar2 != (undefined4 *)0x0) {
    ov_clear(puVar2);
  }
  for (iVar1 = 0xb4; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  unaff_ESI[0x16b] = 0;
  unaff_ESI[0x16c] = 0;
  unaff_ESI[0x16d] = 0;
  unaff_ESI[0x16e] = 0;
  *(undefined1 *)(unaff_ESI + 0x16a) = 0;
  return;
}

Disassembly (0x545760..0x54585c, capstone; phase-4 review):

0x545760: push ebx
0x545761: xor ebx, ebx
0x545763: cmp al, bl
0x545765: push edi
0x545766: mov dword ptr [esi + 4], ebx
0x545769: mov dword ptr [esi], ebx
0x54576b: je 0x5457e9
0x54576d: mov eax, dword ptr [esi + 0x5b0]
0x545773: cmp eax, ebx
0x545775: jne 0x54578c
0x545777: cmp dword ptr [esi + 0x5c0], ebx
0x54577d: je 0x545788
0x54577f: mov byte ptr [esi + 0x5a9], 1
0x545786: jmp 0x5457ab
0x545788: cmp eax, ebx
0x54578a: je 0x54579c
0x54578c: cmp dword ptr [esi + 0x5c0], ebx
0x545792: jne 0x54579c
0x545794: mov byte ptr [esi + 0x5a9], bl
0x54579a: jmp 0x5457ab
0x54579c: cmp byte ptr [esi + 0x5a9], bl
0x5457a2: sete al
0x5457a5: mov byte ptr [esi + 0x5a9], al
0x5457ab: cmp byte ptr [esi + 0x5a9], bl
0x5457b1: jne 0x545827
0x5457b3: lea edi, [esi + 0x2d8]
0x5457b9: cmp edi, ebx
0x5457bb: je 0x5457c6
0x5457bd: push edi
0x5457be: call 0x614510
0x5457c3: add esp, 4
0x5457c6: xor eax, eax
0x5457c8: mov ecx, 0xb4
0x5457cd: rep stosd dword ptr es:[edi], eax
0x5457cf: lea ecx, [esi + 0x5bc]
0x5457d5: mov dword ptr [ecx], eax
0x5457d7: mov dword ptr [ecx + 4], eax
0x5457da: mov dword ptr [ecx + 8], eax
0x5457dd: mov dword ptr [ecx + 0xc], eax
0x5457e0: pop edi
0x5457e1: mov byte ptr [esi + 0x5a8], bl
0x5457e7: pop ebx
0x5457e8: ret 
0x5457e9: cmp dword ptr [esi + 0x5c0], ebx
0x5457ef: mov byte ptr [esi + 0x5a9], 1
0x5457f6: je 0x545827
0x5457f8: lea edi, [esi + 0x2d8]
0x5457fe: cmp edi, ebx
0x545800: je 0x54580b
0x545802: push edi
0x545803: call 0x614510
0x545808: add esp, 4
0x54580b: xor eax, eax
0x54580d: mov ecx, 0xb4
0x545812: rep stosd dword ptr es:[edi], eax
0x545814: xor edx, edx
0x545816: lea eax, [esi + 0x5bc]
0x54581c: mov dword ptr [eax], edx
0x54581e: mov dword ptr [eax + 4], edx
0x545821: mov dword ptr [eax + 8], edx
0x545824: mov dword ptr [eax + 0xc], edx
0x545827: lea edi, [esi + 8]
0x54582a: cmp edi, ebx
0x54582c: je 0x545837
0x54582e: push edi
0x54582f: call 0x614510
0x545834: add esp, 4
0x545837: xor eax, eax
0x545839: mov ecx, 0xb4
0x54583e: rep stosd dword ptr es:[edi], eax
0x545840: xor ecx, ecx
0x545842: lea edx, [esi + 0x5ac]
0x545848: mov dword ptr [edx], ecx
0x54584a: mov dword ptr [edx + 4], ecx
0x54584d: mov dword ptr [edx + 8], ecx
0x545850: mov dword ptr [edx + 0xc], ecx
0x545853: pop edi
0x545854: mov byte ptr [esi + 0x5a8], bl
0x54585a: pop ebx
0x54585b: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
