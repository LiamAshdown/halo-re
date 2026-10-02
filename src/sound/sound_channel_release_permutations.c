// sound_channel_release_permutations  (Ghidra: FUN_0054d0d0, still unnamed there)
// address 0x54d0d0, size 106 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Releases a channel's cached detail-sound references
// and stops its driver voice."; disassembly (scratchpad/disasm/disasm.py) resolves Ghidra's odd
// `iVar2 * 0x18 != -0x724a60` guard as a plain `&sound_channels[channel_index] != NULL` pointer
// check, which is unreachable in practice (the address can never actually be zero for any
// representable channel_index) -- preserved literally rather than dropped. driver->channel_stop
// is vtable slot 0x20 (types/sound.h sound_driver.channel_stop), an exact field match.
// register convention: DI -> channel_index (unaff_DI).
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60
extern data_array *sound_cache_entries; // 0x006ac528
extern sound_driver *current_sound_driver; // 0x00725208, header calls this "sound_driver"

// blam-cc: DI -> channel_index
// Releases the cache page references of a channel's next and current permutations (if any) and
// stops its driver voice.
void sound_channel_release_permutations(int16_t channel_index)
{
    sound_channel *channel;
    sound_cache_entry *entry;

    channel = &sound_channels[channel_index];
    if (channel != 0) { // unreachable in practice, see file header
        if (channel->next_permutation != 0) {
            if (channel->next_permutation->samples_pointer != 0xffffffff) {
                entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data +
                    (channel->next_permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                entry->lock_count -= 1;
            }
            channel->next_permutation = 0;
        }
        if (channel->current_permutation != 0) {
            if (channel->current_permutation->samples_pointer != 0xffffffff) {
                entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data +
                    (channel->current_permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
                entry->lock_count -= 1;
            }
            channel->current_permutation = 0;
        }
    }

    current_sound_driver->channel_stop(channel_index);
}

#if 0
Original Ghidra decompilation (0x54d0d0):

void FUN_0054d0d0(void)

{
  char *pcVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  short unaff_DI;

  iVar4 = DAT_006ac528;
  iVar2 = (int)unaff_DI;
  if (iVar2 * 0x18 != -0x724a60) {
    if ((&DAT_00724a74)[iVar2 * 6] != 0) {
      iVar3 = (*(uint *)((&DAT_00724a74)[iVar2 * 6] + 0x2c) & 0xffff) * 0x10 +
              *(int *)(DAT_006ac528 + 0x34);
      if (iVar3 != 0) {
        pcVar1 = (char *)(iVar3 + 5);
        *pcVar1 = *pcVar1 + -1;
      }
      (&DAT_00724a74)[iVar2 * 6] = 0;
    }
    if ((&DAT_00724a70)[iVar2 * 6] != 0) {
      iVar4 = (*(uint *)((&DAT_00724a70)[iVar2 * 6] + 0x2c) & 0xffff) * 0x10 +
              *(int *)(iVar4 + 0x34);
      if (iVar4 != 0) {
        pcVar1 = (char *)(iVar4 + 5);
        *pcVar1 = *pcVar1 + -1;
      }
      (&DAT_00724a70)[iVar2 * 6] = 0;
    }
  }
  (**(code **)(DAT_00725208 + 0x20))();
  return;
}

Disassembly (0x54d0d0..0x54d139, capstone), showing the "!= 0" pointer guard:

0x54d0d0: movsx eax, di
0x54d0d3: lea ecx, [eax + eax*2]
0x54d0d6: lea ecx, [ecx*8 + 0x724a60]
0x54d0dd: test ecx, ecx
0x54d0df: je 0x54d12f
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
