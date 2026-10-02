// object_notify_predicted_resources_if_valid  (Ghidra: FUN_004f7ad0; renamed, Blam-style, not
// previously named)
// address 0x4f7ad0, size 37 bytes
// name confidence: 0.25 (matches functions.md's summary: "Forwards to a notification routine
//   only if the given handle is valid"; the forwarded pointer lands inside Object.
//   predicted_resources' TagReflexive, offset 0x168..0x174 in types/tags.h, but the exact
//   sub-field at +0x170 is not otherwise identified)
// rewrite confidence: 0.4
// evidence: types/tags.h Object.predicted_resources; global 0x0087bc14 tag_instances.
// register convention: a TagID in EAX, forwarded via a computed pointer in ESI. Confirmed
//   against objdump -d -M intel bin/halo.exe: 0x4f7ad0 cmp eax,0xffffffff at entry with no
//   stack access, and 0x4f7ae8 add esi,0x170 / call 0x4449f0 with ESI as the sole live input.
//   // blam-cc: EAX -> definition_tag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14

extern void predicted_resource_list_touch(uint8_t *predicted_resources_field); // 0x4449f0, foreign module, UNSURE: signature guessed; ESI -> the argument

void object_notify_predicted_resources_if_valid(datum_index definition_tag) // blam-cc: EAX -> definition_tag
{
    if (definition_tag != k_datum_index_none) {
        uint8_t *tag_data = (uint8_t *)tag_instances[definition_tag & 0xffff].data;
        predicted_resource_list_touch(tag_data + 0x170);
    }
}

#if 0
Original Ghidra decompilation (0x4f7ad0):

void FUN_004f7ad0(void)

{
  int in_EAX;

  if (in_EAX != -1) {
    FUN_004449f0();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
