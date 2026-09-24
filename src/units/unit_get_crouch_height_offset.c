// unit_get_crouch_height_offset  (Ghidra: unit_get_crouch_height_offset, renamed)
// address 0x55a2e0, size 173 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: types/tags.h Biped -- the three tag fields this reads are 0x424, 0x428 and 0x42c,
//   which are standing_collision_height, crouching_collision_height and collision_radius, NOT
//   the camera heights (those are 0x400 / 0x404) and NOT crouch_transition_time (0x408). The
//   offsets are pinned by the pad members that bracket them in types/tags.h and are asserted in
//   out/phase4/units_smoke.c. An earlier revision of this file used the camera-height names and
//   so read the wrong three fields; fixed in the phase-4 review pass.
//   The two outputs are the collision *pill*: its height and its radius. Both movement
//   integrators (biped_integrate_movement 0x55bea0 and
//   biped_integrate_movement_with_collision 0x55cfd0) put them straight into
//   biped_movement_solver_data.pill_height / .pill_radius and then subtract the radius back off
//   the solved Z when biped_flags bit 3 ("physics_pill_centered_at_origin") is clear, exactly
//   undoing the `position.z += collision_radius` below. A better name for this function would
//   be unit_get_collision_pill; the phase-2 name is kept because nine callers outside this
//   module still carry it.
//   types/units.h biped_data.crouch_fraction (0x50c); object_get_position (0x4f6900), used here
//   already named in the module.
// FIXED (register inputs, objdump): EAX carries the object_get_position output buffer (call at
//   0x55a314, then dereferenced at 0x55a328); the rewrite let object_get_position return a
//   pointer instead of taking it as a caller-supplied parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// object_get_position (0x4f6900, defined in src/objects/object_get_position.c) writes the
// object position through the pointer in EAX and leaves that same pointer in EAX on return;
// the object index is in ECX. The EAX output buffer is itself a caller-supplied parameter of
// this function (see FIXED note below).
extern void object_get_position(real_point3d *out_position, uint32_t object_index); // 0x4f6900, EAX->out, ECX->object_index

// Produces the biped's collision pill for this tick and lifts its origin onto the pill's
// centre: the object's own position gains one collision_radius unless biped_flags bit 3
// ("physics_pill_centered_at_origin") says the pill is already centred there. The pill height
// is the standing / crouching collision height blended by crouch_fraction, less the two
// hemispherical caps (twice the radius). A spherical biped (bit 4) that is neither
// player-controlled nor carrying object flag 0x400000 gets a zero-height pill, i.e. a plain
// sphere. The radius is echoed through the register-carried second output either way.
// blam-cc: EAX -> object_position, ECX -> object_index, EBX -> pill_radius_out
void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height, float *pill_radius_out)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    object_get_position(object_position, object_index);

    if ((tag->biped_flags & 8) == 0) {
        object_position->z += tag->collision_radius;
    }

    if ((tag->biped_flags & 0x10) == 0 &&
        (unit->controlling_player != k_datum_index_none || (obj->flags & 0x400000) != 0)) {
        *pill_height = ((tag->crouching_collision_height - tag->standing_collision_height) * biped->crouch_fraction +
                        tag->standing_collision_height) - (tag->collision_radius + tag->collision_radius);
        *pill_radius_out = tag->collision_radius;
        return;
    }
    *pill_height = 0.0f;
    *pill_radius_out = tag->collision_radius;
}

#if 0
Original Ghidra decompilation (0x55a2e0):

void FUN_0055a2e0(float *param_1)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  uint in_ECX;
  undefined4 *unaff_EBX;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar3 = object_get_position();
  if ((*(byte *)(iVar2 + 0x2f4) & 8) == 0) {
    *(float *)(iVar3 + 8) = *(float *)(iVar2 + 0x42c) + *(float *)(iVar3 + 8);
  }
  if (((*(byte *)(iVar2 + 0x2f4) & 0x10) == 0) &&
     ((puVar1[0x86] != 0xffffffff || ((puVar1[4] & 0x400000) != 0)))) {
    *param_1 = ((*(float *)(iVar2 + 0x428) - *(float *)(iVar2 + 0x424)) * (float)puVar1[0x143] +
               *(float *)(iVar2 + 0x424)) - (*(float *)(iVar2 + 0x42c) + *(float *)(iVar2 + 0x42c));
    *unaff_EBX = *(undefined4 *)(iVar2 + 0x42c);
    return;
  }
  *param_1 = 0.0;
  *unaff_EBX = *(undefined4 *)(iVar2 + 0x42c);
  return;
}
#endif
