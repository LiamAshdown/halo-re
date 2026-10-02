// sound_build_channel_candidates  (Ghidra: FUN_0054c1d0, still unnamed there)
// address 0x54c1d0, size 279 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md's summary ("class/tag") is refined here: BOTH output
// lists require the same Sound tag (definition_index equality) -- the discriminator is that the
// second list additionally requires the same owner. sound_class_definition
// maximum_sounds_per_tag/maximum_sounds_per_object (0x0069eae0+0/+2, types/sound.h) are copied
// into the output as the two lists' limits, which matches this reading (a per-tag budget for
// "same tag anywhere" and a per-object budget for "same tag on the same owner").
// register convention: ESI -> out (candidate list), stack -> sound_handle.
// sound_channel_candidate_list is in types/sound.h (folded back from the local copies).
// Phase-4 review (disassembly appended below): the type test is
//   sound_channel_type_flags_match(Sound.format, Sound.channel_count, Sound.sample_rate,
//   DX = sound_channels[i].type_flags, candidate location.type)
// i.e. "could this logical channel play the candidate at all"; the earlier draft passed the
// sound's listener_index where the binary reads Sound+0x06 (sample_rate) and dropped the DX
// argument, the other channel's type flags.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *sound_data;      // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances; // 0x0087bc14
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0
extern int16_t sound_channel_count; // 0x007252b4
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60

extern uint8_t sound_channel_type_flags_match(int16_t compressed_requested, int16_t stereo_requested,
    uint16_t sample_rate_44khz_requested, uint16_t flags, int16_t requested_3d); // 0x548520, blam-cc: stack x3, DX, stack

