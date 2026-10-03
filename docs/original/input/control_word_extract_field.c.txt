// control_word_extract_field  (orphan pass 4: FUN_004f3680, no Ghidra name)
// address 0x4f3680, size 102 bytes
// name confidence: 0.5 (out/phase4/objects_types_notes.md names this exact address
//   "object_control_word_field_extract" and describes it precisely: "pulls six different 3-bit
//   fields (shifts 4, 7, 10, 0x10, 0x13, 0xd) out of the packed words at 0x006f1cec /
//   0x006f1ce8." This pass drops the "object_" prefix since the function is moved out of that
//   module; the rest of the name and analysis is unchanged.)
// rewrite confidence: 0.7 (control flow, switch table and register convention confirmed
//   against objdump)
// evidence: out/phase4/objects_types_notes.md, "Not objects-module code": "0x4f3680 / 0x4f3700
//   / 0x4f37d0 / 0x4f3890 / 0x4f39d0 / 0x4f3ad0 read the packed control words at 0x006f1cec and
//   0x006f1ce8, which belong to the input or game module." src/objects/README.md line 461-462
//   repeats this verdict. Sole caller in this batch is control_binding_table_initialize
//   0x4f3700 (this pass, same directory), which calls it once per binding-table row and stores
//   the low 3-bit field group (field_index 0..4) or discards it (field_index 5, used
//   elsewhere). The fallback call to game_variant_option_default_by_index 0x465380
//   (src/game) when the low nibble isn't 8 is preserved verbatim; its own file candidly admits
//   the real meaning of that table is unresolved.
// register convention (confirmed via objdump): EAX = which_word (1 selects
//   control_word_secondary 0x006f1cec, anything else selects control_word_primary 0x006f1ce8),
//   ESI = field_index (0..5, default/out-of-range falls through to returning 0). Return in EAX;
//   Ghidra's ECX is dead output only relevant to the caller it never reaches.
// blam-cc: EAX -> which_word, ESI -> field_index
// FIXED (register inputs, objdump): note phrasing only -- rewritten from the call-style
// "f(x /*EAX*/)" comment the checker cannot parse into "EAX -> which_word, ESI -> field_index".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t control_word_secondary; // 0x006f1cec, UNSURE: not independently named/typed
extern uint32_t control_word_primary;   // 0x006f1ce8, UNSURE: not independently named/typed
extern uint32_t game_variant_option_default_by_index(uint32_t selector); // 0x465380, src/game; UNSURE: real purpose unresolved there too

uint32_t control_word_extract_field(uint32_t which_word, uint32_t field_index)
{
    uint32_t word = (which_word == 1) ? control_word_secondary : control_word_primary;

    if ((word & 0xf) != 8) {
        word = game_variant_option_default_by_index(word & 0xf);
    }

    switch (field_index) {
    case 0: return (word >> 4) & 7;
    case 1: return (word >> 7) & 7;
    case 2: return (word >> 10) & 7;
    case 3: return (word >> 0x10) & 7;
    case 4: return (word >> 0x13) & 7;
    case 5: return (word >> 0xd) & 7;
    default: return 0;
    }
}

#if 0
Original Ghidra decompilation (0x4f3680):

uint FUN_004f3680(void)

{
  int in_EAX;
  uint uVar1;
  uint uVar2;
  uint extraout_ECX;
  undefined4 unaff_ESI;

  uVar2 = 0;
  uVar1 = DAT_006f1cec;
  if (in_EAX != 1) {
    uVar1 = DAT_006f1ce8;
  }
  if ((uVar1 & 0xf) != 8) {
    uVar1 = FUN_00465380();
    uVar2 = extraout_ECX;
  }
  switch(unaff_ESI) {
  case 0:
    return uVar1 >> 4 & 7;
  case 1:
    return uVar1 >> 7 & 7;
  case 2:
    return uVar1 >> 10 & 7;
  case 3:
    return uVar1 >> 0x10 & 7;
  case 4:
    return uVar1 >> 0x13 & 7;
  case 5:
    uVar2 = uVar1 >> 0xd & 7;
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
