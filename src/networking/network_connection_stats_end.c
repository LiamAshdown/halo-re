// network_connection_stats_end  (Ghidra: network_connection_stats_end, already named)
// address 0x440d20, size 82 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/networking_types_notes.md "network_connection_statistics (0x44)":
// "network_connection_stats_end (0x440d20) does +0x00 += now - +0x04 and clears +0x04 and the
// +0x08 byte, which names the first three fields."
// register convention: __cdecl per Ghidra's own signature, but the body calls
// network_connection_stats_lookup_or_add with no arguments of its own -- this function never
// touches EBX/DI itself, so it is simply passing its caller's connection id (EBX) and
// connection key (DI) straight through untouched. Declared here as ordinary parameters so
// that pass-through is explicit; see network_connection_stats_lookup_or_add.c for the same
// register convention on the callee side.
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern uint8_t debug_log_level;          // 0x0087ac06, byte-wide (R01)
extern network_connection_statistics network_connection_stats[k_network_connection_stats_count]; // 0x0087bec0


extern int32_t time_query_performance_counter_ms(void); // foreign module, millisecond tick reader

// blam-cc: connection id in EBX, connection key in DI -- both forwarded unchanged to
// network_connection_stats_lookup_or_add
void network_connection_stats_end(int32_t connection_id, uint16_t connection_key)
{
    int32_t index;
    int32_t now;

    // Short-circuit order matters: lookup_or_add (which can grow the stats table) only runs
    // once statistics logging is actually enabled, exactly as in the original.
    if (2 < debug_log_level &&
        (index = network_connection_stats_lookup_or_add(connection_id, connection_key), index != -1) &&
        network_connection_stats[index].active != 0) {
        now = time_query_performance_counter_ms();
        network_connection_stats[index].connected_duration_ms +=
            now - network_connection_stats[index].active_since_ms;
        network_connection_stats[index].active_since_ms = 0;
        network_connection_stats[index].active = 0;
    }
}

#if 0
Original Ghidra decompilation (0x440d20):

void __cdecl network_connection_stats_end(void)

{
  int iVar1;
  int iVar2;

  if (((2 < DAT_0087ac06) && (iVar1 = network_connection_stats_lookup_or_add(), iVar1 != -1)) &&
     ((&DAT_0087bec8)[iVar1 * 0x44] != '\0')) {
    iVar2 = FUN_00449210();
    (&DAT_0087bec0)[iVar1 * 0x11] =
         (&DAT_0087bec0)[iVar1 * 0x11] + (iVar2 - (&DAT_0087bec4)[iVar1 * 0x11]);
    (&DAT_0087bec4)[iVar1 * 0x11] = 0;
    (&DAT_0087bec8)[iVar1 * 0x44] = 0;
  }
  return;
}
#endif
