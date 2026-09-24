// main_queue_cache_file_open  (Ghidra: FUN_004c8900; still unnamed -> renamed)
// address 0x4c8900, size 46 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: out/phase4/main_types_notes.md "Register arguments confirmed at call sites":
// "FUN_004c8900 0x4c8900: EAX = cache file name or NULL (0x45aeea)". types/main.h pins
// 0x00719774 as main_globals.cache_file_open_pending and 0x00719979 as
// main_globals.pending_cache_file_name[0] ("cache_file_open_pending: open
// pending_cache_file_name (FUN_004c8900 sets)").
// register convention: EAX -> name.

#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include <string.h>

extern main_globals main_globals_data; // 0x00719700

// blam-cc: EAX -> name
// Stages name as the cache file for the main loop to open next frame, or clears the request
// when name is NULL.
void main_queue_cache_file_open(char *name)
{
    if (name != 0) {
        strncpy(main_globals_data.pending_cache_file_name, name, 0xff);
        main_globals_data.cache_file_open_pending = 1;
    } else {
        main_globals_data.pending_cache_file_name[0] = 0;
        main_globals_data.cache_file_open_pending = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4c8900):

void FUN_004c8900(void)

{
  char *in_EAX;

  if (in_EAX != (char *)0x0) {
    _strncpy(&DAT_00719979,in_EAX,0xff);
    DAT_00719774 = 1;
    return;
  }
  DAT_00719979 = 0;
  DAT_00719774 = 0;
  return;
}
#endif
