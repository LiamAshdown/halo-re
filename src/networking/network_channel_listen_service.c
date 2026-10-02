// network_channel_listen_service  (Ghidra: FUN_004dd4e0; named per this rewrite)
// address 0x4dd4e0, size 591 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md: "Processes pending incoming-connection and data
// events on a channel, accepting new child connections and forwarding received data to the
// matching existing child channel." listen_list (+0xa9c), its entries/capacity/last_index/
// unknown_110 scan cursor (+0x104/+0x108/+0x10c/+0x110), children[16] (+0xaa0), connected
// (+0xa98), parent (+0xa94) and child_busy (+0xae1) all match types/networking.h exactly.
// UNSURE (significant): `piVar2[2]` (channel->unknown_008, offset +0x008) is called through as a
// function pointer when non-NULL ("accept override callback"), otherwise
// network_listen_reject_pending_connection is used -- types/networking.h currently leaves this
// field undocumented ("never read or written by the module" does NOT appear for it, but no
// specific purpose is recorded either); this rewrite casts it to a callback type locally rather
// than asserting a header change.
// UNSURE: network_server_validate_join_request (outside this batch's range) is called with no visible argument; supplied
// the listening endpoint as a guess.
// UNSURE: the loopback/local-address check that sets a new child `connected` uses
// network_channel_get_remote_address's queue->last_error side channel exactly like
// network_channel_remote_address_or_default.c, reconstructed the same way (see that file for the general
// pattern) rather than calling the still-unresolved network_channel_remote_address_or_default shared guard, since what is
// needed here is the resolved address itself, which the guard never exposes.
// register/parameter convention: channel in ECX/param_1 (Ghidra recognizes it as a real stack
// parameter here), out-new-child in param_2.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint32_t network_local_address; // 0x006869b0
extern int32_t network_pending_connection_count; // 0x006f16d0

extern int32_t network_channel_list_mark_readable(network_channel_list *list); // 0x4419d0, this module
extern int32_t network_channel_list_remove(network_receive_queue *entry, network_channel_list *list); // 0x441b00, this module
extern network_receive_queue *network_listen_accept_pending_connection(void); // 0x4421b0, this module
extern uint32_t network_listen_reject_pending_connection(int32_t reject_code); // 0x442250, this module
extern network_channel *network_channel_new_child(network_receive_queue *endpoint); // 0x4dd430, this batch
extern char network_channel_transmit(network_channel *channel); // 0x4dd730, this batch
extern int32_t network_server_validate_join_request(network_receive_queue *listen_endpoint); // 0x4e0850, outside this batch, elided arg
extern int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue); // 0x441ce0, this module


