// equipment_create_from_creation_message  (Ghidra: FUN_004bbe20; named from
// out/phase4/items_functions.md and paired with equipment_build_creation_message (0x4bbc90))
// address 0x4bbe20, size 578 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: network delta message type 0x1f (k_message_equipment_creation); types/items.h
//   equipment_creation_message is exactly the 0x58-byte record decoded here -- the placement
//   stores at 0x4bbecd..0x4bbf60 read B+0x00 (definition_tag), B+0x08 (name_index), B+0x0c
//   (owner_hash), B+0x10 (parent_hash), B+0x18 (position), B+0x24 (forward) and B+0x30 (up),
//   and the network-block stores at 0x4bbf9b..0x4bc00c read B+0x14 (object_flags), B+0x18
//   (position), B+0x3c (velocity), B+0x48 (angular_velocity) and B+0x54 (baseline_index);
//   types/objects.h object_placement_data (size 0x88 = the 0x22 dwords the `rep stos` at
//   0x4bbecb clears); types/items.h equipment_data network block at 0x244..0x26c.
// register convention: the incoming record in EAX.
// blam-cc: EAX -> incoming_record
// Ghidra recovers no arguments here: the incoming record arrives in EAX and the decoded message
// lives in an un-typed stack buffer, so every value the function stores decompiles as an
// un-assigned `local_*`. objdump -d -M intel bin/halo.exe resolves all of it. For 0x4c5c10:
//     4c5c10: mov ecx,DWORD PTR [eax]       ; eax = the record, in EAX
//     4c5c12: mov edx,DWORD PTR [ecx]       ; the mode word is behind TWO indirections
//     4c5c1a: test edx,edx / jne <reject>   ; nonzero -> message_delta_decode_compound_field_staged, drop the message
//     4c5c26: lea ecx,[esp+0x10]            ; ECX = the decoded-message buffer, call it B
//     4c5c2a: call 0x4ec590 / cmp al,0x1    ; decodes into B; only AL == 1 proceeds
// Every later [esp+N] is B + (N - 0x10), adjusted for the pushes in between, and each one lands
// on a field types/items.h already documents. So message_delta_decode_compound_field takes its destination in ECX (it
// is not the no-argument predicate the decompilation suggests) and the message below is a real
// struct rather than a row of zero placeholders. The hash -> datum_index resolution through
// object_network_id_table + 0x28 is the same idiom as
// src/objects/object_apply_linked_impulse.c and the four ammo handlers in this module.
// (this function is 0x4bbe20; the buffer base is `lea ecx,[esp+0x18]` at 0x4bbe36.)
// UNSURE: network_index_cache_insert_if_free (0x4e9cd0) is the networking hash-table insert. It takes the table root
// in EAX (0x4bbf7a loads the literal 0x6870d8), the new object's datum_index in ECX and the
// message's object_hash on the stack; only the stack argument is expressible here.
// UNSURE: the forward/up basis is re-orthogonalized rather than used as decoded -- two cross
// products, then both vectors normalized -- so a slightly stale replicated basis still yields
// an orthonormal one.
// UNSURE: equipment_data.last_update_valid / last_update_state (0x26c..0x294) are NOT written
// here, unlike weapon_apply_network_update's 0x310/0x314 pair; the equipment update applier
// (0x4bc250) is one of Ghidra's misses, so nothing in the export writes them.
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)

// FIXED 2026-09-28 (networking call audit): message_delta_decode_compound_field / _forced / _staged take the
// decode context first (EAX) and the destination second (ECX); the calls here had the context missing or the two
// swapped.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data; // 0x008603b0
extern network_id_table *object_network_id_table; // 0x00687130
    // its value is the hash -> datum_index array that resolves
    // equipment_creation_message.parent_hash into object_placement_data.role
extern network_id_table *player_network_id_table; // 0x00687558
    // this one player_network_id_table too; the name is UNSURE in both places); resolves
    // equipment_creation_message.owner_hash into object_placement_data.owner_linkage. UNSURE name.

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0, out = stack_operand x ecx_operand
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context: rejects (skips) the message
extern void network_index_cache_insert_if_free(int32_t object_hash); // 0x4e9cd0, networking; see file header
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern void object_set_position_and_recalculate(real_point3d *position, datum_index object_index); // 0x4f52c0

