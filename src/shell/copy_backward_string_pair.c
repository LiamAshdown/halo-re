// copy_backward_string_pair  (Ghidra: FUN_0057cf10; MSVC 7.1 std::copy_backward over hwreq_string_pair)
// address 0x57cf10, size 59 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence: hwreq_pair_vector_insert_n 0x57be20 calls it to open the hole in place. objdump 0x57cf10..0x57cf4a:
//   walks both ranges down one 0x38-byte pair at a time, assigning first and second with
//   string_assign_substr(dest, source, 0, npos) (0x57b830), until the source reaches first; returns the new
//   destination start.
// blam-cc: EBX -> first, ECX -> last, EAX -> dest_end; returns the destination start

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern msvc_std_string *string_assign_substr(msvc_std_string *this, const msvc_std_string *right, uint32_t pos,
    uint32_t count); // 0x57b830

hwreq_string_pair *copy_backward_string_pair(hwreq_string_pair *first, hwreq_string_pair *last,
    hwreq_string_pair *dest_end)
{
    while (last != first) {
        last--;
        dest_end--;
        string_assign_substr(&dest_end->first, &last->first, 0, 0xffffffff);
        string_assign_substr(&dest_end->second, &last->second, 0, 0xffffffff);
    }
    return dest_end;
}
