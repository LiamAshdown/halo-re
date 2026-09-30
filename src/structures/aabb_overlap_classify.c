// aabb_overlap_classify  (Ghidra: FUN_005541b0, already named by an earlier pass)
// address 0x5541b0, size 175 bytes
// name confidence: 0.9 -- already named in the phase4 function list and matches its body exactly:
//   a symmetric two-box overlap/containment test with no other plausible role.
// rewrite confidence: 0.95 -- a small, pure, branch-free (aside from the two returns) function;
//   the only judgment call is which box is "self" (ECX) and which is "other" (EDX), resolved by
//   0x00553c40's call site (ECX = the candidate subcluster box just built off the tag, EDX = the
//   caller's query box) and by structure_bsp_overlap's own doc comment ("2 means no clipping
//   needed").
// evidence: types/structures.h structure_bsp_overlap enum; caller 0x00553c40 (this batch) loads
//   ECX from a freshly-computed ScenarioStructureBSPSubcluster bounds pointer and EDX from its own
//   incoming query-box argument (verified by disassembly of 0x553c40).
// register convention: in_ECX -> box_a, in_EDX -> box_b. The roles are NOT fixed: at
//   0x553c40 ECX is the candidate subcluster box and EDX the query box, while at 0x553f10 and
//   0x5540c0 it is the other way round (ECX = query box, EDX = the node/leaf box). Only the
//   containment branch is asymmetric, and only the 0x553f10 / 0x5540c0 pair reads the
//   _contained result, where "box_a encloses box_b" == "the query encloses this node" is
//   exactly the condition that lets the subtree skip further testing. No stack
//   parameters, no return-register mismatch.
//   // blam-cc: ECX -> box_a, EDX -> box_b
// UNSURE: which of the two boxes is logically "the candidate" vs "the query" is a naming choice;
//   the arithmetic is symmetric except for the containment branch, which is unambiguous from the
//   code (self contains other iff self.min <= other.min and other.max <= self.max on every axis).

#include "tags.h"
#include "math.h"
#include "memory.h"
#include "structures.h"
#include "fn_structures.h"

// blam-cc: ECX -> box_a, EDX -> box_b
structure_bsp_overlap aabb_overlap_classify(real_rectangle3d *box_a, real_rectangle3d *box_b)
{
    if (box_b->x.lower <= box_a->x.upper && box_a->x.lower <= box_b->x.upper &&
        box_b->y.lower <= box_a->y.upper && box_a->y.lower <= box_b->y.upper &&
        box_b->z.lower <= box_a->z.upper && box_a->z.lower <= box_b->z.upper) {
        if (box_a->x.lower <= box_b->x.lower && box_b->x.upper <= box_a->x.upper &&
            box_a->y.lower <= box_b->y.lower && box_b->y.upper <= box_a->y.upper &&
            box_a->z.lower <= box_b->z.lower && box_b->z.upper <= box_a->z.upper) {
            return _structure_bsp_overlap_contained;
        }
        return _structure_bsp_overlap_partial;
    }
    return _structure_bsp_overlap_none;
}

#if 0
Original Ghidra decompilation (0x5541b0):

undefined4 aabb_overlap_classify(void)

{
  float *in_ECX;
  float *in_EDX;

  if ((((*in_EDX <= in_ECX[1]) && (*in_ECX <= in_EDX[1])) && (in_EDX[2] <= in_ECX[3])) &&
     (((in_ECX[2] <= in_EDX[3] && (in_EDX[4] <= in_ECX[5])) && (in_ECX[4] <= in_EDX[5])))) {
    if (((*in_ECX <= *in_EDX) && (in_EDX[1] <= in_ECX[1])) &&
       ((in_ECX[2] <= in_EDX[2] &&
        (((in_EDX[3] <= in_ECX[3] && (in_ECX[4] <= in_EDX[4])) && (in_EDX[5] <= in_ECX[5])))))) {
      return 2;
    }
    return 1;
  }
  return 0;
}
#endif
