// antenna_render_wire
// address 0x4fb3e0, size 228 bytes, zero recorded callers (dead/unreferenced in this binary)
// name confidence: 0.4 (still object_definition_predict in Ghidra/functions.md, which itself
//   flags the inherited name as wrong: "used for antenna wire rendering, despite its earlier
//   'predict' name"; renamed here to match what the body actually does)
// rewrite confidence: 0.2 (zero callers means there is no call site to cross-check register
//   assignment against, unlike antenna_update_physics / antenna_apply_marker_delta in this same
//   file group; Ghidra's own decompile mixes a param_1..param_9 signature with in_EAX/unaff_BP/
//   unaff_EDI/in_stack_000000d0, i.e. it could not resolve this function's calling convention
//   either. Preserved as a literal, offset-based transliteration rather than guessing field
//   identities the way the fully-resolved functions in this batch do.)
// evidence: types/tags.h Antenna (vertices TagReflexive 0xc4/0xc8), AntennaVertex (color
//   ColorARGB 0x2c, cutoff/length fields nearby); types/objects.h antenna_vertex (stride 0x20).
//   0x511700 is named build_sprite by the phase2 naming pass; 0x511620 is
//   unexamined.
// UNSURE: every parameter and every unaff_/in_ value below is a raw offset guess, not a named
//   field: in_EAX (tested against 0x44) is assumed to be some object/widget flags word;
//   unaff_EDI is assumed to be the Antenna tag pointer (its +0xc4/+0xc8 match vertices.count/
//   .pointer exactly, which is the only concrete anchor in this function); in_stack_000000d0 is
//   assumed to be the antenna instance pointer (by analogy with antenna_update_physics's
//   identical "+0x1c+i*0x20" vertex indexing); unaff_BP is the 0-based segment loop counter.
//   The gate `*(float *)(iVar2 + 0x34) != 0.0` lands inside the NEXT vertex's velocity field by
//   this addressing, which is suspicious enough that the whole offset scheme here may be wrong;
//   kept as literal arithmetic rather than resolved to a (possibly incorrect) named field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern void build_sprites_end(void); // 0x511620, unexamined, called once at the end unconditionally
extern void build_sprite(); // 0x511700, eight stack arguments.
    // The two call sites in this module disagree on the types of arguments 3 and 7
    // (a vector versus a marker record, a float versus a pointer), so no prototype is
    // asserted -- see the FUN_00450870 convention used elsewhere in this module.

void antenna_render_wire(uint32_t widget_flags /*in_EAX, UNSURE*/, float scale /*param_2*/,
                          Antenna *antenna_tag /*unaff_EDI, UNSURE*/, antenna *ant /*in_stack_000000d0, UNSURE*/)
    // blam-cc: UNSURE, see file header; this signature is a best-effort reconstruction with no
    // call site to verify it against
{
    if ((widget_flags & 0x44) != 0) {
        AntennaVertex *tag_vertices = (AntennaVertex *)antenna_tag->vertices.pointer;
        int16_t i = 0;

        do {
            uint8_t *vertex = (uint8_t *)ant + i * 0x20;      // antenna+0x1c already folded in
                                                                // by the caller per UNSURE above
            AntennaVertex *tag_vertex = &tag_vertices[i];
            real_vector3d segment;

            segment.i = *(float *)((uint8_t *)ant + i * 0x20 + 0x3c) - *(float *)(vertex + 0x1c);
            segment.j = *(float *)(vertex + 0x40) - *(float *)(vertex + 0x20);
            segment.k = *(float *)(vertex + 0x44) - *(float *)(vertex + 0x24);

            if (*(float *)(vertex + 0x34) != 0.0f && scale > 0.0f) {
                ColorARGB color = tag_vertex->color;
                build_sprite(1, vertex + 0x1c, &segment, 0,
                                             *(void **)(vertex + 0x34), &color, scale, 0);
                    // UNSURE: `*(float *)(vertex + 0x34)` is tested as a float above but passed
                    // as a pointer-sized arg here, faithfully matching Ghidra's own
                    // `*(undefined4 *)(iVar2 + 0x34)` reuse of the same bytes
            }

            i = i + 1;
        } while (i < antenna_tag->vertices.count);
    }
    build_sprites_end();
}

#if 0
Original Ghidra decompilation (0x4fb3e0):

void object_definition_predict
               (undefined4 param_1,float param_2,float param_3,float param_4,float param_5,
               undefined4 param_6,undefined4 param_7,undefined4 param_8,undefined4 param_9)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  short unaff_BP;
  int unaff_EDI;
  undefined4 uStack00000038;
  int in_stack_000000d0;

  uStack00000038 = 4;
  if ((in_EAX & 0x44) != 0) {
    iVar1 = 0;
    do {
      iVar2 = in_stack_000000d0 + iVar1 * 0x20;
      param_3 = *(float *)(in_stack_000000d0 + 0x3c + iVar1 * 0x20) - *(float *)(iVar2 + 0x1c);
      iVar1 = iVar1 * 0x80 + *(int *)(unaff_EDI + 200);
      param_4 = *(float *)(iVar2 + 0x40) - *(float *)(iVar2 + 0x20);
      param_6 = *(undefined4 *)(iVar1 + 0x2c);
      param_5 = *(float *)(iVar2 + 0x44) - *(float *)(iVar2 + 0x24);
      param_7 = *(undefined4 *)(iVar1 + 0x30);
      param_8 = *(undefined4 *)(iVar1 + 0x34);
      param_9 = *(undefined4 *)(iVar1 + 0x38);
      if ((*(float *)(iVar2 + 0x34) != 0.0) && (0.0 < param_2)) {
        render_billboard_quad_build
                  (1,iVar2 + 0x1c,&param_3,0,*(undefined4 *)(iVar2 + 0x34),&param_6,param_2,0);
      }
      unaff_BP = unaff_BP + 1;
      iVar1 = (int)unaff_BP;
    } while (iVar1 < *(int *)(unaff_EDI + 0xc4));
  }
  FUN_00511620();
  return;
}
#endif
