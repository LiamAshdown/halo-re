// sound_ogg_stream_read  (Ghidra: sound_ogg_stream_read, already named)
// address 0x5451d0, size 340 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Decodes the requested number of PCM bytes from
//   the active Ogg Vorbis stream, cross-lapping into the other buffer when switching streams.";
//   types/sound.h sound_stream_decoder (decoded_bytes 0x004, ogg_vorbis_file[2][0x2d0] at 0x008,
//   active_file 0x5a9) accounts for every offset; the slot-selection formula (active_file == 0 ->
//   index 1, else -> index 0) matches src/sound/sound_ogg_stream_open.c's own selection exactly.
//   ov_read/ov_clear/ov_crosslap are standard libvorbisfile entry points.
// register convention: sound_stream_decoder* in EDI (unaff_EDI); destination buffer, byte count,
//   and crosslap-request flag pointer as the three stack parameters Ghidra recognizes directly
//   (param_1, param_2, param_3).
// blam-cc: EDI -> decoder, stack -> (buffer, size, want_crosslap)
// UNSURE: *param_3 (want_crosslap) is read but never written by this function, so it is a genuine
//   caller-supplied flag; its exact meaning (e.g. "a loop boundary was just crossed") is not
//   confirmed beyond what triggers the one-time ov_crosslap/ov_clear pair on the first loop
//   iteration.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "vorbisfile.h"
#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"
#include <stdio.h>


// blam-cc: EDI -> decoder, stack -> (buffer, size, want_crosslap)
// Decodes up to `size` bytes of PCM from the decoder's currently active Ogg Vorbis file into
// `buffer`, performing a one-time cross-lap from the other slot into this one first when
// `*want_crosslap` is set. Returns the number of bytes actually decoded (which may be less than
// `size` on EOF or a read error).
int32_t sound_ogg_stream_read(sound_stream_decoder *decoder, char *buffer, int32_t size, char *want_crosslap)
{
    int32_t total = 0;
    uint8_t crosslapped = 0;
    int32_t bitstream_index;
    void *vorbis_file;
    int32_t read_result;

    for (;;) {
        if (*want_crosslap != 0 && crosslapped == 0) {
            void *current = (decoder->active_file == 0) ? decoder->ogg_vorbis_file[1] : decoder->ogg_vorbis_file[0];
            void *other = (decoder->active_file == 0) ? decoder->ogg_vorbis_file[0] : decoder->ogg_vorbis_file[1];
            int32_t crosslap_result = ov_crosslap(other, current);

            ov_clear(other);
            if (crosslap_result < 0) {
                sound_ogg_error_to_string(crosslap_result);
            }
            crosslapped = 1;
        }

        vorbis_file = (decoder->active_file == 0) ? decoder->ogg_vorbis_file[1] : decoder->ogg_vorbis_file[0];

        read_result = ov_read(vorbis_file, buffer + total, size, 0, 2, 1, &bitstream_index);
        if (read_result < 1) {
            break;
        }

        size -= read_result;
        total += read_result;
        if (size == 0) {
            decoder->decoded_bytes += total;
            return total;
        }
    }

    if (read_result < 0 && read_result != -3) {
        char message[1024];
        sprintf(message, "ov_read failed trying to read %d bytes", size);
        sound_ogg_error_to_string(read_result);
    }

    decoder->decoded_bytes += total;
    return total;
}

#if 0
Original Ghidra decompilation (0x5451d0):

int sound_ogg_stream_read(int param_1,int param_2,char *param_3)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int unaff_EDI;
  undefined1 auStack_408 [4];
  int iStack_404;
  char acStack_400 [1024];

  iVar3 = 0;
  bVar1 = false;
  while( true ) {
    if (*param_3 == '\0') {
      if (*(char *)(unaff_EDI + 0x5a9) == '\0') {
        iVar2 = unaff_EDI + 0x2d8;
      }
      else {
        iVar2 = unaff_EDI + 8;
      }
    }
    else {
      if (!bVar1) {
        if (*(char *)(unaff_EDI + 0x5a9) == '\0') {
          iVar2 = unaff_EDI + 0x2d8;
          iVar4 = unaff_EDI + 8;
        }
        else {
          iVar2 = unaff_EDI + 8;
          iVar4 = unaff_EDI + 0x2d8;
        }
        iStack_404 = ov_crosslap(iVar4,iVar2);
        ov_clear(iVar4);
        if (iStack_404 < 0) {
          sound_ogg_error_to_string(iStack_404);
        }
        bVar1 = true;
      }
      if (*(char *)(unaff_EDI + 0x5a9) == '\0') {
        iVar2 = unaff_EDI + 0x2d8;
      }
      else {
        iVar2 = unaff_EDI + 8;
      }
    }
    iVar2 = ov_read(iVar2,iVar3 + param_1,param_2,0,2,1,auStack_408);
    if (iVar2 < 1) break;
    param_2 = param_2 - iVar2;
    iVar3 = iVar3 + iVar2;
    if (param_2 == 0) {
      *(int *)(unaff_EDI + 4) = *(int *)(unaff_EDI + 4) + iVar3;
      return iVar3;
    }
  }
  if ((iVar2 < 0) && (iVar2 != -3)) {
    _sprintf(acStack_400,"ov_read failed trying to read %d bytes",param_2);
    sound_ogg_error_to_string(iVar2);
  }
  *(int *)(unaff_EDI + 4) = *(int *)(unaff_EDI + 4) + iVar3;
  return iVar3;
}

