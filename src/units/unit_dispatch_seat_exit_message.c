// unit_dispatch_seat_exit_message  (Ghidra: FUN_0056c400)
// address 0x56c400, size 69 bytes, name confidence 0.25, rewrite confidence 0.2
// functions.md: "Dispatches to either a local seat-exit handler or a network handler depending
// on a flag read from the object pointed to by in_EAX."
// blam-cc: in_EAX -> message, plus two ambient locals (local_c, local_8) this decompilation
//   never shows being written -- modelled as explicit parameters since unit_try_start_seat_exit_animation needs a
//   validity flag and unit_detach_from_seat needs a player index.
// UNSURE: message_delta_decode_compound_field's and message_delta_decode_compound_field_staged's real signatures, and exactly what local_c/local_8
//   represent, are not recovered.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern uint8_t *network_message_table; // 0x00687130, UNSURE (PTR_DAT_00687130)

extern uint8_t message_delta_decode_compound_field(void);                                     // 0x4ec590, UNSURE signature
extern void message_delta_decode_compound_field_staged(void);                                        // 0x4ec670, UNSURE signature
extern uint8_t unit_try_start_seat_exit_animation(void);                                     // 0x56c470, UNSURE signature  // real signature (unit_try_start_seat_exit_animation.c): uint8_t unit_try_start_seat_exit_animation(uint8_t force_flag, uint32_t unit_index); Ghidra recovered 0 of 2 args at this call site
extern void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event); // 0x56c640, UNSURE signature

void unit_dispatch_seat_exit_message(int32_t *message, int32_t player_slot, uint8_t already_handled)
    // blam-cc: in_EAX -> message, ambient -> player_slot, already_handled
{
    if (*(int32_t *)*message == 0) {
        uint8_t ok = message_delta_decode_compound_field();
        if ((ok != 0) && (player_slot != 0)) {
            int32_t unit_index = (*(int32_t **)(network_message_table + 0x28))[player_slot];
            if (unit_index != -1) {
                if ((already_handled == 1) || (unit_try_start_seat_exit_animation() == 0)) {
                    unit_detach_from_seat((uint32_t)unit_index, 0, 1, 0);
                }
                return;
            }
        }
    } else {
        message_delta_decode_compound_field_staged();
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56c400):

void FUN_0056c400(void)

{
  int iVar1;
  char cVar2;
  undefined4 *in_EAX;
  int local_c;
  char local_8;

  if (*(int *)*in_EAX == 0) {
    cVar2 = FUN_004ec590();
    if (((cVar2 != '\0') && (local_c != 0)) &&
       (iVar1 = *(int *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_c * 4), iVar1 != -1)) {
      if ((local_8 != '\x01') && (cVar2 = FUN_0056c470(), cVar2 != '\0')) {
        return;
      }
      FUN_0056c640(iVar1,0,1,0);
      return;
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
