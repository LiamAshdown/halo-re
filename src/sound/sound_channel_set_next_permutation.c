// sound_channel_set_next_permutation  (Ghidra: FUN_0054cd30, still unnamed there)
// address 0x54cd30, size 107 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Releases a playback channel's cached detail-sound
// reference and notifies the sound driver."; sound_channel.next_permutation/current_permutation/
// play_time (types/sound.h, 0x14/0x10/0x08) match.
// register convention: CX -> channel_index, EDI -> permutation, stack -> (first_person,
// sound_class, streaming).
// Phase-4 review: confirmed by the disassembly in the #if 0 block: the three stack words are
// forwarded unchanged, after (ECX, EDI), to sound_driver.channel_play (vtable +0x18). The only
// difference kept on purpose: the binary decrements the lock of entries[samples_pointer & 0xffff]
// without testing samples_pointer for -1 (it tests the entry address for NULL).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"

extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60
extern data_array *sound_cache_entries; // 0x006ac528
extern sound_driver *current_sound_driver; // 0x00725208, header calls this "sound_driver"

// blam-cc: CX -> channel_index, EDI -> permutation, stack -> (unknown, sound_class, streaming)
void sound_channel_set_next_permutation(int16_t channel_index, SoundPermutation *permutation,
    int16_t unknown, int16_t sound_class, uint8_t streaming)
{
    sound_channel *channel;

    channel = &sound_channels[channel_index];

    if (channel->next_permutation != 0) {
        if (channel->next_permutation->samples_pointer != 0xffffffff) {
            sound_cache_entry *entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data +
                (channel->next_permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
            entry->lock_count -= 1;
        }
    }

    current_sound_driver->channel_play(channel_index, permutation, unknown, sound_class, streaming);

    if (channel->current_permutation != 0) {
        channel->next_permutation = permutation;
    } else {
        channel->current_permutation = permutation;
        channel->play_time = 0.0f;
    }
}

#if 0
Original Ghidra decompilation (0x54cd30):

void FUN_0054cd30(void)

{
  char *pcVar1;
  int iVar2;
  int iVar3;
  short in_CX;
  undefined4 unaff_EDI;

  iVar2 = (int)in_CX;
  if ((&DAT_00724a74)[iVar2 * 6] != 0) {
    iVar3 = (*(uint *)((&DAT_00724a74)[iVar2 * 6] + 0x2c) & 0xffff) * 0x10 +
            *(int *)(DAT_006ac528 + 0x34);
    if (iVar3 != 0) {
      pcVar1 = (char *)(iVar3 + 5);
      *pcVar1 = *pcVar1 + -1;
    }
  }
  (**(code **)(DAT_00725208 + 0x18))();
  if ((&DAT_00724a70)[iVar2 * 6] != 0) {
    (&DAT_00724a74)[iVar2 * 6] = unaff_EDI;
    return;
  }
  (&DAT_00724a70)[iVar2 * 6] = unaff_EDI;
  (&DAT_00724a68)[iVar2 * 6] = 0;
  return;
}

Disassembly (0x54cd30..0x54cd9b, capstone; phase-4 review):

0x54cd30: movsx eax, cx
0x54cd33: push esi
0x54cd34: lea esi, [eax + eax*2]
0x54cd37: mov eax, dword ptr [esi*8 + 0x724a74]
0x54cd3e: test eax, eax
0x54cd40: lea esi, [esi*8 + 0x724a60]
0x54cd47: je 0x54cd66
0x54cd49: mov eax, dword ptr [eax + 0x2c]
0x54cd4c: mov edx, dword ptr [0x6ac528]
0x54cd52: and eax, 0xffff
0x54cd57: push ebx
0x54cd58: mov ebx, dword ptr [edx + 0x34]
0x54cd5b: shl eax, 4
0x54cd5e: add eax, ebx
0x54cd60: pop ebx
0x54cd61: je 0x54cd66
0x54cd63: dec byte ptr [eax + 5]
0x54cd66: mov eax, dword ptr [esp + 0x10]
0x54cd6a: mov edx, dword ptr [esp + 0xc]
0x54cd6e: push eax
0x54cd6f: mov eax, dword ptr [esp + 0xc]
0x54cd73: push edx
0x54cd74: push eax
0x54cd75: push edi
0x54cd76: push ecx
0x54cd77: mov ecx, dword ptr [0x725208]
0x54cd7d: call dword ptr [ecx + 0x18]
0x54cd80: mov eax, dword ptr [esi + 0x10]
0x54cd83: add esp, 0x14
0x54cd86: test eax, eax
0x54cd88: je 0x54cd8f
0x54cd8a: mov dword ptr [esi + 0x14], edi
0x54cd8d: pop esi
0x54cd8e: ret 
0x54cd8f: mov dword ptr [esi + 0x10], edi
0x54cd92: mov dword ptr [esi + 8], 0
0x54cd99: pop esi
0x54cd9a: ret 
#endif
