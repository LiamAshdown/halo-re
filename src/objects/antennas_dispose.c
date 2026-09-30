// antennas_dispose  (Ghidra: antennas_dispose, already named)
// address 0x4faa40, size 18 bytes
// name confidence: 0.55 (already carries this name; matches functions.md's summary: "Marks the
//   antenna data array as disposing and deletes all antenna instances")
// rewrite confidence: 0.75
// evidence: types/memory.h data_array (valid 0x24); global 0x008603ac antenna_data.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_memory.h"

extern data_array *antenna_data; // 0x008603ac


void antennas_dispose(void)
{
    antenna_data->valid = 1;
    data_delete_all(antenna_data);
}

#if 0
Original Ghidra decompilation (0x4faa40):

void antennas_dispose(void)

{
  *(undefined1 *)(DAT_008603ac + 0x24) = 1;
  data_delete_all();
  return;
}
#endif
