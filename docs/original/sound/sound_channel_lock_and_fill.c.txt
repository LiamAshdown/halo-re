// sound_channel_lock_and_fill  (Ghidra: sound_channel_lock_and_fill, already named)
// address 0x547a00, size 172 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Locks a channel's DirectSound ring buffer,
//   fills the returned write segment(s) with queued audio data, and unlocks it."; directsound_
//   channel.buffer/write_cursor (0x670/0x078, types/sound.h); IDirectSoundBuffer vtable slot 0x2c
//   is Lock(dwOffset, dwBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2,
//   dwFlags), slot 0x4c is Unlock(pvAudioPtr1, dwAudioBytes1, pvAudioPtr2, dwAudioBytes2); reuses
//   sound_channel_fill_pcm_data (0x547ab0, this batch).
// register convention: channel index in BX (unaff_BX); fill size as the one stack parameter
//   Ghidra recognizes directly (param_1).
// blam-cc: BX -> channel_index, stack -> fill_size
// Phase-4 review (disassembly appended below): Unlock is the standard (ptr1, bytes1, ptr2,
//   bytes2); Lock's out-pointers are (&ptr1, &bytes1, &ptr2, &bytes2). The fills are
//   fill_pcm_data(channel, ptr1, write_cursor, &source_crosslap; EAX = bytes1) and, when the
//   lock wrapped, fill_pcm_data(channel, ptr2, 0, &source_crosslap; EAX = bytes2). The draft
//   filled segment 1 twice. Returns AL: 1 when Lock and Unlock both succeed.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

extern void sound_channel_fill_pcm_data(int16_t channel_index, uint8_t *destination, int32_t base_position,
    uint8_t *crosslap, int32_t byte_count); // 0x547ab0, blam-cc: stack x4, EAX


// blam-cc: BX -> channel_index, stack -> fill_size
// Locks `fill_size` bytes of the channel's ring buffer at its write_cursor, fills the one or
// two returned segments from the channel's sources, and unlocks.
uint8_t sound_channel_lock_and_fill(int16_t channel_index, uint32_t fill_size)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    void **vtable = *(void ***)channel->buffer;
    void *ptr1;
    uint32_t bytes1;
    void *ptr2;
    uint32_t bytes2;

    if (((directsound_buffer_lock_proc)vtable[0x2c / 4])(channel->buffer, channel->write_cursor, fill_size,
            &ptr1, &bytes1, &ptr2, &bytes2, 0) < 0) {
        return 0;
    }

    sound_channel_fill_pcm_data(channel_index, (uint8_t *)ptr1, channel->write_cursor, &channel->source_crosslap,
        (int32_t)bytes1);
    if (ptr2 != (void *)0) {
        sound_channel_fill_pcm_data(channel_index, (uint8_t *)ptr2, 0, &channel->source_crosslap, (int32_t)bytes2);
    }

    vtable = *(void ***)channel->buffer;
    if (((directsound_buffer_unlock_proc)vtable[0x4c / 4])(channel->buffer, ptr1, bytes1, ptr2, bytes2) < 0) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x547a00):

undefined4 sound_channel_lock_and_fill(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  short unaff_BX;
  int iVar3;
  undefined1 *puVar4;
  undefined1 *puVar5;
  undefined1 local_10 [4];
  undefined1 local_c [4];
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  iVar3 = (int)unaff_BX;
  puVar5 = local_8;
  puVar4 = local_4;
  iVar2 = *(int *)(&DAT_007254a8 + iVar3 * 0x678);
  iVar1 = (**(code **)(*(int *)(&DAT_00725aa0)[iVar3 * 0x19e] + 0x2c))
                    ((int *)(&DAT_00725aa0)[iVar3 * 0x19e],iVar2,param_1,puVar4,puVar5,local_10,
                     local_c,0);
  if (-1 < iVar1) {
    sound_channel_fill_pcm_data();
    if (iVar2 != 0) {
      sound_channel_fill_pcm_data();
    }
    iVar2 = (**(code **)(*(int *)(&DAT_00725aa0)[iVar3 * 0x19e] + 0x4c))
                      ((int *)(&DAT_00725aa0)[iVar3 * 0x19e],puVar5,puVar4,iVar2,param_1);
    if (-1 < iVar2) {
      return 1;
    }
  }
  return 0;
}

Disassembly (0x547a00..0x547aac, capstone; phase-4 review):

0x547a00: sub esp, 0x10
0x547a03: push esi
0x547a04: push 0
0x547a06: movsx esi, bx
0x547a09: lea edx, [esp + 0xc]
0x547a0d: imul esi, esi, 0x678
0x547a13: push edx
0x547a14: lea edx, [esp + 0xc]
0x547a18: push edx
0x547a19: lea edx, [esp + 0x18]
0x547a1d: push edx
0x547a1e: lea edx, [esp + 0x20]
0x547a22: push edx
0x547a23: mov edx, dword ptr [esp + 0x2c]
0x547a27: push edx
0x547a28: add esi, 0x725430
0x547a2e: mov edx, dword ptr [esi + 0x78]
0x547a31: mov eax, dword ptr [esi + 0x670]
0x547a37: mov ecx, dword ptr [eax]
0x547a39: push edx
0x547a3a: push eax
0x547a3b: call dword ptr [ecx + 0x2c]
0x547a3e: test eax, eax
0x547a40: jl 0x547aa5
0x547a42: mov eax, dword ptr [esi + 0x78]
0x547a45: mov ecx, dword ptr [esp + 0x10]
0x547a49: push edi
0x547a4a: lea edi, [esi + 0x90]
0x547a50: push edi
0x547a51: push eax
0x547a52: mov eax, dword ptr [esp + 0x18]
0x547a56: push ecx
0x547a57: push ebx
0x547a58: call 0x547ab0
0x547a5d: mov eax, dword ptr [esp + 0x18]
0x547a61: add esp, 0x10
0x547a64: test eax, eax
0x547a66: je 0x547a7d
0x547a68: push edi
0x547a69: push 0
0x547a6b: push eax
0x547a6c: mov eax, dword ptr [esp + 0x18]
0x547a70: push ebx
0x547a71: call 0x547ab0
0x547a76: mov eax, dword ptr [esp + 0x18]
0x547a7a: add esp, 0x10
0x547a7d: mov ecx, dword ptr [esp + 0xc]
0x547a81: mov esi, dword ptr [esi + 0x670]
0x547a87: mov edx, dword ptr [esi]
0x547a89: push ecx
0x547a8a: mov ecx, dword ptr [esp + 0x18]
0x547a8e: push eax
0x547a8f: mov eax, dword ptr [esp + 0x18]
0x547a93: push eax
0x547a94: push ecx
0x547a95: push esi
0x547a96: call dword ptr [edx + 0x4c]
0x547a99: test eax, eax
0x547a9b: pop edi
0x547a9c: jl 0x547aa5
0x547a9e: mov al, 1
0x547aa0: pop esi
0x547aa1: add esp, 0x10
0x547aa4: ret 
0x547aa5: xor al, al
0x547aa7: pop esi
0x547aa8: add esp, 0x10
0x547aab: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
