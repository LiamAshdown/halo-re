// unit_recalculate_position  (Ghidra: unit_recalculate_position, renamed)
// address 0x558eb0, size 494 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: object.position is object+0x05c/0x060/0x064 (objects.h; Ghidra spells the Z offset
//   as decimal 100). The only *effect* the function has is calling
//   object_set_position_and_recalculate (0x4f52c0), once unconditionally and once more if that
//   call moved the object's position by more than 2 world units -- an unrolled two-step
//   convergence. Its sole caller, biped_update (0x5590a0), invokes it only for an object whose
//   network_role == 1, whose object+0x18 byte == 1 and which has no parent object, i.e. a
//   locally simulated root biped.
// RENAME: the previous name in this repo was unit_refresh_anchor_position, which asserts a write
//   this function never performs -- the cached point at object+0x1c is only ever *read* here, and
//   every value derived from it feeds calls whose results are discarded. Renamed and logged in
//   symbols/agent_phase4_units.txt.
// register convention: object index in EAX (Ghidra's in_EAX), preserved across both
//   object_set_position_and_recalculate calls, which are themselves EAX-based and show no
//   bound arguments.
//   // blam-cc: EAX -> object_index
// UNSURE: the first block is dead in a release build. real_is_valid (0x4476c0) and __isnan are
//   called and their results dropped, and the chained `x < 5000.0 != (x == 5000.0)` idiom is how
//   MSVC 7.1 emits `x < 5000.0f` from an x87 compare -- i.e. this is a compiled-in ASSERT chain
//   on the midpoint of the anchor and the current position, not a computation.
// UNSURE: local_c/local_8/local_4 are assigned *only* on the object_nudge_position_by_velocity() == 0 path and are
//   read unconditionally afterwards. The likely original is
//   `if (!object_nudge_position_by_velocity(&local_c, &local_8, &local_4)) { fall back to the anchor point; }` --
//   a predicate with three register/stack output pointers Ghidra could not bind. That reading is
//   recorded but NOT assumed: the code below keeps Ghidra's structure exactly, so the fallback
//   assignment is the only one present and the other path leaves the midpoint undefined. Since
//   the block is assert-only this does not change behaviour, but it does mean the three floats
//   must not be trusted as "equal to the anchor".
// UNSURE: object_nudge_position_by_velocity and DAT_00689471 belong to other, not-yet-processed modules.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t DAT_00689471;    // 0x00689471, UNSURE: unresolved global (a debug/assert toggle;
                                //   the whole block it gates has no side effects)

extern char object_nudge_position_by_velocity(void); // 0x4f7c40, UNSURE signature/module; see file header  // real signature (object_nudge_position_by_velocity.c): uint8_t object_nudge_position_by_velocity(uint32_t object_index); Ghidra recovered 0 of 1 args at this call site
extern void object_set_position_and_recalculate(uint32_t object_index); // 0x4f52c0, UNSURE args:  // real signature (object_set_position_and_recalculate.c): void object_set_position_and_recalculate(real_point3d *position, uint32_t object_index); Ghidra recovered 1 of 2 args at this call site
                                                                        //   EAX-based, none bound
extern double sqrt(double x);      // a single x87 FSQRT instruction in the original (Ghidra's SQRT())
extern int __isnan(double x);      // 0x624494, msvcrt
extern int real_is_valid(float x); // 0x4476c0, math module

