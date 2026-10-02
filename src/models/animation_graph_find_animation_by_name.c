// animation_graph_find_animation_by_name  (Ghidra: model_get_region_index_by_name, wrong name
// and wrong object; renamed per out/phase4/models_types_notes.md "Misnamed or misattributed
// functions" table -- searches ModelAnimations.animations (stride 0xb4), not model regions)
// address 0x4d6ab0, size 81 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/models_types_notes.md: "Wrong object: it searches
//   ModelAnimations.animations (+0x74/+0x78, stride 0xb4), not model regions." VERIFIED
//   against objdump -d -M intel bin/halo.exe (scratchpad/halo_disasm.txt, 0x4d6ab0..0x4d6b00):
//   a plain linear scan (not the binary search animation_graph_marker_group... i.e.
//   model_marker_group_index_from_name/0x4d77c0 uses), comparing `name` against each
//   animation's TagString (which starts at the animation's own base address) with _stricmp,
//   pushed as _stricmp(name, &animations[i]).
// register convention: animation graph tag id in EAX (in_EAX), name in EBX (unaff_EBX).
//   // blam-cc: EAX -> animation_graph_tag, EBX -> name

#include "crt.h"
#include "tags.h"
#include "math.h"
#include "cache.h"
#include "models.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14

// Linear search of an animation graph's animations block for one whose name matches (case
// insensitive). Returns its index, or -1 if none matches.
int16_t animation_graph_find_animation_by_name(datum_index animation_graph_tag, const char *name)
{
    ModelAnimations *graph;
    ModelAnimationsAnimation *animations;
    int16_t i; // di: incremented as a word, movsx'd for the compare, returned in AX

    graph = (ModelAnimations *)tag_instances[animation_graph_tag & 0xffff].data;
    animations = (ModelAnimationsAnimation *)graph->animations.pointer;

    for (i = 0; (int32_t)i < graph->animations.count; i++) {
        if (_stricmp(name, (const char *)&animations[i]) == 0) {
            return i;
        }
    }
    return -1;
}

#if 0
Ghidra could not resolve this function return value or its register arguments (they are all
register-passed and get dropped), so no usable pseudocode came out of it. Reconstructed
instead from objdump -d -M intel bin/halo.exe, 0x4d6ab0..0x4d6b00:

004d6ab0  mov ecx,ds:0x87bc14
004d6ab6  and eax,0xffff
004d6abb  shl eax,0x5
004d6abe  push esi
004d6abf  mov esi,[eax+ecx+0x14]        ; graph = tag_instances[tag & 0xffff].data
004d6ac3  mov eax,[esi+0x74]            ; graph->animations.count
004d6ac6  push edi
004d6ac7  xor edi,edi                   ; i = 0
004d6ac9  test eax,eax
004d6acb  jle 0x4d6af4                  ; count <= 0 -> not found
004d6acd  xor eax,eax
004d6ad0  mov edx,[esi+0x78]            ; graph->animations.pointer
004d6ad3  imul eax,eax,0xb4
004d6ad9  add eax,edx                   ; &animations[i]
004d6adb  push eax
004d6adc  push ebx                      ; name
004d6add  call 0x628d8b                 ; __stricmp(name, &animations[i])
004d6ae2  add esp,0x8
004d6ae5  test eax,eax
004d6ae7  je 0x4d6afb                   ; match -> return i
004d6ae9  mov ecx,[esi+0x74]
004d6aec  inc edi
004d6aed  movsx eax,di
004d6af0  cmp eax,ecx
004d6af2  jl 0x4d6ad0
004d6af4  pop edi
004d6af5  or ax,0xffff                  ; not found -> -1
004d6af9  pop esi
004d6afa  ret
004d6afb  mov ax,di
004d6afe  pop edi
004d6aff  pop esi
004d6b00  ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
