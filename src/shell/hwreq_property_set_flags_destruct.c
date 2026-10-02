// hwreq_property_set_flags_destruct  (Ghidra: FUN_0057b990; MSVC 7.1 vector<hwreq_string_pair>::_Tidy)
// address 0x57b990, size 65 bytes
// name confidence: 0.7  rewrite confidence: 0.9
// evidence: hwreq_parser_destruct 0x57a010 and hwreq_parser_parse_propertyset_directive 0x57a3e0 release a
//   property set's flag vector with it. objdump 0x57b990..0x57b9d0: when the vector was allocated, every pair is
//   destroyed (hwreq_string_pair_destruct 0x5785b0) and the array freed; first / last / end are cleared either way.
// blam-cc: EBX -> set

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hwreq_string_pair_destruct(hwreq_string_pair *pair); // 0x5785b0
extern void free(void *block); // 0x6277e8, CRT free

void hwreq_property_set_flags_destruct(hwreq_property_set *set)
{
    hwreq_string_pair *pair = (hwreq_string_pair *)set->flags.first;

    if (pair != 0) {
        hwreq_string_pair *last = (hwreq_string_pair *)set->flags.last;

        for (; pair != last; pair++) {
            hwreq_string_pair_destruct(pair);
        }
        free((void *)set->flags.first);
    }
    set->flags.first = 0;
    set->flags.last = 0;
    set->flags.end = 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
