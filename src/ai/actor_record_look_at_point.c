// actor_record_look_at_point  (Ghidra: actor_record_look_at_point, renamed)
// address 0x421bc0, size 88 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: types/ai.h actor.look_at_priority (0x2ee) is cited as written by exactly this
//   address ("0x421bc0 keeps only the highest-priority look-at point"), together with the
//   four fields immediately after it (look_at_unknown_2f4/2f8/2fc/300/304). The optional
//   pointer argument supplies a 3-float point copied verbatim into 0x2fc/0x300/0x304.
// register convention: EAX -> actor_index, ECX -> optional point (may be NULL), DX -> priority
//   (int16), stack param_1 -> a caller-supplied 32-bit value stored verbatim.
//   // blam-cc: EAX -> actor_index, ECX -> point, EDX -> priority, stack -> data
// UNSURE: the original writes exactly one byte (0 or 1) at +0x2f8 as a "point present" flag;
//   types/ai.h types that offset as a 4-byte float (look_at_unknown_2f8), evidently from a
//   different function's wider access. The byte store is kept at its original width through
//   a cast on that field so this file does not silently zero or preserve its other 3 bytes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index, ECX -> point, EDX -> priority, stack -> data
// Registers a new look-at point of interest for the actor if `priority` outranks whatever
// is currently recorded at actor.look_at_priority. `point`, when non-NULL, is copied
// verbatim (three dwords) into the trailing fields and the "point present" flag is set to 1;
// when NULL, only that flag is cleared to 0.
void actor_record_look_at_point(datum_index actor_index, const uint32_t *point, int16_t priority, uint32_t data)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->look_at_priority < priority) {
        self->look_at_priority = priority;
        self->look_at_unknown_2f4 = data;
        if (point == 0) {
            *(uint8_t *)&self->look_at_unknown_2f8 = 0;
            return;
        }
        *(uint8_t *)&self->look_at_unknown_2f8 = 1;
        // Verbatim dword copy, not a numeric conversion: look_at_unknown_300 is typed as
        // float in types/ai.h (from another accessor's reading), but the source is an
        // arbitrary 32-bit value at the caller and must land bit-for-bit, so this copies
        // raw bytes rather than assigning through the field's declared type.
        memcpy(&self->look_at_unknown_2fc, &point[0], sizeof(uint32_t));
        memcpy(&self->look_at_unknown_300, &point[1], sizeof(uint32_t));
        memcpy(&self->look_at_unknown_304, &point[2], sizeof(uint32_t));
    }
}

#if 0
Original Ghidra decompilation (0x421bc0):

void FUN_00421bc0(undefined4 param_1)

{
  uint in_EAX;
  int iVar1;
  undefined4 *in_ECX;
  short in_DX;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (*(short *)(iVar1 + 0x2ee) < in_DX) {
    *(short *)(iVar1 + 0x2ee) = in_DX;
    *(undefined4 *)(iVar1 + 0x2f4) = param_1;
    if (in_ECX == (undefined4 *)0x0) {
      *(undefined1 *)(iVar1 + 0x2f8) = 0;
      return;
    }
    *(undefined1 *)(iVar1 + 0x2f8) = 1;
    *(undefined4 *)(iVar1 + 0x2fc) = *in_ECX;
    *(undefined4 *)(iVar1 + 0x300) = in_ECX[1];
    *(undefined4 *)(iVar1 + 0x304) = in_ECX[2];
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
