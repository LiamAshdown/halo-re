// projectile_create_from_network  (Ghidra: FUN_004c0ca0; renamed per
// out/phase4/projectiles_types_notes.md: "receiver of 0x1e")
// address 0x4c0ca0, size 552 bytes
// name confidence: 0.7   rewrite confidence: 0.75 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: structural analog of src/items/equipment_create_from_creation_message.c (0x4bbe20);
//   types/projectiles.h projectile_creation_message is exactly the 0x54-byte record decoded
//   here; types/objects.h object_placement_data (size 0x88, the 0x22 dwords the `rep stos`
//   clears; flags bit 0x02 "gates the connect-to-map step", set here -- equipment's sibling
//   function does not set it); types/projectiles.h projectile_data network block at
//   0x27c..0x27b.
// register convention: the incoming record in EAX.
// blam-cc: EAX -> incoming_record
// UNSURE: same caveat as equipment_create_from_creation_message.c -- message_delta_decode_compound_field decodes
// straight into an untyped stack buffer that Ghidra never types as a struct, so every field
// this function stores decompiles as a bare `local_*`. The mapping below follows the exact same
// per-field arithmetic (frame offset = message offset, up to a constant) that the equipment
// sibling function used, and reuses its already-established externs. Unlike that sibling,
// object_placement_data.definition_tag/name_index/position are never visibly written in this
// pack -- they fall inside the same zeroed 0x22-dword buffer Ghidra never re-touches with an
// explicit store here, so they are reconstructed by analogy with the equipment function (which
// resolved the equivalent stores from disassembly) rather than confirmed independently.
// UNSURE: the forward/up basis is re-orthogonalized rather than used as decoded -- two cross
// products, then both vectors normalized -- exactly as in the equipment sibling.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data; // 0x008603b0
extern void *object_pooled_node_globals_006870d8; // 0x006870d8, the object hash/pooled-node
    // globals block; same global src/objects/object_delete_by_pooled_node_id.c uses
extern uint8_t *object_pooled_node_globals; // 0x00687130, +0x28 off it is the hash -> datum_index
    // array that resolves projectile_creation_message.creating_object_hash into
    // object_placement_data.role
extern uint8_t *network_message_table_b; // 0x00687558, same shape; resolves
    // projectile_creation_message.owner_hash into object_placement_data.owner_linkage

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0, out = stack_operand x ecx_operand
extern uint8_t message_delta_decode_compound_field(void *out_state, void *incoming_record); // 0x4ec590, networking;
    // decodes the message body into out_state.
    // blam-cc: ECX -> out_state, EAX -> incoming_record (0x4ec590 opens with
    // `mov edi,[eax]` and passes ECX straight through to 0x4ed1d0)
extern int32_t message_delta_decode_compound_field_staged(void); // 0x4ec670, networking; drops the message
extern void network_index_cache_insert_if_free(void *pooled_node_globals, datum_index object_index,
                         int32_t object_hash); // 0x4e9cd0, networking: binds the new object to
    // the hash it was announced under. blam-cc: EAX -> pooled_node_globals (the literal
    // 0x006870d8, `mov eax,0x6870d8` at 0x4c0e20), ECX -> object_index, stack -> object_hash.
    // Sibling of the FUN_004e9d40(globals, object_index) call in
    // src/objects/object_delete_by_pooled_node_id.c.
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0
extern void object_set_position_and_recalculate(real_point3d *position, datum_index object_index); // 0x4f52c0

