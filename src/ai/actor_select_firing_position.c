// actor_select_firing_position  (Ghidra: actor_select_firing_position, renamed)
// address 0x413e50, size 516 bytes
// name confidence: 0.55  rewrite confidence: 0.5
// evidence: it is the wrapper around actor_find_best_firing_position @0x412ba0. It resolves
//   the three group masks with actor_get_firing_position_group_mask @0x412880 (once with the
//   actor own searching state, once forced off, once forced on), decides which becomes
//   query.group_mask and which becomes the softer query.marked_group_mask, and when the
//   search comes back empty it hand-builds one actor_firing_position_candidate around the
//   position the actor already holds and runs it through actor_firing_position_evaluate
//   @0x412820.
// register convention: the query is in EBX and the out-candidate in EDI; actor_index and
//   the three out-parameters are the Ghidra-recognized stack parameters.
//
// UNSURE: param_3 is never read by the body. It is passed straight through to
// actor_get_firing_position_group_mask in SI, which is the only way the three calls can
// differ in anything but the search override, so it is taken to be the mask kind.
// vector3d_distance_squared (0x401020) takes EAX / ECX: at 0x413ff7..0x414001 EAX = the held
// firing position (the pointer also stored in out_candidate->position) and ECX = query + 0x604,
// query->target_position.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data;    // 0x00880360
extern Scenario *global_scenario; // 0x00746f8c
extern const real_vector3d *global_origin3d_pointer; // 0x00696714

extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, src/math; blam-cc: EAX a, ECX b


