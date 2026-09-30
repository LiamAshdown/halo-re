// network_channel_transmit  (Ghidra: network_channel_transmit, already named)
// address 0x4dd730, size 504 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Pumps queued outgoing bytes for a channel from
// its internal ring buffer into the underlying transport's write queue in bounded-size chunks."
// channel->incoming (+0x00c, named "transport-incoming" per types/networking.h) is what gets
// drained here -- NOT the +0x544 "out" stream the header's own comment attributes to this
// function; see network_channel_queue_message.c's header for the same discrepancy. endpoint
// (+0x000), endpoint->data_ready (+0x004) and endpoint->incoming (+0x010, the receive queue's
// own circular_buffer*) all match types/networking.h's network_receive_queue.
// UNSURE: this function drains from `channel->incoming` in up-to-0x5000-byte chunks into a
// local scratch, then calls circular_buffer_write(scratch) with only ONE visible argument; the
// destination circular_buffer* (presumably endpoint->incoming, i.e. handing bytes to the
// transport layer to actually send) is elided and reconstructed as such.
// UNSURE: the exact meaning of "iVar1 == 0/-4/-3" fallthrough cases (0 breaks the loop entirely,
// -4 sets k_network_channel_dead, anything else marks empty and stops) is preserved by literal
// value rather than named constants, since no symbolic names for these circular_buffer_write
// return codes are documented anywhere in this module.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <string.h>

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern int32_t network_pending_connection_count; // 0x006f16d0, UNSURE: reused here per globals list

extern int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue); // 0x441ce0, this module
extern int32_t circular_buffer_write(uint8_t *data, uint32_t byte_count, circular_buffer *stream); // 0x4d01c0, memory module; UNSURE: stream arg elided here

char network_channel_transmit(network_channel *channel)
{
    large_integer counter;
    circular_buffer *incoming;
    int32_t available;
    char done;
    uint32_t chunk;
    uint32_t incoming_available;
    uint8_t scratch[0x5000];
    int32_t send_result;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    incoming = channel->incoming;
    available = incoming->write_cursor - incoming->read_cursor;
    done = 1;
    if (available < 0) {
        available = available + incoming->capacity;
    }
    available = incoming->capacity - available;

    network_channel_get_remote_address(0, channel->endpoint); // UNSURE: out-address argument elided

    do {
        chunk = (uint32_t)(available - 1);
        incoming = channel->incoming;
        if (channel->endpoint->data_ready != 1 || network_pending_connection_count < 1) {
            incoming = channel->incoming;
            if (incoming == 0) {
                break;
            }
            incoming_available = incoming->write_cursor - incoming->read_cursor;
            if ((int32_t)incoming_available < 0) {
                incoming_available = incoming_available + incoming->capacity;
            }
            if ((int32_t)incoming_available < 1) {
                break;
            }
        }
        if ((int32_t)chunk < 1) {
            break;
        }
        if (0x4fff < (int32_t)chunk) {
            chunk = 0x5000;
        }
        incoming = channel->incoming;
        incoming_available = incoming->write_cursor - incoming->read_cursor;
        if ((int32_t)incoming_available < 0) {
            incoming_available = incoming_available + incoming->capacity;
        }
        if (channel->endpoint->unknown_05 == 1) { // UNSURE: offset +0x05 has no named field
            if (incoming_available != 0) {
                goto do_transfer;
            }
            channel->flags = channel->flags | k_network_channel_dead;
            done = 0;
            goto after_transfer;
        }
        if (incoming_available == 0) {
            break;
        }
    do_transfer:
        if ((int32_t)chunk < (int32_t)incoming_available) {
            incoming_available = chunk;
        }
        {
            int32_t read_cursor = incoming->read_cursor;
            uint8_t *scratch_cursor = scratch;
            int32_t tail = incoming->write_cursor - read_cursor;
            if (tail < 0) {
                tail = tail + incoming->capacity;
            }
            if ((int32_t)incoming_available <= tail) {
                uint32_t tail_room = (uint32_t)incoming->capacity - (uint32_t)read_cursor;
                uint32_t first_copy = incoming_available;
                if (tail_room <= incoming_available) {
                    memcpy(scratch, incoming->data + read_cursor, tail_room);
                    read_cursor = 0;
                    scratch_cursor = scratch + tail_room;
                    first_copy = incoming_available - tail_room;
                }
                if ((int32_t)first_copy > 0) {
                    memcpy(scratch_cursor, incoming->data + read_cursor, first_copy);
                    read_cursor = read_cursor + first_copy;
                }
                incoming->read_cursor = read_cursor;
            }
        }
        if ((int32_t)incoming_available < 1) {
            if (incoming_available != 0xfffffffc) {
                if (incoming_available == 0xfffffffd) {
                    channel->flags = channel->flags | k_network_channel_dead;
                    done = 0;
                    goto after_transfer;
                }
                done = 0;
                goto after_transfer;
            }
            break;
        }
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        channel->last_activity_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
        send_result = circular_buffer_write(scratch, incoming_available, channel->endpoint->incoming); // UNSURE: stream arg
        (void)send_result;
    after_transfer:
        incoming = channel->incoming;
        available = incoming->write_cursor - incoming->read_cursor;
        if (available < 0) {
            available = available + incoming->capacity;
        }
        available = incoming->capacity - available;
    } while (done != 0);
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    return done;
}

