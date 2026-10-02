// projectile_request_state  (Ghidra: item_update_max_permutation_reached; renamed per
// out/phase4/projectiles_types_notes.md "Renames this pass establishes" -- the inherited name
// comes from an unrelated symbol; the code has nothing to do with permutations)
// address 0x4bf0f0, size 38 bytes
// name confidence: 0.9   rewrite confidence: 0.95
// evidence: out/phase4/projectiles_types_notes.md: "if (*(int16 *)(object + 0x230) < cx)
//   *(int16 *)(object + 0x230) = cx; eax = projectile index, cx = requested projectile_state";
//   `objdump -d -M intel bin/halo.exe` at 0x4bf0f0 confirms exactly that (cmp cx,[eax+0x230] /
//   jle skip / mov [eax+0x230],cx, no other instructions). types/projectiles.h projectile_state
//   is "only ever raised, never lowered" citing this function by name.
// register convention: projectile index in EAX, requested state in CX (the low 16 bits of ECX).
// blam-cc: EAX -> projectile_index, CX (low half of ECX) -> requested_state

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

// Raises the projectile's state to requested_state, but only if it is not already at or past
// it -- state never moves backward. Every caller in this module uses this instead of writing
// projectile_data.state directly for exactly that reason.
void projectile_request_state(datum_index projectile_index, int16_t requested_state)
{
    object *obj = ((object_header *)object_data->data)[projectile_index & 0xffff].data;
    projectile_data *pd = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);

    if (pd->state < requested_state) {
        pd->state = requested_state;
    }
}

#if 0
Original Ghidra decompilation (0x4bf0f0):

void item_update_max_permutation_reached(void)

{
  int iVar1;
  uint in_EAX;
  short in_CX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (*(short *)(iVar1 + 0x230) < in_CX) {
    *(short *)(iVar1 + 0x230) = in_CX;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
