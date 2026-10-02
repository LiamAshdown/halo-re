// hs_syntax_data_byte_swap  (not a Ghidra function; the byte-swap proc of the "hs_syntax_data_definition" tag data
//   definition at 0x0068e3ec (+0x0c, stored at 0x0068e3f8); no C existed, so that stored pointer trapped as
//   unlisted_48b150)
// address 0x48b150, size 199 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x48b150..0x48b216: nothing for an empty block. When the block starts with the
//   data array name "script node" in swapped byte order (the 12 bytes at 0x0066919c), the 0x38 byte header
//   (byte-swap definition 0x0068e39c) and all 0x4000 syntax nodes (0x0068e3d8, 0x14 bytes each) are swapped.
//   Otherwise the size must be 0x38 plus a whole number of nodes: the header is swapped (when there is data) and
//   then each node, through 0x4cfd50 -- a helper (EDI definition, EBX count, stack data) that exists only for this
//   call and is inlined here. The first argument (the owning element) is not used.
// blam-cc: cdecl (called through the tag data definition)

#include "tags.h"
#include "memory.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void struct_definition_byte_swap(byte_swap_definition *definition, int32_t data,
    int32_t *codes, int32_t *out_size, int32_t *out_record_count); // 0x4cfee0
extern byte_swap_definition hs_syntax_data_header_byte_swap_definition; // 0x0068e39c
extern byte_swap_definition hs_syntax_node_byte_swap_definition; // 0x0068e3d8

// "script node" as the big-endian data array header stores it (each dword reversed); 0x0066919c.
static const uint8_t k_swapped_script_node_name[12] = { 0x63, 0x73, 0x69, 0x72, 0x74, 0x70, 0x6e, 0x20, 0x00, 0x65, 0x64, 0x6f };

void hs_syntax_data_byte_swap(void *element, uint8_t *data, uint32_t size)
{
    byte_swap_definition *header = &hs_syntax_data_header_byte_swap_definition;
    byte_swap_definition *node = &hs_syntax_node_byte_swap_definition;
    uint32_t count;
    uint32_t i;

    (void)element;
    if (size == 0) {
        return;
    }
    if (memcmp(data, k_swapped_script_node_name, sizeof(k_swapped_script_node_name)) == 0) {
        if (data == 0) {
            return;
        }
        struct_definition_byte_swap(header, (int32_t)data, header->codes, 0, 0);
        for (i = 0; i < 0x4000; i++) {
            struct_definition_byte_swap(node, (int32_t)(data + node->size * i), node->codes, 0, 0);
        }
        return;
    }
    if ((int32_t)(size - 0x38) < 0 || (size - 0x38) % 0x14 != 0) {
        return;
    }
    count = (size - 0x38) / 0x14;
    if (data != 0) {
        struct_definition_byte_swap(header, (int32_t)data, header->codes, 0, 0);
    }
    if (data + 0x38 != 0) {
        for (i = 0; i < count; i++) {
            struct_definition_byte_swap(node, (int32_t)(data + 0x38 + node->size * i), node->codes, 0, 0);
        }
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
