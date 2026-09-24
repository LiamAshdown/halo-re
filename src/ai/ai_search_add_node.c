// ai_search_add_node  (Ghidra: ai_search_add_node, renamed)
// address 0x43b5a0, size 483 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: types/ai.h ai_search_context.node_count(+0x2c, capped at 0x80)/origin(+0x10)/
//   goal_point_id(+0x1c)/best_cost(+0x24)/best_node(+0x20)/heap_count(+0x1430)/heap(+0x1432)/
//   nodes(+0x30, stride 0x28); ai_search_node.position/z/direction/length/point_id/side/
//   side_link/cost/parent. phase-4 summary "adds a new open-list node to the AI point-search
//   graph (reusing an existing chained node when appropriate) and pushes it onto the
//   cost-ordered min-heap." Calls vector2d_normalize_with_length (established elsewhere) and
//   ai_search_heap_sift_up @0x43b450 (this rewrite).
// register convention: EDI -> context, BX -> chain_head (the first node already anchored at
//   this point, or -1); stack -> position, z, point_id, side, extra_cost.
//   // blam-cc: EDI -> context, EBX -> chain_head, stack -> position, z, point_id, side,
//   //   extra_cost
//
// UNSURE: node.parent (+0x24) is reused here as a "next node anchored at the same point_id"
// chain link, not as the search-tree parent the header otherwise documents it as; the two
// uses do not conflict in practice because a freshly-created node has no real parent yet.
// The mid-loop side-compatibility test (a pair of 2D cross-product sign comparisons against
// the direction vectors of `chain_head`'s own node and a candidate side-linked node) is
// reproduced exactly but not independently rederived here.

// FIXED (difftest + objdump 0x43b6f0..0x43b76b): a new node's parent is chain_head and both side links
//   are -1 (the draft had them swapped); the best node is tracked by length, not cost; the return is
//   the 16-bit index in AX.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern float vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, established elsewhere (normalizes in place, returns the pre-normalize length)
extern void ai_search_heap_sift_up(ai_search_context *context, int16_t index); // 0x43b450

