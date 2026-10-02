// predicted_resource_list_touch  (Ghidra: FUN_004449f0; renamed -- not Bungie-attested, chosen
// to match the cache_functions.md summary: "Walks a list of mixed bitmap/sound resource
// references, touching each one to force it into its respective cache")
// address 0x4449f0, size 101 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: types/tags.h PredictedResource (type 0x00 = bitmap/sound selector, resource_index
// 0x02, tag 0x04, size 0x08 confirmed by the stride) and Bitmap (bitmap_data TagReflexive at
// 0x60, pointer field therefore at 0x64) layouts. The whole function's register-passed argument
// (unaff_ESI in Ghidra's own decompile) and the entire bitmap-resolution computation for the
// type == 0 case are elided by Ghidra's pseudo-C -- both calls show as FUN_00444550(0,1) and
// FUN_00444a60() with no argument setup at all -- and were reconstructed from raw disassembly
// (objdump -d -M intel --start-address=0x4449f0 --stop-address=0x444a60 bin/halo.exe).
// register convention: a TagReflexive-shaped {count, pointer} list in ESI (unaff_ESI); this
// module never touches the reflexive's own `definition` field, only `count` and `pointer`.
//
// UNSURE: the list parameter's own struct identity (a PredictedResource TagReflexive on some
// owning tag, e.g. Effect/DamageEffect) is not established by this module -- only that it has
// the same {count, pointer, definition} shape as every other TagReflexive, which is why
// types/tags.h's own TagReflexive is reused here rather than inventing a new type.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "tags.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14

extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // this module, texture_cache_get.c
extern void sound_tag_touch_permutations(TagID tag); // this module, sound_tag_touch_permutations.c

// blam-cc: resources in ESI
// Walks a PredictedResource list (bitmap/sound references predicted to be needed soon, e.g. by
// an about-to-fire effect), forcing each one into its respective streaming cache: type 0
// resolves the BitmapData at resources[i].resource_index on the referenced Bitmap tag and calls
// texture_cache_get(bitmap, wait = 0, allocate_if_missing = 1); type 1 forwards the referenced
// tag straight to sound_tag_touch_permutations. Any other type is skipped.
void predicted_resource_list_touch(TagReflexive *resources)
{
    int16_t index;
    PredictedResource *element;

    index = 0;
    if (0 < (int32_t)resources->count) {
        do {
            element = &((PredictedResource *)resources->pointer)[index];

            if (element->type == predictedresourcetype_bitmap) {
                Bitmap *bitmap_tag = (Bitmap *)tag_instances[element->tag.index].data;
                BitmapData *bitmap = (BitmapData *)((uint8_t *)bitmap_tag->bitmap_data.pointer +
                    (int32_t)(int16_t)element->resource_index * sizeof(BitmapData));
                texture_cache_get(bitmap, 0, 1);
            } else if (element->type == predictedresourcetype_sound) {
                sound_tag_touch_permutations(element->tag);
            }

            index = index + 1;
        } while ((int32_t)index < (int32_t)resources->count);
    }
    return;
}

#if 0
Original Ghidra decompilation (0x4449f0):

void FUN_004449f0(void)

{
  short sVar1;
  int iVar2;
  int *unaff_ESI;
  short sVar3;

  sVar3 = 0;
  if (0 < *unaff_ESI) {
    iVar2 = 0;
    do {
      sVar1 = *(short *)(unaff_ESI[1] + iVar2 * 8);
      if (sVar1 == 0) {
        FUN_00444550(0,1);
      }
      else if (sVar1 == 1) {
        FUN_00444a60();
      }
      sVar3 = sVar3 + 1;
      iVar2 = (int)sVar3;
    } while (iVar2 < *unaff_ESI);
  }
  return;
}

Raw disassembly (0x4449f0..0x444a60) recovering the elided EAX/tag_instances resolution:

004449f0:  mov    eax,[esi]                push edi ; xor edi,edi ; test eax,eax ; jle 444a56
004449f9:  xor    eax,eax                  jmp 444a00
00444a00:  mov    ecx,[esi+0x4]            lea eax,[ecx+eax*8]        ; &list[i]
00444a06:  movsx  ecx,word ptr [eax]       sub ecx,0 ; je 444a1b      ; type == 0 (bitmap)
00444a0e:  dec    ecx                      jne 444a4c                 ; type == 1 (sound)
00444a11:  mov    eax,[eax+0x4]            call 444a60                ; sound_tag_touch_permutations(tag)
00444a1b:  mov    edx,[eax+0x4]            movsx eax,word ptr [eax+0x2]
00444a22:  mov    ecx,ds:0x87bc14          and edx,0xffff ; shl edx,0x5
00444a31:  mov    edx,[edx+ecx+0x14]       mov ecx,[edx+0x64]         ; tag->bitmap_data.pointer
00444a38:  lea    eax,[eax+eax*2]          shl eax,0x4                ; resource_index * 0x30
00444a3e:  push   0x1                      push 0x0
00444a42:  add    eax,ecx                  call 444550                ; texture_cache_get(bitmap,0,1)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
