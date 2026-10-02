// sound_channel_stream_update  (Ghidra: FUN_005478c0)
// address 0x5478c0, size 307 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Determines how much ring-buffer space a
//   streaming channel needs refilled and triggers the fill, tracking loop transitions.";
//   directsound_channel.write_cursor/streaming_bytes/streaming/state (0x078/0x084/0x009/0x000,
//   types/sound.h) match; the recomputed ring-buffer size (channels * sample_rate * 6) equals
//   buffer_size exactly (block_align(=channels*2) * sample_rate * 3, as built in
//   src/sound/sound_channel_create.c), so this rewrite reads channel->buffer_size directly
//   instead of re-deriving it; IDirectSoundBuffer vtable slot 0x10 is GetCurrentPosition (called
//   here purely for its side effect -- its two outputs are never read, mirroring
//   src/sound/sound_channel_refresh_cursor.c); reuses sound_channel_lock_and_fill (0x547a00, this
//   batch).
// register convention: AX -> channel_index. Every caller also pushes one stack word (a
//   crosslap byte) that this function never reads; it is modeled as `unused`.
// Phase-4 review (disassembly appended below): the two positions the draft took as ESI / EDI
//   inputs are the play and write cursors returned by IDirectSoundBuffer::GetCurrentPosition
//   (vtable +0x10) into this function's own locals; the draft discarded those outputs and used
//   register garbage instead. lock_and_fill gets BX = channel_index.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

extern uint8_t sound_channel_lock_and_fill(int16_t channel_index, uint32_t fill_size); // 0x547a00, blam-cc: BX, stack


