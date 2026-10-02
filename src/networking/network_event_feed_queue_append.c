// network_event_feed_queue_append  (Ghidra: network_event_feed_queue_append, already named)
// address 0x4e7ff0, size 68 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md ("Queues one event record into a 16-slot buffer
// and triggers a flush of the buffer once it fills."); network_event_feed_flush (this module, 0x4e8040) is
// the flush this calls once the 16th slot is filled.
// register convention: EAX -> queue (the event feed state block), EDX -> key (a 2-dword header:
// a network index/type pair), stack -> payload (a 12-dword record copied into the slot).
//   // blam-cc: EAX -> queue, EDX -> key, stack -> payload
// UNSURE: no header declares this queue's type; it is not one this batch's types notes attribute
// to networking.h (the summary calls it a generic 16-slot buffer), so it is left as a raw byte
// pointer with offsets exactly as Ghidra shows rather than inventing a struct.

#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void network_event_feed_flush(void); // this module, 0x4e8040; UNSURE: arguments (called with none
    // visible in the decompile, presumably an implicit EAX -> queue passthrough)

// Appends one event record (a 2-dword key plus a 12-dword payload) to the 16-slot queue at
// queue+0x04 (count) / queue+0x08 (keys, stride 8) / queue+0x88 (payloads, stride 0x30), and
// flushes the queue once the 16th slot is filled.
void network_event_feed_queue_append(uint8_t *queue, uint32_t *key, uint32_t *payload)
    // blam-cc: EAX -> queue, EDX -> key, stack -> payload
{
    int32_t count;
    uint32_t *slot_key;
    uint32_t *slot_payload;
    int32_t i;

    count = *(int32_t *)(queue + 4);
    slot_key = (uint32_t *)(queue + 8 + count * 8);
    slot_key[0] = key[0];
    slot_key[1] = key[1];
    slot_payload = (uint32_t *)(queue + 0x88 + count * 0x30);
    for (i = 0xc; i != 0; i = i - 1) {
        *slot_payload = *payload;
        payload = payload + 1;
        slot_payload = slot_payload + 1;
    }
    count = *(int32_t *)(queue + 4) + 1;
    *(int32_t *)(queue + 4) = count;
    if (count == 0x10) {
        network_event_feed_flush();
    }
}

#if 0
Original Ghidra decompilation (0x4e7ff0), from tools/pack.py 0x4e7ff0:

void network_event_feed_queue_append(undefined4 *param_1)

{
  int in_EAX;
  int iVar1;
  undefined4 *in_EDX;
  int iVar2;
  undefined4 *puVar3;

  iVar2 = *(int *)(in_EAX + 4);
  *(undefined4 *)(in_EAX + 8 + iVar2 * 8) = *in_EDX;
  *(undefined4 *)(in_EAX + 0xc + iVar2 * 8) = in_EDX[1];
  puVar3 = (undefined4 *)(iVar2 * 0x30 + 0x88 + in_EAX);
  for (iVar1 = 0xc; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *param_1;
    param_1 = param_1 + 1;
    puVar3 = puVar3 + 1;
  }
  iVar2 = *(int *)(in_EAX + 4) + 1;
  *(int *)(in_EAX + 4) = iVar2;
  if (iVar2 == 0x10) {
    FUN_004e8040();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
