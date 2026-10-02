// sort_median_of_three  (Ghidra: FUN_005109d0)
// address 0x5109d0, size 183 bytes (0x5109d0..0x510a86)
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
// template: std::_Med3(_First, _Mid, _Last, _Pred): three compare-and-swaps.
// register convention: first in ECX, mid in EAX, last in EDX (moved to ESI at 0x5109d1), the functor on the stack
//   (all five calls in sort_median set exactly these three registers).
//   // blam-cc: ECX -> first, EAX -> mid, EDX -> last, stack -> predicate

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

static void rendered_particle_swap(rendered_particle_datum *a, rendered_particle_datum *b)
{
    rendered_particle_datum temporary = *a;
    *a = *b;
    *b = temporary;
}

// Sorts the three elements *first, *mid, *last.
void sort_median_of_three(rendered_particle_datum *first, rendered_particle_datum *mid,
    rendered_particle_datum *last, int32_t predicate)
{
    if (rendered_particle_compare(mid, first) < 0) {
        rendered_particle_swap(mid, first);
    }
    if (rendered_particle_compare(last, mid) < 0) {
        rendered_particle_swap(last, mid);
    }
    if (rendered_particle_compare(mid, first) < 0) {
        rendered_particle_swap(mid, first);
    }
}

#if 0
Original Ghidra decompilation (0x5109d0):

void FUN_005109d0(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 *in_EAX;
  undefined4 *in_ECX;
  undefined4 *in_EDX;
  int iVar3;
  
  iVar3 = (int)*(short *)((int)in_EAX + 2) - (int)*(short *)((int)in_ECX + 2);
  if ((iVar3 == 0) &&
     (iVar3 = (int)*(short *)(in_EAX + 1) - (int)*(short *)(in_ECX + 1), iVar3 == 0)) {
    iVar3 = (uint)*(byte *)((int)in_EAX + 6) - (uint)*(byte *)((int)in_ECX + 6);
  }
  if (iVar3 < 0) {
    uVar1 = *in_EAX;
    uVar2 = in_EAX[1];
    *in_EAX = *in_ECX;
    in_EAX[1] = in_ECX[1];
    *in_ECX = uVar1;
    in_ECX[1] = uVar2;
  }
  iVar3 = (int)*(short *)((int)in_EDX + 2) - (int)*(short *)((int)in_EAX + 2);
  if ((iVar3 == 0) &&
     (iVar3 = (int)*(short *)(in_EDX + 1) - (int)*(short *)(in_EAX + 1), iVar3 == 0)) {
    iVar3 = (uint)*(byte *)((int)in_EDX + 6) - (uint)*(byte *)((int)in_EAX + 6);
  }
  if (iVar3 < 0) {
    uVar1 = *in_EDX;
    uVar2 = in_EDX[1];
    *in_EDX = *in_EAX;
    in_EDX[1] = in_EAX[1];
    *in_EAX = uVar1;
    in_EAX[1] = uVar2;
  }
  iVar3 = (int)*(short *)((int)in_EAX + 2) - (int)*(short *)((int)in_ECX + 2);
  if ((iVar3 == 0) &&
     (iVar3 = (int)*(short *)(in_EAX + 1) - (int)*(short *)(in_ECX + 1), iVar3 == 0)) {
    iVar3 = (uint)*(byte *)((int)in_EAX + 6) - (uint)*(byte *)((int)in_ECX + 6);
  }
  if (iVar3 < 0) {
    uVar1 = *in_EAX;
    uVar2 = in_EAX[1];
    *in_EAX = *in_ECX;
    in_EAX[1] = in_ECX[1];
    *in_ECX = uVar1;
    in_ECX[1] = uVar2;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
