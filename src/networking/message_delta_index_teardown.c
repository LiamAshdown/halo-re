// message_delta_index_teardown  (reached only through a .data code pointer; no C existed)
// address 0x4e9b90, size 43 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9b90..0x4e9bba: frees the slot table (GlobalFree) and disposes the hash table
//   (the descriptor is taken only when kind 13  table flag is 1).
// blam-cc: cdecl

#include "message_delta_codec.h"

extern void hash_table_dispose(hash_table *table); // 0x4f04c0, EDI table
extern void *__stdcall GlobalFree(void *memory);

void message_delta_index_teardown(message_delta_field_type *field_type)
{
    int32_t *descriptor = message_delta_field_type_table[13].unknown_00[4] == 1 ? (int32_t *)field_type->array_descriptor : 0;

    GlobalFree((void *)descriptor[10]);
    hash_table_dispose((hash_table *)(descriptor + 3));
}
