// hs_object_angle_predicate_helper  (Ghidra: FUN_004878f0)
// address 0x4878f0, size 180 bytes
// name confidence: 0.35 (out/phase4/hs_functions.md: "Per-object predicate helper that converts
//   an angle argument from degrees to radians and forwards it to a lower-level geometry test,
//   gated by a context validity check")
// rewrite confidence: 0.4
// evidence: the degrees-to-radians multiply (0.017453292 == pi/180) and the final call's result
//   being this function's own return value.
// register convention: object index in EAX (in_EAX); param_1 unused directly (forwarded into the
//   discarded FUN_004f6080 call, register-implicit); angle in degrees as the recognized stack
//   parameter (param_2).
//   // blam-cc: EAX -> object_index, stack -> (param_1, angle_degrees)
// UNSURE: FUN_004f6080's arguments here are entirely register-implicit and not recoverable from
//   the decompile (unlike hs_object_detach_and_place_at_location's call to the same address,
//   which does have explicit stack arguments) -- its return value is discarded here, only its
//   side effect matters, and neither could be determined. unit_point_within_look_cone's own semantics (the
//   "lower-level geometry test") are likewise not recovered; only that it takes the angle in
//   radians and its result becomes this function's return value.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern void *object_try_and_get(int32_t type_mask); // objects module, 0x4f6ec0
extern int16_t object_get_node_local_transform(datum_index object_index, void *marker_index, void *out_buffer,
    char param_4); // UNSURE: this call site's args are register-implicit; the signature is
    // the one hs_effect_spawn_on_marker.c and hs_object_detach_and_place_at_location.c prove;
    // objects
                                  // module, 0x4f6080 (compare the 4-argument call site in
                                  // hs_object_detach_and_place_at_location.c)
extern uint32_t unit_point_within_look_cone(float angle_radians); // units(?) module, 0x56c100

// Gated by `object_index` being valid: if the object can be fetched as a type-3 control (see
// object_try_and_get), performs an unrecovered side effect via FUN_004f6080, then always converts
// `angle_degrees` to radians and returns the result of the geometry test unit_point_within_look_cone. Returns 0
// (well, `object_index`'s garbage upper bytes with a zero low byte, simplified to a plain 0 here
// since only the low byte is ever a meaningful boolean) when `object_index` is
// k_datum_index_none.
uint32_t hs_object_angle_predicate_helper(datum_index object_index, void *param_1, float angle_degrees)
{
    void *control;

    if (object_index == k_datum_index_none) {
        return 0;
    }
    control = object_try_and_get(3);
    if (control != 0) {
        /* UNSURE: all four arguments are register-implicit here (Ghidra shows a bare call).
           The signature is the one hs_effect_spawn_on_marker.c and
           hs_object_detach_and_place_at_location.c prove; the values are not recovered. */
        object_get_node_local_transform(object_index, control, 0, 0);
    }
    return unit_point_within_look_cone(angle_degrees * 0.017453292f);
}

#if 0
Original Ghidra decompilation (0x4878f0):

uint FUN_004878f0(undefined4 param_1,float param_2)

{
  uint in_EAX;
  int iVar1;
  uint uVar2;

  uVar2 = in_EAX & 0xffffff00;
  if (in_EAX != 0xffffffff) {
    iVar1 = object_try_and_get(3);
    if (iVar1 != 0) {
      FUN_004f6080();
    }
    uVar2 = FUN_0056c100(param_2 * 0.017453292);
  }
  return uVar2;
}
#endif
