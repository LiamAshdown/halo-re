// server_browser_server_passes_filter  (Ghidra: server_browser_server_passes_filter, already
// named)
// address 0x4b7080, size 716 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_types_notes.md "server browser" section: this is its entire
// evidence base for the GameSpy accessors (FUN_00617490 string, FUN_006174d0 bool, FUN_00617aa0
// ping, FUN_00617c10 int) and for server_browser_filters (0x0071948b..0x00719490);
// server_browser_ping_limits (0x00695400) and server_browser_allow_password/_empty/_full
// (0x006953f9/fa/fb) and server_browser_require_valid_entry (0x006953f0) match exactly. The
// nine key strings referenced (numplayers, maxplayers, password, dedicated, game_classic,
// gametype, mapname, teamplay, gamever) appear in exactly the order the nine accessor calls
// run, which is what pins each anonymous call to its key.
// register convention: entry pointer in EAX (in_EAX); every accessor call below shows zero
// visible arguments in Ghidra's own decompile (entry and, where applicable, the key string are
// both invisible register pass-throughs) -- reconstructed explicitly from the key-string/call
// ordering match above.
// UNSURE: FUN_00617aa0 (ping) and the six-times-repeated FUN_00617c10 (numplayers then
// maxplayers) calls are each called 2-3 times in a row with identical arguments (a bounds
// check, then a re-fetch of the real value); kept as repeated calls exactly as decompiled
// rather than collapsed into one.
// UNSURE: the teamplay check's polarity reads inverted relative to types/networking.h's
// server_browser_filters.teamplay comment ("1 team games only, 2 free for all only"): as
// decompiled, a team-only filter (1) only passes servers whose "teamplay" key is NOT 1, and a
// free-for-all filter (2) only passes servers whose key IS 1 -- reproduced exactly, not
// corrected, since neither the key's own polarity nor the filter enum's was independently
// re-verified here.
// UNSURE: DAT_0071948e's five-way switch falls through to `switchD_004b72c7_default` (skip the
// gametype check) both when the filter byte is 0 and when it is anything above 5; reproduced
// with the same fallthrough.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>

extern uint8_t server_browser_require_valid_entry; // 0x006953f0
extern uint8_t server_browser_filter_dedicated_only; // 0x0071948b
extern uint8_t server_browser_allow_password;        // 0x006953f9
extern uint8_t server_browser_filter_classic_only;   // 0x0071948c
extern uint8_t server_browser_allow_empty;           // 0x006953fa
extern uint8_t server_browser_allow_full;            // 0x006953fb
extern uint8_t server_browser_filter_ping_limit_index; // 0x00719490
extern int32_t server_browser_ping_limits[];         // 0x00695400
extern uint8_t server_browser_filter_gametype;       // 0x0071948e
extern uint8_t server_browser_filter_teamplay;       // 0x0071948f
extern uint8_t server_browser_filter_allow_unknown_map; // 0x0071948d

extern uint32_t FUN_006175f0(int32_t object); // 0x6175f0: returns the uint32 at object+0x00 // foreign, GameSpy library; entry validity check
extern int32_t FUN_00617aa0(void *entry); // foreign, GameSpy library; ping accessor, no key
extern int32_t FUN_00617c10(void *entry, const char *key, int32_t default_value); // foreign, GameSpy int accessor // foreign, GameSpy library; int accessor
extern int32_t FUN_006174d0(void *entry, const char *key, int32_t default_value); // foreign, GameSpy bool accessor // foreign, GameSpy library; bool accessor
extern char *FUN_00617490(void *entry, const char *key, const char *default_value); // foreign, GameSpy library; string accessor
extern uint8_t autopatch_version_string_is_outdated(const char *gamever); // 0x5781c0, outside this
    // session's range; returns its answer in AL, which is all Ghidra's `cVar1` reads here
extern int32_t map_list_find_known_map_index(const char *mapname); // foreign, outside this session's range
extern int32_t __stricmp(const char *a, const char *b);

