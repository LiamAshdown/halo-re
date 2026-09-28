// hwreq_property_set_apply  (not a Ghidra function; merges one property set into another)
// address 0x57b470, size 77 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x57b470..0x57b4ba: for every (name, value) pair of the source's flag vector,
//   hwreq_property_set_upsert (0x578410) into the target with the two strings' c_str().
// blam-cc: EAX source, stack -> target (callee pops 4)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void hwreq_property_set_upsert(hwreq_property_set *property_set, char *key, char *value); // 0x578410

static char *c_str(msvc_std_string *s)
{
    return s->capacity >= 0x10 ? (char *)s->buffer.heap_buffer : s->buffer.inline_buffer;
}

void hwreq_property_set_apply(hwreq_property_set *source, hwreq_property_set *target)
{
    hwreq_string_pair *pair = (hwreq_string_pair *)source->flags.first;
    hwreq_string_pair *end = (hwreq_string_pair *)source->flags.last;

    for (; pair != end; pair++) {
        hwreq_property_set_upsert(target, c_str(&pair->first), c_str(&pair->second));
    }
}
