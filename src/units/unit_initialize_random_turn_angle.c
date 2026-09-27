// unit_initialize_random_turn_angle  (Ghidra: FUN_00570650; renamed from the phase2 proposal)
// address 0x570650, size 207 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary)
// rewrite confidence: 0.4
// evidence: types/units.h unit_data.flags (0x204, bit 0x2000000 = _unit_flag_idle_turn_seeded),
//   .actor_index (0x1f4, decimal 500), .idle_turn_angle (0x414); types/objects.h object.forward
//   (0x074, i/j pair used with fpatan for a heading angle); math.h random_seed_global.
// register convention: unit object index in EAX (in_EAX).
//   // blam-cc: EAX -> object_index
// UNSURE: actor_resolve_wander_or_look_direction (an actor-side predicate with no traced arguments) is out of this
//   module's range.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern random_seed random_seed_global;   // 0x00719cd0

extern double atan2(double y, double x); // fpatan
extern uint8_t actor_resolve_wander_or_look_direction(datum_index actor_index, real_vector3d *out_direction); // 0x4287a0, EAX, ECX

// One-time initialization of the unit's random idle-turn target angle: seeded from its current
// heading (or zero, for an AI-controlled unit where actor_resolve_wander_or_look_direction applies) plus a small random
// offset, only run once per activation of the idle_turn_seeded flag.
void unit_initialize_random_turn_angle(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    float half_range;

    if ((unit->flags & _unit_flag_idle_turn_seeded) != 0) {
        return;
    }
    unit->flags |= _unit_flag_idle_turn_seeded;

    real_vector3d direction; // [esp+0x8], unused afterwards
    // 0x570690: EAX = the actor, ECX = a scratch direction; the draft passed neither
    if (unit->actor_index == k_datum_index_none || actor_resolve_wander_or_look_direction(unit->actor_index, &direction) == 0) {
        float angle = (float)atan2((double)obj->forward.j, (double)obj->forward.i);
        if (angle > 3.1415927f) {
            angle -= 6.2831855f;
        }
        unit->idle_turn_angle = angle;
        half_range = 1.7453293f;
    } else {
        unit->idle_turn_angle = 0.0f;
        half_range = 0.43633232f;
    }

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    unit->idle_turn_angle = (half_range - -half_range) * (float)(random_seed_global >> 0x10) *
                            1.5259022e-05f + -half_range + unit->idle_turn_angle;
}

#if 0
Original Ghidra decompilation (0x570650):

void FUN_00570650(void)

{
  int iVar1;
  float fVar2;
  char cVar3;
  uint in_EAX;
  float10 fVar4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((*(uint *)(iVar1 + 0x204) & 0x2000000) == 0) {
    *(uint *)(iVar1 + 0x204) = *(uint *)(iVar1 + 0x204) | 0x2000000;
    if ((*(int *)(iVar1 + 500) == -1) || (cVar3 = FUN_004287a0(), cVar3 == '\0')) {
      fVar4 = (float10)fpatan((float10)*(float *)(iVar1 + 0x78),(float10)*(float *)(iVar1 + 0x74));
      if ((float10)3.1415927 < fVar4) {
        fVar4 = fVar4 - (float10)6.2831855;
      }
      *(float *)(iVar1 + 0x414) = (float)fVar4;
      fVar2 = 1.7453293;
    }
    else {
      fVar2 = 0.43633232;
      *(undefined4 *)(iVar1 + 0x414) = 0;
    }
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    *(float *)(iVar1 + 0x414) =
         (fVar2 - -fVar2) * (float)(random_seed_global >> 0x10) * 1.5259022e-05 + -fVar2 +
         *(float *)(iVar1 + 0x414);
    return;
  }
  return;
}
#endif
