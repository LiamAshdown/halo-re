// server_browser_sort_comparator_select  (Ghidra: server_browser_sort_comparator_select, already
// named)
// address 0x4ba970, size 50 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md summary ("Returns the qsort comparator function
// pointer matching the server browser's currently selected sort column"); the sort column
// global sits immediately before types/networking.h's server_browser_filters block
// (0x0071948b), matching the notes' "Unresolved: the sort-column index... separate scalar
// globals with no shared base".
// register convention: __cdecl, no parameters.
// UNSURE: three of the five comparators (0x4b6cd0, 0x4b6e70, 0x4b6fb0) are outside this batch's
// address range and still carry Ghidra LAB_ labels rather than function names; declared here by
// address only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t server_browser_sort_column; // 0x00719489, UNSURE name (see file header)


extern int32_t server_list_scroll_clamp(const void *, const void *); // 0x4b7360-adjacent, outside this batch
extern int32_t server_list_compare_by_ping_then_hostname(const void *, const void *);     // outside this batch
extern int32_t server_list_compare_by_gametype(const void *, const void *); // 0x4b6fb0, outside this batch, UNSURE name
extern int32_t server_list_compare_by_players(const void *, const void *); // 0x4b6e70, outside this batch, UNSURE name
extern int32_t server_list_compare_by_hostname(const void *, const void *); // 0x4b6cd0, outside this batch, UNSURE name

// blam-cc: no register inputs
server_browser_sort_comparator server_browser_sort_comparator_select(void)
{
    switch (server_browser_sort_column) {
    case 1:
        return server_list_scroll_clamp;
    case 2:
        return server_list_compare_by_gametype;
    case 3:
        return server_list_compare_by_ping_then_hostname;
    case 4:
        return server_list_compare_by_players;
    default:
        return server_list_compare_by_hostname;
    }
}

#if 0
Original Ghidra decompilation (0x4ba970):

void * __cdecl server_browser_sort_comparator_select(void)

{
  switch(DAT_00719489) {
  case 1:
    return server_list_compare_by_mapname_then_hostname;
  case 2:
    return &LAB_004b6fb0;
  case 3:
    return server_list_compare_by_ping_then_hostname;
  case 4:
    return &LAB_004b6e70;
  default:
    return &LAB_004b6cd0;
  }
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
