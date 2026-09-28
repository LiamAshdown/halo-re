// network_connection_stats_record_packet  (Ghidra: FUN_00440b20, still unnamed -> renamed)
// address 0x440b20, size 505 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("updates a connection's per-packet
// statistics counters ... for one transmitted or received packet"); out/phase4/
// networking_types_notes.md "network_connection_statistics (0x44)" names every field this
// function touches; the "misattributed" note explains that 0x440b20 (with 0x440d80) is what
// let the session offsets (network_client + 0xb14, network_server + 8) be cross-checked, and
// that the GameSpy connection object this function keys off of is foreign library data, not a
// Blam struct.
// register convention: GameSpy connection handle in EAX (in_EAX), payload byte length in ECX
// (in_ECX), then three Ghidra-recognized stack byte flags: is_sent, is_reliable, is_resend.
// FIXED in the review pass: gamespy_array_length and gt2GetRemotePort are one-instruction GameSpy
// accessors (0x6175f0 returns the uint32 at object+0x00; 0x6147d0 returns the uint16 at
// object+0x04 in AX), and the disassembly at 0x440b8a..0x440b9b is
//   push edi / call 0x6175f0 / push edi / mov ebx,eax / call 0x6147d0 / movzx edi,ax
// followed by the call to 0x440a80, so they supply exactly the connection id and key
// network_connection_stats_lookup_or_add needs, both read off the GameSpy connection and
// the key zero-extended from 16 bits.
// UNSURE: `0x1c` added to the payload length is assumed to be a fixed per-packet header/
// overhead byte count (e.g. IP+UDP+protocol headers), not independently confirmed.
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>

extern uint8_t debug_log_level;          // 0x0087ac06, byte-wide (R01)
extern uint8_t network_statistics_logging_enabled; // 0x006f14b4
extern network_connection_statistics network_connection_stats[k_network_connection_stats_count]; // 0x0087bec0
extern network_summary_statistics network_summary_stats; // 0x0087bea0

extern void network_bandwidth_graph_accumulate_sent(int32_t enabled); // 0x4d79d0, this module (sent bandwidth graph)
extern void network_bandwidth_graph_accumulate_received(int32_t enabled); // 0x4d7a50, this module (received bandwidth graph)
extern void *gt2GetConnectionData(void *gamespy_connection); // foreign, GameSpy library
extern uint32_t gamespy_array_length(int32_t object); // 0x6175f0: returns the uint32 at object+0x00 // foreign, GameSpy library; see UNSURE
extern uint16_t gt2GetRemotePort(int32_t object); // 0x6147d0: returns the uint16 at object+0x04 in AX // foreign, GameSpy library; see UNSURE
extern int32_t network_connection_stats_lookup_or_add(int32_t connection_id, uint16_t connection_key); // 0x440a80, this module
extern int32_t time_query_performance_counter_ms(void); // foreign module, millisecond tick reader

// blam-cc: GameSpy connection handle in EAX (in_EAX), payload byte length in ECX (in_ECX),
// is_sent/is_reliable/is_resend as ordinary stack parameters (param_1/2/3)
void network_connection_stats_record_packet(void *gamespy_connection, int32_t payload_length,
                                             uint8_t is_sent, uint8_t is_reliable, uint8_t is_resend)
{
    int32_t total_bytes;
    int32_t *stats_index_field;
    int32_t index;

    if (2 < debug_log_level) {
        total_bytes = payload_length + 0x1c;
        if (is_sent == 0) {
            network_bandwidth_graph_accumulate_received(1);
        } else {
            network_bandwidth_graph_accumulate_sent(1);
        }
        if (network_statistics_logging_enabled == 1 && gamespy_connection != 0 &&
            (gamespy_connection = gt2GetConnectionData(gamespy_connection), gamespy_connection != 0)) {
            stats_index_field = (int32_t *)((uint8_t *)gamespy_connection + 0x14);
            if (*stats_index_field == -1) {
                index = network_connection_stats_lookup_or_add(
                    (int32_t)gamespy_array_length((int32_t)(uintptr_t)gamespy_connection),
                    gt2GetRemotePort((int32_t)(uintptr_t)gamespy_connection));
                *stats_index_field = index;
                network_connection_stats[index].active = 1;
                network_connection_stats[*stats_index_field].active_since_ms = time_query_performance_counter_ms();
            }
            index = *stats_index_field;
            if (is_sent == 1) {
                network_connection_stats[index].bytes_sent += total_bytes;
                network_connection_stats[index].interval_bytes_sent += total_bytes;
                network_connection_stats[index].packets_sent += 1;
                network_connection_stats[index].interval_packets_sent += 1;
                if (is_reliable == 1) {
                    network_connection_stats[index].reliable_bytes_sent += total_bytes;
                    network_connection_stats[index].interval_reliable_bytes_sent += total_bytes;
                }
                if (is_resend == 1) {
                    network_connection_stats[index].resend_bytes_sent += total_bytes;
                    network_connection_stats[index].interval_resend_bytes_sent += total_bytes;
                }
                network_summary_stats.packets_sent += 1;
                network_summary_stats.bytes_sent += total_bytes;
                return;
            }
            network_connection_stats[index].bytes_received += total_bytes;
            network_connection_stats[index].interval_bytes_received += total_bytes;
            network_connection_stats[index].packets_received += 1;
            network_connection_stats[index].interval_packets_received += 1;
            network_summary_stats.packets_received += 1;
            network_summary_stats.bytes_received += total_bytes;
        }
    }
}

