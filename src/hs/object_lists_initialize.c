// object_lists_initialize  (Ghidra: object_lists_initialize, already named)
// address 0x48b250, size 80 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: CEA prototype string overlap on all three literals; types/hs.h
//   k_hs_object_list_header_count (0x30) / k_hs_object_list_reference_count (0x80).
// register convention: cc=__cdecl, no parameters.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_saved_games.h"


extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

// Creates the two global data arrays backing the object_list script type: the list headers, and
// the singly-linked reference nodes each list's chain is built from.
void object_lists_initialize(void)
{
    char name[256];

    object_list_header_data = game_state_new("object list header", k_hs_object_list_header_count, 0xc /* EBX at the original call */);
    sprintf(name, "%s reference", "list object");
    object_list_reference_data = game_state_new(name, k_hs_object_list_reference_count, 0xc /* EBX at the original call */);
}

#if 0
Original Ghidra decompilation (0x48b250):

void __cdecl object_lists_initialize(void)

{
  char local_100 [256];

  DAT_0087a464 = game_state_new("object list header",0x30);
  _sprintf(local_100,"%s reference","list object");
  DAT_0087a468 = game_state_new(local_100,0x80);
  return;
}
#endif
