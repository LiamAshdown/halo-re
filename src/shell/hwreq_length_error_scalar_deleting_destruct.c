// hwreq_length_error_scalar_deleting_destruct  (orphan pass 4: FUN_005783c0, no Ghidra name)
// address 0x5783c0, size 30 bytes
// name confidence: 0.5 (standard MSVC scalar deleting destructor thunk for
//   hwreq_length_error_destruct 0x5783b0, this pass; same shape as
//   hwreq_parse_exception_scalar_deleting_destruct.c)
// rewrite confidence: 0.6 (trivial, standard MSVC thunk shape)
// evidence: src/shell/README.md: "0x5783b0 / 0x5783c0 are length_error (destructor and scalar
//   deleting destructor)".
// register convention: ECX = this, stack byte free_flag (bit 0).
// blam-cc: hwreq_length_error_scalar_deleting_destruct(hwreq_parse_exception *this /*ECX*/, uint8_t free_flag /*stack*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

typedef struct hwreq_parse_exception hwreq_parse_exception; // opaque here; see hwreq_parse_exception_construct.c

extern void hwreq_length_error_destruct(hwreq_parse_exception *this); // 0x5783b0, same pass
extern void _free(void *ptr); // 0x6277e8

void hwreq_length_error_scalar_deleting_destruct(hwreq_parse_exception *this, uint8_t free_flag)
{
    hwreq_length_error_destruct(this);
    if (free_flag & 1) {
        _free(this);
    }
}

#if 0
Original Ghidra decompilation (0x5783c0):

void FUN_005783c0(byte param_1)

{
  void *in_ECX;

  FUN_005783b0();
  if ((param_1 & 1) != 0) {
    _free(in_ECX);
  }
  return;
}
#endif
