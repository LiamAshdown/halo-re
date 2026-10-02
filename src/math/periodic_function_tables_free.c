// periodic_function_tables_free  (Ghidra: periodic_function_tables_free, already named)
// address 0x4cc960, size 75 bytes
// name confidence: 0.65   rewrite confidence: 0.85
// evidence: types/math.h periodic_function section; frees the same 12+6 tables
// periodic_function_tables_init @0x4cc8d0 allocates, in the same order, and clears the
// initialized flag.
// register convention: __cdecl, no arguments.

#include "win32.h"
#include "tags.h"
#include "math.h"


#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern periodic_function_table *periodic_function_tables[12]; // 0x006b7aa8
extern periodic_function_table *transition_function_tables[6]; // 0x006b7ad8
extern uint8_t periodic_functions_initialized; // 0x006b7af0

// Frees the periodic-function lookup tables allocated by periodic_function_tables_init.
void periodic_function_tables_free(void)
{
    int16_t i;

    if (periodic_functions_initialized != 0) {
        for (i = 0; i < 12; i++) {
            GlobalFree(periodic_function_tables[i]);
        }
        for (i = 0; i < 6; i++) {
            GlobalFree(transition_function_tables[i]);
        }
        periodic_functions_initialized = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4cc960):

void __cdecl periodic_function_tables_free(void)

{
  int iVar1;
  undefined4 *puVar2;

  if (DAT_006b7af0 != '\0') {
    puVar2 = &DAT_006b7aa8;
    iVar1 = 0xc;
    do {
      GlobalFree((HGLOBAL)*puVar2);
      puVar2 = puVar2 + 1;
      iVar1 = iVar1 + -1;
    } while (iVar1 != 0);
    puVar2 = &DAT_006b7ad8;
    iVar1 = 6;
    do {
      GlobalFree((HGLOBAL)*puVar2);
      puVar2 = puVar2 + 1;
      iVar1 = iVar1 + -1;
    } while (iVar1 != 0);
    DAT_006b7af0 = '\0';
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
