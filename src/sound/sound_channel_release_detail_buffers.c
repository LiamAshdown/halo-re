// sound_channel_release_detail_buffers  (Ghidra: FUN_0054d020, still unnamed there)
// address 0x54d020, size 176 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Releases a channel's cached detail-sound buffers
// once the driver's active voice budget requires it, and accumulates its play-time weight.";
// sound_channel fields (current_permutation 0x10, next_permutation 0x14, play_time 0x08,
// current_pitch 0x0c) match types/sound.h exactly (0x00724a60 + those offsets).
// register convention: CX -> channel_index.
// Phase-4 review (disassembly appended below): sound_cache_touch gets (0, 0, BL 0, EDI = the
// newly promoted current permutation), confirmed; the lock released when promoting is the OLD
// current permutation's (the draft released the queued one's, then promoted it).
// The CONCAT22 return (channel_get_state's low 16 bits plus leftover high bits) is written here
// as a plain int16_t return, matching the AX-only-return pattern confirmed by disassembly for
// sound_definition_check_promotion.c (0x54b050).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"

extern sound_driver *current_sound_driver;  // 0x00725208, header calls this "sound_driver"
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60
extern data_array *sound_cache_entries;     // 0x006ac528
extern float sound_time_delta;              // 0x00725210

extern uint8_t sound_cache_touch(uint8_t allocate_if_missing, uint8_t lock, uint8_t wait_until_loaded,
    void *permutation); // 0x443e10, cache module, blam-cc: stack, stack, BL, EDI

// blam-cc: CX -> channel_index
// Once the driver reports this channel's voice has fewer than 2 (then fewer than 1) sources
// queued, releases the cached page reference for its queued/current permutation and promotes the
// queued one forward. Always accumulates this channel's play_time by pitch * elapsed time.
// Returns the driver's channel state (clamped to 0 if sound_cache_touch reports failure).
int16_t sound_channel_release_detail_buffers(int16_t channel_index)
{
    sound_channel *channel;
    sound_cache_entry *entry;
    int16_t channel_state;

    channel = &sound_channels[channel_index];
    channel_state = current_sound_driver->channel_get_state(channel_index);

    if (channel->next_permutation != 0 && channel_state < 2) {
        // the finished current permutation gives up its cache lock (the binary reads
        // current_permutation here without a NULL test)
        if (channel->current_permutation->samples_pointer != 0xffffffff) {
            entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data +
                (channel->current_permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
            entry->lock_count -= 1;
        }
        channel->current_permutation = channel->next_permutation;
        channel->next_permutation = 0;
        channel->play_time = 0.0f;
        if (!sound_cache_touch(0, 0, 0, channel->current_permutation)) {
            channel_state = 0;
        }
    }

    if (channel->current_permutation != 0 && channel_state < 1) {
        if (channel->current_permutation->samples_pointer != 0xffffffff) {
            entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data +
                (channel->current_permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));
            entry->lock_count -= 1;
        }
        channel->current_permutation = 0;
    }

    channel->play_time = sound_time_delta * channel->current_pitch + channel->play_time;
    return channel_state;
}

#if 0
Original Ghidra decompilation (0x54d020):

undefined4 FUN_0054d020(void)

