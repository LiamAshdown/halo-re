// actor_link_to_unit_cluster  (Ghidra: actor_link_to_unit_cluster, renamed)
// address 0x4279f0, size 457 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: types/ai.h actor.cluster_unit_index(0x24)/cluster_count(0x1e)/unknown_20(0x20)/
//   swarm_index(0x28)/encounter_index(0x34)/team(0x3e)/keep_unit_alive(0x13); types/units.h
//   unit_data.swarm_actor_index(0x1f8)/swarm_next_unit_index(0x1fc), and the same unnamed
//   "previous" link at +0x200 used by actor_remove_from_unit_cluster @0x427c90 (this
//   function is the one that undoes). Shares its whole tail (encounter-team copy, object
//   header flag twiddling, object_mark_pending_delete, unit_refresh_targeting_flag_and_weapons) with
//   actor_attach_to_unit @0x427560, which has the identical sequence.
//   UNSURE: ai_encounter_stamp_team_from_unit's real behavior/signature is not established anywhere in this
//   module (outside this rewrite's range); called here with no visible arguments in Ghidra's
//   decompile, kept that way. The boolean return here (Ghidra's CONCAT31 pattern) is real,
//   unlike the noise seen on actor_scale_value_by_ally_exposure.
// register convention: EAX -> actor_index, stack -> unit_index (Ghidra already resolved the
//   stack argument; the register argument is Ghidra's own "param_1").
//   // blam-cc: EAX -> actor_index, stack -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_ai.h"
#include "fn_memory.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *swarm_data;      // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358
extern data_array *encounter_data;  // 0x008802c8


extern void object_mark_pending_delete(datum_index object_index); // 0x4f50f0, UNSURE signature
extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_index, uint8_t initial_targeting_flag); // 0x569bf0, stack, CL

// blam-cc: EAX -> actor_index, stack -> unit_index
// Adds an actor to a unit's multi-actor cluster (linked list of controlling actors),
// creating a backing swarm_component entry when the actor already has a swarm, and undoing
// whatever the unit and the actor were previously attached to first. Returns true if the
// actor ends up linked (including the trivial case where it already was), false only when a
// required swarm_component datum could not be allocated.
uint8_t actor_link_to_unit_cluster(datum_index actor_index, datum_index unit_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
    object *unit_object = header->data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);
    datum_index new_component = (datum_index)k_datum_index_none;

    if (unit->swarm_actor_index == actor_index) {
        return 1;
    }

    if (self->swarm_index != (datum_index)k_datum_index_none) {
        new_component = datum_new(swarm_component_data);
        if (new_component == (datum_index)k_datum_index_none) {
            return 0;
        }
    }

    if (unit->swarm_actor_index != (datum_index)k_datum_index_none) {
        actor_remove_from_unit_cluster(unit->swarm_actor_index, unit_index);
    }
    if (unit->actor_index != (datum_index)k_datum_index_none) {
        actor_delete(unit->actor_index, 0);
    }
    if (self->unit_index != (datum_index)k_datum_index_none) {
        actor_unlink_unit(actor_index);
    }

    unit->swarm_actor_index = actor_index;
    unit->swarm_next_unit_index = self->cluster_unit_index;
    *(uint32_t *)((uint8_t *)unit_object + 0x200) = 0xffffffff; // UNSURE: unnamed "previous" link
    if (self->cluster_unit_index != (datum_index)k_datum_index_none) {
        object *head_object = ((object_header *)object_data->data)[self->cluster_unit_index & 0xffff].data;
        *(uint32_t *)((uint8_t *)head_object + 0x200) = unit_index; // UNSURE, see above
    }
    self->cluster_unit_index = unit_index;

    if (self->swarm_index != (datum_index)k_datum_index_none) {
        swarm_add_component(new_component, unit_index, self->swarm_index);
    }

    self->cluster_count = self->cluster_count + 1;
    self->unknown_20 = self->unknown_20 + 1;

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
        ai_encounter_stamp_team_from_unit(self->encounter_index, unit_index); // 0x427b33: EAX = encounter, ECX = unit
        ((struct object *)unit_object)->owner_team = enc->team; // UNSURE: object+0xb8, see actor_attach_to_unit
    }
    self->team = ((struct object *)unit_object)->owner_team;

    {
        uint8_t flags_before = header->flags;
        header->flags = flags_before & ~_object_header_in_pvs_pass_bit;
        if ((flags_before & _object_header_active_bit) == 0) {
            object_mark_pending_delete(unit_index);
        }
        if (self->keep_unit_alive == 0) {
            object_mark_pending_delete(unit_index);
        } else if ((header->flags & _object_header_active_bit) != 0) {
            header->flags &= ~_object_header_active_bit;
        }
    }

    unit_refresh_targeting_flag_and_weapons(unit_index, 1); // CL = 1
    return 1;
}

