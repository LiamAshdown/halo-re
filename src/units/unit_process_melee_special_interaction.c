// unit_process_melee_special_interaction  (Ghidra: FUN_0056ff40; renamed from the phase2
//   proposal)
// address 0x56ff40, size 512 bytes
// name confidence: 0.3 (phase2 proposal at 0.3, matches functions.md summary)
// rewrite confidence: 0.2
// evidence: types/tags.h UnitFlags bit 0x1000 = impact_melee_attaches_to_unit, bit 0x2000 =
//   impact_melee_dies_on_shields, bit 0x400000 = shields_fry_infection_forms (the attacker's
//   own tag flags, at absolute tag offset 0x17c = Unit.unit_flags); types/objects.h object.type
//   (0x0b4), .shield_vitality (0x0e4), .vitality_flags (0x106), .parent_object (0x11c),
//   .flags (0x010), .up (0x080); types/units.h unit_data.flags (0x204); callees
//   unit_cause_melee_damage (0x56f2d0, this batch), object_set_health_frozen_flag,
//   object_delete, vector3d_cross_product (0x4052c0, established elsewhere in this module).
// register convention: attacking unit index in EAX (in_EAX), target unit index on the stack
//   (param_1).
//   // blam-cc: EAX -> attacker_index, stack -> target_index
// UNSURE: the three zero-argument calls in the first branch (unit_cause_melee_damage,
//   object_set_health_frozen_flag, object_delete) are guessed to act on the target and the
//   attacker respectively, matching the "attacker dies from touching a shielded target that
//   fries infection forms" reading of the flag combination.
// UNSURE: the orthonormal-basis rebuild (the two vector3d_cross_product/normalize pairs) and the
//   final object_set_position_and_relink / object_attach_to_object calls are register-only in
//   the decompile; the basis construction is reproduced with raw offsets into the attacker
//   object rather than named fields, since it is only partially legible.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern real_vector3d *global_up3d_pointer; // 0x00696720

extern void unit_cause_melee_damage(uint32_t unit_index, uint8_t suppress_effect,
    uint32_t target_object_index, int16_t p4, int16_t p5, int16_t p6, uint32_t p7); // 0x56f2d0
extern void object_set_health_frozen_flag(uint32_t object_index); // 0x4eda20, UNSURE signature
extern void object_delete(uint32_t object_index); // 0x4f5bd0, UNSURE exact signature
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand,
                                    real_vector3d *stack_operand); // 0x4052c0
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index); // 0x4f5350
extern void object_attach_to_object(uint32_t parent_index, uint32_t child_index,
                                     uint32_t marker_word); // 0x4f6440
extern void unit_try_ready_weapon(int32_t a, int32_t b); // 0x569a20, sets melee_state per other files' evidence  // real signature (unit_try_ready_weapon.c): uint8_t unit_try_ready_weapon(uint32_t unit_index, uint8_t is_melee, int32_t fire_trigger_event); Ghidra recovered 2 of 3 args at this call site

