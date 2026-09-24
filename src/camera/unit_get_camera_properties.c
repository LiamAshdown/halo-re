// unit_get_camera_properties  (Ghidra: FUN_00447110; renamed for this rewrite)
// address 0x447110, size 127 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/camera_types_notes.md's unit_camera_properties section ("FUN_00447110
// returns UnitSeat +0x84 or Unit +0x1a8"); confirmed field-for-field against objdump: the
// 0x11c/0x1a8/0x2e8/0x2f0 offsets match Unit/UnitSeat (types/tags.h) and unit_data (types/units.h)
// exactly, and the object_try_and_get call site (ECX still holds the parent handle loaded two
// instructions earlier, stack argument 2 = _object_mask_vehicle) matches its declared prototype.
// register convention: unit's datum_index in EAX (in_EAX), no other parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "camera.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

// blam-cc: object_index in ECX, kind mask on the stack (0x4f6ec0; verified at this call site)
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);

// blam-cc: EAX -> unit
// Returns a unit's camera marker/animation data block: if the unit is seated in a vehicle,
// the seat's own block (UnitSeat +0x84) when the seat's camera flags say to use it; otherwise
// the unit's own block (Unit tag +0x1a8).
unit_camera_properties *unit_get_camera_properties(datum_index unit)
{
    object *unit_object;
    object *vehicle_object;
    unit_data *unit_extension;
    Unit *vehicle_tag;
    UnitSeat *seat;

    unit_object = ((object_header *)object_data->data)[unit & 0xffff].data;

    if (unit_object->parent_object != (datum_index)k_datum_index_none) {
        vehicle_object = object_try_and_get(unit_object->parent_object, _object_mask_vehicle);
        if (vehicle_object != 0) {
            unit_extension = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);
            vehicle_tag = (Unit *)tag_instances[vehicle_object->definition_tag & 0xffff].data;
            seat = &((UnitSeat *)vehicle_tag->seats.pointer)[unit_extension->vehicle_seat_index];
            if ((seat->flags & 0x15) != 0) {
                // UNSURE: UnitSeatFlags has no published bit numbering in types/tags.h, so the
                // three flags this mask covers are not named individually.
                return (unit_camera_properties *)&seat->camera_marker_name;
            }
        }
    }
    return (unit_camera_properties *)
        ((uint8_t *)tag_instances[unit_object->definition_tag & 0xffff].data + 0x1a8);
}

#if 0
Original Ghidra decompilation (0x447110):

int FUN_00447110(void)

{
  uint *puVar1;
  int iVar2;
  uint in_EAX;
  uint *puVar3;
  int iVar4;
  int iVar5;

  iVar2 = DAT_0087bc14;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((((puVar1[0x47] != 0xffffffff) &&
       (puVar3 = (uint *)object_try_and_get(2), puVar3 != (uint *)0x0)) &&
      (iVar5 = *(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + iVar2) + 0x2e8),
      iVar4 = (short)puVar1[0xbc] * 0x11c, (*(byte *)(iVar4 + iVar5) & 0x15) != 0)) &&
     (iVar5 = iVar4 + iVar5 + 0x84, iVar5 != 0)) {
    return iVar5;
  }
  return *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + iVar2) + 0x1a8;
}
#endif
