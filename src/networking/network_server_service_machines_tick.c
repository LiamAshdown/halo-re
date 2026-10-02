// network_server_service_machines_tick  (Ghidra: FUN_004e11d0, unnamed)
// address 0x4e11d0, size 191 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Per-frame pass over the session's 16 machine
// slots that advances pending connections and checks established ones for timeout." The
// channel-flags tests (`~(flags>>4)&1` = not k_network_channel_dead, `flags&6` =
// k_network_channel_client|k_network_channel_transmit_pending) and the endpoint's flags bit0
// (connection-oriented) all match types/networking.h exactly. FUN_004df090 sets exactly the
// timer_14/timer_18 fields that network_server_check_machine_timeout.c reads, confirming it
// operates on a network_machine (or an object sharing that layout).
// register convention: the single parameter is a genuine stack (cdecl) parameter.
// blam-cc: stack -> server
// UNSURE: FUN_004df090's real name/signature; called here with a literal 0 duration.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char network_channel_service(int32_t mode); // 0x4dd110, other module (UNSURE args beyond this site)
extern void network_machine_timer_start(network_machine *machine, int32_t duration_ms); // 0x4df090, other module
extern char network_server_check_machine_timeout(network_server_globals *server, network_machine *machine); // 0x4e0ef0, this batch
extern char network_channel_drain_bitstream(network_server_globals *server, network_machine *machine); // 0x4e1290, this batch

// For each of the 16 machine slots with a connected id: if the machine is still "pending"
// (unknown_10 == 0), either restarts its timeout timer (when its channel is dead, unservicable,
// missing the client/transmit-pending flags, or lacks a connection-oriented endpoint) or drains
// its incoming bitstream, restarting the timer again if that fails. Once the machine is no
// longer pending, checks it for timeout. Returns false as soon as any machine's timeout check
// fails.
char network_server_service_machines_tick(network_server_globals *server)
{
    int32_t i;
    char ok;
    char skip_timeout_check;

    ok = 1;
    for (i = 0; i < 16; i = i + 1) {
        network_machine *machine;

        machine = &server->machines[i];
        skip_timeout_check = 0;

        if (machine->machine_id != -1) {
            if (machine->disconnect_timer_active == 0) {
                network_channel *channel;
                char service_ok;
                char proceed;

                channel = machine->channel;
                proceed = 1;
                if ((channel->flags & 0x10) != 0) { // k_network_channel_dead
                    proceed = 0;
                } else {
                    service_ok = network_channel_service(0);
                    if (service_ok == 0) {
                        proceed = 0;
                    } else if ((channel->flags & 0x06) == 0) {
                        proceed = 0;
                    } else if (channel->endpoint == 0 || (channel->endpoint->flags & 0x01) == 0) {
                        proceed = 0;
                    }
                }

                if (!proceed) {
                    network_machine_timer_start(machine, 0);
                } else {
                    char drained;

                    drained = network_channel_drain_bitstream(server, machine);
                    if (drained == 0) {
                        network_machine_timer_start(machine, 0);
                    }
                    ok = 1; // matches the original: cVar2 is unconditionally left at 1 after
                            // this branch, whether or not the drain itself succeeded
                }

                if (machine->disconnect_timer_active == 0) {
                    skip_timeout_check = 1;
                }
            }

            if (!skip_timeout_check && network_server_check_machine_timeout(server, machine) == 0) {
                ok = 0;
            }
        }

        if (ok == 0) {
            return 0;
        }
    }
    return ok;
}

#if 0
Original Ghidra decompilation (0x4e11d0):

char FUN_004e11d0(int param_1)

{
  char cVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  int *piVar5;

  iVar4 = 0;
  cVar2 = '\x01';
  piVar5 = (int *)(param_1 + 0x3b8);
  do {
    if (0xf < iVar4) {
      return cVar2;
    }
    if ((short)piVar5[3] != -1) {
      if ((char)piVar5[4] == '\0') {
        if (((((~(byte)(*(uint *)(*piVar5 + 0xa8c) >> 4) & 1) == 0) ||
             (cVar1 = FUN_004dd110(0), cVar1 == '\0')) ||
            ((*(byte *)((int *)*piVar5 + 0x2a3) & 6) == 0)) ||
           ((iVar3 = *(int *)*piVar5, iVar3 == 0 || ((*(byte *)(iVar3 + 0xc) & 1) == 0)))) {
          FUN_004df090(0);
        }
        else {
          cVar2 = FUN_004e1290(param_1,piVar5);
          if (cVar2 == '\0') {
            FUN_004df090(0);
            cVar2 = '\x01';
          }
        }
        if ((char)piVar5[4] == '\0') goto LAB_004e127c;
      }
      iVar3 = FUN_004e0ef0(param_1,piVar5);
      if (iVar3 == 0) {
        cVar2 = '\0';
      }
    }
LAB_004e127c:
    iVar4 = iVar4 + 1;
    piVar5 = piVar5 + 0x18;
    if (cVar2 == '\0') {
      return '\0';
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
