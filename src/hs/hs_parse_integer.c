// hs_parse_integer  (not a Ghidra function; an hs primitive parser)
// address 0x486b80, size 195 bytes
// name confidence: 0.7  rewrite confidence: 0.90
// evidence: hs_parse_primitive_procedures 0x0065b668 (indexed by hs type) entry 7 (short) and 8 (long) = 0x486b80; hs_parse_primitive
//   0x486480 calls it with the node pushed (0x486522). Only reachable through that table. First-boot track:
//   hs_compile_postprocess re-parses every primitive when a scenario's scripts load.
// objdump 0x486b80..0x486c42: after an optional '-', every character must be a digit (isdigit 0x62532f), else
//   "this is not a valid integer." at the node's offset. The value is atoi (0x625926) of the whole token. A valid
//   short outside -32768..32767 records "shorts must be in the range [-32767, 32768].". A long (type 8) stores all
//   32 bits at +0x10, a short the low word, whether or not it was valid. Returns whether it was valid.
// blam-cc: stack -> node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern data_array *hs_syntax_data;       // 0x0087a474
extern char *hs_compiled_source;         // 0x006b14c0
extern char *hs_compile_error;           // 0x006b14d4
extern int32_t hs_compile_error_offset;  // 0x006b14d8
#include <ctype.h>
#include <stdlib.h>

char hs_parse_integer(datum_index node_index)
{
    hs_syntax_node *node = (hs_syntax_node *)((uint8_t *)hs_syntax_data->data + (node_index & 0xffff) * 0x14);
    char *p = hs_compiled_source + node->source_offset;
    char valid = 1;
    int32_t value;

    if (*p == '-') {
        p++;
    }
    for (; *p != 0; p++) {
        if (!isdigit((unsigned char)*p)) { // the original passes the sign-extended char; scripts are ASCII
            hs_compile_error = (char *)"this is not a valid integer.";
            hs_compile_error_offset = node->source_offset;
            valid = 0;
            break;
        }
    }
    value = atoi(hs_compiled_source + node->source_offset);
    if (valid && node->type != 8 && (value > 0x7fff || value < -0x8000)) {
        hs_compile_error = (char *)"shorts must be in the range [-32767, 32768].";
        hs_compile_error_offset = node->source_offset;
        valid = 0;
    }
    if (node->type == 8) {
        node->data.long_value = value;
    } else {
        node->data.short_value = (int16_t)value;
    }
    return valid;
}
