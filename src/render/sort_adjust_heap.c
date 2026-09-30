// sort_adjust_heap  (Ghidra: FUN_00510a90)
// address 0x510a90, size 133 bytes (0x510a90..0x510b14)
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
// template: std::_Adjust_heap(_First, _Hole, _Bottom, _Val, _Pred): sift the hole down to a leaf along
//   the larger child, then _Push_heap the value back up to its place.
// register convention: first in ECX (moved to ESI and left there for the tail call, which is how sort_push_heap
//   receives it), hole in EAX, bottom in EDI, the 8-byte value (two dwords) and the functor on the
//   stack (callers 0x5109a0..0x5109b5 and 0x5107f5..0x510817).
//   // blam-cc: ECX -> first, EAX -> hole, EDI -> bottom, stack -> (value, predicate)

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

// Re-establishes the heap property below `hole` for a heap of `bottom` elements, storing value.
void sort_adjust_heap(rendered_particle_datum *first, int32_t hole, int32_t bottom,
    rendered_particle_datum value, int32_t predicate)
{
    int32_t top = hole;
    int32_t index;

    for (index = 2 * hole + 2; index < bottom; index = 2 * index + 2) {
        if (rendered_particle_compare(&first[index], &first[index - 1]) < 0) {
            index--;
        }
        first[hole] = first[index];
        hole = index;
    }
    if (index == bottom) {
        first[hole] = first[bottom - 1];
        hole = bottom - 1;
    }
    sort_push_heap(first, hole, top, value, predicate);
}

#if 0
Original Ghidra decompilation (0x510a90):

void FUN_00510a90(void)

{
  int in_EAX;
  int in_ECX;
  int iVar1;
  int iVar2;
  int unaff_EDI;
  
  while( true ) {
    iVar1 = in_EAX * 2 + 2;
    if (unaff_EDI <= iVar1) break;
    iVar2 = (int)*(short *)(in_ECX + 2 + iVar1 * 8) - (int)*(short *)(in_ECX + -6 + iVar1 * 8);
    if ((iVar2 == 0) &&
       (iVar2 = (int)*(short *)(in_ECX + 4 + iVar1 * 8) - (int)*(short *)(in_ECX + -4 + iVar1 * 8),
       iVar2 == 0)) {
      iVar2 = (uint)*(byte *)(in_ECX + 6 + iVar1 * 8) - (uint)*(byte *)(in_ECX + -2 + iVar1 * 8);
    }
    if (iVar2 < 0) {
      iVar1 = in_EAX * 2 + 1;
    }
    *(undefined4 *)(in_ECX + in_EAX * 8) = *(undefined4 *)(in_ECX + iVar1 * 8);
    *(undefined4 *)(in_ECX + 4 + in_EAX * 8) = *(undefined4 *)(in_ECX + 4 + iVar1 * 8);
    in_EAX = iVar1;
  }
  if (iVar1 == unaff_EDI) {
    *(undefined4 *)(in_ECX + in_EAX * 8) = *(undefined4 *)(in_ECX + -8 + unaff_EDI * 8);
    *(undefined4 *)(in_ECX + 4 + in_EAX * 8) = *(undefined4 *)(in_ECX + -4 + unaff_EDI * 8);
  }
  FUN_00510b20();
  return;
}
#endif
