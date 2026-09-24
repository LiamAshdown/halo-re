// sound_channel_queue_source  (Ghidra: FUN_00547c80)
// address 0x547c80, size 721 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: out/phase4/sound_functions.md summary "Queues a new sound source for playback on a
//   channel, initializing or chaining onto the channel's streaming state machine."; reached
//   through sound_driver.channel_play (0x548380 maps the logical channel to its hardware channel
//   and passes CL = its `streaming` argument); directsound_channel state/source/next_source/
//   source_crosslap/next_source_crosslap/source_end_cursor/write_cursor/streaming/sound_class/
//   streaming_bytes (types/sound.h); IDirectSoundBuffer +0x10 GetCurrentPosition, +0x30 Play,
//   +0x34 SetCurrentPosition; IDirectSound3DListener +0x44 CommitDeferredSettings.
// Phase-4 review: rewritten from the disassembly appended below. The draft invented a fourth
//   "play immediately" parameter (it is a local: set when the channel was not streaming, or its
//   play and write cursors coincide), passed placeholder cursors to stream_update (it reads the
//   cursors itself), treated the refresh_cursor result as a pointer (it is the play cursor),
//   passed an extra argument to CommitDeferredSettings, set the queued next source's crosslap
//   from the wrong value, and missed the second fill after a lost buffer is restored.
// register convention: stack -> (channel_index, source, sound_class), CL -> crosslap.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430
extern void *directsound_listener;            // 0x00746114, IDirectSound3DListener *
extern uint8_t directsound_deferred_dirty;    // 0x00746132
extern int32_t k_sound_sample_rates[2];       // 0x0065e4f8, { 22050, 44100 }

extern uint8_t sound_channel_lock_and_fill(int16_t channel_index, uint32_t fill_size); // 0x547a00, blam-cc: BX, stack
extern uint32_t sound_channel_refresh_cursor(int16_t channel_index); // 0x547890, blam-cc: AX
extern void sound_channel_stream_update(int16_t channel_index, uint8_t unused); // 0x5478c0, blam-cc: AX, stack
extern int32_t sound_channel_restore_buffer(void *buffer, uint8_t *was_restored_out); // 0x547c10, blam-cc: ESI, EBX


// blam-cc: stack -> (channel_index, source, sound_class), CL -> crosslap
// Idle channel: takes `source`, fills the ring buffer (all of it, or the part between the
// cursors if it is still streaming silence) and, if the buffer was not already running, rewinds,
// restores and starts it looping. Playing channel: the source becomes the queued next source (or
// the current one, if the channel had run dry). Queued channel: replaces the queued next source,
// or, when the current source ended far enough before the play cursor, splices the new source in
// right at that end point.
void sound_channel_queue_source(int16_t channel_index, SoundPermutation *source, int16_t sound_class, uint8_t crosslap)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    int32_t sample_rate = k_sound_sample_rates[(channel->type_flags & _sound_channel_44khz_bit) >> 2];
    int32_t buffer_size = (((channel->type_flags & _sound_channel_stereo_bit) != 0) + 1) * sample_rate * 6;
    uint8_t start_playback = 0;

    if (crosslap != 0 && source->format != soundformat_ogg_vorbis) {
        crosslap = 0;
    }

    switch (channel->state) {
    case _directsound_channel_queued:
        if (channel->next_source != (SoundPermutation *)0) {
            channel->next_source = source;
            channel->next_source_crosslap = 1;
            sound_channel_stream_update(channel_index, channel->source_crosslap);
            return;
        }
        {
            int32_t play_cursor = (int32_t)sound_channel_refresh_cursor(channel_index);
            int32_t source_end = channel->source_end_cursor;
            int32_t delta = play_cursor - source_end;

            if (delta < 0) {
                delta += buffer_size;
            }
            if (delta < buffer_size - sample_rate / 10) {
                channel->write_cursor = source_end;
                channel->source = source;
                channel->source_crosslap = 1;
                sound_channel_lock_and_fill(channel_index, (uint32_t)delta);
                channel->write_cursor = play_cursor;
                return;
            }
        }
        channel->next_source = source;
        channel->next_source_crosslap = 1;
        sound_channel_stream_update(channel_index, channel->source_crosslap);
        return;

    case _directsound_channel_playing:
        channel->state = _directsound_channel_queued;
        if (channel->source != (SoundPermutation *)0) {
            channel->next_source = source;
            channel->next_source_crosslap = crosslap;
        } else {
            channel->source = source;
            channel->write_cursor = channel->source_end_cursor;
        }
        sound_channel_stream_update(channel_index, crosslap);
        return;

    case _directsound_channel_idle: {
        int32_t fill_size = buffer_size;

        channel->state = _directsound_channel_playing;
        channel->source = source;
        if (channel->streaming != 0) {
            void **vtable = *(void ***)channel->buffer;
            int32_t play_cursor;
            int32_t write_cursor;

            ((directsound_buffer_get_current_position_proc)vtable[0x10 / 4])(channel->buffer, &play_cursor,
                &write_cursor);
            channel->write_cursor = write_cursor;
            channel->source_end_cursor = -1;
            if (play_cursor < write_cursor) {
                fill_size = buffer_size - write_cursor + play_cursor;
            } else if (play_cursor > write_cursor) {
                fill_size = play_cursor - write_cursor;
            } else {
                start_playback = 1;
            }
        } else {
            channel->write_cursor = 0;
            channel->source_end_cursor = -1;
            start_playback = 1;
        }

        channel->source_crosslap = 0;
        channel->sound_class = sound_class;
        channel->streaming_bytes = -1;
        if (sound_channel_lock_and_fill(channel_index, (uint32_t)fill_size) == 0) {
            return;
        }

        ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[0x44 / 4])(directsound_listener);
        directsound_deferred_dirty = 0;
        channel->streaming = 0;
        if (start_playback) {
            void **vtable = *(void ***)channel->buffer;
            uint8_t restored = 0;

            ((directsound_buffer_set_current_position_proc)vtable[0x34 / 4])(channel->buffer, channel->write_cursor);
            if (sound_channel_restore_buffer(channel->buffer, &restored) < 0) {
                return;
            }
            if (restored) {
                sound_channel_lock_and_fill(channel_index, (uint32_t)fill_size);
            }
            vtable = *(void ***)channel->buffer;
            ((directsound_buffer_play_proc)vtable[0x30 / 4])(channel->buffer, 0, 0, 1); // DSBPLAY_LOOPING
        }
        return;
    }

    default:
        return;
    }
}

