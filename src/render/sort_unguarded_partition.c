// sort_unguarded_partition  (Ghidra: FUN_00510500)
// address 0x510500, size 734 bytes (0x510500..0x5107dd)
// name confidence: 0.8   rewrite confidence: 0.8
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
// template: std::_Unguarded_partition(_First, _Last, _Pred): median (ninther) pivot, grows the run of
//   elements equal to the pivot [pfirst, plast), partitions the rest around it and returns that
//   equal run as a pair.
// register convention: plain cdecl with MSVC's hidden return-slot pointer first (0x510449 reads the pair back out
//   of the caller's frame; 0x5107cb..0x5107d7 stores [ecx] = pfirst, [ecx+4] = plast, returns ECX).
//   // blam-cc: stack -> (result, first, last, predicate), returns result

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

typedef struct rendered_particle_range { // std::pair<iterator, iterator>, returned through a hidden pointer
    rendered_particle_datum *first;
    rendered_particle_datum *second;
} rendered_particle_range;


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

// Partitions [first, last) around a median pivot and returns the range of elements equal to it.
rendered_particle_range *sort_unguarded_partition(rendered_particle_range *result,
    rendered_particle_datum *first, rendered_particle_datum *last, int32_t predicate)
{
    rendered_particle_datum *mid = first + (last - first) / 2;
    rendered_particle_datum *pfirst;
    rendered_particle_datum *plast;
    rendered_particle_datum *gfirst;
    rendered_particle_datum *glast;

    sort_median(first, mid, last - 1, predicate);
    pfirst = mid;
    plast = pfirst + 1;

    while (first < pfirst &&
        !(rendered_particle_compare(pfirst - 1, pfirst) < 0) &&
        !(rendered_particle_compare(pfirst, pfirst - 1) < 0)) {
        pfirst--;
    }
    while (plast < last &&
        !(rendered_particle_compare(plast, pfirst) < 0) &&
        !(rendered_particle_compare(pfirst, plast) < 0)) {
        plast++;
    }

    gfirst = plast;
    glast = pfirst;
    for (;;) {
        for (; gfirst < last; gfirst++) {
            if (rendered_particle_compare(pfirst, gfirst) < 0) {
                continue;
            } else if (rendered_particle_compare(gfirst, pfirst) < 0) {
                break;
            } else {
                rendered_particle_swap(plast++, gfirst);
            }
        }
        for (; first < glast; glast--) {
            if (rendered_particle_compare(glast - 1, pfirst) < 0) {
                continue;
            } else if (rendered_particle_compare(pfirst, glast - 1) < 0) {
                break;
            } else {
                rendered_particle_swap(--pfirst, glast - 1);
            }
        }
        if (glast == first && gfirst == last) {
            result->first = pfirst;
            result->second = plast;
            return result;
        }

        if (glast == first) {
            // no room at the bottom: rotate the pivot run upward
            if (plast != gfirst) {
                rendered_particle_swap(pfirst, plast);
            }
            plast++;
            rendered_particle_swap(pfirst++, gfirst++);
        } else if (gfirst == last) {
            // no room at the top: rotate the pivot run downward
            if (--glast != --pfirst) {
                rendered_particle_swap(glast, pfirst);
            }
            rendered_particle_swap(pfirst, --plast);
        } else {
            rendered_particle_swap(gfirst++, --glast);
        }
    }
}

#if 0
Original Ghidra decompilation (0x510500):

