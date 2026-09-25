// actor_evaluate_grenade_target_position  (Ghidra: actor_evaluate_grenade_target_position, renamed)
// address 0x40de70, size 485 bytes
// name confidence: 0.35   rewrite confidence: 0.25
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

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, vector in ECX

extern uint8_t unit_is_in_busy_animation_state(datum_index actor_index); // UNSURE: no visible arg at this call site
extern uint8_t actor_probe_step_direction(datum_index actor_index); // UNSURE: no visible arg
extern uint8_t unit_scripted_action_animation_exists(datum_index actor_index); // UNSURE: no visible arg
extern uint8_t actor_queue_secondary_action(int32_t request_kind, real_vector2d *direction);

// blam-cc: EBX -> actor_index
uint8_t actor_evaluate_grenade_target_position(datum_index actor_index)
{
    actor *self;
    prop *target_prop;
    object_header *unit_header;
    object *unit_obj;
    void *unit_tag_data;
    Actor *actor_def;
    real_vector2d dir;
    float dot;
    uint8_t result;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    result = 0;

    if (self->active_unit_index != (datum_index)k_datum_index_none) return 0;
    if (self->secondary_action != -1) return 0;
    if (self->unit_index != (datum_index)k_datum_index_none && unit_is_in_busy_animation_state(self->unit_index) != 0) return 0;
    if (self->unknown_504 != 0) return 0;
    if (self->target_unit_index == (datum_index)k_datum_index_none) return 0;

    target_prop = (prop *)((uint8_t *)prop_data->data + (self->target_unit_index & 0xffff) * sizeof(prop));

    unit_header = (object_header *)object_data->data + (self->unit_index & 0xffff);
    unit_obj = unit_header->data;
    unit_tag_data = tag_instances[unit_obj->definition_tag & 0xffff].data;
    if (*(float *)((uint8_t *)unit_tag_data + 0x234) <= 0.0f) return 0;

    actor_def = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;

    if ((actor_def->flags & 0x200000) == 0) {
        // not flying: normalize the target-relative direction before dotting it with facing
        real_vector2d n;
        n.i = target_prop->unknown_e0.x;
        n.j = target_prop->unknown_e0.y;
        if (vector2d_normalize_with_length(&n) <= 0.0f) {
            goto build_direction;
        }
        dot = n.i * self->facing.i + n.j * self->facing.j;
    } else {
        dot = target_prop->unknown_e0.y * self->facing.j + target_prop->unknown_e0.z * self->facing.k;
        dot = target_prop->unknown_e0.x * self->facing.i + dot;
    }
    if (dot <= 0.4f) return 0;

build_direction:
    dir.i = target_prop->unknown_e0.x;
    dir.j = target_prop->unknown_e0.y;
    vector2d_normalize_with_length(&dir);
    if (actor_probe_step_direction(actor_index) != 0 && unit_scripted_action_animation_exists(actor_index) != 0) {
        result = actor_queue_secondary_action(6, &dir);
    }
    return result;
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
