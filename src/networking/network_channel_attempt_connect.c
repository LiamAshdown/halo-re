// network_channel_attempt_connect  (Ghidra: FUN_00441f60, still unnamed -> renamed)
// address 0x441f60, size 214 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("attempts to (re)establish a network
// channel connection, flagging a disconnect/error state and notifying elsewhere if the attempt
// fails"); the queue fields written (+0x05 unknown_05, +0x0e last_error) match
// network_receive_queue; the address fields read (+0x00 ipv4, +0x12 port) match
// s_network_address; 0x0071c2de matches types/networking.h's documented host-handoff request
// flag.
// register convention: s_network_address * in EAX (in_EAX), network_receive_queue * in ESI
// (unaff_ESI) -- neither is this function's own declared parameter, both are threaded through
// from callers exactly like network_channel_get_remote_address.c and
// network_channel_receive_callback.c in this same file group; unused_param_1 and
// use_query_socket are genuine stack parameters (confirmed by the only two call sites, both in
// out/phase2/networking/01.md, passing literal (0x96640, 1)).
// UNSURE: unused_param_1 (0x96640 at both call sites) is never read anywhere in this function's
// body; kept as an unused parameter exactly as decompiled rather than dropped.
// FIXED in the review pass, all three from the disassembly at 0x441ff5..0x442035:
//   - the function returns in AX, not EAX (mov ax,bx / mov ax,di with edi = 0xfffffff0), so
//     the failure result is the int16_t -16, not the int32_t 0xfff0 an earlier draft returned.
//     types/networking.h now carries it as k_network_error_connect_failed.
//   - 0x00718fa4 is accessed with WORD PTR (cmp .,0xffff / mov .,0x7), so it is an int16_t.
//   - FUN_00614830 takes two arguments here as well: 0x441fe6 is
//     mov eax,[esi]; push esi; push eax, i.e. (queue->socket, queue), matching
//     network_listen_accept_pending_connection.c's call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_query_socket; // 0x006f14c8
extern int32_t network_game_socket;  // 0x006f14c4
extern int16_t network_join_error_code; // 0x00718fa4, WORD-sized (see note above)
extern uint8_t network_host_handoff_requested;      // 0x0071c2de

extern void network_channels_open(void); // 0x441300, this module
extern void chat_close(void); // foreign
extern uint32_t FUN_00614890(uint32_t ipv4); // foreign, GameSpy library
extern void FUN_006148b0(uint32_t address, uint16_t port, void *out_address); // 0x6148b0: fills a
    // 0x16-byte address record (stride confirmed by the imul esi,esi,0x16 at 0x6148c8) // foreign
extern int32_t FUN_006145a0(int32_t socket); // foreign, GameSpy library
extern void FUN_00614830(int32_t socket, network_receive_queue *queue); // foreign, GameSpy library // foreign, GameSpy library

// blam-cc: address in EAX (in_EAX), receive-queue pointer in ESI (unaff_ESI);
// unused_param_1/use_query_socket are ordinary stack parameters
// Picks the query or game socket (use_query_socket != 0 selects query), and if that socket
// exists and reports a successful connect (FUN_006145a0 returns 0), clears the queue's
// disconnect flag and error. Otherwise marks the queue disconnected, arms the host-handoff
// retry state, closes chat, and reports error 0xfff0.
int16_t network_channel_attempt_connect(s_network_address *address, network_receive_queue *queue,
                                         int32_t unused_param_1, uint8_t use_query_socket)
{
    uint32_t formatted_address;
    int32_t socket;
    int32_t connect_result;
    uint8_t address_buf[24];

    formatted_address = FUN_00614890(address->ipv4);
    FUN_006148b0(formatted_address, address->port, address_buf);
    network_channels_open();
    socket = network_query_socket;
    if (use_query_socket == 0) {
        socket = network_game_socket;
    }
    if (socket != 0) {
        connect_result = FUN_006145a0(socket);
        if (connect_result == 0) {
            queue->unknown_05 = 0;
            FUN_00614830(queue->socket, queue);
            queue->last_error = 0;
            return 0;
        }
    }
    queue->unknown_05 = 1;
    if (network_join_error_code == -1) {
        network_join_error_code = 7;
    }
    network_host_handoff_requested = 1;
    chat_close();
    queue->last_error = k_network_error_connect_failed;
    return k_network_error_connect_failed;
}

#if 0
Original Ghidra decompilation (0x441f60):

undefined4 FUN_00441f60(undefined4 param_1,char param_2)

{
  undefined4 *in_EAX;
  undefined4 uVar1;
  int iVar2;
  undefined4 *unaff_ESI;
  undefined1 local_18 [24];

  uVar1 = FUN_00614890(*in_EAX);
  FUN_006148b0(uVar1,*(undefined2 *)((int)in_EAX + 0x12),local_18);
  network_channels_open();
  iVar2 = DAT_006f14c8;
  if (param_2 == '\0') {
    iVar2 = DAT_006f14c4;
  }
  if (iVar2 != 0) {
    iVar2 = FUN_006145a0(iVar2);
    if (iVar2 == 0) {
      *(undefined1 *)((int)unaff_ESI + 5) = 0;
      FUN_00614830(*unaff_ESI);
      *(undefined2 *)((int)unaff_ESI + 0xe) = 0;
      return 0;
    }
  }
  *(undefined1 *)((int)unaff_ESI + 5) = 1;
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 7;
  }
  DAT_0071c2de = 1;
  chat_close();
  *(undefined2 *)((int)unaff_ESI + 0xe) = 0xfff0;
  return 0xfff0;
}
#endif
