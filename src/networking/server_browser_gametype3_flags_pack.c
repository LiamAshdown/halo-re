// server_browser_gametype3_flags_pack  (Ghidra: FUN_005769c0; named per this rewrite)
// address 0x5769c0, size 92 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary: "Encodes two booleans and five small
// numeric options into a compact code tagged with type id 3."
// register convention: source struct pointer in ECX (in_ECX).
// blam-cc: ECX -> options

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


// blam-cc: ECX -> options
// Encodes two booleans and five small numeric options into a compact code tagged with type id 3
// (the low 3 bits).
uint32_t server_browser_gametype3_flags_pack(server_browser_gametype3_options *options)
{
    return (((((((((uint32_t)options->value_14 & 0x1f) << 2 | ((uint32_t)options->value_10 & 3)) << 2 |
                 ((uint32_t)options->value_0c & 3)) << 2 | ((uint32_t)options->value_08 & 3)) << 2 |
               ((uint32_t)options->value_04 & 3)) << 1 | (uint32_t)(options->flag1 != 0)) << 1 |
             (uint32_t)(options->flag0 != 0)) << 3 | 3);
}

#if 0
Original Ghidra decompilation (0x5769c0):

uint FUN_005769c0(void)

{
  char *in_ECX;

  return (((((((*(uint *)(in_ECX + 0x14) & 0x1f) << 2 | *(uint *)(in_ECX + 0x10) & 3) << 2 |
             *(uint *)(in_ECX + 0xc) & 3) << 2 | *(uint *)(in_ECX + 8) & 3) << 2 |
           *(uint *)(in_ECX + 4) & 3) << 1 | (uint)(in_ECX[1] != '\0')) << 1 |
         (uint)(*in_ECX != '\0')) << 3 | 3;
}
#endif
