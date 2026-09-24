// unit_spawn_with_starting_weapons  (Ghidra: FUN_00572110; renamed from the phase2 proposal)
// address 0x572110, size 760 bytes
// name confidence: 0.3 (phase2 proposal at 0.3, matches functions.md summary)
// rewrite confidence: 0.2 -- mirrors the treatment in src/units/unit_network_create_update_apply.c
//   for the same reason: the whole decode of the incoming record (a script/console command
//   argument list, per functions.md) into an object_placement_data happens through
//   vector3d_cross_product / vector3d_normalize_with_length calls and message-table lookups
//   whose arguments Ghidra could not bind, so that half is carried as an opaque decode step.
// evidence: the writes into the newly created object at the end all match biped_data fields
//   exactly by offset (the same 0x52c..0x54c network-block staging area
//   unit_network_create_update_apply.c uses, here re-purposed to stage a position/orientation
//   before copying it into object.velocity/angular_velocity/forward/up at 0x68/0x8c/0x74/0x80);
//   types/units.h unit_data.weapons[4] (0x2f8); callees object_new_with_datum_role_control,
//   object_set_position_and_recalculate.
// register convention: an incoming command-record pointer in EAX (in_EAX).
//   // blam-cc: EAX -> command_record
// UNSURE: PTR_DAT_00687130 / PTR_DAT_00687558 are two parallel "weapon name" lookup tables
//   (per units_types_notes and the sibling function unit_broadcast_state_change_event.c's own
//   UNSURE note on the same globals); the +0x28 field and the two weapon-name indices decoded
//   from the record (local_f4/local_f8, staged here as decoded_weapon_a_name/_b_name) are
//   preserved as raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t *network_message_table;       // 0x00687130, PTR_DAT_00687130, UNSURE shape
extern uint8_t *network_message_table_b;     // 0x00687558, PTR_DAT_00687558, UNSURE shape

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand,
                                    real_vector3d *stack_operand); // 0x4052c0
extern uint8_t message_delta_decode_compound_field(void); // 0x4ec590, UNSURE module
extern void message_delta_decode_compound_field_staged(void);    // 0x4ec670, UNSURE module
extern void network_index_cache_insert_if_free(uint32_t param_1); // 0x4e9cd0, UNSURE module
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern void object_set_position_and_recalculate(uint32_t object_index); // 0x4f52c0, UNSURE args  // real signature (object_set_position_and_recalculate.c): void object_set_position_and_recalculate(real_point3d *position, uint32_t object_index); Ghidra recovered 1 of 2 args at this call site
extern void unit_pickup_weapon(uint32_t param_1); // 0x56d400, UNSURE signature  // real signature (unit_pickup_weapon.c): uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index); Ghidra recovered 1 of 3 args at this call site

