// hwreq_out_of_range_scalar_deleting_destruct  (orphan pass 4: FUN_005783f0, no Ghidra name)
// address 0x5783f0, size 30 bytes
// name confidence: 0.5 (standard MSVC scalar deleting destructor thunk for
//   hwreq_out_of_range_destruct 0x5783e0, this pass)
// rewrite confidence: 0.6 (trivial, standard MSVC thunk shape)
// evidence: src/shell/README.md: "0x5783e0 / 0x5783f0 are out_of_range (destructor and scalar
//   deleting destructor)".
// register convention: ECX = this, stack byte free_flag (bit 0).
// blam-cc: hwreq_out_of_range_scalar_deleting_destruct(hwreq_parse_exception *this /*ECX*/, uint8_t free_flag /*stack*/)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
typedef struct hwreq_parse_exception hwreq_parse_exception; // opaque here; see hwreq_parse_exception_construct.c

extern void hwreq_out_of_range_destruct(hwreq_parse_exception *self); // 0x5783e0, same pass

void hwreq_out_of_range_scalar_deleting_destruct(hwreq_parse_exception *self, uint8_t free_flag)
{
    hwreq_out_of_range_destruct(self);
    if (free_flag & 1) {
        free(self);
    }
}

#if 0
Original Ghidra decompilation (0x5783f0):

void FUN_005783f0(byte param_1)

{
  void *in_ECX;

  FUN_005783e0();
  if ((param_1 & 1) != 0) {
    _free(in_ECX);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
