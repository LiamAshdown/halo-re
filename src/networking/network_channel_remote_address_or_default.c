// network_channel_remote_address_or_default  (Ghidra: FUN_004dd390; renamed by the review pass
// from the rewriters' network_message_decode_guard)
// address 0x4dd390, size 84 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: the disassembly settles what the decompilation could not. 0x4dd390 is
//     push esi / mov esi,ecx / test esi,esi / je ret        -> ECX is the out record, may be NULL
//     mov edi,[eax] / test edi,edi / je default             -> EAX is a network_channel *, and
//                                                              [eax] is channel->endpoint
//     call 0x441ce0                                         -> network_channel_get_remote_address
//                                                              with ESI = out record, EDI = queue
//     test ax,ax / je ret-unchanged                         -> 0x441ce0 returns its error code in
//                                                              AX (0, or 0xfff1 on the no-address
//                                                              path at 0x441de1), NOT void
//     zero six dwords at [esi] / mov WORD [esi+0x10],4      -> the default is 0.0.0.0:0 with
//                                                              size = k_network_address_size_ipv4
// So this function resolves the remote address of `channel` into `out_address`, substituting a
// zeroed IPv4 address when the channel has no endpoint or the transport cannot report one.
// The rewriters modeled the record as an opaque "decode result" with a `state` field; the
// WORD 4 at +0x10 is s_network_address::size and the dword at +0x00 that every decode handler
// compares is s_network_address::ipv4 -- the handlers are checking the sender against the peer
// they expect. Renamed and retyped accordingly; see types/networking.h's network_resolved_address.
// Call sites confirming the EAX argument is the channel itself, not a slot holding it:
//   0x4dbcc0  mov eax,[esi+0xadc]        (client->channel)
//   0x4dc090  mov eax,[esi+0xadc]        (client->channel)
//   0x4d9f1d  mov eax,[esi+0xadc]        (client->channel), ecx = client+0xef8
//   0x4df690  mov eax,[esi]              (machine->channel)
//   0x4defd4  mov eax,edi                (the freshly accepted child channel)
// register convention: channel in EAX, out_address in ECX. blam-cc: EAX -> channel, ECX -> out_address

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue); // 0x441ce0, this module

// blam-cc: EAX -> channel, ECX -> out_address
void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address)
{
    network_receive_queue *queue;
    int use_default;

    if (out_address == 0) {
        return;
    }
    use_default = 0;
    queue = channel->endpoint;
    if (queue == 0) {
        use_default = 1;
    } else if (network_channel_get_remote_address(&out_address->address, queue) != 0) {
        use_default = 1;
    }
    if (use_default) {
        out_address->address.ipv4 = 0;
        out_address->address.ipv6_1 = 0;
        out_address->address.ipv6_2 = 0;
        out_address->address.ipv6_3 = 0;
        *(uint32_t *)&out_address->address.size = 0; // one dword covering size and port
        out_address->unknown_14 = 0;
        out_address->address.size = k_network_address_size_ipv4;
    }
}

#if 0
Original Ghidra decompilation (0x4dd390):

void FUN_004dd390(void)

{
  undefined2 uVar1;
  int in_EAX;
  undefined4 *in_ECX;
  int unaff_EDI;

  if (in_ECX != (undefined4 *)0x0) {
    if (*(int *)in_EAX == 0) {
      *in_ECX = 0;
      in_ECX[1] = 0;
      in_ECX[2] = 0;
      in_ECX[3] = 0;
      in_ECX[4] = 0;
      in_ECX[5] = 0;
      *(undefined2 *)((int)in_ECX + 0x10) = 4;
    }
    else {
      uVar1 = network_channel_get_remote_address();
      if (uVar1 != 0) {
        *in_ECX = 0;
        in_ECX[1] = 0;
        in_ECX[2] = 0;
        in_ECX[3] = 0;
        in_ECX[4] = 0;
        in_ECX[5] = 0;
        *(undefined2 *)((int)in_ECX + 0x10) = 4;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
