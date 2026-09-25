// unit_noop_569670  (Ghidra: FUN_00569670)
// address 0x569670, size 118 bytes, name confidence 0.25, rewrite confidence 0.9
// functions.md: "Currently a no-op; its referenced globals suggest it once performed unit/tag-
// table work that has since been inlined away."
// UNSURE: Ghidra's own decompilation is a bare `return;`, but that is wrong -- objdump
// 0x569670..0x5696e5 shows a real, non-trivial body (118 bytes, EAX read live at entry, three
// callers in camera_observer_*.c that all capture its return value). Ghidra apparently failed to
// recover the EAX-passed argument and rendered the whole function as unreachable/empty. Not
// renamed to a real Blam name since this pass only recovers the register input; the shape
// (resolve to the parent object when seated in an invisible/gunner seat, else return the input
// unchanged) matches this module's exclude_object usage in camera_observer_update.c.
// FIXED (register inputs, objdump): EAX (object_index) is read live at entry (0x569670
// `cmp eax,0xffffffff`) and the function was previously a stubbed-out no-op that never used it;
// transcribed the real body from the disassembly instead. No blam-cc note existed before.
// blam-cc: EAX -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

uint32_t unit_noop_569670(uint32_t object_index)
{
    uint32_t result = object_index;

    if (object_index != 0xffffffff) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

        if (obj->parent_object != (datum_index)0xffffffff) {
            unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

            if (unit->vehicle_seat_index != -1) {
                object *parent = ((object_header *)object_data->data)[(uint16_t)obj->parent_object].data;
                Unit *parent_tag = (Unit *)tag_instances[(uint16_t)parent->definition_tag].data;
                UnitSeat *seat = (UnitSeat *)((uint8_t *)parent_tag->seats.pointer +
                                               (uint32_t)unit->vehicle_seat_index * 0x11c);

                if ((seat->flags & 0x9) != 0) { // UnitSeatFlags invisible(0x1) | gunner(0x8)
                    result = (uint32_t)obj->parent_object;
                }
            }
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x569670):

void FUN_00569670(void)

{
  return;
}
#endif
