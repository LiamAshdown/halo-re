// actor_mode_fight_update  (not a Ghidra function; AI "fight" mode)
// address 0x4035b0, size 128 bytes
// name confidence: 0.4  rewrite confidence: 0.9
// evidence: actor_mode_definitions[3] ("fight") update slot (+0x14).
// objdump 0x4035b0..0x40362f: looks follow the target (+0x426 = +0x358), look mode 5 (+0x3e8), aim mode 2 (+0x3ec),
//   firing mode 4 (+0x3fc), the other look flags (+0x424, +0x425, +0x427, +0x428) cleared; an actor on foot
//   (+0x15e not 4) at alertness (+0x6e) 5 or more also searches (+0x454) with look mode 7.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

void actor_mode_fight_update(uint32_t actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    actor[0x426] = actor[0x358];
    ((struct actor *)actor)->vocalization_unknown_3e8 = 5;
    ((struct actor *)actor)->vocalization_unknown_3ec = 2;
    ((struct actor *)actor)->look_posture = 4;
    actor[0x427] = 0;
    actor[0x428] = 0;
    actor[0x424] = 0;
    actor[0x425] = 0;
    if (((struct actor *)actor)->vehicle_driving_type != 4 && ((struct actor *)actor)->combat_status >= 5) {
        actor[0x454] = 1;
        ((struct actor *)actor)->vocalization_unknown_3e8 = 7;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
