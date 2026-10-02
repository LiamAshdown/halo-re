// string_throw_length_error  (not a Ghidra function; MSVC 7.1 std::_String_base::_Xlen)
// address 0x638eb4, size 64 bytes
// name confidence: 0.8  rewrite confidence: 0.85
// evidence: msvc_string_assign_n 0x57bc90 and string_assign_substr 0x57b830 call it when a count exceeds max_size
//   (0xfffffffe). objdump 0x638eb4..0x638ef3: std::string("string too long" 0x006550b8) (0x57b520), the logic_error
//   constructor (0x5782b0), the vtable replaced by length_error's (0x0065508c), then
//   _CxxThrowException(&exception, 0x00673524). Never returns.
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

extern void hwreq_key_string_construct_cstr(msvc_std_string *self, const char *source); // 0x57b520, blam-cc: ECX -> this, stack -> source
extern void *length_error_vtable; // 0x0065508c
extern const char string_string_too_long[]; // 0x006550b8 "string too long"
extern uint8_t length_error_throw_info[]; // 0x00673524, _ThrowInfo for std::length_error

void string_throw_length_error(void)
{
    msvc_std_string message;
    hwreq_parse_exception exception;

    hwreq_key_string_construct_cstr(&message, string_string_too_long);
    hwreq_parse_exception_construct(&exception, &message);
    exception.vtable = (uint32_t)&length_error_vtable;
    _CxxThrowException(&exception, (_ThrowInfo *)length_error_throw_info);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
