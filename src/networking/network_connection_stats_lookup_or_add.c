// network_connection_stats_lookup_or_add  (Ghidra: network_connection_stats_lookup_or_add,
// already named)
// address 0x440a80, size 160 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: out/phase4/networking_types_notes.md "network_connection_statistics (0x44) and
// network_summary_statistics (0x1c)": the stride is pinned three ways in this exact function
// ("(&DAT_0087becc)[i*0x11]" dword array, "(&DAT_0087bed0)[i*0x22]" word array and
// "(&DAT_0087bec8)[i*0x44]" byte array all resolve to the same element").
// register convention: connection id in EBX (unaff_EBX), connection key in DI, the low word
// of EDI (unaff_DI). Ghidra types this void; callers (network_connection_stats_record_packet, network_connection_stats_end)
// both use its result as an index, so the found/new slot index is returned via the implicit
// EAX convention (same situation as src/memory/circular_buffer_new.c).
// UNSURE: once network_connection_stats_count reaches k_network_connection_stats_count (255)
// it stops growing, but a new record's index is still taken from the un-clamped count value,
// so a 256th distinct connection writes one record past the end of the array. Preserved
// exactly as decompiled; not a rewrite bug.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_connection_stats_count; // 0x006f14bc
extern network_connection_statistics network_connection_stats[k_network_connection_stats_count]; // 0x0087bec0

// blam-cc: connection id in EBX (unaff_EBX), connection key in DI (unaff_DI, low word of EDI)
int32_t network_connection_stats_lookup_or_add(int32_t connection_id, uint16_t connection_key)
{
    int32_t count;
    int32_t i;

    count = network_connection_stats_count;
    i = 0;
    if (0 < network_connection_stats_count) {
        do {
            if (network_connection_stats[i].connection_id == connection_id &&
                network_connection_stats[i].connection_key == connection_key) {
                return i;
            }
            i = i + 1;
        } while (i < network_connection_stats_count);
    }
    if (network_connection_stats_count < 0xff) {
        network_connection_stats_count = network_connection_stats_count + 1;
    }
    network_connection_stats[count].connection_id = connection_id;
    network_connection_stats[count].connection_key = connection_key;
    network_connection_stats[count].connected_duration_ms = 0;
    network_connection_stats[count].bytes_sent = 0;
    network_connection_stats[count].bytes_received = 0;
    network_connection_stats[count].reliable_bytes_sent = 0;
    network_connection_stats[count].resend_bytes_sent = 0;
    network_connection_stats[count].interval_bytes_sent = 0;
    network_connection_stats[count].interval_bytes_received = 0;
    network_connection_stats[count].interval_reliable_bytes_sent = 0;
    network_connection_stats[count].interval_resend_bytes_sent = 0;
    network_connection_stats[count].packets_sent = 0;
    network_connection_stats[count].packets_received = 0;
    network_connection_stats[count].interval_packets_sent = 0;
    network_connection_stats[count].interval_packets_received = 0;
    return count;
}

#if 0
Original Ghidra decompilation (0x440a80):

void network_connection_stats_lookup_or_add(void)

{
  int iVar1;
  int iVar2;
  short *psVar3;
  int unaff_EBX;
  short unaff_DI;

  iVar1 = DAT_006f14bc;
  iVar2 = 0;
  if (0 < DAT_006f14bc) {
    psVar3 = &DAT_0087bed0;
    do {
      if ((*(int *)(psVar3 + -2) == unaff_EBX) && (*psVar3 == unaff_DI)) {
        if (iVar2 != -1) {
          return;
        }
        break;
      }
      iVar2 = iVar2 + 1;
      psVar3 = psVar3 + 0x22;
    } while (iVar2 < DAT_006f14bc);
  }
  if (DAT_006f14bc < 0xff) {
    DAT_006f14bc = DAT_006f14bc + 1;
  }
  iVar2 = iVar1 * 0x44;
  (&DAT_0087becc)[iVar1 * 0x11] = unaff_EBX;
  (&DAT_0087bed0)[iVar1 * 0x22] = unaff_DI;
  (&DAT_0087bec0)[iVar1 * 0x11] = 0;
  (&DAT_0087bed4)[iVar1 * 0x11] = 0;
  (&DAT_0087bed8)[iVar1 * 0x11] = 0;
  *(undefined4 *)(&DAT_0087bedc + iVar2) = 0;
  *(undefined4 *)(&DAT_0087bee0 + iVar2) = 0;
  (&DAT_0087bee4)[iVar1 * 0x11] = 0;
  (&DAT_0087bee8)[iVar1 * 0x11] = 0;
  (&DAT_0087beec)[iVar1 * 0x11] = 0;
  (&DAT_0087bef0)[iVar1 * 0x11] = 0;
  *(undefined4 *)(&DAT_0087bef4 + iVar2) = 0;
  *(undefined4 *)(&DAT_0087bef8 + iVar2) = 0;
  (&DAT_0087befc)[iVar1 * 0x11] = 0;
  *(undefined4 *)(&DAT_0087bf00 + iVar2) = 0;
  return;
}
#endif
