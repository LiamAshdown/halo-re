// hwreq_string_assign_cstr  (orphan pass 4: FUN_0057b590, no Ghidra name)
// address 0x57b590, size 30 bytes
// name confidence: 0.5 (MSVC 7.1 std::basic_string<char>::assign(const char *s): computes
//   strlen(s) and forwards to the assign(const char*, size_t) helper, out/phase4/shell_types_notes.md's
//   "57b590 string::assign(const char *)")
// rewrite confidence: 0.55 (standard library code, trivial strlen loop, confirmed against the
//   decompilation)
// evidence: types/shell.h msvc_std_string.
// register convention: EDX = const char *s (in_EDX). Callee FUN_0057bc90 is
//   `string::assign(const char*, size_t)`, module=lib:crt per pack.py, not this pass.
// blam-cc: EDX -> s, stack -> dest (popped by ret 4)
// FIXED (register inputs, objdump): the note wrote the register mapping as a call-style
// signature annotation ("dest /*stack*/, s /*EDX*/"), which the checker does not parse as a
// register mapping, so EDX -> s was dropped; rewritten in the plain "REG -> name" form.
// Orphan pass 4 review (objdump 0x57b590..0x57b5ab): the destination is NOT implicit. It is the
//   single stack argument (`mov ecx,[esp+0x8]` after one push, then `ret 0x4`), loaded into ECX
//   as the `this` of the thiscall string::assign(const char *, size_t) at 0x57bc90, which gets
//   (s, strlen(s)) on the stack. Both callers push the destination and pass the source in EDX
//   (e.g. 0x57aa19..0x57aa1f). The earlier rewrite dropped the destination.
// UNSURE: FUN_0057bc90 (string::assign(const char*, size_t)) is an opaque lib:crt extern, not
//   rewritten here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern msvc_std_string *msvc_string_assign_n(msvc_std_string *dest, const char *s, uint32_t count); // 0x57bc90, blam-cc: ECX dest, stack (s, count)

msvc_std_string *hwreq_string_assign_cstr(msvc_std_string *dest, const char *s)
{
    const char *cursor = s;
    do {
        // matches the original's post-increment loop, which leaves cursor one past the
        // terminator so that (cursor - (s + 1)) == strlen(s)
        cursor++;
    } while (*(cursor - 1) != '\0');
    return msvc_string_assign_n(dest, s, (uint32_t)(cursor - (s + 1)));
}

#if 0
Original Ghidra decompilation (0x57b590):

void FUN_0057b590(void)

{
  char cVar1;
  char *pcVar2;
  char *in_EDX;

  pcVar2 = in_EDX;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  FUN_0057bc90(in_EDX,(int)pcVar2 - (int)(in_EDX + 1));
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
