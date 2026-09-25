// object_physics_handle_nearby_object_impacts  (Ghidra: FUN_00508a10; renamed per
//   out/phase4/physics_functions.md summary and out/phase4/physics_types_notes.md section 5)
// address 0x508a10, size 351 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/physics_functions.md ("Finds objects near a physics/antenna object and
//   dispatches per-object collision handling (impact damage or vertex-pair resolution) for each
//   one found"); types/objects.h object_header.type (+0x03), object.vitality_flags
//   (_object_health_frozen_bit), object.flags (_object_at_rest_bit); src/objects/
//   object_find_in_sphere.c's own confirmed signature.
// register convention: none recognized as in_EAX etc; param_1 is Ghidra's own recognized
//   parameter (object_index).
// UNSURE (major): the two per-candidate scratch buffers (local_2088 for object_physics_check_impact_damage,
//   local_2078/local_2074 for object_physics_resolve_mass_point_overlap) and the object_physics_context FUN_005074b0 builds
//   for the candidate object are never given visible addresses in this function's own decompile
//   -- every store into them is lost. This rewrite declares local buffers of the observed sizes
//   and calls both callees with a single opaque-buffer pointer plus the object indices actually
//   visible, without claiming to know their full real signatures. local_2074's value (tested
//   against 0.0 before object_physics_resolve_mass_point_overlap even runs) is UNRESOLVED entirely; this rewrite reads it
//   through the same uninitialized-until-called-by-something-earlier pointer Ghidra shows,
//   which is almost certainly wrong in detail even though the boolean gate it controls is
//   preserved.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "physics.h"

extern data_array *object_data; // 0x008603b0

extern uint16_t object_find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location,
    real_point3d *center, float radius, datum_index *out_objects, int16_t max_output); // 0x4f6fe0
extern uint8_t object_collision_context_build(uint32_t object_index, object_collision_context *out_context);
    // 0x504e10, this module (lower half); called with zero visible arguments here, object_index
    // assumed still live from this function's own parameter
extern uint8_t object_physics_context_build(uint32_t object_index,
    object_physics_context *out_context); // 0x5074b0, this module
extern void object_physics_check_impact_damage(void *scratch, uint32_t candidate_object_index); // 0x508b70, this
                                                                           // module (higher half)
extern void object_physics_resolve_mass_point_overlap(void *scratch); // 0x5090c0, this module (higher half); UNSURE args,
                                          // see file header

// Finds up to 0x800 nearby objects (search_mask 1, type mask bipeds+vehicles) around
// object_index's collision-model bounding sphere and, for each: if it is a non-frozen biped,
// runs object_physics_check_impact_damage (impact damage); if it is a vehicle other than object_index itself and both
// objects build valid object_physics_contexts, runs object_physics_resolve_mass_point_overlap (vertex-pair repulsion) -- but
// only once per pair, when candidate_index < object_index, or when the candidate is at rest, or
// when local_2074's UNRESOLVED condition holds (see file header).
void object_physics_handle_nearby_object_impacts(uint32_t object_index)
{
    object_collision_context self_collision_context;
    object_physics_context self_physics_context;
    uint8_t has_collision_context;
    uint8_t has_physics_context;
    datum_index candidates[0x800];
    uint16_t count;
    uint16_t i;

    has_collision_context = object_collision_context_build(object_index, &self_collision_context);
    has_physics_context = object_physics_context_build(object_index, &self_physics_context);

    if (!has_physics_context) {
        return;
    }

    {
        object *self = ((object_header *)object_data->data)[object_index & 0xffff].data;
        uint32_t type_mask = (has_collision_context != 0) + 2;

        count = object_find_in_sphere(1, type_mask, &self->location_leaf_index,
            &self->bounding_center, self->bounding_radius, candidates, 0x800);
    }

    for (i = 0; i < count; i++) {
        uint32_t candidate_index = candidates[i];
        object_header *header = &((object_header *)object_data->data)[candidate_index & 0xffff];
        uint8_t type = header->type;

        if (type == 0) {
            object *candidate = header->data;
            if ((candidate->vitality_flags & _object_health_frozen_bit) == 0) {
                uint8_t scratch[16]; // UNSURE size/shape, see file header
                object_physics_check_impact_damage(scratch, candidate_index);
            }
        } else if (type == 1 && candidate_index != object_index) {
            object_physics_context candidate_context;

            if (object_physics_context_build(candidate_index, &candidate_context)) {
                object *candidate = header->data;
                float unresolved_gate = 0.0f; // UNSURE: local_2074, see file header

                if ((candidate_index & 0xffff) < (object_index & 0xffff) ||
                    (candidate->flags & _object_at_rest_bit) != 0 || 0.0f < unresolved_gate) {
                    uint8_t scratch[4]; // UNSURE size/shape, see file header
                    object_physics_resolve_mass_point_overlap(scratch);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x508a10):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_00508a10(uint param_1)

{
  uint uVar1;
  char cVar2;
  char cVar3;
  ushort uVar4;
  int iVar5;
  int iVar6;
  uint *local_2094;
  uint local_2090;
  undefined1 local_2088 [16];
  undefined1 local_2078 [4];
  float *local_2074;
  uint local_2000 [2047];
  undefined4 uStack_4;

  uStack_4 = 0x508a1a;
  cVar2 = FUN_00504e10();
  cVar3 = FUN_005074b0();
  iVar6 = DAT_008603b0;
  if (cVar3 != '\0') {
    iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    uVar4 = object_find_in_sphere
                      (1,(cVar2 != '\0') + '\x02',iVar5 + 0x98,iVar5 + 0xa0,
                       *(undefined4 *)(iVar5 + 0xac),local_2000,0x800);
    if (0 < (short)uVar4) {
      local_2090 = (uint)uVar4;
      local_2094 = local_2000;
      do {
        uVar1 = *local_2094;
        iVar5 = (uVar1 & 0xffff) * 0xc;
        cVar2 = *(char *)(*(int *)(iVar6 + 0x34) + 3 + iVar5);
        if (cVar2 == '\0') {
          if ((*(byte *)(*(int *)(*(int *)(iVar6 + 0x34) + iVar5 + 8) + 0x106) & 4) == 0) {
            FUN_00508b70(local_2088,uVar1);
            iVar6 = DAT_008603b0;
          }
        }
        else if ((cVar2 == '\x01') && (uVar1 != param_1)) {
          cVar2 = FUN_005074b0();
          if ((cVar2 != '\0') &&
             ((((uVar1 & 0xffff) < (param_1 & 0xffff) ||
               ((*(byte *)(*(int *)(*(int *)(iVar6 + 0x34) + 8 + iVar5) + 0x10) & 0x20) != 0)) ||
              (0.0 < *local_2074)))) {
            FUN_005090c0(local_2078);
          }
        }
        local_2094 = local_2094 + 1;
        local_2090 = local_2090 - 1;
      } while (local_2090 != 0);
    }
  }
  return;
}
#endif