#if 0
Original Ghidra decompilation (0x440b20):

void FUN_00440b20(char param_1,char param_2,char param_3)

{
  int in_EAX;
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int in_ECX;
  int iVar4;

  if (2 < DAT_0087ac06) {
    iVar4 = in_ECX + 0x1c;
    if (param_1 == '\0') {
      FUN_004d7a50(1);
    }
    else {
      FUN_004d79d0(1);
    }
    if (((DAT_006f14b4 == '\x01') && (in_EAX != 0)) && (iVar1 = FUN_00614840(), iVar1 != 0)) {
      if (*(int *)(iVar1 + 0x14) == -1) {
        FUN_006175f0();
        FUN_006147d0();
        iVar2 = network_connection_stats_lookup_or_add();
        *(int *)(iVar1 + 0x14) = iVar2;
        (&DAT_0087bec8)[iVar2 * 0x44] = 1;
        uVar3 = FUN_00449210();
        (&DAT_0087bec4)[*(int *)(iVar1 + 0x14) * 0x11] = uVar3;
      }
      iVar2 = *(int *)(iVar1 + 0x14);
      if (param_1 == '\x01') {
        (&DAT_0087bed4)[iVar2 * 0x11] = (&DAT_0087bed4)[iVar2 * 0x11] + iVar4;
        (&DAT_0087bee4)[*(int *)(iVar1 + 0x14) * 0x11] =
             (&DAT_0087bee4)[*(int *)(iVar1 + 0x14) * 0x11] + iVar4;
        iVar2 = *(int *)(iVar1 + 0x14) * 0x44;
        *(int *)(&DAT_0087bef4 + iVar2) = *(int *)(&DAT_0087bef4 + iVar2) + 1;
        (&DAT_0087befc)[*(int *)(iVar1 + 0x14) * 0x11] =
             (&DAT_0087befc)[*(int *)(iVar1 + 0x14) * 0x11] + 1;
        if (param_2 == '\x01') {
          iVar2 = *(int *)(iVar1 + 0x14) * 0x44;
          *(int *)(&DAT_0087bedc + iVar2) = *(int *)(&DAT_0087bedc + iVar2) + iVar4;
          (&DAT_0087beec)[*(int *)(iVar1 + 0x14) * 0x11] =
               (&DAT_0087beec)[*(int *)(iVar1 + 0x14) * 0x11] + iVar4;
        }
        if (param_3 == '\x01') {
          iVar2 = *(int *)(iVar1 + 0x14) * 0x44;
          *(int *)(&DAT_0087bee0 + iVar2) = *(int *)(&DAT_0087bee0 + iVar2) + iVar4;
          (&DAT_0087bef0)[*(int *)(iVar1 + 0x14) * 0x11] =
               (&DAT_0087bef0)[*(int *)(iVar1 + 0x14) * 0x11] + iVar4;
        }
        DAT_0087beac = DAT_0087beac + 1;
        DAT_0087bea4 = DAT_0087bea4 + iVar4;
        return;
      }
      (&DAT_0087bed8)[iVar2 * 0x11] = (&DAT_0087bed8)[iVar2 * 0x11] + iVar4;
      (&DAT_0087bee8)[*(int *)(iVar1 + 0x14) * 0x11] =
           (&DAT_0087bee8)[*(int *)(iVar1 + 0x14) * 0x11] + iVar4;
      iVar2 = *(int *)(iVar1 + 0x14) * 0x44;
      *(int *)(&DAT_0087bef8 + iVar2) = *(int *)(&DAT_0087bef8 + iVar2) + 1;
      iVar1 = *(int *)(iVar1 + 0x14) * 0x44;
      *(int *)(&DAT_0087bf00 + iVar1) = *(int *)(&DAT_0087bf00 + iVar1) + 1;
      DAT_0087beb0 = DAT_0087beb0 + 1;
      DAT_0087bea8 = DAT_0087bea8 + iVar4;
    }
  }
  return;
}
#endif
