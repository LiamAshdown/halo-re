// controls_binding_clear  (Ghidra: FUN_004b4e20, named in phase 4)
// address 0x4b4e20, size 259 bytes
// name confidence: 0.45   rewrite confidence: 0.95
// evidence: rewritten from objdump 0x4b4e20..0x4b4f22 in the phase-4 review (an earlier rewrite
// did not model the register arguments). EAX is the action index into the
// 0x18 byte action table at 0x00692fe8, the stack argument the device. Keyboard (0) and mouse
// (1) bindings of an action whose unbindable column bit is set are left alone (returns 0). The
// current binding is found with controls_enumerate_next_assignable_action (EAX device, ECX
// record, EDI action name, no retry); none returns 0. For a gamepad (device 2 and up) of an
// action with column bit 2, the two gamepad control words of the working profile (+0x32a and
// +0x32c per gamepad, stride 4) that hold the control become -1. Otherwise the binding is
// dropped from the current binding table at 0x007127d4 (3 dwords per action, found with
// input_action_name_to_index, EBX name) when it matches, and control_profile_clear_binding
// (ESI record) clears it. Returns 1 in both cases.
// register convention: EAX action index; one stack argument.
//   // blam-cc: action_index -> EAX

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t controls_action_table[][0x18];   // 0x00692fe8
extern int32_t selected_saved_item;             // 0x00714e7c
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80
extern int32_t controls_current_binding_table[][3]; // 0x007127d4

extern uint8_t controls_enumerate_next_assignable_action(int32_t device, int16_t *record, const char *action_name,
                                                          uint8_t accept_reserved_on_retry); // 0x4b43e0, blam-cc: EAX device, ECX record, EDI action_name
extern int16_t input_action_name_to_index(const char *action_name); // 0x48fe60, blam-cc: EBX action_name
extern void control_profile_clear_binding(const int16_t *record); // 0x53ad00, blam-cc: ESI record

// blam-cc: action_index -> EAX
uint8_t controls_binding_clear(int32_t action_index, int32_t device)
{
    uint8_t *entry = controls_action_table[action_index];
    int16_t record[6];

    if (device == 0 && (entry[0x14] & 1) != 0) {
        return 0;
    }
    if (device == 1 && (entry[0x14] & 2) != 0) {
        return 0;
    }
    if (controls_enumerate_next_assignable_action(device, record, (const char *)entry, 0) == 0) {
        return 0;
    }
    if (device >= 2 && (entry[0x14] & 4) != 0) {
        uint8_t *profile = (selected_saved_item & 0xf) != 0 ? (uint8_t *)0 : saved_item_working_copy;
        int16_t gamepad = record[1];
        int16_t control = record[3];

        if (*(int16_t *)(profile + 0x32c + gamepad * 4) == control) {
            *(int16_t *)(profile + 0x32c + gamepad * 4) = -1;
        }
        if (*(int16_t *)(profile + 0x32a + gamepad * 4) == control) {
            *(int16_t *)(profile + 0x32a + gamepad * 4) = -1;
        }
        return 1;
    }
    {
        int32_t *current = controls_current_binding_table[input_action_name_to_index((const char *)entry)];
        if (memcmp(current, record, 12) == 0) {
            current[0] = 0;
            current[1] = 0;
            current[2] = 0;
        }
    }
    control_profile_clear_binding(record);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4b4e20):

undefined4 FUN_004b4e20(int param_1)

{
  byte bVar1;
  char cVar2;
  short sVar3;
  int in_EAX;
  uint uVar4;
  int iVar5;
  int iVar6;
  int *piVar7;
  int *piVar8;
  bool bVar9;
  undefined4 local_c;
  short local_6;

  if (param_1 == 0) {
    bVar1 = (&DAT_00692ffc)[in_EAX * 0x18] & 1;
  }
  else {
    if (param_1 != 1) goto LAB_004b4e56;
    bVar1 = (&DAT_00692ffc)[in_EAX * 0x18] & 2;
  }
  if (bVar1 != 0) {
    return 0;
  }
LAB_004b4e56:
  cVar2 = FUN_004b43e0(0);
  if (cVar2 == '\0') {
    return 0;
  }
  if ((param_1 < 2) || (((&DAT_00692ffc)[in_EAX * 0x18] & 4) == 0)) {
    sVar3 = input_action_name_to_index();
    iVar5 = sVar3 * 0xc;
    iVar6 = 3;
    bVar9 = true;
    piVar7 = (int *)(&DAT_007127d4 + iVar5);
    piVar8 = &local_c;
    do {
      if (iVar6 == 0) break;
      iVar6 = iVar6 + -1;
      bVar9 = *piVar7 == *piVar8;
      piVar7 = piVar7 + 1;
      piVar8 = piVar8 + 1;
    } while (bVar9);
    if (bVar9) {
      *(int *)(&DAT_007127d4 + iVar5) = 0;
      *(undefined4 *)(&DAT_007127d8 + iVar5) = 0;
      *(undefined4 *)(&DAT_007127dc + iVar5) = 0;
    }
    control_profile_clear_binding();
  }
  else {
    iVar5 = (int)local_c._2_2_;
    uVar4 = ~-(uint)((DAT_00714e7c & 0xf) != 0) & 0x714e80;
    if (*(short *)(uVar4 + 0x32c + iVar5 * 4) == local_6) {
      *(undefined2 *)(uVar4 + 0x32c + iVar5 * 4) = 0xffff;
    }
    if (*(short *)(uVar4 + 0x32a + iVar5 * 4) == local_6) {
      *(undefined2 *)(uVar4 + 0x32a + iVar5 * 4) = 0xffff;
      return 1;
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
