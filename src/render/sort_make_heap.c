// sort_make_heap  (Ghidra: FUN_00510980)
// address 0x510980, size 70 bytes (0x510980..0x5109c5)
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
// template: std::_Make_heap(_First, _Last, _Pred): _Adjust_heap from the last parent down to the root.
// register convention: last in EAX, first and the functor on the stack (0x5104d4..0x5104dc).
//   // blam-cc: EAX -> last, stack -> (first, predicate)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "effects.h"
#include "interface.h"
#include "render.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

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

// Arranges [first, last) into a heap.
void sort_make_heap(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate)
{
    int32_t bottom = (int32_t)(last - first);
    int32_t hole;

    for (hole = bottom / 2; hole > 0; ) {
        hole--;
        sort_adjust_heap(first, hole, bottom, first[hole], predicate);
    }
}

#if 0
Original Ghidra decompilation (0x510980):

void FUN_00510980(int param_1,undefined4 param_2)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  int in_EAX;
  int iVar3;
  
  iVar3 = (in_EAX - param_1 >> 3) - (in_EAX - param_1 >> 0x1f) >> 1;
  if (0 < iVar3) {
    param_1 = param_1 + iVar3 * 8;
    do {
      puVar1 = (undefined4 *)(param_1 + -4);
      puVar2 = (undefined4 *)(param_1 + -8);
      param_1 = param_1 + -8;
      iVar3 = iVar3 + -1;
      FUN_00510a90(*puVar2,*puVar1,param_2);
    } while (0 < iVar3);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
