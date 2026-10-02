// biped_network_baseline_take  (not a Ghidra function; the biped object type definition's +0x68 "take a network
//   baseline" hook; no C existed, so it trapped as unlisted_55b3d0)
// address 0x55b3d0, size 108 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x55b3d0..0x55b43b: for a live biped (object_try_and_get mask 1) bumps the baseline
//   index (+0x527), snapshots +0xe0 into +0x530 and +0xe4 / 3 into +0x534, the unit word +0x31e into +0x52c and
//   (+0x104 > 0) into +0x538, marks the state valid (+0x526 = 1) and resets the sequence (+0x528 = 0).
// blam-cc: stack -> object_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX object, stack mask

void biped_network_baseline_take(uint32_t object_index)
{
    uint8_t *obj = (uint8_t *)object_try_and_get(object_index, 1);

    if (obj == 0) {
        return;
    }
    obj[0x527]++;
    ((struct biped_object *)obj)->biped.network_shield_vitality = ((unit_object *)obj)->base.shield_vitality * 0.33333334f;
    *(uint32_t *)&((struct biped_object *)obj)->biped.network_body_vitality = *(uint32_t *)&((unit_object *)obj)->base.body_vitality;
    obj[0x526] = 1;
    obj[0x528] = 0;
    obj[0x538] = (uint8_t)(((unit_object *)obj)->base.shield_stun_ticks > 0);
    ((struct biped_object *)obj)->biped.network_grenade_counts = *(int16_t *)(obj + 0x31e);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
