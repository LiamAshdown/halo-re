// render_contrails  (Ghidra: contrail_render_by_object_type_mask, phase-2 name; CEA
// render_contrails(render_type_flags), hint only; renamed)
// address 0x50df20, size 222 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: objdump -d -M intel 0x50df20..0x50dffd. EDI is loaded from contrail_data
//   (0x0087abec, types/effects.h) right before the first datum_next 0x4d0630 (EDX = -1); the
//   second walk at the bottom is datum_next inlined. The mask is a cdecl stack argument
//   ([esp+0x18] after the five pushes, the callers push it: render_window 0x50c43b pushes
//   0xfffffff3), tested against 1 << Contrail.render_type (the byte at +0x18, shifted by CL, so
//   only its low 5 bits count): the bits select Contrail render types, not object types, hence
//   the CEA name. Each of the four point lists with at least two points goes to render_contrail
//   0x50e090 (cdecl: contrail, definition, instance).
// review fix (phase-4 gate): the mask is a stack argument, not EAX; the phase-2 name and the
//   object_type_mask parameter name were wrong about what the bits select.
// register convention: cdecl, one stack argument.
//   // blam-cc: cdecl
// UNSURE: nothing.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *contrail_data;   // 0x0087abec, effects module
extern tag_instance *tag_instances; // 0x0087bc14, cache module
extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module;
                                                                       // blam-cc: DX=after_index, EDI=array

extern void render_contrail(contrail *c, Contrail *definition, int16_t instance); // 0x50e090,
                                                                                 // this module (cdecl)

// Draws every live contrail whose Contrail render type is selected by render_type_flags, one
// point list (of the four marker permutations) at a time.
void render_contrails(uint32_t render_type_flags) // blam-cc: cdecl
{
    datum_index index = datum_next(-1, contrail_data);

    while (index != k_datum_index_none) {
        contrail *c = &((contrail *)contrail_data->data)[(uint16_t)index];
        Contrail *definition = (Contrail *)tag_instances[(uint16_t)c->definition_index].data;
        int16_t i;

        for (i = 0; i < 4; i++) {
            if ((render_type_flags & (1u << ((uint8_t)definition->render_type & 0x1f))) != 0 &&
                c->point_count[i] >= 2) {
                render_contrail(c, definition, i);
            }
        }

        index = datum_next((int16_t)index, contrail_data);
    }
}

#if 0
Original Ghidra decompilation (0x50df20):

void contrail_render_by_object_type_mask(uint param_1)

{
  uint uVar1;
  short *psVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  uint uVar7;

  uVar1 = datum_next();
  do {
    do {
      if (uVar1 == 0xffffffff) {
        return;
      }
      iVar5 = (uVar1 & 0xffff) * 0x44;
      iVar6 = iVar5 + *(int *)(DAT_0087abec + 0x34);
      iVar5 = *(int *)((*(uint *)(iVar5 + 4 + *(int *)(DAT_0087abec + 0x34)) & 0xffff) * 0x20 + 0x14
                      + DAT_0087bc14);
      iVar4 = 0;
      psVar2 = (short *)(iVar6 + 0x2c);
      do {
        if (((param_1 & 1 << (*(byte *)(iVar5 + 0x18) & 0x1f)) != 0) && (1 < *psVar2)) {
          contrail_geometry_build_segment(iVar6,iVar5,iVar4);
        }
        iVar4 = iVar4 + 1;
        psVar2 = psVar2 + 1;
      } while ((short)iVar4 < 4);
      uVar7 = 0xffffffff;
      iVar5 = uVar1 + 1;
      sVar3 = (short)iVar5;
      uVar1 = uVar7;
    } while ((sVar3 < 0) || (*(short *)(DAT_0087abec + 0x2e) <= sVar3));
    psVar2 = (short *)((int)sVar3 * (int)*(short *)(DAT_0087abec + 0x22) +
                      *(int *)(DAT_0087abec + 0x34));
    do {
      if (*psVar2 != 0) {
        uVar1 = (int)*psVar2 << 0x10 | (int)(short)iVar5;
        break;
      }
      iVar5 = iVar5 + 1;
      psVar2 = (short *)((int)psVar2 + (int)*(short *)(DAT_0087abec + 0x22));
    } while ((short)iVar5 < *(short *)(DAT_0087abec + 0x2e));
  } while( true );
}
#endif
