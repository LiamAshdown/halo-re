// message_delta_index_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4e9b10, size 125 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9b10..0x4e9b8c: a positive count and bucket count; once per type: the hash
//   table at +0x0c (bucket count), a GlobalAlloc  count-long table of -1 at +0x28 (+0x24 cleared), key -1 mapped to 0
//   and slot 0 taken (1).
// blam-cc: cdecl

#include "win32.h"
#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hash_table_initialize(hash_table *table, int32_t bucket_count); // 0x4f0470, ESI table, EAX buckets
extern void hash_table_set_or_remove(hash_table *table, int32_t key, int32_t value); // 0x4f0530

uint8_t message_delta_index_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    if (descriptor[0] < 1 || descriptor[1] < 1) {
        return 0;
    }
    if (field_type->initialized == 0) {
        int32_t *table;

        hash_table_initialize((hash_table *)(descriptor + 3), descriptor[1]);
        table = (int32_t *)GlobalAlloc(0, descriptor[0] * 4);
        descriptor[10] = (int32_t)table;
        descriptor[9] = 0;
        memset(table, 0xff, descriptor[0] * 4);
        hash_table_set_or_remove((hash_table *)(descriptor + 3), -1, 0);
        *(int32_t *)descriptor[10] = 1;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
