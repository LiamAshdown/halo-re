// actor_update_swarm_component_position  (Ghidra: actor_update_swarm_component_position, renamed)
// address 0x428130, size 80 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: types/ai.h swarm_component.position (0x04, an object_get_position out-parameter)
//   and swarm_component.marker_index (0x10); types/objects.h object.type (0xb4); calls
//   object_get_position (0x4f6900, already established: EAX -> out_position,
//   ECX -> object_index).
//   UNSURE: unit+0x4d8 is named ground_surface_index in types/units.h for the general unit
//   case, which fits the "recorded marker/surface" idea loosely but was not independently
//   re-derived here.
// register convention: EAX -> component_index, ECX -> unit_index.
//   // blam-cc: EAX -> component_index, ECX -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_ai.h"

extern data_array *object_data;          // 0x008603b0
extern data_array *swarm_component_data; // 0x00880358

extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900

// blam-cc: EAX -> component_index, ECX -> unit_index
// Given a swarm-slot index and a unit index, records either that unit's ground/death
// position marker (when it is a biped) or none into the shared swarm-member position cache,
// alongside the unit's current position.
void actor_update_swarm_component_position(datum_index component_index, datum_index unit_index)
{
    object *unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    swarm_component *component = &((swarm_component *)swarm_component_data->data)[component_index & 0xffff];
    datum_index marker;

    marker = (unit_object->type == 0) ? *(datum_index *)((uint8_t *)unit_object + 0x4d8) : (datum_index)k_datum_index_none;

    object_get_position(&component->position, unit_index);
    component->marker_index = marker;
}

#if 0
Original Ghidra decompilation (0x428130):

void FUN_00428130(void)

{
  int iVar1;
  int iVar2;
  uint in_EAX;
  uint in_ECX;
  undefined4 uVar3;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  iVar2 = *(int *)(DAT_00880358 + 0x34);
  uVar3 = 0xffffffff;
  if (*(short *)(iVar1 + 0xb4) == 0) {
    uVar3 = *(undefined4 *)(iVar1 + 0x4d8);
  }
  object_get_position();
  *(undefined4 *)((in_EAX & 0xffff) * 0x40 + iVar2 + 0x10) = uVar3;
  return;
}
#endif
