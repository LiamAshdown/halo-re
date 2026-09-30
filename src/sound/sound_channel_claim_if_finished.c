// sound_channel_claim_if_finished  (Ghidra: FUN_00547ff0)
// address 0x547ff0, size 84 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Checks whether a channel has finished playing
//   and, if so, claims it as no longer free for reuse."; directsound_channel.streaming (0x009),
//   streaming_bytes (0x084), free (0x008) (types/sound.h); IDirectSoundBuffer vtable slot 0x24 is
//   GetStatus (out status matches DSBSTATUS_PLAYING 0x1 / DSBSTATUS_LOOPING 0x4).
// register convention: channel index in AX (in_AX).
// blam-cc: AX -> channel_index
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

// blam-cc: AX -> channel_index
// Queries the channel's hardware playback status; fails (returns 0) if the query itself fails, or
// if the channel is not marked streaming and is still reported playing/looping. Otherwise resets
// streaming_bytes (for a streaming channel) and clears `free`, claiming the slot.
uint32_t sound_channel_claim_if_finished(int16_t channel_index)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    void **vtable = *(void ***)channel->buffer;
    int32_t (__stdcall *get_status)(void *, uint32_t *) = (int32_t (__stdcall *)(void *, uint32_t *))vtable[0x24 / 4];
    uint32_t status;
    int32_t hr = get_status(channel->buffer, &status);

    if (hr < 0) {
        return 0;
    }

    if (channel->streaming == 0) {
        if ((status & 5) != 0) {
            return 0;
        }
    } else {
        channel->streaming_bytes = -1;
    }

    channel->free = 0;
    return 1;
}

#if 0
Original Ghidra decompilation (0x547ff0):

undefined4 FUN_00547ff0(void)

{
  short in_AX;
  int iVar1;
  uint unaff_ESI;
  int iVar2;
  undefined1 local_4 [4];

  iVar2 = in_AX * 0x678;
  iVar1 = (**(code **)(*(int *)(&DAT_00725aa0)[in_AX * 0x19e] + 0x24))
                    ((int *)(&DAT_00725aa0)[in_AX * 0x19e],local_4);
  if (iVar1 < 0) {
    return 0;
  }
  if ((&DAT_00725439)[iVar2] == '\0') {
    if ((unaff_ESI & 5) != 0) {
      return 0;
    }
  }
  else {
    *(undefined4 *)(&DAT_007254b4 + iVar2) = 0xffffffff;
  }
  (&DAT_00725438)[iVar2] = 0;
  return 1;
}

Disassembly (0x547ff0..0x548044, capstone; phase-4 review):

0x547ff0: push ecx
0x547ff1: push ebx
0x547ff2: push esi
0x547ff3: movsx esi, ax
0x547ff6: imul esi, esi, 0x678
0x547ffc: lea edx, [esp + 8]
0x548000: add esi, 0x725430
0x548006: mov eax, dword ptr [esi + 0x670]
0x54800c: mov ecx, dword ptr [eax]
0x54800e: push edx
0x54800f: push eax
0x548010: xor bl, bl
0x548012: call dword ptr [ecx + 0x24]
0x548015: test eax, eax
0x548017: jl 0x54803e
0x548019: cmp byte ptr [esi + 9], bl
0x54801c: je 0x548031
0x54801e: mov dword ptr [esi + 0x84], 0xffffffff
0x548028: mov byte ptr [esi + 8], bl
0x54802b: pop esi
0x54802c: mov al, 1
0x54802e: pop ebx
0x54802f: pop ecx
0x548030: ret 
0x548031: test byte ptr [esp + 8], 5
0x548036: je 0x548028
0x548038: pop esi
0x548039: xor al, al
0x54803b: pop ebx
0x54803c: pop ecx
0x54803d: ret 
0x54803e: pop esi
0x54803f: mov al, bl
0x548041: pop ebx
0x548042: pop ecx
0x548043: ret 
#endif
