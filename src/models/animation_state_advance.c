// animation_state_advance  (Ghidra: FUN_004d48d0, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d48d0, size 218 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (review pass: checked against objdump)
// evidence: types notes "It steps an animation_state and has nothing to do with regions." and
//   the "animation_state" / "animation_state_advance_result" struct sections: advances the
//   caller's animation_state by one frame against the ModelAnimations tag resolved from
//   animation_graph_tag_index, optionally reporting the currently-playing sound's tag id, and
//   returns which of the five animation_state_advance_result cases applies. tag_instances /
//   ModelAnimations / ModelAnimationsAnimation layouts confirmed against types/cache.h and
//   types/tags.h.
// register convention: graph tag index in EAX (in_EAX), animation_state pointer in ESI
//   (unaff_ESI), sound tag id output pointer (may be NULL) in EBX (unaff_EBX); random stream
//   as the recognized stack parameter (param_1).
//   // blam-cc: EAX -> animation_graph_tag_index, ESI -> state, EBX -> sound_tag_id,
//   //   stack -> random_stream

#include "tags.h"
#include "math.h"
#include "cache.h"
#include "models.h"

extern tag_instance *tag_instances; // 0x0087bc14

extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
                                                    animation_random_stream stream); // 0x4d6280; EAX, DX, stack

// Advances *state by one frame against the ModelAnimations tag animation_graph_tag_index and
// reports what happened. When sound_tag_id is non-NULL, it receives the tag id of the sound
// that starts on the frame just advanced from (-1 if there is none).
animation_state_advance_result animation_state_advance(uint32_t animation_graph_tag_index,
                                                         animation_state *state,
                                                         int32_t *sound_tag_id,
                                                         animation_random_stream random_stream)
{
    ModelAnimations *graph;
    ModelAnimationsAnimation *animation;
    ModelAnimationsAnimationGraphSoundReference *sound_references;
    int16_t frame_index;
    int16_t frame_count;
    int16_t loop_frame_index;
    int16_t clamped_loop_frame;

    graph = (ModelAnimations *)tag_instances[animation_graph_tag_index & 0xffff].data;
    animation = (ModelAnimationsAnimation *)((uint8_t *)graph->animations.pointer +
                                              state->animation_index * (int)sizeof(ModelAnimationsAnimation));

    if (sound_tag_id != 0) {
        // both are word compares (cmp di,0xffff / cmp bp,[esi+2]) and the index is movsx'd
        if ((int16_t)animation->sound == -1 || (int16_t)animation->sound_frame_index != state->frame_index) {
            *sound_tag_id = -1;
        } else {
            sound_references = (ModelAnimationsAnimationGraphSoundReference *)graph->sound_references.pointer;
            *sound_tag_id = *(int32_t *)&sound_references[(int16_t)animation->sound].sound.tag_id;
        }
    }

    state->frame_index = state->frame_index + 1;
    frame_index = state->frame_index;
    frame_count = animation->frame_count;
    if (frame_count <= frame_index) {
        loop_frame_index = animation->loop_frame_index;
        if (0 < loop_frame_index) {
            clamped_loop_frame = frame_count - 1;
            if (loop_frame_index <= frame_count - 1) {
                clamped_loop_frame = loop_frame_index;
            }
            state->frame_index = clamped_loop_frame;
            return _animation_advance_looped;
        }
        state->animation_index = animation_choose_random_permutation(animation_graph_tag_index,
                                                                     (int16_t)animation->main_animation_index, random_stream);
        state->frame_index = 0;
        return _animation_advance_next_animation;
    }
    if (frame_index + 1 == frame_count && animation->loop_frame_index == 0) {
        return _animation_advance_last_frame;
    }
    if (frame_index != (int16_t)animation->key_frame_index && frame_index != (int16_t)animation->second_key_frame_index) {
        return _animation_advance_none;
    }
    return _animation_advance_key_frame;
}

#if 0
Original Ghidra decompilation (0x4d48d0):

undefined4 FUN_004d48d0(undefined4 param_1)

{
  short sVar1;
  short sVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;
  undefined4 *unaff_EBX;
  short *unaff_ESI;

  iVar3 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar4 = *unaff_ESI * 0xb4 + *(int *)(iVar3 + 0x78);
  if (unaff_EBX != (undefined4 *)0x0) {
    if ((*(short *)(iVar4 + 0x3c) == -1) || (*(short *)(iVar4 + 0x3e) != unaff_ESI[1])) {
      *unaff_EBX = 0xffffffff;
    }
    else {
      *unaff_EBX = *(undefined4 *)(*(int *)(iVar3 + 0x58) + 0xc + *(short *)(iVar4 + 0x3c) * 0x14);
    }
  }
  unaff_ESI[1] = unaff_ESI[1] + 1;
  sVar2 = unaff_ESI[1];
  sVar1 = *(short *)(iVar4 + 0x22);
  if (sVar1 <= sVar2) {
    sVar2 = *(short *)(iVar4 + 0x2e);
    if (0 < sVar2) {
      iVar3 = sVar1 + -1;
      if ((int)sVar2 <= sVar1 + -1) {
        iVar3 = (int)sVar2;
      }
      unaff_ESI[1] = (short)iVar3;
      return 4;
    }
    sVar2 = FUN_004d6280(param_1);
    *unaff_ESI = sVar2;
    unaff_ESI[1] = 0;
    return 3;
  }
  if ((sVar2 + 1 == (int)sVar1) && (*(short *)(iVar4 + 0x2e) == 0)) {
    return 2;
  }
  if ((sVar2 != *(short *)(iVar4 + 0x34)) && (sVar2 != *(short *)(iVar4 + 0x36))) {
    return 0;
  }
  return 1;
}
#endif
