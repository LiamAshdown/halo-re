// network_session_host_start_info_set  (Ghidra: FUN_00576100; named per this rewrite)
// address 0x576100, size 126 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: strings "dedicated", "player_flags", "game_flags", "game_classic"; out/phase4/
// networking_functions.md summary: "Stores host/map/variant name strings and a game-type value
// into globals used to start a network session, and registers well-known Halo script globals
// (dedicated, player_flags, game_flags, game_classic...)."
// register convention: host name in EAX (in_EAX), map name in ESI (unaff_ESI), variant name in
// EDI (unaff_EDI, may be NULL), game type as the recognized stack parameter.
// blam-cc: EAX -> host_name, ESI -> map_name, EDI -> variant_name, stack -> game_type

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_session_start_host_name[];    // 0x00722798
extern char network_session_start_map_name[];      // 0x007227a0
extern char network_session_start_variant_name[];  // 0x007227a8
extern int32_t network_session_start_game_type;    // 0x007227b8

extern void FUN_0061bb40(int32_t script_global_id, const char *name); // foreign hs script-global registration

// blam-cc: EAX -> host_name, ESI -> map_name, EDI -> variant_name, stack -> game_type
// Stashes the host, map and (optional) variant names plus a game-type value into the globals a
// new network session is started from, then registers the well-known hs script globals
// (dedicated, player_flags, game_flags, game_classic) a dedicated server exposes.
void network_session_host_start_info_set(char *host_name, char *map_name, char *variant_name, int32_t game_type)
{
    char *dest;
    char *src;

    dest = network_session_start_host_name;
    src = host_name;
    do {
        *dest++ = *src;
    } while (*src++ != 0);

    dest = network_session_start_map_name;
    src = map_name;
    do {
        *dest++ = *src;
    } while (*src++ != 0);

    if (variant_name != 0) {
        dest = network_session_start_variant_name;
        src = variant_name;
        do {
            *dest++ = *src;
        } while (*src++ != 0);
    }

    network_session_start_game_type = game_type;
    FUN_0061bb40(0x33, "dedicated");
    FUN_0061bb40(0x34, "player_flags");
    FUN_0061bb40(0x35, "game_flags");
    FUN_0061bb40(0x36, "game_classic");
}

#if 0
Original Ghidra decompilation (0x576100):

void FUN_00576100(undefined4 param_1)

{
  char cVar1;
  char *in_EAX;
  int iVar2;
  char *unaff_ESI;
  char *unaff_EDI;

  iVar2 = (int)&DAT_00722798 - (int)in_EAX;
  do {
    cVar1 = *in_EAX;
    in_EAX[iVar2] = cVar1;
    in_EAX = in_EAX + 1;
  } while (cVar1 != '\0');
  iVar2 = (int)&DAT_007227a0 - (int)unaff_ESI;
  do {
    cVar1 = *unaff_ESI;
    unaff_ESI[iVar2] = cVar1;
    unaff_ESI = unaff_ESI + 1;
  } while (cVar1 != '\0');
  if (unaff_EDI != (char *)0x0) {
    iVar2 = (int)&DAT_007227a8 - (int)unaff_EDI;
    do {
      cVar1 = *unaff_EDI;
      unaff_EDI[iVar2] = cVar1;
      unaff_EDI = unaff_EDI + 1;
    } while (cVar1 != '\0');
  }
  DAT_007227b8 = param_1;
  FUN_0061bb40(0x33,"dedicated");
  FUN_0061bb40(0x34,"player_flags");
  FUN_0061bb40(0x35,"game_flags");
  FUN_0061bb40(0x36,"game_classic");
  return;
}
#endif
