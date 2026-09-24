// projectile_apply_network_update  (Ghidra: item_apply_network_update; renamed per
// out/phase4/projectiles_types_notes.md: "projectile row +0x70")
// address 0x4c1070, size 501 bytes
// name confidence: 0.8   rewrite confidence: 0.85 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: exact structural analog of src/items/weapon_apply_network_update.c (0x4c6070), one
//   register-only field group instead of weapon's (also carries ammo/age); types/projectiles.h
//   projectile_network_update_header (object_hash 0x00, baseline_index 0x04, sequence 0x05,
//   is_delta 0x06) and projectile_data.network_baseline_index/network_sequence/network_state
//   (0x27a/0x27b/0x27c), .last_update_valid/.last_update_state (0x294/0x298); types/objects.h
//   object.flags (0x010) _object_took_network_update_bit (0x8000000, "has taken a network
//   update", folded into types/objects.h this pass from out/phase4/projectiles_types_notes.md)
//   and _object_needs_cluster_update_bit (0x800); global 0x00696140
//   projectile_network_update_position_tolerance.
// register convention: item/projectile index in the first parameter; the incoming update record
//   pointer in the second. Both are already Ghidra-recognized __cdecl parameters.
// blam-cc: stack -> (projectile_index, update_record)
// UNSURE: `update_record` is a generic message-delta-system record; the pointer at +0x44
//   (update_record[0x11]) is the projectile_network_update_header, same double-indirection idiom
//   as weapon_apply_network_update.c.
// UNSURE: object 0x1c/0x48 are the interpolation block per
//   out/phase4/projectiles_types_notes.md's correction to types/objects.h (not yet folded into
//   that header), so they are written here as raw offsets rather than named fields, exactly as
//   weapon_apply_network_update.c already does for the same bytes.
// NOTE on the decode buffer (corrected against `objdump -d -M intel --start-address=0x4c1070
//   --stop-address=0x4c1270 bin/halo.exe`; Ghidra's hoisted loads make this read backwards, and
//   src/items/weapon_apply_network_update.c still carries the same misreading for 0x4c6070):
//   0x4c10ef..0x4c1122 copies projectile_data.network_state into a 0x18-byte LOCAL at
//   [esp+0x10], and 0x4c1127 `lea ecx,[esp+0x10]` then hands that local to the decoder as its
//   out-parameter. So the local is seeded with the old state (the delta baseline / the value
//   any field the message omits keeps) and comes back holding the DECODED state; everything
//   applied to the object afterwards -- velocity, the 0x1c/0x48 pair, last_update_state, and
//   the write-back into network_state under is_delta -- is the decoded state, NOT a restored
//   pre-decode decoded. The decoder additionally gets EDX = &projectile_data.network_state
//   (0x4c10ef `lea esi,[ebp+0x27c]`, 0x4c112f `mov edx,esi`) and EAX = update_record.
// UNSURE: vector3d_distance's two point arguments are register-only; ECX = &object.position
//   (0x4c11da `lea ecx,[ebp+0x5c]`), EAX = the decoded position (`lea eax,[esp+0x10]`).
// reconciled: R26 object +0x18/+0x1c/+0x44/+0x48 raw writes -> network_position_valid/network_position/network_velocity_valid/network_velocity

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data; // 0x008603b0
extern real projectile_network_update_position_tolerance; // 0x00696140

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern real vector3d_distance(real_point3d *a, real_point3d *b); // 0x4088b0, math module
extern int32_t message_delta_decode_compound_field_staged(void); // 0x4ec670, networking; drops the message
extern uint8_t message_delta_decode_compound_field_forced(void *out_state, void *baseline_state, uint32_t *update_record,
                            int32_t param); // 0x4ec600, networking; delta decode.
    // blam-cc: ECX -> out_state, EDX -> baseline_state, EAX -> update_record, stack -> 0
extern uint8_t message_delta_decode_compound_field(void *out_state, void *incoming_record); // 0x4ec590, networking;
    // decodes the message body into out_state.
    // blam-cc: ECX -> out_state, EAX -> incoming_record (0x4ec590 opens with
    // `mov edi,[eax]` and passes ECX straight through to 0x4ed1d0). This is the full/creation
    // decode; message_delta_decode_compound_field_forced below is the delta one.
extern void object_set_position_and_recalculate(real_point3d *position, datum_index object_index); // 0x4f52c0