// Re-derives a locally simulated root biped's object position through
// object_set_position_and_recalculate, and repeats the call once if the first one displaced the
// object by more than 2 world units. When the cached anchor point at object+0x1c is already
// within 5 units of the current position and the DAT_00689471 toggle is set, it first runs a
// (result-discarding, assert-only) validity check on the midpoint of the two points.
void unit_recalculate_position(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    // unit_object_anchor (types/units.h) names object+0x1c, which types/objects.h leaves inside
    // unknown_019/unknown_022 because the objects module never reads it.
    real_point3d *anchor = &((unit_object_anchor *)obj)->cached_anchor_point;

    float anchor_x = anchor->x, anchor_y = anchor->y, anchor_z = anchor->z;
    float position_x = obj->position.x, position_y = obj->position.y, position_z = obj->position.z;
    float dx = anchor_x - position_x, dy = anchor_y - position_y, dz = anchor_z - position_z;

    if (sqrt(dx * dx + dy * dy + dz * dz) <= 5.0 && DAT_00689471 != 0) {
        float mid_source_x;  // local_c -- see the UNSURE note in the file header
        float mid_source_y;  // local_8
        float mid_source_z;  // local_4

        if (object_nudge_position_by_velocity() == 0) {
            mid_source_x = anchor_x;
            mid_source_y = anchor_y;
            mid_source_z = anchor_z;
        }
        {
            float mid_x = (position_x + mid_source_x) * 0.5f;
            float mid_y = (mid_source_y + position_y) * 0.5f;
            // assert-only: every result below is discarded
            if (!__isnan((double)mid_x) && mid_x >= -5000.0f && mid_x < 5000.0f &&
                real_is_valid(mid_y) && mid_y >= -5000.0f && mid_y < 5000.0f) {
                real_is_valid((mid_source_z + position_z) * 0.5f);
            }
        }
    }

    object_set_position_and_recalculate(object_index);

    // position *after* the recalculation against the position captured before it
    dx = obj->position.x - position_x;
    dy = obj->position.y - position_y;
    dz = obj->position.z - position_z;
    if (sqrt(dx * dx + dy * dy + dz * dz) > 2.0) {
        object_set_position_and_recalculate(object_index);
    }
}

#if 0
Original Ghidra decompilation (0x558eb0):

void FUN_00558eb0(void)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  char cVar11;
  uint in_EAX;
  int iVar12;
  float local_c;
  float local_8;
  float local_4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  fVar2 = *(float *)(iVar1 + 0x1c);
  fVar3 = *(float *)(iVar1 + 0x20);
  fVar4 = *(float *)(iVar1 + 0x5c);
  fVar5 = *(float *)(iVar1 + 0x60);
  fVar6 = *(float *)(iVar1 + 100);
  fVar8 = fVar2 - fVar4;
  fVar7 = *(float *)(iVar1 + 0x24);
  fVar10 = fVar3 - fVar5;
  fVar9 = fVar7 - fVar6;
  if ((SQRT(fVar8 * fVar8 + fVar10 * fVar10 + fVar9 * fVar9) <= 5.0) && (DAT_00689471 != '\0')) {
    cVar11 = FUN_004f7c40();
    if (cVar11 == '\0') {
      local_c = fVar2;
      local_8 = fVar3;
      local_4 = fVar7;
    }
    fVar2 = (fVar4 + local_c) * 0.5;
    fVar3 = (local_8 + fVar5) * 0.5;
    iVar12 = __isnan((double)fVar2);
    if ((((iVar12 == 0) && (-5000.0 <= fVar2)) && (fVar2 < 5000.0 != (fVar2 == 5000.0))) &&
       (((iVar12 = real_is_valid(fVar3), (char)iVar12 != '\0' && (-5000.0 <= fVar3)) &&
        (fVar3 < 5000.0 != (fVar3 == 5000.0))))) {
      real_is_valid((local_4 + fVar6) * 0.5);
    }
  }
  object_set_position_and_recalculate();
  fVar4 = *(float *)(iVar1 + 0x5c) - fVar4;
  fVar5 = *(float *)(iVar1 + 0x60) - fVar5;
  fVar6 = *(float *)(iVar1 + 100) - fVar6;
  if (2.0 < SQRT(fVar4 * fVar4 + fVar5 * fVar5 + fVar6 * fVar6)) {
    object_set_position_and_recalculate();
  }
  return;
}
#endif
