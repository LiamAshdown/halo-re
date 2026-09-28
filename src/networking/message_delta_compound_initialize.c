// message_delta_compound_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4e9580, size 94 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9580..0x4e95dd: a positive count and every binding  type present and
//   initializing.
// blam-cc: cdecl

#include "message_delta_codec.h"

uint8_t message_delta_compound_initialize(message_delta_field_type *field_type)
{
    message_delta_array_field_list *list = (message_delta_array_field_list *)field_type->array_descriptor;
    uint8_t result = message_delta_field_type_table[9].unknown_00[4] != 1;
    int32_t i;

    if (list->count <= 0) {
        return 0;
    }
    for (i = 0; i < list->count; i++) {
        message_delta_field_binding *binding = &list->fields[i];

        if (binding == 0 || binding->field_type == 0) {
            return 0;
        }
        result = MESSAGE_DELTA_INITIALIZE(binding->field_type);
        if (result != 1) {
            return 0;
        }
    }
    return result;
}
