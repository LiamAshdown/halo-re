// network_channel_receive_callback  (Ghidra: network_channel_receive_callback, already named)
// address 0x441ed0, size 94 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("per-channel data-received handler:
// records the sender's address for connectionless channels, or appends the payload to the
// channel's receive circular buffer for connection-oriented ones"); the tested flag (+0x0c bit
// 0) is network_receive_queue.flags bit0 "connection oriented" per types/networking.h, and the
// registered lookup FUN_00614840(gamespy_handle) -> network_receive_queue* is reused from
// network_connection_stats_record_packet.c, whose evidence note documents the same callee.
// register convention: all three parameters are real (stack/callback) parameters, not
// Ghidra-recognized registers -- this is registered as a callback with a fixed signature by
// the foreign transport library, called with (handle, data, length).
// UNSURE: FUN_006148b0's local 24-byte output buffer is filled and then never read again; kept
// exactly as decompiled rather than assumed dead, since the callee may have an internal side
// effect (e.g. caching the formatted address) that this module does not see. UNSURE: the
// `circular_buffer_write(length, queue->incoming, data)` argument order for the one visible
// call-site parameter (source buffer) is reconstructed from circular_buffer_write's own
// documented register convention (byte count EAX, stream EDX, source as the recognized
// parameter), not shown explicitly at this call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void *FUN_00614840(void *gamespy_connection); // foreign, GameSpy library; -> network_receive_queue*
extern uint32_t FUN_006175f0(int32_t object); // 0x6175f0: returns the uint32 at object+0x00 // foreign, GameSpy library; address byte source
extern uint16_t FUN_006147d0(int32_t object); // 0x6147d0: returns the uint16 at object+0x04 in AX // foreign, GameSpy library; port
extern void FUN_006148b0(uint32_t address, uint16_t port, void *out_address); // 0x6148b0: fills a
    // 0x16-byte address record (stride confirmed by the imul esi,esi,0x16 at 0x6148c8) // foreign
extern uint32_t circular_buffer_write(uint32_t byte_count, circular_buffer *stream, uint8_t *source); // 0x4d01c0, memory module

// blam-cc: handle, data, length are ordinary callback parameters
void network_channel_receive_callback(void *handle, uint8_t *data, int32_t length)
{
    network_receive_queue *queue;
    uint32_t address;
    uint16_t port;
    uint8_t address_buf[24];

    queue = (network_receive_queue *)FUN_00614840(handle);
    if (queue != 0) {
        if ((queue->flags & 1) == 0) {
            address = FUN_006175f0((int32_t)handle);
            port = FUN_006147d0((int32_t)handle);
            FUN_006148b0(address, port, address_buf);
        } else if (0 < length) {
            circular_buffer_write((uint32_t)length, queue->incoming, data);
            return;
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x441ed0):

void network_channel_receive_callback(undefined4 param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined1 local_18 [24];

  iVar1 = FUN_00614840(param_1);
  if (iVar1 != 0) {
    if ((*(byte *)(iVar1 + 0xc) & 1) == 0) {
      uVar2 = FUN_006175f0(param_1);
      uVar3 = FUN_006147d0(param_1);
      FUN_006148b0(uVar2,uVar3,local_18);
    }
    else if (0 < param_3) {
      circular_buffer_write(param_2);
      return;
    }
  }
  return;
}
#endif
