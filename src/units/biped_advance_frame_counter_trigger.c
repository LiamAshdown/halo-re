// biped_advance_frame_counter_trigger  (Ghidra: biped_advance_frame_counter_trigger, renamed)
// address 0x55eb90, size 143 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump 0x55eb90..0x55ec1e)
// evidence: biped_data.frame_counter/frame_counter_limit/unknown_508 (types/units.h, see
//   biped_update_animation_frame_trigger); 0x006f187c is a globals structure whose +9 byte gates
//   several trigger paths (UNSURE, not named elsewhere in this batch).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_units.h"

extern data_array *object_data;  // 0x008603b0
extern uint8_t *cinematic_globals_ptr; // 0x006f187c, UNSURE
extern uint8_t unit_updates_suppressed; // 0x0071c419


// Advances the biped's animation frame counter (frame_counter); once it reaches the loaded
// threshold (frame_counter_limit), invalidates the cached comparison (unknown_508). Then, unless updates
// are globally suppressed, fires paired trigger events (ids 5) once the counter reaches exactly
// 2, or if the comparison is unresolved and the threshold is small. Reports state 0x15 or 0x16
// depending on whether the comparison flag reads 1.
void biped_advance_frame_counter_trigger(uint32_t object_index, char *state_out)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    int8_t frame_count = biped->frame_counter + 1;

    biped->frame_counter = frame_count;
    if (biped->frame_counter_limit <= frame_count) {
        biped->unknown_508 = -1;
    }

    if (cinematic_globals_ptr[9] == 0 && unit_updates_suppressed == 0 &&
        (frame_count == 2 || (biped->unknown_508 == -1 && biped->frame_counter_limit < 2))) {
        // unit_index rides in EBX (see that function's header); Ghidra bound only the two
        // stack arguments, which are its 2nd and 3rd parameters.
        unit_fire_animation_sound_trigger(object_index, 5, 0);
        unit_fire_animation_sound_trigger(object_index, 5, 1);
    }

    *state_out = (biped->unknown_508 == 1) + 0x15;
}

#if 0
Original Ghidra decompilation (0x55eb90):

void FUN_0055eb90(char *param_1)

{
  int iVar1;
  char cVar2;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  cVar2 = *(char *)(iVar1 + 0x4d0) + '\x01';
  *(char *)(iVar1 + 0x4d0) = cVar2;
  if (*(char *)(iVar1 + 0x4d1) <= cVar2) {
    *(undefined2 *)(iVar1 + 0x508) = 0xffff;
  }
  if (((*(char *)(DAT_006f187c + 9) == '\0') && (DAT_0071c419 == '\0')) &&
     ((cVar2 == '\x02' || ((*(short *)(iVar1 + 0x508) == -1 && (*(char *)(iVar1 + 0x4d1) < '\x02')))
      ))) {
    FUN_00560590(5,0);
    FUN_00560590(5,1);
  }
  *param_1 = (*(short *)(iVar1 + 0x508) == 1) + '\x15';
  return;
}
#endif
