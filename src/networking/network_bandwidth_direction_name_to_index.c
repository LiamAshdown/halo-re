// network_bandwidth_direction_name_to_index  (Ghidra: FUN_004d8a50; renamed, no prior name)
// address 0x4d8a50, size 44 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md summary ("Converts a direction-name string (e.g.
// 'sent'/'recv') into the corresponding index for the network bandwidth debug graph"); identical
// shape to network_bandwidth_unit_name_to_index (0x4d8a20) one function above it, indexing
// network_bandwidth_direction_label_table, already named from
// src/networking/network_bandwidth_graph_instance_update_layout.c and
// src/networking/network_bandwidth_graph_update.c (0x0065d430, indexed by
// network_bandwidth_graph::direction_index). The two extra globals Ghidra lists
// (0x0066c2f0, 0x0066c2e8) are not referenced by this function's decompiled body and are
// omitted here.
// register convention: the string to match arrives in EDI (unaff_EDI). // blam-cc: EDI -> name
// UNSURE: none.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b
extern const char *network_bandwidth_direction_label_table[2]; // 0x0065d430

int32_t network_bandwidth_direction_name_to_index(const char *name) // blam-cc: EDI -> name
{
    int32_t i;

    i = 0;
    do {
        if (__stricmp(name, network_bandwidth_direction_label_table[i]) == 0) {
            return i;
        }
        i = i + 1;
    } while (i < 2);
    return -1;
}

#if 0
Original Ghidra decompilation (0x4d8a50):

int FUN_004d8a50(void)

{
  int iVar1;
  int iVar2;
  char *unaff_EDI;

  iVar2 = 0;
  do {
    iVar1 = __stricmp(unaff_EDI,(&PTR_DAT_0065d430)[iVar2]);
    if (iVar1 == 0) {
      return iVar2;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 2);
  return -1;
}
#endif