// blam-cc: ESI -> out, stack -> sound_handle
// Scans every logical playback channel for ones playing the exact same sound tag as
// `sound_handle` (recording up to 16 in out->tag_matches), and among those, ones additionally
// owned by the same owner (recording up to 16 in out->owner_matches). Also copies the class's
// per-tag/per-object channel limits into the output.
void sound_build_channel_candidates(sound_channel_candidate_list *out, datum_index sound_handle)
{
    sound *candidate;
    Sound *definition;
    int16_t channel_index;
    datum_index other_handle;
    sound *other;

    candidate = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    definition = (Sound *)tag_instances[candidate->definition_index & 0xffff].data;

    out->tag_match_count = 0;
    out->owner_match_count = 0;
    out->tag_match_limit = sound_class_definitions[definition->sound_class].maximum_sounds_per_tag;
    out->owner_match_limit = sound_class_definitions[definition->sound_class].maximum_sounds_per_object;

    for (channel_index = 0; channel_index < sound_channel_count; channel_index++) {
        other_handle = sound_channels[channel_index].sound_index;
        if (other_handle != 0xffffffff && other_handle != sound_handle) {
            other = (sound *)((uint8_t *)sound_data->data + (other_handle & 0xffff) * sizeof(sound));
            if (sound_channel_type_flags_match(definition->format, definition->channel_count, definition->sample_rate,
                    sound_channels[channel_index].type_flags, candidate->location.type) &&
                candidate->definition_index == other->definition_index) {
                out->tag_matches[out->tag_match_count] = channel_index;
                out->tag_match_count += 1;
                if (candidate->owner_index != 0xffffffff && candidate->owner_index == other->owner_index) {
                    out->owner_matches[out->owner_match_count] = channel_index;
                    out->owner_match_count += 1;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x54c1d0):

void FUN_0054c1d0(uint param_1)

{
  uint uVar1;
  char cVar2;
  int iVar3;
  short sVar4;
  short *unaff_ESI;
  int iVar5;
  int iVar6;

  iVar5 = (param_1 & 0xffff) * 0xb0;
  iVar6 = iVar5 + *(int *)(DAT_007252c0 + 0x34);
  iVar5 = *(int *)((*(uint *)(iVar5 + 8 + *(int *)(DAT_007252c0 + 0x34)) & 0xffff) * 0x20 + 0x14 +
                  DAT_0087bc14);
  sVar4 = 0;
  *unaff_ESI = 0;
  unaff_ESI[0x12] = 0;
  unaff_ESI[0x11] = *(short *)(&DAT_0069eae0 + *(short *)(iVar5 + 4) * 0x2c);
  unaff_ESI[0x23] = *(short *)(&DAT_0069eae2 + *(short *)(iVar5 + 4) * 0x2c);
  if (0 < DAT_007252b4) {
    do {
      uVar1 = (&DAT_00724a60)[sVar4 * 6];
      if ((uVar1 != 0xffffffff) && (uVar1 != param_1)) {
        iVar3 = (uVar1 & 0xffff) * 0xb0 + *(int *)(DAT_007252c0 + 0x34);
        cVar2 = FUN_00548520(*(undefined2 *)(iVar5 + 0x6e),*(undefined2 *)(iVar5 + 0x6c),
                             *(undefined2 *)(iVar5 + 6),*(undefined2 *)(iVar6 + 0x14));
        if ((cVar2 != '\0') && (*(int *)(iVar6 + 8) == *(int *)(iVar3 + 8))) {
          unaff_ESI[*unaff_ESI + 1] = sVar4;
          *unaff_ESI = *unaff_ESI + 1;
          if ((*(int *)(iVar6 + 0xc) != -1) && (*(int *)(iVar6 + 0xc) == *(int *)(iVar3 + 0xc))) {
            unaff_ESI[unaff_ESI[0x12] + 0x13] = sVar4;
            unaff_ESI[0x12] = unaff_ESI[0x12] + 1;
          }
        }
      }
      sVar4 = sVar4 + 1;
    } while (sVar4 < DAT_007252b4);
  }
  return;
}

Disassembly (0x54c1d0..0x54c2e7, capstone; phase-4 review):

0x54c1d0: push ecx
0x54c1d1: mov eax, dword ptr [0x7252c0]
0x54c1d6: mov edx, dword ptr [0x87bc14]
0x54c1dc: push ebx
0x54c1dd: push ebp
0x54c1de: mov ebp, dword ptr [eax + 0x34]
0x54c1e1: push edi
0x54c1e2: mov edi, dword ptr [esp + 0x14]
0x54c1e6: and edi, 0xffff
0x54c1ec: imul edi, edi, 0xb0
0x54c1f2: mov ecx, dword ptr [edi + ebp + 8]
0x54c1f6: add edi, ebp
0x54c1f8: and ecx, 0xffff
0x54c1fe: shl ecx, 5
0x54c201: mov ebx, dword ptr [ecx + edx + 0x14]
0x54c205: xor ebp, ebp
0x54c207: mov word ptr [esi], bp
0x54c20a: mov word ptr [esi + 0x24], bp
0x54c20e: movsx eax, word ptr [ebx + 4]
0x54c212: imul eax, eax, 0x2c
0x54c215: mov cx, word ptr [eax + 0x69eae0]
0x54c21c: mov word ptr [esi + 0x22], cx
0x54c220: movsx edx, word ptr [ebx + 4]
0x54c224: imul edx, edx, 0x2c
0x54c227: mov ax, word ptr [edx + 0x69eae2]
0x54c22e: mov word ptr [esi + 0x46], ax
0x54c232: cmp word ptr [0x7252b4], bp
0x54c239: jle 0x54c2e2
0x54c23f: nop 
0x54c240: movsx eax, bp
0x54c243: lea ecx, [eax + eax*2]
0x54c246: mov eax, dword ptr [ecx*8 + 0x724a60]
0x54c24d: cmp eax, -1
0x54c250: lea ecx, [ecx*8 + 0x724a60]
0x54c257: je 0x54c2d4
0x54c259: cmp eax, dword ptr [esp + 0x14]
0x54c25d: je 0x54c2d4
0x54c25f: mov edx, dword ptr [0x7252c0]
0x54c265: and eax, 0xffff
0x54c26a: imul eax, eax, 0xb0
0x54c270: add eax, dword ptr [edx + 0x34]
0x54c273: xor edx, edx
0x54c275: mov dx, word ptr [ebx + 6]
0x54c279: mov dword ptr [esp + 0xc], eax
0x54c27d: xor eax, eax
0x54c27f: mov ax, word ptr [edi + 0x14]
0x54c283: push eax
0x54c284: push edx
0x54c285: xor eax, eax
0x54c287: mov ax, word ptr [ebx + 0x6c]
0x54c28b: xor edx, edx
0x54c28d: mov dx, word ptr [ebx + 0x6e]
0x54c291: push eax
0x54c292: push edx
0x54c293: mov dx, word ptr [ecx + 4]
0x54c297: call 0x548520
0x54c29c: add esp, 0x10
0x54c29f: test al, al
0x54c2a1: je 0x54c2d4
0x54c2a3: mov ecx, dword ptr [esp + 0xc]
0x54c2a7: mov eax, dword ptr [edi + 8]
0x54c2aa: cmp eax, dword ptr [ecx + 8]
0x54c2ad: jne 0x54c2d4
0x54c2af: movsx edx, word ptr [esi]
0x54c2b2: mov word ptr [esi + edx*2 + 2], bp
0x54c2b7: inc word ptr [esi]
0x54c2ba: mov eax, dword ptr [edi + 0xc]
0x54c2bd: cmp eax, -1
0x54c2c0: je 0x54c2d4
0x54c2c2: cmp eax, dword ptr [ecx + 0xc]
0x54c2c5: jne 0x54c2d4
0x54c2c7: movsx eax, word ptr [esi + 0x24]
0x54c2cb: mov word ptr [esi + eax*2 + 0x26], bp
0x54c2d0: inc word ptr [esi + 0x24]
0x54c2d4: inc ebp
0x54c2d5: cmp bp, word ptr [0x7252b4]
0x54c2dc: jl 0x54c240
0x54c2e2: pop edi
0x54c2e3: pop ebp
0x54c2e4: pop ebx
0x54c2e5: pop ecx
0x54c2e6: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
