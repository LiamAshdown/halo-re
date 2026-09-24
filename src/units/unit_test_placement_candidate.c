// unit_test_placement_candidate  (Ghidra: unit_test_placement_candidate, renamed)
// address 0x55aa20, size 267 bytes
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: global_up3d indirect pointer 0x00696720 (see the ground-adjust cluster's notes);
//   matches functions.md's summary, "Tests a candidate position offset from a base point
//   against level collision and returns it if valid."
// register convention: a base position in ESI (the callee, object_get_position, writes its
//   result through it per this module's convention) and an output object-index pointer in EBX;
//   param_1/param_2 are Ghidra-recognized stack parameters.
//   // blam-cc: ESI -> base_position (in/out), EBX -> out_hit_object, stack -> distance, out_position
// UNSURE: object_get_position's own implicit output channel (modeled here as writing through
//   base_position directly, consistent with this rewrite's other callers of it); FUN_00502060's
//   exact signature (a line/ray collision test against DAT_00746f98, given six visible args).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern void *DAT_00746f98; // UNSURE global, passed straight through to FUN_00502060
extern real_vector3d *global_up3d_pointer; // 0x00696720

// object_get_position (0x4f6900, defined in src/objects/object_get_position.c) writes the
// object position through the pointer in EAX and leaves that same pointer in EAX on return;
// the object index is in ECX. Ghidra binds a different subset of the two operands at each call
// site in this module, so the declaration is left unprototyped.
extern real_point3d *object_get_position();
extern char FUN_00502060(void *context, uint32_t param_2, uint32_t param_3, real_point3d *start,
                          real_vector3d *delta, uint32_t max_distance_bits); // UNSURE signature

// Casts a short ray from 0.4 units above the unit's position along direction*distance, and if
// it hits something within range, writes the hit position (scaled back by the hit fraction)
// through out_position and the hit object/surface reference through out_hit_object.
char unit_test_placement_candidate(float distance, real_point3d *out_position, real_vector3d *direction,
                                    void **out_hit_object)
{
    real_point3d base_position;
    real_vector3d delta;
    char hit;
    float hit_fraction;
    void *hit_reference[3];

    object_get_position(&base_position);
    base_position.x += global_up3d_pointer->i * 0.4f;
    base_position.y += global_up3d_pointer->j * 0.4f;
    base_position.z += global_up3d_pointer->k * 0.4f;
    delta.i = distance * direction->i;
    delta.j = distance * direction->j;
    delta.k = distance * direction->k;

    hit = FUN_00502060(DAT_00746f98, 0, 0, &base_position, &delta, 0x7f7fffff);
    if (!hit) {
        return 0;
    }
    if (out_position != 0) {
        out_position->x = delta.i * hit_fraction + base_position.x;
        out_position->y = delta.j * hit_fraction + base_position.y;
        out_position->z = delta.k * hit_fraction + base_position.z;
    }
    if (out_hit_object != 0) {
        out_hit_object[0] = hit_reference[0];
        out_hit_object[1] = hit_reference[1];
        out_hit_object[2] = hit_reference[2];
    }
    return hit;
}

#if 0
Original Ghidra decompilation (0x55aa20):

undefined4 FUN_0055aa20(float param_1,float *param_2)

{
  char cVar1;
  undefined4 *unaff_EBX;
  float *unaff_ESI;
  float local_430;
  float local_42c;
  float local_428;
  float local_424;
  float local_420;
  float local_41c;
  float local_418;
  undefined4 *local_414;
  undefined4 local_410;

  object_get_position();
  local_430 = *(float *)PTR_DAT_00696720 * 0.4 + local_430;
  local_42c = *(float *)(PTR_DAT_00696720 + 4) * 0.4 + local_42c;
  local_428 = *(float *)(PTR_DAT_00696720 + 8) * 0.4 + local_428;
  local_424 = param_1 * *unaff_ESI;
  local_420 = param_1 * unaff_ESI[1];
  local_41c = param_1 * unaff_ESI[2];
  cVar1 = FUN_00502060(DAT_00746f98,0,0,&local_430,&local_424,0x7f7fffff);
  if (cVar1 == '\0') {
    local_410 = 0xffffffff;
  }
  else {
    if (param_2 != (float *)0x0) {
      *param_2 = local_424 * local_418 + local_430;
      param_2[1] = local_420 * local_418 + local_42c;
      param_2[2] = local_41c * local_418 + local_428;
    }
    if (unaff_EBX != (undefined4 *)0x0) {
      *unaff_EBX = *local_414;
      unaff_EBX[1] = local_414[1];
      unaff_EBX[2] = local_414[2];
      return local_410;
    }
  }
  return local_410;
}
#endif
