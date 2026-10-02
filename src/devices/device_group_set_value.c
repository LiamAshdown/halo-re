// device_group_set_value  (Ghidra: device_group_set_value, already named; functions.md: "Sets a
// device group's shared value ..., marking it as changing and broadcasting the state-change
// effect/sound to all objects that reference [it]")
// address 0x44bd70, size 292 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: types/devices.h device_group (flags at +0x02, value at +0x04), device_group_flags
// (_device_group_can_change_only_once_bit, _device_group_changed_bit -- the locked test is
// both bits set, refusing the change); types/objects.h object_iterator, _object_mask_device;
// types/tags.h Device (repowered.tag_id 0x1fc, depowered.tag_id 0x1ec).
// register convention: group index in ESI (unaff_SI), value as the sole recognized stack
// parameter (Ghidra's own `device_group_set_value(float param_1)`).
//   // blam-cc: ESI -> group_index, stack -> value
// Resolved against disassembly (objdump -d -M intel bin/halo.exe, 0x44bd70-0x44be93), which is
// necessary here for three things Ghidra's decompile completely drops:
//   1. The return value: every exit does a plain `mov al,dl` or `mov al,bl` with dl seeded to 0
//      and bl only ever set to 1 on the accepted-change path, so this is a genuine 0/1 bool,
//      not the packed FPU-flag garbage Ghidra's `uVar3`/`uVar4` values suggest.
//   2. Which object.power_group/position_group the broadcast loop checks: only power_group
//      (object+0x1f8), matching the "repowered/depowered" effect this fires (see point 3) --
//      never position_group. This is the same field regardless of whether the group just
//      changed is itself a power or a position group; see the UNSURE this creates for
//      src/devices/device_machine_update.c's two calls into this function with a
//      position_group in ESI.
//   3. The ECX argument to each device_play_state_change_effect call: it is Device.repowered
//      (0x1fc) when the newly-applied value is non-zero, Device.depowered (0x1ec) when it is
//      exactly 0.0 (0x44be4e-0x44be6d) -- entirely invisible in Ghidra's bare
//      `device_play_state_change_effect();` call.
// Why the broadcast keys on power_group even when the caller hands it a position group: the
// group table is a shared namespace, and the value this function writes is read back by
// device_update_change_values through BOTH links -- group[power_group].value becomes
// device_data.power, group[position_group].value becomes device_data.position. So one group
// index N can be a control's position_group and a machine's power_group at the same time, which
// is exactly how a switch powers a machine: device_change_power_state writes the control's
// position group N, and this function's loop then finds the machine (power_group == N) and fires
// ITS Device.repowered / Device.depowered. Nothing is mismatched; see
// src/devices/device_change_power_state.c's header for the full chain.
// UNSURE (the residue): device_machine_update.c's two calls pass a door's own position_group,
// and this loop will not match that door itself unless its power_group happens to equal it. For
// a plain automatic door with no power group wired, no object matches and no effect fires -- the
// door's opened/closed pair is played by device_update_change_values instead (tag+0x1cc /
// tag+0x1dc), which is the function that actually owns that transition. Consistent, but the
// scenario-side wiring that makes it so is outside this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "devices.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *device_groups; // 0x0087abf0
extern tag_instance *tag_instances; // 0x0087bc14

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, objects module
extern void device_play_state_change_effect(uint32_t object_index, TagID tag_id); // 0x44c1a0, this batch
    // blam-cc: EAX -> object_index, ECX -> tag_id

uint8_t device_group_set_value(uint16_t group_index, float value)
{
    device_group *group;
    uint16_t flags;
    object_iterator iterator;
    object *obj;

    if (value < 0.0f) {
        value = 0.0f;
    } else if (1.0f < value) {
        value = 1.0f;
    }

    if (group_index == 0xffff) {
        return 0;
    }

    group = &((device_group *)device_groups->data)[group_index];
    if (group->value == value) {
        return 0;
    }

    flags = group->flags;
    if ((flags & (1u << _device_group_can_change_only_once_bit)) != 0 &&
        (flags & (1u << _device_group_changed_bit)) != 0) {
        return 0; // locked: can_change_only_once and already changed once
    }

    group->flags = flags | (1u << _device_group_changed_bit);
    group->value = value;

    iterator.type_mask = _object_mask_device;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        device_data *candidate_dev = (device_data *)((uint8_t *)obj + sizeof(object));

        if (candidate_dev->power_group == (int16_t)group_index) { // see header note 2
            Device *tag = (Device *)tag_instances[obj->definition_tag & 0xffff].data;
            // 0x44be4e-0x44be6d: `fld [esp+0x18]; fcomp ds:0x672ac0; test ah,0x41; jne` picks
            // tag+0x1ec (depowered) when value <= 0.0 and tag+0x1fc (repowered) otherwise.
            // Since value is already clamped to [0,1] above, "<= 0" and "!= 0" coincide.
            // The object index handed to the effect is iterator.handle, read back from the
            // iterator at 0x44be6d (`mov eax,[esp+0xc]`, iterator base esp+0x4, so +0x08).
            device_play_state_change_effect(iterator.handle,
                (value != 0.0f) ? tag->repowered.tag_id : tag->depowered.tag_id);
        }
        obj = object_iterator_next(&iterator);
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x44bd70), from tools/pack.py 0x44bd70:

uint device_group_set_value(float param_1)

{
  float fVar1;
  ushort uVar2;
  undefined4 in_EAX;
  uint uVar3;
  int iVar4;
  ushort unaff_SI;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;
  undefined2 uVar5;

  uVar5 = (undefined2)((uint)in_EAX >> 0x10);
  uVar3 = CONCAT22(uVar5,(ushort)(param_1 < 0.0) << 8 | (ushort)NAN(param_1) << 10 |
                         (ushort)(param_1 == 0.0) << 0xe);
  if (param_1 < 0.0) {
    param_1 = 0.0;
  }
  else {
    uVar3 = CONCAT22(uVar5,(ushort)(param_1 < 1.0) << 8 | (ushort)NAN(param_1) << 10 |
                           (ushort)(param_1 == 1.0) << 0xe);
    if (param_1 < 1.0 == 0 && (param_1 == 1.0) == 0) {
      param_1 = 1.0;
    }
  }
  if (unaff_SI != 0xffff) {
    fVar1 = *(float *)(*(int *)(DAT_0087abf0 + 0x34) + 4 + (uint)unaff_SI * 8);
    iVar4 = *(int *)(DAT_0087abf0 + 0x34) + (uint)unaff_SI * 8;
    uVar3 = (uint)(ushort)((ushort)(fVar1 < param_1) << 8 |
                           (ushort)(NAN(fVar1) || NAN(param_1)) << 10 |
                          (ushort)(fVar1 == param_1) << 0xe);
    if (fVar1 != param_1) {
      uVar2 = *(ushort *)(iVar4 + 2);
      uVar3 = (uint)uVar2;
      if (((uVar2 & 1) == 0) || ((uVar2 & 2) == 0)) {
        *(ushort *)(iVar4 + 2) = uVar2 | 2;
        *(float *)(iVar4 + 4) = param_1;
        local_4 = 0x86868686;
        local_10 = 0x380;
        local_c = 0;
        local_a = 0;
        local_8 = 0xffffffff;
        iVar4 = object_iterator_next(&local_10);
        while (iVar4 != 0) {
          if (*(ushort *)(iVar4 + 0x1f8) == unaff_SI) {
            device_play_state_change_effect();
          }
          iVar4 = object_iterator_next(&local_10);
        }
        return 1;
      }
    }
  }
  return uVar3 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
