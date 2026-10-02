// network_listen_accept_pending_connection  (Ghidra: network_listen_accept_pending_connection, already named)
// address 0x4421b0, size 149 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/networking_types_notes.md "network_pending_connection (0x14)": the
// dword array `&DAT_0087bc0c` indexed by `count*5` is one network_pending_connection stride
// (0x14 == 5 dwords) *before* the documented array base (0x0087bc20), which is exactly what
// turns "pop the most recent" into `network_pending_connections[count - 1]`; the queue fields
// written (+0x00 socket, +0x0c flags bit0) match network_receive_queue.
// register convention: __cdecl, no arguments.
// UNSURE: the accept-configuration record passed to gt2Accept is a 4-dword stack
// record (result code, receive callback, two more code pointers); one of those two pointers
// (LAB_00441f30) is a raw code address in the unlifted gap between network_channel_receive_
// callback's end and network_channel_attempt_connect's start, and the other (FUN_0044ad80) is
// outside this session's address range -- both are declared only by address, never rewritten
// here, matching network_channels_open.c's precedent for gap trampolines.
// UNSURE: the trailing network_channel_get_remote_address()/network_address_to_string() calls
// are shown with no visible arguments; both callees' own files document an ESI (address-out)
// and EDI (queue) convention for the first and an EAX (address) convention for the second, so
// this rewrite introduces one local `s_network_address` scratch value to carry the pointer
// that must be threaded through both calls (ESI reused as EAX) -- the exact scratch location
// Ghidra's decompile omits, but not the effect (formatting the newly accepted peer's address).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t network_pending_connection_count; // 0x006f16d0
extern network_pending_connection network_pending_connections[k_network_pending_connection_count]; // 0x0087bc20

// network_listen_accept_config is the stack-only record passed to gt2Accept;
// it now lives in types/networking.h (folded from this file during the review pass).

extern void network_channel_gap_441f30(void); // raw code address, see UNSURE
extern void function_do_nothing(void); // 0x44ad80

extern int32_t gt2Accept(int32_t reply_socket, network_listen_accept_config *config); // foreign, GameSpy library
extern network_receive_queue *network_receive_queue_new(void); // 0x441bf0, this module
extern void gt2SetConnectionData(int32_t socket, network_receive_queue *queue); // foreign, GameSpy library // foreign, GameSpy library
extern void network_channel_receive_callback(void *handle, uint8_t *data, int32_t length); // 0x441ed0, this module
extern int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue); // 0x441ce0, this module
extern char *network_address_to_string(s_network_address *addr); // 0x440570, this module

// blam-cc: __cdecl, no arguments
// Pops the most recently queued pending connection (if any) and, if the transport accepts it,
// builds a fresh receive queue for it, flags it connection-oriented, and formats its remote
// address. Always decrements the pending count when there was an entry. Returns the new queue,
// or NULL if there was nothing pending, the transport rejected it, or the queue allocation
// failed.
network_receive_queue *network_listen_accept_pending_connection(void)
{
    network_receive_queue *queue;
    network_pending_connection *entry;
    network_listen_accept_config config;
    int32_t accepted;
    s_network_address remote_address; // see UNSURE

    queue = 0;
    if (0 < network_pending_connection_count) {
        entry = &network_pending_connections[network_pending_connection_count - 1];
        config.result = 0;
        config.receive_callback = (void *)network_channel_receive_callback;
        config.error_callback = (void *)network_channel_gap_441f30;
        config.connect_callback = (void *)function_do_nothing;
        accepted = gt2Accept(entry->reply_socket, &config);
        if (accepted == 1) {
            queue = network_receive_queue_new();
            if (queue != 0) {
                queue->socket = entry->reply_socket;
                gt2SetConnectionData(entry->reply_socket, queue);
                queue->flags = queue->flags | 1;
                network_channel_get_remote_address(&remote_address, queue);
                network_address_to_string(&remote_address);
            }
        }
        network_pending_connection_count = network_pending_connection_count - 1;
    }
    return queue;
}

#if 0
Original Ghidra decompilation (0x4421b0):

undefined4 * network_listen_accept_pending_connection(void)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 local_28;
  code *local_24;
  undefined1 *local_20;
  code *local_1c;

  puVar3 = (undefined4 *)0x0;
  if (0 < DAT_006f16d0) {
    local_28 = 0;
    local_24 = network_channel_receive_callback;
    local_20 = &LAB_00441f30;
    local_1c = FUN_0044ad80;
    iVar2 = thunk_FUN_0061ce80((&DAT_0087bc0c)[DAT_006f16d0 * 5],&local_28);
    if (iVar2 == 1) {
      puVar3 = network_receive_queue_new();
      if (puVar3 != (undefined4 *)0x0) {
        uVar1 = (&DAT_0087bc0c)[DAT_006f16d0 * 5];
        *puVar3 = uVar1;
        FUN_00614830(uVar1,puVar3);
        *(byte *)(puVar3 + 3) = *(byte *)(puVar3 + 3) | 1;
        network_channel_get_remote_address();
        network_address_to_string();
      }
    }
    DAT_006f16d0 = DAT_006f16d0 + -1;
  }
  return puVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