#if 0
Original Ghidra decompilation (0x4dd730):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

char __cdecl network_channel_transmit(int *channel)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  undefined8 uVar9;
  char local_5035;
  int local_5034;
  undefined4 *local_502c;
  LARGE_INTEGER local_5028 [4];
  undefined4 local_5008 [5119];
  undefined4 uStack_c;

  uStack_c = 0x4dd740;
  QueryPerformanceCounter(local_5028);
  iVar4 = channel[3];
  iVar1 = *(int *)(iVar4 + 0xc) - *(int *)(iVar4 + 8);
  local_5035 = '\x01';
  if (iVar1 < 0) {
    iVar1 = iVar1 + *(int *)(iVar4 + 0x10);
  }
  iVar1 = *(int *)(iVar4 + 0x10) - iVar1;
  network_channel_get_remote_address();
  do {
    uVar6 = iVar1 - 1;
    iVar4 = *channel;
    if ((*(char *)(iVar4 + 4) != '\x01') || (DAT_006f16d0 < 1)) {
      iVar1 = *(int *)(iVar4 + 0x10);
      if (iVar1 == 0) break;
      iVar2 = *(int *)(iVar1 + 0xc) - *(int *)(iVar1 + 8);
      if (iVar2 < 0) {
        iVar2 = iVar2 + *(int *)(iVar1 + 0x10);
      }
      if (iVar2 < 1) break;
    }
    if ((int)uVar6 < 1) break;
    if (0x4fff < (int)uVar6) {
      uVar6 = 0x5000;
    }
    iVar1 = *(int *)(iVar4 + 0x10);
    uVar3 = *(int *)(iVar1 + 0xc) - *(int *)(iVar1 + 8);
    if ((int)uVar3 < 0) {
      uVar3 = uVar3 + *(int *)(iVar1 + 0x10);
    }
    if (*(char *)(iVar4 + 5) == '\x01') {
      if (uVar3 != 0) goto LAB_004dd7e3;
LAB_004dd8e0:
      channel[0x2a3] = channel[0x2a3] | 0x10;
LAB_004dd8ea:
      local_5035 = '\0';
    }
    else {
      if (uVar3 == 0) break;
LAB_004dd7e3:
      if ((int)uVar6 < (int)uVar3) {
        uVar3 = uVar6;
      }
      local_5034 = *(int *)(iVar1 + 8);
      local_502c = local_5008;
      iVar4 = *(int *)(iVar1 + 0xc) - local_5034;
      if (iVar4 < 0) {
        iVar4 = iVar4 + *(int *)(iVar1 + 0x10);
      }
      if ((int)uVar3 <= iVar4) {
        uVar5 = *(int *)(iVar1 + 0x10) - local_5034;
        uVar6 = uVar3;
        if ((int)uVar5 <= (int)uVar3) {
          puVar7 = (undefined4 *)(*(int *)(iVar1 + 0x14) + local_5034);
          puVar8 = local_5008;
          for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
            *puVar8 = *puVar7;
            puVar7 = puVar7 + 1;
            puVar8 = puVar8 + 1;
          }
          for (uVar6 = uVar5 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
            *(undefined1 *)puVar8 = *(undefined1 *)puVar7;
            puVar7 = (undefined4 *)((int)puVar7 + 1);
            puVar8 = (undefined4 *)((int)puVar8 + 1);
          }
          local_5034 = 0;
          local_502c = (undefined4 *)((int)local_5008 + uVar5);
          uVar6 = uVar3 - uVar5;
        }
        if (0 < (int)uVar6) {
          puVar7 = (undefined4 *)(*(int *)(iVar1 + 0x14) + local_5034);
          for (uVar5 = uVar6 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
            *local_502c = *puVar7;
            puVar7 = puVar7 + 1;
            local_502c = local_502c + 1;
          }
          local_5034 = local_5034 + uVar6;
          for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
            *(undefined1 *)local_502c = *(undefined1 *)puVar7;
            puVar7 = (undefined4 *)((int)puVar7 + 1);
            local_502c = (undefined4 *)((int)local_502c + 1);
          }
        }
        *(int *)(iVar1 + 8) = local_5034;
      }
      if ((int)uVar3 < 1) {
        if (uVar3 != 0xfffffffc) {
          if (uVar3 == 0xfffffffd) goto LAB_004dd8e0;
          goto LAB_004dd8ea;
        }
        break;
      }
      QueryPerformanceCounter(local_5028);
      uVar9 = __allmul(local_5028[0].s.LowPart,local_5028[0].s.HighPart,1000,0);
      iVar4 = __alldiv(uVar9,DAT_006ac8f8,DAT_006ac8fc);
      channel[1] = iVar4;
      circular_buffer_write(local_5008);
    }
    iVar4 = channel[3];
    iVar1 = *(int *)(iVar4 + 0xc) - *(int *)(iVar4 + 8);
    if (iVar1 < 0) {
      iVar1 = iVar1 + *(int *)(iVar4 + 0x10);
    }
    iVar1 = *(int *)(iVar4 + 0x10) - iVar1;
  } while (local_5035 != '\0');
  QueryPerformanceCounter(local_5028);
  return local_5035;
}
#endif
