// actor_attach_to_unit  (Ghidra: actor_attach_to_unit, already named)
// address 0x427560, size 382 bytes
// name confidence: 0.55   rewrite confidence: 0.35
// evidence: types/ai.h actor.unit_index(0x18)/encounter_index(0x34)/team(0x3e)/
//   counts_toward_encounter(0x1c); encounter.live_count(0x1c); types/units.h
//   unit_data.actor_index(object+0x1f4)/swarm_actor_index(object+0x1f8). Shares its tail
//   (encounter-team copy, object header flag twiddling, object_mark_pending_delete,
//   unit_refresh_targeting_flag_and_weapons) with actor_link_to_unit_cluster @0x4279f0, which has the identical
//   sequence (see that file for the flags-before/flags-after re-read this preserves).
//   UNSURE: object+0xbe (compared against 99) has no established field name; objects.h only
//   names +0xbc as an opaque unknown_0bc dword, so +0xbe would be its upper 16 bits. Kept as
//   a raw offset. ai_encounter_stamp_team_from_unit's real signature is unknown (outside this rewrite's range).
// register convention: Ghidra already resolved both parameters as ordinary parameters.
//   // blam-cc: EAX -> actor_index, ECX -> unit_index (matching the sibling function's
//   register roles; not independently re-verified with objdump for this file).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *encounter_data;  // 0x008802c8

extern void actor_unlink_unit(datum_index actor_index); // 0x427bc0
extern void actor_remove_from_unit_cluster(datum_index actor_index, datum_index unit_index); // 0x427c90
extern void actor_delete(datum_index actor_index, uint32_t flag); // 0x427e60
extern void actor_refresh_combat_context(datum_index actor_index); // 0x4297a0, in this rewrite range, not yet written when this file was authored
extern void ai_encounter_stamp_team_from_unit(void); // 0x436710, UNSURE signature, not in this rewrite range
extern void object_mark_pending_delete(datum_index object_index); // 0x4f50f0, UNSURE signature
extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_index); // 0x569bf0, UNSURE signature

// blam-cc: EAX -> actor_index, ECX -> unit_index
// Binds an actor to a unit object as its controller, cleaning up any prior bindings (the
// unit's existing cluster or direct controller, and this actor's own existing unit) first,
// then copies the owning encounter's team onto both, and if the unit's own placement id
// (offset 0xbe, see UNSURE) is 100 or greater, counts this actor toward its encounter's live
// count. Does nothing if the unit is already directly controlled by this actor.
void actor_attach_to_unit(datum_index actor_index, datum_index unit_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
    object *unit_object = header->data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);

    if (unit->actor_index == actor_index) {
        return;
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

    self->unit_index = unit_index;
    unit->actor_index = actor_index;

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
        ai_encounter_stamp_team_from_unit();
        *(int16_t *)((uint8_t *)unit_object + 0xb8) = enc->team; // UNSURE: object+0xb8, see actor_link_to_unit_cluster
    }
    self->team = *(int16_t *)((uint8_t *)unit_object + 0xb8);

    if (*(int16_t *)((uint8_t *)unit_object + 0xbe) > 99) { // UNSURE offset, see file header
        self->counts_toward_encounter = 1;
        if (self->encounter_index != (datum_index)k_datum_index_none) {
            encounter *enc = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
            enc->live_count = enc->live_count + 1;
        }
    }

    actor_refresh_combat_context(actor_index);

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

    unit_refresh_targeting_flag_and_weapons(unit_index);
}

#if 0
Original Ghidra decompilation (0x427560):

void actor_attach_to_unit(uint param_1,uint param_2)

{
  short *psVar1;
  byte bVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;

  iVar7 = (param_2 & 0xffff) * 0xc;
  iVar6 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
  if (*(uint *)(iVar3 + 500) != param_1) {
    if (*(int *)(iVar3 + 0x1f8) != -1) {
      actor_remove_from_unit_cluster(param_2);
    }
    if (*(int *)(iVar3 + 500) != -1) {
      actor_delete(0);
    }
    if (*(int *)(iVar6 + 0x18) != -1) {
      actor_unlink_unit();
    }
    *(uint *)(iVar6 + 0x18) = param_2;
    *(uint *)(iVar3 + 500) = param_1;
    uVar4 = *(uint *)(iVar6 + 0x34);
    if (uVar4 != 0xffffffff) {
      iVar5 = *(int *)(DAT_008802c8 + 0x34);
      FUN_00436710();
      *(undefined2 *)(iVar3 + 0xb8) = *(undefined2 *)((uVar4 & 0xffff) * 0x6c + iVar5 + 2);
    }
    *(undefined2 *)(iVar6 + 0x3e) = *(undefined2 *)(iVar3 + 0xb8);
    if (99 < *(short *)(iVar3 + 0xbe)) {
      *(undefined1 *)(iVar6 + 0x1c) = 1;
      if (*(uint *)(iVar6 + 0x34) != 0xffffffff) {
        psVar1 = (short *)((*(uint *)(iVar6 + 0x34) & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34)
                          + 0x1c);
        *psVar1 = *psVar1 + 1;
      }
    }
    actor_refresh_combat_context(param_1);
    iVar3 = DAT_008603b0;
    bVar2 = *(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + iVar7);
    *(byte *)(*(int *)(DAT_008603b0 + 0x34) + iVar7 + 2) = bVar2 & 0xbf;
    if ((bVar2 & 1) == 0) {
      object_mark_pending_delete();
    }
    if (*(char *)(iVar6 + 0x13) == '\0') {
      object_mark_pending_delete();
    }
    else {
      bVar2 = *(byte *)(*(int *)(iVar3 + 0x34) + 2 + iVar7);
      if ((bVar2 & 1) != 0) {
        *(byte *)(*(int *)(iVar3 + 0x34) + iVar7 + 2) = bVar2 & 0xfe;
      }
    }
    FUN_00569bf0(param_2);
  }
  return;
}
#endif