// Handles an incoming network equipment-creation update: decodes the message, re-orthonormalizes
// the replicated forward/up basis, resolves the owner and parent hashes into object handles,
// spawns the equipment through object_new_with_datum_role_control, registers it in the
// networking hash table, and then seeds its network block, position, velocity and spin.
void equipment_create_from_creation_message(void *incoming_record)
{
    equipment_creation_message decoded;
    real_vector3d forward, up, cross;
    object_placement_data placement;
    uint32_t role_material, owner_material;
    datum_index new_object_index;
    object *obj;
    equipment_data *ed;
    uint8_t *zero;
    int32_t i;

    // The mode word sits behind two indirections; nonzero means "not a create for us".
    if (*(int32_t *)*(int32_t **)incoming_record != 0) {
        message_delta_decode_compound_field_staged(incoming_record);
        return;
    }
    if (message_delta_decode_compound_field(incoming_record, &decoded) != 1) { // the original tests == 1, not merely non-zero
        return;
    }

    forward = decoded.forward;
    up = decoded.up;
    vector3d_cross_product(&cross, &up, &forward); // cross = forward x up
    vector3d_cross_product(&up, &forward, &cross); // up    = cross x forward
    vector3d_normalize_with_length(&forward);
    vector3d_normalize_with_length(&up);

    role_material = 0xffffffff;
    if (decoded.parent_hash != 0) {
        role_material = ((uint32_t *)object_network_id_table->handles)[decoded.parent_hash];
    }
    owner_material = 0xffffffff;
    if (decoded.owner_hash != 0) {
        owner_material = ((uint32_t *)player_network_id_table->handles)[decoded.owner_hash];
    }

    zero = (uint8_t *)&placement; // the `rep stos` of 0x22 dwords at 0x4bbecb
    for (i = 0; i < (int32_t)sizeof(placement); i++) {
        zero[i] = 0;
    }
    placement.definition_tag = decoded.definition_tag;
    placement.owner_linkage = owner_material;
    placement.role = role_material;
    placement.owner_team = decoded.name_index;
    placement.position = decoded.position;
    placement.forward = forward;
    placement.up = up;

    new_object_index = object_new_with_datum_role_control(&placement, 1);
    if (new_object_index == (datum_index)k_datum_index_none) {
        return;
    }

    network_index_cache_insert_if_free(decoded.object_hash);

    obj = ((object_header *)object_data->data)[new_object_index & 0xffff].data;
    ed = (equipment_data *)((uint8_t *)obj + k_item_extension_offset);

    obj->flags |= decoded.object_flags;
    ed->network_state.position = decoded.position;
    ed->network_state.velocity = decoded.velocity;
    ed->network_state.angular_velocity = decoded.angular_velocity;
    ed->network_baseline_index = decoded.baseline_index;
    ed->network_state_valid = 1;
    ed->network_sequence = 0;

    object_set_position_and_recalculate(&ed->network_state.position, new_object_index);

    obj->velocity = ed->network_state.velocity;
    obj->angular_velocity = ed->network_state.angular_velocity;
}

#if 0
Original Ghidra decompilation (0x4bbe20):

void FUN_004bbe20(void)

{
  char cVar1;
  undefined4 *in_EAX;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 *puVar6;
  undefined4 local_f0;
  int local_e8;
  int local_e4;
  uint local_e0;
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
  undefined4 local_a4;
  undefined1 local_a0;
  undefined1 local_98 [12];
  undefined4 local_8c [13];
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if (cVar1 == '\x01') {
      vector3d_cross_product(&local_d0);
      vector3d_cross_product(local_98);
      vector3d_normalize_with_length();
      vector3d_normalize_with_length();
      uVar5 = 0xffffffff;
      if (local_e4 != 0) {
        uVar5 = *(undefined4 *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_e4 * 4);
      }
      uVar4 = 0xffffffff;
      if (local_e8 != 0) {
        uVar4 = *(undefined4 *)(*(int *)(PTR_DAT_00687558 + 0x28) + local_e8 * 4);
      }
      puVar6 = local_8c;
      for (iVar3 = 0x22; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar6 = 0;
        puVar6 = puVar6 + 1;
      }
      local_58 = local_d0;
      local_4c = local_c4;
      local_54 = local_cc;
      local_50 = local_c8;
      local_48 = local_c0;
      local_44 = local_bc;
      local_8c[2] = uVar4;
      local_8c[3] = uVar5;
      uVar2 = object_new_with_datum_role_control(local_8c,1);
      if (uVar2 != 0xffffffff) {
        FUN_004e9cd0(local_f0);
        iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
        *(uint *)(iVar3 + 0x10) = *(uint *)(iVar3 + 0x10) | local_e0;
        *(undefined4 *)(iVar3 + 0x248) = local_dc;
        *(undefined4 *)(iVar3 + 0x24c) = local_d8;
        *(undefined4 *)(iVar3 + 0x250) = local_d4;
        *(undefined4 *)(iVar3 + 0x254) = local_b8;
        *(undefined4 *)(iVar3 + 600) = local_b4;
        *(undefined4 *)(iVar3 + 0x25c) = local_b0;
        *(undefined4 *)(iVar3 + 0x260) = local_ac;
        *(undefined4 *)(iVar3 + 0x264) = local_a8;
        *(undefined4 *)(iVar3 + 0x268) = local_a4;
        *(undefined1 *)(iVar3 + 0x245) = local_a0;
        *(undefined1 *)(iVar3 + 0x244) = 1;
        *(undefined1 *)(iVar3 + 0x246) = 0;
        object_set_position_and_recalculate();
        *(undefined4 *)(iVar3 + 0x68) = *(undefined4 *)(iVar3 + 0x254);
        *(undefined4 *)(iVar3 + 0x6c) = *(undefined4 *)(iVar3 + 600);
        *(undefined4 *)(iVar3 + 0x70) = *(undefined4 *)(iVar3 + 0x25c);
        *(undefined4 *)(iVar3 + 0x8c) = *(undefined4 *)(iVar3 + 0x260);
        *(undefined4 *)(iVar3 + 0x90) = *(undefined4 *)(iVar3 + 0x264);
        *(undefined4 *)(iVar3 + 0x94) = *(undefined4 *)(iVar3 + 0x268);
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
