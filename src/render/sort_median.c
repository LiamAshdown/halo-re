// sort_median  (Ghidra: FUN_005108f0)
// address 0x5108f0, size 140 bytes (0x5108f0..0x51097b)
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
// template: std::_Median(_First, _Mid, _Last, _Pred): Tukey's ninther for more than 40 elements
//   (step = (count + 1) / 8, four _Med3 calls), else one _Med3.
// register convention: first in ECX, mid in EBX, last and the functor on the stack (the partition 0x510522..0x510529:
//   `push pred; lea ecx,[edi-0x8]; push ecx; mov ecx,esi` with EBX = mid).
//   // blam-cc: ECX -> first, EBX -> mid, stack -> (last, predicate)

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
extern void sort_median_of_three(rendered_particle_datum *first, rendered_particle_datum *mid,
    rendered_particle_datum *last, int32_t predicate); // 0x5109d0, blam-cc: ECX first, EAX mid, EDX last, stack predicate

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

// Orders first, mid and last (a ninther of nine samples for long ranges) so that *mid is the median.
void sort_median(rendered_particle_datum *first, rendered_particle_datum *mid, rendered_particle_datum *last,
    int32_t predicate)
{
    int32_t count = (int32_t)(last - first);

    if (count > 40) {
        int32_t step = (count + 1) / 8;

        sort_median_of_three(first, first + step, first + 2 * step, predicate);
        sort_median_of_three(mid - step, mid, mid + step, predicate);
        sort_median_of_three(last - 2 * step, last - step, last, predicate);
        sort_median_of_three(first + step, mid, last - step, predicate);
    } else {
        sort_median_of_three(first, mid, last, predicate);
    }
}

#if 0
Original Ghidra decompilation (0x5108f0):

void FUN_005108f0(int param_1,undefined4 param_2)

{
  int in_ECX;
  
  if (0x28 < param_1 - in_ECX >> 3) {
    FUN_005109d0(param_2);
    FUN_005109d0(param_2);
    FUN_005109d0(param_2);
    FUN_005109d0(param_2);
    return;
  }
  FUN_005109d0(param_2);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
