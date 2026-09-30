// flags_dispose
// address 0x4fb4f0, size 18 bytes
// name confidence: 0.6 (functions.md: "Marks the flag data array as disposing and deletes all
//   flag instances")
// rewrite confidence: 0.75
// evidence: identical shape to lights_dispose_all.c's `array->valid = 1; data_delete_all(array)`
//   pair; types/memory.h data_array.valid at 0x24.
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_memory.h"

extern data_array *flag_data; // 0x008603a8


    // as in lights_dispose_all.c; memory module, 0x4d0580

void flags_dispose(void)
{
    flag_data->valid = 1;
    data_delete_all(flag_data); // UNSURE: Ghidra shows no visible arguments
}

#if 0
Original Ghidra decompilation (0x4fb4f0):

void flags_dispose(void)

{
  *(undefined1 *)(DAT_008603a8 + 0x24) = 1;
  data_delete_all();
  return;
}
#endif