// blam-cc: entry pointer in EAX (in_EAX)
uint8_t server_browser_server_passes_filter(void *entry)
{
    int32_t probe;
    int32_t ping;
    int32_t numplayers;
    int32_t maxplayers;
    int32_t has_password;
    int32_t is_dedicated;
    int32_t is_classic;
    char *gametype_name;
    char *mapname;
    int32_t is_teamplay;
    char *gamever;
    uint8_t version_outdated;
    const char *gametype_filter_name;
    int32_t teamplay_mismatch;

    if (entry == 0 ||
        (server_browser_require_valid_entry != 0 &&
         (probe = (int32_t)FUN_006175f0((int32_t)(uintptr_t)entry), probe == 0))) {
        return 0;
    }

    probe = FUN_00617aa0(entry);
    maxplayers = -1;
    if (probe < -1) {
        ping = -1;
    } else {
        probe = FUN_00617aa0(entry);
        if (probe < 10000) {
            ping = FUN_00617aa0(entry);
        } else {
            ping = 9999;
        }
    }

    probe = FUN_00617c10(entry, "numplayers", 0);
    if (probe < -1) {
        numplayers = -1;
    } else {
        probe = FUN_00617c10(entry, "numplayers", 0);
        if (probe < 0x11) {
            numplayers = FUN_00617c10(entry, "numplayers", 0);
        } else {
            numplayers = 0x10;
        }
    }

    probe = FUN_00617c10(entry, "maxplayers", 0);
    if (-2 < probe) {
        probe = FUN_00617c10(entry, "maxplayers", 0);
        if (probe < 0x11) {
            maxplayers = FUN_00617c10(entry, "maxplayers", 0);
        } else {
            maxplayers = 0x10;
        }
    }

    has_password = FUN_006174d0(entry, "password", 0);
    is_dedicated = FUN_006174d0(entry, "dedicated", 0);
    is_classic = FUN_006174d0(entry, "game_classic", 0);
    gametype_name = FUN_00617490(entry, "gametype", "");
    mapname = FUN_00617490(entry, "mapname", "");
    is_teamplay = FUN_006174d0(entry, "teamplay", 0);
    gamever = FUN_00617490(entry, "gamever", "");
    version_outdated = autopatch_version_string_is_outdated(gamever);

    if (0x270e < ping) {
        return 0;
    }
    if (numplayers == -1) {
        return 0;
    }
    if (maxplayers == -1) {
        return 0;
    }
    if (server_browser_filter_dedicated_only != 0 && is_dedicated != 1) {
        return 0;
    }
    if (server_browser_allow_password == 0 && has_password == 1) {
        return 0;
    }
    if (server_browser_filter_classic_only != 0 && is_classic != 1) {
        return 0;
    }
    if (server_browser_allow_empty == 0 && numplayers == 0) {
        return 0;
    }
    if (server_browser_allow_full == 0 && maxplayers <= numplayers) {
        return 0;
    }
    if (server_browser_filter_ping_limit_index != 0 &&
        server_browser_ping_limits[server_browser_filter_ping_limit_index] < ping) {
        return 0;
    }
    if (version_outdated == 0) {
        return 0;
    }
    if (server_browser_filter_gametype != 0) {
        switch (server_browser_filter_gametype) {
        case 1:
            gametype_filter_name = "CTF";
            break;
        case 2:
            gametype_filter_name = "Slayer";
            break;
        case 3:
            gametype_filter_name = "Oddball";
            break;
        case 4:
            gametype_filter_name = "King";
            break;
        case 5:
            gametype_filter_name = "Race";
            break;
        default:
            goto skip_gametype_check;
        }
        probe = __stricmp(gametype_name, gametype_filter_name);
        if (probe != 0) {
            return 0;
        }
    }
skip_gametype_check:
    if (is_teamplay == 1) {
        teamplay_mismatch = (server_browser_filter_teamplay == 1);
    } else {
        teamplay_mismatch = (server_browser_filter_teamplay == 2);
    }
    if (!teamplay_mismatch &&
        (server_browser_filter_allow_unknown_map != 0 ||
         (probe = map_list_find_known_map_index(mapname), probe != -1))) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4b7080):

