// sort_rotate  (Ghidra: FUN_00510ba0)
// address 0x510ba0, size 173 bytes (0x510ba0..0x510c4f)
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
// template: std::_Rotate(_First, _Mid, _Last) for random-access iterators: gcd(shift, count) cycles, each
//   percolated around once. No comparisons, so no functor argument.
// register convention: mid in EAX, last in EBX, first as the one stack argument (0x510883..0x510886: `push edi;
//   mov eax,esi` with EBX = esi + 8).
//   // blam-cc: EAX -> mid, EBX -> last, stack -> first

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "effects.h"
#include "interface.h"
#include "render.h"

// Rotates [first, last) so that *mid becomes the first element.
void sort_rotate(rendered_particle_datum *first, rendered_particle_datum *mid, rendered_particle_datum *last)
{
    int32_t shift = (int32_t)(mid - first);
    int32_t count = (int32_t)(last - first);
    int32_t factor;

    for (factor = shift; factor != 0; ) {
        int32_t remainder = count % factor;
        count = factor;
        factor = remainder;
    }

    if (count < last - first) {
        for (; count > 0; count--) {
            rendered_particle_datum *hole = first + count;
            rendered_particle_datum *next = hole;
            rendered_particle_datum hole_value = *hole;
            rendered_particle_datum *next1 = (next + shift == last) ? first : next + shift;

            while (next1 != hole) {
                *next = *next1;
                next = next1;
                next1 = (shift < last - next1) ? next1 + shift : first + (shift - (last - next1));
            }
            *next = hole_value;
        }
    }
}

#if 0
Original Ghidra decompilation (0x510ba0):

void FUN_00510ba0(undefined4 *param_1)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  int iVar6;
  undefined4 *puVar7;
  int in_EAX;
  int iVar8;
  int iVar9;
  undefined4 *unaff_EBX;
  undefined4 *puVar10;
  int iVar11;
  
  puVar5 = param_1;
  iVar8 = (int)unaff_EBX - (int)param_1 >> 3;
  iVar11 = in_EAX - (int)param_1 >> 3;
  iVar6 = iVar11;
  iVar9 = iVar8;
  while (iVar3 = iVar6, iVar3 != 0) {
    iVar6 = iVar9 % iVar3;
    iVar9 = iVar3;
  }
  if ((iVar9 < iVar8) && (0 < iVar9)) {
    puVar10 = param_1 + iVar9 * 2;
    param_1 = (undefined4 *)iVar9;
    do {
      uVar1 = *puVar10;
      uVar2 = puVar10[1];
      puVar7 = puVar10 + iVar11 * 2;
      puVar4 = puVar10;
      if (puVar10 + iVar11 * 2 == unaff_EBX) {
        puVar7 = puVar5;
      }
      while (puVar7 != puVar10) {
        *puVar4 = *puVar7;
        puVar4[1] = puVar7[1];
        iVar9 = (int)unaff_EBX - (int)puVar7 >> 3;
        puVar4 = puVar7;
        if (iVar11 < iVar9) {
          puVar7 = puVar7 + iVar11 * 2;
        }
        else {
          puVar7 = puVar5 + (iVar11 - iVar9) * 2;
        }
      }
      puVar4[1] = uVar2;
      puVar10 = puVar10 + -2;
      param_1 = (undefined4 *)((int)param_1 + -1);
      *puVar4 = uVar1;
    } while (param_1 != (undefined4 *)0x0);
  }
  return;
}
#endif
