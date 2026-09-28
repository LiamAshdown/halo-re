// sound_ogg_stream_open  (Ghidra: sound_ogg_stream_open, already named)
// address 0x544eb0, size 192 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Opens an Ogg Vorbis decode stream over a
//   memory buffer, choosing between two double-buffered stream slots for gapless loop
//   transitions."; types/sound.h sound_stream_decoder (decoded_bytes 0x004, ogg_vorbis_file
//   [2][0x2d0] at 0x008, active_file 0x5a9, memory_files[2] at 0x5ac) accounts for every offset
//   used: when active_file == 0, this opens the *other* slot (index 1, offsets 0x5bc/0x2d8) so
//   the currently-playing slot 0 is left untouched; otherwise it opens slot 0 (0x5ac/0x008).
// register convention: sound_stream_decoder* in ESI (unaff_ESI); data pointer and size as the two
//   stack parameters Ghidra recognizes directly (param_1, param_2).
// blam-cc: ESI -> decoder, stack -> (data, size)
// UNSURE: the four LAB_005449d50/da0/dc0/de0 addresses are small local callback trampolines
//   embedded in this function's own code range (not in the 134-function list, the same situation
//   types/sound.h documents for the sound_driver wrapper addresses); declared here only as
//   address-tagged function pointers in libvorbisfile's standard ov_callbacks order
//   (read/seek/close/tell) -- not independently confirmed against a disassembly.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "vorbisfile.h"
#include "tags.h"
#include "memory.h"
#include "sound.h"

extern uint32_t ov_read_thunk(void *destination, uint32_t size, uint32_t count, sound_ogg_memory_file *file); // 0x544d50
extern int32_t ov_seek_thunk(sound_ogg_memory_file *file, uint32_t offset_low, int32_t offset_high, int32_t whence); // 0x544da0
extern int32_t ov_close_thunk(sound_ogg_memory_file *file); // 0x544dc0
extern int32_t ov_tell_thunk(sound_ogg_memory_file *file); // 0x544de0


// blam-cc: ESI -> decoder, stack -> (data, size)
// Opens an Ogg Vorbis decode stream over an in-memory buffer, using whichever of the decoder's
// two double-buffered slots is not currently the active one (so the playing slot survives a
// gapless loop transition). Returns 1 on success, 0 on failure.
uint8_t sound_ogg_stream_open(sound_stream_decoder *decoder, void *data, int32_t size)
{
    sound_ogg_memory_file *memory_file;
    void *ogg_vorbis_file;
    int32_t result;

    if (decoder->active_file == 0) {
        memory_file = &decoder->memory_files[1];
        ogg_vorbis_file = decoder->ogg_vorbis_file[1];
    } else {
        memory_file = &decoder->memory_files[0];
        ogg_vorbis_file = decoder->ogg_vorbis_file[0];
    }

    memory_file->position = 0;
    memory_file->end_of_file = 0;
    memory_file->data = data;
    memory_file->size = size;

    decoder->decoded_bytes = 0;

    result = ov_open_callbacks(memory_file, ogg_vorbis_file, (char *)0, 0,
        (void *)ov_read_thunk, (void *)ov_seek_thunk, (void *)ov_close_thunk, (void *)ov_tell_thunk);

    if (result < 0) {
        return 0;
    }

    decoder->open = 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x544eb0):

undefined4 sound_ogg_stream_open(undefined4 param_1,undefined4 param_2)

