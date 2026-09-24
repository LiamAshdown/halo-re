// message_delta_parameters_protocol_find_registered  (Ghidra: message_delta_parameters_protocol_find_registered, already named)
// address 0x4ec110, size 136 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md summary; strides message_delta_parameters[] by 3
// (name, type, value) exactly like the rest of this family.
// register convention: no register-passed arguments beyond the __cdecl-shaped stack parameters
// Ghidra recognized.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t message_delta_parameter_count;               // 0x0071cfb0
extern message_delta_parameter message_delta_parameters[];  // 0x006b86c0

extern int32_t strcmp(const char *a, const char *b);

// Finds a registered dynamic parameter by exact name match. When found and out_value is
// non-NULL, writes the parameter's stored value pointer through it. Returns 1 if found, 0
// otherwise.
uint8_t message_delta_parameters_protocol_find_registered(char *name, void **out_value)
{
    int32_t i;

    for (i = 0; i < message_delta_parameter_count; i++) {
        if (strcmp(name, message_delta_parameters[i].name) == 0) {
            if (out_value != 0) {
                *out_value = message_delta_parameters[i].value;
            }
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ec110):

int message_delta_parameters_protocol_find_registered(byte *param_1,undefined4 *param_2)

{
  byte bVar1;
  uint3 uVar2;
  byte *pbVar3;
  int iVar4;
  undefined4 *puVar5;
  byte *pbVar6;
  int iVar7;
  bool bVar8;

  iVar7 = 0;
  uVar2 = (uint3)((uint)DAT_0071cfb0 >> 8);
  if (DAT_0071cfb0 < 1) {
    return (uint)uVar2 << 8;
  }
  puVar5 = &DAT_006b86c0;
  do {
    pbVar6 = (byte *)*puVar5;
    pbVar3 = param_1;
    do {
      bVar1 = *pbVar3;
      bVar8 = bVar1 < *pbVar6;
      if (bVar1 != *pbVar6) {
LAB_004ec154:
        iVar4 = (1 - (uint)bVar8) - (uint)(bVar8 != 0);
        goto LAB_004ec159;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar3[1];
      bVar8 = bVar1 < pbVar6[1];
      if (bVar1 != pbVar6[1]) goto LAB_004ec154;
      pbVar3 = pbVar3 + 2;
      pbVar6 = pbVar6 + 2;
    } while (bVar1 != 0);
    iVar4 = 0;
LAB_004ec159:
    if (iVar4 == 0) {
      if (param_2 == (undefined4 *)0x0) {
        return 1;
      }
      *param_2 = *(undefined4 *)(&DAT_006b86c8)[iVar7 * 3];
      return 1;
    }
    iVar7 = iVar7 + 1;
    puVar5 = puVar5 + 3;
    if (DAT_0071cfb0 <= iVar7) {
      return (uint)uVar2 << 8;
    }
  } while( true );
}
#endif
