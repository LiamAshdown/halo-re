// server_browser_gametype1_flags_pack  (Ghidra: FUN_005767d0; named per this rewrite)
// address 0x5767d0, size 179 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary: "Encodes 4 boolean flags plus a
// time-limit enumeration into a compact code tagged with type id 1, likely for LAN game-browser
// advertisement." The low 3 bits (tag 1) match the sibling type-2/3/5 packers' own low-bit tags.
// register convention: 4-byte boolean array in ECX (in_ECX).
// blam-cc: ECX -> flags
// UNSURE: the exact meaning of each boolean and of the time-limit values (0, 0x708 == 30 min,
// 0xe10 == 1 h, 0x1518, 9000, 18000 ticks).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


// blam-cc: ECX -> options
// Encodes 4 boolean flags plus a time-limit enumeration into a compact code tagged with type id
// 1 (the low 3 bits).
uint32_t server_browser_gametype1_flags_pack(server_browser_gametype1_options *options)
{
    uint8_t *flags = options->flags;
    int32_t time_limit = options->time_limit;
    uint32_t bits = (((uint32_t)(flags[3] != 0) << 1 | (uint32_t)(flags[2] != 0)) << 1 |
                      (uint32_t)(flags[1] != 0)) << 1 | (uint32_t)(flags[0] != 0);
    if (time_limit == 0) {
        return bits << 3 | 1;
    }
    if (time_limit == 0x708) {
        return bits << 3 | 0x81;
    }
    if (time_limit == 0xe10) {
        return bits << 3 | 0x101;
    }
    if (time_limit == 0x1518) {
        return bits << 3 | 0x181;
    }
    if (time_limit == 9000) {
        return bits << 3 | 0x201;
    }
    if (time_limit == 18000) {
        return bits << 3 | 0x281;
    }
    return bits << 3 | 1;
}

#if 0
Original Ghidra decompilation (0x5767d0):

uint FUN_005767d0(void)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  char *in_ECX;

  iVar1 = *(int *)(in_ECX + 4);
  uVar2 = (((uint)(in_ECX[3] != '\0') << 1 | (uint)(in_ECX[2] != '\0')) << 1 |
          (uint)(in_ECX[1] != '\0')) << 1 | (uint)(*in_ECX != '\0');
  uVar3 = uVar2 << 3 | 1;
  if (iVar1 == 0) {
    return uVar2 << 3 | 1;
  }
  if (iVar1 == 0x708) {
    return uVar2 << 3 | 0x81;
  }
  if (iVar1 == 0xe10) {
    return uVar2 << 3 | 0x101;
  }
  if (iVar1 == 0x1518) {
    return uVar2 << 3 | 0x181;
  }
  if (iVar1 == 9000) {
    return uVar2 << 3 | 0x201;
  }
  if (iVar1 == 18000) {
    uVar3 = uVar2 << 3 | 0x281;
  }
  return uVar3;
}
#endif
