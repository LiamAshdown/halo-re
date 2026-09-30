// sort_push_heap  (Ghidra: FUN_00510b20)
// address 0x510b20, size 122 bytes (0x510b20..0x510b99)
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: objdump -d -M intel bin/halo.exe 0x510410..0x510c4f. The ten functions 0x510410..0x510ba0 are
//   one MSVC 7.1 (Dinkumware 4.05) std::sort instantiation over rendered_particle_datum with a
//   comparison functor, called once from render_particles (0x50fd90, src/render/render_particles.c).
//   Every function was matched instruction by instruction against the 7.1 <algorithm> template it
//   instantiates (named below). Following the project convention for the shell hwreq STL code, the
//   instantiation is rewritten as plain C over the concrete element type.
//   The comparison is inlined everywhere as `compare(a, b) < 0`, compare = the difference of the
//   sign-extended words at +0x02 (definition_index, movsx), then +0x04 (cluster_index), then the
//   zero-extended byte at +0x06 (first_person); types/render.h documents the same key.
//   The functor itself is empty: it is passed by value as one never-read stack dword (`predicate`).
// template: std::_Push_heap(_First, _Hole, _Top, _Val, _Pred): move parents down while they order before
//   the value, then store it.
// register convention: first in ESI (left there by sort_adjust_heap), hole in EAX, then top, the value and the
//   functor on the stack (0x510afa..0x510b0a). The value arrives as two dwords: 0x510b45 reads
//   definition_index as `[esp+0x14] sar 16` (signed), cluster_index as the low word of the second
//   dword (`movsx edx,bx`) and first_person as its byte 2.
//   // blam-cc: ESI -> first, EAX -> hole, stack -> (top, value, predicate)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "effects.h"
#include "interface.h"
#include "render.h"
#include "fn_render.h"

// The inlined comparison of every function in this instantiation (see the file header).
static int32_t rendered_particle_compare(const rendered_particle_datum *a, const rendered_particle_datum *b)
{
    int32_t difference = (int32_t)(int16_t)a->definition_index - (int32_t)(int16_t)b->definition_index;

    if (difference == 0) {
        difference = (int32_t)a->cluster_index - (int32_t)b->cluster_index;
        if (difference == 0) {
            difference = (int32_t)a->first_person - (int32_t)b->first_person;
        }
    }
    return difference;
}

// Moves `value` up from `hole` (no higher than `top`) to its heap position.
void sort_push_heap(rendered_particle_datum *first, int32_t hole, int32_t top,
    rendered_particle_datum value, int32_t predicate)
{
    int32_t index;

    for (index = (hole - 1) / 2; top < hole && rendered_particle_compare(&first[index], &value) < 0;
        index = (hole - 1) / 2) {
        first[hole] = first[index];
        hole = index;
    }
    first[hole] = value;
}

#if 0
Original Ghidra decompilation (0x510b20):

void FUN_00510b20(int param_1,int param_2,uint param_3)

{
  int in_EAX;
  int iVar1;
  int iVar2;
  int unaff_ESI;
  
  while (param_1 < in_EAX) {
    iVar1 = (in_EAX + -1) / 2;
    iVar2 = (int)*(short *)(unaff_ESI + 2 + iVar1 * 8) - (param_2 >> 0x10);
    if ((iVar2 == 0) &&
       (iVar2 = (int)*(short *)(unaff_ESI + 4 + iVar1 * 8) - (int)(short)param_3, iVar2 == 0)) {
      iVar2 = (uint)*(byte *)(unaff_ESI + 6 + iVar1 * 8) - (param_3 >> 0x10 & 0xff);
    }
    if (-1 < iVar2) break;
    *(undefined4 *)(unaff_ESI + in_EAX * 8) = *(undefined4 *)(unaff_ESI + iVar1 * 8);
    *(undefined4 *)(unaff_ESI + 4 + in_EAX * 8) = *(undefined4 *)(unaff_ESI + 4 + iVar1 * 8);
    in_EAX = iVar1;
  }
  *(uint *)(unaff_ESI + 4 + in_EAX * 8) = param_3;
  *(int *)(unaff_ESI + in_EAX * 8) = param_2;
  return;
}
#endif