#if 0
Original Ghidra decompilation (0x547c80):

/* WARNING: Removing unreachable block (ram,0x00547f2a) */

void FUN_00547c80(short param_1,int param_2,undefined2 param_3)

{
  undefined1 uVar1;
  short sVar2;
  char cVar3;
  int iVar4;
  char in_CL;
  int iVar5;
  undefined4 unaff_EBX;
  int iVar6;
  int iVar7;
  int iVar8;
  int local_8;
  int local_4;

  iVar7 = (int)param_1;
  iVar8 = iVar7 * 0x678;
  local_4 = 0;
  local_8 = 0;
  iVar6 = ((((&DAT_00725468)[iVar8] & 2) != 0) + 1) *
          *(int *)(&DAT_0065e4f8 + ((byte)(&DAT_00725468)[iVar8] & 4)) * 6;
  if ((in_CL != '\0') && (*(short *)(param_2 + 0x28) != 3)) {
    in_CL = '\0';
  }
  sVar2 = (&DAT_00725430)[iVar7 * 0x33c];
  if (sVar2 == 0) {
    cVar3 = (&DAT_00725439)[iVar8];
    (&DAT_00725430)[iVar7 * 0x33c] = 1;
    (&DAT_007254b8)[iVar7 * 0x19e] = param_2;
    if (cVar3 == '\0') {
      *(undefined4 *)(&DAT_007254a8 + iVar8) = 0;
      *(undefined4 *)(&DAT_007254ac + iVar8) = 0xffffffff;
    }
    else {
      (**(code **)(*(int *)(&DAT_00725aa0)[iVar7 * 0x19e] + 0x10))
                ((int *)(&DAT_00725aa0)[iVar7 * 0x19e],&local_4,&local_8);
      *(int *)(&DAT_007254a8 + iVar8) = local_8;
      *(undefined4 *)(&DAT_007254ac + iVar8) = 0xffffffff;
      if (local_4 < local_8) {
        iVar6 = (iVar6 - local_8) + local_4;
      }
      else if (local_8 < local_4) {
        iVar6 = local_4 - local_8;
      }
    }
    (&DAT_007254c0)[iVar8] = 0;
    *(undefined2 *)(&DAT_00725434 + iVar8) = param_3;
    *(undefined4 *)(&DAT_007254b4 + iVar8) = 0xffffffff;
    cVar3 = sound_channel_lock_and_fill(iVar6);
    if (cVar3 != '\0') {
      (**(code **)(*DAT_00746114 + 0x44))(DAT_00746114);
      DAT_00746132 = 0;
      (&DAT_00725439)[iVar8] = 0;
      if ((char)((uint)unaff_EBX >> 0x18) != '\0') {
        (**(code **)(*(int *)(&DAT_00725aa0)[iVar7 * 0x19e] + 0x34))
                  ((int *)(&DAT_00725aa0)[iVar7 * 0x19e],*(undefined4 *)(&DAT_007254a8 + iVar8));
        iVar6 = FUN_00547c10();
        if (-1 < iVar6) {
          (**(code **)(*(int *)(&DAT_00725aa0)[iVar7 * 0x19e] + 0x30))
                    ((int *)(&DAT_00725aa0)[iVar7 * 0x19e],0,0,1);
        }
      }
    }
  }
  else {
    if (sVar2 == 1) {
      iVar6 = (&DAT_007254b8)[iVar7 * 0x19e];
      (&DAT_00725430)[iVar7 * 0x33c] = 2;
      if (iVar6 != 0) {
        (&DAT_007254bc)[iVar7 * 0x19e] = param_2;
        (&DAT_007254c1)[iVar8] = in_CL;
        FUN_005478c0(in_CL);
        return;
      }
      (&DAT_007254b8)[iVar7 * 0x19e] = param_2;
      *(undefined4 *)(&DAT_007254a8 + iVar8) = *(undefined4 *)(&DAT_007254ac + iVar8);
      FUN_005478c0(in_CL);
      return;
    }
    if (sVar2 == 2) {
      if ((&DAT_007254bc)[iVar7 * 0x19e] != 0) {
        uVar1 = (&DAT_007254c0)[iVar8];
        (&DAT_007254bc)[iVar7 * 0x19e] = param_2;
        (&DAT_007254c1)[iVar8] = 1;
        FUN_005478c0(uVar1);
        return;
      }
      iVar4 = FUN_00547890();
      iVar5 = iVar4 - *(int *)(&DAT_007254ac + iVar8);
      if (iVar5 < 0) {
        iVar5 = iVar5 + iVar6;
      }
      if (iVar5 < iVar6 - *(int *)(&DAT_0065e4f8 + ((byte)(&DAT_00725468)[iVar8] & 4)) / 10) {
        *(int *)(&DAT_007254a8 + iVar8) = *(int *)(&DAT_007254ac + iVar8);
        (&DAT_007254b8)[iVar7 * 0x19e] = param_2;
        (&DAT_007254c0)[iVar8] = 1;
        sound_channel_lock_and_fill(iVar5);
        *(int *)(&DAT_007254a8 + iVar8) = iVar4;
        return;
      }
      uVar1 = (&DAT_007254c0)[iVar8];
      (&DAT_007254bc)[iVar7 * 0x19e] = param_2;
      (&DAT_007254c1)[iVar8] = 1;
      FUN_005478c0(uVar1);
      return;
    }
  }
  return;
}

