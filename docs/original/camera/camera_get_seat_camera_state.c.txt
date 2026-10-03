// camera_get_seat_camera_state  (Ghidra: FUN_00445b20; renamed, Blam-style, not previously named)
// address 0x445b20, size 219 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/camera_functions.md "Determines which non-default camera type a seated
//   unit should use, based on the vehicle seat's camera-related flags." Every field/flag this
//   reads matches types/camera.h director_seat_camera_state and types/tags.h UnitSeatFlags
//   exactly (bit 0x10 third_person_camera, bit 0x40 third_person_on_enter) and
//   types/units.h unit_animation_state (0x1a seat_enter, 0x1b seat_exit).
// review fixes (phase 4 gate): the result is a 16-bit value (0x445bef mov ax,di; both callers
//   test cmp ax,0x1), and the unit fields are read through unit_data at object +0x1f4
//   (+0x2f0 seat index, +0x2a3 animation state); the earlier cast of the object pointer
//   itself to unit_data read 0x1f4 bytes too early.
// register convention: unit handle in ECX (in_ECX); output director_seat_camera_state as the
//   single cdecl stack parameter (confirmed with objdump: `mov ebp,[esp+8]` right after the
//   prologue, `mov word ptr [ebp],di` writes through it before ECX is even tested).
//   // blam-cc: ECX -> unit, stack -> out_state
// UNSURE: this reads the parent object directly out of the object_header data_array (no
//   object_try_and_get validity/type check), matching the compiled code exactly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: ECX -> unit, stack -> out_state; result in AX
// True when the seated unit's vehicle seat has UnitSeatFlags::third_person_camera set. Also
// reports whether the seat is mid seat-enter/seat-exit animation through *out_state, which take
// priority over the plain "seated" state.
int16_t camera_get_seat_camera_state(datum_index unit, int16_t *out_state)
{
    object_header *headers = (object_header *)object_data->data;
    object *unit_object;
    datum_index parent;
    int16_t result = 0;

    *out_state = _director_seat_camera_none;
    if (unit == k_datum_index_none) {
        return 0;
    }

    unit_object = headers[unit & 0xffff].data;
    parent = unit_object->parent_object;
    if (parent == k_datum_index_none) {
        return 0;
    }

    {
        object *parent_object = headers[parent & 0xffff].data;
        if ((1 << (parent_object->type & 0x1f)) & 3) {
            Unit *parent_unit_tag = (Unit *)tag_instances[parent_object->definition_tag & 0xffff].data;
            uint8_t *seats = (uint8_t *)parent_unit_tag->seats.pointer;
            int16_t seat_index = ((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))->vehicle_seat_index;
            uint32_t seat_flags = *(uint32_t *)(seats + (int32_t)seat_index * sizeof(UnitSeat));

            result = (seat_flags & 0x10) != 0; // UnitSeatFlags::third_person_camera

            if ((seat_flags & 0x40) != 0) { // UnitSeatFlags::third_person_on_enter
                if (((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))->animation_state == _unit_animation_state_seat_enter) {
                    *out_state = _director_seat_camera_entering;
                    return 1;
                }
                if (((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))->animation_state == _unit_animation_state_seat_exit) {
                    *out_state = _director_seat_camera_exiting;
                    return 1;
                }
            }
        }

        *out_state = _director_seat_camera_seated;
        return result;
    }
}

#if 0
Original Ghidra decompilation (0x445b20):

bool FUN_00445b20(undefined2 *param_1)

{
  int iVar1;
  uint *puVar2;
  uint uVar3;
  uint in_ECX;
  bool bVar4;
  bool bVar5;

  bVar5 = false;
  *param_1 = 0;
  if (in_ECX == 0xffffffff) {
    return false;
  }
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  bVar4 = false;
  if (*(uint *)(iVar1 + 0x11c) != 0xffffffff) {
    puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                       (*(uint *)(iVar1 + 0x11c) & 0xffff) * 0xc);
    if (((1 << ((byte)puVar2[0x2d] & 0x1f) & 3U) != 0) &&
       (uVar3 = *(uint *)(*(short *)(iVar1 + 0x2f0) * 0x11c +
                         *(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8)
                         ), bVar5 = (uVar3 & 0x10) != 0, (uVar3 >> 6 & 1) != 0)) {
      if (*(char *)(iVar1 + 0x2a3) == '\x1a') {
        *param_1 = 1;
        return true;
      }
      if (*(char *)(iVar1 + 0x2a3) == '\x1b') {
        *param_1 = 3;
        return true;
      }
    }
    *param_1 = 2;
    bVar4 = bVar5;
  }
  return bVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
