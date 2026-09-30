// sound_instance_apply_pending_definition_switch  (Ghidra: FUN_0054ddc0, still unnamed there)
// address 0x54ddc0, size 239 bytes
// name confidence: 0.45   rewrite confidence: 0.9
// evidence: out/phase4/sound_functions.md "Reassigns a pitch-range channel's predicted-resource
// target object, freeing a conflicting channel for non-music sound classes."; applies
// sound.pending_definition_index (queued by sound_instance_queue_definition_switch.c, 0x54dd90)
// and re-picks a pitch range/permutation for the new definition, then enforces the same
// per-tag/per-owner channel budgets as sound_pick_channel_for_instance.c (0x54c2f0).
// register convention: EBX -> sound_handle.
// Phase-4 review (disassembly appended below): confirmed as written: pick_for_pitch(AX = the
// old pitch range, ECX = new tag, pitch) and pick_random(AX = new range, CX = -1, new tag).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#include "fn_sound.h"

extern data_array *sound_data;      // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances; // 0x0087bc14
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60


extern void sound_instance_stop(datum_index sound_handle); // this module, 0x54b180

// blam-cc: EBX -> sound_handle
// Applies a queued definition switch (sound.pending_definition_index): re-resolves the sound's
// pitch range and permutation against the new tag. If it already has a channel, re-checks the
// per-tag/per-owner channel budgets under the new tag and stops either a conflicting channel or,
// failing that, this sound itself.
void sound_instance_apply_pending_definition_switch(datum_index sound_handle)
{
    sound *instance;
    Sound *definition;
    sound_channel_candidate_list candidates;
    int16_t *list;
    int16_t count;
    int16_t channel_index;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    definition = (Sound *)tag_instances[instance->pending_definition_index & 0xffff].data;

    instance->flags |= _sound_permutation_pending_bit;
    instance->definition_index = instance->pending_definition_index;
    instance->pending_definition_index = 0xffffffff;
    instance->pitch_range_index = sound_permutation_pick_for_pitch(instance->pitch_range_index, definition,
        instance->pitch);
    instance->permutation_index = sound_permutation_pick_random(instance->pitch_range_index, -1, definition);

    if (instance->channel_index != -1) {
        sound_build_channel_candidates(&candidates, sound_handle);

        if (candidates.owner_match_count < candidates.owner_match_limit) {
            if (candidates.tag_match_count < candidates.tag_match_limit) {
                return;
            }
            list = candidates.tag_matches;
            count = candidates.tag_match_count;
        } else {
            list = candidates.owner_matches;
            count = candidates.owner_match_count;
        }

        channel_index = sound_pick_replaceable_channel(sound_handle, list, count);
        if (channel_index != -1) {
            sound_instance_stop(sound_channels[channel_index].sound_index);
            return;
        }
        sound_instance_stop(sound_handle);
    }
}

#if 0
Original Ghidra decompilation (0x54ddc0):

void FUN_0054ddc0(void)

