// vehicle_network_baseline_take  (not a Ghidra function; the vehicle object type definition's +0x68 "take a network
//   baseline" hook; no C existed, so it trapped as unlisted_572410)
// address 0x572410, size 182 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x572410..0x5724c5: for a live vehicle (object_try_and_get mask 2) bumps the
//   baseline index (+0x526), marks +0x525 and +0x528, clears +0x527, and snapshots the position (+0x5c -> +0x52c),
//   velocity (+0x68 -> +0x538), angular velocity (+0x8c -> +0x544), forward (+0x74 -> +0x550) and up (+0x80 ->
//   +0x55c).
// blam-cc: stack -> object_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX object, stack mask

static void copy3(uint8_t *obj, int32_t to, int32_t from)
{
    ((uint32_t *)(obj + to))[0] = ((uint32_t *)(obj + from))[0];
    ((uint32_t *)(obj + to))[1] = ((uint32_t *)(obj + from))[1];
    ((uint32_t *)(obj + to))[2] = ((uint32_t *)(obj + from))[2];
}

void vehicle_network_baseline_take(uint32_t object_index)
{
    uint8_t *obj = (uint8_t *)object_try_and_get(object_index, 2);

    if (obj == 0) {
        return;
    }
    obj[0x526]++;
    obj[0x525] = 1;
    obj[0x528] = 1;
    copy3(obj, 0x52c, 0x5c);
    copy3(obj, 0x538, 0x68);
    copy3(obj, 0x544, 0x8c);
    copy3(obj, 0x550, 0x74);
    obj[0x527] = 0;
    copy3(obj, 0x55c, 0x80);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
