// cache_file_find_slot_by_name  (Ghidra: cache_file_find_slot_by_name, already named)
// address 0x443770, size 51 bytes
// name confidence: 0.65   rewrite confidence: 0.85
// evidence: out/phase4/cache_types_notes.md cache_file_slot section: "DAT_006a9454 is
// header+0x20 (the name cache_file_find_slot_by_name compares)"; loop bound 6 matches
// k_cache_file_slot_count.
// register convention: filename in EDI (unaff_EDI).

#include "crt.h"
#include "tags.h"
#include "cache.h"

extern cache_file_slot cache_file_slots[k_cache_file_slot_count]; // 0x006a9428


// blam-cc: filename in EDI (unaff_EDI)
// Searches the fixed 6-entry array of open cache-file slots for one whose header name matches
// filename (case-insensitive), returning its index, or -1 if none matches.
int16_t cache_file_find_slot_by_name(char *filename)
{
    int16_t slot_index;

    slot_index = 0;
    do {
        if (_stricmp(filename, cache_file_slots[slot_index].header.name) == 0) {
            return slot_index;
        }
        slot_index = slot_index + 1;
    } while (slot_index < k_cache_file_slot_count);
    return -1;
}

#if 0
Original Ghidra decompilation (0x443770):

short cache_file_find_slot_by_name(void)

{
  int iVar1;
  short sVar2;
  char *unaff_EDI;

  sVar2 = 0;
  do {
    iVar1 = __stricmp(unaff_EDI,&DAT_006a9454 + sVar2 * 0x80c);
    if (iVar1 == 0) {
      return sVar2;
    }
    sVar2 = sVar2 + 1;
  } while (sVar2 < 6);
  return -1;
}
#endif
