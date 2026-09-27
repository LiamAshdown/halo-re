// biped_trigger_on_velocity_threshold  (Ghidra: biped_trigger_on_velocity_threshold, renamed)
// address 0x55ec20, size 109 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump 0x55ec20..0x55ec8c)
// evidence: biped_data.unknown_502 (types/units.h), object.velocity (0x068, objects.h).
// blam-cc: EAX -> object_index
// FIXED (register inputs, objdump): this file had no blam-cc note at all; EAX carries
// object_index (read at 0x55ec2a `mov ebx,eax`, then masked for the object lookup).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t unit_updates_suppressed; // 0x0071c419

extern void unit_fire_animation_sound_trigger(uint32_t unit_index, uint32_t trigger_kind, int16_t contact_point_index); // 0x560590, next batch

// Fires a paired trigger event (id 2) once a unit exceeds a small velocity threshold (~0.033
// units/tick) after having been still for more than 3 ticks (biped_data.unknown_502).
void biped_trigger_on_velocity_threshold(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if ((int8_t)biped->unknown_502 > 3 &&
        obj->velocity.i * obj->velocity.i + obj->velocity.j * obj->velocity.j +
                obj->velocity.k * obj->velocity.k > 0.0011111111f &&
        unit_updates_suppressed == 0) {
        // unit_index rides in EBX; Ghidra bound only the two stack arguments
        unit_fire_animation_sound_trigger(object_index, 2, 0);
        unit_fire_animation_sound_trigger(object_index, 2, 1);
    }
}

#if 0
Original Ghidra decompilation (0x55ec20):

void FUN_0055ec20(void)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((('\x03' < *(char *)(iVar1 + 0x502)) &&
      (0.0011111111 <
       *(float *)(iVar1 + 0x70) * *(float *)(iVar1 + 0x70) +
       *(float *)(iVar1 + 0x6c) * *(float *)(iVar1 + 0x6c) +
       *(float *)(iVar1 + 0x68) * *(float *)(iVar1 + 0x68))) && (DAT_0071c419 == '\0')) {
    FUN_00560590(2,0);
    FUN_00560590(2,1);
  }
  return;
}
#endif
