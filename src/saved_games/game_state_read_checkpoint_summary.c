// game_state_read_checkpoint_summary  (Ghidra: FUN_00538320, renamed)
// address 0x538320, size 100 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/saved_games_functions.md summary "Reads the persistent checkpoint
// header and returns its embedded name/id via register out-parameters, likely for UI display."
// Same stack-allocated game_state_header buffer and saved_game_validate_crc call pattern as
// game_state_load_checkpoint (0x538280) and game_state_read_checkpoint_summary's sibling
// readers; out/phase4/saved_games_types_notes.md's register-conventions note: "0x538320
// returns [difficulty] (ESI) and scenario name (EDI)". The hand-rolled offset-delta copy loop
// is rewritten as strcpy, following the precedent in src/interface/console_update_display.c.
// register convention: out_difficulty in ESI, out_scenario_name in EDI (both confirmed by the
// module's register-convention notes); EAX = corrupt_flag, forwarded unchanged to
// saved_game_validate_crc.
//   // blam-cc: EAX -> corrupt_flag, ESI -> out_difficulty, EDI -> out_scenario_name
// FIXED (register inputs, objdump): EAX carries corrupt_flag (pushed at 0x538327, the first
// argument pushed for the call to saved_game_validate_crc, i.e. its last stack parameter). The
// old rewrite hardcoded that argument to 0/NULL like the sibling game_state_load_checkpoint,
// but this function's own EAX is live-in and passed straight through instead.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"


extern char *strcpy(char *dest, const char *source);

// blam-cc: EAX -> corrupt_flag, ESI -> out_difficulty, EDI -> out_scenario_name
// Reads and crc-validates the persistent checkpoint (savegame.bin) header without applying it,
// and returns its difficulty and scenario name for display. On failure, returns a default
// difficulty of 1 and an empty name.
uint8_t game_state_read_checkpoint_summary(uint8_t *corrupt_flag, int16_t *out_difficulty,
    char *out_scenario_name)
{
    game_state_header header;

    if (saved_game_validate_crc(k_game_state_size, k_game_state_header_size, (uint8_t *)&header,
            &header.file_checksum, corrupt_flag) != 0) {
        *out_difficulty = header.difficulty;
        strcpy(out_scenario_name, header.scenario_name);
        return 1;
    }
    *out_difficulty = 1;
    *out_scenario_name = 0;
    return 0;
}

#if 0
Original Ghidra decompilation (0x538320):

uint FUN_00538320(void)

{
  char cVar1;
  uint uVar2;
  char *pcVar3;
  int iVar4;
  undefined2 *unaff_ESI;
  undefined1 *unaff_EDI;
  char local_148 [290];
  undefined2 local_26;
  undefined1 local_4 [4];

  uVar2 = saved_game_validate_crc(local_4);
  if ((char)uVar2 != '\0') {
    *unaff_ESI = local_26;
    pcVar3 = local_148;
    iVar4 = (int)unaff_EDI - (int)pcVar3;
    do {
      cVar1 = *pcVar3;
      pcVar3[iVar4] = cVar1;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
    return CONCAT31((int3)((uint)pcVar3 >> 8),1);
  }
  *unaff_ESI = 1;
  *unaff_EDI = 0;
  return uVar2 & 0xffffff00;
}
#endif
