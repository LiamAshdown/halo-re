// device_change_power_state  (Ghidra: device_change_power_state, already named)
// address 0x44ae30, size 234 bytes
// name confidence: 0.5   rewrite confidence: 0.8 (disassembly-verified end to end)
// evidence: out/phase2/results/devices_00.json (0x44ae30 entry); types/tags.h DeviceControl.type
//   (DeviceType: toggle_switch/on_button/off_button/call_button, 0x290) and .call_value (0x294);
//   types/devices.h device_group (0x0087abf0), device_data.position_group (0x204) -- confirmed
//   independently by src/devices/device_machine_update.c's FUN_0044c0c0 cross-check (out of
//   this batch) using the identical offset for the same field; out/phase4/devices_types_notes.md
//   confirms tag+0x290 is exactly this switch's DeviceType.
// register convention: a fallback float value in ECX (reachable only for a malformed
// DeviceControl.type, see the note below), object id in EBX.
//   // blam-cc: ECX = fallback_value, EBX = object_id
// The field read at object+0x204 is device_data.position_group, not power_group (0x1f8) --
// `mov si,WORD PTR [eax+0x204]` at 0x44ae51, unambiguous. That looked like it contradicted the
// function's name, but the two fit together once device_group_set_value's own behaviour is taken
// into account, and the phase-4 review resolved it:
//   - this function writes the CONTROL's position group, group index N;
//   - device_group_set_value (0x44bd70) then walks every device object and fires
//     Device.repowered / Device.depowered on each one whose device_data.POWER group is also N
//     (`cmp WORD PTR [eax+0x1f8],si` at 0x44be41);
//   - device_update_change_values (0x44b720) copies group[power_group].value into
//     device_data.power, so that machine's power now follows group N.
// So a level is wired with control.position_group == machine.power_group, one shared group
// drives both the switch's own visual throw and the target machine's power, and
// "change power state" is an accurate name for what pressing the switch does. This also
// explains device_group_set_value's repowered/depowered choice, which otherwise looks
// mismatched against a position group. HYPOTHESIS about the scenario wiring, not provable from
// this module alone -- every step of the mechanism is in the code, but that a designer always
// sets the two indices equal is not. Nothing here depends on it.
// The `float in_ECX` fallback is REACHABLE, not dead: the dispatch at 0x44ae70..0x44ae7c is
// `movsx eax,WORD PTR [edi+0x290]; cmp eax,3; ja 0x44aebe; jmp [eax*4+0x44af1c]`, and 0x44aebe
// is the `mov ecx,[esp+0x8]; push ecx; call 0x44bd70` that applies the value. [esp+0x8] is the
// `push ecx` slot from the function's own prologue at 0x44ae30 (three pushes deep by then), so a
// DeviceControl.type outside 0..3 -- including a negative one, which the unsigned `ja` also
// sends there -- forwards the caller's ECX float straight to device_group_set_value untouched.
// An earlier revision of this file called the default unreachable; it is only unreachable for
// well-formed tag data.
// Resolved against disassembly (objdump -d -M intel bin/halo.exe, 0x44ae30..0x44af19) in the
// phase-4 review: the three device_play_state_change_effect tail-calls load ECX from
//   0x44aee0  mov ecx,[edi+0x2f4]   value > 0.5   -> DeviceControl.on.tag_id
//   0x44aef2  mov ecx,[edi+0x304]   value <= 0.5  -> DeviceControl.off.tag_id
//   0x44af04  mov ecx,[edi+0x314]   rejected      -> DeviceControl.deny.tag_id
// and DeviceControl's layout in types/tags.h puts exactly those three there: Device is 0x290,
// then type 0x290 / triggers_when 0x292 / call_value 0x294 / 80 bytes of padding, so
// on = 0x2e8, off = 0x2f8, deny = 0x308, each a 0x10-byte TagDependency whose tag_id is at
// +0x0c -> 0x2f4, 0x304, 0x314. Three fields, three offsets, exact match. The structural guess
// the first pass made was right; it is no longer an UNSURE. All three sites are `jmp 0x44c1a0`
// tail-calls with `mov eax,ebx` first, which also confirms the
// device_play_state_change_effect(EAX = object_index, ECX = tag_id) convention.
// The 0.5 threshold is `fcomp ds:0x672abc` + `test ah,0x41` at both 0x44ae93 (the toggle's read
// of the group value) and 0x44aecf (the applied value), i.e. "<= 0.5" in both places.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "devices.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *device_groups;   // 0x0087abf0

