// server_browser_player_list_populate  (Ghidra: server_browser_player_list_populate, already
// named)
// address 0x4b73e0, size 254 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary implied by its name (already assigned)
// plus the literal strings "numplayers"/"player"/"score" and the two swprintf formats
// (default-name fallback vs. name+score); ticker_text_buffer_append is this module's own
// rewrite.
// register convention: GameSpy entry pointer in EBX (unaff_EBX).
// UNSURE: SBServerGetPlayerStringValue (name/score-by-index accessor) is called with zero visible arguments at
// every site; reconstructed as (entry, key, index) from the "player"/"score" literal strings
// and the loop's own index variable -- the real argument shape (a single "player_%d"-style key,
// vs. separate key+index) was not independently confirmed.
// note: ticker_text_buffer_append's `self` (EDI) is invisible in Ghidra's decompile; the
// disassembly of both call sites (0x4b7440, 0x4b74bc) loads 0x006b5e58, the instance
// types/networking.h names server_browser_player_ticker after this very function.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

extern wchar_t DAT_00669cc8[]; // default player-name string, see UNSURE

extern int32_t SBServerGetIntValue(void *entry, const char *key, int32_t default_value); // foreign, GameSpy int accessor // foreign, GameSpy library, int accessor
extern char *SBServerGetPlayerStringValue(void *entry, int32_t index, const char *key, const char *default_value); // 0x617530 SBServerGetPlayerStringValue
// swprintf comes from <wchar.h>; not redeclared here to avoid a conflicting-prototype error.
extern ticker_text_buffer server_browser_player_ticker; // 0x006b5e58
extern void ticker_text_buffer_append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self); // 0x4b8a60, this module

// blam-cc: GameSpy entry pointer in EBX (unaff_EBX)
// Clears the ticker, then appends one formatted row per player (name and score, or a default
// placeholder row when the name lookup fails), clamping the player count to 0..16.
int32_t server_browser_player_list_populate(void *entry)
{
    int32_t probe;
    int32_t player_count;
    int32_t i;
    char *name;
    char *score;
    wchar_t row[0x100];

    if (entry == 0) {
        return 1;
    }
    probe = SBServerGetIntValue(entry, "numplayers", 0);
    if (probe < 0) {
        player_count = 0;
    } else {
        probe = SBServerGetIntValue(entry, "numplayers", 0);
        if (probe < 0x11) {
            player_count = SBServerGetIntValue(entry, "numplayers", 0);
        } else {
            player_count = 0x10;
        }
    }
    ticker_text_buffer_append(0, 0, &server_browser_player_ticker);
    i = 0;
    if (0 < player_count) {
        do {
            name = SBServerGetPlayerStringValue(entry, i, "player", 0); // FIXED 2026-09-28: 0x4b7450 pushes (server, index, key, NULL)
            if (name == 0) {
                swprintf(row, 0x100, L"  %s %d     ", DAT_00669cc8, 0);
            } else {
                score = SBServerGetPlayerStringValue(entry, i, "score", "--"); // FIXED 2026-09-28: default "--" (0x0066b038) at 0x4b7486
                swprintf(row, 0x100, L"  %S %S     ", name, score);
            }
            ticker_text_buffer_append(row, 0, &server_browser_player_ticker);
            i = i + 1;
        } while (i < player_count);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4b73e0):

undefined4 server_browser_player_list_populate(void)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int unaff_EBX;
  int iVar4;
  wchar_t local_200 [256];

  if (unaff_EBX == 0) {
    return 1;
  }
  iVar1 = FUN_00617c10();
  if (iVar1 < 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = FUN_00617c10();
    if (iVar1 < 0x11) {
      iVar1 = FUN_00617c10();
    }
    else {
      iVar1 = 0x10;
    }
  }
  ticker_text_buffer_append(0,0);
  iVar4 = 0;
  if (0 < iVar1) {
    do {
      iVar2 = FUN_00617530();
      if (iVar2 == 0) {
        FID_conflict_swprintf(local_200,0x100,L"  %s %d     ",&DAT_00669cc8,0);
      }
      else {
        uVar3 = FUN_00617530();
        FID_conflict_swprintf(local_200,0x100,L"  %S %S     ",iVar2,uVar3);
      }
      ticker_text_buffer_append(local_200,0);
      iVar4 = iVar4 + 1;
    } while (iVar4 < iVar1);
  }
  return 1;
}
#endif
