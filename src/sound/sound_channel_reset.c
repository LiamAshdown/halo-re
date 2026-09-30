// sound_channel_reset  (Ghidra: FUN_00547f60)
// address 0x547f60, size 130 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Resets a sound channel back to the idle/free
//   pool, stopping or restarting its stream as appropriate."; directsound_channel fields (source/
//   next_source 0x088/0x08c, type_flags 0x038, sound_class 0x004, streaming 0x009, streaming_bytes
//   0x084, source_started 0x098, state 0x000, free 0x008) all match types/sound.h by offset;
//   decoder.open at 0x648 matches the header's own cross-reference note on this exact address
//   ("0x00725a78 in 0x547f60: it is channel 0 decoder.open (0xa0 + 0x5a8 = 0x648)");
//   IDirectSoundBuffer vtable slot 0x48 is Stop; reuses FUN_005478c0 (this batch).
// register convention: channel index in AX (in_AX).
// blam-cc: AX -> channel_index
// UNSURE: 0x00722b58 has no established name (types/sound.h lists it only as "referenced, owned
//   elsewhere"); kept as a raw global boolean-style test.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern uint32_t config_enable_stop_start; // 0x00722b58, UNSURE, see file header
extern uint8_t sound_stopping_all; // 0x007252b7
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430


// blam-cc: AX -> channel_index
// Clears the channel's queued sources; for a weapon-fire 3D channel not mid-stop-all, keeps it
// streaming and re-triggers a refill instead of a hard stop. Otherwise stops the DirectSound
// buffer outright. Either way resets the channel to idle/free.
void sound_channel_reset(int16_t channel_index)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    uint8_t keep_streaming = (config_enable_stop_start == 0); // UNSURE, see file header

    channel->source = (SoundPermutation *)0;
    channel->next_source = (SoundPermutation *)0;

    if (keep_streaming && (channel->type_flags & _sound_channel_3d_bit) != 0 &&
        channel->sound_class == soundclass_weapon_fire && sound_stopping_all == 0) {
        channel->streaming = 1;
        sound_channel_stream_update(channel_index, 0); // pushes a 0 the callee never reads
    } else {
        void **vtable = *(void ***)channel->buffer;
        int32_t (__stdcall *stop)(void *) = (int32_t (__stdcall *)(void *))vtable[0x48 / 4];
        stop(channel->buffer);
        channel->streaming_bytes = -1;
        channel->streaming = 0;
    }

    channel->source_started = 0;
    channel->state = _directsound_channel_idle;
    channel->decoder.open = 0;
    channel->free = 1;
    channel->sound_class = -1;
}

#if 0
Original Ghidra decompilation (0x547f60):

void FUN_00547f60(void)

{
  short in_AX;
  int iVar1;
  int iVar2;
  bool bVar3;

  iVar1 = (int)in_AX;
  iVar2 = iVar1 * 0x678;
  bVar3 = DAT_00722b58 == 0;
  (&DAT_007254b8)[iVar1 * 0x19e] = 0;
  (&DAT_007254bc)[iVar1 * 0x19e] = 0;
  if ((((bVar3) && (((&DAT_00725468)[iVar2] & 1) != 0)) && (*(short *)(&DAT_00725434 + iVar2) == 4))
     && (DAT_007252b7 == '\0')) {
    (&DAT_00725439)[iVar2] = 1;
    FUN_005478c0(0);
  }
  else {
    (**(code **)(*(int *)(&DAT_00725aa0)[iVar1 * 0x19e] + 0x48))
              ((int *)(&DAT_00725aa0)[iVar1 * 0x19e]);
    *(undefined4 *)(&DAT_007254b4 + iVar2) = 0xffffffff;
    (&DAT_00725439)[iVar2] = 0;
  }
  (&DAT_007254c8)[iVar2] = 0;
  (&DAT_00725430)[iVar1 * 0x33c] = 0;
  (&DAT_00725a78)[iVar2] = 0;
  (&DAT_00725438)[iVar2] = 1;
  *(undefined2 *)(&DAT_00725434 + iVar2) = 0xffff;
  return;
}

Disassembly (0x547f60..0x547fe2, capstone; phase-4 review):

0x547f60: mov ecx, dword ptr [0x722b58]
0x547f66: push ebx
0x547f67: push esi
0x547f68: movsx esi, ax
0x547f6b: imul esi, esi, 0x678
0x547f71: xor ebx, ebx
0x547f73: add esi, 0x725430
0x547f79: cmp ecx, ebx
0x547f7b: mov dword ptr [esi + 0x88], ebx
0x547f81: mov dword ptr [esi + 0x8c], ebx
0x547f87: jne 0x547fad
0x547f89: test byte ptr [esi + 0x38], 1
0x547f8d: je 0x547fad
0x547f8f: cmp word ptr [esi + 4], 4
0x547f94: jne 0x547fad
0x547f96: cmp byte ptr [0x7252b7], bl
0x547f9c: jne 0x547fad
0x547f9e: push ebx
0x547f9f: mov byte ptr [esi + 9], 1
0x547fa3: call 0x5478c0
0x547fa8: add esp, 4
0x547fab: jmp 0x547fc6
0x547fad: mov eax, dword ptr [esi + 0x670]
0x547fb3: mov ecx, dword ptr [eax]
0x547fb5: push eax
0x547fb6: call dword ptr [ecx + 0x48]
0x547fb9: mov dword ptr [esi + 0x84], 0xffffffff
0x547fc3: mov byte ptr [esi + 9], bl
0x547fc6: mov byte ptr [esi + 0x98], bl
0x547fcc: mov word ptr [esi], bx
0x547fcf: mov byte ptr [esi + 0x648], bl
0x547fd5: mov byte ptr [esi + 8], 1
0x547fd9: mov word ptr [esi + 4], 0xffff
0x547fdf: pop esi
0x547fe0: pop ebx
0x547fe1: ret 
#endif
