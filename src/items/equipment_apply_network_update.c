// equipment_apply_network_update  (Ghidra: missed_4bc250, created by hand this pass -- Ghidra
// never recovered it as a function; only reachable through the equipment object_type_definition
// row)
// address 0x4bc250, size 461 bytes
// name confidence: 0.65   rewrite confidence: 0.75
// evidence: structural analog of src/projectiles/projectile_apply_network_update.c (0x4c1070)
//   and src/items/weapon_apply_network_update.c (0x4c6070), confirmed instruction by instruction
//   against `objdump -d -M intel --start-address=0x4bc250 --stop-address=0x4bc420 bin/halo.exe`;
//   the equipment row (0x0069b810) carries this address at +0x70, the same column that holds
//   projectile_apply_network_update at +0x70 on the projectile row. types/items.h
//   equipment_data (network_baseline_index 0x245, network_sequence 0x246, network_state 0x248,
//   last_update_valid 0x26c, last_update_state 0x270 -- this function is the proof for both of
//   those two UNSURE fields), equipment_network_state (0x24 bytes: position/velocity/
//   angular_velocity), weapon_network_update_header (reused here: same {..., baseline_index+4,
//   sequence+5, force_baseline+6} shape reached through update_record[0x11], already folded into
//   this module's header out of weapon_apply_network_update.c). types/objects.h
//   object.velocity/position, object_flags (_object_at_rest_bit 0x20,
//   _object_took_network_update_bit 0x8000000).
// register convention: item index in ECX at entry (feeds object_try_and_get's hidden slot
//   directly, `mov ecx,[esp+4]` before the call); the incoming update record is the second stack
//   parameter, kept live in EAX across the whole function. blam-cc matches weapon/projectile's
//   own apply_network_update siblings.
// blam-cc: stack -> (item_index, update_record)
// Cleanup-pass review (objdump 0x4bc250..0x4bc41c): the draft dropped the angular_velocity
//   store (object+0x8c from decoded+0x18); added.
// UNSURE: `update_record` is a generic message-delta-system record belonging to the networking
//   module, outside this batch's address range; see weapon_apply_network_update.c for the same
//   double-indirection mode-word idiom (*(int *)update_record[0]).
// UNSURE: object+0x1c / object+0x48 are the interpolation-block position/velocity pair
//   (out/phase4/projectiles_types_notes.md's correction to types/objects.h, not yet folded into
//   that header); written here as raw offsets, exactly as the weapon and projectile siblings do.
// UNSURE: global 0x00695e18 (2.0f) is this type's position-tolerance constant, named here to
//   match weapon_network_update_position_tolerance (0x696550) and
//   projectile_network_update_position_tolerance (0x696140); not folded into types/items.h.
// UNSURE: the relink condition is "moved farther than tolerance, OR the item is currently at
//   rest" -- confirmed against the objdump FPU compare/branch pair (fcomp/fnstsw/test ah,0x41),
//   not copied from the weapon/projectile siblings, which gate on a different bit
//   (_object_needs_cluster_update_bit) instead of _object_at_rest_bit.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"

extern real equipment_network_update_position_tolerance; // 0x00695e18, 2.0f; UNSURE name, by
    // analogy with weapon_network_update_position_tolerance (0x696550) and
    // projectile_network_update_position_tolerance (0x696140)
extern double sqrt(double x); // declared locally, as in weapon_apply_network_update.c: -I types
    // shadows <math.h> with types/math.h

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0,
    // blam-cc: ECX -> object_index, stack -> type_mask
extern int32_t message_delta_decode_compound_field_staged(void); // 0x4ec670, networking; drops the message
extern uint8_t message_delta_decode_compound_field_forced(void *out_state, void *baseline_state, uint32_t *update_record,
                            int32_t param); // 0x4ec600, networking; delta decode.
    // blam-cc: ECX -> out_state, EDX -> baseline_state, EAX -> update_record, stack -> 0
extern uint8_t message_delta_decode_compound_field(void *out_state, uint32_t *update_record); // 0x4ec590, networking;
    // full/creation decode. blam-cc: ECX -> out_state, EAX -> update_record
extern void object_set_position_and_recalculate(real_point3d *position, datum_index object_index); // 0x4f52c0,
    // blam-cc: ESI -> position, EDI -> object_index

