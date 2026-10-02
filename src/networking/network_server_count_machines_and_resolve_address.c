// network_server_count_machines_and_resolve_address  (Ghidra: FUN_004e0d30, unnamed)
// address 0x4e0d30, size 438 bytes
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Counts occupied machine-table slots up to the
// first free one and, for a new connection, fetches its remote address via
// network_channel_get_remote_address." Ghidra's own decompilation removes eleven blocks as
// "unreachable", so most of this 438-byte function's body is not available here; only the
// surviving control flow is transcribed. param_1+6 matches network_server_globals::flags;
// param_1+0x3c4 matches ::machines[0].machine_id.
// register convention: EAX = passthrough (returned unchanged, masked, whenever the gate at
// server->flags bit0 is clear), ESI = address_out (s_network_address *, forwarded to
// network_channel_get_remote_address), stack = server (network_server_globals *),
// connection (network_receive_queue **, only ever tested for non-NULL and forwarded).
// blam-cc: EAX -> passthrough, ESI -> address_out, stack -> server, connection
// UNSURE: this function's true behaviour is materially larger than what survives Ghidra's
// dead-code elimination (it references network_channel_list_add,
// network_server_build_game_info_packet (FUN_004e0950) and
// network_server_build_full_game_info_packet (FUN_004e0bd0) as callees, none of which appear
// in the reachable code shown here); only the visible fragment is rewritten.
// UNSURE: the early-exit value inside the loop (`(uint)psVar1 & 0xffffff00`, the *pointer*
// one-past the 17th machine slot, masked) looks like leftover register reuse rather than a
// meaningful count; preserved literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue); // 0x441ce0

uint32_t network_server_count_machines_and_resolve_address(uint32_t eax_passthrough,
                                                             s_network_address *address_out,
                                                             network_server_globals *server,
                                                             network_receive_queue **connection)
{
    if ((server->flags & 1) != 0) {
        int32_t i;
        int16_t *machine_id_ptr;

        i = 0;
        machine_id_ptr = (int16_t *)((uint8_t *)server + 0x3c4);
        while (*machine_id_ptr != -1) {
            i = i + 1;
            machine_id_ptr = machine_id_ptr + 0x30;
            if (i > 15) {
                return ((uint32_t)machine_id_ptr) & 0xffffff00;
            }
        }
        if (*connection != 0) {
            network_channel_get_remote_address(address_out, *connection);
        }
        eax_passthrough = 0;
    }
    return eax_passthrough & 0xffffff00;
}

#if 0
Original Ghidra decompilation (0x4e0d30):

/* WARNING: Removing unreachable block (ram,0x004e0de3) */
/* WARNING: Removing unreachable block (ram,0x004e0ded) */
/* WARNING: Removing unreachable block (ram,0x004e0df4) */
/* WARNING: Removing unreachable block (ram,0x004e0e00) */
/* WARNING: Removing unreachable block (ram,0x004e0e6a) */
/* WARNING: Removing unreachable block (ram,0x004e0e73) */
/* WARNING: Removing unreachable block (ram,0x004e0e9b) */
/* WARNING: Removing unreachable block (ram,0x004e0ea1) */
/* WARNING: Removing unreachable block (ram,0x004e0eab) */
/* WARNING: Removing unreachable block (ram,0x004e0ec9) */
/* WARNING: Removing unreachable block (ram,0x004e0eb5) */

uint FUN_004e0d30(int param_1,int *param_2)

{
  uint in_EAX;
  short *psVar1;
  int iVar2;

  if ((*(byte *)(param_1 + 6) & 1) != 0) {
    iVar2 = 0;
    psVar1 = (short *)(param_1 + 0x3c4);
    while (*psVar1 != -1) {
      iVar2 = iVar2 + 1;
      psVar1 = psVar1 + 0x30;
      if (0xf < iVar2) {
        return (uint)psVar1 & 0xffffff00;
      }
    }
    if (*param_2 != 0) {
      network_channel_get_remote_address();
    }
    in_EAX = 0;
  }
  return in_EAX & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
