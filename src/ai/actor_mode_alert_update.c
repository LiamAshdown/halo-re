// actor_mode_alert_update  (not a Ghidra function; AI "alert" mode)
// address 0x401410, size 78 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: actor_mode_definitions[2] ("alert") update slot (+0x14).
// objdump 0x401410..0x40145d: firing mode (+0x3fc) 1; an actor definition (+0x58) with flag 0x40 also sets the look
//   flags +0x426 and +0x427.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern Scenario *global_scenario;

#define B(o) (actor[(o)])
#define W(o) (*(int16_t *)(actor + (o)))
#define D(o) (*(uint32_t *)(actor + (o)))
#define F(o) (*(float *)(actor + (o)))

void actor_mode_alert_update(uint32_t actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;

    W(0x3fc) = 1;
    if (*(uint8_t *)tag_instances[D(0x58) & 0xffff].data & 0x40) {
        B(0x426) = 1;
        B(0x427) = 1;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
