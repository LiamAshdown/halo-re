// server_browser_custom_options_pack  (Ghidra: FUN_00576180; named per this rewrite)
// address 0x576180, size 788 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary: "Packs a custom game variant's
// numeric/boolean options into a compact bitfield pair for textual (\"%d,%d\") serialization."
// The source struct's field offsets (0x00..0x40) do not match types/game.h's game_variant
// (whose first 0x30 bytes are its UTF-16 name), so this operates on a separate, narrower
// "custom options" view -- not attested anywhere else in this module and so not declared in
// types/networking.h as server_browser_custom_options, a flat block of raw offsets (folded in
// from this file by the 2026-09-20 review pass).
// register convention: source struct pointer in EAX (in_EAX, unresolved register read).
// blam-cc: EAX -> options
// UNSURE: every individual field's meaning; only the bit-packing arithmetic itself is
// transcribed exactly. The float comparisons (0x3f000000 == 0.5f, 0x3f800000 == 1.0f, etc.) are
// exact bit patterns, not reconstructed float literals, to avoid any FP-compare rewrite risk.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"



extern char server_browser_custom_options_text[]; // 0x006ef91c, shared "%d,%d" scratch buffer
extern int32_t sprintf(char *buffer, const char *format, ...);

// blam-cc: EAX -> options
// Packs a custom game variant's numeric/boolean options into a compact bitfield pair (low ~28
// bits and a second ~10-bit field) and formats them as "%d,%d" into a shared scratch buffer.
char *server_browser_custom_options_pack(server_browser_custom_options *options)
{
    uint32_t low;
    uint32_t high;
    uint32_t extra;

    low = 0x40000000;
    if (options->gametype_like != 0) {
        if (options->gametype_like == 1) {
            low = 0x40000001;
        } else if (options->gametype_like == 3) {
            low = 0x40000002;
        } else if (options->gametype_like == 5) {
            low = 0x40000003;
        }
    }
    if (options->float_bits_20 != 0x3f000000) {
        if (options->float_bits_20 == 0x3f800000) {
            low = low | 4;
        } else if (options->float_bits_20 == 0x3fc00000) {
            low = low | 8;
        } else if (options->float_bits_20 == 0x40000000) {
            low = low | 0xc;
        } else if (options->float_bits_20 == 0x40400000) {
            low = low | 0x10;
        } else if (options->float_bits_20 == 0x40800000) {
            low = low | 0x14;
        }
    }
    low = low ^ (options->flags_a * 4 & 0x20);
    if (options->respawn_time != 0) {
        if (options->respawn_time == 0x96) {
            low = low | 0x40;
        } else if (options->respawn_time == 300) {
            low = low | 0x80;
        } else if (options->respawn_time == 0x1c2) {
            low = low | 0xc0;
        }
    }
    if (options->respawn_time_growth != 0) {
        if (options->respawn_time_growth == 0x96) {
            low = low | 0x100;
        } else if (options->respawn_time_growth == 300) {
            low = low | 0x200;
        } else if (options->respawn_time_growth == 0x1c2) {
            low = low | 0x300;
        }
    }
    {
        uint32_t bit4 = options->flags_a & 0x10;
        uint32_t bit3 = (uint32_t)(options->odd_man_out != 0) << 3;
        high = (bit3 | bit4) << 7 | low;
        if (options->suicide_penalty == 0) {
            high = (bit3 | bit4) << 7 | low;
        } else if (options->suicide_penalty == 0x96) {
            high = ((bit3 | bit4) << 7 | low) | 0x1000;
        } else if (options->suicide_penalty == 300) {
            high = ((bit3 | bit4) << 7 | low) | 0x2000;
        } else if (options->suicide_penalty == 0x1c2) {
            high = high | 0x3000;
        }
    }
    high = high ^ ((options->flags_a & 4) << 0xc);
    if ((int32_t)options->starting_equipment < 0xe) {
        high = high ^ ((options->starting_equipment & 0xf) << 0xf);
    }
    high = high ^ ((options->flags_a & 0x20) << 0xe);
    if ((int32_t)options->objective_indicator < 3) {
        high = high ^ ((options->objective_indicator & 3) << 0x14);
    }
    high = ((((options->flags_a & 2) << 1 | (options->flags_a & 1)) << 5 | (options->flags_a & 0x40)) << 0x11) | high;
    if (options->friendly_fire_mode < 4) {
        high = high ^ ((uint32_t)(options->friendly_fire_mode & 3) << 0x19);
    }
    if (options->friendly_fire_penalty != 0) {
        if (options->friendly_fire_penalty == 0x96) {
            high = high | 0x8000000;
        } else if (options->friendly_fire_penalty == 300) {
            high = high | 0x10000000;
        } else if (options->friendly_fire_penalty == 0x1c2) {
            high = high | 0x18000000;
        }
    }

    extra = 0;
    if (options->time_limit == 0) {
        extra = 0;
    } else if (options->time_limit == 900) {
        extra = 1;
    } else if (options->time_limit == 0x708) {
        extra = 2;
    } else if (options->time_limit == 0xa8c) {
        extra = 3;
    } else if (options->time_limit == 0xe10) {
        extra = 4;
    } else if (options->time_limit == 0x1518) {
        extra = 5;
    } else if (options->time_limit == 9000) {
        extra = 6;
    }
    {
        uint32_t nibble1 = options->vehicle_set & 0xf;
        if (nibble1 < 9) {
            extra = extra | (nibble1 << 3);
        }
    }
    {
        uint32_t nibble2 = options->alternate_vehicle_set & 0xf;
        if (nibble2 < 9) {
            extra = (nibble2 << 7) | extra;
        }
    }

    sprintf(server_browser_custom_options_text, "%d,%d",
            high ^ ((uint32_t)(options->team_switch_restricted != 0) << 0x1d), extra);
    return server_browser_custom_options_text;
}

