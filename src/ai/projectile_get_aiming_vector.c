// projectile_get_aiming_vector  (Ghidra: FUN_004beec0; renamed)
// address 0x4beec0, size 179 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x4beec0..0x4bef72: every register / stack slot maps onto the two solvers.)
// evidence: out/phase4/projectiles_types_notes.md "0x4beb30, 0x4bee20, 0x4beec0 are AI
//   ballistic-aiming helpers, not projectile state... 0x4beec0 is the dispatcher: it reads
//   tag + 0x1e4 (initial_velocity) as the default speed when the caller passes no speed, then
//   tests tag + 0x17c bit 0x02 (ai_must_use_ballistic_aiming) together with tag + 0x1cc
//   (air_gravity_scale) > 0 to pick between the gravity-arc solver 0x4beb30 and the
//   straight-line solver 0x4bee20, and reports which one it used through its last out
//   parameter"; src/projectiles/README.md "Misattributed functions" table gives the same
//   description. Its two out/functions.json callers landing on named functions are both in
//   src/ai (actor_solve_grenade_lob @0x410780, actor_get_grenade_launch_velocity @0x410980);
//   the third (weapon_trigger_get_aiming_vector @0x4c2b40) is in src/items. cleanup
//   pass 4 orphan pass: neither projectiles nor items claimed this address; picked up here in
//   ai alongside its two callees, projectile_solve_ballistic_arc (0x4beb30) and
//   projectile_solve_straight_line (0x4bee20), both written in this same batch.
// register convention: Ghidra's own decompile of this function is already a clean, fully
//   resolved 11-stack-parameter signature plus one register input (in_EAX, an optional speed
//   override). Disassembly of both call sites (the ballistic call at 0x4bef0f..0x4bef3a and the
//   straight-line call at 0x4bef50..0x4bef5d) additionally shows this function's OWN incoming
//   ECX register -- never read anywhere in its own body -- is forwarded unchanged into both
//   children's in_EAX (their "target position" input), which is how the ballistic-arc and
//   straight-line solvers reconstruct a "target position" argument this function's Ghidra
//   decompile never explicitly names for itself. Existing callers in this repo
//   (src/ai/actor_solve_grenade_lob.c, src/ai/actor_get_grenade_launch_velocity.c,
//   src/items/weapon_trigger_get_aiming_vector.c) carried their own, mutually inconsistent
//   extern declarations for FUN_004beec0 (11, 6 and 11 stack-only arguments). The orphan pass 4
//   review re-derived each call site from objdump (0x4107e9..0x410823, 0x4109ca..0x4109f1,
//   0x4c2b76..0x4c2bcd) and moved all three onto this signature.
//   // blam-cc: ECX -> target, EAX -> speed_in (nullable), stack -> (tag, origin,
//   //          unused_param_3, max_time, max_speed_override, use_high_arc, out_direction,
//   //          out_speed, out_time_or_fraction, out_range_or_length, out_used_straight_line)
// UNSURE: param_3 (the third stack slot) is read nowhere in this function's body; kept as an
//   unused parameter rather than dropped, per this repo's convention for Ghidra-recognized dead
//   parameters (e.g. src/physics/physics_model_slide_along_contacts.c's param_4).
// UNSURE: the eleventh stack slot (out_range_or_length here) is forwarded to
//   projectile_solve_ballistic_arc's out_range in the gravity branch but to
//   projectile_solve_straight_line's out_length in the straight branch -- the same caller-owned
//   pointer slot is read differently by the two solvers. This is preserved exactly as the
//   disassembly shows it; it is not a rewrite error.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#include "fn_ai.h"


// Chooses between the gravity-arc and straight-line aiming solvers for a shot from *origin to
// *target using tag's Projectile definition, and reports which one it used through
// *out_used_straight_line (1 for the straight-line solver, 0 for the gravity-arc solver; either
// out pointer may be NULL). speed_in overrides the tag's initial_velocity when non-NULL.
// max_time, max_speed_override and use_high_arc are only meaningful for the gravity-arc branch;
// out_range_or_length is forwarded to whichever solver ran, which reads it differently (see the
// UNSURE note above).
// Returns the chosen solver's result byte: AL survives the `mov byte [ebp],0/1` tails
// (0x4bef3d..0x4bef4f, 0x4bef62..0x4bef72), so callers test it (orphan pass 4 review; the
// earlier rewrite returned void).
uint8_t projectile_get_aiming_vector(real_point3d *target, real *speed_in, Projectile *tag,
    real_point3d *origin, void *unused_param_3, real *max_time, real *max_speed_override,
    uint8_t use_high_arc, real_vector3d *out_direction, real *out_speed,
    real *out_time_or_fraction, real *out_range_or_length, uint8_t *out_used_straight_line)
{
    real speed;
    uint8_t solved;

    if (speed_in == (real *)0) {
        speed = tag->initial_velocity;
    } else {
        speed = *speed_in;
    }

    if (((tag->projectile_flags & _projectile_definition_ai_must_use_ballistic_aiming_bit) == 0) ||
        (tag->air_gravity_scale <= 0.0f)) {
        solved = projectile_solve_straight_line(target, origin, speed, out_time_or_fraction, out_direction,
            out_speed, out_range_or_length);
        if (out_used_straight_line != (uint8_t *)0) {
            *out_used_straight_line = 1;
        }
    } else {
        solved = projectile_solve_ballistic_arc(target, origin, speed, tag->air_gravity_scale, max_time,
            use_high_arc, out_direction, max_speed_override, out_speed, out_time_or_fraction,
            out_range_or_length, (real *)0, (real *)0);
        if (out_used_straight_line != (uint8_t *)0) {
            *out_used_straight_line = 0;
        }
    }
    return solved;
}

#if 0
Original Ghidra decompilation (0x4beec0):

void FUN_004beec0(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5,undefined4 param_6,undefined4 param_7,undefined4 param_8,
                 undefined4 param_9,undefined4 param_10,undefined1 *param_11)

{
  undefined4 *in_EAX;
  undefined4 uVar1;

  if (in_EAX == (undefined4 *)0x0) {
    uVar1 = *(undefined4 *)(param_1 + 0x1e4);
  }
  else {
    uVar1 = *in_EAX;
  }
  if (((*(byte *)(param_1 + 0x17c) & 2) == 0) || (*(float *)(param_1 + 0x1cc) <= 0.0)) {
    FUN_004bee20(uVar1,param_2,param_9);
    if (param_11 != (undefined1 *)0x0) {
      *param_11 = 1;
    }
  }
  else {
    FUN_004beb30(uVar1,*(undefined4 *)(param_1 + 0x1cc),param_4,param_6,param_8,param_9,param_10,0,0
                );
    if (param_11 != (undefined1 *)0x0) {
      *param_11 = 0;
      return;
    }
  }
  return;
}
#endif