// Handles an incoming network projectile-creation update: decodes the message, re-orthonormalizes
// the replicated forward/up basis, resolves the creating-object and owner hashes into object
// handles, spawns the projectile through object_new_with_datum_role_control (with the
// connect-to-map placement flag set), registers it in the networking hash table, and then seeds
// its network block, position and velocity.
void projectile_create_from_network(void *incoming_record)
{
    projectile_creation_message decoded;
    real_vector3d forward, up, cross;
    object_placement_data placement;
    uint32_t role_material, owner_material;
    datum_index new_object_index;
    object *obj;
    projectile_data *proj;
    uint8_t *zero;
    int32_t i;

    // The mode word sits behind two indirections; nonzero means "not a create for us".
    if (*(int32_t *)*(int32_t **)incoming_record != 0) {
        message_delta_decode_compound_field_staged();
        return;
    }
    if (message_delta_decode_compound_field(&decoded, incoming_record) != 1) { // the original tests == 1, not merely non-zero
        return;
    }

    forward = decoded.forward;
    up = decoded.up;
    vector3d_cross_product(&cross, &up, &forward); // cross = forward x up
    vector3d_cross_product(&up, &forward, &cross); // up    = cross x forward
    vector3d_normalize_with_length(&forward);
    vector3d_normalize_with_length(&up);

    role_material = 0xffffffff;
    if (decoded.creating_object_hash != 0) {
        role_material = (*(uint32_t **)(object_pooled_node_globals + 0x28))[decoded.creating_object_hash];
    }
    owner_material = 0xffffffff;
    if (decoded.owner_hash != 0) {
        owner_material = (*(uint32_t **)(network_message_table_b + 0x28))[decoded.owner_hash];
    }

    zero = (uint8_t *)&placement; // the `rep stos` of 0x22 dwords
    for (i = 0; i < (int32_t)sizeof(placement); i++) {
        zero[i] = 0;
    }
    placement.definition_tag = decoded.definition_tag; // UNSURE, see file header
    placement.name_index = decoded.name_index;          // UNSURE, see file header
    placement.position = decoded.position;              // UNSURE, see file header
    placement.forward = forward;
    placement.up = up;
    placement.angular_velocity = decoded.angular_velocity;
    placement.flags |= 0x02; // connect-to-map; not set by the equipment sibling
    placement.owner_linkage = owner_material;
    placement.role = role_material;

    new_object_index = object_new_with_datum_role_control(&placement, 1);
    if (new_object_index == (datum_index)k_datum_index_none) {
        return;
    }

    network_index_cache_insert_if_free(object_pooled_node_globals_006870d8, new_object_index, decoded.object_hash);

    obj = ((object_header *)object_data->data)[new_object_index & 0xffff].data;
    proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);

    proj->network_state.position = decoded.position;
    proj->network_state.velocity = decoded.velocity;
    proj->network_baseline_index = decoded.baseline_index;
    proj->network_state_valid = 1;
    proj->network_sequence = 0;

    object_set_position_and_recalculate(&proj->network_state.position, new_object_index);

    obj->velocity = proj->network_state.velocity;
}

#if 0
Original Ghidra decompilation (0x4c0ca0):

void FUN_004c0ca0(void)

{
  char cVar1;
  undefined4 *in_EAX;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  uint *puVar6;
  undefined4 local_f0;
  int local_e8;
  int local_e4;
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
  uint local_8c [13];
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if (cVar1 == '\x01') {
      vector3d_cross_product(&local_d4);
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
      local_58 = local_d4;
      local_4c = local_c8;
      local_54 = local_d0;
      local_50 = local_cc;
      local_40 = local_b0;
      local_48 = local_c4;
      local_44 = local_c0;
      local_8c[1] = local_8c[1] | 2;
      local_3c = local_ac;
      local_38 = local_a8;
      local_8c[2] = uVar4;
      local_8c[3] = uVar5;
      uVar2 = object_new_with_datum_role_control(local_8c,1);
      if (uVar2 != 0xffffffff) {
        FUN_004e9cd0(local_f0);
        iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
        *(undefined4 *)(iVar3 + 0x27c) = local_e0;
        *(undefined4 *)(iVar3 + 0x280) = local_dc;
        *(undefined4 *)(iVar3 + 0x284) = local_d8;
        *(undefined4 *)(iVar3 + 0x288) = local_bc;
        *(undefined4 *)(iVar3 + 0x28c) = local_b8;
        *(undefined4 *)(iVar3 + 0x290) = local_b4;
        *(undefined1 *)(iVar3 + 0x27a) = local_a4;
        *(undefined1 *)(iVar3 + 0x279) = 1;
        *(undefined1 *)(iVar3 + 0x27b) = 0;
        object_set_position_and_recalculate();
        *(undefined4 *)(iVar3 + 0x68) = *(undefined4 *)(iVar3 + 0x288);
        *(undefined4 *)(iVar3 + 0x6c) = *(undefined4 *)(iVar3 + 0x28c);
        *(undefined4 *)(iVar3 + 0x70) = *(undefined4 *)(iVar3 + 0x290);
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