Disassembly (0x547c80..0x547f51, capstone; phase-4 review):

0x547c80: sub esp, 0x10
0x547c83: push ebx
0x547c84: push ebp
0x547c85: push esi
0x547c86: push edi
0x547c87: movsx edi, word ptr [esp + 0x24]
0x547c8c: imul edi, edi, 0x678
0x547c92: mov bl, byte ptr [edi + 0x725468]
0x547c98: xor edx, edx
0x547c9a: mov dl, byte ptr [edi + 0x725468]
0x547ca0: xor eax, eax
0x547ca2: mov dword ptr [esp + 0x1c], eax
0x547ca6: mov dword ptr [esp + 0x18], eax
0x547caa: mov byte ptr [esp + 0x13], al
0x547cae: mov byte ptr [esp + 0x14], cl
0x547cb2: and edx, 4
0x547cb5: shr edx, 2
0x547cb8: test bl, 2
0x547cbb: setne al
0x547cbe: inc eax
0x547cbf: imul eax, dword ptr [edx*4 + 0x65e4f8]
0x547cc7: mov edx, dword ptr [esp + 0x28]
0x547ccb: lea esi, [eax + eax*2]
0x547cce: shl esi, 1
0x547cd0: test cl, cl
0x547cd2: je 0x547ce1
0x547cd4: cmp word ptr [edx + 0x28], 3
0x547cd9: je 0x547ce1
0x547cdb: xor cl, cl
0x547cdd: mov byte ptr [esp + 0x14], cl
0x547ce1: movsx eax, word ptr [edi + 0x725430]
0x547ce8: sub eax, 0
0x547ceb: je 0x547e3b
0x547cf1: dec eax
0x547cf2: je 0x547dd8
0x547cf8: dec eax
0x547cf9: jne 0x547f49
0x547cff: mov eax, dword ptr [edi + 0x7254bc]
0x547d05: test eax, eax
0x547d07: je 0x547d33
0x547d09: xor eax, eax
0x547d0b: mov al, byte ptr [edi + 0x7254c0]
0x547d11: mov dword ptr [edi + 0x7254bc], edx
0x547d17: mov byte ptr [edi + 0x7254c1], 1
0x547d1e: push eax
0x547d1f: mov eax, dword ptr [esp + 0x28]
0x547d23: call 0x5478c0
0x547d28: add esp, 4
0x547d2b: pop edi
0x547d2c: pop esi
0x547d2d: pop ebp
0x547d2e: pop ebx
0x547d2f: add esp, 0x10
0x547d32: ret 
0x547d33: mov eax, dword ptr [esp + 0x24]
0x547d37: call 0x547890
0x547d3c: mov ebx, dword ptr [edi + 0x7254ac]
0x547d42: mov ebp, eax
0x547d44: mov ecx, ebp
0x547d46: sub ecx, ebx
0x547d48: jns 0x547d4c
0x547d4a: add ecx, esi
0x547d4c: xor edx, edx
0x547d4e: mov dl, byte ptr [edi + 0x725468]
0x547d54: mov eax, 0x66666667
0x547d59: and edx, 4
0x547d5c: shr edx, 2
0x547d5f: mov edx, dword ptr [edx*4 + 0x65e4f8]
0x547d66: imul edx
0x547d68: sar edx, 2
0x547d6b: mov eax, edx
0x547d6d: shr eax, 0x1f
0x547d70: add eax, edx
0x547d72: sub esi, eax
0x547d74: cmp ecx, esi
0x547d76: jge 0x547daa
0x547d78: mov edx, dword ptr [esp + 0x28]
0x547d7c: mov dword ptr [edi + 0x7254a8], ebx
0x547d82: mov ebx, dword ptr [esp + 0x24]
0x547d86: push ecx
0x547d87: mov dword ptr [edi + 0x7254b8], edx
0x547d8d: mov byte ptr [edi + 0x7254c0], 1
0x547d94: call 0x547a00
0x547d99: add esp, 4
0x547d9c: mov dword ptr [edi + 0x7254a8], ebp
0x547da2: pop edi
0x547da3: pop esi
0x547da4: pop ebp
0x547da5: pop ebx
0x547da6: add esp, 0x10
0x547da9: ret 
0x547daa: mov eax, dword ptr [esp + 0x28]
0x547dae: xor ecx, ecx
0x547db0: mov cl, byte ptr [edi + 0x7254c0]
0x547db6: mov dword ptr [edi + 0x7254bc], eax
0x547dbc: mov eax, dword ptr [esp + 0x24]
0x547dc0: mov byte ptr [edi + 0x7254c1], 1
0x547dc7: push ecx
0x547dc8: call 0x5478c0
0x547dcd: add esp, 4
0x547dd0: pop edi
0x547dd1: pop esi
0x547dd2: pop ebp
0x547dd3: pop ebx
0x547dd4: add esp, 0x10
0x547dd7: ret 
0x547dd8: mov eax, dword ptr [edi + 0x7254b8]
0x547dde: test eax, eax
0x547de0: mov word ptr [edi + 0x725430], 2
0x547de9: je 0x547e10
0x547deb: mov eax, dword ptr [esp + 0x14]
0x547def: push eax
0x547df0: mov eax, dword ptr [esp + 0x28]
0x547df4: mov dword ptr [edi + 0x7254bc], edx
0x547dfa: mov byte ptr [edi + 0x7254c1], cl
0x547e00: call 0x5478c0
0x547e05: add esp, 4
0x547e08: pop edi
0x547e09: pop esi
0x547e0a: pop ebp
0x547e0b: pop ebx
0x547e0c: add esp, 0x10
0x547e0f: ret 
0x547e10: mov eax, dword ptr [esp + 0x14]
0x547e14: mov dword ptr [edi + 0x7254b8], edx
0x547e1a: mov edx, dword ptr [edi + 0x7254ac]
0x547e20: push eax
0x547e21: mov eax, dword ptr [esp + 0x28]
0x547e25: mov dword ptr [edi + 0x7254a8], edx
0x547e2b: call 0x5478c0
0x547e30: add esp, 4
0x547e33: pop edi
0x547e34: pop esi
0x547e35: pop ebp
0x547e36: pop ebx
0x547e37: add esp, 0x10
0x547e3a: ret 
0x547e3b: mov al, byte ptr [edi + 0x725439]
0x547e41: test al, al
0x547e43: mov ebx, 1
0x547e48: mov word ptr [edi + 0x725430], bx
0x547e4f: mov dword ptr [edi + 0x7254b8], edx
0x547e55: je 0x547e94
0x547e57: mov eax, dword ptr [edi + 0x725aa0]
0x547e5d: mov ecx, dword ptr [eax]
0x547e5f: lea edx, [esp + 0x18]
0x547e63: push edx
0x547e64: lea edx, [esp + 0x20]
0x547e68: push edx
0x547e69: push eax
0x547e6a: call dword ptr [ecx + 0x10]
0x547e6d: mov eax, dword ptr [esp + 0x18]
0x547e71: mov ebp, dword ptr [esp + 0x1c]
0x547e75: or ecx, 0xffffffff
0x547e78: cmp ebp, eax
0x547e7a: mov dword ptr [edi + 0x7254a8], eax
0x547e80: mov dword ptr [edi + 0x7254ac], ecx
0x547e86: jge 0x547e8e
0x547e88: sub esi, eax
0x547e8a: add esi, ebp
0x547e8c: jmp 0x547eab
0x547e8e: jle 0x547ea7
0x547e90: sub ebp, eax
0x547e92: jmp 0x547ead
0x547e94: or ecx, 0xffffffff
0x547e97: mov dword ptr [edi + 0x7254a8], 0
0x547ea1: mov dword ptr [edi + 0x7254ac], ecx
0x547ea7: mov byte ptr [esp + 0x13], bl
0x547eab: mov ebp, esi
0x547ead: mov ax, word ptr [esp + 0x2c]
0x547eb2: mov ebx, dword ptr [esp + 0x24]
0x547eb6: push ebp
0x547eb7: mov byte ptr [edi + 0x7254c0], 0
0x547ebe: mov word ptr [edi + 0x725434], ax
0x547ec5: mov dword ptr [edi + 0x7254b4], ecx
0x547ecb: call 0x547a00
0x547ed0: add esp, 4
0x547ed3: test al, al
0x547ed5: je 0x547f49
0x547ed7: mov eax, dword ptr [0x746114]
0x547edc: mov ecx, dword ptr [eax]
0x547ede: push eax
0x547edf: call dword ptr [ecx + 0x44]
0x547ee2: mov al, byte ptr [esp + 0x13]
0x547ee6: xor bl, bl
0x547ee8: cmp al, bl
0x547eea: mov byte ptr [0x746132], bl
0x547ef0: mov byte ptr [edi + 0x725439], bl
0x547ef6: je 0x547f49
0x547ef8: mov ecx, dword ptr [edi + 0x7254a8]
0x547efe: mov eax, dword ptr [edi + 0x725aa0]
0x547f04: mov edx, dword ptr [eax]
0x547f06: push ecx
0x547f07: push eax
0x547f08: call dword ptr [edx + 0x34]
0x547f0b: mov esi, dword ptr [edi + 0x725aa0]
0x547f11: mov byte ptr [esp + 0x13], bl
0x547f15: lea ebx, [esp + 0x13]
0x547f19: call 0x547c10
0x547f1e: test eax, eax
0x547f20: jl 0x547f49
0x547f22: mov al, byte ptr [esp + 0x13]
0x547f26: test al, al
0x547f28: je 0x547f37
0x547f2a: mov ebx, dword ptr [esp + 0x24]
0x547f2e: push ebp
0x547f2f: call 0x547a00
0x547f34: add esp, 4
0x547f37: mov edi, dword ptr [edi + 0x725aa0]
0x547f3d: mov edx, dword ptr [edi]
0x547f3f: push 1
0x547f41: push 0
0x547f43: push 0
0x547f45: push edi
0x547f46: call dword ptr [edx + 0x30]
0x547f49: pop edi
0x547f4a: pop esi
0x547f4b: pop ebp
0x547f4c: pop ebx
0x547f4d: add esp, 0x10
0x547f50: ret 
#endif
