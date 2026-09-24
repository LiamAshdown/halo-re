// device_machine_update  (Ghidra: device_machine_update, already named; functions.md: "Per-
// object tick update for a device machine: advances its automatic-open proximity check, tracks
// how long it has sat fully open before auto-closing, and moves any objects riding on it by the
// device's [motion]")
// address 0x44b0a0, size 1320 bytes
// name confidence: 0.55   rewrite confidence: 0.4
// evidence: types/devices.h device_machine_data (ticks_since_fully_open 0x218,
//   last_elevator_position 0x21c), device_data (flags 0x1f4, power_group/power 0x1f8/0x1fc,
//   position_group/position 0x204/0x208, type_flags 0x214); types/tags.h DeviceMachine
//   (machine_type 0x290, machine_flags 0x292, elevator_node 0x2ea, door_open_time_ticks 0x320),
//   Device (automatic_activation_radius 0x21c, inverse_position_transition_time 0x288,
//   inverse_depowered_position_transition_time 0x280); types/objects.h object
//   (location_leaf_index 0x098, bounding_center 0x0a0, bounding_radius 0x0ac, forward 0x074,
//   vitality_flags 0x106, nodes 0x1f0), object_vitality_flags (_object_health_frozen_bit),
//   real_matrix4x3 (position 0x28); global 0x008603b0 object_data, 0x0087bc14 tag_instances,
//   0x0087abf0 device_groups; callees device_group_set_value (0x44bd70, this batch),
//   object_find_in_sphere, object_set_cluster_and_parent, object_unlink_cluster_or_notify_parent
//   (established elsewhere in the objects module).
// register convention: object index is already a plain, genuinely-stack parameter in Ghidra's
//   own output (`device_machine_update(uint param_1)`); no unresolved registers appear.
// Resolved against disassembly (objdump -d -M intel bin/halo.exe, 0x44b0a0..0x44b5cf) at three
// points where Ghidra's decompile is misleading or silently opaque:
//   1. 0x44b399/0x44b3ae: both device_group_set_value calls pass THIS device's own
//      position_group in ESI (confirmed at 0x44b34c and 0x44b3a9), not a fresh argument Ghidra
//      could show. Per device_group_set_value.c's own disassembly, that function looks up
//      objects by matching power_group, and fires the tag's repowered/depowered effect -- so
//      these two calls, made with a position_group, do not target this door's own
//      opened/closed effect or even necessarily this door's own object. Preserved verbatim.
//   2. The elevator gate at 0x44b3cc is `test BYTE PTR [ebx+0x292],0x4`, and EBX is reloaded
//      with the TAG pointer at 0x44b374 (`mov ebx,[esp+0x18]`) -- so it is the tag's
//      DeviceMachine.machine_flags (elevator, MachineFlags bit 2), NOT `type_flags` on the
//      object (edi+0x214), which is a different field and the one the
//      does_not_operate_automatically / one_sided bits earlier in this same function do come
//      from. Ghidra happens to render this one correctly (`*(byte *)(iVar15 + 0x292) & 4`);
//      the note is kept because the two flag words are one letter apart in the decompile and
//      an earlier revision of this file read the wrong one.
//   3. 0x44b592-0x44b5b0: the final "push the cached position back out" block reads
//      object.position and writes it back to the very same address (esi and edi are the same
//      object pointer here; iVar15 in Ghidra's text is a redundant re-fetch of it) -- a literal
//      self-assignment. The actual effect of this block is the relink the two surrounding
//      object_unlink_cluster_or_notify_parent/object_set_cluster_and_parent calls perform, not
//      a position change; preserved as-is rather than "corrected" to copy from device_data.
//   4. 0x44b290 reads the multiplayer/campaign selector as `mov edx,DWORD PTR ds:0x6f1d20`
//      followed by `test edx,edx` -- a full 32-bit load. R04 unified every declaration
//      on game.h game_engine_definition *current_game_engine (all accesses are DWORD).
// UNSURE: object+0xb8, read here as a signed int16 team index (0x44b298 `mov ax,WORD PTR
//   [esi+0xb8]`, then the signed pair `test ax,ax; jl` / `cmp ax,0xa; jge`) and used to index a
//   per-team bitmask at game_globals_006b0b84+0xa4, is documented in types/objects.h as name_index.
//   out/phase4/devices_types_notes.md item 5 flags the same conflict and assigns it to the
//   objects module owner rather than resolving it here; this file keeps the raw offset rather
//   than object->name_index so the mismatch stays visible. The WIDTH and SIGNEDNESS of the read
//   are disassembly-confirmed; only which field lives there is open.
// object+0x4d4, compared against this device's own object index to find elevator riders, is
//   types/units.h biped_data.last_ground_object_index: the rider search passes type mask 1
//   (bipeds only) to object_find_in_sphere, and 0x44b4e4 compares [rider+0x4d4] with the
//   machine's object index.
// UNSURE: object+0x106 bit 0x04 (used here through object_vitality_flags'
//   _object_health_frozen_bit) is plausible but not proven for this exact use.
// UNSURE: the tag-derived flags word this function tests on a nearby candidate
//   (tag_instances[candidate_tag].data + sizeof(Object), mask 0x4000) is UnitFlags'
//   cannot_open_doors_automatically bit 14 per types/tags.h's own bitfield comment, but there
//   is no named bit constant for it in this codebase yet (DeviceFlags/UnitFlags are documented
//   as plain bitfield comments, not enums), so the mask is spelled out literally.
// reconciled: R46 biped_data +0x4d4 last_ground_surface_index -> last_ground_object_index (an object datum); the elevator rider test reads it through biped_data instead of a raw offset
// reconciled: R04 0x006f1d20 int32_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "devices.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *device_groups; // 0x0087abf0
extern game_time_globals *game_time; // 0x006f1d6c (types/game.h), +0x0c game_time is the tick
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
    // multiplayer/campaign selector (per devices_types_notes.md), a wider read than the
    // uint8_t use at this same address in the items module