uint * FUN_00510500(uint *param_1,undefined4 *param_2,undefined4 *param_3,undefined4 param_4)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  int iVar6;
  short *psVar7;
  undefined4 *puVar8;
  undefined4 *puVar9;
  undefined4 *puVar10;
  undefined4 *local_10;
  undefined4 *local_c;
  
  puVar3 = param_2 + (((int)param_3 - (int)param_2 >> 3) - ((int)param_3 - (int)param_2 >> 0x1f) >>
                     1) * 2;
  FUN_005108f0(param_3 + -2,param_4);
  puVar8 = puVar3 + 2;
  if (param_2 < puVar3) {
    psVar7 = (short *)(puVar3 + -1);
    do {
      iVar6 = (int)psVar7[-1] - (int)psVar7[3];
      if ((iVar6 == 0) && (iVar6 = (int)*psVar7 - (int)psVar7[4], iVar6 == 0)) {
        iVar6 = (uint)*(byte *)(psVar7 + 1) - (uint)*(byte *)(psVar7 + 5);
      }
      if (iVar6 < 0) break;
      iVar6 = (int)psVar7[3] - (int)psVar7[-1];
      if ((iVar6 == 0) && (iVar6 = (int)psVar7[4] - (int)*psVar7, iVar6 == 0)) {
        iVar6 = (uint)*(byte *)(psVar7 + 5) - (uint)*(byte *)(psVar7 + 1);
      }
      if (iVar6 < 0) break;
      puVar3 = puVar3 + -2;
      psVar7 = psVar7 + -4;
    } while (param_2 < puVar3);
  }
  puVar9 = puVar8;
  local_10 = puVar8;
  local_c = puVar3;
  puVar4 = puVar3;
  if (puVar8 < param_3) {
    while( true ) {
      iVar6 = (int)*(short *)((int)puVar8 + 2) - (int)*(short *)((int)puVar3 + 2);
      if ((iVar6 == 0) &&
         (iVar6 = (int)*(short *)(puVar8 + 1) - (int)*(short *)(puVar3 + 1), iVar6 == 0)) {
        iVar6 = (uint)*(byte *)((int)puVar8 + 6) - (uint)*(byte *)((int)puVar3 + 6);
      }
      puVar9 = puVar8;
      local_10 = puVar8;
      if (iVar6 < 0) break;
      iVar6 = (int)*(short *)((int)puVar3 + 2) - (int)*(short *)((int)puVar8 + 2);
      if ((iVar6 == 0) &&
         (iVar6 = (int)*(short *)(puVar3 + 1) - (int)*(short *)(puVar8 + 1), iVar6 == 0)) {
        iVar6 = (uint)*(byte *)((int)puVar3 + 6) - (uint)*(byte *)((int)puVar8 + 6);
      }
      if ((iVar6 < 0) ||
         (puVar8 = puVar8 + 2, puVar9 = puVar8, local_10 = puVar8, param_3 <= puVar8)) break;
    }
  }