void equipment_apply_network_update(datum_index item_index, uint32_t *update_record)
{
    object *obj;
    equipment_data *ed;
    weapon_network_update_header *header;
    equipment_network_state decoded;
    real dx, dy, dz;

    obj = object_try_and_get(item_index, _object_mask_equipment);
    if (obj == 0) {
        message_delta_decode_compound_field_staged();
        return;
    }
    ed = (equipment_data *)((uint8_t *)obj + k_item_extension_offset);
    header = (weapon_network_update_header *)update_record[0x11];

    if ((obj->flags & _object_took_network_update_bit) != 0 && *(int32_t *)update_record[0] == 1 &&
        (header->baseline_index != ed->network_baseline_index ||
         (header->sequence <= ed->network_sequence &&
          (int)((uint32_t)(header->sequence - ed->network_sequence) + 0xff) > 0x1d))) {
        message_delta_decode_compound_field_staged();
        return;
    }

    // seed the decode buffer with the current baseline, then decode into it
    decoded = ed->network_state;

    {
        uint8_t accept = (*(int32_t *)update_record[0] == 1)
                             ? message_delta_decode_compound_field_forced(&decoded, &ed->network_state, update_record, 0)
                             : message_delta_decode_compound_field(&decoded, update_record);
        if (accept != 0) {
            ed->network_sequence = header->sequence;
            obj->flags |= _object_took_network_update_bit;

            if (header->force_baseline != 0) {
                ed->network_baseline_index = header->baseline_index;
                ed->network_state = decoded; // the decoded state becomes the new baseline
            }

            obj->velocity = decoded.velocity;
            obj->angular_velocity = decoded.angular_velocity; // 0x4bc35d..0x4bc370
            // UNSURE: these two writes land inside types/objects.h's still-unresolved
            // unknown_019[7]/player_visibility_mask/unknown_022[0x3a] region (object 0x018..0x05c),
            // the interpolation position/velocity pair the projectiles pass identified.
            *(real_point3d *)((uint8_t *)obj + 0x1c) = decoded.position;
            *(real_vector3d *)((uint8_t *)obj + 0x48) = decoded.velocity;
            obj->unknown_018 = 1;
            *((uint8_t *)obj + 0x44) = 1;

            dx = decoded.position.x - obj->position.x;
            dy = decoded.position.y - obj->position.y;
            dz = decoded.position.z - obj->position.z;
            if (equipment_network_update_position_tolerance < (real)sqrt(dx * dx + dy * dy + dz * dz) ||
                (obj->flags & _object_at_rest_bit) != 0) {
                object_set_position_and_recalculate(&decoded.position, item_index);
            }

            ed->last_update_valid = 1;
            ed->last_update_state = decoded;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4bc250):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void missed_4bc250(uint param_1,undefined4 *param_2)

{
  char cVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  float *pfVar5;
  float *pfVar6;
  float local_24 [4];
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar2 = object_try_and_get(8);
  if (iVar2 == 0) {
    message_delta_decode_compound_field_staged();
    return;
  }
  iVar4 = param_2[0x11];
  if ((((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0x10) &
        0x8000000) != 0) && (*(int *)*param_2 == 1)) &&
     ((*(char *)(iVar4 + 4) != *(char *)(iVar2 + 0x245) ||
      (((uint)*(byte *)(iVar4 + 5) <= (uint)*(byte *)(iVar2 + 0x246) &&
       (0x1d < (int)(((uint)*(byte *)(iVar4 + 5) - (uint)*(byte *)(iVar2 + 0x246)) + 0xff))))))) {
    message_delta_decode_compound_field_staged();
    return;
  }
  pfVar5 = (float *)(iVar2 + 0x248);
  pfVar6 = local_24;
  for (iVar3 = 9; iVar3 != 0; iVar3 = iVar3 + -1) {
    *pfVar6 = *pfVar5;
    pfVar5 = pfVar5 + 1;
    pfVar6 = pfVar6 + 1;
  }
  if (*(int *)*param_2 == 1) {
    cVar1 = message_delta_decode_compound_field_forced(0);
  }
  else {
    cVar1 = message_delta_decode_compound_field();
  }
  if (cVar1 != '\0') {
    *(undefined1 *)(iVar2 + 0x246) = *(undefined1 *)(iVar4 + 5);
    *(uint *)(iVar2 + 0x10) = *(uint *)(iVar2 + 0x10) | 0x8000000;
    if (*(char *)(iVar4 + 6) != '\0') {
      *(undefined1 *)(iVar2 + 0x245) = *(undefined1 *)(iVar4 + 4);
      pfVar5 = local_24;
      pfVar6 = (float *)(iVar2 + 0x248);
      for (iVar4 = 9; iVar4 != 0; iVar4 = iVar4 + -1) {
        *pfVar6 = *pfVar5;
        pfVar5 = pfVar5 + 1;
        pfVar6 = pfVar6 + 1;
      }
    }
    *(float *)(iVar2 + 0x68) = local_24[3];
    *(undefined4 *)(iVar2 + 0x6c) = local_14;
    *(undefined4 *)(iVar2 + 0x70) = local_10;
    *(undefined4 *)(iVar2 + 0x8c) = local_c;
    *(undefined4 *)(iVar2 + 0x90) = local_8;
    *(undefined4 *)(iVar2 + 0x94) = local_4;
    *(float *)(iVar2 + 0x1c) = local_24[0];
    *(float *)(iVar2 + 0x20) = local_24[1];
    *(float *)(iVar2 + 0x24) = local_24[2];
    *(float *)(iVar2 + 0x48) = local_24[3];
    *(undefined4 *)(iVar2 + 0x4c) = local_14;
    *(undefined4 *)(iVar2 + 0x50) = local_10;
    *(undefined1 *)(iVar2 + 0x18) = 1;
    *(undefined1 *)(iVar2 + 0x44) = 1;
    local_24[0] = local_24[0] - *(float *)(iVar2 + 0x5c);
    local_24[1] = local_24[1] - *(float *)(iVar2 + 0x60);
    local_24[2] = local_24[2] - *(float *)(iVar2 + 100);
    if ((_DAT_00695e18 <
         SQRT(local_24[1] * local_24[1] + local_24[0] * local_24[0] + local_24[2] * local_24[2])) ||
       ((*(byte *)(iVar2 + 0x10) & 0x20) != 0)) {
      object_set_position_and_recalculate();
    }
    *(undefined1 *)(iVar2 + 0x26c) = 1;
    pfVar5 = local_24;
    pfVar6 = (float *)(iVar2 + 0x270);
    for (iVar4 = 9; iVar4 != 0; iVar4 = iVar4 + -1) {
      *pfVar6 = *pfVar5;
      pfVar5 = pfVar5 + 1;
      pfVar6 = pfVar6 + 1;
    }
  }
  return;
}
#endif
