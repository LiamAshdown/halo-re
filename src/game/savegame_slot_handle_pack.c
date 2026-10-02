// savegame_slot_handle_pack  (Ghidra: FUN_0053e630; renamed, no established name)
// address 0x53e630, size 36 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/halo_decompiled.c's caller saved_game_enumerate_by_type (0x53c4e0, outside this
//   batch) calls this as `FUN_0053e630(local_7)` once per matching index record while iterating
//   the save-game index (this batch's savegame_index_read_slot-shaped loop), and stores the
//   result as one entry of its own output array -- consistent with this packing a per-slot
//   "handle" (slot number in the high bits, a small type/flag nibble, plus two single-bit
//   flags) for the caller to hand back to savegame_index_read_slot/_write_slot later.
// register convention: a slot index in EAX (in_EAX, masked to 12 bits), a 4-bit field in ECX
//   (in_ECX), a boolean in DL (in_DL); `flag_bit_31` is this function's own recognized stack
//   parameter.
//   // blam-cc: EAX -> slot_index, ECX -> type_nibble, DL -> flag_bit_30, stack -> flag_bit_31
// UNSURE: the caller's actual EAX/ECX/DL values at its one call site are not re-derived here
//   (saved_game_enumerate_by_type is outside this batch); field names are placeholders for the
//   bit layout Ghidra shows, not confirmed semantics.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EAX -> slot_index, ECX -> type_nibble, DL -> flag_bit_30, stack -> flag_bit_31
// Packs a 12-bit slot index, a 4-bit type nibble and two boolean flags into one 32-bit handle:
// bits 16..27 = slot_index, bits 0..3 = type_nibble, bit 30 = flag_bit_30, bit 31 = flag_bit_31.
uint32_t savegame_slot_handle_pack(uint32_t slot_index, uint32_t type_nibble, uint8_t flag_bit_30,
    uint8_t flag_bit_31)
{
    uint32_t result = ((slot_index & 0xfff) << 0x10) | (type_nibble & 0xf);
    if (flag_bit_30 == 1) {
        result = result | 0x40000000;
    }
    if (flag_bit_31 == 1) {
        result = result | 0x80000000;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x53e630), from tools/pack.py 0x53e630:

uint FUN_0053e630(char param_1)

{
  uint in_EAX;
  uint uVar1;
  uint in_ECX;
  char in_DL;

  uVar1 = (in_EAX & 0xfff) << 0x10 | in_ECX & 0xf;
  if (in_DL == '\x01') {
    uVar1 = uVar1 | 0x40000000;
  }
  if (param_1 == '\x01') {
    uVar1 = uVar1 | 0x80000000;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
