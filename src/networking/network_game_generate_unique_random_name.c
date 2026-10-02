// network_game_generate_unique_random_name  (Ghidra: network_game_generate_unique_random_name,
// already named)
// address 0x4df730, size 96 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Picks a random default player name via
// network_game_get_random_player_name(), retrying until it doesn't collide with any currently
// active player, and copies the result into the output buffer." Same session scan base
// (container + 0x1aa == session.players[0].name, stride 0x10 wchars == 0x20 bytes) as
// network_player_name_collision_check.c.
// register convention: session container in EAX (in_EAX). blam-cc: EAX -> session, stack ->
// out_name

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, this batch
extern wchar_t *network_game_get_random_player_name(void); // 0x4dea80, this batch

// blam-cc: EAX -> session
void network_game_generate_unique_random_name(network_game_session *session, wchar_t *out_name)
{
    wchar_t *candidate;
    int32_t collisions;
    int32_t i;

    do {
        candidate = network_game_get_random_player_name();
        collisions = 0;
        for (i = 0; i < 0x10; i++) {
            if (network_player_entry_validate(&session->players[i]) != 0 &&
                wcscmp((wchar_t *)session->players[i].name, candidate) == 0) {
                collisions = collisions + 1;
            }
        }
    } while (collisions != 0);
    wcsncpy(out_name, candidate, 0xb);
    out_name[0xb] = L'\0';
}

#if 0
Original Ghidra decompilation (0x4df730):

void network_game_generate_unique_random_name(wchar_t *param_1)

{
  char cVar1;
  int in_EAX;
  wchar_t *_Str2;
  int iVar2;
  int iVar3;
  wchar_t *_Str1;
  int iVar4;

  do {
    _Str2 = (wchar_t *)network_game_get_random_player_name();
    iVar3 = 0;
    iVar4 = 0x10;
    _Str1 = (wchar_t *)(in_EAX + 0x1aa);
    do {
      cVar1 = FUN_004de9f0();
      if (cVar1 != '\0') {
        iVar2 = _wcscmp(_Str1,_Str2);
        if (iVar2 == 0) {
          iVar3 = iVar3 + 1;
        }
      }
      _Str1 = _Str1 + 0x10;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  } while (iVar3 != 0);
  _wcsncpy(param_1,_Str2,0xb);
  param_1[0xb] = L'\0';
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
