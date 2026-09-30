// network_host_update_tick  (Ghidra: FUN_004def80; named per this rewrite)
// address 0x4def80, size 239 bytes
// name confidence: 0.3   rewrite confidence: 0.15
// evidence: out/phase4/networking_functions.md: "Per-tick client-side network update handler:
// pulls the next queued packet, refreshes the map/variant cycle list periodically, and
// dispatches processing to one of three state-specific handlers based on [state]." host->flags
// bit1 (k_network_server_host, matches network_game_server_host_new.c) and host->unknown_004
// (the state dispatched on 0/1/2, matching network_game_server_host_dispose.c's own 0/2 test)
// match types/networking.h.
// FIXED in the review pass (this was the file's largest UNSURE): `network_channel_service(local_20)` is
// network_channel_service's real third, stack-passed argument, which Ghidra dropped from that
// function's own signature. 0x4def97..0x4defa8 is
//     mov edi,[esi]                 ; channel   = host->listen_channel
//     lea edx,[esp+0xc] / push edx  ; out_new_child = &new_child
//     xor eax,eax                   ; timeout_ms = 0
//     mov [esp+0x10],0              ; new_child = NULL before the call
// and 0x4defbb reads the local straight back. So the listening service hands the newly
// accepted child channel back through that pointer, and the rest of this function is the
// accept path for it. See src/networking/network_channel_service.c.
// FIXED: 0x4defc3 pushes the new child as network_server_count_machines_and_resolve_address's second argument, and 0x4defe1 pushes
// it as network_channel_remove_child's `child` (with EDI reloaded to host->listen_channel);
// the first draft passed 0 for both.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t time_query_performance_counter_ms(void); // outside this batch, tick/ms counter
extern char network_channel_service(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child); // 0x4dd110
extern int32_t network_channel_remove_child(network_channel *parent, network_channel *child); // 0x4dd090, this batch
extern char network_server_count_machines_and_resolve_address(network_server_globals *host, network_channel *new_child); // 0x4e0d30, outside this batch
extern void network_map_cycle_list_broadcast(void); // 0x4deec0, this batch
extern char network_server_service_machines_tick(network_server_globals *host); // 0x4e11d0, outside this batch, elided arg
extern char network_server_heartbeat_tick(network_server_globals *host); // 0x4e15a0, outside this batch, elided arg
extern char network_server_status_periodic_print(network_server_globals *host); // 0x4e1520, outside this batch, elided arg
extern char network_server_resend_challenge_periodic(network_server_globals *host); // 0x4e1450, outside this batch, elided arg

extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, this module

char network_host_update_tick(network_server_globals *host)
{
    char service_result;
    char proceed;
    network_channel *new_child;
    network_resolved_address sender;
    uint32_t now_ms;

    service_result = 1;
    if ((host->flags >> 1) & 1) {
        new_child = 0;
        service_result = network_channel_service(host->listen_channel, 0, &new_child);
        if (service_result == 1) {
            proceed = 1;
            if (new_child != 0) {
                service_result = network_server_count_machines_and_resolve_address(host, new_child);
                if (service_result == 1) {
                    network_channel_remote_address_or_default(new_child, &sender);
                    proceed = 1;
                } else {
                    proceed = (char)network_channel_remove_child(host->listen_channel, new_child);
                }
            }
            service_result = 0;
            if (proceed != 0) {
                now_ms = time_query_performance_counter_ms();
                if ((uint32_t)((int32_t)host->last_stamp_ms + 3000) < now_ms) {
                    network_map_cycle_list_broadcast();
                    host->last_stamp_ms = now_ms;
                }
                service_result = network_server_service_machines_tick(host);
                if (service_result == 0) {
                    return 0;
                }
                if (host->state == 0) {
                    return network_server_heartbeat_tick(host);
                }
                if (host->state != 1) {
                    if (host->state != 2) {
                        return 0;
                    }
                    return network_server_resend_challenge_periodic(host);
                }
                return network_server_status_periodic_print(host);
            }
        }
    }
    return service_result;
}

#if 0
Original Ghidra decompilation (0x4def80):

char FUN_004def80(void)

{
  short sVar1;
  int iVar2;
  char cVar3;
  char cVar4;
  int in_EAX;
  uint uVar5;
  int local_20 [8];

  cVar3 = '\x01';
  if ((*(byte *)(in_EAX + 6) >> 1 & 1) != 0) {
    local_20[0] = 0;
    cVar3 = FUN_004dd110(local_20);
    iVar2 = local_20[0];
    if (cVar3 == '\x01') {
      cVar4 = '\x01';
      if (local_20[0] != 0) {
        cVar3 = FUN_004e0d30();
        if (cVar3 == '\x01') {
          FUN_004dd390();
          cVar4 = '\x01';
        }
        else {
          cVar4 = FUN_004dd090(iVar2);
        }
      }
      cVar3 = '\0';
      if (cVar4 != '\0') {
        uVar5 = FUN_00449210();
        if (*(int *)(in_EAX + 0x9c0) + 3000U < uVar5) {
          FUN_004deec0();
          *(uint *)(in_EAX + 0x9c0) = uVar5;
        }
        cVar3 = FUN_004e11d0();
        if (cVar3 == '\0') {
          return '\0';
        }
        sVar1 = *(short *)(in_EAX + 4);
        if (sVar1 == 0) {
          cVar3 = FUN_004e15a0();
          return cVar3;
        }
        if (sVar1 != 1) {
          if (sVar1 != 2) {
            return '\0';
          }
          cVar3 = FUN_004e1450();
          return cVar3;
        }
        cVar3 = FUN_004e1520();
        return cVar3;
      }
    }
  }
  return cVar3;
}
#endif
