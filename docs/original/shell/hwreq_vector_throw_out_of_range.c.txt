// hwreq_vector_throw_out_of_range  (Ghidra: FUN_0057b9e0; MSVC 7.1 std::vector<T>::_Xran)
// address 0x57b9e0, size 105 bytes
// name confidence: 0.75  rewrite confidence: 0.85
// evidence: the hwreq parser's indexed vtable getters (0x5786f0, 0x578740, 0x5787c0, 0x578810) call it when the
//   index is not below the pair count. objdump 0x57b9e0..0x57ba48: an empty std::string (capacity 0xf) assigned
//   "invalid vector<T> subscript" (0x0067226c, 0x1b characters) with msvc_string_assign_n 0x57bc90, the logic_error
//   constructor (0x5782b0), the vtable replaced by out_of_range's (0x00655098), then
//   _CxxThrowException(&exception, 0x00673560). Never returns.
// blam-cc: no arguments; noreturn

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
typedef struct hwreq_parse_exception {
    uint32_t vtable;         // 0x00
    uint32_t dofree;         // 0x04
    uint32_t legacy_what;    // 0x08
    msvc_std_string message; // 0x0c
} hwreq_parse_exception; // size 0x28

extern hwreq_parse_exception *hwreq_parse_exception_construct(hwreq_parse_exception *self,
    const msvc_std_string *message); // 0x5782b0, blam-cc: ECX -> this, stack -> message
/* _CxxThrowException(void *, _ThrowInfo *) is declared by the C++ runtime headers (CRT: 0x639177) */

extern msvc_std_string *msvc_string_assign_n(msvc_std_string *self, const char *source, uint32_t count); // 0x57bc90
extern void *out_of_range_vtable; // 0x00655098
extern const char string_invalid_vector_subscript[]; // 0x0067226c "invalid vector<T> subscript"
extern uint8_t out_of_range_throw_info[]; // 0x00673560, _ThrowInfo for std::out_of_range

void hwreq_vector_throw_out_of_range(void)
{
    msvc_std_string message;
    hwreq_parse_exception exception;

    message.capacity = 0xf;
    message.size = 0;
    message.buffer.inline_buffer[0] = 0;
    msvc_string_assign_n(&message, string_invalid_vector_subscript, 0x1b);
    hwreq_parse_exception_construct(&exception, &message);
    exception.vtable = (uint32_t)&out_of_range_vtable;
    _CxxThrowException(&exception, (_ThrowInfo *)out_of_range_throw_info);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
