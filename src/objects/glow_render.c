// glow_render
// address 0x4fe570, size 259 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fe570 |
//   lightning_render | glow_render")
// rewrite confidence: 0.5
// evidence: types/objects.h globals list (glow_data 0x008603a0), glow_particle (t 0x28 doubles
//   as the vertex-position argument here via +0x2c, flags 0x54, next 0x5c); types/memory.h
//   data_array (size 0x22, last_index 0x2e, data 0x34); resolved against
//   glow_render_dispatch.c's call site (`mov ecx,esi; call 0x4fe570`), where esi carries the
//   full packed glow_handle (index low 16, salt high 16), the same shape this function itself
//   validates against.
// register convention: `in_ECX` unresolved by Ghidra; ECX -> glow_handle per the call site.
// blam-cc: ECX -> glow_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *glow_data; // 0x008603a0

extern void build_sprite(); // 0x511700, eight stack arguments.
    // The two call sites in this module disagree on the types of arguments 3 and 7
    // (a vector versus a marker record, a float versus a pointer), so no prototype is
    // asserted -- see the FUN_00450870 convention used elsewhere in this module.
extern void build_sprites_end(void); // 0x511620, unexamined, called once at the end unconditionally

void glow_render(datum_index glow_handle /*ECX*/) // blam-cc: ECX -> glow_handle
{
    glow *entry = 0;

    {
        int16_t index = (int16_t)glow_handle;

        if (index >= 0 && index < glow_data->last_index) {
            glow *candidate = (glow *)((uint8_t *)glow_data->data + glow_data->size * index);
            int16_t salt = (int16_t)(glow_handle >> 16);
            if (candidate->identifier != 0 && (salt == 0 || salt == candidate->identifier)) {
                entry = candidate;
            }
        }
    }

    {
        glow_particle *p;
        // UNSURE: on an invalid handle the original falls through to iVar2=0 and still
        // dereferences *(int*)(0 + 0x250), i.e. reads through a null pointer; guarded here as
        // "no particles" instead of reproducing that read.
        for (p = (entry != 0) ? entry->first_particle : 0; p != 0;
             p = *(glow_particle **)((uint8_t *)p + 0x5c)) {
            uint8_t *pb = (uint8_t *)p;
            uint8_t *marker = (uint8_t *)entry + *(int16_t *)(pb + 2) * 0x6c + 0x44;

            build_sprite(0, pb + 0x2c, marker, 0,
                                         *(void **)(pb + 0x24), pb + 0xc, *(void **)(pb + 0x58), 0);
        }
    }

    build_sprites_end();
}

#if 0
Original Ghidra decompilation (0x4fe570):

void lightning_render(void)

{
  int iVar1;
  int iVar2;
  short sVar3;
  undefined4 in_ECX;
  short sVar4;

  sVar3 = (short)in_ECX;
  if ((-1 < sVar3) && (sVar3 < *(short *)(DAT_008603a0 + 0x2e))) {
    iVar2 = (int)*(short *)(DAT_008603a0 + 0x22) * (int)sVar3;
    sVar3 = *(short *)(iVar2 + *(int *)(DAT_008603a0 + 0x34));
    iVar2 = iVar2 + *(int *)(DAT_008603a0 + 0x34);
    if ((sVar3 != 0) && ((sVar4 = (short)((uint)in_ECX >> 0x10), sVar4 == 0 || (sVar4 == sVar3))))
    goto LAB_004fe5b6;
  }
  iVar2 = 0;
LAB_004fe5b6:
  for (iVar1 = *(int *)(iVar2 + 0x250); iVar1 != 0; iVar1 = *(int *)(iVar1 + 0x5c)) {
    render_billboard_quad_build
              (0,iVar1 + 0x2c,*(short *)(iVar1 + 2) * 0x6c + 0x44 + iVar2,0,
               *(undefined4 *)(iVar1 + 0x24),iVar1 + 0xc,*(undefined4 *)(iVar1 + 0x58),0);
  }
  FUN_00511620();
  return;
}
#endif
