// unit_process_melee_special_interaction  (Ghidra: FUN_0056ff40; renamed from the phase2 proposal)
// address 0x56ff40, size 610 bytes
// name confidence: 0.3 (phase2 proposal at 0.3, matches functions.md summary)
// rewrite confidence: 0.9
// REWRITTEN from objdump 0x56ff40..0x5701a1 (the whole function; 0x570140 "unit_detach_from_parent" is its tail).
//   EAX: attacker, stack: (target, node word pair, region word pair, material dword, contact point, contact plane,
//   contact leaf) as the biped lunge trace (0x55dfbe) builds them. A melee that dies on shields (Unit flag 0x2000,
//   an infection form) against a shielded biped whose tag fries infection forms (0x400000) deals its melee damage
//   and deletes the attacker. One that attaches (0x1000) to a biped or vehicle (not dying, not already carrying the
//   attacker through a vehicle chain) stops, faces into the contact plane (forward = -normal, left from forward x up,
//   falling back to global up then global forward), moves to the contact point in its leaf, attaches to the target at
//   the struck node, is flagged (object 0x20, unit 0x8000) and readies its weapon.
// blam-cc: EAX -> attacker_index, stack -> target_index, node_pair, region_pair, material, contact_point,
//   contact_plane, contact_leaf

// FIXED 2026-09-28: global_origin3d_pointer here is the global at its address comment, global_zero_vector3d_pointer (the name belonged to another
// global at a different address, so the link bound it there).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern real_vector3d *global_origin3d_pointer; // 0x00696714
extern real_vector3d *global_forward3d_pointer;     // 0x00696718
extern real_vector3d *global_up3d_pointer;          // 0x00696720

extern void unit_cause_melee_damage(uint32_t unit_index, uint8_t suppress_effect, uint32_t target_object_index,
    int16_t damage_param4, int16_t damage_param5, int16_t damage_param6, uint32_t damage_param7); // 0x56f2d0
extern void object_set_health_frozen_flag(uint32_t object_index); // 0x4eda20, EAX
extern void object_delete(uint32_t object_index); // 0x4f5bd0, EAX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index,
                                           bsp_leaf_reference *location); // 0x4f5350, ESI, EDI, stack
extern void object_attach_to_object(uint32_t parent_index, uint32_t child_index, int16_t marker_index); // 0x4f6440
extern uint8_t unit_try_ready_weapon(uint32_t unit_index, uint8_t forced, const real_vector2d *direction); // 0x569a20, EDI, stack

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

void unit_process_melee_special_interaction(uint32_t attacker_index, uint32_t target_index, uint32_t node_pair,
                                            uint32_t region_pair, uint32_t material, real_point3d *contact_point,
                                            real_plane3d *contact_plane, bsp_leaf_reference *contact_leaf)
{
    uint8_t *attacker = OBJECT_DATA(attacker_index);
    uint32_t unit_flags = *(uint32_t *)(TAG_DATA(*(datum_index *)attacker) + 0x17c);
    uint8_t *target = OBJECT_DATA(target_index);

    if ((unit_flags & 0x2000) && ((struct object *)target)->type == 0 && ((struct object *)target)->shield_vitality > 0.0f &&
        (*(uint32_t *)(TAG_DATA(*(datum_index *)target) + 0x17c) & 0x400000)) {
        // 0x56ffc6: an infection form touching a frying shield
        unit_cause_melee_damage(attacker_index, 1, target_index, (int16_t)node_pair, (int16_t)region_pair,
                                (int16_t)material, (uint32_t)contact_plane);
        object_set_health_frozen_flag(attacker_index);
        object_delete(attacker_index);
        return;
    }
    if (!(unit_flags & 0x1000) || !((1u << (target[0xb4] & 0x1f)) & 3) || (target[0x106] & 4)) {
        return;
    }
    {
        datum_index parent = ((struct object *)target)->parent_object;

        while (parent != k_datum_index_none) {
            uint8_t *p = OBJECT_DATA(parent);

            if (parent == attacker_index || ((struct object *)p)->type != 1) {
                return;
            }
            parent = ((struct object *)p)->parent_object;
        }
    }
    {
        real_vector3d *forward = (real_vector3d *)(attacker + 0x74);
        real_vector3d *up = (real_vector3d *)(attacker + 0x80);
        real_vector3d left;

        *(real_vector3d *)&((struct object *)attacker)->velocity.i = *global_origin3d_pointer;
        *(real_vector3d *)&((struct object *)attacker)->angular_velocity.i = *global_origin3d_pointer;
        *forward = contact_plane->normal;
        forward->i = -forward->i;
        forward->j = -forward->j;
        forward->k = -forward->k;
        vector3d_cross_product(&left, forward, up);
        if (vector3d_normalize_with_length(&left) == 0.0f) {
            vector3d_cross_product(&left, forward, global_up3d_pointer);
            if (vector3d_normalize_with_length(&left) == 0.0f) {
                left = *global_forward3d_pointer;
            }
        }
        vector3d_cross_product(up, &left, forward);
    }
    object_set_position_and_relink(contact_point, attacker_index, contact_leaf);
    object_attach_to_object(target_index, attacker_index, (int16_t)node_pair);
    ((struct object *)attacker)->flags |= 0x20;
    *(uint32_t *)(attacker + 0x204) |= 0x8000;
    unit_try_ready_weapon(attacker_index, 1, 0);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
