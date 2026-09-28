// actor_unlink_unit  (Ghidra: actor_unlink_unit, already named)
// address 0x427bc0, size 197 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: types/ai.h actor.unit_index (0x18)/counts_toward_encounter (0x1c)/
//   encounter_index (0x34); encounter.live_count (0x1c); types/objects.h object_header
//   (stride 0xc, flags at +0x02) and object.parent_object (0x11c)/location_cluster_index
//   (0x9c). Calls unit_refresh_targeting_flag_and_weapons (0x569bf0, not in this rewrite range, UNSURE signature).
// register convention: EAX -> actor_index; no other register operands are read.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data;     // 0x00880360
extern data_array *object_data;    // 0x008603b0
extern data_array *encounter_data; // 0x008802c8

extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_index, uint8_t initial_targeting_flag); // 0x569bf0, stack, CL

// blam-cc: EAX -> actor_index
// Detaches the actor from its single bound unit, marking the object header's "in PVS pass"
// bit and, if the unit was never actually placed (no parent, no cluster), clearing its
// active bit too. Clears the unit's back-reference, decrements the owning encounter's live
// count if this actor was counted toward it, and clears both of the actor's own links.
void actor_unlink_unit(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    datum_index unit_index = self->unit_index;

    if (unit_index != (datum_index)k_datum_index_none) {
        object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
        object *unit_object = header->data;

        header->flags |= _object_header_in_pvs_pass_bit;
        if (unit_object->parent_object == (datum_index)k_datum_index_none &&
            unit_object->location_cluster_index == -1) {
            if ((header->flags & _object_header_active_bit) != 0) {
                header->flags &= ~_object_header_active_bit;
            }
        }

        unit_refresh_targeting_flag_and_weapons(unit_index, 0); // CL = 0

        *(datum_index *)((uint8_t *)unit_object + 500) = (datum_index)k_datum_index_none; // unit_data.actor_index (object+0x1f4)

        if (self->counts_toward_encounter != 0 && self->encounter_index != (datum_index)k_datum_index_none) {
            encounter *enc = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
            enc->live_count = enc->live_count - 1;
        }

        self->unit_index = (datum_index)k_datum_index_none;
        self->counts_toward_encounter = 0;
    }
}

#if 0
Original Ghidra decompilation (0x427bc0):

void actor_unlink_unit(void)

{
  short *psVar1;
  byte *pbVar2;
  byte bVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint in_EAX;
  int iVar8;
  int iVar9;

  iVar7 = DAT_008603b0;
  iVar8 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar4 = *(uint *)(iVar8 + 0x18);
  if (uVar4 != 0xffffffff) {
    iVar5 = *(int *)(DAT_008603b0 + 0x34);
    iVar6 = *(int *)(iVar5 + 8 + (uVar4 & 0xffff) * 0xc);
    iVar9 = (uVar4 & 0xffff) * 0xc;
    pbVar2 = (byte *)(iVar9 + 2 + iVar5);
    iVar5 = *(int *)(iVar9 + 8 + iVar5);
    *pbVar2 = *pbVar2 | 0x40;
    if ((*(int *)(iVar5 + 0x11c) == -1) && (*(short *)(iVar5 + 0x9c) == -1)) {
      iVar9 = *(int *)(iVar7 + 0x34) + iVar9;
      bVar3 = *(byte *)(iVar9 + 2);
      if ((bVar3 & 1) != 0) {
        *(byte *)(iVar9 + 2) = bVar3 & 0xfe;
      }
    }
    FUN_00569bf0(*(undefined4 *)(iVar8 + 0x18));
    *(undefined4 *)(iVar6 + 500) = 0xffffffff;
    if ((*(char *)(iVar8 + 0x1c) != '\0') && (*(uint *)(iVar8 + 0x34) != 0xffffffff)) {
      psVar1 = (short *)((*(uint *)(iVar8 + 0x34) & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34) +
                        0x1c);
      *psVar1 = *psVar1 + -1;
    }
    *(undefined4 *)(iVar8 + 0x18) = 0xffffffff;
    *(undefined1 *)(iVar8 + 0x1c) = 0;
  }
  return;
}
#endif
