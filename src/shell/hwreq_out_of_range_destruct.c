// hwreq_out_of_range_destruct  (orphan pass 4: FUN_005783e0, no Ghidra name)
// address 0x5783e0, size 11 bytes
// name confidence: 0.5 (MSVC 7.1 std::out_of_range::~out_of_range; src/shell/README.md:
//   "0x5783e0 / 0x5783f0 are `out_of_range` (destructor and scalar deleting destructor)")
// rewrite confidence: 0.55 (trivial, same shape as hwreq_length_error_destruct.c)
// evidence: src/shell/README.md / out/phase4/shell_types_notes.md, "Misattributed entries".
// register convention: ECX = this.
// blam-cc: hwreq_out_of_range_destruct(hwreq_parse_exception *this /*ECX*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

typedef struct hwreq_parse_exception hwreq_parse_exception; // opaque here; see hwreq_parse_exception_construct.c

extern void *out_of_range_vtable; // 0x00655098
extern void hwreq_parse_exception_destruct(hwreq_parse_exception *self); // 0x578310, same pass

void hwreq_out_of_range_destruct(hwreq_parse_exception *self)
{
    *(void **)self = &out_of_range_vtable;
    hwreq_parse_exception_destruct(self);
}

#if 0
Original Ghidra decompilation (0x5783e0):

void FUN_005783e0(void)

{
  undefined4 *in_ECX;

  *in_ECX = &PTR_FUN_00655098;
  hwreq_parse_exception_destruct();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
