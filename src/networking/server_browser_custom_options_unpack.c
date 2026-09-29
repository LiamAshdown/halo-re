// server_browser_custom_options_unpack  (Ghidra: FUN_005764a0; named per this rewrite)
// address 0x5764a0, size 680 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary: "Parses a \"%d,%d\" text string and
// unpacks the bitfields back into a game variant options struct, mirroring
// game_variant_custom_options_encode." Exact bit-for-bit mirror of
// server_browser_custom_options_pack.c (0x576180); every case there has a matching case here,
// which is the main evidence the struct guess in that file is at least internally consistent.
// register convention: source text in EDX (in_EDX), destination struct in ESI (unaff_ESI).
// blam-cc: EDX -> text, ESI -> out
// UNSURE: same field-meaning caveats as server_browser_custom_options_pack.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"



extern int32_t sscanf(const char *buffer, const char *format, ...);

// blam-cc: EDX -> text, ESI -> out
// Parses a "%d,%d" text string and unpacks the bitfields back into a custom game-options struct,
// mirroring server_browser_custom_options_pack field for field and bit for bit.
void server_browser_custom_options_unpack(char *text, server_browser_custom_options *out)
{
    uint32_t low;
    uint32_t high;

    sscanf(text, "%d,%d", &low, &high);

    switch (low & 3) {
    case 1: out->gametype_like = 1; break;
    case 2: out->gametype_like = 3; break;
    case 3: out->gametype_like = 5; break;
    default: out->gametype_like = 0; break;
    }

    switch ((low >> 2) & 7) {
    case 0: out->float_bits_20 = 0x3f000000; break;
    case 2: out->float_bits_20 = 0x3fc00000; break;
    case 3: out->float_bits_20 = 0x40000000; break;
    case 4: out->float_bits_20 = 0x40400000; break;
    case 5: out->float_bits_20 = 0x40800000; break;
    default: out->float_bits_20 = 0x3f800000; break;
    }

    if ((low & 0x20) == 0) {
        out->flags_a = out->flags_a & 0xfffffff7;
    } else {
        out->flags_a = out->flags_a | 8;
    }

    switch ((low >> 6) & 3) {
    case 1: out->respawn_time = 0x96; break;
    case 2: out->respawn_time = 300; break;
    case 3: out->respawn_time = 0x1c2; break;
    default: out->respawn_time = 0; break;
    }

    switch ((low >> 8) & 3) {
    case 1: out->respawn_time_growth = 0x96; break;
    case 2: out->respawn_time_growth = 300; break;
    case 3: out->respawn_time_growth = 0x1c2; break;
    default: out->respawn_time_growth = 0; break;
    }

    out->odd_man_out = (uint8_t)((low >> 10) & 1);

    if ((low & 0x800) == 0) {
        out->flags_a = out->flags_a & 0xffffffef;
    } else {
        out->flags_a = out->flags_a | 0x10;
    }

    switch ((low >> 0xc) & 3) {
    case 1: out->suicide_penalty = 0x96; break;
    case 2: out->suicide_penalty = 300; break;
    case 3: out->suicide_penalty = 0x1c2; break;
    default: out->suicide_penalty = 0; break;
    }

    if ((low & 0x4000) == 0) {
        out->flags_a = out->flags_a & 0xfffffffb;
    } else {
        out->flags_a = out->flags_a | 4;
    }

    {
        uint32_t nibble = (low >> 0xf) & 0xf;
        out->starting_equipment = (nibble < 0xe) ? nibble : 0;
    }

    if ((low & 0x80000) == 0) {
        out->flags_a = out->flags_a & 0xffffffdf;
    } else {
        out->flags_a = out->flags_a | 0x20;
    }

    {
        uint32_t two_bits = (low >> 0x14) & 3;
        out->objective_indicator = (two_bits <= 2) ? two_bits : 0;
    }

    if ((low & 0x400000) == 0) {
        out->flags_a = out->flags_a & 0xfffffffe;
    } else {
        out->flags_a = out->flags_a | 1;
    }
    if ((low & 0x800000) == 0) {
        out->flags_a = out->flags_a & 0xffffffbf;
    } else {
        out->flags_a = out->flags_a | 0x40;
    }
    if ((low & 0x1000000) == 0) {
        out->flags_a = out->flags_a & 0xfffffffd;
    } else {
        out->flags_a = out->flags_a | 2;
    }

    {
        uint8_t two_bits = (uint8_t)((low >> 0x19) & 3);
        out->friendly_fire_mode = (two_bits < 4) ? two_bits : 0;
    }

    switch ((low >> 0x1b) & 3) {
    case 1: out->friendly_fire_penalty = 0x96; break;
    case 2: out->friendly_fire_penalty = 300; break;
    case 3: out->friendly_fire_penalty = 0x1c2; break;
    default: out->friendly_fire_penalty = 0; break;
    }

    out->team_switch_restricted = (low & 0x20000000) == 0x20000000;

    switch (high & 7) {
    case 1: out->time_limit = 900; break;
    case 2: out->time_limit = 0x708; break;
    case 3: out->time_limit = 0xa8c; break;
    case 4: out->time_limit = 0xe10; break;
    case 5: out->time_limit = 0x1518; break;
    case 6: out->time_limit = 9000; break;
    default: out->time_limit = 0; break;
    }

    {
        uint32_t nibble1 = (high >> 3) & 0xf;
        uint32_t nibble2 = (high >> 7) & 0xf;
        out->vehicle_set = (nibble1 < 9) ? nibble1 : 0;
        out->alternate_vehicle_set = (nibble2 < 9) ? nibble2 : 0;
    }
}

