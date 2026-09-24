// message_delta_parameters_protocol_receive_update  (Ghidra: message_delta_parameters_protocol_receive_update, already named)
// address 0x4ec000, size 78 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary; the destination block this decodes into
// is 260 bytes (a 1-byte sequence number immediately followed by up to 64 packed int32 values),
// matching message_delta_parameters_protocol_send_update's own 260-byte local_104 buffer
// (sequence byte at [0], then message_delta_parameters_protocol_pack_values's flattened array).
// register convention: decode context as the recognized parameter (in_EAX).
// blam-cc: EAX -> context

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t message_delta_parameters_enabled;            // 0x0071cfa8
extern int32_t message_delta_parameters_protocol_sequence;  // 0x0071cfac

extern uint8_t message_delta_decode_compound_field(void **context, void *destination); // 0x4ec590, this module
extern void message_delta_decode_compound_field_staged(void **context); // 0x4ec670, this module
extern void message_delta_parameters_protocol_format_received_values(int32_t *values); // 0x4ec230, this module

// blam-cc: EAX -> context
// Handles an incoming dynamic-parameters protocol message: for a baseline (non-incremental)
// message, decodes the sequence byte plus the packed value array and, on success, formats the
// values back into text and latches the new sequence number; for an incremental message, drains
// the field through the staged/scratch decoder without applying it.
void message_delta_parameters_protocol_receive_update(void **context)
{
    message_delta_decode_state *state = (message_delta_decode_state *)context[0];

    if (message_delta_parameters_enabled == 1) {
        if (state->incremental == 0) {
            struct {
                uint8_t sequence;
                uint8_t pad[3];
                int32_t values[64];
            } destination;

            if (message_delta_decode_compound_field(context, &destination) == 1) {
                message_delta_parameters_protocol_format_received_values(destination.values);
                message_delta_parameters_protocol_sequence = destination.sequence;
            }
        } else {
            message_delta_decode_compound_field_staged(context);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ec000):

void message_delta_parameters_protocol_receive_update(void)

{
  char cVar1;
  undefined4 *in_EAX;
  byte local_104;
  undefined1 local_100 [256];

  if (DAT_0071cfa8 == '\x01') {
    if (*(int *)*in_EAX == 0) {
      cVar1 = FUN_004ec590();
      if (cVar1 == '\x01') {
        FUN_004ec230(local_100);
        DAT_0071cfac = (uint)local_104;
        return;
      }
    }
    else {
      FUN_004ec670();
    }
  }
  return;
}
#endif
