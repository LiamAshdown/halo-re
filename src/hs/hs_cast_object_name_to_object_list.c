// hs_cast_object_name_to_object_list  (not a Ghidra function; an hs type cast reached only through the cast table)
// address 0x48aba0, size 99 bytes
// name confidence: 0.6  rewrite confidence: 0.9
// evidence: hs_thread_return / hs_coerce_value call dword [ecx*4+0x68bc10] with the value pushed; this is
//   table 0x68bc10 entries 1166..1171 (to object_list from the object name types).
// objdump 0x48aba0..0x48ac02: a name index outside [0, 0x200) gives -1; otherwise the named object
//   (object_name_list 0x006b8cb8) -- none gives -1 -- becomes a new list exactly as
//   hs_object_list_new_singleton (0x48ac10) builds one: datum_new (EDX = object_list_header_data), count 0,
//   first reference -1 when allocated, then object_list_reference_add (EAX list, stack object) either way.
// blam-cc: stack -> name_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX


extern data_array *object_list_header_data; // 0x0087a464
extern datum_index *object_name_list;       // 0x006b8cb8

datum_index hs_cast_object_name_to_object_list(int32_t name_index)
{
    int16_t name = (int16_t)name_index;
    datum_index object_index;
    datum_index header_index;

    if (name < 0 || name >= 0x200) {
        return k_datum_index_none;
    }
    object_index = object_name_list[name];
    if (object_index == k_datum_index_none) {
        return k_datum_index_none;
    }
    header_index = datum_new(object_list_header_data);
    if (header_index != k_datum_index_none) {
        object_list_header *header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (header_index & 0xffff) * 0x0c);

        header->count = 0;
        header->first_reference = k_datum_index_none;
    }
    object_list_reference_add(header_index, object_index);
    return header_index;
}
