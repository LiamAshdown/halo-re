// animation_choose_random_permutation  (Ghidra: FUN_004d6280, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d6280, size 164 bytes
// name confidence: 0.45   rewrite confidence: 0.85 (review pass: checked against objdump)
// evidence: out/phase4/models_types_notes.md register table and Globals section (tag data at
//   tag_instances[].data, the two LCG seeds). ModelAnimationsAnimation.relative_weight (+0x44)
//   and .next_animation (+0x38) match types/tags.h. The comparison
//   `fVar3 < fVar2 != (fVar3 == fVar2)` in the Ghidra output is the compiler's NaN-safe
//   spelling of `fVar3 <= fVar2` (exactly one of <, ==, > holds for ordered floats, and the
//   weights here are never NaN), so it is written the plain way.
//   Ghidra shows the return as CONCAT22 of a low 16-bit animation index and a high half that
//   is leftover EDX bits from the tag data pointer arithmetic, not a real value: the register
//   table (cross-checked at the three call sites) is authoritative that only the low 16 bits
//   (DX) are the real result, so this returns plain int16_t.
// register convention: animation graph tag id in EAX (in_EAX, only the low 16 bits used),
//   starting animation index in DX (in_DX); random stream selector as the recognized stack
//   parameter (param_1).
//   // blam-cc: EAX -> animation_graph_tag, DX -> first_animation, stack -> stream

#include "tags.h"
#include "math.h"
#include "cache.h"
#include "models.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14
extern random_seed random_seed_global; // 0x00719cd0
extern random_seed effect_random_seed; // 0x00719cd4

// Walks the next_animation chain starting at first_animation, picking the first entry whose
// cumulative relative_weight is at or above a fresh random threshold in [0,1). Returns -1 if
// first_animation is -1, or if the walk runs off the end of the chain without a weight ever
// reaching the threshold.
int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
                                             animation_random_stream stream)
{
    ModelAnimations *graph;
    ModelAnimationsAnimation *animations;
    random_seed seed;
    real threshold;
    int16_t animation;

    graph = (ModelAnimations *)tag_instances[animation_graph_tag & 0xffff].data;
    animations = (ModelAnimationsAnimation *)graph->animations.pointer;

    if (stream == _animation_random_global) {
        random_seed_global = random_seed_global * k_random_multiplier + k_random_increment;
        seed = random_seed_global;
    } else {
        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        seed = effect_random_seed;
    }
    threshold = (real)(seed >> k_random_value_shift) * 1.5259022e-05f;

    animation = first_animation;
    while (animation != -1) {
        if (threshold <= animations[animation].relative_weight) {
            break;
        }
        animation = (int16_t)animations[animation].next_animation;
    }
    return animation;
}

#if 0
Original Ghidra decompilation (0x4d6280):

undefined4 FUN_004d6280(int param_1)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  uint in_EAX;
  int iVar4;
  uint uVar6;
  short in_DX;
  ushort uVar5;

  iVar4 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (param_1 == 1) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    uVar6 = random_seed_global;
  }
  else {
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    uVar6 = DAT_00719cd4;
  }
  fVar3 = (float)(uVar6 >> 0x10) * 1.5259022e-05;
  if (in_DX == -1) {
    return CONCAT22((short)((uint)iVar4 >> 0x10),0xffff);
  }
  piVar1 = (int *)(iVar4 + 0x78);
  do {
    fVar2 = *(float *)(in_DX * 0xb4 + 0x44 + *piVar1);
    uVar5 = (ushort)((uint)iVar4 >> 0x10);
    iVar4 = (uint)uVar5 << 0x10;
    if (fVar3 < fVar2 != (fVar3 == fVar2)) break;
    in_DX = *(short *)(in_DX * 0xb4 + *piVar1 + 0x38);
  } while (in_DX != -1);
  return CONCAT22(uVar5,in_DX);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
