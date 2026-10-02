// player_compute_view_forward_vector  (Ghidra: FUN_00473d70; named per this rewrite)
// address 0x473d70, size 282 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x473d70..0x473e89 (EAX player, ECX yaw/pitch, ESI out).)
// evidence: types/units.h unit_data (vehicle_seat_index +0x2f0); types/tags.h Unit::seats
// (a TagReflexive of UnitSeat, matching src/game/game_engine_build_local_player_control_input.c's
// established `((UnitSeat *)parent_definition->seats.pointer)[unit->vehicle_seat_index]` idiom)
// and UnitSeatFlags bit 0x10 (third_person_camera); types/objects.h object (forward +0x74, up
// +0x80, parent_object +0x11c); src/math established vector3d_cross_product /
// vector3d_normalize_with_length / matrix4x3_from_forward_up / matrix4x3_transform_normal
// signatures. objdump -d -M intel --start-address=0x473d70 --stop-address=0x473e90 bin/halo.exe
// pins every register.
// register convention: EAX -> player_handle, ECX -> yaw_pitch (2 floats: yaw then pitch),
//   ESI -> out_forward.
//   // blam-cc: EAX -> player_handle, ECX -> yaw_pitch, ESI -> out_forward
//
// CORRECTED: this function's only caller in the image,
// game_engine_compute_local_player_look_vector (0x471f40, an earlier batch), passes
// `player_globals::local_player_units[local_player_index]` (a UNIT handle) in EAX. This
// function indexes player_data with EAX (`(EAX & 0xffff) * sizeof(player)`), so it wants a
// PLAYER handle -- `player_globals::local_players[local_player_index]` -- not a unit handle.
// That earlier file's own extern signature is also missing this function's third argument
// (out_forward, ESI) entirely. Both look like real bugs in that file, out of this batch's
// range; flagged for a follow-up pass rather than fixed here.
// UNSURE: 0x00696728 and 0x0069672c are pointer variables to constant vectors (0,-1,0) and
// (0,0,-1) respectively (read from bin/halo.exe's .data/.rdata); their broader identity/purpose
// elsewhere in the image is not established.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data;      // 0x0087a480
extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern real_vector3d *reference_axis_006696728; // 0x00696728 -> (0.0, -1.0, 0.0); UNSURE identity
extern real_vector3d *global_down3d_pointer;  // 0x0069672c -> (0.0, 0.0, -1.0); UNSURE identity

extern double fcos(double radians); // a single x87 FCOS instruction
extern double fsin(double radians); // a single x87 FSIN instruction
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand,
                                    real_vector3d *stack_operand); // 0x4052c0, out = stack_operand x ecx_operand
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place, vector in ECX
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward,
                                       real_matrix4x3 *out); // 0x4cb970, up in EAX, forward in ECX
extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal,
                                        real_matrix4x3 *m); // 0x4cbec0

// Computes a forward-facing unit vector from (yaw, pitch) into *out_forward. Then, if
// player_handle's unit is attached to a parent object (e.g. seated in a vehicle) and that
// parent's seat definition does not have the third_person_camera flag set, re-derives an
// orthonormal basis from the parent's up vector and out_forward (falling back to a second
// reference axis if the first choice of cross-product axis turns out to be degenerate), and
// transforms out_forward in place through that basis -- i.e. reorients the raw view vector into
// the vehicle's own coordinate frame for a first-person seat.
void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch, real_vector3d *out_forward)
    // blam-cc: EAX -> player_handle, ECX -> yaw_pitch, ESI -> out_forward
{
    player *plr;
    object *unit_obj;
    object *parent_obj;
    Unit *parent_definition;
    UnitSeat *seat;
    unit_data *unit;
    real_vector3d cross_result;
    real_matrix4x3 basis;
    real length;

    out_forward->i = (float)(fcos(yaw_pitch[1]) * fcos(yaw_pitch[0]));
    out_forward->j = (float)(fcos(yaw_pitch[1]) * fsin(yaw_pitch[0]));
    out_forward->k = (float)fsin(yaw_pitch[1]);

    plr = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    if (plr->unit == (datum_index)-1) {
        return;
    }

    unit_obj = ((object_header *)object_data->data)[plr->unit & 0xffff].data;
    if (unit_obj->parent_object == (datum_index)-1) {
        return;
    }

    parent_obj = object_try_and_get(unit_obj->parent_object, _object_mask_vehicle);
    if (parent_obj == (object *)0) {
        return;
    }

    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    parent_definition = (Unit *)tag_instances[parent_obj->definition_tag & 0xffff].data;
    seat = &((UnitSeat *)parent_definition->seats.pointer)[unit->vehicle_seat_index];
    if ((seat->flags & 0x10) != 0) { // third_person_camera
        return;
    }

    vector3d_cross_product(&cross_result, reference_axis_006696728, &parent_obj->up);
    length = vector3d_normalize_with_length(&cross_result);
    if (length == 0.0f) {
        vector3d_cross_product(&cross_result, global_down3d_pointer, &parent_obj->up);
        vector3d_normalize_with_length(&cross_result);
    }
    matrix4x3_from_forward_up(&parent_obj->up, &cross_result, &basis);
    matrix4x3_transform_normal(out_forward, out_forward, &basis);
}

#if 0
Original Ghidra decompilation (0x473d70), from tools/pack.py 0x473d70:

void FUN_00473d70(void)

{
  int iVar1;
  uint uVar2;
  uint in_EAX;
  uint *puVar3;
  float *in_ECX;
  float *unaff_ESI;
  float10 fVar4;
  float10 fVar5;
  undefined1 local_3c [56];

  fVar4 = (float10)fcos((float10)in_ECX[1]);
  iVar1 = *(int *)(DAT_0087a480 + 0x34);
  fVar5 = (float10)fcos((float10)*in_ECX);
  *unaff_ESI = (float)(fVar5 * fVar4);
  fVar5 = (float10)fsin((float10)*in_ECX);
  unaff_ESI[1] = (float)(fVar5 * fVar4);
  fVar4 = (float10)fsin((float10)in_ECX[1]);
  unaff_ESI[2] = (float)fVar4;
  uVar2 = *(uint *)((in_EAX & 0xffff) * 0x200 + iVar1 + 0x34);
  if ((uVar2 != 0xffffffff) &&
     (iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc),
     *(int *)(iVar1 + 0x11c) != -1)) {
    puVar3 = (uint *)object_try_and_get(2);
    if ((puVar3 != (uint *)0x0) &&
       ((*(byte *)(*(short *)(iVar1 + 0x2f0) * 0x11c +
                  *(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8)) &
        0x10) == 0)) {
      vector3d_cross_product(puVar3 + 0x20);
      fVar4 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 == fVar4) {
        vector3d_cross_product(puVar3 + 0x20);
        vector3d_normalize_with_length();
      }
      matrix4x3_from_forward_up(local_3c);
      matrix4x3_transform_normal(local_3c);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
