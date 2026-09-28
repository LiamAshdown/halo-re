// player_set_pending_interaction_action  (Ghidra: player_set_pending_interaction_action,
// already named)
// address 0x478e00, size 262 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x478e00..0x478f05 (EAX player, EBX candidate, stack priority / seat).)
// evidence: out/phase4/game_types_notes.md ("player_set_pending_interaction_action 0x478e00 --
//   0x24 interaction object, 0x28 priority type (0xb clears), 0x2a seat"), matching
//   types/game.h player::interaction_object/interaction_type/interaction_seat exactly;
//   types/objects.h object::position (0x5c).
// register convention: the acting player's index in EAX (in_EAX) and a candidate interaction
//   object in EBX (unaff_EBX); `priority_type` and `seat` are this function's own two stack
//   parameters.
//   // blam-cc: EAX -> player_index, EBX -> candidate_object, stack -> priority_type, seat
// UNSURE: object dword offset 0x64 (used here as position.z's sibling field, per types/objects.h
//   real_point3d position spanning 0x5c-0x67) -- confirmed consistent with that struct.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern double sqrt(double x); // x87 FSQRT

// blam-cc: EAX -> player_index, EBX -> candidate_object, stack -> priority_type, seat
// Updates the player's pending interaction slot to `candidate_object` (with `priority_type`,
// `seat`) unless: priority_type is 0xb (a request to clear, always accepted) is not the case
// and there is already a higher-priority pending interaction, or, when priority_type ties the
// existing one, the existing candidate is no farther from the player's own unit than the new
// one is.
void player_set_pending_interaction_action(int16_t priority_type, int16_t seat,
    uint32_t player_index, uint32_t candidate_object)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));

    if (priority_type != 0xb) {
        if (priority_type == p->interaction_type) {
            object *unit = (object *)((object_header *)object_data->data)[p->unit & 0xffff].data;
            object *existing = (object *)((object_header *)object_data->data)[p->interaction_object & 0xffff].data;
            object *candidate = (object *)((object_header *)object_data->data)[candidate_object & 0xffff].data;

            float ex = existing->position.x - unit->position.x;
            float ey = existing->position.y - unit->position.y;
            float ez = existing->position.z - unit->position.z;
            float cx = candidate->position.x - unit->position.x;
            float cy = candidate->position.y - unit->position.y;
            float cz = candidate->position.z - unit->position.z;

            if (sqrt(ey * ey + ex * ex + ez * ez) <= sqrt(cx * cx + cy * cy + cz * cz)) {
                return;
            }
        } else if (priority_type <= p->interaction_type) {
            return;
        }
    }

    p->interaction_type = priority_type;
    p->interaction_object = (datum_index)candidate_object;
    p->interaction_seat = seat;
}

#if 0
Original Ghidra decompilation (0x478e00), from tools/pack.py 0x478e00:

void player_set_pending_interaction_action(short param_1,undefined2 param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  uint in_EAX;
  uint unaff_EBX;
  int iVar10;

  iVar10 = (in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  if (param_1 != 0xb) {
    if (param_1 == *(short *)(iVar10 + 0x28)) {
      iVar1 = *(int *)(DAT_008603b0 + 0x34);
      iVar2 = *(int *)(iVar1 + 8 + (*(uint *)(iVar10 + 0x34) & 0xffff) * 0xc);
      iVar3 = *(int *)(iVar1 + 8 + (*(uint *)(iVar10 + 0x24) & 0xffff) * 0xc);
      fVar4 = *(float *)(iVar3 + 0x5c) - *(float *)(iVar2 + 0x5c);
      fVar5 = *(float *)(iVar3 + 0x60) - *(float *)(iVar2 + 0x60);
      iVar1 = *(int *)(iVar1 + 8 + (unaff_EBX & 0xffff) * 0xc);
      fVar6 = *(float *)(iVar3 + 100) - *(float *)(iVar2 + 100);
      fVar7 = *(float *)(iVar1 + 0x5c) - *(float *)(iVar2 + 0x5c);
      fVar8 = *(float *)(iVar1 + 0x60) - *(float *)(iVar2 + 0x60);
      fVar9 = *(float *)(iVar1 + 100) - *(float *)(iVar2 + 100);
      if (SQRT(fVar5 * fVar5 + fVar4 * fVar4 + fVar6 * fVar6) <=
          SQRT(fVar7 * fVar7 + fVar8 * fVar8 + fVar9 * fVar9)) {
        return;
      }
    }
    else if (param_1 <= *(short *)(iVar10 + 0x28)) {
      return;
    }
  }
  *(short *)(iVar10 + 0x28) = param_1;
  *(uint *)(iVar10 + 0x24) = unaff_EBX;
  *(undefined2 *)(iVar10 + 0x2a) = param_2;
  return;
}
#endif
