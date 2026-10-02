// network_receive_queue_new  (Ghidra: network_receive_queue_new, already named)
// address 0x441bf0, size 136 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: out/phase4/networking_types_notes.md "network_receive_queue (0x1c)" section names
// every field this constructor writes (socket, data_ready, unknown_05, socket_key == -1,
// flags == 0, unknown_0d == 0x14, last_error == 0, incoming, unknown_14 == -1, unknown_18);
// the inner allocation is byte-for-byte types/memory.h's circular_buffer (name, 'circ'
// signature at +0x04, capacity at +0x10, data == base+0x18), named "received_data_queue" here.
// register convention: __cdecl, no arguments.
// UNSURE: none.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void network_channels_open(void);
extern void network_handle_registry_close_all(void); // 0x441bb0

// blam-cc: __cdecl, no arguments
// Ensures the main channels are open and any stale handles are swept, then allocates and
// zero/default-initializes a network_receive_queue and its backing 0x10001-byte circular
// buffer ("received_data_queue"). Returns NULL if either GlobalAlloc fails; if the outer
// allocation succeeds but the inner one does not, `incoming` is left NULL.
network_receive_queue *network_receive_queue_new(void)
{
    network_receive_queue *queue;
    circular_buffer *buffer;

    network_channels_open();
    network_handle_registry_close_all();
    queue = (network_receive_queue *)GlobalAlloc(0, 0x1c);
    if (queue != 0) {
        queue->socket = 0;
        queue->data_ready = 0;
        queue->connection_failed = 0;
        queue->socket_key = 0xffffffff;
        queue->flags = 0;
        queue->unknown_0d = 0x14;
        queue->last_error = 0;
        buffer = (circular_buffer *)GlobalAlloc(0, 0x10019);
        if (buffer != 0) {
            buffer->name = 0;
            buffer->signature = 0;
            buffer->read_cursor = 0;
            buffer->write_cursor = 0;
            buffer->capacity = 0;
            buffer->data = 0;
            buffer->name = (char *)"received_data_queue";
            buffer->signature = 0x63697263; // 'circ'
            buffer->capacity = 0x10001;
            buffer->data = (uint8_t *)buffer + 0x18;
        }
        queue->incoming = buffer;
        queue->unknown_14 = 0xffffffff;
        queue->reject_reason = 0;
    }
    return queue;
}

#if 0
Original Ghidra decompilation (0x441bf0):

void * __cdecl network_receive_queue_new(void)

{
  undefined4 *puVar1;
  undefined4 *puVar2;

  network_channels_open();
  FUN_00441bb0();
  puVar1 = GlobalAlloc(0,0x1c);
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = 0;
    *(undefined1 *)(puVar1 + 1) = 0;
    *(undefined1 *)((int)puVar1 + 5) = 0;
    puVar1[2] = 0xffffffff;
    *(undefined1 *)(puVar1 + 3) = 0;
    *(undefined1 *)((int)puVar1 + 0xd) = 0x14;
    *(undefined2 *)((int)puVar1 + 0xe) = 0;
    puVar2 = GlobalAlloc(0,0x10019);
    if (puVar2 != (undefined4 *)0x0) {
      *puVar2 = 0;
      puVar2[1] = 0;
      puVar2[2] = 0;
      puVar2[3] = 0;
      puVar2[4] = 0;
      puVar2[5] = 0;
      *puVar2 = "received_data_queue";
      puVar2[1] = 0x63697263;
      puVar2[4] = 0x10001;
      puVar2[5] = puVar2 + 6;
    }
    puVar1[4] = puVar2;
    puVar1[5] = 0xffffffff;
    puVar1[6] = 0;
  }
  return puVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
