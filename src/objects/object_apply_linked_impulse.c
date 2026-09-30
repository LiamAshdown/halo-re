// object_apply_linked_impulse
// address 0x4efc80, size 112 bytes
// name confidence: 0.3 (still FUN_004efc80 in Ghidra; named from out/phase4/objects_functions.md's
// summary, "Validates a linked object then scales a direction vector by a magnitude and forwards
// it as a physics impulse")
// rewrite confidence: 0.25
// evidence: item_accelerate already named elsewhere; everything else in this function is UNSURE.
// UNSURE: same pattern as object_apply_shield_charge_and_notify.c — `network_id`, and the vector
// components `direction_i/impulse_scale/direction_j/direction_k` are read with no visible prior assignment,
// meaning message_delta_decode_compound_field writes them through implicit pointers this decompile lost.
// register convention: a pointer-to-pointer parameter in EAX (in_EAX).
// blam-cc: EAX=message

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_objects.h"

extern network_id_table *object_network_id_table; // 0x00687130

extern int8_t message_delta_decode_compound_field(void *globals, void *out_value); // UNSURE: out of range, 0x4ec590
extern uint8_t message_delta_decode_compound_field_staged(void **context); // UNSURE: zero visible args; out of range, 0x4ec670
extern void item_accelerate(real_vector3d *impulse, int32_t param_2); // 0x4bd080

void object_apply_linked_impulse(void **message)
{
    int32_t network_id; // UNSURE: apparently written by message_delta_decode_compound_field through an implicit pointer
    float impulse_scale, direction_i, direction_j, direction_k; // UNSURE: same

    if (*(int32_t *)*message != 0) {
        message_delta_decode_compound_field_staged(0); // UNSURE: the EAX operand is not visible here
        return;
    }

    if (message_delta_decode_compound_field(message, &network_id) != 0 && network_id != 0 &&
        ((int32_t *)object_network_id_table->handles)[network_id] != -1) { // UNSURE: array base at +0x28
        real_vector3d impulse;

        impulse.i = direction_i * impulse_scale;
        impulse.j = direction_j * impulse_scale;
        impulse.k = direction_k * impulse_scale;
        item_accelerate(&impulse, 0);
    }
}

#if 0
Original Ghidra decompilation (0x4efc80):

void FUN_004efc80(void)

{
  char cVar1;
  undefined4 *in_EAX;
  float local_20;
  float local_1c;
  float local_18;
  int local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if (((cVar1 != '\0') && (local_14 != 0)) &&
       (*(int *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_14 * 4) != -1)) {
      local_20 = local_c * local_10;
      local_1c = local_8 * local_10;
      local_18 = local_4 * local_10;
      item_accelerate(&local_20,0);
      return;
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
