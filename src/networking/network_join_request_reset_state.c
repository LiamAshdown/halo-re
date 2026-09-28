// network_join_request_reset_state  (Ghidra: FUN_004e0ab0)
// address 0x4e0ab0, size 63 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x4e0ab0..0x4e0aee: EAX machine, stack response: the CD key check of a joining
//   machine with its remote ip (the local address 0x006869b0 for loopback 127.0.0.1), its challenge (+0x52) and CD
//   key local id (+0x5c). (Name kept.)
// blam-cc: EAX -> machine, stack -> response

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <wchar.h>

extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, blam-cc: EAX channel, ECX out
extern uint32_t network_local_address; // 0x006869b0
extern uint8_t network_session_host_reject_or_cleanup_client(const char *response, const char *challenge, uint32_t ip,
    int32_t local_id); // 0x575ff0, blam-cc: EAX response, ECX challenge, EDX ip, ESI local_id

uint8_t network_join_request_reset_state(network_machine *machine, const char *response)
{
    network_resolved_address address;
    uint32_t ip;

    network_channel_remote_address_or_default(machine != 0 ? machine->channel : 0, &address);
    ip = *(uint32_t *)&address;
    if (ip == 0x7f000001) {
        ip = network_local_address;
    }
    return network_session_host_reject_or_cleanup_client(response, (const char *)machine + 0x52, ip,
        *(int32_t *)((uint8_t *)machine + 0x5c));
}

#if 0
Original Ghidra decompilation (0x4e0ab0):

void FUN_004e0ab0(void)

{
  FUN_004dd390();
  FUN_00575ff0();
  return;
}
#endif
