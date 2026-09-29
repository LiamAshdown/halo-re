// actor_set_units_active  (Ghidra: actor_set_units_active, already named)
// address 0x427860, size 305 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x427860..0x427990 (EAX actor, BL dormant).)
// evidence: types/ai.h actor.active(0x08)/keep_unit_alive(0x13)/swarm(0x06)/
//   swarm_index(0x28)/cluster_unit_index(0x24)/idle_counter(0x4a per the 0x14 clear, see
//   UNSURE); types/objects.h object_header (stride 0xc, flags at +0x02,
//   _object_header_active_bit). Calls object_mark_pending_delete / object_clear_pending_delete_flag
//   (0x4f50f0 / 0x4f5130), both UNSURE signature (not established elsewhere in this repo).
//   UNSURE: iVar4+0x14 (cleared to 0 when deactivating) does not obviously correspond to any
//   single named actor field at that byte offset (0x14 falls inside the small run between
//   unit_index and counts_toward_encounter that types/ai.h already names individually);
//   likely a decompiler mis-slice of adjacent fields. Left as a raw offset.
// register convention: EAX -> actor_index, BL -> activate.
//   // blam-cc: EAX -> actor_index, EBX -> activate

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_ai.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *swarm_data;  // 0x0088035c
extern data_array *object_data; // 0x008603b0

extern void object_mark_pending_delete(datum_index object_index); // 0x4f50f0, UNSURE signature
extern void object_clear_pending_delete_flag(datum_index object_index); // 0x4f5130, UNSURE signature

// blam-cc: EAX -> actor_index, EBX -> dormant
// Puts the actor's units to sleep (dormant = 1) or wakes them (dormant = 0), whether it has a lone unit, a cluster
// (walked through swarm_next_unit_index) or a swarm (its component unit_index[] array). Header byte +2 bit 0 is the
// object's active bit: waking sets it (0x4f50f0, unless the object is parented or flagged 0x100000), sleeping clears
// it (0x4f5130). Actor +0x13 records the state (a sleeping actor's +0x14 counter resets). No-op unless the actor is
// active (+0x08) and not already in the requested state. BL is 1 at 0x42781d (actor deactivation), 0x429406 (idle
// timeout), 0x437918 and 0x438114 (encounter deactivation); every other call site passes 0.
void actor_set_units_active(datum_index actor_index, uint8_t dormant)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->active != 0 && self->keep_unit_alive != dormant) {
        if (self->swarm == 0) {
            if (self->unit_index != (datum_index)k_datum_index_none) {
                if (dormant == 0) {
                    object_mark_pending_delete(self->unit_index);
                } else {
                    object_clear_pending_delete_flag(self->unit_index);
                }
            }
        } else if (self->swarm_index == (datum_index)k_datum_index_none) {
            datum_index unit_index = self->cluster_unit_index;
            while (unit_index != (datum_index)k_datum_index_none) {
                object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
                object *unit_object = header->data;

                if (dormant == 0) {
                    object_mark_pending_delete(unit_index);
                } else if ((header->flags & _object_header_active_bit) != 0) {
                    header->flags &= ~_object_header_active_bit;
                }
                unit_index = *(datum_index *)((uint8_t *)unit_object + 0x1fc); // unit_data.swarm_next_unit_index
            }
        } else {
            swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
            int16_t i;

            for (i = 0; i < s->component_count; i++) {
                if (dormant == 0) {
                    object_mark_pending_delete(s->unit_index[i]);
                } else {
                    object_header *header = &((object_header *)object_data->data)[s->unit_index[i] & 0xffff];
                    if ((header->flags & _object_header_active_bit) != 0) {
                        header->flags &= ~_object_header_active_bit;
                    }
                }
            }
        }

        self->keep_unit_alive = dormant;
        if (dormant == 0) {
            *(int16_t *)((uint8_t *)self + 0x14) = 0; // UNSURE offset, see file header
        }
    }
}

#if 0
Original Ghidra decompilation (0x427860):

void actor_set_units_active(void)

{
  byte bVar1;
  uint uVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  int iVar6;
  char unaff_BL;
  short sVar7;

  iVar3 = DAT_008603b0;
  iVar4 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if ((*(char *)(iVar4 + 8) != '\0') && (*(char *)(iVar4 + 0x13) != unaff_BL)) {
    if (*(char *)(iVar4 + 6) == '\0') {
      if (*(int *)(iVar4 + 0x18) != -1) {
        if (unaff_BL == '\0') {
          object_mark_pending_delete();
        }
        else {
          object_clear_pending_delete_flag();
        }
      }
    }
    else if (*(uint *)(iVar4 + 0x28) == 0xffffffff) {
      uVar2 = *(uint *)(iVar4 + 0x24);
      while (uVar2 != 0xffffffff) {
        iVar6 = (uVar2 & 0xffff) * 0xc;
        iVar5 = *(int *)(iVar6 + 8 + *(int *)(iVar3 + 0x34));
        if (unaff_BL == '\0') {
          object_mark_pending_delete();
        }
        else {
          iVar6 = *(int *)(iVar3 + 0x34) + iVar6;
          bVar1 = *(byte *)(iVar6 + 2);
          if ((bVar1 & 1) != 0) {
            *(byte *)(iVar6 + 2) = bVar1 & 0xfe;
          }
        }
        uVar2 = *(uint *)(iVar5 + 0x1fc);
      }
    }
    else {
      iVar5 = (*(uint *)(iVar4 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
      sVar7 = 0;
      if (0 < *(short *)(iVar5 + 2)) {
        do {
          if (unaff_BL == '\0') {
            object_mark_pending_delete();
          }
          else {
            iVar6 = *(int *)(iVar3 + 0x34) + (*(uint *)(iVar5 + 0x18 + sVar7 * 4) & 0xffff) * 0xc;
            bVar1 = *(byte *)(iVar6 + 2);
            if ((bVar1 & 1) != 0) {
              *(byte *)(iVar6 + 2) = bVar1 & 0xfe;
            }
          }
          sVar7 = sVar7 + 1;
        } while (sVar7 < *(short *)(iVar5 + 2));
      }
    }
    *(char *)(iVar4 + 0x13) = unaff_BL;
    if (unaff_BL == '\0') {
      *(undefined2 *)(iVar4 + 0x14) = 0;
    }
  }
  return;
}
#endif
