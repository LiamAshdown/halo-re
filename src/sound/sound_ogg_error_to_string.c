// sound_ogg_error_to_string  (Ghidra: sound_ogg_error_to_string, already named)
// address 0x544f70, size 407 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Formats a libvorbisfile/Ogg Vorbis error code
//   into its corresponding human-readable description string."; the string literals and error
//   code constants match libvorbisfile's OV_* error codes exactly (-1 FALSE, -2 EOF, -3 HOLE,
//   -0x80..-0x8a the OV_E* codes).
// register convention: __cdecl, vorbis_error_code as the recognized stack parameter (Ghidra's own
//   signature already shows this cleanly).
// blam-cc: stack -> vorbis_error_code
// UNSURE: the formatted text is sprintf'd into a large (4092-byte) on-stack buffer that this
//   function never returns, stores globally, or otherwise uses -- Ghidra's own __chkstk/
//   alloca_probe marker confirms a real, large stack allocation existed in the original binary,
//   so this is preserved as a genuine (if outwardly pointless) local buffer rather than
//   reinterpreted as types/sound.h's separately-documented "0x006e35c8 error text buffer" global,
//   which is not clearly evidenced as this function's actual destination.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"
#include <stdio.h>

// blam-cc: stack -> vorbis_error_code
// Formats a libvorbisfile/Ogg Vorbis OV_* error code into its human-readable description.
void sound_ogg_error_to_string(int32_t vorbis_error_code)
{
    char buffer[4092];

    switch (vorbis_error_code) {
    case -0x8a:
        sprintf(buffer, "The given stream is not seekable.");
        return;
    case -0x89:
        sprintf(buffer,
            "The given link exists in the Vorbis data stream, but is not decipherable due to garbacge or corruption.");
        return;
    case -0x88:
        sprintf(buffer, "Bad packet.");
        return;
    case -0x87:
        sprintf(buffer, "Not audio.");
        return;
    case -0x86:
        sprintf(buffer, "The bitstream format revision of the given stream is not supported.");
        return;
    case -0x85:
        sprintf(buffer,
            "The file/data is apparently an Ogg Vorbis stream, but contains a corrupted or undecipherable header.");
        return;
    case -0x84:
        sprintf(buffer, "The given file/data was not recognized as Ogg Vorbis data.");
        return;
    case -0x83:
        sprintf(buffer,
            "Either an invalid argument, or incompletely initialized argument passed to libvorbisfile call.");
        return;
    case -0x82:
        sprintf(buffer, "Feature not implemented.");
        return;
    case -0x81:
        sprintf(buffer, "Internal inconsistency in decode state. Continuing is likely not possible.");
        return;
    case -0x80:
        sprintf(buffer, "Read error while fetching compressed data for decode.");
        return;
    case -3:
        sprintf(buffer,
            "Vorbisfile encoutered missing or corrupt data in the bitstream. Recovery is normally automatic and this return code is for informational purposes only.");
        return;
    case -2:
        sprintf(buffer, "EOF");
        return;
    case -1:
        sprintf(buffer, "Not true, or no data available.");
        return;
    default:
        sprintf(buffer, "Unknown error");
        return;
    }
}

#if 0
Original Ghidra decompilation (0x544f70):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void __cdecl sound_ogg_error_to_string(int vorbis_error_code)

{
  char local_1000 [4092];
  undefined4 uStack_4;

  uStack_4 = 0x544f7a;
  switch(vorbis_error_code) {
  case -0x8a:
    _sprintf(local_1000,"The given stream is not seekable.");
    return;
  case -0x89:
    _sprintf(local_1000,
             "The given link exists in the Vorbis data stream, but is not decipherable due to garbacge or corruption."
            );
    return;
  case -0x88:
    _sprintf(local_1000,"Bad packet.");
    return;
  case -0x87:
    _sprintf(local_1000,"Not audio.");
    return;
  case -0x86:
    _sprintf(local_1000,"The bitstream format revision of the given stream is not supported.");
    return;
  case -0x85:
    _sprintf(local_1000,
             "The file/data is apparently an Ogg Vorbis stream, but contains a corrupted or undecipherable header."
            );
    return;
  case -0x84:
    _sprintf(local_1000,"The given file/data was not recognized as Ogg Vorbis data.");
    return;
  case -0x83:
    _sprintf(local_1000,
             "Either an invalid argument, or incompletely initialized argument passed to libvorbisfile call."
            );
    return;
  case -0x82:
    _sprintf(local_1000,"Feature not implemented.");
    return;
  case -0x81:
    _sprintf(local_1000,"Internal inconsistency in decode state. Continuing is likely not possible."
            );
    return;
  case -0x80:
    _sprintf(local_1000,"Read error while fetching compressed data for decode.");
    return;
  default:
    _sprintf(local_1000,"Unknown error");
    return;
  case -3:
    _sprintf(local_1000,
             "Vorbisfile encoutered missing or corrupt data in the bitstream. Recovery is normally automatic and this return code is for informational purposes only."
            );
    return;
  case -2:
    _sprintf(local_1000,"EOF");
    return;
  case -1:
    _sprintf(local_1000,"Not true, or no data available.");
    return;
  }
}

