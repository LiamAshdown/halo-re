// unit_project_onto_aiming_axis  (Ghidra: FUN_005658f0; named for this rewrite)
// address 0x5658f0, size 208 bytes (0x5658f0..0x5659bf, single `ret` at 0x5659bf)
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: objdump -d -M intel 0x5658f0..0x5659c0 (the whole body; Ghidra dropped both register
//   parameters and both callees' register arguments).
//   - 0x5658f0..0x565914: object_data (0x008603b0) header lookup of the stack `unit_index`
//     (stride 0xc, object pointer at +8).
//   - 0x56591b..0x56592f: when `use_unit_aiming_vector` is set, copies object + 0x23c..0x244 into
//     the EBX vector; types/units.h names object + 0x23c unit_data.aiming_vector ("the current
//     aim").
//   - 0x56593a..0x56598c: when `project_point` is set, calls unit_get_camera_position (0x568f80,
//     ECX = unit_index, EDI = &camera, matching src/units/unit_get_camera_position.c's
//     blam-cc) and replaces the EAX point with camera + ((point - camera) . axis) * axis, i.e.
//     the point's projection onto the line through the unit's camera along `axis`.
//   - 0x56598f..0x5659ba: object_get_root_object_velocities (0x4f6aa0, EAX = unit_index,
//     ESI = &velocity, EDI = 0 so no angular velocity) and writes velocity . axis to *out_speed:
//     how fast the unit's root object (the unit, or the vehicle it rides) moves along the axis.
//   The only caller is trigger_create_projectiles (0x4c4edd), inside the autoaim block that
//   src/items/trigger_create_projectiles.c has not ported: it passes the autoaim target as
//   `unit_index`, a weapon-tag flag bit (tag + 0x17c, bit 3) as `project_point`, and a byte that
//   is cleared when the target has a gunner or is an actor in state 4 as
//   `use_unit_aiming_vector`. That is why this lives in units: it reads unit_data and calls a
//   units function, it is not a pure math helper as units/README.md's skip list guessed.
// UNSURE: the name. What the caller does with the projected point and the axial speed (lead the
//   shot? bias the spread?) is inside that unported block, so the name only describes the
//   mechanics.
// register convention (prologue: `mov esi,eax` at 0x565906 and EBX dereferenced at 0x565922 with
//   no prior write; caller 0x4c4ed5/0x4c4ed9 loads `lea ebx,[esp+0x48]` and `lea eax,[esp+0x24]`
//   immediately before the call, and pops 0x10 bytes of stack arguments):
//   // blam-cc: stack -> (unit_index, out_speed, project_point, use_unit_aiming_vector),
//   //          EAX -> point (in/out), EBX -> axis (in/out)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern void unit_get_camera_position(datum_index unit_index, real_point3d *out_position); // 0x568f80, blam-cc: ECX unit_index, EDI out
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity); // 0x4f6aa0, blam-cc: EAX object_index, ESI out_velocity, EDI out_angular_velocity

// Optionally replaces `axis` with the unit's current aiming vector, optionally projects `point`
// onto the line through the unit's camera along `axis`, and returns in *out_speed the component
// of the unit's root-object velocity along `axis`.
void unit_project_onto_aiming_axis(datum_index unit_index, real *out_speed, uint8_t project_point,
    uint8_t use_unit_aiming_vector, real_point3d *point, real_vector3d *axis) // blam-cc: EAX point, EBX axis
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    real_point3d camera;
    real_vector3d velocity;
    real distance;

    if (use_unit_aiming_vector) {
        *axis = ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->aiming_vector;
    }

    if (project_point) {
        unit_get_camera_position(unit_index, &camera);
        distance = (point->x - camera.x) * axis->i + (point->z - camera.z) * axis->k +
            (point->y - camera.y) * axis->j;
        point->x = distance * axis->i + camera.x;
        point->y = distance * axis->j + camera.y;
        point->z = distance * axis->k + camera.z;
    }

    object_get_root_object_velocities(unit_index, &velocity, (real_vector3d *)0);
    *out_speed = velocity.j * axis->j + velocity.k * axis->k + velocity.i * axis->i;
}

#if 0
Original Ghidra decompilation (0x5658f0):

void FUN_005658f0(uint param_1,float *param_2,char param_3,char param_4)

{
  int iVar1;
  float fVar2;
  float *in_EAX;
  float *unaff_EBX;
  float local_c;
  float local_8;
  float local_4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (param_4 != '\0') {
    *unaff_EBX = *(float *)(iVar1 + 0x23c);
    unaff_EBX[1] = *(float *)(iVar1 + 0x240);
    unaff_EBX[2] = *(float *)(iVar1 + 0x244);
  }
  if (param_3 != '\0') {
    unit_get_camera_position();
    fVar2 = (in_EAX[1] - local_8) * unaff_EBX[1] +
            (in_EAX[2] - local_4) * unaff_EBX[2] + (*in_EAX - local_c) * *unaff_EBX;
    *in_EAX = fVar2 * *unaff_EBX + local_c;
    in_EAX[1] = fVar2 * unaff_EBX[1] + local_8;
    in_EAX[2] = fVar2 * unaff_EBX[2] + local_4;
  }
  FUN_004f6aa0();
  *param_2 = local_c * *unaff_EBX + local_4 * unaff_EBX[2] + local_8 * unaff_EBX[1];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