extern uint8_t device_group_set_value(uint16_t group_index, float value); // 0x44bd70, this batch
extern void device_play_state_change_effect(uint32_t object_index, TagID tag_id); // 0x44c1a0 (out of range)
    // blam-cc: EAX = object_index, ECX = tag_id (signature per
    // src/devices/device_group_set_value.c's disassembly-verified header)

// Computes the target value for a device_control's own (position) group from its
// DeviceControl.type -- an auto-threshold toggle, a forced on/off, or a custom call_value --
// applies it through device_group_set_value, and plays whichever state-change effect/sound
// corresponds to the outcome (rejected, settled off, or settled on).
void device_change_power_state(float fallback_value, uint32_t object_id) // blam-cc: see header
{
    object *obj = ((object_header *)object_data->data)[object_id & 0xffff].data;
    device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
    DeviceControl *tag = (DeviceControl *)tag_instances[obj->definition_tag & 0xffff].data;
    int16_t group_index = dev->position_group; // the control's own group; see header
    float target;
    uint8_t changed;

    if (group_index == (int16_t)0xffff) {
        return;
    }

    target = fallback_value; // the reachable `ja 0x44aebe` default; see header
    switch (tag->type) {
    case devicetype_toggle_switch:
        // 0x44ae8c `movzx eax,si`: the group index is scaled UNSIGNED, like everywhere else
        // in this module.
        if (((device_group *)device_groups->data)[(uint16_t)group_index].value <= 0.5f) {
            target = 1.0f;
            break;
        }
        /* fall through: current value is already above the threshold, so this switch turns off */
    case devicetype_off_button:
        target = 0.0f;
        break;
    case devicetype_on_button:
        target = 1.0f;
        break;
    case devicetype_call_button:
        target = tag->call_value;
        break;
    }

    changed = device_group_set_value((uint16_t)group_index, target);
    if (!changed) {
        device_play_state_change_effect(object_id, tag->deny.tag_id); // ECX <- tag+0x314
        return;
    }
    if (target <= 0.5f) {
        device_play_state_change_effect(object_id, tag->off.tag_id); // ECX <- tag+0x304
        return;
    }
    device_play_state_change_effect(object_id, tag->on.tag_id); // ECX <- tag+0x2f4
}

#if 0
Original Ghidra decompilation (0x44ae30):

void device_change_power_state(void)

{
  ushort uVar1;
  uint *puVar2;
  int iVar3;
  char cVar4;
  float in_ECX;
  uint unaff_EBX;
  float local_4;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc);
  uVar1 = (ushort)puVar2[0x81];
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (uVar1 == 0xffff) {
    return;
  }
  local_4 = in_ECX;
  switch(*(undefined2 *)(iVar3 + 0x290)) {
  case 0:
    if (*(float *)(*(int *)(DAT_0087abf0 + 0x34) + 4 + (uint)uVar1 * 8) <= 0.5)
    goto switchD_0044ae7c_caseD_1;
  case 2:
    local_4 = 0.0;
    break;
  case 1:
switchD_0044ae7c_caseD_1:
    local_4 = 1.0;
    break;
  case 3:
    local_4 = *(float *)(iVar3 + 0x294);
  }
  cVar4 = device_group_set_value(local_4);
  if (cVar4 == '\0') {
    device_play_state_change_effect();
    return;
  }
  if (local_4 <= 0.5) {
    device_play_state_change_effect();
    return;
  }
  device_play_state_change_effect();
  return;
}
#endif
