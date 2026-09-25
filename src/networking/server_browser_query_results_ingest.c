// server_browser_query_results_ingest  (Ghidra: server_browser_query_results_ingest, already
// named)
// address 0x4baae0, size 104 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Pulls new results out of the
// server-browser query engine into the local server list, then re-sorts and marks the query as
// finished"); types/networking.h server_list_globals (0x007196bc): its declared `list`/
// `result_count` fields are exactly the `data`/`count` fields the 0x4ba870/0x4ba8a0/0x4ba940
// helpers index, which is the evidence that server_list *is* the array
// dynamic_pointer_array_add_unique / server_browser_result_array_sort operate on.
// register convention: EAX -> list (server_list_globals*), forwarded unchanged (via ESI) to
// server_list_reset's entry arg, dynamic_pointer_array_add_unique's array arg and
// server_browser_result_array_sort's array arg -- all three calls read the same live register
// (objdump 0x4baae1 `mov esi,eax`; 0x4bab02/0x4bab36 `mov eax,esi` before the reset/sort calls;
// dynamic_pointer_array_add_unique's own ESI -> array convention matches the untouched esi at
// its call site). The already-committed `&server_list` argument is this same value.
// FIXED (register inputs, objdump): EAX is read live at entry (saved to ESI before any other
// instruction) and was previously replaced by the global address &server_list at each call site;
// added as a genuine `list` parameter and forwarded instead.
// note: this file's evidence is what extended types/networking.h's server_list_globals from
// two fields to four (capacity at +0x08, pending_count at +0x0c); it is one global, and the
// per-file TYPES-GAP typedef that used to sit here was folded into the header.
// UNSURE: `dynamic_pointer_array_add_unique` (0x4ba8a0) and `server_browser_result_array_sort`
// (0x4ba9c0) are both called with an empty argument list in Ghidra's decompile here, the same
// register-continuity elision documented in server_browser_result_array_sort.c. Reconstructed
// as taking the enumerated record handle and `&server_list` respectively, matching how
// both callees are defined elsewhere in this batch.
// UNSURE: `master_server_query_engine`, `server_browser_initialized` and
// `server_browser_query_pending` are named from behavior only; FUN_00617020/FUN_00617030/
// FUN_006175c0 are GameSpy library accessors outside this batch's address range.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>

extern uint8_t server_browser_initialized;   // 0x00719470
extern void *master_server_query_engine;     // 0x0071946c, GameSpy query object handle
extern uint8_t server_browser_query_pending;  // 0x0071948a, cleared once ingested

extern server_list_globals server_list; // 0x007196bc, see file header

extern void server_list_reset(uint8_t *entry); // 0x4b65f0, outside this batch; blam-cc: EAX -> entry
extern int32_t dynamic_pointer_array_add_unique(void *value, server_list_globals *array); // 0x4ba8a0, this batch
extern void server_browser_result_array_sort(server_list_globals *array); // 0x4ba9c0, this batch

extern int32_t FUN_00617020(void *query_engine, int32_t index); // 0x617020, GameSpy: enumerate result at index
extern int32_t FUN_00617030(void *engine); // foreign, GameSpy library: result count                // 0x617030, GameSpy: result count
extern int32_t FUN_006175c0(int32_t record);                    // 0x6175c0, GameSpy: record validity check

// blam-cc: EAX -> list
void server_browser_query_results_ingest(server_list_globals *list)
{
    if (server_browser_initialized != 0 && master_server_query_engine != 0) {
        int32_t result_count = FUN_00617030(master_server_query_engine);
        int32_t i = 0;

        server_list_reset((uint8_t *)list);

        if (0 < result_count) {
            do {
                int32_t record = FUN_00617020(master_server_query_engine, i);

                if (FUN_006175c0(record) != 0) {
                    dynamic_pointer_array_add_unique((void *)(intptr_t)record, list);
                }
                i = i + 1;
            } while (i < result_count);
        }

        server_browser_result_array_sort(list);
        server_browser_query_pending = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4baae0):

void server_browser_query_results_ingest(void)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;

  if ((DAT_00719470 != '\0') && (DAT_0071946c != 0)) {
    iVar1 = FUN_00617030(DAT_0071946c);
    server_list_reset();
    iVar4 = 0;
    if (0 < iVar1) {
      do {
        uVar2 = FUN_00617020(DAT_0071946c,iVar4);
        iVar3 = FUN_006175c0(uVar2);
        if (iVar3 != 0) {
          FUN_004ba8a0();
        }
        iVar4 = iVar4 + 1;
      } while (iVar4 < iVar1);
    }
    FUN_004ba9c0();
    DAT_0071948a = 0;
  }
  return;
}
#endif
