// string_throw_out_of_range  (not a Ghidra function; MSVC 7.1 std::_String_base::_Xran)
// address 0x638e74, size 64 bytes
// name confidence: 0.8  rewrite confidence: 0.85
// evidence: string_erase 0x57bd80, string_compare and string_assign_substr 0x57b830 call it when a position is past
//   the string's size. objdump 0x638e74..0x638eb3: std::string("invalid string position" 0x006550a0) (0x57b520), the
//   logic_error constructor (0x5782b0), the vtable replaced by out_of_range's (0x00655098), then
//   _CxxThrowException(&exception, 0x00673560). Never returns. (Earlier notes called 0x638e74 _Xlen and named it
//   string_throw_length_error; the vtable and message show it is _Xran. _Xlen is 0x638eb4.)
// blam-cc: no arguments; noreturn

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"

typedef struct hwreq_parse_exception {
    uint32_t vtable;         // 0x00
    uint32_t dofree;         // 0x04
    uint32_t legacy_what;    // 0x08
    msvc_std_string message; // 0x0c
} hwreq_parse_exception; // size 0x28

extern hwreq_parse_exception *hwreq_parse_exception_construct(hwreq_parse_exception *this,
    const msvc_std_string *message); // 0x5782b0, blam-cc: ECX -> this, stack -> message
extern __declspec(noreturn) void __stdcall _CxxThrowException(void *object, void *throw_info); // CRT: 0x639177

extern void hwreq_key_string_construct_cstr(msvc_std_string *this, const char *source); // 0x57b520, blam-cc: ECX -> this, stack -> source
extern void *out_of_range_vtable; // 0x00655098
extern const char string_invalid_string_position[]; // 0x006550a0 "invalid string position"
extern uint8_t out_of_range_throw_info[]; // 0x00673560, _ThrowInfo for std::out_of_range

void string_throw_out_of_range(void)
{
    msvc_std_string message;
    hwreq_parse_exception exception;

    hwreq_key_string_construct_cstr(&message, string_invalid_string_position);
    hwreq_parse_exception_construct(&exception, &message);
    exception.vtable = (uint32_t)&out_of_range_vtable;
    _CxxThrowException(&exception, out_of_range_throw_info);
}