#if 0
Original Ghidra decompilation (0x5764a0):

void FUN_005764a0(void)

{
  byte bVar1;
  uint uVar2;
  uint uVar3;
  char *in_EDX;
  int unaff_ESI;
  uint local_8;
  uint local_4;

  _sscanf(in_EDX,"%d,%d",&local_8,&local_4);
  switch(local_8 & 3) {
  default:
    *(undefined4 *)(unaff_ESI + 0x1c) = 0;
    break;
  case 1:
    *(undefined4 *)(unaff_ESI + 0x1c) = 1;
    break;
  case 2:
    *(undefined4 *)(unaff_ESI + 0x1c) = 3;
    break;
  case 3:
    *(undefined4 *)(unaff_ESI + 0x1c) = 5;
  }
  switch(local_8 >> 2 & 7) {
  case 0:
    *(undefined4 *)(unaff_ESI + 0x20) = 0x3f000000;
    break;
  default:
    *(undefined4 *)(unaff_ESI + 0x20) = 0x3f800000;
    break;
  case 2:
    *(undefined4 *)(unaff_ESI + 0x20) = 0x3fc00000;
    break;
  case 3:
    *(undefined4 *)(unaff_ESI + 0x20) = 0x40000000;
    break;
  case 4:
    *(undefined4 *)(unaff_ESI + 0x20) = 0x40400000;
    break;
  case 5:
    *(undefined4 *)(unaff_ESI + 0x20) = 0x40800000;
  }
  if ((local_8 & 0x20) == 0) {
    uVar2 = *(uint *)(unaff_ESI + 4) & 0xfffffff7;
  }
  else {
    uVar2 = *(uint *)(unaff_ESI + 4) | 8;
  }
  *(uint *)(unaff_ESI + 4) = uVar2;
  switch(local_8 >> 6 & 3) {
  default:
    *(undefined4 *)(unaff_ESI + 0x14) = 0;
    break;
  case 1:
    *(undefined4 *)(unaff_ESI + 0x14) = 0x96;
    break;
  case 2:
    *(undefined4 *)(unaff_ESI + 0x14) = 300;
    break;
  case 3:
    *(undefined4 *)(unaff_ESI + 0x14) = 0x1c2;
  }
  switch(local_8 >> 8 & 3) {
  default:
    *(undefined4 *)(unaff_ESI + 0x10) = 0;
    break;
  case 1:
    *(undefined4 *)(unaff_ESI + 0x10) = 0x96;
    break;
  case 2:
    *(undefined4 *)(unaff_ESI + 0x10) = 300;
    break;
  case 3:
    *(undefined4 *)(unaff_ESI + 0x10) = 0x1c2;
  }
  *(byte *)(unaff_ESI + 0xc) = (byte)(local_8 >> 10) & 1;
  if ((local_8 & 0x800) == 0) {
    uVar2 = *(uint *)(unaff_ESI + 4) & 0xffffffef;
  }
  else {
    uVar2 = *(uint *)(unaff_ESI + 4) | 0x10;
  }
  *(uint *)(unaff_ESI + 4) = uVar2;
  switch(local_8 >> 0xc & 3) {
  default:
    *(undefined4 *)(unaff_ESI + 0x18) = 0;
    break;
  case 1:
    *(undefined4 *)(unaff_ESI + 0x18) = 0x96;
    break;
  case 2:
    *(undefined4 *)(unaff_ESI + 0x18) = 300;
    break;
  case 3:
    *(undefined4 *)(unaff_ESI + 0x18) = 0x1c2;
  }
  if ((local_8 & 0x4000) == 0) {
    uVar2 = *(uint *)(unaff_ESI + 4) & 0xfffffffb;
  }
  else {
    uVar2 = *(uint *)(unaff_ESI + 4) | 4;
  }
  *(uint *)(unaff_ESI + 4) = uVar2;
  uVar2 = local_8 >> 0xf & 0xf;
  *(uint *)(unaff_ESI + 0x28) = -(uint)(uVar2 < 0xe) & uVar2;
  if ((local_8 & 0x80000) == 0) {
    uVar2 = *(uint *)(unaff_ESI + 4) & 0xffffffdf;
  }
  else {
    uVar2 = *(uint *)(unaff_ESI + 4) | 0x20;
  }
  *(uint *)(unaff_ESI + 4) = uVar2;
  uVar2 = local_8 >> 0x14 & 3;
  *(uint *)(unaff_ESI + 8) = ~-(uint)(2 < uVar2) & uVar2;
  if ((local_8 & 0x400000) == 0) {
    uVar2 = *(uint *)(unaff_ESI + 4) & 0xfffffffe;
  }
  else {
    uVar2 = *(uint *)(unaff_ESI + 4) | 1;
  }
  *(uint *)(unaff_ESI + 4) = uVar2;
  if ((local_8 & 0x800000) == 0) {
    uVar2 = uVar2 & 0xffffffbf;
  }
  else {
    uVar2 = uVar2 | 0x40;
  }
  *(uint *)(unaff_ESI + 4) = uVar2;
  if ((local_8 & 0x1000000) == 0) {
    uVar2 = uVar2 & 0xfffffffd;
  }
  else {
    uVar2 = uVar2 | 2;
  }
  *(uint *)(unaff_ESI + 4) = uVar2;
  bVar1 = (byte)(local_8 >> 0x18);
  if ((bVar1 >> 1 & 3) < 4) {
    *(byte *)(unaff_ESI + 0x38) = bVar1 >> 1 & 3;
  }
  else {
    *(undefined1 *)(unaff_ESI + 0x38) = 0;
  }
  switch(local_8 >> 0x1b & 3) {
  default:
    *(undefined4 *)(unaff_ESI + 0x3c) = 0;
    break;
  case 1:
    *(undefined4 *)(unaff_ESI + 0x3c) = 0x96;
    break;
  case 2:
    *(undefined4 *)(unaff_ESI + 0x3c) = 300;
    break;
  case 3:
    *(undefined4 *)(unaff_ESI + 0x3c) = 0x1c2;
  }
  *(bool *)(unaff_ESI + 0x40) = (local_8 & 0x20000000) == 0x20000000;
  switch(local_4 & 7) {
  default:
    *(undefined4 *)(unaff_ESI + 0x34) = 0;
    break;
  case 1:
    *(undefined4 *)(unaff_ESI + 0x34) = 900;
    break;
  case 2:
    *(undefined4 *)(unaff_ESI + 0x34) = 0x708;
    break;
  case 3:
    *(undefined4 *)(unaff_ESI + 0x34) = 0xa8c;
    break;
  case 4:
    *(undefined4 *)(unaff_ESI + 0x34) = 0xe10;
    break;
  case 5:
    *(undefined4 *)(unaff_ESI + 0x34) = 0x1518;
    break;
  case 6:
    *(undefined4 *)(unaff_ESI + 0x34) = 9000;
  }
  uVar2 = local_4 >> 3 & 0xf;
  uVar3 = local_4 >> 7 & 0xf;
  *(undefined4 *)(unaff_ESI + 0x2c) = 0;
  *(undefined4 *)(unaff_ESI + 0x30) = 0;
  *(uint *)(unaff_ESI + 0x2c) = -(uint)(uVar2 < 9) & uVar2;
  *(uint *)(unaff_ESI + 0x30) = -(uint)(uVar3 < 9) & uVar3;
  return;
}
#endif