// blam-cc: EBX -> query, EDI -> out_candidate; stack -> actor_index, out_previous_owner,
//          path_context, out_path_ok (the goal kind is query +0x04)
// Picks a firing position for the actor. When the mask for the actor current searching state
// is a strict subset of the union of both states, the wider union becomes the hard filter and
// the narrower own-state mask becomes a soft preference worth 8.0. If nothing is found the
// actor keeps the position it already holds, provided that claim is still marked live, and
// that one position is scored and vetted on its own. Returns the chosen firing position index
// or -1; a search that lands on a position outside the own-state mask toggles the searching
// flag so the next tick looks at the other half.
int16_t actor_select_firing_position(datum_index actor_index,
                                     actor_firing_position_query *query,
                                     actor_firing_position_candidate *out_candidate,
                                     uint32_t *out_previous_owner, path_find_context *path_context,
                                     uint8_t *out_path_ok)
{
    // 0x413e84: the group masks use the query's goal kind (+0x04, SI); the third stack argument is the path
    // context handed through to actor_find_best_firing_position
    int16_t kind = query->goal_kind;
    actor *self;
    ScenarioEncounter *encounter_definition;
    ScenarioFiringPosition *firing_positions;
    uint32_t own_mask;
    uint32_t not_searching_mask;
    uint32_t searching_mask;
    int16_t result;
    int16_t held;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    if (self->encounter_index == (datum_index)0xffffffff) {
        return -1;
    }

    own_mask = actor_get_firing_position_group_mask(actor_index, kind, 0);
    not_searching_mask = actor_get_firing_position_group_mask(actor_index, kind, 2);
    searching_mask = actor_get_firing_position_group_mask(actor_index, kind, 1);

    if (own_mask < (not_searching_mask | searching_mask)) {
        query->marked_group_mask = own_mask;
        query->marked_group_penalty = 8.0f;
        query->group_mask = not_searching_mask | searching_mask;
    } else {
        query->group_mask = own_mask;
    }

    query->allow_random_fallback =
        (uint8_t)(self->firing_position_index == -1 || self->unknown_3ba == 0);
    query->collect_all = 1;

    // UNSURE: the remaining arguments travel in registers this frame already holds.
    result = (int16_t)actor_find_best_firing_position(actor_index, query, out_candidate,
                                                      out_previous_owner, path_context, out_path_ok);

    if (result == -1) {
        held = self->firing_position_index;
        if (held == -1 || self->unknown_3ba == 0) {
            return result;
        }

        encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
                                    [self->encounter_index & 0xffff];
        firing_positions =
            (ScenarioFiringPosition *)encounter_definition->firing_positions.pointer;

        out_candidate->position = (uint32_t)(uint8_t *)&firing_positions[held];
        out_candidate->distance_from_actor = 3.4028235e+38f;
        out_candidate->distance_from_target = 3.4028235e+38f;
        out_candidate->segment_distance = 3.4028235e+38f;
        out_candidate->firing_position_index = held;
        out_candidate->request_result = 0;
        out_candidate->direction_from_target.i = global_origin3d_pointer->i;
        out_candidate->direction_from_target.j = global_origin3d_pointer->j;
        out_candidate->direction_from_target.k = global_origin3d_pointer->k;
        out_candidate->direction_from_actor.i = global_origin3d_pointer->i;
        out_candidate->direction_from_actor.j = global_origin3d_pointer->j;
        out_candidate->direction_from_actor.k = global_origin3d_pointer->k;
        if (query->have_target == 0) {
            out_candidate->distance_squared_to_target = 0.0f;
        } else {
            out_candidate->distance_squared_to_target = vector3d_distance_squared(
                (real_point3d *)&firing_positions[held], &query->target_position);
        }

        if (actor_firing_position_evaluate(out_candidate, query, actor_index) == 0) {
            held = -1;
            self->firing_position_index = -1;
        }
        *out_previous_owner = 0xffffffff;
        *out_path_ok = 0;
        return held;
    }

    // The winner came from the wider union: if it is not in the mask for the actor current
    // searching state, flip that state so the next pass looks at the other half.
    if ((own_mask & (1u << (((uint8_t *)out_candidate->position)[0xc] & 0x1f))) == 0) {
        self->unknown_98 = (uint8_t)(self->unknown_98 == 0);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x413e50):

short FUN_00413e50(uint param_1,undefined4 *param_2,undefined4 param_3,undefined1 *param_4)

{
  undefined *puVar1;
  char cVar2;
  short sVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  uint *unaff_EBX;
  short sVar8;
  int iVar9;
  int *unaff_EDI;
  float10 fVar10;

  iVar9 = (param_1 & 0xffff) * 0x724;
  iVar7 = *(int *)(DAT_00880360 + 0x34) + iVar9;
  if (*(int *)(iVar7 + 0x34) == -1) {
    sVar3 = -1;
  }
  else {
    uVar4 = FUN_00412880(0);
    uVar5 = FUN_00412880(2);
    uVar6 = FUN_00412880(1);
    if (uVar4 < (uVar5 | uVar6)) {
      unaff_EBX[0x12] = uVar4;
      unaff_EBX[0x13] = 0x41000000;
      *unaff_EBX = uVar5 | uVar6;
    }
    else {
      *unaff_EBX = uVar4;
    }
    if ((*(short *)(iVar7 + 0x3b8) == -1) || (*(char *)(iVar7 + 0x3ba) == '\0')) {
      *(undefined1 *)((int)unaff_EBX + 0x15) = 1;
    }
    else {
      *(undefined1 *)((int)unaff_EBX + 0x15) = 0;
    }
    *(undefined1 *)(unaff_EBX + 5) = 1;
    sVar3 = FUN_00412ba0(param_1);
    puVar1 = PTR_DAT_00696714;
    if (sVar3 == -1) {
      sVar8 = *(short *)(iVar7 + 0x3b8);
      if ((sVar8 != -1) && (*(char *)(iVar7 + 0x3ba) != '\0')) {
        *unaff_EDI = *(int *)((*(uint *)(iVar7 + 0x34) & 0xffff) * 0xb0 + 0x9c +
                             *(int *)(global_scenario + 0x430)) + sVar8 * 0x18;
        unaff_EDI[2] = 0x7f7fffff;
        unaff_EDI[6] = 0x7f7fffff;
        unaff_EDI[7] = 0x7f7fffff;
        *(short *)(unaff_EDI + 1) = sVar8;
        *(undefined2 *)((int)unaff_EDI + 6) = 0;
        unaff_EDI[8] = *(int *)puVar1;
        unaff_EDI[9] = *(int *)(puVar1 + 4);
        unaff_EDI[10] = *(int *)(puVar1 + 8);
        unaff_EDI[3] = *(int *)puVar1;
        unaff_EDI[4] = *(int *)(puVar1 + 4);
        unaff_EDI[5] = *(int *)(puVar1 + 8);
        if ((char)unaff_EBX[0x17f] == '\0') {
          unaff_EDI[0xb] = 0;
        }
        else {
          fVar10 = (float10)FUN_00401020();
          unaff_EDI[0xb] = (int)(float)fVar10;
        }
        cVar2 = FUN_00412820();
        if (cVar2 == '\0') {
          sVar8 = -1;
          *(undefined2 *)(iVar7 + 0x3b8) = 0xffff;
        }
        *param_2 = 0xffffffff;
        *param_4 = 0;
        return sVar8;
      }
    }
    else if ((uVar4 & 1 << (*(byte *)(*unaff_EDI + 0xc) & 0x1f)) == 0) {
      *(bool *)(*(int *)(DAT_00880360 + 0x34) + iVar9 + 0x98) =
           *(char *)(*(int *)(DAT_00880360 + 0x34) + 0x98 + iVar9) == '\0';
      return sVar3;
    }
  }
  return sVar3;
}
#endif
