// actor_command_list_reset_record  (not a Ghidra function; the per-member callback of 0x407140)
// address 0x406dd0, size 88 bytes
// name confidence: 0.4  rewrite confidence: 0.9
// evidence: pushed as the actor_swarm_for_each_component callback by 0x407140 (the command-list mode setup
//   the ai_command_list script function runs through 0x434d90), with a pointer to a flag byte as its extra.
// objdump 0x406dd0..0x406e27: zeroes the 0x24-byte component record, sets its first byte to 0xff and bit 0 of
//   its byte +4 to (*flag != 0); when the fifth argument is a non-null record (the non-swarm path passes the
//   caller record +0x2c there) its 0x58 bytes are zeroed and its word +2 set to -1.
// blam-cc: stack -> actor, unit, extra, component_record, secondary_record, flag pointer (swarm callback shape)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

void actor_command_list_reset_record(uint32_t actor_index, datum_index unit_index, uint16_t extra,
    void *component_record, int32_t secondary_record, uint32_t callback_extra)
{
    uint8_t *record = (uint8_t *)component_record;
    uint8_t *secondary = (uint8_t *)(uintptr_t)secondary_record;

    (void)actor_index;
    (void)unit_index;
    (void)extra;
    memset(record, 0, 0x24);
    record[0] = 0xff;
    if (*(uint8_t *)(uintptr_t)callback_extra != 0) {
        record[4] |= 1;
    } else {
        record[4] &= 0xfe;
    }
    if (secondary != 0) {
        memset(secondary, 0, 0x58);
        *(int16_t *)(secondary + 2) = -1;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
