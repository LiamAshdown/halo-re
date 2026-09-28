// actor_play_first_valid_vocalization  (Ghidra: FUN_0040e260; really: send an actor to the first seat of a vehicle
//   it can board -- the name is historical, nothing is vocalized)
// address 0x40e260, size 275 bytes
// name confidence: 0.1   rewrite confidence: 0.9
// REWRITTEN from objdump 0x40e260..0x40e372 (the draft dropped the EAX list and ECX vehicle). EAX: a seat list or
//   0, ECX: the vehicle; stack (actor, seat name, seat flags, count). Without a list the vehicle's seats matching
//   the name and flags are collected (0x56a310, at most 16). The first listed seat (-1 = taken) the actor's unit
//   may use (0x565150) for which a mode 9 order can be built (0x408a30) puts the actor in mode 9 (0x40d8d0);
//   the seat is struck from the list and 1 returned.
// blam-cc: EAX -> seat_list, ECX -> vehicle_index, stack -> (actor_index, seat_name, seat_flags, count)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern uint8_t unit_seat_index_is_valid(uint32_t other_object_index, uint32_t unit_index, int16_t seat_index); // 0x565150, EAX, ECX, DX
extern int16_t unit_find_seats_matching_name_and_flags(uint32_t unit_index, char *name_filter, uint16_t flag_selector,
                                                       int16_t *out_indices, int16_t max_indices); // 0x56a310
extern uint8_t actor_build_order_investigate_encounter_point(uint32_t vehicle_index, uint32_t actor_index, int16_t seat_index,
                                                             uint8_t *order); // 0x408a30, EBX, stack
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0

uint8_t actor_play_first_valid_vocalization(int16_t *seat_list, datum_index vehicle_index, datum_index actor_index,
                                            char *seat_name, int16_t seat_flags, int16_t count)
{
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    int16_t local_list[16];
    uint8_t order[k_actor_mode_data_size]; // 0x84: actor_set_mode copies the mode's data_size bytes
    int16_t i;

    if (seat_list == 0) {
        seat_list = local_list;
        count = unit_find_seats_matching_name_and_flags(vehicle_index, seat_name, (uint16_t)seat_flags, local_list, 16);
    }
    for (i = 0; i < count; i++) {
        int16_t seat = seat_list[i];

        if (seat == -1 || !unit_seat_index_is_valid(((actor *)act)->unit_index, vehicle_index, seat)) {
            continue;
        }
        if (actor_build_order_investigate_encounter_point(vehicle_index, actor_index, seat, order)) {
            actor_set_mode(actor_index, 9, order);
            seat_list[i] = -1;
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x40e260):

undefined1 FUN_0040e260(undefined4 param_1,undefined4 param_2,undefined4 param_3,short param_4)

{
  short sVar1;
  char cVar2;
  undefined1 *in_EAX;
  short sVar3;
  undefined1 *local_ac;
  short local_a8;
  undefined1 local_a4 [32];
  undefined1 local_84 [132];

  if (in_EAX == (undefined1 *)0x0) {
    local_ac = local_a4;
    local_a8 = FUN_0056a310();
  }
  else {
    local_a8 = param_4;
    local_ac = in_EAX;
  }
  sVar3 = 0;
  if (local_a8 < 1) {
    return 0;
  }
  while (((sVar1 = *(short *)(local_ac + sVar3 * 2), sVar1 == -1 ||
          (cVar2 = FUN_00565150(), cVar2 == '\0')) ||
         (cVar2 = actor_build_order_investigate_encounter_point(param_1,sVar1,local_84),
         cVar2 == '\0'))) {
    sVar3 = sVar3 + 1;
    if (local_a8 <= sVar3) {
      return 0;
    }
  }
  actor_set_mode(param_1,9,local_84);
  *(undefined2 *)(local_ac + sVar3 * 2) = 0xffff;
  return 1;
}
#endif
