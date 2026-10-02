// hs_global_get_value  (Ghidra: hs_global_get_value_pointer -- misattributed per
// out/phase4/hs_types_notes.md: "returns the *value*, not a pointer... should be
// hs_global_get_value")
// address 0x48a720, size 74 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: out/phase4/hs_types_notes.md misattribution note; types/hs.h hs_global (value 0x04)
//   and k_hs_builtin_global_count (0x1eb).
// register convention: hs_global_reference in EAX (in_EAX).
//   // blam-cc: EAX -> reference

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_global_read_value(hs_global_reference reference); // this module, 0x48aec0

extern data_array *hs_globals_data; // 0x0087a46c

// Returns the current raw value of `reference` (after first syncing it from its bound engine
// variable, if it is a builtin -- see hs_global_read_value).
int32_t hs_global_get_value(hs_global_reference reference)
{
    hs_global *slot;
    uint16_t index;

    hs_global_read_value(reference);
    index = reference & k_hs_global_index_mask;
    if ((reference & k_hs_global_builtin_bit) != 0) {
        slot = (hs_global *)((uint8_t *)hs_globals_data->data + index * 8);
    } else {
        slot = (hs_global *)((uint8_t *)hs_globals_data->data + (index + k_hs_builtin_global_count) * 8);
    }
    return slot->value.long_value;
}

#if 0
Original Ghidra decompilation (0x48a720):

undefined4 hs_global_get_value_pointer(void)

{
  uint in_EAX;

  hs_global_read_value();
  if ((in_EAX & 0x8000) != 0) {
    return *(undefined4 *)(*(int *)(DAT_0087a46c + 0x34) + 4 + (in_EAX & 0x7fff) * 8);
  }
  return *(undefined4 *)(*(int *)(DAT_0087a46c + 0x34) + 4 + ((in_EAX & 0x7fff) + 0x1eb) * 8);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
