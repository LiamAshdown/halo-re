// camera_script_set_animation  (Ghidra: FUN_00444b30; renamed)
// address 0x444b30, size 206 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: types/camera.h camera_script_globals block comment: "Writers: camera_set 0x444c00
//   ..., the camera animation setter 0x444b30, ..."; out/phase4/camera_types_notes.md "0x444b30
//   lies just below the range [0x444c00..0x449170]. It is the camera animation setter, which
//   writes camera_script_globals" and "0x38 / 0x3c (DWORD / WORD) are written by 0x444b30 and
//   read by 0x444d50 case 1 (imul edi,edi,0xb4 into the animation block, frame_count +0x22)".
//   src/cache/README.md "Misattributed functions": "0x444b30 - Animation playback
//   (ModelAnimationsAnimation), not sound playback as the batch summary claimed. Belongs to
//   whichever module owns first-person animation state." src/camera/README.md's own address
//   range starts at 0x444c00, one function below this address, which is why camera never wrote
//   it even though it is the module that owns camera_script_globals. cleanup pass 4 orphan
//   pass: picked up here in camera, alongside camera_script_globals's other writers.
// register convention: `objdump -d -M intel --start-address=0x444b30 --stop-address=0x444c00
//   bin/halo.exe`: EBX = animation_tag (a ModelAnimations tag datum_index, tested against -1 at
//   entry), one plain stack argument (the animation name to search for). No return value
//   (Ghidra: `void FUN_00444b30(char *param_1)`, unaff_EBX).
//   // blam-cc: EBX -> animation_tag, stack -> name
// UNSURE: `tag->nodes.count == 1` (ModelAnimations offset 0x68, TagReflexive.count) gates the
//   whole search; kept literally since the field and offset are confirmed, but why a
//   single-node graph is required for camera animation playback is not established.
// UNSURE: time_remaining is set from an INTEGER division of frame_count by 30 (the disassembly
//   uses a magic-number reciprocal multiply/shift for `/30`, then casts the truncated integer
//   result to float with FILD/FSTP), not a float division; preserved exactly -- a fractional
//   frame_count/30 is truncated toward zero before becoming the float ticks-remaining value.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "camera.h"

extern tag_instance *tag_instances;         // 0x0087bc14
extern camera_script_globals camera_script; // 0x006869d0

// hs camera animation set: looks up `name` (case-insensitively) among the ModelAnimations tag
// `animation_tag`'s animations, and if found points camera_script_globals at it in animation
// mode (camera_point_index -1, object none, field_of_view the 70 degree default, time_remaining
// the animation's frame_count / 30 seconds). Does nothing if animation_tag is
// k_datum_index_none, the tag has other than exactly one graph node, or no animation matches.
void camera_script_set_animation(datum_index animation_tag, char *name)
{
    ModelAnimations *tag;
    int32_t index;
    ModelAnimationsAnimation *anim;

    if (animation_tag == k_datum_index_none) {
        return;
    }
    tag = (ModelAnimations *)tag_instances[animation_tag & 0xffff].data;
    if (tag->nodes.count != 1) {
        return;
    }
    if (tag->animations.count <= 0) {
        return;
    }

    index = 0;
    for (;;) {
        anim = (ModelAnimationsAnimation *)((uint8_t *)tag->animations.pointer +
                                            index * sizeof(ModelAnimationsAnimation));
        if (_stricmp(name, anim->name.string) == 0) {
            break;
        }
        index = index + 1;
        if (tag->animations.count <= index) {
            return;
        }
    }

    camera_script.camera_point_index = -1;
    camera_script.object = k_datum_index_none;
    camera_script.mode = _camera_script_mode_animation;
    camera_script.changed = 1;
    camera_script.field_of_view = 1.2217305f; // 70 degrees in radians, the same default camera_set uses
    camera_script.time_remaining = (real)(int32_t)(anim->frame_count / 30);
    camera_script.animation_tag = animation_tag;
    camera_script.animation_index = (int16_t)index;
}

#if 0
Original Ghidra decompilation (0x444b30):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00444b30(char *param_1)

{
  int iVar1;
  int iVar2;
  char *_Str2;
  uint unaff_EBX;
  short sVar3;

  if (((unaff_EBX != 0xffffffff) &&
      (iVar1 = *(int *)((unaff_EBX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
      *(int *)(iVar1 + 0x68) == 1)) && (sVar3 = 0, 0 < *(int *)(iVar1 + 0x74))) {
    iVar2 = 0;
    while( true ) {
      _Str2 = (char *)(iVar2 * 0xb4 + *(int *)(iVar1 + 0x78));
      iVar2 = __stricmp(param_1,_Str2);
      if (iVar2 == 0) break;
      sVar3 = sVar3 + 1;
      iVar2 = (int)sVar3;
      if (*(int *)(iVar1 + 0x74) <= iVar2) {
        return;
      }
    }
    _DAT_006869d4 = 0xffff;
    DAT_00686a04 = 0xffffffff;
    DAT_006869d2 = 1;
    DAT_006869d1 = 1;
    DAT_00686a00 = 0x3f9c61aa;
    _DAT_006869d8 = (float)((int)*(short *)(_Str2 + 0x22) / 0x1e);
    DAT_00686a08 = unaff_EBX;
    DAT_00686a0c = sVar3;
  }
  return;
}
#endif
