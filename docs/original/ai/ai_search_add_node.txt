// ai_search_add_node  (Ghidra: ai_search_add_node, renamed)
// address 0x43b5a0, size 483 bytes
// name confidence: 0.45  rewrite confidence: 0.9 (VERIFIED against objdump 0x43b5a0..0x43b782; FIXED the ancestor walk: the binary rejects a node when an ancestor has the same point_id but a DIFFERENT side (jne at 0x43b5fc) and keeps walking when the side matches -- the C had it inverted, so every search result node (point -1, side 0, same as the root) was refused and every AI path failed (runtime trace a10 cryo tech))
// REWRITTEN from objdump 0x43b5a0..0x43b782. EDI = context, BX = the parent node (or -1); stack: position (2D),
//   surface index (stored at +0x08, typed z in types/ai.h), point_id, side, base cost. Refused (-1) past 0x80 nodes. With a parent, the chain of ancestors that share
//   the point id is walked: the same point on the same side is a duplicate (-1). At the first ancestor with
//   another point id, a point id equal to the goal (+0x1c, not -1) marks a goal node: if that ancestor's link on
//   the other side leads to a node facing the origin (its direction . this delta > 0) and the turn between the
//   parent's direction and it disagrees with this delta (product of the two cross products < 0), the node is
//   refused; that ancestor's link on this side is claimed when it is free or points back at the parent.
//   The node stores position, z, the delta to the origin (+0x10) and its length (0x4018e0 normalises it), cost =
//   length + base cost, point id, side, parent and no links; a goal node closer than the best (+0x24) becomes the
//   best (+0x20). It is pushed on the heap (sift up, EDX context, CX slot) while there is room. Returns its index.
// blam-cc: EDI -> context, BX -> parent, stack -> position, z, point_id, side, base_cost

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, ECX
extern void ai_search_heap_sift_up(ai_search_context *context, int16_t index); // 0x43b450, EDX context, CX index

int16_t ai_search_add_node(ai_search_context *context, int16_t parent, real_point2d *position, int32_t surface_index,
    int16_t point_id, uint8_t side, float base_cost)
{
    float dx;
    float dy;
    uint8_t goal = 0;
    int16_t index;
    ai_search_node *node;

    if (context->node_count >= 0x80) {
        return -1;
    }
    dx = context->origin.x - position->x;
    dy = context->origin.y - position->y;
    if (parent != -1) {
        int16_t walk = parent;
        ai_search_node *ancestor;

        for (;;) {
            ancestor = &context->nodes[walk];
            if (ancestor->point_id != point_id) {
                break;
            }
            if (ancestor->side != side) { // 0x43b5f9: cmp [ecx+0x1a],al; jne reject (was inverted)
                return -1;
            }
            walk = ancestor->parent;
            if (walk == -1) {
                ancestor = 0;
                break;
            }
        }
        if (ancestor != 0 && point_id == context->goal_point_id && context->goal_point_id != -1) {
            int16_t *links = &ancestor->side_link;
            int16_t other = links[side == 0];

            goal = 1;
            if (other != -1) {
                ai_search_node *linked = &context->nodes[other];

                if (dy * linked->direction.j + dx * linked->direction.i > 0.0f) {
                    real_vector2d *parent_direction = &context->nodes[parent].direction;
                    float turn = parent_direction->j * linked->direction.i - linked->direction.j * parent_direction->i;
                    float delta_turn = dy * linked->direction.i - dx * linked->direction.j;

                    if (delta_turn * turn < 0.0f) {
                        return -1;
                    }
                }
            }
            if (links[side] == parent || links[side] == -1) {
                links[side] = context->node_count;
            }
        }
    }

    index = context->node_count++;
    node = &context->nodes[index];
    node->position = *position;
    *(int32_t *)&node->z = surface_index; // +0x08 holds a surface index (0x43b6ed dword copy)
    node->direction.i = dx;
    node->direction.j = dy;
    node->length = vector2d_normalize_with_length(&node->direction);
    node->cost = node->length + base_cost;
    node->point_id = point_id;
    node->side = side;
    node->parent = parent;
    (&node->side_link)[0] = -1;
    (&node->side_link)[1] = -1;
    if (goal && node->length < context->best_cost) {
        context->best_cost = node->length;
        context->best_node = index;
    }
    if (context->heap_count < 0x80) {
        int16_t slot = context->heap_count++;

        context->heap[slot] = index;
        ai_search_heap_sift_up(context, slot);
    }
    return index;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
