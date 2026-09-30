// device_update_change_values  (Ghidra: device_update_change_values, already named;
// functions.md: "Advances the device's cached power and position values toward their target
// group values at a rate governed by the tag definition, returning whether the position is
// still settling and triggering state-change effects")
// address 0x44b720, size 749 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: types/devices.h device_data (flags, power/power_change/power_group,
//   position/position_change/position_group, delay_ticks), device_group (value at +0x04);
//   types/tags.h Device (inverse_power_acceleration_time 0x274, inverse_power_transition_time
//   0x278, inverse_depowered_position_acceleration_time 0x27c,
//   inverse_depowered_position_transition_time 0x280, inverse_position_acceleration_time 0x284,
//   inverse_position_transition_time 0x288, delay_time_ticks 0x28c, device_flags
//   position_loops bit 0, open.tag_id 0x1ac, close.tag_id 0x1bc, opened.tag_id 0x1cc,
//   closed.tag_id 0x1dc, delay_effect.tag_id 0x218); callees device_play_state_change_effect
//   (0x44c1a0, this batch), real_seek_toward_clamped (established, src/math).
// register convention: object index is already a plain, genuinely-stack parameter
//   (`device_update_change_values(uint param_1)`); no unresolved registers appear.
// Resolved against disassembly (objdump -d -M intel bin/halo.exe, 0x44b720..0x44ba0c), because
// Ghidra elides every argument to real_seek_toward_clamped (wrap flag, the two float* state
// pointers) and to all five device_play_state_change_effect calls (which tag_id field feeds
// ECX). The two calls' stack argument order confirms real_seek_toward_clamped(wrap, velocity,
// value, target, accel, max_speed, range_min, range_max) exactly as already declared in
// src/math/real_seek_toward_clamped.c. The five effect call sites resolve to:
//   - power settle: no effect (power has no open/close/opened/closed pair of its own here).
//   - position, still accelerating (cVar14==0) and direction reversed mid-flight
//     (new_position_change != 0 and old*new <= 0): "open" (0x1ac) if new > old, else
//     "close" (0x1bc) (0x44b9d4/0x44b9dc).
//   - position, settled (cVar14!=0): "opened" (0x1cc) if the OLD position_change was > 0.0,
//     else "closed" (0x1dc) (0x44b979/0x44b98e).
//   - position, delay counter just reached 1 (sVar1==1): delay_effect (0x218) (0x44b8ff).
// All five effect sites and the delay gate re-confirmed by a second disassembly pass
// (phase-4 review). The `fcomp`/`test ah,0x41`/`jne` at 0x44b9c7-0x44b9d2 takes the branch when
// new_position_change is less than OR equal to old (C0|C3), so new > old selects tag+0x1ac and
// new <= old selects tag+0x1bc. Device's own layout fixes those two: Object is 0x17c, then
// device_flags 0x17c, six floats 0x180..0x197, device_a_in..d_in 0x198..0x19f, then
// open 0x1a0 / close 0x1b0 / opened 0x1c0 / closed 0x1d0 / depowered 0x1e0 / repowered 0x1f0,
// each a 0x10-byte TagDependency whose tag_id sits at +0x0c -- so open.tag_id IS 0x1ac,
// close.tag_id 0x1bc, opened.tag_id 0x1cc, closed.tag_id 0x1dc, and delay_effect.tag_id 0x218.
// Every offset the disassembly loads into ECX lands exactly on one of those. No UNSURE remains.
// The settled pair keys on the byte flag at [esp+0x13], seeded to 1 at 0x44b864 and cleared at
// 0x44b8aa when position_change <= 0, i.e. it is (old_position_change > 0): opened when set,
// closed when clear.
// The delay gate's middle term at 0x44b8d4 is `fld DWORD PTR [edi]`, and EDI is loaded with
// `lea edi,[ebp+0x208]` at 0x44b816 -- it is device_data.POSITION, not position_change. An
// earlier revision of this file had position_change there; corrected in the phase-4 review.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "devices.h"
#include "fn_math.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *device_groups; // 0x0087abf0

extern void device_play_state_change_effect(uint32_t object_index, TagID tag_id); // 0x44c1a0, this batch
    // blam-cc: EAX -> object_index, ECX -> tag_id


