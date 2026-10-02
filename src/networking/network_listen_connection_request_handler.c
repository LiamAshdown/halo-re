// network_listen_connection_request_handler  (Ghidra: network_listen_connection_request_handler, already named)
// address 0x442090, size 217 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md "network_pending_connection (0x14)" section:
// "network_listen_connection_request_handler (0x442090) refuses past 30 entries (0x1e <
// DAT_006f16d0) and writes five dwords at 0x0087bc20 + count*0x14 from its param_5, param_2,
// param_3, param_4 (low word) and *param_6". types/networking.h's network_error_code values 2
// and 3 match this function's two rejection paths exactly.
// register convention: none -- this is a genuine 7-parameter callback registered with the
// transport library (see network_listen_start.c), called with all parameters on the stack.
// UNSURE: the struct write of `remote_port` in the original packs it with the adjacent pad
// field into one 4-byte store whose upper 16 bits come from an uninitialized stack local
// (`local_c`); only the port itself is meaningful; the garbage that lands in the padding is not
// reproduced here.
// UNSURE: the `if (local_1c != 0)` guard on the second gt2Reject call is unreachable
// on the success path (local_1c is never set to nonzero there) but is kept exactly as
// decompiled rather than removed.

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

extern void gt2Reject(int32_t socket, void *buffer, int32_t length); // foreign, GameSpy library; send reply
extern void gt2GetSocketData(int32_t listen_handle); // foreign, GameSpy library
extern void gt2AddressToString(uint32_t address, uint16_t port, void *out_address); // 0x6148b0: fills a
    // 0x16-byte address record (stride confirmed by the imul esi,esi,0x16 at 0x6148c8) // foreign

// Refuses a new request once 30 are already queued, or if the payload is missing/too short
// (fewer than 4 bytes). Otherwise records the request in the next pending-connection slot.
void network_listen_connection_request_handler(int32_t listen_handle, int32_t reply_socket,
                                                 uint32_t remote_address, uint32_t remote_port_raw,
                                                 int32_t transport_handle, uint32_t *payload,
                                                 uint32_t payload_length)
{
    int32_t reply_code;
    uint32_t first_payload_word;
    int32_t index;
    uint8_t address_buf[24];

    reply_code = 0;
    if (k_network_pending_connection_count < network_pending_connection_count) {
        reply_code = k_network_listen_error_queue_full;
        gt2Reject(reply_socket, &reply_code, 4);
        return;
    }
    if (payload != 0 && 3 < payload_length) {
        first_payload_word = *payload;
        index = network_pending_connection_count;
        network_pending_connections[index].reply_socket = reply_socket;
        network_pending_connections[index].transport_handle = transport_handle;
        network_pending_connections[index].remote_address = remote_address;
        network_pending_connections[index].remote_port = (uint16_t)remote_port_raw;
        network_pending_connections[index].first_payload_word = first_payload_word;
        network_pending_connection_count = network_pending_connection_count + 1;
        gt2GetSocketData(listen_handle);
        gt2AddressToString(remote_address, (uint16_t)remote_port_raw, address_buf);
        if (reply_code != 0) {
            gt2Reject(reply_socket, &reply_code, 4);
        }
        return;
    }
    reply_code = k_network_listen_error_bad_payload;
    gt2Reject(reply_socket, &reply_code, 4);
}

#if 0
Original Ghidra decompilation (0x442090):

void network_listen_connection_request_handler
               (undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
               undefined4 param_5,undefined4 *param_6,uint param_7)

{
  int iVar1;
  undefined4 uVar2;
  int local_1c;
  undefined1 local_18 [12];
  undefined4 local_c;

  local_1c = 0;
  if (0x1e < DAT_006f16d0) {
    local_1c = 2;
    thunk_FUN_0061cee0(param_2,&local_1c,4);
    return;
  }
  if ((param_6 != (undefined4 *)0x0) && (3 < param_7)) {
    uVar2 = *param_6;
    iVar1 = DAT_006f16d0 * 0x14;
    *(undefined4 *)(&DAT_0087bc20 + iVar1) = param_2;
    *(undefined4 *)(&DAT_0087bc24 + iVar1) = param_5;
    *(undefined4 *)(&DAT_0087bc28 + iVar1) = param_3;
    local_c = CONCAT22(local_c._2_2_,(short)param_4);
    *(undefined4 *)(&DAT_0087bc2c + iVar1) = local_c;
    *(undefined4 *)(&DAT_0087bc30 + iVar1) = uVar2;
    DAT_006f16d0 = DAT_006f16d0 + 1;
    FUN_00614820(param_1);
    FUN_006148b0(param_3,param_4,local_18);
    if (local_1c != 0) {
      thunk_FUN_0061cee0(param_2,&local_1c,4);
    }
    return;
  }
  local_1c = 3;
  thunk_FUN_0061cee0(param_2,&local_1c,4);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
