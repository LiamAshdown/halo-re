// sv_players  (Ghidra: sv_players, already named)
// address 0x4e2c70, size 244 bytes
// name confidence: 0.75   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md; CEA-pdb match on "sv_players is a server-only
// function!"; types/networking.h network_player_entry array at server+0x1aa (session+0x1a2);
// types/game.h player::unknown_dc/medal_streak_count/medal_streak_timer at the exact offsets
// (+0xdc/+0xe0/+0xe4) this function reads off the resolved player pointer. This is also the
// function out/phase4/networking_types_notes.md documents as spanning the mid-body address
// 0x4e2d4b ("network_game_server_add_player_to_game__hook_add_player", misattributed, not a
// real function -- see that file's note and this batch's summary).
// register convention: __cdecl, no arguments.
// The real function spans 0x4e2c70..0x4e2e40 (the size in the header above is Ghidra's truncated
// first piece; the row loop and the "server-only" error path live past 0x4e2d64).
// Rewritten from the disassembly: header row and per-player rows go to console_out with three
// different color pointers (EAX = [0x685214] header, [0x686af8] rows, [0x6851fc] the error text);
// each valid row (network_player_entry_validate, EAX = entry) looks up the live player through
// sv_players_find_by_team_index_desired (ESI = entry->slot_index) and the player datum with an inline
// salt check; the row prints machine_index+1, the entry's name converted to ASCII (12 chars max),
// "Red"/"Blue", player.ping (+0xdc), the engine callback string ([current_game_engine+0x54](handle,
// buffer) fills the wide "Score" text), medal_streak_count (+0xe0) and medal_streak_timer/30.
// reconciled: R04 0x006f1d20 void * network_engine_callback_block -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

// VERIFIED against disassembly 0x4e2c70..0x4e2e41 (2026-09-30; header size 244 is only the first piece, real size 465): FIXED: three different console color pointers, header labels are literal strings (the old code read string bytes as pointers), team colour test was inverted (team_index 0 = Red), validate/find/convert take their register args, name column is the ASCII-converted entry name, datum resolve done inline exactly, sprintf (no count)
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdio.h>
#include <wchar.h>

extern int16_t network_game_mode; // 0x00719720, 2 == host
extern network_server_globals *network_server; // 0x0071c2d4
extern data_array *player_data; // 0x0087a480
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern wchar_t k_empty_string[]; // 0x0065512c
extern char network_team_color_name_red[]; // 0x0066db48 "Red"
extern char network_team_color_name_blue[]; // 0x0066db40 "Blue"

extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, EAX -> entry
extern uint32_t sv_players_find_by_team_index_desired(int8_t team_index_desired); // 0x4e2c10, blam-cc: ESI -> team_index_desired
extern uint8_t *string_convert_unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity); // 0x557950, blam-cc: ESI -> dest, EDI -> source, stack -> capacity
extern void *global_white_argb; // 0x006851fc, a ColorARGB * the original loads into EAX
extern void *console_color_00685214; // 0x00685214, a ColorARGB *
extern void *console_color_00686af8; // 0x00686af8, a ColorARGB *
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Inline datum resolve at 0x4e2d02..0x4e2d3c: NULL when the index is out of range, the slot's
// identifier is 0, or the handle carries a nonzero identifier that differs from the slot's.
static player *sv_players_resolve_player(uint32_t handle)
{
    int16_t index = (int16_t)handle;
    int16_t salt = (int16_t)(handle >> 16);
    uint8_t *element;

    if (index < 0 || index >= player_data->maximum_count) {
        return 0;
    }
    element = (uint8_t *)player_data->data + (int32_t)player_data->size * (int32_t)index;
    if (*(int16_t *)element == 0) {
        return 0;
    }
    if (salt != 0 && *(int16_t *)element != salt) {
        return 0;
    }
    return (player *)element;
}

