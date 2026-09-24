// sort_introsort_loop  (Ghidra: sort_introsort_loop, already named)
// address 0x510410, size 235 bytes (0x510410..0x5104fa)
// name confidence: 0.8 (Ghidra/phase-2 name, kept: it is exactly the std::_Sort introsort loop)   rewrite confidence: 0.85
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
// template: std::_Sort(_First, _Last, _Ideal, _Pred): quicksort while more than _ISORT_MAX (32) elements
//   remain and the division budget lasts (_Ideal = _Ideal / 2 + _Ideal / 4 per split), recursing
//   into the smaller half; heap sort once the budget runs out; insertion sort for 2..32 elements.
// register convention: plain cdecl (render_particles pushes first, last, count, functor).
//   // blam-cc: stack -> (first, last, ideal, predicate)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "effects.h"
#include "interface.h"
#include "render.h"

typedef struct rendered_particle_range { // std::pair<iterator, iterator>, returned through a hidden pointer
    rendered_particle_datum *first;
    rendered_particle_datum *second;
} rendered_particle_range;

extern rendered_particle_range *sort_unguarded_partition(rendered_particle_range *result,
    rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate); // 0x510500, cdecl
extern void sort_heap_sort(rendered_particle_datum *first, rendered_particle_datum *last,
    int32_t predicate); // 0x5107e0, blam-cc: EBX first, EAX last, stack predicate
extern void sort_insertion_sort(rendered_particle_datum *first, rendered_particle_datum *last,
    int32_t predicate); // 0x510830, cdecl
extern void sort_make_heap(rendered_particle_datum *first, rendered_particle_datum *last,
    int32_t predicate); // 0x510980, blam-cc: EAX last, stack (first, predicate)

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

// Sorts [first, last) by rendered_particle_compare. `ideal` is the remaining quicksort division
// budget (the caller passes the element count).
void sort_introsort_loop(rendered_particle_datum *first, rendered_particle_datum *last, int32_t ideal,
    int32_t predicate)
{
    int32_t count;
    rendered_particle_range mid;

    for (count = (int32_t)(last - first); count > 32 && ideal > 0; count = (int32_t)(last - first)) {
        sort_unguarded_partition(&mid, first, last, predicate);
        ideal = ideal / 2;
        ideal += ideal / 2;

        if (mid.first - first < last - mid.second) {
            sort_introsort_loop(first, mid.first, ideal, predicate);
            first = mid.second;
        } else {
            sort_introsort_loop(mid.second, last, ideal, predicate);
            last = mid.first;
        }
    }

    if (count > 32) {
        if (last - first > 1) {   // std::make_heap's own guard, inlined at 0x5104c8
            sort_make_heap(first, last, predicate);
        }
        sort_heap_sort(first, last, predicate);
    } else if (count > 1) {
        sort_insertion_sort(first, last, predicate);
    }
}

#if 0
Original Ghidra decompilation (0x510410):

void sort_introsort_loop(int param_1,int param_2,int param_3,undefined4 param_4)

{
  int iVar1;
  int local_8;
  int local_4;
  
  iVar1 = param_2 - param_1;
  do {
    iVar1 = iVar1 >> 3;
    if (iVar1 < 0x21) {
LAB_005104a7:
      if (1 < iVar1) {
        FUN_00510830(param_1,param_2,param_4);
      }
      return;
    }
    if (param_3 < 1) {
      if (0x20 < iVar1) {
        if (8 < (int)(param_2 - param_1 & 0xfffffff8U)) {
          FUN_00510980(param_1,param_4);
        }
        FUN_005107e0(param_4);
        return;
      }
      goto LAB_005104a7;
    }
    FUN_00510500(&local_8,param_1,param_2,param_4);
    iVar1 = local_4;
    param_3 = param_3 / 2 + (param_3 / 2) / 2;
    if ((int)(local_8 - param_1 & 0xfffffff8U) < (int)(param_2 - local_4 & 0xfffffff8U)) {
      sort_introsort_loop(param_1,local_8,param_3,param_4);
      param_1 = iVar1;
    }
    else {
      sort_introsort_loop(local_4,param_2,param_3,param_4);
      param_2 = local_8;
    }
    iVar1 = param_2 - param_1;
  } while( true );
}
#endif
