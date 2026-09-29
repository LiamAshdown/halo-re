// actor_get_firing_positions  (Ghidra: actor_get_firing_positions, already named)
// address 0x41c1e0, size 213 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: out/phase2/results/ai_02.json -- when actor flag +6 (actor.swarm) is clear,
//   bulk-copies the actor's local 14-dword firing-position block from +0x120; otherwise scans
//   an encounter firing-point cluster table (swarm_data / swarm_component_data, per types/ai.h)
//   for the nearest entry to the query point, tracking a minimum squared distance that is
//   computed but never returned through the (void) signature Ghidra recovered.
// register convention: EAX -> actor_index, ECX -> out_block (14 dwords, 0x38 bytes), EDX ->
//   query_point.
//   // blam-cc: EAX -> actor_index, ECX -> out_block, EDX -> query_point
//
// UNSURE: the 14-dword block copied out of actor+0x120 spans types/ai.h's aim_origin (0x120),
// body_position (0x12c) and unknown_138 (0x138..0x158) as one contiguous run; this function is
// evidence that the whole run is really a cached local firing-position list, but the header
// keeps the stronger, separately-established names for those sub-ranges, so the copy below is
// written as a raw dword loop rather than through those fields.
// UNSURE: the tracked minimum squared distance (local_4 in the original) is computed in the
// cluster-scan path and then never read again before the trailing call and the function's
// return -- Ghidra's own signature is void, so this is preserved as-is rather than assumed to
// be an FPU-register return.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data;          // 0x00880360
extern data_array *swarm_data;          // 0x0088035c
extern data_array *swarm_component_data;// 0x00880358

// Fills a caller-provided struct with the actor's unit position/orientation plus a pair of
// fields from the root object at the top of its parent chain. Called here with no visible
// arguments; EAX/ECX/EDX are presumably still live from this function's own entry.


// blam-cc: EAX -> actor_index, ECX -> out_block, EDX -> query_point
// Fetches the actor's set of candidate firing positions: for a non-swarm actor this is its
// cached local list (copied verbatim into out_block), and for a swarm actor it instead scans
// the nearest cluster of swarm-component positions relative to query_point.
void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point)
{
    actor *self;
    uint32_t *src;
    int32_t i;
    swarm *group;
    int16_t component_count;
    swarm_component *component;
    float dx, dy, dz;
    float distance_squared;
    float min_distance_squared;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->swarm == 0) {
        src = (uint32_t *)&self->aim_origin;
        for (i = 0xe; i != 0; i--) {
            *out_block = *src;
            src++;
            out_block++;
        }
        return;
    }

    group = (swarm *)((uint8_t *)swarm_data->data + (self->swarm_index & 0xffff) * sizeof(swarm));
    component_count = group->component_count;
    min_distance_squared = 3.4028235e+38f;
    {
    datum_index nearest_unit = k_datum_index_none; // ebx

    if (0 < component_count) {
        for (i = 0; i < component_count; i++) {
            component = (swarm_component *)((uint8_t *)swarm_component_data->data +
                                            (group->component_index[i] & 0xffff) * sizeof(swarm_component));
            dx = query_point->x - component->position.x;
            dy = query_point->y - component->position.y;
            dz = query_point->z - component->position.z;
            distance_squared = dz * dz + dy * dy + dx * dx;
            if (distance_squared < min_distance_squared) {
                min_distance_squared = distance_squared;
                nearest_unit = group->unit_index[i]; // 0x41c285
            }
        }
    }

    // FIXED (objdump 0x41c296): EBX = the nearest component's unit, stack = the caller's block; the draft passed nothing.
    actor_fill_unit_position_context(nearest_unit, (actor_unit_position_context *)out_block);
    }
}

#if 0
Original Ghidra decompilation (0x41c1e0):

void actor_get_firing_positions(void)

{
  ushort uVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  uint in_EAX;
  int iVar5;
  int iVar6;
  undefined4 *in_ECX;
  float *in_EDX;
  uint uVar7;
  undefined4 *puVar8;
  float local_4;

  iVar5 = (in_EAX & 0xffff) * 0x724;
  iVar6 = iVar5 + *(int *)(DAT_00880360 + 0x34);
  if (*(char *)(iVar5 + 6 + *(int *)(DAT_00880360 + 0x34)) == '\0') {
    puVar8 = (undefined4 *)(iVar6 + 0x120);
    for (iVar5 = 0xe; iVar5 != 0; iVar5 = iVar5 + -1) {
      *in_ECX = *puVar8;
      puVar8 = puVar8 + 1;
      in_ECX = in_ECX + 1;
    }
    return;
  }
  iVar5 = (*(uint *)(iVar6 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
  uVar1 = *(ushort *)(iVar5 + 2);
  local_4 = 3.4028235e+38;
  if (0 < (short)uVar1) {
    iVar5 = iVar5 + 0x18;
    uVar7 = (uint)uVar1;
    do {
      iVar6 = (*(uint *)(iVar5 + 0x40) & 0xffff) * 0x40;
      fVar2 = *in_EDX - *(float *)(iVar6 + 4 + *(int *)(DAT_00880358 + 0x34));
      iVar6 = iVar6 + 4 + *(int *)(DAT_00880358 + 0x34);
      fVar4 = in_EDX[1] - *(float *)(iVar6 + 4);
      fVar3 = in_EDX[2] - *(float *)(iVar6 + 8);
      fVar2 = fVar3 * fVar3 + fVar4 * fVar4 + fVar2 * fVar2;
      if (fVar2 < local_4) {
        local_4 = fVar2;
      }
      iVar5 = iVar5 + 4;
      uVar7 = uVar7 - 1;
    } while (uVar7 != 0);
  }
  FUN_004296c0();
  return;
}
#endif