// Handles special melee interactions between two units based on the attacker's own tag flags:
// impact_melee_dies_on_shields (kills the attacker and freezes/melees the target when the
// target is a shielded biped whose own tag fries infection forms), or
// impact_melee_attaches_to_unit (climbs the target's vehicle-parent chain and, if it reaches a
// distinct root, repositions and attaches the attacker onto it).
void unit_process_melee_special_interaction(uint32_t attacker_index, uint32_t target_index)
{
    object *attacker = ((object_header *)object_data->data)[attacker_index & 0xffff].data;
    UnitFlags attacker_flags = ((Unit *)tag_instances[attacker->definition_tag & 0xffff].data)->unit_flags;
    object *target = ((object_header *)object_data->data)[target_index & 0xffff].data;

    if ((attacker_flags & 0x2000) != 0 && target->type == 0 && target->shield_vitality > 0.0f &&
        (((Unit *)tag_instances[target->definition_tag & 0xffff].data)->unit_flags & 0x400000) != 0) {
        unit_cause_melee_damage(target_index, 0, 0xffffffff, -1, -1, -1, 0);
        object_set_health_frozen_flag(attacker_index);
        object_delete(attacker_index);
        return;
    }

    if ((attacker_flags & 0x1000) != 0 && (1 << (target->type & 0x1f) & 3) != 0 &&
        (target->vitality_flags & 4) == 0) {
        uint32_t chain = target->parent_object;

        if (chain != k_datum_index_none) {
            do {
                object *link = ((object_header *)object_data->data)[chain & 0xffff].data;
                if (chain == attacker_index) {
                    return;
                }
                if (link->type != 1) {
                    return;
                }
                chain = link->parent_object;
            } while (chain != k_datum_index_none);
        }

        {
            // UNSURE: this rebuilds attacker->up (0x080) and attacker->forward (0x074) from
            // the target's position/forward via the world-up constant, mirroring the
            // orthonormal-basis idiom used elsewhere; the exact registers are not recoverable.
            real_vector3d *up = (real_vector3d *)((uint8_t *)attacker + 0x80);
            real_vector3d *forward = (real_vector3d *)((uint8_t *)attacker + 0x74);
            real_point3d target_forward_negated;

            *up = *global_up3d_pointer;
            target_forward_negated.x = -target->forward.i;
            target_forward_negated.y = -target->forward.j;
            target_forward_negated.z = -target->forward.k;
            *forward = *(real_vector3d *)&target_forward_negated;

            vector3d_cross_product(up, forward, up);
            if (vector3d_normalize_with_length(up) == 0.0f) {
                *up = *global_up3d_pointer;
                vector3d_normalize_with_length(up);
            }
            vector3d_cross_product(forward, up, forward);

            object_set_position_and_relink(&target->position, attacker_index); // UNSURE: position source
            object_attach_to_object(target_index, attacker_index, 0); // UNSURE: marker word
            attacker->flags |= 0x20;
            {
                unit_data *attacker_unit = (unit_data *)((uint8_t *)attacker + k_unit_data_offset);
                attacker_unit->flags |= 0x8000;
            }
            unit_try_ready_weapon(1, 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x56ff40):

void FUN_0056ff40(uint param_1)

{
  float *pfVar1;
  uint *puVar2;
  uint *puVar3;
  int iVar4;
  undefined *puVar5;
  uint in_EAX;
  uint uVar6;
  float10 fVar7;
  float *in_stack_00000018;
  undefined4 in_stack_0000001c;

  puVar5 = PTR_DAT_00696714;
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar6 = *(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x17c);
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (((((uVar6 & 0x2000) != 0) && ((short)puVar3[0x2d] == 0)) && (0.0 < (float)puVar3[0x39])) &&
     ((*(uint *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x17c) & 0x400000) != 0
     )) {
    unit_cause_melee_damage();
    object_set_health_frozen_flag();
    object_delete();
    return;
  }
  if ((((uVar6 & 0x1000) != 0) && ((1 << ((byte)puVar3[0x2d] & 0x1f) & 3U) != 0)) &&
     ((*(byte *)((int)puVar3 + 0x106) & 4) == 0)) {
    uVar6 = puVar3[0x47];
    if (uVar6 != 0xffffffff) {
      do {
        iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
        if (uVar6 == in_EAX) {
          return;
        }
        if (*(short *)(iVar4 + 0xb4) != 1) {
          return;
        }
        uVar6 = *(uint *)(iVar4 + 0x11c);
      } while (uVar6 != 0xffffffff);
    }
    puVar2[0x1a] = *(uint *)PTR_DAT_00696714;
    puVar2[0x1b] = *(uint *)(puVar5 + 4);
    puVar2[0x1c] = *(uint *)(puVar5 + 8);
    puVar2[0x23] = *(uint *)puVar5;
    puVar2[0x24] = *(uint *)(puVar5 + 4);
    puVar2[0x25] = *(uint *)(puVar5 + 8);
    pfVar1 = (float *)(puVar2 + 0x1d);
    *pfVar1 = *in_stack_00000018;
    puVar2[0x1e] = (uint)in_stack_00000018[1];
    puVar2[0x1f] = (uint)in_stack_00000018[2];
    *pfVar1 = -*pfVar1;
    puVar2[0x1e] = (uint)-(float)puVar2[0x1e];
    puVar2[0x1f] = (uint)-(float)puVar2[0x1f];
    vector3d_cross_product(puVar2 + 0x20);
    fVar7 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar7) {
      vector3d_cross_product(PTR_DAT_00696720);
      vector3d_normalize_with_length();
    }
    vector3d_cross_product(pfVar1);
    object_set_position_and_relink(in_stack_0000001c);
    object_attach_to_object(param_1);
    puVar2[4] = puVar2[4] | 0x20;
    puVar2[0x81] = puVar2[0x81] | 0x8000;
    FUN_00569a20(1,0);
  }
  return;
}
#endif
