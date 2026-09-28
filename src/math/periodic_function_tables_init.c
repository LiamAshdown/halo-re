// periodic_function_tables_init  (Ghidra: periodic_function_tables_init, already named)
// address 0x4cc8d0, size 135 bytes
// name confidence: 0.65   rewrite confidence: 0.75
// evidence: types/math.h periodic_function/transition_function sections (12 tables of 0x400
//   bytes at periodic_function_tables, 6 at transition_function_tables; forces
//   random_seed_global to 0x20f3f660 first so the tables are reproducible).
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "math.h"

extern void *__stdcall GlobalAlloc(uint32_t flags, uint32_t bytes); // 0x0063a0b0 import thunk
extern void periodic_function_build_table(periodic_function_t type, uint8_t *out); // 0x4ccdb0
extern void periodic_function_build_transition_table(transition_function_t type, uint8_t *table); // 0x4cccb0

extern periodic_function_table *periodic_function_tables[12]; // 0x006b7aa8
extern periodic_function_table *transition_function_tables[6]; // 0x006b7ad8
extern uint8_t periodic_functions_initialized; // 0x006b7af0
extern random_seed random_seed_global; // 0x00719cd0

// Allocates and fills the byte lookup tables backing the engine's periodic (wave) function
// evaluator.
void periodic_function_tables_init(void)
{
    int16_t i;
    uint8_t *table;

    periodic_functions_initialized = 1;
    random_seed_global = 0x20f3f660;

    for (i = 0; i < k_periodic_function_count; i++) {
        table = (uint8_t *)GlobalAlloc(0, k_periodic_function_table_size);
        periodic_function_tables[i] = (periodic_function_table *)table;
        if (table == 0) {
            periodic_functions_initialized = 0;
        } else {
            periodic_function_build_table(i, table);
        }
    }

    for (i = 0; i < k_transition_function_count; i++) {
        table = (uint8_t *)GlobalAlloc(0, k_periodic_function_table_size);
        transition_function_tables[i] = (periodic_function_table *)table;
        if (table == 0) {
            periodic_functions_initialized = 0;
        } else {
            periodic_function_build_transition_table(i, table);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4cc8d0):

void __cdecl periodic_function_tables_init(void)

{
  uchar *out;
  HGLOBAL pvVar1;
  undefined4 *puVar2;
  short sVar3;

  DAT_006b7af0 = 1;
  DAT_00719cd0 = 0x20f3f660;
  sVar3 = 0;
  puVar2 = &DAT_006b7aa8;
  do {
    out = GlobalAlloc(0,0x400);
    *puVar2 = out;
    if (out == (uchar *)0x0) {
      DAT_006b7af0 = 0;
    }
    else {
      periodic_function_build_table(sVar3,out);
    }
    sVar3 = sVar3 + 1;
    puVar2 = puVar2 + 1;
  } while (sVar3 < 0xc);
  sVar3 = 0;
  puVar2 = &DAT_006b7ad8;
  do {
    pvVar1 = GlobalAlloc(0,0x400);
    *puVar2 = pvVar1;
    if (pvVar1 == (HGLOBAL)0x0) {
      DAT_006b7af0 = 0;
    }
    else {
      FUN_004cccb0(pvVar1);
    }
    sVar3 = sVar3 + 1;
    puVar2 = puVar2 + 1;
  } while (sVar3 < 6);
  return;
}
#endif