extern void *game_globals_006b0b84; // 0x006b0b84, +0xa4 is the per-team bitmask array this reads

extern uint8_t device_group_set_value(uint16_t group_index, float value); // 0x44bd70, this batch
    // blam-cc: ESI -> group_index, stack -> value; returns 0/1 in AL (both call sites here
    // discard it, but the declaration must agree with src/devices/device_group_set_value.c)
extern int16_t object_find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location,
    real_point3d *center, float radius, datum_index *out_objects, int16_t max_output); // 0x4f6fe0
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0

uint32_t device_machine_update(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_machine_data *dev = (device_machine_data *)((uint8_t *)obj + sizeof(object));
    DeviceMachine *tag = (DeviceMachine *)tag_instances[obj->definition_tag & 0xffff].data;

    // A gear machine has no group value of its own: it continuously rotates its position
    // group's value, blending the powered/depowered transition rates by the current power
    // level, and wraps back into 0.0 .. 1.0 rather than settling.
    if (tag->machine_type == machinetype_gear) {
        float position = tag->base.inverse_position_transition_time * dev->device.power +
            (1.0f - dev->device.power) * tag->base.inverse_depowered_position_transition_time +
            dev->device.position;
        dev->device.position = position;
        if (1.0f <= position) {
            dev->device.position = position - 1.0f;
        }
        dev->device.position_change = 0.0f;
        dev->device.flags |= (1u << _device_position_changed_bit);
        if (dev->device.position_group != -1) {
            ((device_group *)device_groups->data)[(uint16_t)dev->device.position_group].value =
                dev->device.position;
        }
    }

    // Automatic-open proximity scan: only for doors (machine_type == door) that do not have
    // does_not_operate_automatically set, staggered so only 1 in 4 doors is scanned per tick.
    if ((dev->device.type_flags & (1u << _device_machine_does_not_operate_automatically_bit)) == 0 &&
        tag->machine_type == machinetype_door &&
        (game_time->game_time + (int32_t)object_index & 3) == 0) {
        datum_index candidates[k_device_machine_activation_maximum];
        int16_t candidate_count;
        int should_open = 0;
        float radius = (0.0001f <= tag->base.automatic_activation_radius)
            ? tag->base.automatic_activation_radius : obj->bounding_radius;

        candidate_count = object_find_in_sphere(1, 1, &obj->location_leaf_index,
            &obj->bounding_center, radius, candidates, k_device_machine_activation_maximum);
        if (0 < candidate_count) {
            int16_t i;
            for (i = 0; i < candidate_count; i++) {
                object *candidate = ((object_header *)object_data->data)[candidates[i] & 0xffff].data;
                int counts = 1;
                int passes_side_test = 1;

                if (((candidate->vitality_flags & _object_health_frozen_bit) != 0) ||
                    ((*(uint32_t *)((uint8_t *)tag_instances[candidate->definition_tag & 0xffff].data
                        + sizeof(Object)) & 0x4000) != 0)) { // UnitFlags.cannot_open_doors_automatically
                    counts = 0;
                }

                if ((dev->device.type_flags & (1u << _device_machine_one_sided_bit)) != 0 &&
                    dev->device.position == 0.0f) {
                    // One-sided and fully closed: only a candidate on the front side, or one
                    // exempted by the team test below, passes.
                    int16_t team = *(int16_t *)((uint8_t *)candidate + 0xb8); // UNSURE, see header
                    int exempt;
                    if (current_game_engine == 0) {
                        if (team < 0 || 9 < team) {
                            exempt = 1;
                        } else {
                            exempt = (*(uint32_t *)((uint8_t *)game_globals_006b0b84 + 0xa4 +
                                ((team + 10) >> 5) * 4) & (1u << ((team + 10) & 0x1f))) == 0;
                        }
                    } else {
                        exempt = team != 1;
                    }
                    passes_side_test = exempt ||
                        (candidate->bounding_center.x - obj->bounding_center.x) * obj->forward.i +
                        (candidate->bounding_center.y - obj->bounding_center.y) * obj->forward.j +
                        (candidate->bounding_center.z - obj->bounding_center.z) * obj->forward.k <= 0.0f;
                }

                if (counts && passes_side_test) {
                    should_open = 1;
                }
            }
            if (should_open) {
                if (dev->device.position_group != -1) {
                    device_group_set_value((uint16_t)dev->device.position_group, 1.0f);
                    // blam-cc: position_group in ESI, per disassembly; see header note 1
                }
                dev->ticks_since_fully_open = k_device_machine_open_grace_ticks;
            }
        }
    }

    // Auto-close a fully-open door once it has sat open for door_open_time_ticks.
    if (tag->machine_type == machinetype_door) {
        if (dev->device.position == 1.0f) {
            dev->ticks_since_fully_open++;
            if ((int32_t)tag->door_open_time_ticks < dev->ticks_since_fully_open &&
                dev->device.position_group != -1) {
                device_group_set_value((uint16_t)dev->device.position_group, 0.0f);
                // blam-cc: position_group in ESI, per disassembly; see header note 1
            }
        } else {
            dev->ticks_since_fully_open = 0;
        }
    }

    // Elevator: translate every rider by the delta of the named node's world position since
    // the previous tick. Gated on the TAG's machine_flags (elevator, bit 2), not the object's
    // own type_flags -- see header note 2.
    if ((tag->machine_flags & 0x4) != 0) {
        if (tag->elevator_node != (uint16_t)0xffff) {
            // 0x44b3f7 `movsx edx,ax` then `imul edx,edx,0x34`: the node index is scaled as a
            // SIGNED int16, and obj->nodes.offset at 0x44b3fa likewise (`movsx eax,[ecx+0x1f2]`).
            real_matrix4x3 *node = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset) +
                (int16_t)tag->elevator_node;
            float dx = node->position.x - dev->last_elevator_position.x;
            float dy = node->position.y - dev->last_elevator_position.y;
            float dz = node->position.z - dev->last_elevator_position.z;

            if (dx != 0.0f || dy != 0.0f || dz != 0.0f) {
                datum_index riders[k_device_machine_rider_maximum];
                int16_t rider_count = object_find_in_sphere(1, 1, &obj->location_leaf_index,
                    &obj->bounding_center, obj->bounding_radius, riders, k_device_machine_rider_maximum);
                if (0 < rider_count) {
                    int16_t i;
                    for (i = 0; i < rider_count; i++) {
                        object *rider = ((object_header *)object_data->data)[riders[i] & 0xffff].data;
                        biped_data *rider_biped = (biped_data *)((uint8_t *)rider + k_unit_object_size);
                        if (rider_biped->last_ground_object_index == object_index) { // 0x44b4e4
                            real_point3d p = rider->position;
                            object_unlink_cluster_or_notify_parent(riders[i]);
                            rider->position.x = p.x + dx;
                            rider->position.y = p.y + dy;
                            rider->position.z = p.z + dz;
                            object_set_cluster_and_parent(riders[i], 0);
                        }
                    }
                }
            }
            dev->last_elevator_position = node->position;
        }
    }

    // If a position update elsewhere this tick moved the cached value, relink the object into
    // the world. Per disassembly (header note 3) this writes object.position back into itself;
    // the relink calls, not a value change, are the actual effect.
    if ((dev->device.flags & (1u << _device_position_changed_bit)) != 0) {
        object_unlink_cluster_or_notify_parent(object_index);
        obj->position = obj->position;
        object_set_cluster_and_parent(object_index, 0);
        dev->device.flags &= ~(uint32_t)(1u << _device_position_changed_bit);
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x44b0a0), from tools/pack.py 0x44b0a0:

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 device_machine_update(uint param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  short sVar4;
  uint *puVar5;
  uint *puVar6;
  uint uVar7;
  int iVar8;
  float fVar9;
  bool bVar10;
  float fVar11;
  float fVar12;
  bool bVar13;
  ushort uVar14;
  int iVar15;
  int iVar16;
  uint *puVar17;
  char cVar18;
  uint *local_206c;
  uint local_2068;
  uint local_2040 [16];
  uint local_2000 [2047];
  undefined4 uStack_4;

  uStack_4 = 0x44b0aa;
  iVar16 = (param_1 & 0xffff) * 0xc;
  puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
  iVar15 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(short *)(iVar15 + 0x290) == 2) {
    fVar9 = *(float *)(iVar15 + 0x288) * (float)puVar5[0x7f] +
            (1.0 - (float)puVar5[0x7f]) * *(float *)(iVar15 + 0x280) + (float)puVar5[0x82];
    puVar5[0x82] = (uint)fVar9;
    if (1.0 <= fVar9) {
      puVar5[0x82] = (uint)(fVar9 - 1.0);
    }
    puVar5[0x83] = 0;
    puVar5[0x7d] = puVar5[0x7d] | 4;
    if ((ushort)puVar5[0x81] != 0xffff) {
      *(uint *)(*(int *)(DAT_0087abf0 + 0x34) + 4 + (uint)(ushort)puVar5[0x81] * 8) = puVar5[0x82];
    }
  }
  if ((puVar5[0x85] & 1) == 0) {
    if (*(short *)(iVar15 + 0x290) != 0) goto LAB_0044b3cc;
    if ((*(int *)(DAT_006f1d6c + 0xc) + param_1 & 3) == 0) {
      bVar13 = false;
      if (0.0001 <= *(float *)(iVar15 + 0x21c)) {
        local_206c = *(uint **)(iVar15 + 0x21c);
      }
      else {
        local_206c = (uint *)puVar5[0x2b];
      }
      uVar14 = object_find_in_sphere(1,1,puVar5 + 0x26,puVar5 + 0x28,local_206c,local_2040,0x10);
      if (0 < (short)uVar14) {
        local_206c = (uint *)(uint)uVar14;
        puVar17 = local_2040;
        do {
          puVar6 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*puVar17 & 0xffff) * 0xc);
          bVar10 = true;
          if (((*(byte *)((int)puVar6 + 0x106) & 4) != 0) ||
             ((*(uint *)(*(int *)((*puVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x17c) & 0x4000
              ) != 0)) {
            bVar10 = false;
          }
          if (((puVar5[0x85] & 2) == 0) || ((float)puVar5[0x82] != 0.0)) {
LAB_0044b327:
            if (bVar10) {
              bVar13 = true;
            }
          }
          else {
            sVar4 = (short)puVar6[0x2e];
            if (DAT_006f1d20 == 0) {
              if ((sVar4 < 0) || (9 < sVar4)) goto LAB_0044b327;
              cVar18 = '\x01' - ((1 << ((byte)(sVar4 + 10) & 0x1f) &
                                 *(uint *)(DAT_006b0b84 + 0xa4 + (sVar4 + 10 >> 5) * 4)) != 0);
            }
            else {
              cVar18 = sVar4 != 1;
            }
            if ((cVar18 != '\0') ||
               (((float)puVar6[0x28] - (float)puVar5[0x28]) * (float)puVar5[0x1d] +
                ((float)puVar6[0x29] - (float)puVar5[0x29]) * (float)puVar5[0x1e] +
                ((float)puVar6[0x2a] - (float)puVar5[0x2a]) * (float)puVar5[0x1f] <= 0.0))
            goto LAB_0044b327;
          }
          puVar17 = puVar17 + 1;
          local_206c = (uint *)((int)local_206c - 1);
        } while (local_206c != (uint *)0x0);
        if (bVar13) {
          if ((short)puVar5[0x81] != -1) {
            device_group_set_value(0x3f800000);
          }
          puVar5[0x86] = 0xfffffffd;
        }
      }
    }
  }
  if (*(short *)(iVar15 + 0x290) == 0) {
    if (puVar5[0x82] == 0x3f800000) {
      uVar7 = puVar5[0x86];
      puVar5[0x86] = uVar7 + 1;
      if ((*(int *)(iVar15 + 800) < (int)(uVar7 + 1)) && ((short)puVar5[0x81] != -1)) {
        device_group_set_value(0);
      }
    }
    else {
      puVar5[0x86] = 0;
    }
  }
