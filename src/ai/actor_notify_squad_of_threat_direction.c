// actor_notify_squad_of_threat_direction  (Ghidra: actor_notify_squad_of_threat_direction, renamed)
// address 0x4234f0, size 269 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/ai.h actor.unit_index (0x18), actor.aim_origin (0x120), actor.
//   awareness_level (0x6a); types/tags.h Actor.surprise_distance (0x2b0). Calls
//   vector3d_normalize_with_length (0x401990), actor_record_look_at_point (0x421bc0),
//   actor_queue_search_position (0x421af0) and ai_communication_broadcast (0x42d340), all
//   already established in this module. Ghidra's own pseudocode drops a THIRD input
//   entirely -- the point pointer -- which objdump (bin/halo.exe 0x4234f0..0x4235ef) shows
//   arrives in EBX and is read nowhere else in the decompile; the two stack shorts it does
//   show (param_1, param_2) are otherwise accurate.
// register convention: EBX -> point (real_point3d*), EDI -> actor_index, stack -> event_kind,
//   grenade_type_code.
//   // blam-cc: EBX -> point, EDI -> actor_index, stack -> event_kind, grenade_type_code

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void actor_record_look_at_point(datum_index actor_index, const uint32_t *point, int16_t priority, uint32_t data); // 0x421bc0
extern void actor_queue_search_position(datum_index actor_index, real_point3d *position, int16_t priority,
                                        real_vector3d *velocity, uint32_t unknown_324, uint32_t unknown_328,
                                        uint32_t unknown_33c, uint32_t unknown_340, uint32_t unknown_344,
                                        uint8_t unknown_348); // 0x421af0
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340

// blam-cc: EBX -> point, EDI -> actor_index, stack -> event_kind, grenade_type_code
// If the actor controls a unit and event_kind is 2, broadcasts a category-0xb squad event
// with grenade_type_code (0/1/2) remapped to a descending severity code (3/2/1). Either way,
// if the actor controls a unit, records a look-at point toward `point` (at priority 4, only
// when event_kind is 2 and the actor is alert enough and within surprise range) and
// unconditionally queues a matching priority-4 search position with no duration.
void actor_notify_squad_of_threat_direction(const real_point3d *point, datum_index actor_index,
                                            int16_t event_kind, int16_t grenade_type_code)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);
    datum_index unit_index = self->unit_index;

    if (unit_index == (datum_index)k_datum_index_none) {
        return;
    }

    if (event_kind == 2) {
        int32_t severity = -1;
        if (grenade_type_code == 0) severity = 3;
        else if (grenade_type_code == 1) severity = 2;
        else if (grenade_type_code == 2) severity = 1;
        ai_communication_broadcast(0xb, unit_index, (datum_index)k_datum_index_none, severity,
                                   (datum_index)k_datum_index_none, (datum_index)k_datum_index_none, 0);
    }

    {
        real_vector3d direction;
        float length;

        direction.i = point->x - self->aim_origin.x;
        direction.j = point->y - self->aim_origin.y;
        direction.k = point->z - self->aim_origin.z;
        length = vector3d_normalize_with_length(&direction);

        if (self->awareness_level < 3 && length < actor_tag->surprise_distance && event_kind == 2) {
            actor_record_look_at_point(actor_index, (const uint32_t *)&direction, 4, 0xffffffff);
        }
        actor_queue_search_position(actor_index, 0, 4, &direction, 0xffffffff, 0, 0, 0xffffffff, 0, 0);
    }
}

#if 0
Original Ghidra decompilation (0x4234f0):

void FUN_004234f0(short param_1,short param_2)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  uint unaff_EDI;
  float10 fVar5;

  iVar4 = (unaff_EDI & 0xffff) * 0x724;
  iVar1 = *(int *)(iVar4 + 0x18 + *(int *)(DAT_00880360 + 0x34));
  iVar4 = iVar4 + *(int *)(DAT_00880360 + 0x34);
  iVar2 = *(int *)((*(uint *)(iVar4 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (iVar1 != -1) {
    if (param_1 == 2) {
      uVar3 = 0xffffffff;
      if (param_2 == 0) {
        uVar3 = 3;
      }
      else if (param_2 == 1) {
        uVar3 = 2;
      }
      else if (param_2 == 2) {
        uVar3 = 1;
      }
      ai_communication_broadcast(0xb,iVar1,0xffffffff,uVar3,0xffffffff,0xffffffff,0);
    }
    fVar5 = (float10)vector3d_normalize_with_length();
    if (((*(short *)(iVar4 + 0x6a) < 3) && (fVar5 < (float10)*(float *)(iVar2 + 0x2b0))) &&
       (param_1 == 2)) {
      FUN_00421bc0(0xffffffff);
    }
    FUN_00421af0(0xffffffff,0,0,0xffffffff,0,0);
  }
  return;
}

Ground truth from objdump (bin/halo.exe @ 0x4234f0..0x4235ef) recovering EBX (the point
pointer) and the exact register setup for both truncated calls; see file header.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
