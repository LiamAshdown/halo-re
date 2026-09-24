// sound_channel_type_flags_match  (Ghidra: FUN_00548520)
// address 0x548520, size 104 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Tests whether a channel's capability flag bits
//   match a requested combination of 3D/stereo/hardware options."; types/sound.h
//   sound_channel_type_flags (_sound_channel_3d_bit 0x01, _sound_channel_stereo_bit 0x02,
//   _sound_channel_44khz_bit 0x04, _sound_channel_compressed_bit 0x08).
// register convention: compressed/stereo/44khz-requested as three recognized stack parameters
//   (param_1, param_2, param_3), flags to test in DX (in_DX), 3D-requested (only checked for
//   non-stereo channels) as the fourth stack parameter (param_4).
// blam-cc: stack -> (compressed_requested, stereo_requested, sample_rate_44khz_requested), DX ->
//   flags, stack -> requested_3d
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

// blam-cc: stack -> (compressed_requested, stereo_requested, sample_rate_44khz_requested), DX ->
// flags, stack -> requested_3d
// True if `flags` (a directsound_channel.type_flags value) has its 44kHz, stereo, and compressed
// bits set exactly as requested, and -- for a non-stereo (mono) channel only -- its 3D bit also
// matches `requested_3d`.
uint8_t sound_channel_type_flags_match(int16_t compressed_requested, int16_t stereo_requested,
    uint16_t sample_rate_44khz_requested, uint16_t flags, int16_t requested_3d)
{
    // the binary compares the 44 kHz bit with the raw argument (Sound.sample_rate, 0 or 1)
    uint8_t matches = (uint16_t)((flags >> 2) & 1) == sample_rate_44khz_requested &&
        ((flags & _sound_channel_stereo_bit) != 0) == (stereo_requested != 0) &&
        ((flags & _sound_channel_compressed_bit) != 0) == (compressed_requested != 0);

    if ((flags & _sound_channel_stereo_bit) == 0 &&
        ((flags & _sound_channel_3d_bit) != 0) != (requested_3d != 0)) {
        matches = 0;
    }

    return matches;
}

#if 0
Original Ghidra decompilation (0x548520):

bool FUN_00548520(short param_1,short param_2,ushort param_3,short param_4)

{
  bool bVar1;
  uint uVar2;
  ushort in_DX;

  uVar2 = (uint)(short)in_DX;
  bVar1 = ((ushort)(uVar2 >> 2) & 1) == param_3 &&
          ((~(uVar2 >> 1) & 1) == (uint)(param_2 == 0) &&
          (~(uVar2 >> 3) & 1) == (uint)(param_1 == 0));
  if (((in_DX & 2) == 0) && ((~uVar2 & 1) != (uint)(param_4 == 0))) {
    bVar1 = false;
  }
  return bVar1;
}

Disassembly (0x548520..0x548588, capstone; phase-4 review):

0x548520: push ebx
0x548521: push esi
0x548522: movsx ecx, dx
0x548525: mov esi, ecx
0x548527: shr esi, 3
0x54852a: xor ebx, ebx
0x54852c: not esi
0x54852e: and esi, 1
0x548531: cmp word ptr [esp + 0xc], bx
0x548536: mov al, 1
0x548538: sete bl
0x54853b: cmp esi, ebx
0x54853d: je 0x548541
0x54853f: xor al, al
0x548541: mov esi, ecx
0x548543: shr esi, 1
0x548545: xor ebx, ebx
0x548547: not esi
0x548549: and esi, 1
0x54854c: cmp word ptr [esp + 0x10], bx
0x548551: sete bl
0x548554: cmp esi, ebx
0x548556: je 0x54855a
0x548558: xor al, al
0x54855a: mov esi, ecx
0x54855c: shr esi, 2
0x54855f: and esi, 1
0x548562: cmp si, word ptr [esp + 0x14]
0x548567: pop esi
0x548568: pop ebx
0x548569: je 0x54856d
0x54856b: xor al, al
0x54856d: test dl, 2
0x548570: jne 0x548587
0x548572: xor edx, edx
0x548574: not ecx
0x548576: and ecx, 1
0x548579: cmp word ptr [esp + 0x10], dx
0x54857e: sete dl
0x548581: cmp ecx, edx
0x548583: je 0x548587
0x548585: xor al, al
0x548587: ret 
#endif