Disassembly (0x5451d0..0x545324, capstone; phase-4 review):

0x5451d0: sub esp, 0x40c
0x5451d6: push ebx
0x5451d7: push ebp
0x5451d8: mov ebp, dword ptr [esp + 0x41c]
0x5451df: xor ebx, ebx
0x5451e1: push esi
0x5451e2: mov byte ptr [esp + 0xf], bl
0x5451e6: jmp 0x5451f0
0x5451e8: lea esp, [esp]
0x5451ef: nop 
0x5451f0: mov eax, dword ptr [esp + 0x424]
0x5451f7: cmp byte ptr [eax], 0
0x5451fa: je 0x545285
0x545200: mov al, byte ptr [esp + 0xf]
0x545204: test al, al
0x545206: jne 0x545251
0x545208: mov al, byte ptr [edi + 0x5a9]
0x54520e: test al, al
0x545210: je 0x54521e
0x545212: lea ecx, [edi + 8]
0x545215: lea esi, [edi + 0x2d8]
0x54521b: push ecx
0x54521c: jmp 0x545228
0x54521e: lea edx, [edi + 0x2d8]
0x545224: lea esi, [edi + 8]
0x545227: push edx
0x545228: push esi
0x545229: call 0x614520
0x54522e: push esi
0x54522f: mov dword ptr [esp + 0x20], eax
0x545233: call 0x614510
0x545238: mov eax, dword ptr [esp + 0x20]
0x54523c: add esp, 0xc
0x54523f: test eax, eax
0x545241: jge 0x54524c
0x545243: push eax
0x545244: call 0x544f70
0x545249: add esp, 4
0x54524c: mov byte ptr [esp + 0xf], 1
0x545251: mov al, byte ptr [edi + 0x5a9]
0x545257: test al, al
0x545259: je 0x545265
0x54525b: lea eax, [esp + 0x10]
0x54525f: push eax
0x545260: lea eax, [edi + 8]
0x545263: jmp 0x5452b7
0x545265: mov edx, dword ptr [esp + 0x41c]
0x54526c: lea ecx, [esp + 0x10]
0x545270: push ecx
0x545271: push 1
0x545273: push 2
0x545275: push 0
0x545277: push ebp
0x545278: lea eax, [ebx + edx]
0x54527b: push eax
0x54527c: lea ecx, [edi + 0x2d8]
0x545282: push ecx
0x545283: jmp 0x5452ca
0x545285: mov al, byte ptr [edi + 0x5a9]
0x54528b: test al, al
0x54528d: je 0x5452ac
0x54528f: mov eax, dword ptr [esp + 0x41c]
0x545296: lea edx, [esp + 0x10]
0x54529a: push edx
0x54529b: push 1
0x54529d: push 2
0x54529f: push 0
0x5452a1: push ebp
0x5452a2: lea ecx, [ebx + eax]
0x5452a5: push ecx
0x5452a6: lea edx, [edi + 8]
0x5452a9: push edx
0x5452aa: jmp 0x5452ca
0x5452ac: lea eax, [esp + 0x10]
0x5452b0: push eax
0x5452b1: lea eax, [edi + 0x2d8]
0x5452b7: mov ecx, dword ptr [esp + 0x420]
0x5452be: push 1
0x5452c0: push 2
0x5452c2: push 0
0x5452c4: push ebp
0x5452c5: lea edx, [ebx + ecx]
0x5452c8: push edx
0x5452c9: push eax
0x5452ca: call 0x6144f0
0x5452cf: mov esi, eax
0x5452d1: add esp, 0x1c
0x5452d4: test esi, esi
0x5452d6: jle 0x5452f8
0x5452d8: sub ebp, esi
0x5452da: add ebx, esi
0x5452dc: test ebp, ebp
0x5452de: jne 0x5451f0
0x5452e4: mov eax, dword ptr [edi + 4]
0x5452e7: add eax, ebx
0x5452e9: pop esi
0x5452ea: mov dword ptr [edi + 4], eax
0x5452ed: pop ebp
0x5452ee: mov eax, ebx
0x5452f0: pop ebx
0x5452f1: add esp, 0x40c
0x5452f7: ret 
0x5452f8: jge 0x545318
0x5452fa: cmp esi, -3
0x5452fd: je 0x545318
0x5452ff: push ebp
0x545300: lea ecx, [esp + 0x1c]
0x545304: push 0x671358
0x545309: push ecx
0x54530a: call 0x623693
0x54530f: push esi
0x545310: call 0x544f70
0x545315: add esp, 0x10
0x545318: mov eax, dword ptr [edi + 4]
0x54531b: add eax, ebx
0x54531d: pop esi
0x54531e: mov dword ptr [edi + 4], eax
0x545321: pop ebp
0x545322: mov eax, ebx
#endif
