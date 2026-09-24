// unit_update_vitality_fractions  (Ghidra: unit_update_vitality_fractions)
// address 0x561b80, size 300 bytes
// name confidence: 0.2 (renamed from phase2's "unit_update_speed_scale_from_limits"; the body
//   only ever touches object.body_vitality/shield_vitality and their maximums, not any speed
//   or scale field)   rewrite confidence: 0.3
// evidence: types/objects.h object.maximum_body_vitality/maximum_shield_vitality (0xd8/0xdc),
//   .body_vitality/.shield_vitality (0xe0/0xe4). object_set_shield_depleted_flag,
//   object_set_health_frozen_flag.
// register convention: unit index in EAX, body damage fraction input in the first stack param,
//   shield damage fraction input in the second.
//   // blam-cc: in_EAX -> unit_index, param_1 -> body_delta, param_2 -> shield_delta
// UNSURE: `fVar < 0.0 == (fVar == 0.0)` is reproduced as `fVar > 0.0f`, the only value for
//   which that expression is true; same for the two `!= 0.0` simplifications below it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern void object_set_health_frozen_flag(void);  // 0x4eda20, UNSURE: no traced args  // real signature (object_set_health_frozen_flag.c): void object_set_health_frozen_flag(uint32_t object_index); Ghidra recovered 0 of 1 args at this call site
extern void object_set_shield_depleted_flag(void); // 0x4edb10, UNSURE: no traced args  // real signature (object_set_shield_depleted_flag.c): void object_set_shield_depleted_flag(uint32_t object_index); Ghidra recovered 0 of 1 args at this call site

void unit_update_vitality_fractions(uint32_t unit_index, float body_delta, float shield_delta) // blam-cc: see file header
{
    if (unit_index == (uint32_t)-1) {
        return;
    }
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
        return;
    }

    float shield_fraction = (obj->maximum_shield_vitality > 0.0f)
                                 ? ((shield_delta < obj->maximum_shield_vitality)
                                        ? shield_delta / obj->maximum_shield_vitality
                                        : 1.0f)
                                 : 0.0f;
    float body_fraction = (obj->maximum_body_vitality > 0.0f)
                               ? ((body_delta < obj->maximum_body_vitality)
                                      ? body_delta / obj->maximum_body_vitality
                                      : 1.0f)
                               : 0.0f;

    if (obj->shield_vitality > 0.0f && shield_fraction != 0.0f) {
        object_set_shield_depleted_flag();
    }
    obj->shield_vitality = shield_fraction;

    if (obj->body_vitality > 0.0f && body_fraction != 0.0f) {
        object_set_health_frozen_flag();
    }
    obj->body_vitality = body_fraction;
}

#if 0
Original Ghidra decompilation (0x561b80):

void FUN_00561b80(float param_1,float param_2)

{
  int iVar1;
  uint in_EAX;
  float local_8;
  float local_4;

  if ((in_EAX != 0xffffffff) &&
     (iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc),
     (*(byte *)(iVar1 + 0x106) & 4) == 0)) {
    if (*(float *)(iVar1 + 0xdc) < 0.0 == (*(float *)(iVar1 + 0xdc) == 0.0)) {
      if (param_2 < *(float *)(iVar1 + 0xdc)) {
        local_8 = param_2 / *(float *)(iVar1 + 0xdc);
      }
      else {
        local_8 = 1.0;
      }
    }
    else {
      local_8 = 0.0;
    }
    if (*(float *)(iVar1 + 0xd8) < 0.0 == (*(float *)(iVar1 + 0xd8) == 0.0)) {
      if (param_1 < *(float *)(iVar1 + 0xd8)) {
        local_4 = param_1 / *(float *)(iVar1 + 0xd8);
      }
      else {
        local_4 = 1.0;
      }
    }
    else {
      local_4 = 0.0;
    }
    if ((0.0 < *(float *)(iVar1 + 0xe4)) && (local_8 < 0.0 != (local_8 == 0.0))) {
      object_set_shield_depleted_flag();
    }
    *(float *)(iVar1 + 0xe4) = local_8;
    if ((0.0 < *(float *)(iVar1 + 0xe0)) && (local_4 < 0.0 != (local_4 == 0.0))) {
      object_set_health_frozen_flag();
    }
    *(float *)(iVar1 + 0xe0) = local_4;
  }
  return;
}
#endif
