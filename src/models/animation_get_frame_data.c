// animation_get_frame_data  (Ghidra: FUN_004d4810, unnamed; renamed per
// out/phase4/models_types_notes.md "Misnamed or misattributed functions" table)
// address 0x4d4810, size 52 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/models_functions.md summary ("computes the address of the raw
//   compressed vertex data for a given animation frame index") plus the types notes, which
//   correct this to animation frame data, not vertex data: when the animation is compressed
//   (flags bit 0) and the module-wide decode toggle is set, this returns the compressed header
//   base (frame_data.pointer + offset_to_compressed_data); otherwise it returns the address of
//   the uncompressed frame (frame_data.pointer + frame_size * frame).
// register convention: animation in ECX (in_ECX); frame as the recognized stack parameter
//   (param_1).
//   // blam-cc: ECX -> animation, stack -> frame

#include "tags.h"
#include "math.h"
#include "models.h"

extern uint8_t animation_compressed_data_enabled; // 0x006894b4

// Returns the address of the frame's animation data: the compressed header base when the
// animation is compressed and compressed decoding is enabled, otherwise the byte offset of the
// requested uncompressed frame within frame_data.
void *animation_get_frame_data(ModelAnimationsAnimation *animation, int16_t frame)
{
    int use_compressed;

    use_compressed = ((animation->flags & 1) != 0) && (animation_compressed_data_enabled != 0);
    if (use_compressed) {
        return (uint8_t *)animation->frame_data.pointer + animation->offset_to_compressed_data;
    }
    // movsx frame_size, movsx frame, imul (0x4d4835..0x4d483e)
    return (uint8_t *)animation->frame_data.pointer + (int32_t)(int16_t)animation->frame_size * (int32_t)frame;
}

#if 0
Original Ghidra decompilation (0x4d4810):

int FUN_004d4810(short param_1)

{
  bool bVar1;
  int in_ECX;

  bVar1 = true;
  if (((*(byte *)(in_ECX + 0x3a) & 1) == 0) || (DAT_006894b4 == '\0')) {
    bVar1 = false;
  }
  if (bVar1) {
    return *(int *)(in_ECX + 0xac) + *(int *)(in_ECX + 0x88);
  }
  return *(int *)(in_ECX + 0xac) + (int)*(short *)(in_ECX + 0x24) * (int)param_1;
}
#endif
