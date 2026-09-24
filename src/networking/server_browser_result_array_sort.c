// server_browser_result_array_sort  (Ghidra: FUN_004ba9c0; named per this rewrite)
// address 0x4ba9c0, size 157 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("Sorts the server-browser result array
// by the active sort column while preserving which entry stays selected, then re-clamps the
// visible page window"); shares the dynamic_pointer_array layout with
// dynamic_pointer_array_find_index.c and friends; server_browser_sort_comparator_select.c for
// the comparator lookup.
// register convention: server_list_globals* in EAX (in_EAX, unresolved).
//   // blam-cc: EAX -> array
// note: this function was named dynamic_pointer_array_* before the array it walks was
// pinned to the server_list global at 0x007196bc. The type it takes is therefore
// types/networking.h's server_list_globals, whose capacity/pending_count fields were
// folded in from this family's per-file TYPES-GAP typedefs during the review pass.
// UNSURE: `_NumOfElements` (the qsort element count) is read completely uninitialized in
// Ghidra's decompile -- an elided-value artifact, the same pattern documented throughout this
// codebase's more register-heavy functions. Reconstructed as `array->result_count`, the only value in
// scope that makes qsort's call well-defined.
// UNSURE: `server_browser_selected_index`, `server_browser_skip_reselect` and
// `server_list_scroll_offset` are named from behavior only; server_list_scroll_clamp (the scroll-window
// refresh called when the offset changes) is outside this batch's address range.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdlib.h>

extern int32_t server_browser_selected_index; // 0x006953f4, -1 when nothing is selected
extern uint8_t server_browser_skip_reselect;  // 0x00719480, nonzero suppresses the reselect pass
extern int32_t server_list_scroll_offset;  // 0x00719478, visible-page window start, 16 rows



extern server_browser_sort_comparator server_browser_sort_comparator_select(void); // 0x4ba970
extern void server_browser_total_players_compute(server_list_globals *array); // 0x4baa60, this
    // batch; blam-cc: EDI -> array (see that file's header for why the register differs from
    // this function's own EAX -> array)
extern void server_list_scroll_clamp(void); // outside this batch, unnamed, scroll-window refresh

// blam-cc: EAX -> array
void server_browser_result_array_sort(server_list_globals *array)
{
    if (0 < array->result_count) {
        server_browser_sort_comparator comparator;
        void *previous_key;

        if (server_browser_selected_index == -1) {
            previous_key = 0;
        } else {
            previous_key = array->list[server_browser_selected_index];
        }

        comparator = server_browser_sort_comparator_select();
        qsort(array->list, (size_t)array->result_count, 4, comparator); // UNSURE: count, see file header

        if (server_browser_skip_reselect == 0) {
            server_browser_selected_index = -1;

            if (previous_key != 0 && 0 < array->result_count) {
                void **element = array->list;
                int32_t i = 0;

                do {
                    server_browser_selected_index = i;
                    if (*element == previous_key) {
                        break;
                    }
                    i = i + 1;
                    element = element + 1;
                    server_browser_selected_index = -1;
                } while (i < array->result_count);
            }

            if (server_browser_selected_index != -1 &&
                (server_browser_selected_index < server_list_scroll_offset ||
                 server_list_scroll_offset + 0xf <= server_browser_selected_index)) {
                server_list_scroll_offset = server_browser_selected_index;
                server_list_scroll_clamp();
            }
        }

        array->pending_count = 0;
        server_browser_total_players_compute(array);
    }
}

#if 0
Original Ghidra decompilation (0x4ba9c0):

void FUN_004ba9c0(void)

{
  int *in_EAX;
  _PtFuncCompare *_PtFuncCompare;
  int iVar1;
  size_t _NumOfElements;
  int *piVar2;
  int iVar3;
  int iVar4;

  if (0 < in_EAX[1]) {
    if (DAT_006953f4 == -1) {
      iVar4 = 0;
    }
    else {
      iVar4 = *(int *)(*in_EAX + DAT_006953f4 * 4);
    }
    _PtFuncCompare = server_browser_sort_comparator_select();
    _qsort((void *)*in_EAX,_NumOfElements,4,_PtFuncCompare);
    if (DAT_00719480 == '\0') {
      iVar3 = -1;
      DAT_006953f4 = iVar3;
      if ((iVar4 != 0) && (iVar1 = 0, 0 < in_EAX[1])) {
        piVar2 = (int *)*in_EAX;
        do {
          DAT_006953f4 = iVar1;
          if (*piVar2 == iVar4) break;
          iVar1 = iVar1 + 1;
          piVar2 = piVar2 + 1;
          DAT_006953f4 = iVar3;
        } while (iVar1 < in_EAX[1]);
      }
      if ((DAT_006953f4 != -1) &&
         ((DAT_006953f4 < DAT_00719478 || (DAT_00719478 + 0xf <= DAT_006953f4)))) {
        DAT_00719478 = DAT_006953f4;
        FUN_004b7360();
      }
    }
    in_EAX[3] = 0;
    server_browser_total_players_compute();
  }
  return;
}
#endif