uint8_t device_update_change_values(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
    Device *tag = (Device *)tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t still_settling = 0;

    if (dev->power_group != -1) {
        device_group *group = &((device_group *)device_groups->data)[(uint16_t)dev->power_group];
        if (group->value != dev->power || dev->power_change != 0.0f) {
            float old_power = dev->power;
            uint8_t settled = real_seek_toward_clamped(0, &dev->power_change, &dev->power,
                group->value, tag->inverse_power_acceleration_time,
                tag->inverse_power_transition_time, 0.0f, 1.0f);
            still_settling = !settled;
            if (old_power != dev->power) {
                dev->flags |= (1u << _device_position_changed_bit);
            }
        }
    }

    if (dev->position_group != -1) {
        device_group *group = &((device_group *)device_groups->data)[(uint16_t)dev->position_group];

        if (group->value == dev->position && dev->position_change == 0.0f) {
            dev->delay_ticks = 0;
        } else {
            float accel = tag->inverse_position_acceleration_time * dev->power +
                (1.0f - dev->power) * tag->inverse_depowered_position_acceleration_time;
            float max_speed = tag->inverse_position_transition_time * dev->power +
                (1.0f - dev->power) * tag->inverse_depowered_position_transition_time;
            float old_position_change = dev->position_change;

            if (tag->delay_time_ticks <= (float)dev->delay_ticks ||
                dev->position != 0.0f ||
                group->value < dev->position) {
                float old_position = dev->position;
                uint8_t settled;

                if (max_speed < (dev->position_change < 0.0f ? -dev->position_change : dev->position_change)) {
                    dev->position_change = (old_position_change <= 0.0f) ? -max_speed : max_speed;
                }

                settled = real_seek_toward_clamped(
                    (tag->device_flags & 0x1) != 0, // DeviceFlags.position_loops
                    &dev->position_change, &dev->position, group->value, accel, max_speed, 0.0f, 1.0f);

                if (settled == 0) {
                    still_settling = 1;
                    if (dev->position_change != 0.0f &&
                        old_position_change * dev->position_change <= 0.0f) {
                        device_play_state_change_effect(object_index,
                            (dev->position_change > old_position_change)
                                ? tag->open.tag_id
                                : tag->close.tag_id);
                    }
                } else if (old_position_change <= 0.0f) {
                    device_play_state_change_effect(object_index, tag->closed.tag_id);
                } else {
                    device_play_state_change_effect(object_index, tag->opened.tag_id);
                }

                if (old_position != dev->position) {
                    dev->flags |= (1u << _device_position_changed_bit);
                }
                return still_settling;
            }

            dev->delay_ticks++;
            if (dev->delay_ticks == 1) {
                device_play_state_change_effect(object_index, tag->delay_effect.tag_id);
                return still_settling;
            }
        }
    }

    return still_settling;
}

#if 0
Original Ghidra decompilation (0x44b720), from tools/pack.py 0x44b720:

bool device_update_change_values(uint param_1)

{
  short sVar1;
  float *pfVar2;
  float *pfVar3;
  int iVar4;
  float fVar5;
  float fVar6;
  uint *puVar7;
  int iVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  char cVar14;
  bool local_12;

  puVar7 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar8 = *(int *)((*puVar7 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_12 = false;
  if ((ushort)puVar7[0x7e] != 0xffff) {
    iVar4 = *(int *)(DAT_0087abf0 + 0x34) + (uint)(ushort)puVar7[0x7e] * 8;
    if ((*(float *)(iVar4 + 4) != (float)puVar7[0x7f]) || ((float)puVar7[0x80] != 0.0)) {
      fVar9 = (float)puVar7[0x7f];
      cVar14 = real_seek_toward_clamped
                         (*(undefined4 *)(iVar4 + 4),*(undefined4 *)(iVar8 + 0x274),
                          *(undefined4 *)(iVar8 + 0x278),0,0x3f800000);
      local_12 = cVar14 == '\0';
      if (fVar9 != (float)puVar7[0x7f]) {
        puVar7[0x7d] = puVar7[0x7d] | 4;
      }
    }
  }
  if ((ushort)puVar7[0x81] != 0xffff) {
    iVar4 = *(int *)(DAT_0087abf0 + 0x34) + (uint)(ushort)puVar7[0x81] * 8;
    pfVar2 = (float *)(puVar7 + 0x82);
    if ((*(float *)(iVar4 + 4) == (float)puVar7[0x82]) && ((float)puVar7[0x83] == 0.0)) {
      *(undefined2 *)(puVar7 + 0x84) = 0;
    }
    else {
      pfVar3 = (float *)(puVar7 + 0x83);
      fVar9 = *(float *)(iVar8 + 0x27c);
      fVar5 = *(float *)(iVar8 + 0x284);
      fVar12 = *(float *)(iVar8 + 0x288) * (float)puVar7[0x7f] +
               (1.0 - (float)puVar7[0x7f]) * *(float *)(iVar8 + 0x280);
      fVar6 = *pfVar3;
      if (((*(float *)(iVar8 + 0x28c) <= (float)(int)(short)puVar7[0x84]) || (*pfVar2 != 0.0)) ||
         (*(float *)(iVar4 + 4) < *pfVar2)) {
        fVar10 = *pfVar2;
        fVar11 = *pfVar3;
        if (fVar12 < ABS(*pfVar3)) {
          fVar13 = fVar12;
          if (fVar6 <= 0.0) {
            fVar13 = -fVar12;
          }
          *pfVar3 = fVar13;
        }
        cVar14 = real_seek_toward_clamped
                           (*(undefined4 *)(iVar4 + 4),
                            fVar5 * (float)puVar7[0x7f] + (1.0 - (float)puVar7[0x7f]) * fVar9,fVar12
                            ,0,0x3f800000);
        if (cVar14 == '\0') {
          if ((*pfVar3 != 0.0) && (fVar11 * *pfVar3 < 0.0 != (fVar11 * *pfVar3 == 0.0))) {
            device_play_state_change_effect();
          }
          local_12 = true;
        }
        else if (fVar6 <= 0.0) {
          device_play_state_change_effect();
        }
        else {
          device_play_state_change_effect();
        }
        if (fVar10 != *pfVar2) {
          puVar7[0x7d] = puVar7[0x7d] | 4;
        }
        return local_12;
      }
      sVar1 = (short)puVar7[0x84] + 1;
      *(short *)(puVar7 + 0x84) = sVar1;
      if (sVar1 == 1) {
        device_play_state_change_effect();
        return local_12;
      }
    }
  }
  return local_12;
}
#endif
