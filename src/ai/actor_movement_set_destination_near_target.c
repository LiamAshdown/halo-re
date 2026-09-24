// actor_movement_set_destination_near_target  (Ghidra: actor_movement_set_destination_near_target, already named)
// address 0x417910, size 284 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: builds a type-5 movement action whose destination.x/y are reused as a raw prop
// handle and a radius (not a real point -- Ghidra's own *(uint*) compare on destination.x
// confirms this), and whose extra is the prop's relationship object (prop+0x110) or, when
// that is none, the prop's own tracked object (prop+0x18).
// register convention: target prop index in EAX (Ghidra's in_EAX); actor_index and radius
// are genuine stack parameters.
// blam-cc: EAX -> target_prop_index, stack -> actor_index, stack -> radius
// UNSURE: parameter (actor_movement_action+0x10) is never written by this function, on
// either path; it is left at whatever actor.queued_movement.parameter already held, matching
// Ghidra exactly (the 6-dword queued-to-active copy carries the stale value along).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

extern void actor_set_units_active(datum_index actor_index); // 0x427860, EAX -> actor_index
extern uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t param_2, int32_t param_3); // 0x41a460, this module

// blam-cc: EAX -> target_prop_index, stack -> actor_index, stack -> radius
uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index, float radius)
{
    actor *self;
    prop *target;
    int32_t extra;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    self->firing_position_index = -1;
    actor_set_units_active(actor_index);

    if (self->active_movement.type == 5 && *(uint32_t *)&self->active_movement.destination.x == (uint32_t)target_prop_index) {
        if (self->active_movement.destination.y == radius) {
            if (self->needs_new_path != 0 && self->unknown_4a4 == 0) {
                return actor_movement_action_resolve(actor_index, 0, 0);
            }
            return 1;
        }
    }

    *(uint32_t *)&self->queued_movement.destination.x = (uint32_t)target_prop_index;
    target = &((prop *)prop_data->data)[target_prop_index & 0xffff];
    self->queued_movement.type = 5;
    self->queued_movement.cancelled = 0;
    self->queued_movement.destination.y = radius;
    extra = target->relationship_object_index;
    if (extra == -1) {
        extra = (int32_t)target->object_index;
    }
    self->queued_movement.extra = (uint32_t)extra;
    self->active_movement = self->queued_movement;
    return actor_movement_action_resolve(actor_index, 1, 0);
}

#if 0
Original Ghidra decompilation (0x417910):

undefined4 actor_movement_set_destination_near_target(uint param_1,float param_2)

{
  float fVar1;
  uint in_EAX;
  undefined2 extraout_var;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  int iVar5;

  iVar5 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  *(undefined2 *)(iVar5 + 0x3b8) = 0xffff;
  actor_set_units_active();
  if ((*(short *)(iVar5 + 0x46c) == 5) && (*(uint *)(iVar5 + 0x470) == in_EAX)) {
    fVar1 = *(float *)(iVar5 + 0x474);
    if (fVar1 == param_2) {
      if ((*(char *)(iVar5 + 0x4c) != '\0') && (*(char *)(iVar5 + 0x4a4) == '\0')) {
        uVar2 = actor_movement_action_resolve(param_1,0,0);
        return uVar2;
      }
      return CONCAT31((int3)(CONCAT22(extraout_var,
                                      (ushort)(fVar1 < param_2) << 8 |
                                      (ushort)(NAN(fVar1) || NAN(param_2)) << 10 |
                                      (ushort)(fVar1 == param_2) << 0xe) >> 8),1);
    }
  }
  iVar3 = *(int *)(DAT_008802c0 + 0x34);
  *(uint *)(iVar5 + 0x404) = in_EAX;
  iVar3 = (in_EAX & 0xffff) * 0x138 + iVar3;
  *(undefined2 *)(iVar5 + 0x400) = 5;
  *(undefined1 *)(iVar5 + 0x402) = 0;
  *(float *)(iVar5 + 0x408) = param_2;
  iVar4 = *(int *)(iVar3 + 0x110);
  if (iVar4 == -1) {
    iVar4 = *(int *)(iVar3 + 0x18);
  }
  *(int *)(iVar5 + 0x414) = iVar4;
  *(undefined4 *)(iVar5 + 0x46c) = *(undefined4 *)(iVar5 + 0x400);
  *(undefined4 *)(iVar5 + 0x470) = *(undefined4 *)(iVar5 + 0x404);
  *(undefined4 *)(iVar5 + 0x474) = *(undefined4 *)(iVar5 + 0x408);
  *(undefined4 *)(iVar5 + 0x478) = *(undefined4 *)(iVar5 + 0x40c);
  *(undefined4 *)(iVar5 + 0x47c) = *(undefined4 *)(iVar5 + 0x410);
  *(undefined4 *)(iVar5 + 0x480) = *(undefined4 *)(iVar5 + 0x414);
  uVar2 = actor_movement_action_resolve(param_1,1,0);
  return uVar2;
}
#endif