LAB_0044b3cc:
  if ((*(byte *)(iVar15 + 0x292) & 4) != 0) {
    if (*(short *)(iVar15 + 0x2ea) != -1) {
      iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
      iVar15 = (int)*(short *)(iVar8 + 0x1f2) + *(short *)(iVar15 + 0x2ea) * 0x34 + iVar8;
      fVar9 = *(float *)(iVar15 + 0x28) - (float)puVar5[0x87];
      fVar11 = *(float *)(iVar15 + 0x2c) - (float)puVar5[0x88];
      fVar12 = *(float *)(iVar15 + 0x30) - (float)puVar5[0x89];
      if ((((fVar9 != 0.0) || (fVar11 != 0.0)) || (fVar12 != 0.0)) &&
         (uVar14 = object_find_in_sphere
                             (1,1,puVar5 + 0x26,puVar5 + 0x28,puVar5[0x2b],local_2000,0x800),
         0 < (short)uVar14)) {
        local_2068 = (uint)uVar14;
        local_206c = local_2000;
        do {
          uVar7 = *local_206c;
          iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
          if (*(uint *)(iVar8 + 0x4d4) == param_1) {
            fVar1 = *(float *)(iVar8 + 0x5c);
            fVar2 = *(float *)(iVar8 + 0x60);
            fVar3 = *(float *)(iVar8 + 100);
            object_unlink_cluster_or_notify_parent();
            *(float *)(iVar8 + 0x5c) = fVar9 + fVar1;
            *(float *)(iVar8 + 0x60) = fVar11 + fVar2;
            *(float *)(iVar8 + 100) = fVar12 + fVar3;
            object_set_cluster_and_parent(uVar7,0);
          }
          local_206c = local_206c + 1;
          local_2068 = local_2068 - 1;
        } while (local_2068 != 0);
      }
      puVar5[0x87] = (uint)*(float *)(iVar15 + 0x28);
      puVar5[0x88] = *(uint *)(iVar15 + 0x2c);
      puVar5[0x89] = *(uint *)(iVar15 + 0x30);
    }
  }
  if ((puVar5[0x7d] & 4) != 0) {
    iVar15 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
    object_unlink_cluster_or_notify_parent();
    *(uint *)(iVar15 + 0x5c) = puVar5[0x17];
    *(uint *)(iVar15 + 0x60) = puVar5[0x18];
    *(uint *)(iVar15 + 100) = puVar5[0x19];
    object_set_cluster_and_parent(param_1,0);
    puVar5[0x7d] = puVar5[0x7d] & 0xfffffffb;
  }
  return 1;
}
#endif
