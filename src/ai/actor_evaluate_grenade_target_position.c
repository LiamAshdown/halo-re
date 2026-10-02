// actor_evaluate_grenade_target_position  (Ghidra: actor_evaluate_grenade_target_position, renamed)
// address 0x40de70, size 485 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: phase-4 summary "evaluates whether the actor's current threat is positioned
// well enough to grenade and, if so, requests a grenade throw toward it"; ends by calling
// actor_queue_secondary_action(6, &direction), a movement/aim request kind.
// register convention: actor_index in EBX (Ghidra's unaff_EBX).
// blam-cc: EBX -> actor_index
// UNSURE: the float at (Unit tag data)+0x234 is not named in types/tags.h's Unit struct
// (no offset comments there); kept as a raw offset.
// UNSURE: as in actor_check_grenade_facing_and_commit (0x40db00), the vector2d_normalize
// calls here take a hidden pointer to a local 2D vector built from prop.unknown_e0.{x,y},
// which Ghidra's re-display of the pre-call expression obscures; reconstructed explicitly
// below rather than transcribed literally.
// UNSURE: actor_probe_step_direction and FUN_00569470 are called with zero visible arguments; treated as
// taking actor_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX
extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index); // 0x569c90, ECX
extern uint8_t actor_probe_step_direction(datum_index actor_index, float step_distance, real_vector2d *direction,
    uint16_t *variant, float step_up, uint8_t *out_flag, void *extra_param); // 0x417e50, stack, ECX
extern uint8_t unit_scripted_action_animation_exists(uint32_t unit_index, int16_t command); // 0x569470, EAX, ECX
extern uint8_t actor_queue_secondary_action(datum_index actor_index, int16_t action, uint32_t payload[2]); // 0x417a60, EAX, stack

// REWRITTEN from objdump 0x40de70..0x40e054 (misnamed: the actor sidesteps out of its target's line of fire).
//   EBX: actor. On foot (+0x158), without a pending special (+0x418), not busy animating, not +0x504, with a
//   target (+0x270) and a unit that can step (tag +0x234): when the target aims at the actor (its aim +0xe0 . our
//   facing > 0.4, in 3D with Actor flag 0x200000, else flat), probe a sideways step across the aim (0x417e50) and
//   queue dodge 7 (side 1) or 6 as the secondary action when the unit has it. Returns whether queued.
// blam-cc: EBX -> actor_index
uint8_t actor_evaluate_grenade_target_position(datum_index actor_index)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t queued = 0;         // [esp+0xe]
    uint8_t *unit_tag;
    uint8_t *actor_tag;
    uint8_t *p;
    float *facing = (float *)(a + 0x174);

    if (((actor *)a)->active_unit_index != k_datum_index_none || ((actor *)a)->secondary_action != -1) {
        return 0;
    }
    if (((actor *)a)->unit_index != k_datum_index_none && unit_is_in_busy_animation_state(((actor *)a)->unit_index)) {
        return 0;
    }
    if (a[0x504] || ((actor *)a)->target_unit_index == k_datum_index_none) {
        return 0;
    }
    unit_tag = (uint8_t *)tag_instances[*(datum_index *)((uint8_t *)((object_header *)object_data->data)
        [((actor *)a)->unit_index & 0xffff].data) & 0xffff].data;
    p = (uint8_t *)prop_data->data + (((actor *)a)->target_unit_index & 0xffff) * 0x138;
    if (!(*(float *)(unit_tag + 0x234) > 0.0f)) {
        return 0;
    }
    actor_tag = (uint8_t *)tag_instances[((actor *)a)->actor_definition_tag & 0xffff].data;
    if (*(uint32_t *)actor_tag & 0x200000) {
        float dot = *(float *)(p + 0xe8) * facing[2] + *(float *)(p + 0xe4) * facing[1] + *(float *)(p + 0xe0) * facing[0];

        if (!(dot > 0.4f)) {
            return 0;
        }
    } else {
        real_vector2d flat;     // [esp+0x10]

        flat.i = *(float *)(p + 0xe0);
        flat.j = *(float *)(p + 0xe4);
        if (vector2d_normalize_with_length(&flat) > 0.0f && !(flat.j * facing[1] + flat.i * facing[0] > 0.4f)) {
            return 0;
        }
    }
    {
        real_vector2d direction;    // [esp+0x18]
        uint16_t side = 4;          // [esp+0x10]
        uint8_t flag;               // [esp+0xf]
        float extra[4];             // [esp+0x20]
        int16_t action;

        direction.i = *(float *)(p + 0xe0);
        direction.j = *(float *)(p + 0xe4);
        vector2d_normalize_with_length(&direction);
        if (!actor_probe_step_direction(actor_index, *(float *)(unit_tag + 0x234), &direction, &side, 0.0f, &flag, extra)) {
            return 0;
        }
        action = (int16_t)side == 1 ? 7 : 6;
        if (unit_scripted_action_animation_exists(((actor *)a)->unit_index, action)) {
            queued = actor_queue_secondary_action(actor_index, action, (uint32_t *)&direction);
        }
    }
    return queued;
}

#if 0
Original Ghidra decompilation (0x40de70):

undefined1 FUN_0040de70(void)

{
  int iVar1;
  float fVar2;
  float fVar3;
  char cVar4;
  int iVar5;
  uint unaff_EBX;
  float10 fVar6;
  undefined1 local_2e;
  undefined4 local_24;
  undefined4 local_20;

  iVar1 = (unaff_EBX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  local_2e = 0;
  if (*(int *)(iVar1 + 0x158) != -1) {
    return 0;
  }
  if (*(short *)(iVar1 + 0x418) != -1) {
    return 0;
  }
  if ((*(int *)(iVar1 + 0x18) != -1) && (cVar4 = FUN_00569c90(), cVar4 != '\0')) {
    return 0;
  }
  if (*(char *)(iVar1 + 0x504) != '\0') {
    return 0;
  }
  if (*(uint *)(iVar1 + 0x270) == 0xffffffff) {
    return 0;
  }
  iVar5 = (*(uint *)(iVar1 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  if (*(float *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                      (*(uint *)(iVar1 + 0x18) & 0xffff) * 0xc) & 0xffff) * 0x20 +
                          0x14 + DAT_0087bc14) + 0x234) <= 0.0) {
    return 0;
  }
  if ((**(uint **)((*(uint *)(iVar1 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) & 0x200000) == 0
     ) {
    fVar2 = *(float *)(iVar5 + 0xe0);
    fVar3 = *(float *)(iVar5 + 0xe4);
    fVar6 = (float10)vector2d_normalize_with_length();
    if (fVar6 <= (float10)0.0) goto LAB_0040dfc9;
    fVar3 = fVar3 * *(float *)(iVar1 + 0x178);
  }
  else {
    fVar3 = *(float *)(iVar5 + 0xe4) * *(float *)(iVar1 + 0x178) +
            *(float *)(iVar5 + 0xe8) * *(float *)(iVar1 + 0x17c);
    fVar2 = *(float *)(iVar5 + 0xe0);
  }
  if (fVar2 * *(float *)(iVar1 + 0x174) + fVar3 <= 0.4) {
    return 0;
  }
LAB_0040dfc9:
  local_24 = *(undefined4 *)(iVar5 + 0xe0);
  local_20 = *(undefined4 *)(iVar5 + 0xe4);
  vector2d_normalize_with_length();
  cVar4 = FUN_00417e50();
  if ((cVar4 != '\0') && (cVar4 = FUN_00569470(), cVar4 != '\0')) {
    local_2e = FUN_00417a60(6,&local_24);
  }
  return local_2e;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
