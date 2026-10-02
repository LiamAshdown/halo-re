// unit_apply_control_block  (Ghidra: unit_apply_control_block)
// address 0x5639f0, size 289 bytes
// name confidence: 0.4 (phase2 candidate)   rewrite confidence: 0.9 (VERIFIED against objdump 0x5639f0..0x563b10)
// evidence: types/units.h unit_control_data (the whole 0x40-byte source record this function
//   unpacks -- this IS the function the header cites as proof of that struct's layout),
//   unit_data.saved_control/.unknown_4b8/.unknown_4bc (0x478/0x4b8/0x4bc), .throttle (0x278),
//   .primary_trigger (0x284), .aiming_speed (0x288), .desired_weapon_index (0x2f4),
//   .desired_grenade_index (0x31d), .desired_zoom_level (0x321), .control_flags (0x208),
//   .desired_looking_vector/.looking_vector fields (0x254/0x25c... see body),
//   .desired_aiming_vector (0x230), .desired_facing_vector (0x224), .seat_command (0x2a6).
//   global 0x00719720 the connection role (2 = server).
// register convention: unit index in EAX, the control record in EDX, a source id on the stack.
//   // blam-cc: in_EAX -> unit_index, in_EDX -> control, stack param_1 -> source_id

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern int16_t network_game_mode; // 0x00719720 (a WORD; 0x719722 is the screenshot counter), DAT_00719720 (1 = client, 2 = server)

void unit_apply_control_block(uint32_t unit_index, const unit_control_data *control, int32_t source_id) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (network_game_mode == 2) {
        unit->network_update_forced = (control->control_flags & 0x2800) != 0;
        unit->saved_control = *control;
    }

    unit->throttle = control->throttle;
    unit->primary_trigger = control->primary_trigger;
    unit->aiming_speed = control->aiming_speed;
    if (control->weapon_index != -1) {
        unit->desired_weapon_index = control->weapon_index;
    }
    if (control->grenade_index != -1) {
        unit->desired_grenade_index = (int8_t)control->grenade_index;
    }
    unit->desired_zoom_level = (int8_t)control->zoom_level;
    unit->control_flags = control->control_flags;
    unit->desired_looking_vector = control->looking_vector;
    unit->desired_aiming_vector = control->aiming_vector;
    unit->desired_facing_vector = control->facing_vector;
    unit->seat_command = control->animation_state;

    if (source_id != -1) {
        unit->control_update_id = source_id;
        unit->control_update_id_valid = 1;
    } else {
        unit->control_update_id_valid = 0;
    }
}

#if 0
Original Ghidra decompilation (0x5639f0):

void FUN_005639f0(int param_1)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  undefined4 *in_EDX;
  undefined4 *puVar3;
  undefined4 *puVar4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (DAT_00719720 == 2) {
    if ((*(ushort *)((int)in_EDX + 2) & 0x2800) == 0) {
      *(undefined1 *)(iVar1 + 0x474) = 0;
    }
    else {
      *(undefined1 *)(iVar1 + 0x474) = 1;
    }
    puVar3 = in_EDX;
    puVar4 = (undefined4 *)(iVar1 + 0x478);
    for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
  }
  *(undefined4 *)(iVar1 + 0x278) = in_EDX[3];
  *(undefined4 *)(iVar1 + 0x27c) = in_EDX[4];
  *(undefined4 *)(iVar1 + 0x280) = in_EDX[5];
  *(undefined4 *)(iVar1 + 0x284) = in_EDX[6];
  *(undefined1 *)(iVar1 + 0x288) = *(undefined1 *)((int)in_EDX + 1);
  if (*(short *)(in_EDX + 1) != -1) {
    *(short *)(iVar1 + 0x2f4) = *(short *)(in_EDX + 1);
  }
  if (*(short *)((int)in_EDX + 6) != -1) {
    *(undefined1 *)(iVar1 + 0x31d) = *(undefined1 *)((int)in_EDX + 6);
  }
  *(undefined1 *)(iVar1 + 0x321) = *(undefined1 *)(in_EDX + 2);
  *(uint *)(iVar1 + 0x208) = (uint)*(ushort *)((int)in_EDX + 2);
  *(undefined4 *)(iVar1 + 0x254) = in_EDX[0xd];
  *(undefined4 *)(iVar1 + 600) = in_EDX[0xe];
  *(undefined4 *)(iVar1 + 0x25c) = in_EDX[0xf];
  *(undefined4 *)(iVar1 + 0x230) = in_EDX[10];
  *(undefined4 *)(iVar1 + 0x234) = in_EDX[0xb];
  *(undefined4 *)(iVar1 + 0x238) = in_EDX[0xc];
  *(undefined4 *)(iVar1 + 0x224) = in_EDX[7];
  *(undefined4 *)(iVar1 + 0x228) = in_EDX[8];
  *(undefined4 *)(iVar1 + 0x22c) = in_EDX[9];
  *(undefined1 *)(iVar1 + 0x2a6) = *(undefined1 *)in_EDX;
  if (param_1 != -1) {
    *(int *)(iVar1 + 0x4bc) = param_1;
    *(undefined1 *)(iVar1 + 0x4b8) = 1;
    return;
  }
  *(undefined1 *)(iVar1 + 0x4b8) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
