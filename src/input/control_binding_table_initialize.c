// control_binding_table_initialize  (orphan pass 4: FUN_004f3700, no Ghidra name)
// address 0x4f3700, size 197 bytes
// name confidence: 0.3 (inferred purely from behaviour: zeroes a run of the control-binding
//   region described in src/objects/README.md's "object control binding tables" note and
//   reseeds each row's default field from control_word_extract_field / a per-profile table;
//   no Bungie string or symbol names it)
// rewrite confidence: 0.35 (control flow and arithmetic transcribed literally from the
//   decompilation and preserved byte-for-byte via g_control_binding_region; the real field
//   layout of that region is NOT recovered here, matching src/objects/README.md's own stance:
//   "No type is claimed; the addresses are recorded as globals only.")
// evidence: out/phase4/objects_types_notes.md / src/objects/README.md: these six functions
//   "read the packed control words at 0x006f1cec/0x006f1ce8 and belong to the input or game
//   module." types/ai's global_globals (src/ai/actor_get_grenade_launch_velocity.c) is reused
//   here purely because 0x00746fa0 is already declared under that name elsewhere in this
//   codebase; offset +0x168 into it is not otherwise established and may belong to an
//   unrelated subsystem sharing the same monolithic globals block.
// register convention: the function itself takes no parameters. Its call to
//   control_word_extract_field (0x4f3680) passes EAX = outer_row (0 on the first 0x50-byte
//   row, 1 on the second) and ESI = field_index, which runs 0..5 within each outer row: the
//   outer loop jumps back to 0x4f3730, where `xor esi,esi; xor ebp,ebp` resets both inner
//   counters (orphan pass 4 review; the draft let it run 0..11, which sends 6..11 to the
//   extractor's default case and stores 0 for the whole second row).
// UNSURE (function-wide): the real shape of the control-binding region past "it is written in
//   4-byte cells with an outer stride of 0x50 bytes, spanning the 0xa0-byte range
//   0x008603e4..0x00860484 (exactly two 0x50-byte outer rows), each holding six 0xa0-byte inner
//   cells" is not established.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc, UNSURE: nonzero -> "secondary control word in use", see below
extern uint8_t g_control_binding_state;     // 0x008607a0, UNSURE: not independently typed
extern uint8_t g_control_binding_secondary_active; // 0x008607a1, UNSURE: not independently typed
extern Globals *global_globals;
extern uint32_t current_game_engine; // 0x006f1d20, UNSURE: nonzero selects the per-profile default source
extern uint32_t control_word_extract_field(uint32_t which_word, uint32_t field_index); // 0x4f3680, same pass

// UNSURE (function-wide): raw byte offsets into the control-binding region starting at
// 0x008603e4; see file header. Modeled as a byte array so the arithmetic below matches the
// decompilation exactly.
extern uint8_t g_control_binding_region_e4[0xa0]; // base 0x008603e4

void control_binding_table_initialize(void)
{
    uint8_t *row;
    int32_t offset;
    int32_t outer_row = 0;   // objdump: EAX at the control_word_extract_field call, 0 then 1
    int32_t field_index;     // objdump: ESI at the same call, 0..5, reset for each outer row

    g_control_binding_secondary_active = (game_engine_teams_enabled_flag != 0);
    g_control_binding_state = 0;

    row = g_control_binding_region_e4;
    do {
        uint8_t *cell = row;
        offset = 0;
        field_index = 0;
        do {
            // cell points 4 bytes into a control_binding_half (types/input.h), at selected_count
            ((control_binding_half *)(cell - 4))->entry_count = 0;
            ((control_binding_half *)(cell - 4))->selected_count = 0;
            ((control_binding_half *)(cell - 4))->limit = (int32_t)control_word_extract_field(outer_row, field_index);
            field_index++;

            {
                uint8_t *sub = cell + 0x10;
                int32_t count = 8;
                do {
                    ((control_binding_entry *)(sub - 4))->id = -1;       // sub points at the entry's selected flag
                    ((control_binding_entry *)(sub - 4))->selected = 0;
                    sub += 8;
                    count--;
                } while (count != 0);
            }

            if (current_game_engine == 0) {
                ((control_binding_half *)(cell - 4))->profile_default = -1;
            } else {
                ((control_binding_half *)(cell - 4))->profile_default = *(int32_t *)(*(int32_t *)((uint8_t *)global_globals->multiplayer_information.pointer + 0x24) + 0xc + offset);
            }

            offset += 0x10;
            cell += 0xa0;
        } while (offset < 0x60);

        row += 0x50;
        outer_row++;
    } while (row < g_control_binding_region_e4 + 0xa0);
}

#if 0
Original Ghidra decompilation (0x4f3700):

void FUN_004f3700(void)

{
  undefined4 uVar1;
  undefined4 *puVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *local_4;

  DAT_008607a1 = DAT_006f1cbc != '\0';
  DAT_008607a0 = 0;
  local_4 = &DAT_008603e4;
  do {
    iVar4 = 0;
    puVar5 = local_4;
    do {
      puVar5[-1] = 0;
      *puVar5 = 0;
      uVar1 = FUN_004f3680();
      puVar5[1] = uVar1;
      puVar2 = puVar5 + 4;
      iVar3 = 8;
      do {
        puVar2[-1] = 0xffffffff;
        *(undefined1 *)puVar2 = 0;
        puVar2 = puVar2 + 2;
        iVar3 = iVar3 + -1;
      } while (iVar3 != 0);
      if (DAT_006f1d20 == 0) {
        puVar5[2] = 0xffffffff;
      }
      else {
        puVar5[2] = *(undefined4 *)(*(int *)(*(int *)(DAT_00746fa0 + 0x168) + 0x24) + 0xc + iVar4);
      }
      iVar4 = iVar4 + 0x10;
      puVar5 = puVar5 + 0x28;
    } while (iVar4 < 0x60);
    local_4 = local_4 + 0x14;
  } while ((int)local_4 < 0x860484);
  return;
}
#endif
