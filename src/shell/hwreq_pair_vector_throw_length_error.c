// hwreq_pair_vector_throw_length_error  (Ghidra: FUN_0057c130; MSVC 7.1 std::vector<T>::_Xlen)
// address 0x57c130, size 105 bytes
// name confidence: 0.75  rewrite confidence: 0.85
// evidence: hwreq_pair_vector_insert_n 0x57be20 calls it when size + count would pass max_size (0x4924924).
//   objdump 0x57c130..0x57c198: an empty std::string assigned "vector<T> too long" (0x00672258, 0x12 characters)
//   with msvc_string_assign_n 0x57bc90, the logic_error constructor (0x5782b0), the vtable replaced by
//   length_error's (0x0065508c), then _CxxThrowException(&exception, 0x00673524). Never returns.
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
extern __declspec(noreturn) void __stdcall _CxxThrowException(void *object, void *throw_info); // CRT: 0x639177
extern msvc_std_string *msvc_string_assign_n(msvc_std_string *self, const char *source, uint32_t count); // 0x57bc90
extern void *length_error_vtable; // 0x0065508c
extern const char string_vector_too_long[]; // 0x00672258 "vector<T> too long"
extern uint8_t length_error_throw_info[]; // 0x00673524, _ThrowInfo for std::length_error

void hwreq_pair_vector_throw_length_error(void)
{
    msvc_std_string message;
    hwreq_parse_exception exception;

    message.capacity = 0xf;
    message.size = 0;
    message.buffer.inline_buffer[0] = 0;
    msvc_string_assign_n(&message, string_vector_too_long, 0x12);
    hwreq_parse_exception_construct(&exception, &message);
    exception.vtable = (uint32_t)&length_error_vtable;
    _CxxThrowException(&exception, length_error_throw_info);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