// Scans the listening channel's readable-socket list. Data on the listen socket itself triggers
// an accept attempt (subject to the 16-child table and the pending-connection queue); data on an
// already-accepted child's endpoint is forwarded via network_channel_transmit, with a failed
// transmit marking that child dead (k_network_channel_dead) instead of aborting the whole scan.
char network_channel_listen_service(network_channel *channel, network_channel **out_new_child)
{
    int32_t mark_result;
    int32_t idx;
    network_receive_queue *entry;
    char result;
    char found_one;
    circular_buffer *incoming;
    int32_t available;
    int32_t i;
    int32_t reject_code;
    network_channel *new_child;
    s_network_address remote_address;

    *out_new_child = 0;
    result = 1;
    mark_result = network_channel_list_mark_readable(channel->listen_list);
    if (mark_result != 0) {
        if (mark_result == -0xd) {
            return 1;
        }
        return 0;
    }
    channel->listen_list->service_cursor = 0;
    found_one = 0;
    for (;;) {
        idx = channel->listen_list->service_cursor;
        if (channel->listen_list->last_index < idx) {
            return result;
        }
        entry = channel->listen_list->entries[idx];
        channel->listen_list->service_cursor = idx + 1;
        if (entry == 0) {
            return result;
        }
        if (found_one != 0) {
            return result;
        }

        if (entry->data_ready == 1 && network_pending_connection_count > 0) {
            goto handle_readable;
        }
        incoming = entry->incoming;
        if (incoming != 0) {
            available = incoming->write_cursor - incoming->read_cursor;
            if (available < 0) {
                available = available + incoming->capacity;
            }
            if (available > 0) {
                goto handle_readable;
            }
        }
        continue;

    handle_readable:
        if (entry == channel->endpoint) {
            if (channel->listen_list->last_index + 1 < 0x11) {
                if (channel->listening == 0) {
                    reject_code = 2;
                } else {
                    reject_code = network_server_validate_join_request(channel->endpoint);
                    if (reject_code == 0) {
                        entry = network_listen_accept_pending_connection();
                        if (entry != 0) {
                            new_child = network_channel_new_child(entry);
                            if (new_child != 0) {
                                for (i = 0; i < 0x10; i++) {
                                    if (channel->children[i] == 0) {
                                        *out_new_child = new_child;
                                        channel->children[i] = new_child;
                                        new_child->parent = channel;
                                        new_child->connected = 0;
                                        if (channel->child_busy == 0) {
                                            network_channel_get_remote_address(&remote_address, new_child->endpoint);
                                            if (new_child->endpoint->last_error == 0 &&
                                                (remote_address.ipv4 == 0x7f000001 ||
                                                 remote_address.ipv4 == network_local_address)) {
                                                new_child->connected = 1;
                                                channel->child_busy = 1;
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        continue;
                    }
                    // reject_code (the network_server_validate_join_request result) falls through to the reject/callback
                    // dispatch below, matching the original's fall-through when iVar5 != 0.
                }
            } else {
                reject_code = 6;
            }
            if (channel->accept_callback == 0) {
                network_listen_reject_pending_connection(reject_code);
            } else {
                ((network_channel_accept_callback)(void *)(uint32_t)channel->accept_callback)(entry);
            }
        } else {
            for (i = 0; i < 0x10; i++) {
                if (channel->children[i] != 0 && channel->children[i]->endpoint == entry) {
                    result = network_channel_transmit(channel->children[i]);
                    if (result == 0) {
                        network_channel_list_remove(channel->children[i]->endpoint, channel->listen_list); // UNSURE: list arg
                        channel->children[i]->flags = channel->children[i]->flags | k_network_channel_dead;
                        result = 1;
                    }
                    break;
                }
            }
        }
        if (result == 0) {
            return 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4dd4e0):

char FUN_004dd4e0(int *param_1,int *param_2)

{
  int iVar1;
  int *piVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int *piVar6;
  int local_18;

  piVar2 = param_1;
  *param_2 = 0;
  param_1._0_1_ = '\x01';
  sVar3 = FUN_004419d0();
  if (sVar3 != 0) {
    if (sVar3 == -0xd) {
      return '\x01';
    }
    return '\0';
  }
  *(undefined4 *)(piVar2[0x2a7] + 0x110) = 0;
  sVar3 = 0;
LAB_004dd521:
  do {
    iVar5 = piVar2[0x2a7];
    iVar4 = *(int *)(iVar5 + 0x110);
    if (*(int *)(iVar5 + 0x10c) < iVar4) {
      return (char)param_1;
    }
    iVar1 = *(int *)(*(int *)(iVar5 + 0x104) + iVar4 * 4);
    *(int *)(iVar5 + 0x110) = iVar4 + 1;
    if (iVar1 == 0) {
      return (char)param_1;
    }
    if (sVar3 != 0) {
      return (char)param_1;
    }
    if ((*(char *)(iVar1 + 4) == '\x01') && (0 < DAT_006f16d0)) {
LAB_004dd582:
      if (iVar1 == *piVar2) {
        if (*(int *)(piVar2[0x2a7] + 0x10c) + 1 < 0x11) {
          if ((char)piVar2[0x2b8] == '\0') {
            iVar5 = 2;
          }
          else {
            iVar5 = FUN_004e0850();
            if (iVar5 == 0) {
              iVar5 = network_listen_accept_pending_connection();
              if ((iVar5 != 0) && (iVar5 = FUN_004dd430(iVar5), iVar5 != 0)) {
                iVar4 = 0;
                piVar6 = piVar2 + 0x2a8;
                do {
                  if (*piVar6 == 0) {
                    *param_2 = iVar5;
                    piVar2[iVar4 + 0x2a8] = iVar5;
                    *(int **)(iVar5 + 0xa94) = piVar2;
                    *(undefined1 *)(piVar2[iVar4 + 0x2a8] + 0xa98) = 0;
                    if ((*(char *)(*(int *)(piVar2[iVar4 + 0x2a8] + 0xa94) + 0xae1) == '\0') &&
                       ((FUN_004dd390(), local_18 == 0x7f000001 || (local_18 == DAT_006869b0)))) {
                      *(undefined1 *)(piVar2[iVar4 + 0x2a8] + 0xa98) = 1;
                      *(undefined1 *)(*(int *)(piVar2[iVar4 + 0x2a8] + 0xa94) + 0xae1) = 1;
                    }
                    break;
                  }
                  iVar4 = iVar4 + 1;
                  piVar6 = piVar6 + 1;
                } while (iVar4 < 0x10);
              }
              goto LAB_004dd5e2;
            }
          }
        }
        else {
          iVar5 = 6;
        }
        if ((code *)piVar2[2] == (code *)0x0) {
          sVar3 = FUN_00442250(iVar5);
        }
        else {
          (*(code *)piVar2[2])(iVar1);
        }
      }
      else {
        iVar5 = 0;
        piVar6 = piVar2 + 0x2a8;
        do {
          if (((int *)*piVar6 != (int *)0x0) && (*(int *)*piVar6 == iVar1)) {
            param_1._0_1_ = network_channel_transmit((int *)piVar2[iVar5 + 0x2a8]);
            if ((char)param_1 == '\0') {
              network_channel_list_remove(*(undefined4 *)piVar2[iVar5 + 0x2a8]);
              *(uint *)(piVar2[iVar5 + 0x2a8] + 0xa8c) =
                   *(uint *)(piVar2[iVar5 + 0x2a8] + 0xa8c) | 0x10;
              param_1._0_1_ = '\x01';
            }
            goto LAB_004dd521;
          }
          iVar5 = iVar5 + 1;
          piVar6 = piVar6 + 1;
        } while (iVar5 < 0x10);
      }
    }
    else {
      iVar5 = *(int *)(iVar1 + 0x10);
      if (iVar5 != 0) {
        iVar4 = *(int *)(iVar5 + 0xc) - *(int *)(iVar5 + 8);
        if (iVar4 < 0) {
          iVar4 = iVar4 + *(int *)(iVar5 + 0x10);
        }
        if (0 < iVar4) goto LAB_004dd582;
      }
    }
LAB_004dd5e2:
    if ((char)param_1 == '\0') {
      return '\0';
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