joined_r0x005105fc:
  do {
    if (param_3 <= puVar8) {
LAB_00510678:
      puVar10 = puVar3;
      if (param_2 < puVar3) {
        psVar7 = (short *)(puVar3 + -1);
        puVar3 = puVar4;
        do {
          iVar6 = (int)psVar7[-1] - (int)*(short *)((int)puVar3 + 2);
          if ((iVar6 == 0) &&
             (iVar6 = (int)*psVar7 - (int)*(short *)(puVar3 + 1), puVar9 = local_10, iVar6 == 0)) {
            iVar6 = (uint)*(byte *)(psVar7 + 1) - (uint)*(byte *)((int)puVar3 + 6);
          }
          puVar4 = puVar3;
          if (-1 < iVar6) {
            iVar6 = (int)*(short *)((int)puVar3 + 2) - (int)psVar7[-1];
            if ((iVar6 == 0) && (iVar6 = (int)*(short *)(puVar3 + 1) - (int)*psVar7, iVar6 == 0)) {
              iVar6 = (uint)*(byte *)((int)puVar3 + 6) - (uint)*(byte *)(psVar7 + 1);
            }
            puVar10 = local_c;
            if (iVar6 < 0) break;
            uVar1 = puVar3[-2];
            uVar2 = puVar3[-1];
            puVar4 = puVar3 + -2;
            *puVar4 = *(undefined4 *)(psVar7 + -2);
            puVar3[-1] = *(undefined4 *)psVar7;
            *(undefined4 *)(psVar7 + -2) = uVar1;
            *(undefined4 *)psVar7 = uVar2;
          }
          puVar10 = local_c + -2;
          psVar7 = psVar7 + -4;
          puVar3 = puVar4;
          local_c = puVar10;
        } while (param_2 < puVar10);
      }
      if (puVar10 == param_2) {
        if (puVar8 == param_3) {
          param_1[1] = (uint)puVar9;
          *param_1 = (uint)puVar4;
          return param_1;
        }
        if (puVar9 != puVar8) {
          uVar1 = *puVar4;
          uVar2 = puVar4[1];
          *puVar4 = *puVar9;
          puVar4[1] = puVar9[1];
          *puVar9 = uVar1;
          puVar9[1] = uVar2;
        }
        uVar1 = *puVar4;
        uVar2 = puVar4[1];
        *puVar4 = *puVar8;
        puVar4[1] = puVar8[1];
        puVar9 = puVar9 + 2;
        *puVar8 = uVar1;
        puVar8[1] = uVar2;
        puVar8 = puVar8 + 2;
        puVar3 = puVar10;
        local_10 = puVar9;
        puVar4 = puVar4 + 2;
      }
      else {
        puVar3 = puVar10 + -2;
        local_c = puVar3;
        if (puVar8 == param_3) {
          puVar5 = puVar4 + -2;
          if (puVar3 != puVar5) {
            uVar1 = *puVar3;
            uVar2 = puVar10[-1];
            *puVar3 = *puVar5;
            puVar10[-1] = puVar4[-1];
            *puVar5 = uVar1;
            puVar4[-1] = uVar2;
          }
          uVar1 = *puVar5;
          uVar2 = puVar4[-1];
          local_10 = puVar9 + -2;
          *puVar5 = puVar9[-2];
          puVar4[-1] = puVar9[-1];
          *local_10 = uVar1;
          puVar9[-1] = uVar2;
          puVar9 = local_10;
          puVar4 = puVar5;
        }
        else {
          uVar1 = *puVar8;
          uVar2 = puVar8[1];
          *puVar8 = *puVar3;
          puVar8[1] = puVar10[-1];
          *puVar3 = uVar1;
          puVar10[-1] = uVar2;
          puVar8 = puVar8 + 2;
        }
      }
      goto joined_r0x005105fc;
    }
    iVar6 = (int)*(short *)((int)puVar4 + 2) - (int)*(short *)((int)puVar8 + 2);
    if ((iVar6 == 0) &&
       (iVar6 = (int)*(short *)(puVar4 + 1) - (int)*(short *)(puVar8 + 1), puVar3 = local_c,
       iVar6 == 0)) {
      iVar6 = (uint)*(byte *)((int)puVar4 + 6) - (uint)*(byte *)((int)puVar8 + 6);
    }
    if (-1 < iVar6) {
      iVar6 = (int)*(short *)((int)puVar8 + 2) - (int)*(short *)((int)puVar4 + 2);
      if ((iVar6 == 0) &&
         (iVar6 = (int)*(short *)(puVar8 + 1) - (int)*(short *)(puVar4 + 1), iVar6 == 0)) {
        iVar6 = (uint)*(byte *)((int)puVar8 + 6) - (uint)*(byte *)((int)puVar4 + 6);
      }
      local_10 = puVar9;
      if (iVar6 < 0) goto LAB_00510678;
      uVar1 = *puVar9;
      uVar2 = puVar9[1];
      *puVar9 = *puVar8;
      puVar9[1] = puVar8[1];
      puVar8[1] = uVar2;
      puVar9 = puVar9 + 2;
      *puVar8 = uVar1;
      puVar3 = local_c;
    }
    puVar8 = puVar8 + 2;
    local_10 = puVar9;
  } while( true );
}
#endif
