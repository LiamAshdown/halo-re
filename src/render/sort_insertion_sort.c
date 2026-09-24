// sort_insertion_sort  (Ghidra: FUN_00510830)
// address 0x510830, size 191 bytes (0x510830..0x5108ee)
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
// template: std::_Insertion_sort(_First, _Last, _Pred) of VC 7.1, which inserts with std::rotate instead
//   of copy_backward: a new minimum is rotated to the front, anything else is rotated back to the
//   insertion point an unguarded backward scan finds.
// register convention: plain cdecl (0x5104ac..0x5104b3).
//   // blam-cc: stack -> (first, last, predicate)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "effects.h"
#include "interface.h"
#include "render.h"

extern void sort_rotate(rendered_particle_datum *first, rendered_particle_datum *mid,
    rendered_particle_datum *last); // 0x510ba0, blam-cc: EAX mid, EBX last, stack first

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

// Sorts the short range [first, last) by insertion.
void sort_insertion_sort(rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate)
{
    rendered_particle_datum *next;
    rendered_particle_datum *destination;
    rendered_particle_datum *candidate;

    if (first == last) {
        return;
    }
    for (next = first + 1; next != last; next++) {
        if (rendered_particle_compare(next, first) < 0) {
            // a new earliest element: rotate it to the front (std::rotate's own guard inlined)
            if (first != next && next != next + 1) {
                sort_rotate(first, next, next + 1);
            }
        } else {
            destination = next;
            for (candidate = next - 1; rendered_particle_compare(next, candidate) < 0; candidate--) {
                destination = candidate;
            }
            if (destination != next && next != next + 1) {
                sort_rotate(destination, next, next + 1);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x510830):

void FUN_00510830(short *param_1,short *param_2)

{
  short *psVar1;
  int iVar2;
  short *psVar3;
  short *psVar4;
  
  psVar1 = param_1;
  if (param_1 != param_2) {
    while (psVar1 = psVar1 + 4, psVar1 != param_2) {
      iVar2 = (int)psVar1[1] - (int)param_1[1];
      if ((iVar2 == 0) && (iVar2 = (int)psVar1[2] - (int)param_1[2], iVar2 == 0)) {
        iVar2 = (uint)*(byte *)(psVar1 + 3) - (uint)*(byte *)(param_1 + 3);
      }
      if (iVar2 < 0) {
        if ((param_1 != psVar1) && (psVar1 != psVar1 + 4)) {
          FUN_00510ba0(param_1);
        }
      }
      else {
        psVar3 = psVar1 + 2;
        psVar4 = psVar1;
        while( true ) {
          iVar2 = (int)psVar1[1] - (int)psVar3[-5];
          if ((iVar2 == 0) && (iVar2 = (int)psVar1[2] - (int)psVar3[-4], iVar2 == 0)) {
            iVar2 = (uint)*(byte *)(psVar1 + 3) - (uint)*(byte *)(psVar3 + -3);
          }
          if (-1 < iVar2) break;
          psVar4 = psVar3 + -6;
          psVar3 = psVar3 + -4;
        }
        if ((psVar4 != psVar1) && (psVar1 != psVar1 + 4)) {
          FUN_00510ba0(psVar4);
        }
      }
    }
  }
  return;
}
#endif
