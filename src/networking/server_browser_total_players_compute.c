// server_browser_total_players_compute  (Ghidra: server_browser_total_players_compute, already
// named)
// address 0x4baa60, size 117 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Sums the 'numplayers' key/value field
// across all server-browser query results to compute the total number of players online");
// types/networking.h "server browser" section names SBServerGetIntValue the GameSpy int accessor.
// register convention: server_list_globals* in EDI (unaff_EDI, unresolved).
//   // blam-cc: EDI -> array
// note: this function was named dynamic_pointer_array_* before the array it walks was
// pinned to the server_list global at 0x007196bc. The type it takes is therefore
// types/networking.h's server_list_globals, whose capacity/pending_count fields were
// folded in from this family's per-file TYPES-GAP typedefs during the review pass.
// The per-record type is the GameSpy server record, which
// types/networking.h explicitly leaves undeclared ("the queried-server type is not recoverable
// here and none is declared").
// UNSURE: `SBServerGetIntValue` (the GameSpy int accessor) is called three times per record with
// identical arguments; Ghidra shows no common-subexpression elimination here, so each call is
// preserved verbatim rather than assumed to be redundant, in case the accessor has side effects.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t server_browser_total_players; // 0x00719474

extern int32_t SBServerGetIntValue(void *entry, const char *key, int32_t default_value); // foreign, GameSpy int accessor // 0x617c10, GameSpy accessor


// blam-cc: EDI -> array
void server_browser_total_players_compute(server_list_globals *array)
{
    int32_t i = 0;

    server_browser_total_players = 0;
    if (0 < array->result_count) {
        do {
            int32_t num_players = SBServerGetIntValue(array->list[i], "numplayers", -1);

            if (-2 < num_players) {
                num_players = SBServerGetIntValue(array->list[i], "numplayers", -1);
                if (num_players < 0x11) {
                    num_players = SBServerGetIntValue(array->list[i], "numplayers", -1);
                    if (num_players == -1) {
                        goto next;
                    }
                } else {
                    num_players = 0x10;
                }
                server_browser_total_players = server_browser_total_players + num_players;
            }
next:
            i = i + 1;
        } while (i < array->result_count);
    }
}

#if 0
Original Ghidra decompilation (0x4baa60):

void server_browser_total_players_compute(void)

{
  int iVar1;
  int iVar2;
  int *unaff_EDI;

  iVar2 = 0;
  DAT_00719474 = 0;
  if (0 < unaff_EDI[1]) {
    do {
      iVar1 = FUN_00617c10(*(undefined4 *)(*unaff_EDI + iVar2 * 4),"numplayers",0xffffffff);
      if (-2 < iVar1) {
        iVar1 = FUN_00617c10(*(undefined4 *)(*unaff_EDI + iVar2 * 4),"numplayers",0xffffffff);
        if (iVar1 < 0x11) {
          iVar1 = FUN_00617c10(*(undefined4 *)(*unaff_EDI + iVar2 * 4),"numplayers",0xffffffff);
          if (iVar1 == -1) goto LAB_004baacb;
        }
        else {
          iVar1 = 0x10;
        }
        DAT_00719474 = DAT_00719474 + iVar1;
      }
LAB_004baacb:
      iVar2 = iVar2 + 1;
    } while (iVar2 < unaff_EDI[1]);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