{
  undefined4 *puVar1;
  int iVar2;
  int unaff_ESI;

  if (*(char *)(unaff_ESI + 0x5a9) == '\0') {
    puVar1 = (undefined4 *)(unaff_ESI + 0x5bc);
    *puVar1 = 0;
    *(undefined4 *)(unaff_ESI + 0x5c0) = 0;
    *(undefined1 *)(unaff_ESI + 0x5c8) = 0;
    *(undefined4 *)(unaff_ESI + 0x5c4) = 0;
    *(undefined4 *)(unaff_ESI + 0x5c0) = param_1;
    *(undefined4 *)(unaff_ESI + 0x5c4) = param_2;
    iVar2 = unaff_ESI + 0x2d8;
  }
  else {
    puVar1 = (undefined4 *)(unaff_ESI + 0x5ac);
    *puVar1 = 0;
    *(undefined4 *)(unaff_ESI + 0x5b0) = 0;
    *(undefined1 *)(unaff_ESI + 0x5b8) = 0;
    *(undefined4 *)(unaff_ESI + 0x5b4) = 0;
    *(undefined4 *)(unaff_ESI + 0x5b0) = param_1;
    *(undefined4 *)(unaff_ESI + 0x5b4) = param_2;
    iVar2 = unaff_ESI + 8;
  }
  *(undefined4 *)(unaff_ESI + 4) = 0;
  iVar2 = ov_open_callbacks(puVar1,iVar2,0,0,&LAB_00544d50,&LAB_00544da0,&LAB_00544dc0,&LAB_00544de0
                           );
  if (iVar2 < 0) {
    return 0;
  }
  *(undefined1 *)(unaff_ESI + 0x5a8) = 1;
  return 1;
}

Disassembly (0x544eb0..0x544f70, capstone; phase-4 review):

0x544eb0: mov al, byte ptr [esi + 0x5a9]
0x544eb6: push ebx
0x544eb7: push ebp
0x544eb8: mov ebp, dword ptr [esp + 0xc]
0x544ebc: push edi
0x544ebd: xor ebx, ebx
0x544ebf: sub esp, 0x10
0x544ec2: cmp al, bl
0x544ec4: mov ecx, 0x544d50
0x544ec9: mov edx, 0x544da0
0x544ece: mov edi, 0x544dc0
0x544ed3: je 0x544f11
0x544ed5: lea eax, [esi + 0x5ac]
0x544edb: mov dword ptr [eax], ebx
0x544edd: mov dword ptr [eax + 4], ebx
0x544ee0: mov byte ptr [eax + 0xc], bl
0x544ee3: mov dword ptr [eax + 8], ebx
0x544ee6: mov dword ptr [esi + 0x5b0], ebp
0x544eec: mov ebp, dword ptr [esp + 0x24]
0x544ef0: mov dword ptr [esi + 0x5b4], ebp
0x544ef6: mov ebp, esp
0x544ef8: mov dword ptr [ebp], ecx
0x544efb: mov dword ptr [ebp + 4], edx
0x544efe: mov ecx, 0x544de0
0x544f03: mov dword ptr [ebp + 8], edi
0x544f06: push ebx
0x544f07: mov dword ptr [ebp + 0xc], ecx
0x544f0a: push ebx
0x544f0b: lea ecx, [esi + 8]
0x544f0e: push ecx
0x544f0f: jmp 0x544f4e
0x544f11: lea eax, [esi + 0x5bc]
0x544f17: mov dword ptr [eax], ebx
0x544f19: mov dword ptr [eax + 4], ebx
0x544f1c: mov byte ptr [eax + 0xc], bl
0x544f1f: mov dword ptr [eax + 8], ebx
0x544f22: mov dword ptr [esi + 0x5c0], ebp
0x544f28: mov ebp, dword ptr [esp + 0x24]
0x544f2c: mov dword ptr [esi + 0x5c4], ebp
0x544f32: mov ebp, esp
0x544f34: mov dword ptr [ebp], ecx
0x544f37: mov dword ptr [ebp + 4], edx
0x544f3a: push ebx
0x544f3b: mov dword ptr [ebp + 8], edi
0x544f3e: mov ecx, 0x544de0
0x544f43: push ebx
0x544f44: lea edx, [esi + 0x2d8]
0x544f4a: mov dword ptr [ebp + 0xc], ecx
0x544f4d: push edx
0x544f4e: push eax
0x544f4f: mov dword ptr [esi + 4], ebx
0x544f52: call 0x614500
0x544f57: add esp, 0x20
0x544f5a: cmp eax, ebx
0x544f5c: jge 0x544f64
0x544f5e: xor al, al
0x544f60: pop edi
0x544f61: pop ebp
0x544f62: pop ebx
0x544f63: ret 
0x544f64: pop edi
0x544f65: mov al, 1
0x544f67: pop ebp
0x544f68: mov byte ptr [esi + 0x5a8], al
0x544f6e: pop ebx
0x544f6f: ret 
#endif