void projectile_apply_network_update(datum_index projectile_index, uint32_t *update_record)
{
    object *obj;
    projectile_data *proj;
    projectile_network_update_header *header;
    projectile_network_state decoded;

    obj = object_try_and_get(projectile_index, _object_mask_projectile);
    if (obj == 0) {
        message_delta_decode_compound_field_staged();
        return;
    }
    proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
    header = (projectile_network_update_header *)update_record[0x11];

    if ((obj->flags & _object_took_network_update_bit) != 0 && *(int32_t *)update_record[0] == 1 &&
        (header->baseline_index != proj->network_baseline_index ||
         (header->sequence <= proj->network_sequence &&
          (int)((uint32_t)(header->sequence - proj->network_sequence) + 0xff) > 0x1d))) {
        message_delta_decode_compound_field_staged();
        return;
    }

    // seed the decode buffer with the current state, then decode into it -- see file header
    decoded = proj->network_state;

    {
        uint8_t accept = (*(int32_t *)update_record[0] == 1)
                             ? message_delta_decode_compound_field_forced(&decoded, &proj->network_state, update_record, 0)
                             : message_delta_decode_compound_field(&decoded, update_record);
        if (accept != 0) {
            proj->network_sequence = header->sequence;
            obj->flags |= _object_took_network_update_bit;

            if (header->is_delta != 0) {
                proj->network_baseline_index = header->baseline_index;
                proj->network_state = decoded; // the decoded state becomes the new baseline
            }

            obj->velocity = decoded.velocity;
            // the network interpolation block (object 0x018..0x054, objects.h R26)
            obj->network_position = decoded.position;
            obj->network_velocity = decoded.velocity;
            obj->network_position_valid = 1;
            obj->network_velocity_valid = 1;

            if ((*(int32_t *)update_record[0] != 1 ||
                 projectile_network_update_position_tolerance < vector3d_distance(&obj->position, &decoded.position)) &&
                (obj->flags & _object_needs_cluster_update_bit) != 0) {
                object_set_position_and_recalculate(&decoded.position, projectile_index);
            }

            proj->last_update_valid = 1;
            proj->last_update_state = decoded;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4c1070):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl item_apply_network_update(uint item_index,void *update_record)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  char cVar8;
  int iVar9;
  float10 fVar10;

  iVar9 = object_try_and_get(0x20);
  if (iVar9 == 0) {
    FUN_004ec670();
    return;
  }
  iVar1 = *(int *)((int)update_record + 0x44);
  if ((((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (item_index & 0xffff) * 0xc) + 0x10)
        & 0x8000000) != 0) && (**(int **)update_record == 1)) &&
     ((*(char *)(iVar1 + 4) != *(char *)(iVar9 + 0x27a) ||
      (((uint)*(byte *)(iVar1 + 5) <= (uint)*(byte *)(iVar9 + 0x27b) &&
       (0x1d < (int)(((uint)*(byte *)(iVar1 + 5) - (uint)*(byte *)(iVar9 + 0x27b)) + 0xff))))))) {
    FUN_004ec670();
    return;
  }
  uVar2 = *(undefined4 *)(iVar9 + 0x27c);
  uVar3 = *(undefined4 *)(iVar9 + 0x280);
  uVar4 = *(undefined4 *)(iVar9 + 0x284);
  uVar5 = *(undefined4 *)(iVar9 + 0x288);
  uVar6 = *(undefined4 *)(iVar9 + 0x28c);
  uVar7 = *(undefined4 *)(iVar9 + 0x290);
  if (**(int **)update_record == 1) {
    cVar8 = FUN_004ec600(0);
  }
  else {
    cVar8 = FUN_004ec590();
  }
  if (cVar8 != '\0') {
    *(undefined1 *)(iVar9 + 0x27b) = *(undefined1 *)(iVar1 + 5);
    *(uint *)(iVar9 + 0x10) = *(uint *)(iVar9 + 0x10) | 0x8000000;
    if (*(char *)(iVar1 + 6) != '\0') {
      *(undefined1 *)(iVar9 + 0x27a) = *(undefined1 *)(iVar1 + 4);
      *(undefined4 *)(iVar9 + 0x27c) = uVar2;
      *(undefined4 *)(iVar9 + 0x280) = uVar3;
      *(undefined4 *)(iVar9 + 0x284) = uVar4;
      *(undefined4 *)(iVar9 + 0x288) = uVar5;
      *(undefined4 *)(iVar9 + 0x28c) = uVar6;
      *(undefined4 *)(iVar9 + 0x290) = uVar7;
    }
    *(undefined4 *)(iVar9 + 0x68) = uVar5;
    *(undefined4 *)(iVar9 + 0x6c) = uVar6;
    *(undefined4 *)(iVar9 + 0x70) = uVar7;
    *(undefined4 *)(iVar9 + 0x1c) = uVar2;
    *(undefined4 *)(iVar9 + 0x20) = uVar3;
    *(undefined4 *)(iVar9 + 0x24) = uVar4;
    *(undefined4 *)(iVar9 + 0x48) = uVar5;
    *(undefined4 *)(iVar9 + 0x4c) = uVar6;
    *(undefined4 *)(iVar9 + 0x50) = uVar7;
    *(undefined1 *)(iVar9 + 0x18) = 1;
    *(undefined1 *)(iVar9 + 0x44) = 1;
    if (((**(int **)update_record != 1) ||
        (fVar10 = (float10)vector3d_distance(), (float10)_DAT_00696140 < fVar10)) &&
       ((*(uint *)(iVar9 + 0x10) & 0x800) != 0)) {
      object_set_position_and_recalculate();
    }
    *(undefined1 *)(iVar9 + 0x294) = 1;
    *(undefined4 *)(iVar9 + 0x298) = uVar2;
    *(undefined4 *)(iVar9 + 0x29c) = uVar3;
    *(undefined4 *)(iVar9 + 0x2a0) = uVar4;
    *(undefined4 *)(iVar9 + 0x2a4) = uVar5;
    *(undefined4 *)(iVar9 + 0x2a8) = uVar6;
    *(undefined4 *)(iVar9 + 0x2ac) = uVar7;
    return;
  }
  return;
}
#endif
