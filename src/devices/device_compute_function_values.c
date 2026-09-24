// device_compute_function_values  (Ghidra: FUN_0044ba10; renamed per
// out/phase4/devices_types_notes.md: "device_get_change_function_values" was phase 2's guess,
// but this fills object.function_in_values, the input side)
// address 0x44ba10, size 501 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: types/tags.h DeviceIn (the six selector values switched on, already documented in
//   that exact order in types/tags.h's own comment), Device (device_a_in..device_d_in 0x198,
//   inverse_power_transition_time 0x278, inverse_position_transition_time 0x288,
//   delay_time_ticks 0x28c); types/devices.h device_data (power/power_change,
//   position/position_change, position_group, device_flags/device_machine_flags bits 0-2),
//   device_group (flags at +0x02); types/objects.h object (type 0x0b4,
//   function_in_values 0x124, _object_type_device_machine).
// register convention: object index is already a plain, genuinely-stack parameter; Ghidra's own
//   `FUN_0044ba10(short *param_1)` reuses that one parameter register as a `DeviceIn_t *` partway
//   through the function (first as the object index, then repointed at tag->device_a_in), which
//   this rewrite splits into two clearly-named locals instead.
// case 5 (locked) mirrors devices_types_notes.md's own reading of the two device_group flag
//   bits: "the locked predicate everywhere in the module is *both* bits set" refuses a change;
//   here it forces the function's *output* to 1.0 (visually "locked") rather than refusing
//   anything, and is itself overridden back to 0.0 when the door is fully open or the machine
//   carries never_appears_locked.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "devices.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *device_groups; // 0x0087abf0

void device_compute_function_values(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
    Device *tag = (Device *)tag_instances[obj->definition_tag & 0xffff].data;
    DeviceIn_t *selector = &tag->device_a_in;
    float *out = obj->function_in_values;
    int i;

    for (i = 0; i < k_maximum_device_functions; i++, selector++, out++) {
        float value = 0.0f;

        switch (*selector) {
        case devicein_power:
            value = dev->power;
            break;
        case devicein_change_in_power:
            if (dev->power_change != 0.0f) {
                value = (dev->power_change < 0.0f ? -dev->power_change : dev->power_change) /
                    tag->inverse_power_transition_time;
            }
            break;
        case devicein_position:
            value = dev->position;
            break;
        case devicein_change_in_position:
            if (dev->position_change != 0.0f) {
                value = (dev->position_change < 0.0f ? -dev->position_change : dev->position_change) /
                    tag->inverse_position_transition_time;
            }
            break;
        case devicein_locked:
            value = (dev->power == 0.0f) ? 1.0f : 0.0f;
            if (obj->type == _object_type_device_machine && dev->position_group != -1) {
                device_group *group = &((device_group *)device_groups->data)[(uint16_t)dev->position_group];

                if ((dev->type_flags & 0x3) != 0) { // does_not_operate_automatically | one_sided
                    value = 1.0f;
                }
                if ((group->flags & (1u << _device_group_can_change_only_once_bit)) != 0 &&
                    (group->flags & (1u << _device_group_changed_bit)) != 0) {
                    value = 1.0f;
                }
                if (dev->position == 1.0f ||
                    (dev->type_flags & (1u << _device_machine_never_appears_locked_bit)) != 0) {
                    value = 0.0f;
                }
            }
            break;
        case devicein_delay:
            if (tag->delay_time_ticks > 0.0f && (float)dev->delay_ticks != tag->delay_time_ticks) {
                value = (float)dev->delay_ticks / tag->delay_time_ticks;
            }
            break;
        case devicein_none:
            // 0x44ba69 `test ax,ax; je 0x44bb9b` jumps straight to the shared advance step, so
            // this slot's *out keeps whatever it already held. `continue` runs the for-loop's
            // increment, which IS that advance step (edx += 2, ecx += 4, counter--).
            continue;
        default:
            // Not a no-op: the dispatch at 0x44ba7b is `movsx eax,ax; dec eax; cmp eax,5;
            // ja 0x44bb95`, and 0x44bb95 is `mov ecx,[esp+0x10]; fstp [ecx]` -- the 0.0 that
            // was pushed by `fld ds:0x672ac0` at 0x44ba72 gets STORED. So any selector outside
            // 1..6 (including a negative one, which the unsigned `ja` also sends here) writes
            // 0.0 rather than leaving the slot alone. Only devicein_none skips the write.
            value = 0.0f;
            break;
        }

        *out = value;
    }
}

#if 0
Original Ghidra decompilation (0x44ba10), from tools/pack.py 0x44ba10:

void FUN_0044ba10(short *param_1)

{
  float fVar1;
  ushort uVar2;
  uint *puVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  float *local_c;
  int local_8;

  iVar6 = ((uint)param_1 & 0xffff) * 0xc;
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_c = (float *)(puVar3 + 0x49);
  param_1 = (short *)(iVar4 + 0x198);
  local_8 = 4;
  do {
    if (*param_1 != 0) {
      fVar1 = 0.0;
      switch(*param_1) {
      case 1:
        fVar1 = (float)puVar3[0x7f];
        break;
      case 2:
        fVar1 = 0.0;
        if ((float)puVar3[0x80] != 0.0) {
          fVar1 = ABS((float)puVar3[0x80]) / *(float *)(iVar4 + 0x278);
        }
        break;
      case 3:
        fVar1 = (float)puVar3[0x82];
        break;
      case 4:
        fVar1 = 0.0;
        if ((float)puVar3[0x83] != 0.0) {
          fVar1 = ABS((float)puVar3[0x83]) / *(float *)(iVar4 + 0x288);
        }
        break;
      case 5:
        fVar1 = 0.0;
        if ((float)puVar3[0x7f] == 0.0) {
          fVar1 = 1.0;
        }
        if (((short)puVar3[0x2d] == 7) && ((short)puVar3[0x81] != -1)) {
          iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
          if ((*(uint *)(iVar5 + 0x214) & 3) != 0) {
            fVar1 = 1.0;
          }
          uVar2 = *(ushort *)
                   (*(int *)(DAT_0087abf0 + 0x34) + (uint)*(ushort *)(iVar5 + 0x204) * 8 + 2);
          if (((uVar2 & 1) != 0) && ((uVar2 & 2) != 0)) {
            fVar1 = 1.0;
          }
          if ((*(int *)(iVar5 + 0x208) == 0x3f800000) || ((*(uint *)(iVar5 + 0x214) & 4) != 0))
          goto LAB_0044bb8f;
        }
        break;
      case 6:
        if ((*(float *)(iVar4 + 0x28c) < 0.0 == (*(float *)(iVar4 + 0x28c) == 0.0)) &&
           ((float)(int)(short)puVar3[0x84] != *(float *)(iVar4 + 0x28c))) {
          fVar1 = (float)(int)(short)puVar3[0x84] / *(float *)(iVar4 + 0x28c);
        }
        else {
LAB_0044bb8f:
          fVar1 = 0.0;
        }
      }
      *local_c = fVar1;
    }
    param_1 = param_1 + 1;
    local_c = local_c + 1;
    local_8 = local_8 + -1;
    if (local_8 == 0) {
      return;
    }
  } while( true );
}
#endif