{
  uint uVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  undefined2 uVar4;
  short sVar5;
  uint unaff_EBX;
  int iVar6;
  undefined4 local_48 [8];
  short local_26;
  undefined4 local_24 [8];
  short local_2;

  iVar6 = (unaff_EBX & 0xffff) * 0xb0;
  uVar1 = *(uint *)(iVar6 + 0x98 + *(int *)(DAT_007252c0 + 0x34));
  iVar6 = iVar6 + *(int *)(DAT_007252c0 + 0x34);
  uVar2 = *(undefined4 *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  *(byte *)(iVar6 + 4) = *(byte *)(iVar6 + 4) | 8;
  *(uint *)(iVar6 + 8) = uVar1;
  *(undefined4 *)(iVar6 + 0x98) = 0xffffffff;
  uVar4 = sound_permutation_pick_for_pitch(*(undefined4 *)(iVar6 + 0x88));
  *(undefined2 *)(iVar6 + 0x8e) = uVar4;
  uVar4 = sound_permutation_pick_random(uVar2);
  *(undefined2 *)(iVar6 + 0x90) = uVar4;
  if (*(short *)(iVar6 + 0x8c) != -1) {
    FUN_0054c1d0();
    if ((short)local_24[0] < local_2) {
      if ((short)local_48[0] < local_26) {
        return;
      }
      puVar3 = local_48;
      local_24[0] = local_48[0];
    }
    else {
      puVar3 = local_24;
    }
    sVar5 = FUN_0054c5e0((int)puVar3 + 2,local_24[0]);
    if (sVar5 != -1) {
      sound_instance_stop((&DAT_00724a60)[sVar5 * 6]);
      return;
    }
    sound_instance_stop();
  }
  return;
}

Disassembly (0x54ddc0..0x54deaf, capstone; phase-4 review):

0x54ddc0: mov eax, dword ptr [0x7252c0]
0x54ddc5: mov ecx, dword ptr [eax + 0x34]
0x54ddc8: mov edx, dword ptr [0x87bc14]
0x54ddce: sub esp, 0x48
0x54ddd1: push esi
0x54ddd2: mov esi, ebx
0x54ddd4: and esi, 0xffff
0x54ddda: imul esi, esi, 0xb0
0x54dde0: mov eax, dword ptr [esi + ecx + 0x98]
0x54dde7: add esi, ecx
0x54dde9: mov ecx, eax
0x54ddeb: and ecx, 0xffff
0x54ddf1: shl ecx, 5
0x54ddf4: push edi
0x54ddf5: mov edi, dword ptr [ecx + edx + 0x14]
0x54ddf9: or byte ptr [esi + 4], 8
0x54ddfd: mov dword ptr [esi + 8], eax
0x54de00: mov eax, dword ptr [esi + 0x88]
0x54de06: push eax
0x54de07: xor eax, eax
0x54de09: mov ax, word ptr [esi + 0x8e]
0x54de10: mov ecx, edi
0x54de12: mov dword ptr [esi + 0x98], 0xffffffff
0x54de1c: call 0x5454a0
0x54de21: push edi
0x54de22: or ecx, 0xffffffff
0x54de25: mov word ptr [esi + 0x8e], ax
0x54de2c: call 0x545590
0x54de31: add esp, 8
0x54de34: cmp word ptr [esi + 0x8c], -1
0x54de3c: mov word ptr [esi + 0x90], ax
0x54de43: je 0x54dea9
0x54de45: push ebx
0x54de46: lea esi, [esp + 0xc]
0x54de4a: call 0x54c1d0
0x54de4f: mov eax, dword ptr [esp + 0x30]
0x54de53: add esp, 4
0x54de56: cmp ax, word ptr [esp + 0x4e]
0x54de5b: jl 0x54de63
0x54de5d: lea ecx, [esp + 0x2e]
0x54de61: jmp 0x54de72
0x54de63: mov eax, dword ptr [esp + 8]
0x54de67: cmp ax, word ptr [esp + 0x2a]
0x54de6c: jl 0x54dea9
0x54de6e: lea ecx, [esp + 0xa]
0x54de72: push eax
0x54de73: push ecx
0x54de74: mov eax, ebx
0x54de76: call 0x54c5e0
0x54de7b: add esp, 8
0x54de7e: cmp ax, 0xffff
0x54de82: je 0x54dea0
0x54de84: movsx eax, ax
0x54de87: lea edx, [eax + eax*2]
0x54de8a: mov eax, dword ptr [edx*8 + 0x724a60]
0x54de91: push eax
0x54de92: call 0x54b180
0x54de97: add esp, 4
0x54de9a: pop edi
0x54de9b: pop esi
0x54de9c: add esp, 0x48
0x54de9f: ret 
0x54dea0: push ebx
0x54dea1: call 0x54b180
0x54dea6: add esp, 4
0x54dea9: pop edi
0x54deaa: pop esi
0x54deab: add esp, 0x48
0x54deae: ret 
#endif
