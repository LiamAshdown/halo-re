// unit_dispatch_seat_exit_message  (Ghidra: FUN_0056c400)
// address 0x56c400, size 69 bytes, name confidence 0.25, rewrite confidence 0.85
// functions.md: "Dispatches to either a local seat-exit handler or a network handler depending
// on a flag read from the object pointed to by in_EAX."
// blam-cc: EAX -> message (the decode context)
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: the message decodes (0x4ec590, EAX context,
// ECX destination) into a 16-byte local record whose +0 is the unit's network key (0 = none) and +4 a byte that
// skips the exit animation when 1. The key maps through the pooled-node table (0x687130 +0x28) to the unit; unless
// the byte is 1, unit_try_start_seat_exit_animation(1, unit) runs first and the unit is detached only when it
// did not start (0x56c442 jne out). A reliable message (**message != 0) is rejected through 0x4ec670. The previous
// C took the key and the byte as caller parameters, which the dispatcher never passes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t *object_network_id_table; // 0x00687130, the pooled-node table (+0x28: key -> object index)

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message
extern uint8_t unit_try_start_seat_exit_animation(uint8_t force_flag, uint32_t unit_index); // 0x56c470, AL, EDI
extern void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event); // 0x56c640, UNSURE signature

typedef struct unit_seat_exit_message {
    int32_t unit_key;          // 0x00, network key of the unit (0: none)
    uint8_t skip_animation;    // 0x04, 1: detach at once
    uint8_t pad_05[11];
} unit_seat_exit_message;      // 0x10 bytes (sub esp,0x10)

void unit_dispatch_seat_exit_message(int32_t *message)
{
    unit_seat_exit_message decoded;
    int32_t unit_index;

    if (*(int32_t *)*message != 0) {
        message_delta_decode_compound_field_staged(message);
        return;
    }
    if (message_delta_decode_compound_field(message, &decoded) == 0 || decoded.unit_key == 0) {
        return;
    }
    unit_index = (*(int32_t **)(object_network_id_table + 0x28))[decoded.unit_key];
    if (unit_index == -1) {
        return;
    }
    if (decoded.skip_animation == 1 || unit_try_start_seat_exit_animation(1, (uint32_t)unit_index) == 0) {
        unit_detach_from_seat((uint32_t)unit_index, 0, 1, 0);
    }
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
