// network_player_name_collision_check  (Ghidra: FUN_004df6f0; named per this rewrite)
// address 0x4df6f0, size 60 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Checks whether the wide-character name pointed
// to by unaff_EBX already matches any active player's name in the 16-slot table, returning 0 on
// a collision." The scan base (container + 0x1aa) is exactly container->session.players[0].name
// when container is a network_client_globals/network_server_globals (session embedded at +8,
// players[] at session+0x1a2, name the first field of each 0x20-byte entry) -- 0x008 + 0x1a2 =
// 0x1aa.
// register convention: container in EAX (in_EAX), candidate name in EBX (unaff_EBX). blam-cc:
// EAX -> session, EBX -> candidate_name

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

// blam-cc: EAX -> session, EBX -> candidate_name
uint8_t network_player_name_collision_check(network_game_session *session, uint16_t *candidate_name)
{
    int32_t i;

    for (i = 0; i < 0x10; i++) {
        if (network_player_entry_validate(&session->players[i]) != 0) {
            if (wcscmp((wchar_t *)session->players[i].name, (wchar_t *)candidate_name) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4df6f0):

undefined4 FUN_004df6f0(void)

{
  char cVar1;
  int in_EAX;
  int iVar2;
  wchar_t *unaff_EBX;
  wchar_t *_Str1;
  int iVar3;

  iVar3 = 0;
  _Str1 = (wchar_t *)(in_EAX + 0x1aa);
  do {
    cVar1 = FUN_004de9f0();
    if (cVar1 != '\0') {
      iVar2 = _wcscmp(_Str1,unaff_EBX);
      if (iVar2 == 0) {
        return 0;
      }
    }
    iVar3 = iVar3 + 1;
    _Str1 = _Str1 + 0x10;
  } while (iVar3 < 0x10);
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
