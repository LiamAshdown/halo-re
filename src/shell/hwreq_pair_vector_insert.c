// hwreq_pair_vector_insert  (Ghidra: FUN_0057b920; MSVC 7.1 vector<hwreq_string_pair>::insert(iterator, const T &))
// address 0x57b920, size 105 bytes
// name confidence: 0.75  rewrite confidence: 0.85
// evidence: hwreq_device_list_push_back 0x57b5e0 calls it at the end of the vector when there is no spare capacity.
//   objdump 0x57b920..0x57b988: the element offset of where ((where - first) / 0x38 signed, 0 while the vector is
//   empty or never allocated) is taken first, _Insert_n(where, 1, value) (0x57be20) runs, and the returned
//   iterator is rebuilt from the (possibly reallocated) first pointer and written through the hidden result
//   pointer, which is also the return value. (Earlier notes named it device_list_grow_and_insert with a guessed
//   signature that dropped the result pointer.)
// blam-cc: EDI -> this, stack -> result, where, value; returns result

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hwreq_pair_vector_insert_n(msvc_std_vector *self, hwreq_string_pair *where, uint32_t count,
    const hwreq_string_pair *value); // 0x57be20, blam-cc: ECX -> value, stack -> this, where, count

hwreq_string_pair **hwreq_pair_vector_insert(msvc_std_vector *self, hwreq_string_pair **result, hwreq_string_pair *where,
    const hwreq_string_pair *value)
{
    int32_t offset = 0;

    if (self->first != 0 && (int32_t)(self->last - self->first) / (int32_t)sizeof(hwreq_string_pair) != 0) {
        offset = ((int32_t)where - (int32_t)self->first) / (int32_t)sizeof(hwreq_string_pair);
    }
    hwreq_pair_vector_insert_n(self, where, 1, value);
    *result = (hwreq_string_pair *)self->first + offset;
    return result;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
