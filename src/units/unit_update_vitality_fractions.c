// unit_update_vitality_fractions  (Ghidra: unit_update_vitality_fractions)
// address 0x561b80, size 300 bytes
// name confidence: 0.2 (renamed from phase2's "unit_update_speed_scale_from_limits"; the body
//   only ever touches object.body_vitality/shield_vitality and their maximums, not any speed
//   or scale field)   rewrite confidence: 0.9 (VERIFIED against objdump)
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
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void object_set_health_frozen_flag(uint32_t object_index); // 0x4eda20, EAX
extern void object_set_shield_depleted_flag(uint32_t object_index); // 0x4edb10, EDI

// REWRITTEN from objdump 0x561b80..0x561cab: each fraction is the value over its maximum (maximum body
//   +0xd8, shield +0xdc), 1 when the value reaches the maximum, 0 when the maximum is not positive; a
//   positive current shield (+0xe4) or body (+0xe0) that the new fraction takes to zero (or below) first
//   calls object_set_shield_depleted_flag (EDI unit) / object_set_health_frozen_flag (EAX unit). The draft
//   called both with no unit and with the conditions inverted.
void unit_update_vitality_fractions(uint32_t unit_index, float body_delta, float shield_delta) // blam-cc: see file header
{
    object *obj;
    float shield_fraction;
    float body_fraction;

    if (unit_index == (uint32_t)-1) {
        return;
    }
    obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
        return;
    }
    if (!(obj->maximum_shield_vitality > 0.0f)) {
        shield_fraction = 0.0f;
    } else if (!(shield_delta < obj->maximum_shield_vitality)) {
        shield_fraction = 1.0f;
    } else {
        shield_fraction = shield_delta / obj->maximum_shield_vitality;
    }
    if (!(obj->maximum_body_vitality > 0.0f)) {
        body_fraction = 0.0f;
    } else if (!(body_delta < obj->maximum_body_vitality)) {
        body_fraction = 1.0f;
    } else {
        body_fraction = body_delta / obj->maximum_body_vitality;
    }

    if (obj->shield_vitality > 0.0f && !(shield_fraction > 0.0f)) {
        object_set_shield_depleted_flag(unit_index);
    }
    obj->shield_vitality = shield_fraction;
    if (obj->body_vitality > 0.0f && !(body_fraction > 0.0f)) {
        object_set_health_frozen_flag(unit_index);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
