// path_find_set_avoid_sphere  (Ghidra: path_find_set_avoid_sphere, renamed)
// address 0x43a070, size 47 bytes
// name confidence: 0.55  rewrite confidence: 0.55
// evidence: types/ai.h path_find_request.have_avoid_sphere(+0x24)/avoid_position(+0x28)/
//   avoid_object_index(+0x34)/avoid_radius(+0x38)/avoid_weight(+0x3c) -- this function
//   writes exactly those five fields (through path_find_context's copy of the request block,
//   whose leading 0x48 bytes are that same layout) and nothing else. Confirmed further by
//   path_find_score_avoidance_penalty @0x43b3b0 (this rewrite) and path_find_run's own
//   obstacle-avoidance branch (this rewrite), both of which read +0x28/+0x38/+0x3c back as
//   avoid_position/avoid_radius/avoid_weight.
// register convention: EAX -> context, ECX -> position (a real_point3d the caller owns);
//   stack -> avoid_radius, avoid_object_index, avoid_weight (note the swap: the first stack
//   argument becomes avoid_radius at +0x38, the second becomes avoid_object_index at +0x34).
//   // blam-cc: EAX -> context, ECX -> position, stack -> avoid_radius, avoid_object_index,
//   //   avoid_weight

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// blam-cc: EAX -> context, ECX -> position, stack -> avoid_radius, avoid_object_index, avoid_weight
void path_find_set_avoid_sphere(path_find_context *context, const real_point3d *position,
                                float avoid_radius, datum_index avoid_object_index, float avoid_weight)
{
    path_find_request *request = (path_find_request *)context;

    request->have_avoid_sphere = 1;
    request->avoid_position = *position;
    request->avoid_radius = avoid_radius; // note: stored via the swapped stack slot, see header
    request->avoid_object_index = avoid_object_index;
    request->avoid_weight = avoid_weight;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043a070 @ 0x43a070) ----
void FUN_0043a070(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int in_EAX;
  undefined4 *in_ECX;

  *(undefined1 *)(in_EAX + 0x24) = 1;
  *(undefined4 *)(in_EAX + 0x28) = *in_ECX;
  *(undefined4 *)(in_EAX + 0x2c) = in_ECX[1];
  *(undefined4 *)(in_EAX + 0x30) = in_ECX[2];
  *(undefined4 *)(in_EAX + 0x38) = param_1;
  *(undefined4 *)(in_EAX + 0x34) = param_2;
  *(undefined4 *)(in_EAX + 0x3c) = param_3;
  return;
}
#endif