Disassembly (0x544f70..0x545107, capstone; phase-4 review):

0x544f70: mov eax, 0x1000
0x544f75: call 0x628240
0x544f7a: mov eax, dword ptr [esp + 0x1004]
0x544f81: add eax, 0x8a
0x544f86: cmp eax, 0x89
0x544f8b: ja 0x5450ef
0x544f91: movzx eax, byte ptr [eax + 0x545144]
0x544f98: jmp dword ptr [eax*4 + 0x545108]
0x544f9f: lea ecx, [esp]
0x544fa2: push 0x6716cc
0x544fa7: push ecx
0x544fa8: call 0x623693
0x544fad: add esp, 8
0x544fb0: add esp, 0x1000
0x544fb6: ret 
0x544fb7: lea edx, [esp]
0x544fba: push 0x6716c8
0x544fbf: push edx
0x544fc0: call 0x623693
0x544fc5: add esp, 8
0x544fc8: add esp, 0x1000
0x544fce: ret 
0x544fcf: lea eax, [esp]
0x544fd2: push 0x671630
0x544fd7: push eax
0x544fd8: call 0x623693
0x544fdd: add esp, 8
0x544fe0: add esp, 0x1000
0x544fe6: ret 
0x544fe7: lea ecx, [esp]
0x544fea: push 0x6715f4
0x544fef: push ecx
0x544ff0: call 0x623693
0x544ff5: add esp, 8
0x544ff8: add esp, 0x1000
0x544ffe: ret 
0x544fff: lea edx, [esp]
0x545002: push 0x6715a8
0x545007: push edx
0x545008: call 0x623693
0x54500d: add esp, 8
0x545010: add esp, 0x1000
0x545016: ret 
0x545017: lea eax, [esp]
0x54501a: push 0x671588
0x54501f: push eax
0x545020: call 0x623693
0x545025: add esp, 8
0x545028: add esp, 0x1000
0x54502e: ret 
0x54502f: lea ecx, [esp]
0x545032: push 0x671528
0x545037: push ecx
0x545038: call 0x623693
0x54503d: add esp, 8
0x545040: add esp, 0x1000
0x545046: ret 
0x545047: lea edx, [esp]
0x54504a: push 0x6714e8
0x54504f: push edx
0x545050: call 0x623693
0x545055: add esp, 8
0x545058: add esp, 0x1000
0x54505e: ret 
0x54505f: lea eax, [esp]
0x545062: push 0x671480
0x545067: push eax
0x545068: call 0x623693
0x54506d: add esp, 8
0x545070: add esp, 0x1000
0x545076: ret 
0x545077: lea ecx, [esp]
0x54507a: push 0x671438
0x54507f: push ecx
0x545080: call 0x623693
0x545085: add esp, 8
0x545088: add esp, 0x1000
0x54508e: ret 
0x54508f: lea edx, [esp]
0x545092: push 0x67142c
0x545097: push edx
0x545098: call 0x623693
0x54509d: add esp, 8
0x5450a0: add esp, 0x1000
0x5450a6: ret 
0x5450a7: lea eax, [esp]
0x5450aa: push 0x671420
0x5450af: push eax
0x5450b0: call 0x623693
0x5450b5: add esp, 8
0x5450b8: add esp, 0x1000
0x5450be: ret 
0x5450bf: lea ecx, [esp]
0x5450c2: push 0x6713b8
0x5450c7: push ecx
0x5450c8: call 0x623693
0x5450cd: add esp, 8
0x5450d0: add esp, 0x1000
0x5450d6: ret 
0x5450d7: lea edx, [esp]
0x5450da: push 0x671390
0x5450df: push edx
0x5450e0: call 0x623693
0x5450e5: add esp, 8
0x5450e8: add esp, 0x1000
0x5450ee: ret 
0x5450ef: lea eax, [esp]
0x5450f2: push 0x671380
0x5450f7: push eax
0x5450f8: call 0x623693
0x5450fd: add esp, 8
0x545100: add esp, 0x1000
0x545106: ret 
#endif
