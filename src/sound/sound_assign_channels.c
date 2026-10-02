// sound_assign_channels  (Ghidra: FUN_0054c020, still unnamed there)
// address 0x54c020, size 426 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Per-tick pass that assigns newly-due sound instances
// to pitch-range playback channels, stealing or releasing channels as needed."; matches
// sound_class_definition.discard_on_cache_miss (0x0069eae0+0xc == 0x0069eaec, types/sound.h's
// own note: "0 makes 0x54c020 stop a sound whose samples are not resident at its start time")
// and SoundPitchRange.discarded_permutation_index (types/tags.h, 0x3a).
// register convention: void, no parameters.
// Phase-4 review (disassembly appended below): sound_cache_touch is (1, 1, BL 0, EDI = the
// sound's chosen permutation); the draft passed NULL. The rest matched.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *sound_data;       // 0x007252c0, "sounds" 0x200 x 0xb0
extern int32_t sound_time;           // 0x0072520c
extern tag_instance *tag_instances;  // 0x0087bc14
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module
extern uint8_t sound_cache_touch(uint8_t allocate_if_missing, uint8_t lock, uint8_t wait_until_loaded,
    void *permutation); // 0x443e10, cache module, blam-cc: stack, stack, BL, EDI
extern void sound_instance_stop(datum_index sound_handle); // this module, 0x54b180
extern int16_t sound_pick_channel_for_instance(datum_index sound_handle); // this module, 0x54c2f0
extern uint8_t sound_looping_detail_location_proc(datum_index owner, void *callback_data, sound_location *location); // this module, 0x54dc70

// Per-update pass over every live sound whose start_time has arrived: if its permutation is not
// yet resident in the sound cache, either leaves it waiting (retried next pass) or, for classes
// whose discard_on_cache_miss is set, gives up and stops it (remembering the skipped permutation
// so it is not retried). Otherwise requests a playback channel for it, stealing whatever sound
// currently occupies the chosen channel.
void sound_assign_channels(void)
{
    datum_index sound_handle;
    sound *instance;
    Sound *definition;
    SoundPitchRange *pitch_range;
    int16_t target_channel;
    datum_index occupant;

    sound_handle = datum_next(-1, sound_data);
    while (sound_handle != 0xffffffff) {
        instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));

        if (instance->start_time <= sound_time) {
            if (instance->channel_index == -1 &&
                !sound_cache_touch(1, 1, 0, (SoundPermutation *)((SoundPitchRange *)((Sound *)tag_instances[
                    instance->definition_index & 0xffff].data)->pitch_ranges.pointer + instance->pitch_range_index)->
                    permutations.pointer + instance->permutation_index)) {
                if (instance->channel_index == -1 && instance->location_proc != sound_looping_detail_location_proc) {
                    definition = (Sound *)tag_instances[instance->definition_index & 0xffff].data;
                    if (sound_class_definitions[definition->sound_class].discard_on_cache_miss == 0) {
                        pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
                        if (pitch_range->discarded_permutation_index == 0xffff) {
                            pitch_range->discarded_permutation_index = (uint16_t)instance->permutation_index;
                        }
                        sound_instance_stop(sound_handle);
                    }
                }
            } else {
                instance->flags |= _sound_channel_requested_bit;
                target_channel = sound_pick_channel_for_instance(sound_handle);
                if (target_channel == -1) {
                    sound_instance_stop(sound_handle);
                } else {
                    occupant = sound_channels[target_channel].sound_index;
                    if (occupant != sound_handle) {
                        if (occupant != 0xffffffff) {
                            sound_instance_stop(occupant);
                        }
                        sound_channels[target_channel].sound_index = sound_handle;
                        instance->start_time = sound_time;
                    }
                }
            }
        }

        sound_handle = datum_next((int16_t)sound_handle, sound_data);
    }
}

#if 0
Original Ghidra decompilation (0x54c020):

void FUN_0054c020(void)