// blam-cc: AX -> channel_index, stack -> unused
// Refills the part of a playing (or silence-streaming) channel's ring buffer that the play
// cursor has consumed since the last fill. A channel that just switched to streaming silence
// (streaming_bytes == -1) starts from the hardware write cursor and records how much it wrote;
// later passes accumulate streaming_bytes (reset to a full buffer on overflow).
void sound_channel_stream_update(int16_t channel_index, uint8_t unused)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    int32_t buffer_size = channel->buffer_size; // the binary recomputes rate * channels * 6, the same value
    int32_t play_cursor;
    int32_t write_cursor;
    int32_t delta;
    void **vtable;

    (void)unused;
    if (channel->state == _directsound_channel_idle && channel->streaming != 1) {
        return;
    }

    vtable = *(void ***)channel->buffer;
    ((directsound_buffer_get_current_position_proc)vtable[0x10 / 4])(channel->buffer, &play_cursor, &write_cursor);

    if (channel->streaming == 0) {
        delta = play_cursor - channel->write_cursor;
        if (delta < 0) {
            delta += buffer_size;
        }
        if (delta != 0) {
            sound_channel_lock_and_fill(channel_index, (uint32_t)delta);
            channel->write_cursor = play_cursor;
        }
    } else if (channel->streaming_bytes == -1) {
        delta = write_cursor > play_cursor ? play_cursor - write_cursor + buffer_size : play_cursor - write_cursor;
        if (delta != 0) {
            channel->write_cursor = write_cursor;
            sound_channel_lock_and_fill(channel_index, (uint32_t)delta);
            channel->streaming_bytes = delta;
            channel->write_cursor = play_cursor;
        }
    } else {
        delta = play_cursor - channel->write_cursor;
        if (delta < 0) {
            delta += buffer_size;
        }
        if (delta != 0) {
            sound_channel_lock_and_fill(channel_index, (uint32_t)delta);
            channel->write_cursor = play_cursor;
            channel->streaming_bytes += delta;
            if (channel->streaming_bytes < 0) {
                channel->streaming_bytes = buffer_size;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x5478c0):

void FUN_005478c0(void)

{
  int iVar1;
  short in_AX;
  int unaff_ESI;
  int iVar2;
  int iVar3;
  int unaff_EDI;
  int iVar4;
  undefined1 local_8 [4];
  undefined1 local_4 [4];

  iVar2 = (int)in_AX;
  iVar3 = iVar2 * 0x678;
  iVar4 = ((((&DAT_00725468)[iVar3] & 2) != 0) + 1) *
          *(int *)(&DAT_0065e4f8 + ((byte)(&DAT_00725468)[iVar3] & 4)) * 6;
  if (((&DAT_00725430)[iVar2 * 0x33c] != 0) || ((&DAT_00725439)[iVar3] == '\x01')) {
    (**(code **)(*(int *)(&DAT_00725aa0)[iVar2 * 0x19e] + 0x10))
              ((int *)(&DAT_00725aa0)[iVar2 * 0x19e],local_8,local_4);
    if ((&DAT_00725439)[iVar3] == '\0') {
      iVar2 = unaff_EDI - *(int *)(&DAT_007254a8 + iVar3);
      if (iVar2 < 0) {
        iVar2 = iVar2 + iVar4;
      }
      if (iVar2 != 0) {
        sound_channel_lock_and_fill(iVar2);
        *(int *)(&DAT_007254a8 + iVar3) = unaff_EDI;
        return;
      }
    }
    else if (*(int *)(&DAT_007254b4 + iVar3) == -1) {
      if (unaff_EDI < unaff_ESI) {
        iVar4 = (unaff_EDI - unaff_ESI) + iVar4;
      }
      else {
        iVar4 = unaff_EDI - unaff_ESI;
      }
      if (iVar4 != 0) {
        *(int *)(&DAT_007254a8 + iVar3) = unaff_ESI;
        sound_channel_lock_and_fill(iVar4);
        *(int *)(&DAT_007254b4 + iVar3) = iVar4;
        *(int *)(&DAT_007254a8 + iVar3) = unaff_EDI;
        return;
      }
    }
    else {
      iVar2 = unaff_EDI - *(int *)(&DAT_007254a8 + iVar3);
      if (iVar2 < 0) {
        iVar2 = iVar2 + iVar4;
      }
      if (iVar2 != 0) {
        sound_channel_lock_and_fill(iVar2);
        iVar1 = *(int *)(&DAT_007254b4 + iVar3);
        *(int *)(&DAT_007254a8 + iVar3) = unaff_EDI;
        *(int *)(&DAT_007254b4 + iVar3) = iVar1 + iVar2;
        if (iVar1 + iVar2 < 0) {
          *(int *)(&DAT_007254b4 + iVar3) = iVar4;
        }
      }
    }
  }
  return;
}

Disassembly (0x5478c0..0x5479f3, capstone; phase-4 review):

0x5478c0: sub esp, 8
0x5478c3: push ebx
0x5478c4: mov ebx, eax
0x5478c6: push esi
0x5478c7: movsx esi, bx
0x5478ca: imul esi, esi, 0x678
0x5478d0: mov al, byte ptr [esi + 0x725468]
0x5478d6: xor ecx, ecx
0x5478d8: mov cl, byte ptr [esi + 0x725468]
0x5478de: push edi
0x5478df: and ecx, 4
0x5478e2: shr ecx, 2
0x5478e5: test al, 2
0x5478e7: mov eax, 0
0x5478ec: setne al
0x5478ef: inc eax
0x5478f0: imul eax, dword ptr [ecx*4 + 0x65e4f8]
0x5478f8: lea edi, [eax + eax*2]
0x5478fb: shl edi, 1
0x5478fd: cmp word ptr [esi + 0x725430], 0
0x547905: jne 0x547914
0x547907: cmp byte ptr [esi + 0x725439], 1
0x54790e: jne 0x5479ec
0x547914: mov eax, dword ptr [esi + 0x725aa0]
0x54791a: mov edx, dword ptr [eax]
0x54791c: lea ecx, [esp + 0x10]
0x547920: push ecx
0x547921: lea ecx, [esp + 0x10]
0x547925: push ecx
0x547926: push eax
0x547927: call dword ptr [edx + 0x10]
0x54792a: mov al, byte ptr [esi + 0x725439]
0x547930: test al, al
0x547932: jne 0x547964
0x547934: mov eax, dword ptr [esp + 0xc]
0x547938: sub eax, dword ptr [esi + 0x7254a8]
0x54793e: jns 0x547942
0x547940: add eax, edi
0x547942: test eax, eax
0x547944: je 0x5479ec
0x54794a: push eax
0x54794b: call 0x547a00
0x547950: mov edx, dword ptr [esp + 0x10]
0x547954: add esp, 4
0x547957: pop edi
0x547958: mov dword ptr [esi + 0x7254a8], edx
0x54795e: pop esi
0x54795f: pop ebx
0x547960: add esp, 8
0x547963: ret 
0x547964: cmp dword ptr [esi + 0x7254b4], -1
0x54796b: jne 0x5479ad
0x54796d: mov ecx, dword ptr [esp + 0x10]
0x547971: mov eax, dword ptr [esp + 0xc]
0x547975: cmp ecx, eax
0x547977: jle 0x54797f
0x547979: sub eax, ecx
0x54797b: add eax, edi
0x54797d: jmp 0x547981
0x54797f: sub eax, ecx
0x547981: mov edi, eax
0x547983: test edi, edi
0x547985: je 0x5479ec
0x547987: push edi
0x547988: mov dword ptr [esi + 0x7254a8], ecx
0x54798e: call 0x547a00
0x547993: mov eax, dword ptr [esp + 0x10]
0x547997: add esp, 4
0x54799a: mov dword ptr [esi + 0x7254b4], edi
0x5479a0: pop edi
0x5479a1: mov dword ptr [esi + 0x7254a8], eax
0x5479a7: pop esi
0x5479a8: pop ebx
0x5479a9: add esp, 8
0x5479ac: ret 
0x5479ad: mov eax, dword ptr [esi + 0x7254a8]
0x5479b3: push ebp
0x5479b4: mov ebp, dword ptr [esp + 0x10]
0x5479b8: sub ebp, eax
0x5479ba: jns 0x5479be
0x5479bc: add ebp, edi
0x5479be: test ebp, ebp
0x5479c0: je 0x5479eb
0x5479c2: push ebp
0x5479c3: call 0x547a00
0x5479c8: mov eax, dword ptr [esi + 0x7254b4]
0x5479ce: mov ecx, dword ptr [esp + 0x14]
0x5479d2: add esp, 4
0x5479d5: add eax, ebp
0x5479d7: mov dword ptr [esi + 0x7254a8], ecx
0x5479dd: mov dword ptr [esi + 0x7254b4], eax
0x5479e3: jns 0x5479eb
0x5479e5: mov dword ptr [esi + 0x7254b4], edi
0x5479eb: pop ebp
0x5479ec: pop edi
0x5479ed: pop esi
0x5479ee: pop ebx
0x5479ef: add esp, 8
0x5479f2: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
