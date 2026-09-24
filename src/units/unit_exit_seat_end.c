// unit_exit_seat_end  (Ghidra: already named unit_exit_seat_end)
// address 0x56fd40, size 508 bytes
// name confidence: 0.5 (functions.md summary matches what little is legible)
// rewrite confidence: 0.1 -- by far the least legible function in this batch. Ghidra could not
//   recover almost any of this function's real inputs (they show up as in_stack_/unaff_ registers
//   pointing at addresses above the visible stack frame), which is the signature of a function
//   whose entry point Ghidra split awkwardly out of a larger caller -- most likely the tail of
//   whichever function actually ends a seat-exit, sharing unit_melee_lunge_damage_tick's exact
//   damage-record shape and even its melee_damage_countdown field for an unrelated purpose. Only
//   the object offsets that are unambiguous regardless of which base pointer holds them are
//   translated; everything else is preserved as Ghidra's own stack-relative names.
// evidence: types/units.h unit_data.controlling_player (0x218), .melee_damage_countdown (0x28a);
//   types/objects.h object.parent_object (0x11c); the tag_id-at-0x294 idiom matches
//   Unit.melee_damage across every other melee function in this batch.
// register convention: UNRESOLVED. Ghidra shows the unit object pointer as unaff_ESI and a
//   damage-effect source pointer as in_stack_00000048, both of which must come from the
//   caller's own registers/stack rather than from parameters visible at this entry point.
//   // blam-cc: UNSURE -- see header
// UNSURE: essentially everything below the first two floats. This file should be treated as a
//   best-effort transcription, not a verified rewrite, and is a strong candidate for being
//   re-examined once its real caller (a function currently ending elsewhere in the module) is
//   identified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern int8_t object_collision_context_test_segment(void *out_plane, int32_t mask); // 0x504f60, UNSURE: real signature
                                                             //   takes more arguments that are
                                                             //   entirely register/stack-resident
                                                             //   here and not recoverable
extern void matrix4x3_transform_plane(void); // 0x4cbf10, UNSURE: register args  // real signature (matrix4x3_transform_plane.c): void matrix4x3_transform_plane(real_plane3d *out, real_matrix4x3 *m, real_plane3d *plane); Ghidra recovered 0 of 3 args at this call site
extern void object_apply_damage(damage_data *dd, uint32_t object_index, uint32_t param_3); // 0x4ee5e0,  // real signature (object_apply_damage.c): void object_apply_damage(damage_data *dd, uint32_t param_2, int16_t param_3, int16_t param_4, int16_t param_5, uint32_t param_6); Ghidra recovered 3 of 6 args at this call site
    // UNSURE: only 3 arguments are visible at this call site, fewer than the 6-parameter
    // signature established elsewhere in this module (see unit_cause_melee_damage.c); the
    // remaining parameters are presumably also inherited from the caller's frame

// Checks for and applies collision/crush damage when a unit finishes exiting a vehicle seat.
// UNSURE: reproduced as literally as Ghidra's own (mostly unrecoverable) locals allow; see the
// file header before trusting any specific field of this rewrite.
void unit_exit_seat_end(uint32_t unit_index, uint8_t already_hit, uint32_t damage_tag_source_addr)
{
    object *obj = 0; // UNSURE: stands in for unaff_ESI, which this file cannot resolve to a
                      // concrete object pointer; callers of this function need to supply it.
    unit_data *unit;
    float delta_i, delta_j; // fStack00000024/28: an already-computed X component plus
                             // 0.2 * object.forward.k (unaff_ESI + 0x7c)
    uint8_t hit = already_hit;
    damage_data dd = {0};

    (void)obj; // placeholder body: see file header -- this function's real base pointer is not
               // recoverable from the decompile, so its field writes cannot be reproduced safely
    unit = 0;

    delta_j = 0.0f; // UNSURE: fStack00000024 = (float)in_ST0, an x87 value with no visible source
    delta_i = delta_j;

    if (object_collision_context_test_segment((void *)0, 3) != 0) {
        matrix4x3_transform_plane();
        hit = 1;
    }

    dd.damage_effect_tag = *(datum_index *)(damage_tag_source_addr + 0x294);
    dd.responsible_player = 0; // UNSURE: unaff_ESI + 0x218 (controlling_player)

    if (hit) {
        dd.flags |= 2;
        // UNSURE: unaff_ESI + 0x28a (melee_damage_countdown) = 10
    }

    object_apply_damage(&dd, 0 /* UNSURE: unaff_ESI + 0x11c, parent_object */, 0);
    // UNSURE: unaff_ESI + 0x28a -= 1
}

#if 0
Original Ghidra decompilation (0x56fd40):

void unit_exit_seat_end(void)

{
  char cVar1;
  int iVar2;
  char unaff_BL;
  int unaff_ESI;
  undefined4 *puVar3;
  float10 in_ST0;
  float fStack00000024;
  float fStack00000028;
  int in_stack_00000048;
  undefined4 in_stack_0000004c;
  uint in_stack_00000050;
  undefined4 in_stack_00000054;
  undefined2 uStack000000b4;
  undefined2 uStack000000b6;
  undefined2 in_stack_000000b8;
  undefined4 in_stack_000000bc;
  int in_stack_000000c8;
  undefined4 uVar4;
  undefined4 uVar5;

  fStack00000024 = (float)in_ST0;
  fStack00000028 = *(float *)(unaff_ESI + 0x7c) * 0.2;
  cVar1 = FUN_00504f60(&stack0x000000a4,3);
  if (cVar1 != '\0') {
    matrix4x3_transform_plane();
    if (in_stack_000000c8 < 0) {
      fStack00000024 = -fStack00000024;
      fStack00000028 = -fStack00000028;
    }
    unaff_BL = '\x01';
  }
  puVar3 = &stack0x0000004c;
  for (iVar2 = 0x15; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  in_stack_0000004c = *(undefined4 *)(in_stack_00000048 + 0x294);
  in_stack_00000054 = *(undefined4 *)(unaff_ESI + 0x218);
  if (unaff_BL == '\0') {
    uVar5 = 0xffffffff;
    uVar4 = *(undefined4 *)(unaff_ESI + 0x11c);
  }
  else {
    in_stack_00000050 = in_stack_00000050 | 2;
    uVar5 = CONCAT22(uStack000000b6,uStack000000b4);
    uVar4 = *(undefined4 *)(unaff_ESI + 0x11c);
    *(undefined1 *)(unaff_ESI + 0x28a) = 10;
  }
  object_apply_damage(&stack0x0000004c,uVar4,uVar5);
  *(char *)(unaff_ESI + 0x28a) = *(char *)(unaff_ESI + 0x28a) + -1;
  return;
}
#endif
