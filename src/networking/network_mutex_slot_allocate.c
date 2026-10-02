// network_mutex_slot_allocate  (Ghidra: network_mutex_slot_allocate, already named)
// address 0x440420, size 59 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/networking_types_notes.md "network_mutex_record (0x28)"; the loop
// strides `network_mutex_table` by sizeof(network_mutex_record), tests each slot's
// `in_use` byte, and the bound 0x6f12d4 against a first in-use-byte address of 0x6f0dd4
// gives exactly k_network_mutex_table_count (32) slots.
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_mutex_record network_mutex_table[k_network_mutex_table_count]; // 0x006f0db0

// Finds the first free slot in the static mutex-record table, marks it in use, clears its
// handle and the first byte of its name, and returns a pointer to it; returns NULL if every
// slot is already in use.
network_mutex_record *network_mutex_slot_allocate(void)
{
    uint32_t i;

    for (i = 0; i < k_network_mutex_table_count; i++) {
        if (network_mutex_table[i].in_use == 0) {
            network_mutex_table[i].name[0] = 0;
            network_mutex_table[i].handle = 0;
            network_mutex_table[i].in_use = 1;
            return &network_mutex_table[i];
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x440420):

void * __cdecl network_mutex_slot_allocate(void)

{
  char *pcVar1;
  int iVar2;

  iVar2 = 0;
  pcVar1 = &DAT_006f0dd4;
  do {
    if (*pcVar1 == '\0') {
      iVar2 = iVar2 * 0x28;
      *(undefined1 *)(iVar2 + 0x6f0db4) = 0;
      *(undefined4 *)(iVar2 + 0x6f0db0) = 0;
      (&DAT_006f0dd4)[iVar2] = 1;
      return (undefined4 *)(iVar2 + 0x6f0db0);
    }
    pcVar1 = pcVar1 + 0x28;
    iVar2 = iVar2 + 1;
  } while ((int)pcVar1 < 0x6f12d4);
  return (void *)0x0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