undefined1 server_browser_server_passes_filter(void)

{
  char cVar1;
  int in_EAX;
  int iVar2;
  int iVar3;
  int iVar4;
  char *_Str1;
  int iVar5;
  int iVar6;
  bool bVar7;
  char *_Str2;
  int local_c;
  int local_8;

  if ((in_EAX == 0) || ((DAT_006953f0 != '\0' && (iVar2 = FUN_006175f0(), iVar2 == 0)))) {
    return 0;
  }
  iVar2 = FUN_00617aa0();
  iVar6 = -1;
  if (iVar2 < -1) {
    local_c = -1;
  }
  else {
    iVar2 = FUN_00617aa0();
    if (iVar2 < 10000) {
      local_c = FUN_00617aa0();
    }
    else {
      local_c = 9999;
    }
  }
  iVar2 = FUN_00617c10();
  if (iVar2 < -1) {
    local_8 = -1;
  }
  else {
    iVar2 = FUN_00617c10();
    if (iVar2 < 0x11) {
      local_8 = FUN_00617c10();
    }
    else {
      local_8 = 0x10;
    }
  }
  iVar2 = FUN_00617c10();
  if (-2 < iVar2) {
    iVar2 = FUN_00617c10();
    if (iVar2 < 0x11) {
      iVar6 = FUN_00617c10();
    }
    else {
      iVar6 = 0x10;
    }
  }
  iVar2 = FUN_006174d0();
  iVar3 = FUN_006174d0();
  iVar4 = FUN_006174d0();
  _Str1 = (char *)FUN_00617490();
  FUN_00617490();
  iVar5 = FUN_006174d0();
  FUN_00617490();
  cVar1 = autopatch_version_string_is_outdated();
  if (0x270e < local_c) {
    return 0;
  }
  if (local_8 == -1) {
    return 0;
  }
  if (iVar6 == -1) {
    return 0;
  }
  if ((DAT_0071948b != '\0') && (iVar3 != 1)) {
    return 0;
  }
  if ((DAT_006953f9 == '\0') && (iVar2 == 1)) {
    return 0;
  }
  if ((DAT_0071948c != '\0') && (iVar4 != 1)) {
    return 0;
  }
  if ((DAT_006953fa == '\0') && (local_8 == 0)) {
    return 0;
  }
  if ((DAT_006953fb == '\0') && (iVar6 <= local_8)) {
    return 0;
  }
  if ((DAT_00719490 != 0) && (*(int *)(&DAT_00695400 + (uint)DAT_00719490 * 4) < local_c)) {
    return 0;
  }
  if (cVar1 == '\0') {
    return 0;
  }
  if (DAT_0071948e != '\0') {
    switch(DAT_0071948e) {
    case '\x01':
      _Str2 = "CTF";
      break;
    case '\x02':
      _Str2 = "Slayer";
      break;
    case '\x03':
      _Str2 = "Oddball";
      break;
    case '\x04':
      _Str2 = "King";
      break;
    case '\x05':
      _Str2 = "Race";
      break;
    default:
      goto switchD_004b72c7_default;
    }
    iVar2 = __stricmp(_Str1,_Str2);
    if (iVar2 != 0) {
      return 0;
    }
  }
switchD_004b72c7_default:
  if (iVar5 == 1) {
    bVar7 = DAT_0071948f == '\x01';
  }
  else {
    bVar7 = DAT_0071948f == '\x02';
  }
  if ((!bVar7) && ((DAT_0071948d != '\0' || (iVar2 = map_list_find_known_map_index(), iVar2 != -1)))
     ) {
    return 1;
  }
  return 0;
}
#endif
