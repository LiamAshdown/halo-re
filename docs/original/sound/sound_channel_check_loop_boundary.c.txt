// sound_channel_check_loop_boundary  (Ghidra: FUN_00548050)
// address 0x548050, size 146 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Detects when a channel's playback has crossed
//   a recorded loop boundary and advances its playback-transition state accordingly."; matches
//   directsound_channel.state/source_end_cursor/write_cursor (0x000/0x07c/0x078, types/sound.h);
//   IDirectSoundBuffer vtable slot 0x10 is GetCurrentPosition; the wraparound-aware comparison is
//   the standard "did the play cursor cross this ring-buffer boundary" idiom.
// register convention: channel index in AX (in_AX).
// blam-cc: AX -> channel_index
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

// blam-cc: AX -> channel_index
// When the channel has a pending source_end_cursor, checks whether the hardware play cursor has
// crossed from before it to at-or-past write_cursor (wraparound-aware). If so: a queued (state 2)
// channel becomes playing (1) and this returns 1 immediately; a playing (state 1) channel with no
// successor becomes idle (0). Either way source_end_cursor is cleared. Returns the resulting
// state.
directsound_channel_state sound_channel_check_loop_boundary(int16_t channel_index)
{
    directsound_channel *channel = &directsound_channels[channel_index];

    if (channel->state != 0 && channel->source_end_cursor != -1) {
        void **vtable = *(void ***)channel->buffer;
        int32_t (__stdcall *get_current_position)(void *, int32_t *, int32_t *) =
            (int32_t (__stdcall *)(void *, int32_t *, int32_t *))vtable[0x10 / 4];
        int32_t play_cursor;
        int32_t write_cursor_unused;
        int32_t end = channel->source_end_cursor;
        int32_t write = channel->write_cursor;

        get_current_position(channel->buffer, &play_cursor, &write_cursor_unused);

        if ((end < write && end < play_cursor && play_cursor < write) ||
            (write < end && (end < play_cursor || play_cursor < write))) {
            if (channel->state == _directsound_channel_queued) {
                channel->state = _directsound_channel_playing;
                channel->source_end_cursor = -1;
                return (directsound_channel_state)channel->state;
            }
            if (channel->state == _directsound_channel_playing) {
                channel->state = _directsound_channel_idle;
            }
            channel->source_end_cursor = -1;
        }
    }

    return (directsound_channel_state)channel->state;
}

#if 0
Original Ghidra decompilation (0x548050):

short FUN_00548050(void)

{
  short *psVar1;
  short sVar2;
  int iVar3;
  short in_AX;
  int iVar4;
  int iVar5;
  undefined1 *puVar6;
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  iVar4 = (int)in_AX;
  iVar5 = iVar4 * 0x678;
  psVar1 = &DAT_00725430 + iVar4 * 0x33c;
  if (((&DAT_00725430)[iVar4 * 0x33c] != 0) && (*(int *)(&DAT_007254ac + iVar5) != -1)) {
    puVar6 = local_8;
    (**(code **)(*(int *)(&DAT_00725aa0)[iVar4 * 0x19e] + 0x10))
              ((int *)(&DAT_00725aa0)[iVar4 * 0x19e],puVar6,local_4);
    iVar4 = *(int *)(&DAT_007254ac + iVar5);
    iVar3 = *(int *)(&DAT_007254a8 + iVar5);
    if (((iVar4 < iVar3) && ((iVar4 < (int)puVar6 && ((int)puVar6 < iVar3)))) ||
       ((iVar3 < iVar4 && ((iVar4 < (int)puVar6 || ((int)puVar6 < iVar3)))))) {
      if (*psVar1 == 2) {
        *psVar1 = 1;
        sVar2 = *psVar1;
        *(undefined4 *)(&DAT_007254ac + iVar5) = 0xffffffff;
        return sVar2;
      }
      if (*psVar1 == 1) {
        *psVar1 = 0;
      }
      *(undefined4 *)(&DAT_007254ac + iVar5) = 0xffffffff;
    }
  }
  return *psVar1;
}

Disassembly (0x548050..0x5480e2, capstone; phase-4 review):

0x548050: sub esp, 8
0x548053: push esi
0x548054: movsx esi, ax
0x548057: imul esi, esi, 0x678
0x54805d: cmp word ptr [esi + 0x725430], 0
0x548065: lea esi, [esi + 0x725430]
0x54806b: je 0x5480da
0x54806d: cmp dword ptr [esi + 0x7c], -1
0x548071: je 0x5480da
0x548073: mov eax, dword ptr [esi + 0x670]
0x548079: mov ecx, dword ptr [eax]
0x54807b: lea edx, [esp + 8]
0x54807f: push edx
0x548080: lea edx, [esp + 8]
0x548084: push edx
0x548085: push eax
0x548086: call dword ptr [ecx + 0x10]
0x548089: mov eax, dword ptr [esi + 0x7c]
0x54808c: mov ecx, dword ptr [esi + 0x78]
0x54808f: cmp eax, ecx
0x548091: mov edx, dword ptr [esp + 4]
0x548095: jge 0x5480a1
0x548097: cmp eax, edx
0x548099: jge 0x54809f
0x54809b: cmp edx, ecx
0x54809d: jl 0x5480ab
0x54809f: cmp eax, ecx
0x5480a1: jle 0x5480da
0x5480a3: cmp eax, edx
0x5480a5: jl 0x5480ab
0x5480a7: cmp edx, ecx
0x5480a9: jge 0x5480da
0x5480ab: mov ax, word ptr [esi]
0x5480ae: cmp ax, 2
0x5480b2: jne 0x5480c8
0x5480b4: mov word ptr [esi], 1
0x5480b9: mov ax, word ptr [esi]
0x5480bc: mov dword ptr [esi + 0x7c], 0xffffffff
0x5480c3: pop esi
0x5480c4: add esp, 8
0x5480c7: ret 
0x5480c8: cmp ax, 1
0x5480cc: jne 0x5480d3
0x5480ce: mov word ptr [esi], 0
0x5480d3: mov dword ptr [esi + 0x7c], 0xffffffff
0x5480da: mov ax, word ptr [esi]
0x5480dd: pop esi
0x5480de: add esp, 8
0x5480e1: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
