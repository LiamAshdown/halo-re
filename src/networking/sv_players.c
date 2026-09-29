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
// UNSURE (major): the datum-resolve idiom (bounds/salt check against player_data, then a raw
// offset add) is replaced here with a plain datum_get call, which performs the identical check
// internally; this is a simplification, not a literal transcription.
// UNSURE: sv_players_find_by_team_index_desired (FUN_004e2c10) is called here with zero visible
// arguments; the value it should receive (something team/slot-shaped off the current
// network_player_entry) is not recoverable from this function's own decompile.
// UNSURE: network_player_entry_is_valid (FUN_004de9f0) is likewise called with no visible
// arguments.
// UNSURE: the "TK Num"/"TK Timer" column labels (from the header row) are printed from
// player::medal_streak_count/medal_streak_timer, which types/game.h names for an unrelated
// medal-streak mechanism; either the column reuses that same field for team-kill tracking, or
// game.h's name does not universally apply. Preserved literally either way.
// UNSURE: the per-row name lookup goes through a raw function-pointer call at
// current_game_engine+0x54, whose signature is not independently established.
// reconciled: R04 0x006f1d20 void * network_engine_callback_block -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

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
    // "resolve player display name" callback (UNSURE)
extern wchar_t k_empty_string[]; // 0x0065512c (UNSURE: assumed empty/dash)
extern char *network_team_color_names[]; // 0x0066db78/0x0066db80/0x0066db88, the three column
    // header labels "Number"/"Name"/etc referenced positionally by the format string (UNSURE
    // grouping: these are printed as three separate varargs, not a real array)
extern char network_team_color_name_red[]; // 0x0066db48 (UNSURE name)
extern char network_team_color_name_blue[]; // 0x0066db40 (UNSURE name)

extern char network_player_entry_validate(void); // 0x4de9f0, other module, called with no
    // visible arguments (UNSURE, see header)
extern uint32_t sv_players_find_by_team_index_desired(void); // this module, 0x4e2c10,
    // called with no visible arguments (UNSURE, see header)
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, memory module
extern void *console_color_00685214; // 0x00685214, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: prints a formatted scoreboard header, then one row per valid player entry
// in the session's player table, including score and team-kill statistics resolved from the
// matching live player object.
void sv_players(void)
{
    char line[256];
    uint16_t name_buf[256];
    network_player_entry *entry;
    int32_t remaining;

    if (network_game_mode != 2) {
        chimera__console_out((ColorARGB *)console_color_00685214, "sv_players is a server-only function!");
        return;
    }

    snprintf(line, sizeof(line), "%-8s%-*s %-6s %-6s %-6s %-6s %-8s", "Number", 0xc,
             network_team_color_names[0], network_team_color_names[1], network_team_color_names[2],
             "Score", "TK Num", "TK Timer");
    chimera__console_out((ColorARGB *)console_color_00685214, line);

    entry = network_server->session.players;
    remaining = 16;
    do {
        if (network_player_entry_validate() != 0) {
            uint32_t found = sv_players_find_by_team_index_desired();
            player *p = 0;

            if (found != 0xffffffff) {
                p = (player *)datum_get((datum_index)found, player_data);
            }

            name_buf[0] = 0;
            if (found != 0xffffffff) {
                void (*resolve_name)(uint32_t, uint16_t *) =
                    *(void (**)(uint32_t, uint16_t *))((uint8_t *)current_game_engine + 0x54);
                resolve_name(found, name_buf);
            }

            {
                int32_t score, tk_num, tk_timer;
                uint16_t *name_display;
                char *team_color;

                if (p == 0) {
                    score = 0;
                    tk_num = 0;
                    tk_timer = 0;
                    name_display = (uint16_t *)k_empty_string;
                } else {
                    score = p->ping;
                    tk_num = p->medal_streak_count;
                    tk_timer = (p->medal_streak_timer < 0) ? 0 : p->medal_streak_timer / 30;
                    name_display = name_buf;
                }

                team_color = (entry->unknown_1e != 0) ? network_team_color_name_red : network_team_color_name_blue;

                snprintf(line, sizeof(line), "%-3d     %-*s %-6s %-4d   %-6ls %-3d    %-4d",
                         entry->machine_index + 1, 0xc, name_buf, team_color, score,
                         name_display, tk_num, tk_timer);
                chimera__console_out((ColorARGB *)console_color_00685214, line);
            }
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
