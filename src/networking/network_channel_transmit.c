// network_channel_transmit  (Ghidra: network_channel_transmit, already named)
// address 0x4dd730, size 504 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md ("pumps queued bytes ... in bounded-size chunks").
// Rewritten against the disassembly 0x4dd730..0x4dd927: the pump drains the transport's receive
// queue (channel->endpoint->incoming, queue +0x10) in chunks of up to 0x5000 bytes into a local
// scratch buffer and appends each chunk to channel->incoming (channel +0x0c) with
// circular_buffer_write (EAX = count, EDX = channel->incoming, stack = scratch). The chunk size
// is limited by the free space in channel->incoming. (The earlier draft had source and
// destination swapped and passed NULL for the remote-address out parameter.)
// register convention: cdecl, channel on the stack; network_channel_get_remote_address takes
// ESI = &local address, EDI = endpoint.
// blam-cc: (channel on the stack)

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern int32_t network_pending_connection_count; // 0x006f16d0

extern int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue); // 0x441ce0, this module
extern int32_t circular_buffer_write(uint8_t *data, uint32_t byte_count, circular_buffer *stream); // 0x4d01c0, memory module

static int32_t transmit_circular_buffer_used(circular_buffer *buffer)
{
    int32_t used = buffer->write_cursor - buffer->read_cursor;
    if (used < 0) {
        used = used + buffer->capacity;
    }
    return used;
}

// VERIFIED against disassembly 0x4dd730..0x4dd927 (2026-09-30)
char network_channel_transmit(network_channel *channel)
{
    large_integer counter;
    s_network_address remote_address;
    circular_buffer *destination;
    circular_buffer *source;
    network_receive_queue *queue;
    int32_t free_space;
    int32_t chunk;
    int32_t source_available;
    int32_t count;
    int32_t read_cursor;
    int32_t remaining;
    uint8_t *scratch_cursor;
    char done;
    uint8_t scratch[0x5000];

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    destination = channel->incoming;
    done = 1;
    free_space = destination->capacity - transmit_circular_buffer_used(destination) - 1;

    network_channel_get_remote_address(&remote_address, channel->endpoint);

    for (;;) {
        queue = channel->endpoint;
        if (queue->data_ready != 1 || network_pending_connection_count < 1) {
            source = queue->incoming;
            if (source == 0) {
                break;
            }
            if (transmit_circular_buffer_used(source) < 1) {
                break;
            }
        }
        if (free_space < 1) {
            break;
        }
        chunk = (free_space < 0x5000) ? free_space : 0x5000;

        source = queue->incoming;
        source_available = transmit_circular_buffer_used(source);
        if (queue->connection_failed == 1) {
            if (source_available == 0) {
                channel->flags = channel->flags | k_network_channel_dead;
                done = 0;
                goto refresh;
            }
        } else if (source_available == 0) {
            break;
        }

        count = (source_available > chunk) ? chunk : source_available;

        // inline circular_buffer_read of `count` bytes from the transport queue into scratch
        read_cursor = source->read_cursor;
        scratch_cursor = scratch;
        remaining = count;
        if (count <= transmit_circular_buffer_used(source)) {
            int32_t tail_room = source->capacity - read_cursor;
            if (count >= tail_room) {
                memcpy(scratch, source->data + read_cursor, (uint32_t)tail_room);
                read_cursor = 0;
                scratch_cursor = scratch + tail_room;
                remaining = count - tail_room;
            }
            if (remaining > 0) {
                memcpy(scratch_cursor, source->data + read_cursor, (uint32_t)remaining);
                read_cursor = read_cursor + remaining;
            }
            source->read_cursor = read_cursor;
        }

        if (count > 0) {
            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            channel->last_activity_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
            circular_buffer_write(scratch, (uint32_t)count, channel->incoming);
        } else {
            if (count == -4) {
                break;
            }
            if (count == -3) {
                channel->flags = channel->flags | k_network_channel_dead;
            }
            done = 0;
        }
    refresh:
        destination = channel->incoming;
        free_space = destination->capacity - transmit_circular_buffer_used(destination) - 1;
        if (done == 0) {
            break;
        }
    }
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
