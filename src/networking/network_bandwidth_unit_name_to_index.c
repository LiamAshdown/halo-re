// network_bandwidth_unit_name_to_index  (Ghidra: network_bandwidth_unit_name_to_index,
// already named)
// address 0x4d8a20, size 44 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md summary ("Converts a 'bytes'/'packets' unit-name
// string into the corresponding index for the network bandwidth debug graph"); the two strings
// referenced ("bytes", "packets") match network_bandwidth_units_label_table, already named from
// src/networking/network_bandwidth_graph_instance_update_layout.c and
// src/networking/network_bandwidth_graph_update.c (0x0065d428, indexed by
// network_bandwidth_graph::units_index).
// register convention: the string to match arrives in EDI (unaff_EDI). // blam-cc: EDI -> name
// UNSURE: none.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const char *network_bandwidth_units_label_table[2]; // 0x0065d428

int32_t network_bandwidth_unit_name_to_index(const char *name) // blam-cc: EDI -> name
{
    int32_t i;

    i = 0;
    do {
        if (_stricmp(name, network_bandwidth_units_label_table[i]) == 0) {
            return i;
        }
        i = i + 1;
    } while (i < 2);
    return -1;
}

#if 0
Original Ghidra decompilation (0x4d8a20):

int network_bandwidth_unit_name_to_index(void)

{
  int iVar1;
  int iVar2;
  char *unaff_EDI;

  iVar2 = 0;
  do {
    iVar1 = __stricmp(unaff_EDI,(&PTR_s_bytes_0065d428)[iVar2]);
    if (iVar1 == 0) {
      return iVar2;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 2);
  return -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
