// network_channel_reliable_pool_store  (Ghidra: FUN_004dcdb0; named per this rewrite)
// address 0x4dcdb0, size 142 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Stores a header/body message pair into a
// tagged slot of the channel's reliable-message buffer pool for later (re)transmission
// tracking." Calls network_channel_reliable_pool_ensure_capacity(channel, body_bytes,
// header_bytes) with the byte counts rounded up from bit counts (ceil(bits/8)), matching
// types/networking.h's note that this function's argument order is the reverse of that one's.
// register/parameter convention: EAX -> header_bits, ECX -> body_bits (both elided from
// Ghidra's own signature); stack -> channel, body_data, header_data, priority.
// blam-cc: EAX -> header_bits, ECX -> body_bits, stack -> channel, body_data, header_data, priority

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t network_channel_reliable_pool_ensure_capacity(network_channel *channel,
    int32_t body_capacity_needed, int32_t header_capacity_needed); // 0x4dcc30, this batch

// blam-cc: EAX -> header_bits, ECX -> body_bits
void network_channel_reliable_pool_store(network_channel *channel, uint8_t *body_data,
    uint8_t *header_data, int32_t priority, uint32_t header_bits, uint32_t body_bits)
{
    uint32_t header_bytes;
    uint32_t body_bytes;
    int32_t index;
    network_channel_reliable_slot *slot;
    uint32_t i;

    header_bytes = (header_bits >> 3) + ((header_bits & 7) != 0);
    body_bytes = (body_bits >> 3) + ((body_bits & 7) != 0);
    index = network_channel_reliable_pool_ensure_capacity(channel, body_bytes, header_bytes);
    slot = &channel->reliable[index];
    slot->priority = priority;
    slot->header_bits = header_bits;
    slot->body_bits = body_bits;
    slot->pending = 1;
    for (i = 0; i < header_bytes; i++) {
        slot->header[i] = header_data[i];
    }
    for (i = 0; i < body_bytes; i++) {
        slot->body[i] = body_data[i];
    }
}

#if 0
Original Ghidra decompilation (0x4dcdb0):

void FUN_004dcdb0(int param_1,undefined4 *param_2,undefined4 *param_3,undefined4 param_4)

{
  uint in_EAX;
  uint uVar1;
  int iVar2;
  undefined1 *puVar3;
  uint in_ECX;
  uint uVar4;
  uint uVar5;
  undefined4 *puVar6;

  uVar1 = (uint)((in_EAX & 7) != 0) + (in_EAX >> 3);
  uVar5 = (uint)((in_ECX & 7) != 0) + (in_ECX >> 3);
  iVar2 = FUN_004dcc30(param_1,uVar5,uVar1);
  puVar3 = (undefined1 *)(iVar2 * 0x20 + *(int *)(param_1 + 0xa7c));
  *(undefined4 *)(puVar3 + 4) = param_4;
  *(uint *)(puVar3 + 0x10) = in_EAX;
  *(uint *)(puVar3 + 0x14) = in_ECX;
  *puVar3 = 1;
  puVar6 = *(undefined4 **)(puVar3 + 0x18);
  for (uVar4 = uVar1 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *puVar6 = *param_3;
    param_3 = param_3 + 1;
    puVar6 = puVar6 + 1;
  }
  for (uVar1 = uVar1 & 3; uVar1 != 0; uVar1 = uVar1 - 1) {
    *(undefined1 *)puVar6 = *(undefined1 *)param_3;
    param_3 = (undefined4 *)((int)param_3 + 1);
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  puVar6 = *(undefined4 **)(puVar3 + 0x1c);
  for (uVar1 = uVar5 >> 2; uVar1 != 0; uVar1 = uVar1 - 1) {
    *puVar6 = *param_2;
    param_2 = param_2 + 1;
    puVar6 = puVar6 + 1;
  }
  for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined1 *)puVar6 = *(undefined1 *)param_2;
    param_2 = (undefined4 *)((int)param_2 + 1);
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
