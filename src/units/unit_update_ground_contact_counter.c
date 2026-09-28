// unit_update_ground_contact_counter  (Ghidra: FUN_00575640; renamed from the phase2 proposal)
// address 0x575640, size 169 bytes
// name confidence: 0.4 (phase2 proposal at 0.4, matches functions.md summary)
// rewrite confidence: 0.9 (VERIFIED against 0x575640 (saturating airborne/landing counters, contact flags 2 / 0x10; offsets asserted)) -- the contact-point array pointer (unaff_EDI) is register-only and
//   not recoverable; passed in explicitly here as a parameter instead.
// evidence: types/units.h vehicle_data.airborne_ticks (0x4d0), .landing_ticks (0x4d3); the
//   physics.tag_id-at-0x8c double-tag_instances-lookup idiom (Vehicle tag -> Physics tag,
//   field at +0x74 read as a contact-point count) matches the sibling functions in this batch.
// register convention: unit object index in EAX (in_EAX); a contact-point array pointer in EDI
//   (unaff_EDI, stride 0x130).
//   // blam-cc: EAX -> unit_index, EDI -> contact_points
// UNSURE: contact_points' record shape (only the leading flags dword, bits 2 and 0x10, is
//   read here) is not documented anywhere in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// Tracks how long the unit has been airborne, resetting the counter and bumping a
// landing-recovery counter whenever any contact-point marker reports full (bit 2) or partial
// (bit 0x10) ground contact.
void unit_update_ground_contact_counter(uint32_t unit_index, uint8_t *contact_points)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t *physics_tag = tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
    int32_t count = *(int32_t *)(physics_tag + 0x74);
    int32_t i;

    if (vehicle->airborne_ticks != 0xff) {
        vehicle->airborne_ticks += 1;
    }

    for (i = 0; i < count; i++) {
        uint32_t flags = *(uint32_t *)(contact_points + i * 0x130);
        if ((flags & 2) != 0) {
            vehicle->airborne_ticks = 0;
            if ((int8_t)vehicle->landing_ticks != -1) {
                vehicle->landing_ticks += 1;
            }
            return;
        }
        if ((flags & 0x10) != 0) {
            vehicle->airborne_ticks = 0;
        }
    }
    vehicle->landing_ticks = 0;
}

#if 0
Original Ghidra decompilation (0x575640):

void FUN_00575640(void)

{
  uint *puVar1;
  int iVar2;
  uint uVar3;
  uint in_EAX;
  int iVar4;
  short sVar5;
  int unaff_EDI;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar2 = *(int *)((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x8c) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((char)puVar1[0x134] != -1) {
    *(char *)(puVar1 + 0x134) = (char)puVar1[0x134] + '\x01';
  }
  sVar5 = 0;
  if (0 < *(int *)(iVar2 + 0x74)) {
    iVar4 = 0;
    do {
      uVar3 = *(uint *)(iVar4 * 0x130 + unaff_EDI);
      if ((uVar3 & 2) != 0) {
        *(undefined1 *)(puVar1 + 0x134) = 0;
        if (*(char *)((int)puVar1 + 0x4d3) == -1) {
          return;
        }
        *(char *)((int)puVar1 + 0x4d3) = *(char *)((int)puVar1 + 0x4d3) + '\x01';
        return;
      }
      if ((uVar3 & 0x10) != 0) {
        *(undefined1 *)(puVar1 + 0x134) = 0;
      }
      sVar5 = sVar5 + 1;
      iVar4 = (int)sVar5;
    } while (iVar4 < *(int *)(iVar2 + 0x74));
  }
  *(undefined1 *)((int)puVar1 + 0x4d3) = 0;
  return;
}
#endif