// Console command: prints a formatted scoreboard header, then one row per valid player entry
// in the session's player table, including the ping, engine score text and team-kill statistics
// of the matching live player.
void sv_players(void)
{
    char line[256];
    uint16_t score_text[256];
    char ascii_name[16];
    network_player_entry *entry;
    int32_t remaining;

    if (network_game_mode != 2) {
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"sv_players is a server-only function!");
        return;
    }

    sprintf(line, "%-8s%-*s %-6s %-6s %-6s %-6s %-8s", "Number", 0xc, "Name", "Team", "Ping",
            "Score", "TK Num", "TK Timer");
    chimera__console_out((ColorARGB *)console_color_00685214, line);

    entry = network_server->session.players;
    remaining = 16;
    do {
        if (network_player_entry_validate(entry) != 0) {
            uint32_t found = sv_players_find_by_team_index_desired(entry->slot_index);
            player *p = 0;
            int32_t ping;
            int32_t tk_num;
            int32_t tk_timer;
            uint16_t *score_display;
            char *team_color;

            if (found != 0xffffffff) {
                p = sv_players_resolve_player(found);
            }

            score_text[0] = 0;
            if (found != 0xffffffff) {
                void (*resolve_score_text)(uint32_t, uint16_t *) =
                    *(void (**)(uint32_t, uint16_t *))((uint8_t *)current_game_engine + 0x54);
                resolve_score_text(found, score_text);
            }

            // the original leaves ascii_name unwritten when the name does not fit in 11 characters
            ascii_name[0] = 0;
            string_convert_unicode_to_ascii((uint8_t *)ascii_name, entry->name, 0xc);

            if (p == 0) {
                ping = 0;
                tk_num = 0;
                tk_timer = 0;
                score_display = (uint16_t *)k_empty_string;
            } else {
                ping = p->ping;
                tk_num = p->medal_streak_count;
                tk_timer = (p->medal_streak_timer < 0) ? 0 : p->medal_streak_timer / 30;
                score_display = score_text;
            }

            team_color = (entry->team_index == 0) ? network_team_color_name_red : network_team_color_name_blue;

            sprintf(line, "%-3d     %-*s %-6s %-4d   %-6ls %-3d    %-4d",
                    (int32_t)entry->machine_index + 1, 0xc, ascii_name, team_color, ping,
                    score_display, tk_num, tk_timer);
            chimera__console_out((ColorARGB *)console_color_00686af8, line);
        }
        entry = entry + 1;
        remaining = remaining - 1;
    } while (remaining != 0);
}

#if 0
Original Ghidra decompilation (0x4e2c70), from tools/pack.py 0x4e2c70:

void sv_players(void)

{
  char cVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  undefined **ppuVar5;
  undefined4 uVar6;
  int iVar7;
  undefined4 uVar8;
  short sVar9;
  undefined2 *puVar10;
  int iVar11;
  int local_314;
  undefined1 auStack_310 [16];
  char local_300 [256];
  undefined2 local_200 [256];

  iVar11 = DAT_0071c2d4;
  if (DAT_00719720 == 2) {
    _sprintf(local_300,"%-8s%-*s %-6s %-6s %-6s %-6s %-8s","Number",0xc,&DAT_0066db78,&DAT_0066db80,
             &DAT_0066db88,"Score","TK Num","TK Timer");
    chimera__console_out(local_300);
    iVar11 = iVar11 + 0x1aa;
    local_314 = 0x10;
    do {
      cVar1 = FUN_004de9f0();
      if (cVar1 != '\0') {
        iVar3 = FUN_004e2c10();
        iVar7 = 0;
        if (((iVar3 != -1) && (sVar2 = (short)iVar3, -1 < sVar2)) &&
           (sVar2 < *(short *)(DAT_0087a480 + 0x20))) {
          iVar4 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar2;
          sVar2 = *(short *)(iVar4 + *(int *)(DAT_0087a480 + 0x34));
          if ((sVar2 != 0) &&
             ((sVar9 = (short)((uint)iVar3 >> 0x10), sVar9 == 0 || (sVar2 == sVar9)))) {
            iVar7 = iVar4 + *(int *)(DAT_0087a480 + 0x34);
          }
        }
        local_200[0] = 0;
        if (iVar3 != -1) {
          (**(code **)(DAT_006f1d20 + 0x54))(iVar3,local_200);
        }
        FUN_00557950(0xc);
        if (iVar7 == 0) {
          iVar3 = 0;
          uVar8 = 0;
          puVar10 = (undefined2 *)&DAT_0065512c;
          uVar6 = 0;
        }
        else if (*(int *)(iVar7 + 0xe4) < 0) {
          uVar8 = *(undefined4 *)(iVar7 + 0xe0);
          uVar6 = *(undefined4 *)(iVar7 + 0xdc);
          iVar3 = 0;
          puVar10 = local_200;
        }
        else {
          uVar8 = *(undefined4 *)(iVar7 + 0xe0);
          iVar3 = *(int *)(iVar7 + 0xe4) / 0x1e;
          uVar6 = *(undefined4 *)(iVar7 + 0xdc);
          puVar10 = local_200;
        }
        ppuVar5 = &PTR_DAT_0066db48;
        if (*(char *)(iVar11 + 0x1e) != '\0') {
          ppuVar5 = (undefined **)&DAT_0066db40;
        }
        _sprintf(local_300,"%-3d     %-*s %-6s %-4d   %-6ls %-3d    %-4d",
                 *(char *)(iVar11 + 0x1c) + 1,0xc,auStack_310,ppuVar5,uVar6,puVar10,uVar8,iVar3);
        chimera__console_out(local_300);
      }
      iVar11 = iVar11 + 0x20;
      local_314 = local_314 + -1;
    } while (local_314 != 0);
    return;
  }
  chimera__console_out("sv_players is a server-only function!");
  return;
}
#endif