#if 0
Original Ghidra decompilation (0x4279f0):

undefined4 FUN_004279f0(uint param_1,uint param_2)

{
  byte bVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  undefined3 uVar5;
  undefined3 extraout_var;
  int iVar6;
  int iVar7;
  bool local_5;

  iVar7 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar6 = (param_2 & 0xffff) * 0xc;
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  local_5 = true;
  if (*(uint *)(iVar2 + 0x1f8) == param_1) {
    return CONCAT31((int3)((param_2 & 0xffff) >> 8),1);
  }
  if (*(int *)(iVar7 + 0x28) != -1) {
    iVar4 = datum_new();
    local_5 = iVar4 != -1;
    uVar5 = (undefined3)((uint)iVar4 >> 8);
    if (!local_5) goto LAB_00427bad;
  }
  if (*(int *)(iVar2 + 0x1f8) != -1) {
    actor_remove_from_unit_cluster(param_2);
  }
  if (*(int *)(iVar2 + 500) != -1) {
    actor_delete(0);
  }
  if (*(int *)(iVar7 + 0x18) != -1) {
    actor_unlink_unit();
  }
  *(uint *)(iVar2 + 0x1f8) = param_1;
  *(undefined4 *)(iVar2 + 0x1fc) = *(undefined4 *)(iVar7 + 0x24);
  *(undefined4 *)(iVar2 + 0x200) = 0xffffffff;
  if (*(uint *)(iVar7 + 0x24) != 0xffffffff) {
    *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar7 + 0x24) & 0xffff) * 0xc)
             + 0x200) = param_2;
  }
  *(uint *)(iVar7 + 0x24) = param_2;
  if (*(int *)(iVar7 + 0x28) != -1) {
    swarm_add_component();
  }
  *(short *)(iVar7 + 0x1e) = *(short *)(iVar7 + 0x1e) + 1;
  uVar3 = *(uint *)(iVar7 + 0x34);
  *(short *)(iVar7 + 0x20) = *(short *)(iVar7 + 0x20) + 1;
  if (uVar3 != 0xffffffff) {
    iVar4 = *(int *)(DAT_008802c8 + 0x34);
    FUN_00436710();
    *(undefined2 *)(iVar2 + 0xb8) = *(undefined2 *)((uVar3 & 0xffff) * 0x6c + iVar4 + 2);
  }
  iVar4 = DAT_008603b0;
  *(undefined2 *)(iVar7 + 0x3e) = *(undefined2 *)(iVar2 + 0xb8);
  bVar1 = *(byte *)(*(int *)(iVar4 + 0x34) + 2 + iVar6);
  *(byte *)(*(int *)(iVar4 + 0x34) + iVar6 + 2) = bVar1 & 0xbf;
  if ((bVar1 & 1) == 0) {
    object_mark_pending_delete();
  }
  if (*(char *)(iVar7 + 0x13) == '\0') {
    object_mark_pending_delete();
  }
  else {
    bVar1 = *(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + iVar6);
    if ((bVar1 & 1) != 0) {
      *(byte *)(*(int *)(DAT_008603b0 + 0x34) + iVar6 + 2) = bVar1 & 0xfe;
    }
  }
  FUN_00569bf0(param_2);
  uVar5 = extraout_var;
LAB_00427bad:
  return CONCAT31(uVar5,local_5);
}
#endif