{
  int iVar1;
  uint uVar2;
  char cVar3;
  short sVar4;
  uint uVar5;
  short *psVar6;
  int iVar7;
  int iVar8;

  iVar1 = DAT_007252c0;
  uVar5 = datum_next();
  do {
    if (uVar5 == 0xffffffff) {
      return;
    }
    iVar7 = (uVar5 & 0xffff) * 0xb0;
    iVar8 = iVar7 + *(int *)(iVar1 + 0x34);
    if (*(int *)(iVar7 + 0x84 + *(int *)(iVar1 + 0x34)) <= DAT_0072520c) {
      if ((*(short *)(iVar8 + 0x8c) == -1) && (cVar3 = sound_cache_touch(1,1), cVar3 == '\0')) {
        if (((*(short *)(iVar8 + 0x8c) == -1) && (*(code **)(iVar8 + 0x10) != FUN_0054dc70)) &&
           (iVar1 = *(int *)((*(uint *)(iVar8 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
           *(short *)(&DAT_0069eaec + *(short *)(iVar1 + 4) * 0x2c) == 0)) {
          iVar1 = *(int *)(iVar1 + 0x9c);
          if (*(short *)(iVar1 + 0x3a + *(short *)(iVar8 + 0x8e) * 0x48) == -1) {
            *(undefined2 *)(iVar1 + *(short *)(iVar8 + 0x8e) * 0x48 + 0x3a) =
                 *(undefined2 *)(iVar8 + 0x90);
          }
LAB_0054c122:
          sound_instance_stop(uVar5);
        }
      }
      else {
        *(byte *)(iVar8 + 4) = *(byte *)(iVar8 + 4) | 2;
        sVar4 = FUN_0054c2f0(uVar5);
        if (sVar4 == -1) goto LAB_0054c122;
        uVar2 = (&DAT_00724a60)[sVar4 * 6];
        if (uVar2 != uVar5) {
          if (uVar2 != 0xffffffff) {
            sound_instance_stop(uVar2);
          }
          iVar1 = DAT_0072520c;
          (&DAT_00724a60)[sVar4 * 6] = uVar5;
          *(int *)(iVar8 + 0x84) = iVar1;
        }
      }
    }
    iVar7 = uVar5 + 1;
    uVar5 = 0xffffffff;
    sVar4 = (short)iVar7;
    iVar1 = DAT_007252c0;
    if ((-1 < sVar4) && (sVar4 < *(short *)(DAT_007252c0 + 0x2e))) {
      psVar6 = (short *)((int)sVar4 * (int)*(short *)(DAT_007252c0 + 0x22) +
                        *(int *)(DAT_007252c0 + 0x34));
      do {
        if (*psVar6 != 0) {
          uVar5 = (int)*psVar6 << 0x10 | (int)(short)iVar7;
          break;
        }
        iVar7 = iVar7 + 1;
        psVar6 = (short *)((int)psVar6 + (int)*(short *)(DAT_007252c0 + 0x22));
      } while ((short)iVar7 < *(short *)(DAT_007252c0 + 0x2e));
    }
  } while( true );
}

Disassembly (0x54c020..0x54c1ca, capstone; phase-4 review):

0x54c020: push ebx
0x54c021: mov ebx, dword ptr [0x7252c0]
0x54c027: push ebp
0x54c028: push edi
0x54c029: or edx, 0xffffffff
0x54c02c: mov edi, ebx
0x54c02e: call 0x4d0630
0x54c033: mov ebp, eax
0x54c035: cmp ebp, -1
0x54c038: je 0x54c1c6
0x54c03e: push esi
0x54c03f: nop 
0x54c040: mov edx, dword ptr [ebx + 0x34]
0x54c043: mov ecx, dword ptr [0x72520c]
0x54c049: mov esi, ebp
0x54c04b: and esi, 0xffff
0x54c051: imul esi, esi, 0xb0
0x54c057: mov eax, dword ptr [esi + edx + 0x84]
0x54c05e: add esi, edx
0x54c060: cmp eax, ecx
0x54c062: jg 0x54c12b
0x54c068: cmp word ptr [esi + 0x8c], -1
0x54c070: jne 0x54c164
0x54c076: movsx edi, word ptr [esi + 0x90]
0x54c07d: mov ecx, dword ptr [esi + 8]
0x54c080: imul edi, edi, 0x7c
0x54c083: movsx eax, word ptr [esi + 0x8e]
0x54c08a: mov edx, dword ptr [0x87bc14]
0x54c090: and ecx, 0xffff
0x54c096: shl ecx, 5
0x54c099: mov ecx, dword ptr [ecx + edx + 0x14]
0x54c09d: lea edx, [eax + eax*8]
0x54c0a0: mov eax, dword ptr [ecx + 0x9c]
0x54c0a6: mov ebx, dword ptr [eax + edx*8 + 0x40]
0x54c0aa: push 1
0x54c0ac: add edi, ebx
0x54c0ae: push 1
0x54c0b0: xor bl, bl
0x54c0b2: call 0x443e10
0x54c0b7: add esp, 8
0x54c0ba: test al, al
0x54c0bc: jne 0x54c164
0x54c0c2: or edx, 0xffffffff
0x54c0c5: cmp word ptr [esi + 0x8c], dx
0x54c0cc: jne 0x54c12b
0x54c0ce: cmp dword ptr [esi + 0x10], 0x54dc70
0x54c0d5: je 0x54c12b
0x54c0d7: mov ecx, dword ptr [esi + 8]
0x54c0da: mov eax, dword ptr [0x87bc14]
0x54c0df: and ecx, 0xffff
0x54c0e5: shl ecx, 5
0x54c0e8: mov ecx, dword ptr [ecx + eax + 0x14]
0x54c0ec: movsx eax, word ptr [ecx + 4]
0x54c0f0: imul eax, eax, 0x2c
0x54c0f3: cmp word ptr [eax + 0x69eaec], 0
0x54c0fb: jne 0x54c12b
0x54c0fd: movsx eax, word ptr [esi + 0x8e]
0x54c104: mov ecx, dword ptr [ecx + 0x9c]
0x54c10a: lea eax, [eax + eax*8]
0x54c10d: cmp word ptr [ecx + eax*8 + 0x3a], dx
0x54c112: lea eax, [ecx + eax*8]
0x54c115: jne 0x54c122
0x54c117: mov dx, word ptr [esi + 0x90]
0x54c11e: mov word ptr [eax + 0x3a], dx
0x54c122: push ebp
0x54c123: call 0x54b180
0x54c128: add esp, 4
0x54c12b: mov ebx, dword ptr [0x7252c0]
0x54c131: lea ecx, [ebp + 1]
0x54c134: or esi, 0xffffffff
0x54c137: test cx, cx
0x54c13a: jl 0x54c1ba
0x54c13c: mov di, word ptr [ebx + 0x2e]
0x54c140: cmp cx, di
0x54c143: jge 0x54c1ba
0x54c145: movsx edx, word ptr [ebx + 0x22]
0x54c149: mov ebp, dword ptr [ebx + 0x34]
0x54c14c: movsx eax, cx
0x54c14f: imul eax, edx
0x54c152: add eax, ebp
0x54c154: cmp word ptr [eax], 0
0x54c158: jne 0x54c1af
0x54c15a: inc ecx
0x54c15b: add eax, edx
0x54c15d: cmp cx, di
0x54c160: jl 0x54c154
0x54c162: jmp 0x54c1ba
0x54c164: or byte ptr [esi + 4], 2
0x54c168: push ebp
0x54c169: call 0x54c2f0
0x54c16e: add esp, 4
0x54c171: cmp ax, 0xffff
0x54c175: je 0x54c122
0x54c177: movsx eax, ax
0x54c17a: lea edi, [eax + eax*2]
0x54c17d: mov eax, dword ptr [edi*8 + 0x724a60]
0x54c184: cmp eax, ebp
0x54c186: lea edi, [edi*8 + 0x724a60]
0x54c18d: je 0x54c12b
0x54c18f: cmp eax, -1
0x54c192: je 0x54c19d
0x54c194: push eax
0x54c195: call 0x54b180
0x54c19a: add esp, 4
0x54c19d: mov eax, dword ptr [0x72520c]
0x54c1a2: mov dword ptr [edi], ebp
0x54c1a4: mov dword ptr [esi + 0x84], eax
0x54c1aa: jmp 0x54c12b
0x54c1af: movsx esi, word ptr [eax]
0x54c1b2: movsx ecx, cx
0x54c1b5: shl esi, 0x10
0x54c1b8: or esi, ecx
0x54c1ba: cmp esi, -1
0x54c1bd: mov ebp, esi
0x54c1bf: jne 0x54c040
0x54c1c5: pop esi
0x54c1c6: pop edi
0x54c1c7: pop ebp
0x54c1c8: pop ebx
0x54c1c9: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