// blam-cc: EDI -> context, EBX -> chain_head, stack -> position, z, point_id, side, extra_cost
// Returns the new or reused node's index in the low 16 bits, or -1 on failure/rejection.
int16_t ai_search_add_node(ai_search_context *context, int16_t chain_head, real_point2d *position,
                           float z, int16_t point_id, uint8_t side, float extra_cost)
{
    float dx, dy;
    uint8_t goal_side_established;
    int16_t cursor;

    if (0x7f < context->node_count) {
        return -1;
    }

    dx = context->origin.x - position->x;
    goal_side_established = 0;
    dy = context->origin.y - position->y;
    cursor = chain_head;

    for (;;) {
        ai_search_node *cur;

        if (cursor == -1) {
            int16_t new_index = context->node_count;
            ai_search_node *node;

            context->node_count = new_index + 1;
            node = &context->nodes[new_index];
            node->position = *position;
            node->z = z;
            node->direction.i = dx;
            node->direction.j = dy;
            node->length = vector2d_normalize_with_length(&node->direction);
            node->point_id = point_id;
            node->cost = node->length + extra_cost;
            node->side = side;
            node->parent = chain_head;          // mov [esi+0x24],bx
            *(int32_t *)&node->side_link = -1;  // mov dword [esi+0x1c],-1: both side links (0x1c, 0x1e)

            if ((goal_side_established != 0) && (node->length < context->best_cost)) { // fcomp [esi+0x14]
                context->best_cost = node->length;
                context->best_node = new_index;
            }

            if (context->heap_count < 0x80) {
                int16_t slot = context->heap_count;
                context->heap_count = slot + 1;
                context->heap[slot] = new_index;
                ai_search_heap_sift_up(context, slot);
            }

            return new_index;
        }

        cur = &context->nodes[cursor];
        if (cur->point_id != point_id) {
            if ((point_id == context->goal_point_id) && (context->goal_point_id != -1)) {
                int16_t sibling;
                goal_side_established = 1;
                sibling = (side == 0) ? *(int16_t *)((uint8_t *)cur + 0x1c + 2) : *(int16_t *)((uint8_t *)cur + 0x1c);
                if (sibling != -1) {
                    ai_search_node *other = &context->nodes[sibling];
                    if ((0.0f < dx * other->direction.i + dy * other->direction.j) &&
                        ((dy * other->direction.i - dx * other->direction.j) *
                         (context->nodes[chain_head].direction.j * other->direction.i -
                          other->direction.j * context->nodes[chain_head].direction.i) < 0.0f)) {
                        return -1;
                    }
                }
                {
                    int16_t *slot_ptr = (side == 0) ? (int16_t *)((uint8_t *)cur + 0x1c) : (int16_t *)((uint8_t *)cur + 0x1c + 2);
                    if ((*slot_ptr == chain_head) || (*slot_ptr == -1)) {
                        *slot_ptr = context->node_count;
                    }
                }
            }
            cursor = -1;
            continue;
        }

        if (cur->side != side) {
            return -1;
        }
        cursor = cur->parent;
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043b5a0 @ 0x43b5a0) ----
undefined4 FUN_0043b5a0(float *param_1,float param_2,short param_3,byte param_4,float param_5)

{
  int iVar1;
  int iVar2;
  float *pfVar3;
  short sVar4;
  short sVar5;
  float fVar6;
  float fVar7;
  bool bVar8;
  undefined2 extraout_var;
  undefined2 extraout_var_00;
  undefined2 uVar9;
  short unaff_BX;
  int unaff_EDI;
  float10 fVar10;

  if (0x7f < *(short *)(unaff_EDI + 0x2c)) {
    return 0xffffffff;
  }
  fVar6 = *(float *)(unaff_EDI + 0x10) - *param_1;
  bVar8 = false;
  fVar7 = *(float *)(unaff_EDI + 0x14) - param_1[1];
  sVar4 = unaff_BX;
  do {
    if (sVar4 == -1) {
LAB_0043b6c4:
      sVar4 = *(short *)(unaff_EDI + 0x2c);
      *(short *)(unaff_EDI + 0x2c) = sVar4 + 1;
      pfVar3 = (float *)(unaff_EDI + 0x30 + sVar4 * 0x28);
      *pfVar3 = *param_1;
      pfVar3[1] = param_1[1];
      pfVar3[2] = param_2;
      pfVar3[3] = fVar6;
      pfVar3[4] = fVar7;
      fVar10 = (float10)vector2d_normalize_with_length();
      pfVar3[5] = (float)fVar10;
      *(short *)(pfVar3 + 6) = param_3;
      pfVar3[8] = (float)(fVar10 + (float10)param_5);
      *(byte *)((int)pfVar3 + 0x1a) = param_4;
      *(short *)(pfVar3 + 9) = unaff_BX;
      pfVar3[7] = -NAN;
      if ((bVar8) && (pfVar3[5] < *(float *)(unaff_EDI + 0x24))) {
        *(float *)(unaff_EDI + 0x24) = pfVar3[5];
        *(short *)(unaff_EDI + 0x20) = sVar4;
      }
      sVar5 = *(short *)(unaff_EDI + 0x1430);
      uVar9 = extraout_var;
      if (sVar5 < 0x80) {
        *(short *)(unaff_EDI + 0x1430) = sVar5 + 1;
        *(short *)(unaff_EDI + 0x1432 + sVar5 * 2) = sVar4;
        ai_search_heap_sift_up();
        uVar9 = extraout_var_00;
      }
      return CONCAT22(uVar9,sVar4);
    }
    iVar1 = unaff_EDI + 0x30 + sVar4 * 0x28;
    if (*(short *)(iVar1 + 0x18) != param_3) {
      if ((param_3 == *(short *)(unaff_EDI + 0x1c)) && (*(short *)(unaff_EDI + 0x1c) != -1)) {
        bVar8 = true;
        sVar4 = *(short *)(iVar1 + 0x1c + (uint)(param_4 == 0) * 2);
        if ((sVar4 != -1) &&
           ((iVar2 = unaff_EDI + 0x30 + sVar4 * 0x28,
            0.0 < fVar6 * *(float *)(iVar2 + 0xc) + fVar7 * *(float *)(iVar2 + 0x10) &&
            (pfVar3 = (float *)(unaff_EDI + 0x3c + unaff_BX * 0x28),
            (fVar7 * *(float *)(iVar2 + 0xc) - fVar6 * *(float *)(iVar2 + 0x10)) *
            (pfVar3[1] * *(float *)(iVar2 + 0xc) - *(float *)(iVar2 + 0x10) * *pfVar3) < 0.0)))) {
          return 0xffffffff;
        }
        sVar4 = *(short *)(iVar1 + 0x1c + (uint)param_4 * 2);
        if ((sVar4 == unaff_BX) || (sVar4 == -1)) {
          *(short *)(iVar1 + 0x1c + (uint)param_4 * 2) = *(short *)(unaff_EDI + 0x2c);
        }
      }
      goto LAB_0043b6c4;
    }
    if (*(byte *)(iVar1 + 0x1a) != param_4) {
      return 0xffffffff;
    }
    sVar4 = *(short *)(iVar1 + 0x24);
  } while( true );
}
#endif