{
  char *pcVar1;
  char cVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  short in_CX;

  iVar4 = (int)in_CX;
  sVar3 = (**(code **)(DAT_00725208 + 0x24))();
  if (((&DAT_00724a74)[iVar4 * 6] != 0) && (sVar3 < 2)) {
    iVar5 = (*(uint *)((&DAT_00724a70)[iVar4 * 6] + 0x2c) & 0xffff) * 0x10 +
            *(int *)(DAT_006ac528 + 0x34);
    if (iVar5 != 0) {
      pcVar1 = (char *)(iVar5 + 5);
      *pcVar1 = *pcVar1 + -1;
    }
    (&DAT_00724a70)[iVar4 * 6] = (&DAT_00724a74)[iVar4 * 6];
    (&DAT_00724a74)[iVar4 * 6] = 0;
    (&DAT_00724a68)[iVar4 * 6] = 0;
    cVar2 = sound_cache_touch(0,0);
    if (cVar2 == '\0') {
      sVar3 = 0;
    }
  }
  iVar5 = (&DAT_00724a70)[iVar4 * 6];
  if ((iVar5 != 0) && (sVar3 < 1)) {
    iVar5 = (*(uint *)(iVar5 + 0x2c) & 0xffff) * 0x10 + *(int *)(DAT_006ac528 + 0x34);
    if (iVar5 != 0) {
      *(char *)(iVar5 + 5) = *(char *)(iVar5 + 5) + -1;
    }
    (&DAT_00724a70)[iVar4 * 6] = 0;
  }
  (&DAT_00724a68)[iVar4 * 6] =
       DAT_00725210 * *(float *)(&DAT_00724a6c + iVar4 * 0x18) + (float)(&DAT_00724a68)[iVar4 * 6];
  return CONCAT22((short)((uint)iVar5 >> 0x10),sVar3);
}

Disassembly (0x54d020..0x54d0d0, capstone; phase-4 review):

0x54d020: movsx eax, cx
0x54d023: push ebp
0x54d024: push esi
0x54d025: lea esi, [eax + eax*2]
0x54d028: mov eax, dword ptr [0x725208]
0x54d02d: push ecx
0x54d02e: lea esi, [esi*8 + 0x724a60]
0x54d035: call dword ptr [eax + 0x24]
0x54d038: xor ecx, ecx
0x54d03a: add esp, 4
0x54d03d: mov ebp, eax
0x54d03f: cmp dword ptr [esi + 0x14], ecx
0x54d042: je 0x54d08c
0x54d044: cmp bp, 2
0x54d048: jge 0x54d08c
0x54d04a: mov edx, dword ptr [esi + 0x10]
0x54d04d: mov eax, dword ptr [edx + 0x2c]
0x54d050: mov edx, dword ptr [0x6ac528]
0x54d056: push ebx
0x54d057: and eax, 0xffff
0x54d05c: push edi
0x54d05d: mov edi, dword ptr [edx + 0x34]
0x54d060: shl eax, 4
0x54d063: add eax, edi
0x54d065: je 0x54d06a
0x54d067: dec byte ptr [eax + 5]
0x54d06a: mov eax, dword ptr [esi + 0x14]
0x54d06d: push ecx
0x54d06e: push ecx
0x54d06f: xor bl, bl
0x54d071: mov edi, eax
0x54d073: mov dword ptr [esi + 0x10], eax
0x54d076: mov dword ptr [esi + 0x14], ecx
0x54d079: mov dword ptr [esi + 8], ecx
0x54d07c: call 0x443e10
0x54d081: add esp, 8
0x54d084: test al, al
0x54d086: pop edi
0x54d087: pop ebx
0x54d088: jne 0x54d08c
0x54d08a: xor ebp, ebp
0x54d08c: mov eax, dword ptr [esi + 0x10]
0x54d08f: test eax, eax
0x54d091: je 0x54d0bb
0x54d093: cmp bp, 1
0x54d097: jge 0x54d0bb
0x54d099: mov eax, dword ptr [eax + 0x2c]
0x54d09c: mov ecx, dword ptr [0x6ac528]
0x54d0a2: mov edx, dword ptr [ecx + 0x34]
0x54d0a5: and eax, 0xffff
0x54d0aa: shl eax, 4
0x54d0ad: add eax, edx
0x54d0af: je 0x54d0b4
0x54d0b1: dec byte ptr [eax + 5]
0x54d0b4: mov dword ptr [esi + 0x10], 0
0x54d0bb: fld dword ptr [0x725210]
0x54d0c1: mov ax, bp
0x54d0c4: fmul dword ptr [esi + 0xc]
0x54d0c7: fadd dword ptr [esi + 8]
0x54d0ca: fstp dword ptr [esi + 8]
0x54d0cd: pop esi
0x54d0ce: pop ebp
0x54d0cf: ret 
#endif