#if 0
Original Ghidra decompilation (0x576180):

undefined * FUN_00576180(void)

{
  int iVar1;
  uint uVar2;
  int in_EAX;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;

  iVar1 = *(int *)(in_EAX + 0x1c);
  uVar7 = 0;
  uVar3 = 0x40000000;
  if (iVar1 != 0) {
    if (iVar1 == 1) {
      uVar3 = 0x40000001;
    }
    else if (iVar1 == 3) {
      uVar3 = 0x40000002;
    }
    else if (iVar1 == 5) {
      uVar3 = 0x40000003;
    }
  }
  if (*(int *)(in_EAX + 0x20) != 0x3f000000) {
    if (*(int *)(in_EAX + 0x20) == 0x3f800000) {
      uVar3 = uVar3 | 4;
    }
    else if (*(int *)(in_EAX + 0x20) == 0x3fc00000) {
      uVar3 = uVar3 | 8;
    }
    else if (*(int *)(in_EAX + 0x20) == 0x40000000) {
      uVar3 = uVar3 | 0xc;
    }
    else if (*(int *)(in_EAX + 0x20) == 0x40400000) {
      uVar3 = uVar3 | 0x10;
    }
    else if (*(int *)(in_EAX + 0x20) == 0x40800000) {
      uVar3 = uVar3 | 0x14;
    }
  }
  uVar2 = *(uint *)(in_EAX + 4);
  uVar3 = uVar3 ^ uVar2 * 4 & 0x20;
  iVar1 = *(int *)(in_EAX + 0x14);
  if (iVar1 != 0) {
    if (iVar1 == 0x96) {
      uVar3 = uVar3 | 0x40;
    }
    else if (iVar1 == 300) {
      uVar3 = uVar3 | 0x80;
    }
    else if (iVar1 == 0x1c2) {
      uVar3 = uVar3 | 0xc0;
    }
  }
  iVar1 = *(int *)(in_EAX + 0x10);
  if (iVar1 != 0) {
    if (iVar1 == 0x96) {
      uVar3 = uVar3 | 0x100;
    }
    else if (iVar1 == 300) {
      uVar3 = uVar3 | 0x200;
    }
    else if (iVar1 == 0x1c2) {
      uVar3 = uVar3 | 0x300;
    }
  }
  uVar6 = uVar2 & 0x10;
  uVar4 = (uint)(*(char *)(in_EAX + 0xc) != '\0') << 3;
  uVar5 = (uVar4 | uVar6) << 7 | uVar3;
  iVar1 = *(int *)(in_EAX + 0x18);
  if (iVar1 == 0) {
    uVar5 = (uVar4 | uVar6) << 7 | uVar3;
  }
  else if (iVar1 == 0x96) {
    uVar5 = (uVar4 | uVar6) << 7 | uVar3 | 0x1000;
  }
  else if (iVar1 == 300) {
    uVar5 = (uVar4 | uVar6) << 7 | uVar3 | 0x2000;
  }
  else if (iVar1 == 0x1c2) {
    uVar5 = uVar5 | 0x3000;
  }
  uVar5 = uVar5 ^ (uVar2 & 4) << 0xc;
  if ((int)*(uint *)(in_EAX + 0x28) < 0xe) {
    uVar5 = uVar5 ^ (*(uint *)(in_EAX + 0x28) & 0xf) << 0xf;
  }
  uVar5 = uVar5 ^ (uVar2 & 0x20) << 0xe;
  if ((int)*(uint *)(in_EAX + 8) < 3) {
    uVar5 = uVar5 ^ (*(uint *)(in_EAX + 8) & 3) << 0x14;
  }
  uVar5 = (((uVar2 & 2) << 1 | uVar2 & 1) << 5 | uVar2 & 0x40) << 0x11 | uVar5;
  if (*(byte *)(in_EAX + 0x38) < 4) {
    uVar5 = uVar5 ^ (*(byte *)(in_EAX + 0x38) & 3) << 0x19;
  }
  iVar1 = *(int *)(in_EAX + 0x3c);
  if (iVar1 != 0) {
    if (iVar1 == 0x96) {
      uVar5 = uVar5 | 0x8000000;
    }
    else if (iVar1 == 300) {
      uVar5 = uVar5 | 0x10000000;
    }
    else if (iVar1 == 0x1c2) {
      uVar5 = uVar5 | 0x18000000;
    }
  }
  iVar1 = *(int *)(in_EAX + 0x34);
  if (iVar1 == 0) {
    uVar7 = 0;
  }
  else if (iVar1 == 900) {
    uVar7 = 1;
  }
  else if (iVar1 == 0x708) {
    uVar7 = 2;
  }
  else if (iVar1 == 0xa8c) {
    uVar7 = 3;
  }
  else if (iVar1 == 0xe10) {
    uVar7 = 4;
  }
  else if (iVar1 == 0x1518) {
    uVar7 = 5;
  }
  else if (iVar1 == 9000) {
    uVar7 = 6;
  }
  uVar3 = *(uint *)(in_EAX + 0x2c) & 0xf;
  if (uVar3 < 9) {
    uVar7 = uVar7 | uVar3 << 3;
  }
  uVar3 = *(uint *)(in_EAX + 0x30) & 0xf;
  if (uVar3 < 9) {
    uVar7 = uVar3 << 7 | uVar7;
  }
  _sprintf(&DAT_006ef91c,"%d,%d",uVar5 ^ (uint)(*(char *)(in_EAX + 0x40) != '\0') << 0x1d,uVar7);
  return &DAT_006ef91c;
}
#endif
