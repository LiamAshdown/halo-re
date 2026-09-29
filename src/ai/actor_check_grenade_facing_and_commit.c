// actor_check_grenade_facing_and_commit  (Ghidra: actor_check_grenade_facing_and_commit, renamed)
// address 0x40db00, size 300 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x40db00..0x40dc2b; cos 30 deg threshold, offsets probed)
// evidence: phase-4 summary "checks whether the actor is now facing closely enough toward
// its grenade target and, if so, commits to the throw and timestamps the threat record";
// the final commit writes actor.unknown_45c = 1, clears actor.unknown_6a0 (the
// grenade_impact_point valid flag, inferred from its position right before that field) and
// timestamps encounter.unknown_5c with the current game tick -- the same field
// actor_can_throw_grenade_at_target (0x40d9c0) later reads back as a cooldown deadline.
// All of Ghidra's float "NaN-flag" comparisons here have been simplified to plain
// comparisons (fVar < 0 || fVar == 0  ->  fVar <= 0, etc.), which is semantically identical
// for non-NaN floats and is how the rest of this codebase's math rewrites read them.
// register (Ghidra recognized a formal char param_2, so it is left as a plain parameter).
// UNSURE: FUN_00569c90 is called with no visible argument; treated as taking actor_index.
// UNSURE: the dot product at the end multiplies the normalized impact-point delta by
// actor.position.x/.y (0x174/0x178), which is the actor's world position, not an obviously
// named forward vector; kept exactly as the offsets read, flagged for the review pass.
// actor+0x12c/0x130 is actor.body_position; types/ai.h shrank mode_data to 0x84 so the two
// operands are named fields now rather than offsets into the per-mode union.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "game.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *encounter_data;  // 0x008802c8
extern game_time_globals *game_time; // 0x006f1d6c
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, vector in ECX

extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index); // 0x569c90, ECX = the unit (actor +0x18)
extern uint8_t actor_can_throw_grenade_at_target(datum_index actor_index); // 0x40d9c0, this module

// FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original never reads EAX; actor_index arrive(s) on the stack (2 stack argument(s)).
// blam-cc: stack -> actor_index, force_commit
uint8_t actor_check_grenade_facing_and_commit(datum_index actor_index, uint8_t force_commit)
{
    actor *self;
    datum_index unit_index;
    object_header *unit_header;
    object *unit_obj;
    real body_damage;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    unit_index = self->unit_index;

    if (unit_is_in_busy_animation_state(self->unit_index) != 0) {
        return 0;
    }

    unit_header = (object_header *)object_data->data + (unit_index & 0xffff);
    unit_obj = unit_header->data;
    body_damage = unit_obj->current_body_damage;
    if (body_damage > 0.0f) {
        return 0;
    }

    if (force_commit == 0) {
        if (actor_can_throw_grenade_at_target(actor_index) == 0) {
            self->grenade_throw_pending = 0;
        }
    }

    if (self->grenade_throw_pending != 0) {
        real_vector2d delta;
        float dot;

        delta.i = self->grenade_impact_point.x - self->body_position.x;
        delta.j = self->grenade_impact_point.y - self->body_position.y;

        if (vector2d_normalize_with_length(&delta) > 0.0f) {
            dot = delta.i * self->facing.i + delta.j * self->facing.j;
            if (dot >= 0.8660254f) {
                self->unknown_45c = 1;
                self->grenade_throw_pending = 0;
                if (self->encounter_index != (datum_index)k_datum_index_none) {
                    encounter *enc = (encounter *)((uint8_t *)encounter_data->data +
                                                    (self->encounter_index & 0xffff) * sizeof(encounter));
                    enc->last_grenade_time = game_time->game_time; // +0x0c
                }
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x40db00):

uint FUN_0040db00(uint param_1,char param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float10 fVar5;
  char extraout_AL;
  uint uVar6;
  undefined3 uVar8;
  undefined3 extraout_var;
  undefined2 extraout_var_00;
  uint uVar7;
  int iVar9;
  float10 fVar10;

  iVar9 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar7 = *(uint *)(iVar9 + 0x18);
  uVar6 = FUN_00569c90();
  if ((char)uVar6 == '\0') {
    fVar1 = *(float *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc) + 0xec);
    uVar6 = CONCAT22((short)((uint)*(int *)(DAT_008603b0 + 0x34) >> 0x10),
                     (ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 |
                     (ushort)(fVar1 == 0.0) << 0xe);
    if (fVar1 < 0.0 != 0 || (fVar1 == 0.0) != 0) {
      uVar8 = (undefined3)(uVar6 >> 8);
      if ((param_2 == '\0') && (FUN_0040d9c0(), uVar8 = extraout_var, extraout_AL == '\0')) {
        *(undefined1 *)(iVar9 + 0x6a0) = 0;
      }
      uVar6 = CONCAT31(uVar8,*(char *)(iVar9 + 0x6a0));
      if (*(char *)(iVar9 + 0x6a0) != '\0') {
        fVar1 = *(float *)(iVar9 + 0x6a8);
        fVar2 = *(float *)(iVar9 + 300);
        fVar3 = *(float *)(iVar9 + 0x6ac);
        fVar4 = *(float *)(iVar9 + 0x130);
        fVar10 = (float10)vector2d_normalize_with_length();
        fVar5 = (float10)0.0;
        uVar6 = CONCAT22(extraout_var_00,
                         (ushort)(fVar10 < fVar5) << 8 | (ushort)(NAN(fVar10) || NAN(fVar5)) << 10 |
                         (ushort)(fVar10 == fVar5) << 0xe);
        if (fVar10 < fVar5 == 0 && (fVar10 == fVar5) == 0) {
          fVar1 = (fVar1 - fVar2) * *(float *)(iVar9 + 0x174) +
                  (fVar3 - fVar4) * *(float *)(iVar9 + 0x178);
          uVar6 = CONCAT22(extraout_var_00,
                           (ushort)(fVar1 < 0.8660254) << 8 | (ushort)NAN(fVar1) << 10 |
                           (ushort)(fVar1 == 0.8660254) << 0xe);
          if (fVar1 >= 0.8660254) {
            uVar7 = CONCAT31((int3)(uVar6 >> 8),1);
            *(undefined1 *)(iVar9 + 0x45c) = 1;
            *(undefined1 *)(iVar9 + 0x6a0) = 0;
            if (*(uint *)(iVar9 + 0x34) == 0xffffffff) {
              return uVar7;
            }
            *(undefined4 *)
             ((*(uint *)(iVar9 + 0x34) & 0xffff) * 0x6c + 0x5c + *(int *)(DAT_008802c8 + 0x34)) =
                 *(undefined4 *)(DAT_006f1d6c + 0xc);
            return uVar7;
          }
        }
      }
    }
  }
  return uVar6 & 0xffffff00;
}
#endif
