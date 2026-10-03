// hwreq_device_list_push_back  (already named; task-provided)
// address 0x57b5e0, size 135 bytes
// name confidence: 0.4 (already carries this name; standard MSVC 7.1
//   std::vector<hwreq_string_pair>::push_back(const hwreq_string_pair&), with the growth
//   fallback delegated to FUN_0057b920, out of this pass's scope)
// rewrite confidence: 0.5 (the fast in-place-construct path is fully resolved against objdump;
//   the growth/reallocate fallback (FUN_0057b920) is an opaque extern)
// evidence: types/shell.h msvc_std_vector; out/phase4/shell_types_notes.md "57b5e0
//   vector::push_back". Capacity check reproduces the standard `size() < capacity()`
//   computation on both the used range and the allocated range, dividing byte deltas by
//   sizeof(hwreq_string_pair) = 0x38 (matching the vector::size code shape in this same file,
//   hwreq_device_list_size.c).
// register convention (confirmed via objdump): EAX = this (msvc_std_vector *), stack argument
//   = const hwreq_string_pair *value. The call to uninit_fill_n_string_pair (0x57ce80, this
//   pass) passes count=1, dest=this->last, value=value -- matching that function's own
//   confirmed register convention.
// UNSURE: FUN_0057b920 (the reallocate-and-insert growth path) is an opaque extern, not
//   rewritten -- it is not in this pass's address list.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void uninit_fill_n_string_pair(hwreq_string_pair *dest, uint32_t count, const hwreq_string_pair *value); // 0x57ce80, same pass
extern hwreq_string_pair **hwreq_pair_vector_insert(msvc_std_vector *self, hwreq_string_pair **result,
    hwreq_string_pair *where, const hwreq_string_pair *value); // 0x57b920, blam-cc: EDI -> this, stack -> result, where, value

// blam-cc: EAX -> this, stack -> value
// FIXED (first-boot track, objdump 0x57b651..0x57b65b): the grow path calls insert(result, where, value) with a
// hidden result pointer (lea ecx,[esp+0x1c]; push ecx); the old extern dropped it and shifted the arguments.
void hwreq_device_list_push_back(msvc_std_vector *self, const hwreq_string_pair *value)
{
    if (self->first != 0 &&
        (uint32_t)(((int32_t)self->last - (int32_t)self->first) / 0x38) <
        (uint32_t)(((int32_t)self->end - (int32_t)self->first) / 0x38)) {
        void *dest = (void *)self->last;
        uninit_fill_n_string_pair((hwreq_string_pair *)dest, 1, value);
        self->last = (uint32_t)((uint8_t *)dest + 0x38);
        return;
    }
    {
        hwreq_string_pair *result; // the original passes its own dead argument slot as the result pointer
        hwreq_pair_vector_insert(self, &result, (hwreq_string_pair *)self->last, value);
    }
}

#if 0
Original Ghidra decompilation (0x57b5e0):

void hwreq_device_list_push_back(undefined4 param_1)

{
  int iVar1;
  int in_EAX;

  iVar1 = *(int *)(in_EAX + 4);
  if ((iVar1 != 0) &&
     ((uint)((*(int *)(in_EAX + 8) - iVar1) / 0x38) <
      (uint)((*(int *)(in_EAX + 0xc) - iVar1) / 0x38))) {
    iVar1 = *(int *)(in_EAX + 8);
    FUN_0057ce80(iVar1,param_1,param_1);
    *(int *)(in_EAX + 8) = iVar1 + 0x38;
    return;
  }
  FUN_0057b920(&param_1,*(undefined4 *)(in_EAX + 8),param_1);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
