// sort_heap_sort  (Ghidra: FUN_005107e0)
// address 0x5107e0, size 80 bytes (0x5107e0..0x51082f)
// name confidence: 0.75   rewrite confidence: 0.85
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
// template: std::sort_heap(_First, _Last, _Pred) with std::_Pop_heap inlined: repeatedly moves the heap
//   top to the end and re-heaps the rest with _Adjust_heap.
// register convention: first in EBX, last in EAX (`mov esi,eax; sub esi,ebx` at entry; the caller 0x5104e9 loads
//   EAX = last and keeps first in EBX), the functor on the stack.
//   // blam-cc: EBX -> first, EAX -> last, stack -> predicate

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "effects.h"
#include "interface.h"
#include "render.h"

extern void sort_adjust_heap(rendered_particle_datum *first, int32_t hole, int32_t bottom,
    rendered_particle_datum value, int32_t predicate); // 0x510a90, blam-cc: ECX first, EAX hole, EDI bottom, stack (value, predicate)

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

// Turns the heap [first, last) into a sorted range.
void sort_heap_sort(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate)
{
    for (; last - first > 1; last--) {
        rendered_particle_datum value = last[-1];
        last[-1] = *first;
        sort_adjust_heap(first, 0, (int32_t)(last - 1 - first), value, predicate);
    }
}

#if 0
Original Ghidra decompilation (0x5107e0):

void FUN_005107e0(undefined4 param_1)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int in_EAX;
  undefined4 *unaff_EBX;
  int iVar3;
  
  for (iVar3 = in_EAX - (int)unaff_EBX; 1 < iVar3 >> 3; iVar3 = iVar3 + -8) {
    uVar1 = *(undefined4 *)((int)unaff_EBX + iVar3 + -4);
    uVar2 = *(undefined4 *)((int)unaff_EBX + iVar3 + -8);
    *(undefined4 *)((int)unaff_EBX + iVar3 + -8) = *unaff_EBX;
    *(undefined4 *)((int)unaff_EBX + iVar3 + -4) = unaff_EBX[1];
    FUN_00510a90(uVar2,uVar1,param_1);
  }
  return;
}
#endif