// Script/console-callable function that spawns a new unit at a computed placement with an
// initial orientation/velocity and pre-equips it with up to two named weapons.
void unit_spawn_with_starting_weapons(void *command_record)
{
    if (*(int32_t *)*(int32_t **)command_record != 0) {
        message_delta_decode_compound_field_staged();
        return;
    }

    if (message_delta_decode_compound_field() != 1) {
        return;
    }

    {
        // UNSURE: everything decoded here, see file header.
        uint32_t decoded_notify_argument = 0;        // local_100 -> network_index_cache_insert_if_free
        int32_t  decoded_weapon_a_index = 0;          // local_f8
        int32_t  decoded_weapon_b_index = 0;          // local_f4
        int32_t  decoded_seat_indices[4] = {0};        // aiStack_f0
        real_vector3d decoded_position = {0};          // local_e0/dc/d8
        real_vector3d decoded_velocity = {0};          // local_bc/b8/b4 -> shield-block staging
        real_vector3d decoded_angular_velocity = {0};  // local_b0/ac/a8
        int8_t decoded_flag_a4 = 0;                    // local_a4 -> biped 0x526
        real_vector3d decoded_forward = {0};           // local_d4/d0/cc
        real_vector3d decoded_up = {0};                // local_c8/c4/c0

        uint8_t placement[0x88];
        uint32_t new_object_index;
        int i;

        for (i = 0; i < 0x88; i++) {
            placement[i] = 0;
        }

        // The two weapon-name lookups (indexed by the decoded name indices, or -1 when the
        // corresponding index is 0) land in the placement's role/owner-linkage-shaped slots at
        // dword index 2 and 3 (offset 0x08/0x0c), per the original's local_8c[2]/[3] writes.
        {
            uint32_t weapon_a_tag = 0xffffffff;
            uint32_t weapon_b_tag = 0xffffffff;
            if (decoded_weapon_b_index != 0) {
                weapon_a_tag = *(uint32_t *)(*(uint8_t **)(network_message_table + 0x28) + decoded_weapon_b_index * 4);
            }
            if (decoded_weapon_a_index != 0) {
                weapon_b_tag = *(uint32_t *)(*(uint8_t **)(network_message_table_b + 0x28) + decoded_weapon_a_index * 4);
            }
            *(uint32_t *)(placement + 8) = weapon_a_tag;
            *(uint32_t *)(placement + 0xc) = weapon_b_tag;
        }

        new_object_index = object_new_with_datum_role_control((object_placement_data *)placement, 1);
        if (new_object_index == 0xffffffff) {
            return;
        }

        network_index_cache_insert_if_free(decoded_notify_argument);

        {
            object *obj = ((object_header *)object_data->data)[new_object_index & 0xffff].data;
            biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

            biped->network_body_vitality = decoded_position.i;
            *(uint32_t *)((uint8_t *)biped + 0x530) = *(uint32_t *)&decoded_position.j;
            *(uint32_t *)((uint8_t *)biped + 0x534) = *(uint32_t *)&decoded_position.k;
            *(uint32_t *)((uint8_t *)biped + 0x538) = *(uint32_t *)&decoded_velocity.i;
            *(uint32_t *)((uint8_t *)biped + 0x53c) = *(uint32_t *)&decoded_velocity.j;
            *(uint32_t *)((uint8_t *)biped + 0x540) = *(uint32_t *)&decoded_velocity.k;
            *(uint32_t *)((uint8_t *)biped + 0x544) = *(uint32_t *)&decoded_angular_velocity.i;
            *(uint32_t *)((uint8_t *)biped + 0x548) = *(uint32_t *)&decoded_angular_velocity.j;
            *(uint32_t *)((uint8_t *)biped + 0x54c) = *(uint32_t *)&decoded_angular_velocity.k;
            *(uint32_t *)((uint8_t *)biped + 0x550) = *(uint32_t *)&decoded_forward.i;
            *(uint32_t *)((uint8_t *)biped + 0x554) = *(uint32_t *)&decoded_forward.j;
            *(uint32_t *)((uint8_t *)biped + 0x558) = *(uint32_t *)&decoded_forward.k;
            *(uint32_t *)((uint8_t *)biped + 0x55c) = *(uint32_t *)&decoded_up.i;
            *(uint32_t *)((uint8_t *)biped + 0x560) = *(uint32_t *)&decoded_up.j;
            *(uint32_t *)((uint8_t *)biped + 0x564) = *(uint32_t *)&decoded_up.k;
            biped->unknown_526 = decoded_flag_a4;
            biped->ground_adjust_iteration_limit = 1;
            biped->network_update_sequence = 0;

            object_set_position_and_recalculate(new_object_index);

            obj->velocity = *(real_vector3d *)((uint8_t *)biped + 0x538);
            obj->angular_velocity = *(real_vector3d *)((uint8_t *)biped + 0x544);
            obj->forward = *(real_vector3d *)((uint8_t *)biped + 0x550);
            obj->up = *(real_vector3d *)((uint8_t *)biped + 0x55c);

            {
                unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
                unit->unknown_474 = 1;

                for (i = 0; i < 4; i++) {
                    if (decoded_seat_indices[i] == 0 ||
                        *(int32_t *)(*(uint8_t **)(network_message_table + 0x28) + decoded_seat_indices[i] * 4) == -1) {
                        unit->weapons[i] = k_datum_index_none;
                    } else {
                        unit_pickup_weapon(0);
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x572110):

void FUN_00572110(void)

{
  char cVar1;
  undefined4 *in_EAX;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 *puVar5;
  undefined4 uVar6;
  int iVar7;
  undefined4 local_100;
  int local_f8;
  int local_f4;
  int aiStack_f0 [4];
  undefined4 local_e0;
  undefined4 local_dc;
  undefined4 local_d8;
  undefined4 local_d4;
  undefined4 local_d0;
  undefined4 local_cc;
  undefined4 local_c8;
  undefined4 local_c4;
  undefined4 local_c0;
  undefined4 local_bc;
  undefined4 local_b8;
  undefined4 local_b4;
  undefined4 local_b0;
  undefined4 local_ac;
  undefined4 local_a8;
  undefined1 local_a4;
  undefined1 local_98 [12];
  undefined4 local_8c [6];
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if (cVar1 == '\x01') {
      vector3d_cross_product(&local_d4);
      vector3d_cross_product(local_98);
      vector3d_normalize_with_length();
      vector3d_normalize_with_length();
      uVar6 = 0xffffffff;
      if (local_f4 != 0) {
        uVar6 = *(undefined4 *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_f4 * 4);
      }
      uVar4 = 0xffffffff;
      if (local_f8 != 0) {
        uVar4 = *(undefined4 *)(*(int *)(PTR_DAT_00687558 + 0x28) + local_f8 * 4);
      }
      puVar5 = local_8c;
      for (iVar3 = 0x22; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar5 = 0;
        puVar5 = puVar5 + 1;
      }
      local_74 = local_e0;
      local_58 = local_d4;
      local_70 = local_dc;
      local_6c = local_d8;
      local_4c = local_c8;
      local_54 = local_d0;
      local_50 = local_cc;
      local_48 = local_c4;
      local_44 = local_c0;
      local_8c[2] = uVar4;
      local_8c[3] = uVar6;
      uVar2 = object_new_with_datum_role_control(local_8c,1);
      if (uVar2 != 0xffffffff) {
        FUN_004e9cd0(local_100);
        iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
        *(undefined4 *)(iVar3 + 0x52c) = local_e0;
        *(undefined4 *)(iVar3 + 0x530) = local_dc;
        *(undefined4 *)(iVar3 + 0x534) = local_d8;
        *(undefined4 *)(iVar3 + 0x538) = local_bc;
        *(undefined4 *)(iVar3 + 0x53c) = local_b8;
        *(undefined4 *)(iVar3 + 0x540) = local_b4;
        *(undefined4 *)(iVar3 + 0x544) = local_b0;
        *(undefined4 *)(iVar3 + 0x548) = local_ac;
        *(undefined4 *)(iVar3 + 0x54c) = local_a8;
        *(undefined4 *)(iVar3 + 0x550) = local_d4;
        *(undefined4 *)(iVar3 + 0x554) = local_d0;
        *(undefined4 *)(iVar3 + 0x558) = local_cc;
        *(undefined4 *)(iVar3 + 0x55c) = local_c8;
        *(undefined4 *)(iVar3 + 0x560) = local_c4;
        *(undefined4 *)(iVar3 + 0x564) = local_c0;
        *(undefined1 *)(iVar3 + 0x526) = local_a4;
        *(undefined1 *)(iVar3 + 0x525) = 1;
        *(undefined1 *)(iVar3 + 0x527) = 0;
        object_set_position_and_recalculate();
        *(undefined4 *)(iVar3 + 0x68) = *(undefined4 *)(iVar3 + 0x538);
        *(undefined4 *)(iVar3 + 0x6c) = *(undefined4 *)(iVar3 + 0x53c);
        *(undefined4 *)(iVar3 + 0x70) = *(undefined4 *)(iVar3 + 0x540);
        *(undefined4 *)(iVar3 + 0x8c) = *(undefined4 *)(iVar3 + 0x544);
        *(undefined4 *)(iVar3 + 0x90) = *(undefined4 *)(iVar3 + 0x548);
        *(undefined4 *)(iVar3 + 0x94) = *(undefined4 *)(iVar3 + 0x54c);
        *(undefined4 *)(iVar3 + 0x74) = *(undefined4 *)(iVar3 + 0x550);
        *(undefined4 *)(iVar3 + 0x78) = *(undefined4 *)(iVar3 + 0x554);
        *(undefined4 *)(iVar3 + 0x7c) = *(undefined4 *)(iVar3 + 0x558);
        *(undefined4 *)(iVar3 + 0x80) = *(undefined4 *)(iVar3 + 0x55c);
        *(undefined4 *)(iVar3 + 0x84) = *(undefined4 *)(iVar3 + 0x560);
        *(undefined1 *)(iVar3 + 0x475) = 1;
        iVar7 = 0;
        *(undefined4 *)(iVar3 + 0x88) = *(undefined4 *)(iVar3 + 0x564);
        puVar5 = (undefined4 *)(iVar3 + 0x2f8);
        do {
          if ((aiStack_f0[iVar7] == 0) ||
             (*(int *)(*(int *)(PTR_DAT_00687130 + 0x28) + aiStack_f0[iVar7] * 4) == -1)) {
            *puVar5 = 0xffffffff;
          }
          else {
            FUN_0056d400(0);
          }
          iVar7 = iVar7 + 1;
          puVar5 = puVar5 + 1;
        } while (iVar7 < 4);
        return;
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
