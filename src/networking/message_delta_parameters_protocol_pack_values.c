// message_delta_parameters_protocol_pack_values  (Ghidra: message_delta_parameters_protocol_pack_values, already named)
// address 0x4ec1a0, size 138 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md summary; called from
// message_delta_parameters_protocol_send_update right before message_delta_encode_message, so
// the flattened array is the wire payload.
// register convention: destination array pinned in EBX (unaff_EBX); the 4-at-a-time/remainder
// unrolling Ghidra shows is collapsed here into one equivalent loop.
// blam-cc: EBX -> out_values

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t message_delta_parameter_count;               // 0x0071cfb0
extern message_delta_parameter message_delta_parameters[];  // 0x006b86c0

// blam-cc: EBX -> out_values
// Flattens every registered dynamic parameter's current 32-bit value into a contiguous array for
// network transmission.
void message_delta_parameters_protocol_pack_values(int32_t *out_values)
{
    int32_t i;

    for (i = 0; i < message_delta_parameter_count; i++) {
        out_values[i] = *(int32_t *)message_delta_parameters[i].value;
    }
}

#if 0
Original Ghidra decompilation (0x4ec1a0):

void message_delta_parameters_protocol_pack_values(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  int iVar4;
  int unaff_EBX;
  int iVar5;

  iVar1 = DAT_0071cfb0;
  iVar5 = 0;
  if (3 < DAT_0071cfb0) {
    iVar4 = (DAT_0071cfb0 - 4U >> 2) + 1;
    puVar2 = &DAT_006b86c8;
    puVar3 = (undefined4 *)(unaff_EBX + 8);
    iVar5 = iVar4 * 4;
    do {
      puVar3[-2] = *(undefined4 *)*puVar2;
      puVar3[-1] = *(undefined4 *)puVar2[3];
      *puVar3 = *(undefined4 *)puVar2[6];
      puVar3[1] = *(undefined4 *)puVar2[9];
      puVar3 = puVar3 + 4;
      puVar2 = puVar2 + 0xc;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  if (iVar5 < iVar1) {
    puVar3 = &DAT_006b86c8 + iVar5 * 3;
    do {
      *(undefined4 *)(unaff_EBX + iVar5 * 4) = *(undefined4 *)*puVar3;
      iVar5 = iVar5 + 1;
      puVar3 = puVar3 + 3;
    } while (iVar5 < iVar1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
