// player_profile_set_default_server_options  (Ghidra: FUN_0053a150, renamed)
// address 0x53a150, size 105 bytes
// VERIFIED against disassembly 0x53a150..0x53a1b9 (2026-09-30)
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/saved_games_functions.md summary "Sets default server name/password and
// game-option fields, used when starting a server without an existing profile." Same field
// offsets/values as player_profile_initialize's own network defaults
// (out/phase4/saved_games_types_notes.md: "0xd8c..0x1108, FUN_0053a150 writes the same
// defaults"). wcslen ("wide strlen" per src/interface/FUN_004a4a30.c's precedent) is
// called on each literal before the copy but its result is unused, so it is reproduced as a
// dead call rather than omitted.
// register convention: profile in ESI.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern uint16_t empty_string[]; // 0x00660c34, L"" (src/game and src/networking use the same name)


// blam-cc: profile in ESI
void player_profile_set_default_server_options(saved_player_profile *profile)
{
    wcslen(L"Halo");
    wcscpy(profile->server_name, L"Halo");
    wcslen(empty_string);
    wcscpy(profile->server_password, L"");
    profile->unknown_ebe = 0;
    profile->server_maximum_players_index = 3;
    profile->join_server_address[0] = 0;
    profile->connection_type = 1;
    profile->server_port = 0x8fe;
    profile->client_port = 0x8ff;
}

#if 0
Original Ghidra decompilation (0x53a150):

void FUN_0053a150(void)

{
  int unaff_ESI;

  FUN_00625b7a(L"Halo");
  _wcscpy((wchar_t *)(unaff_ESI + 0xd8c),L"Halo");
  FUN_00625b7a(&DAT_00660c34);
  _wcscpy((wchar_t *)(unaff_ESI + 0xeac),L"");
  *(undefined1 *)(unaff_ESI + 0xebe) = 0;
  *(undefined1 *)(unaff_ESI + 0xebf) = 3;
  *(undefined2 *)(unaff_ESI + 0xfc2) = 0;
  *(undefined1 *)(unaff_ESI + 0xfc0) = 1;
  *(undefined2 *)(unaff_ESI + 0x1002) = 0x8fe;
  *(undefined2 *)(&DAT_00001004 + unaff_ESI) = 0x8ff;
  return;
}
#endif
