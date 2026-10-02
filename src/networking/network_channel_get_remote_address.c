// network_channel_get_remote_address  (Ghidra: network_channel_get_remote_address, already named)
// address 0x441ce0, size 276 bytes
// name confidence: 0.6   rewrite confidence: 0.5
// evidence: out/phase4/networking_types_notes.md's s_network_address section and
// network_error_code enum: the out-parameter's fields (dword at +0x00, word at +0x12, word at
// +0x10 set to 4, i.e. k_network_address_size_ipv4) and the -15/0xfff1 error code
// (k_network_error_no_address) line up exactly; the queue pointer's +0x00 (socket) and +0x0e
// (last_error) fields match network_receive_queue.
// FIXED in the review pass: Ghidra declared this void, but it returns its error code in EAX --
// 0 on both success paths (0x441d4f / 0x441dc9 reload the zero from [esp+0x8]) and 0xfffffff1
// (k_network_error_no_address) at 0x441de1 -- and 0x4dd390 tests exactly that (`test ax,ax`).
// The same value is also written to queue->last_error, which is why the void model still
// behaved correctly at the one call site that checked it.
// register convention: out s_network_address * in ESI (unaff_ESI), network_receive_queue * in
// EDI (unaff_EDI).
// UNSURE: gamespy_array_length/gt2GetLocalIP are each called four times with the identical argument and
// their four results are combined with a mask/shift expression that is algebraically a full
// 32-bit byte-swap of a single such call's result (consistent with converting a network-order
// GameSpy address into this struct's host-order-high-byte-first layout) -- kept as four
// separate calls, exactly as decompiled, rather than collapsed into one, since these are
// genuine calls whose independence was not confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t network_game_socket; // 0x006f14c4

extern int32_t gt2GetConnectionState(int32_t socket); // foreign, GameSpy library; connection state
extern uint32_t gamespy_array_length(int32_t object); // 0x6175f0: returns the uint32 at object+0x00 // foreign, GameSpy library; address byte source
extern uint16_t gt2GetRemotePort(int32_t object); // 0x6147d0: returns the uint16 at object+0x04 in AX // foreign, GameSpy library; port
extern uint32_t gt2GetLocalIP(int32_t socket); // foreign, GameSpy library; fallback address byte source
extern uint16_t gt2GetLocalPort(int32_t socket); // foreign, GameSpy library; fallback port

// blam-cc: out address in ESI (unaff_ESI), receive-queue pointer in EDI (unaff_EDI)
// Fills `address` from `queue`'s own socket if it has one and is connected (state 1);
// otherwise falls back to the shared network_game_socket if that is open; otherwise zeroes the
// address and reports k_network_error_no_address through queue->last_error. Always sets
// address->size to k_network_address_size_ipv4 and clears queue->last_error on success.
int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue)
{
    int32_t state;
    uint32_t byte0;
    uint32_t byte1;
    uint32_t byte2;
    uint32_t byte3;

    if (queue->socket != 0) {
        state = gt2GetConnectionState(queue->socket);
        if (state == 1) {
            byte0 = gamespy_array_length(queue->socket);
            byte1 = gamespy_array_length(queue->socket);
            byte2 = gamespy_array_length(queue->socket);
            byte3 = gamespy_array_length(queue->socket);
            address->ipv4 = ((byte0 & 0xff0000 | byte1 >> 0x10) >> 8) |
                            ((byte2 << 0x10 | byte3 & 0xff00) << 8);
            address->port = gt2GetRemotePort(queue->socket);
            address->size = k_network_address_size_ipv4;
            queue->last_error = 0;
            return 0;
        }
    }
    if (network_game_socket != 0) {
        byte0 = gt2GetLocalIP(network_game_socket);
        byte1 = gt2GetLocalIP(network_game_socket);
        byte2 = gt2GetLocalIP(network_game_socket);
        byte3 = gt2GetLocalIP(network_game_socket);
        address->ipv4 = ((byte0 & 0xff0000 | byte1 >> 0x10) >> 8) |
                        ((byte2 << 0x10 | byte3 & 0xff00) << 8);
        address->port = gt2GetLocalPort(network_game_socket);
        address->size = k_network_address_size_ipv4;
        queue->last_error = 0;
        return 0;
    }
    address->ipv4 = 0;
    address->port = 0;
    address->size = k_network_address_size_ipv4;
    queue->last_error = (int16_t)k_network_error_no_address;
    return (int16_t)k_network_error_no_address;
}

#if 0
Original Ghidra decompilation (0x441ce0):

void network_channel_get_remote_address(void)

{
  undefined2 uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint *unaff_ESI;
  int *unaff_EDI;

  if (*unaff_EDI != 0) {
    iVar2 = FUN_006147a0(*unaff_EDI);
    if (iVar2 == 1) {
      uVar3 = FUN_006175f0(*unaff_EDI);
      uVar4 = FUN_006175f0(*unaff_EDI);
      iVar2 = FUN_006175f0(*unaff_EDI);
      uVar5 = FUN_006175f0(*unaff_EDI);
      *unaff_ESI = (uVar3 & 0xff0000 | uVar4 >> 0x10) >> 8 | (iVar2 << 0x10 | uVar5 & 0xff00) << 8;
      uVar1 = FUN_006147d0(*unaff_EDI);
      *(undefined2 *)((int)unaff_ESI + 0x12) = uVar1;
      *(undefined2 *)(unaff_ESI + 4) = 4;
      *(undefined2 *)((int)unaff_EDI + 0xe) = 0;
      return;
    }
  }
  if (DAT_006f14c4 != 0) {
    uVar3 = FUN_006147e0(DAT_006f14c4);
    uVar4 = FUN_006147e0(DAT_006f14c4);
    iVar2 = FUN_006147e0(DAT_006f14c4);
    uVar5 = FUN_006147e0(DAT_006f14c4);
    *unaff_ESI = (uVar3 & 0xff0000 | uVar4 >> 0x10) >> 8 | (iVar2 << 0x10 | uVar5 & 0xff00) << 8;
    uVar1 = FUN_006147f0(DAT_006f14c4);
    *(undefined2 *)((int)unaff_ESI + 0x12) = uVar1;
    *(undefined2 *)(unaff_ESI + 4) = 4;
    *(undefined2 *)((int)unaff_EDI + 0xe) = 0;
    return;
  }
  *unaff_ESI = 0;
  *(undefined2 *)((int)unaff_ESI + 0x12) = 0;
  *(undefined2 *)(unaff_ESI + 4) = 4;
  *(undefined2 *)((int)unaff_EDI + 0xe) = 0xfff1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
