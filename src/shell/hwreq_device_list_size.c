// hwreq_device_list_size  (already named; task-provided)
// address 0x57b5b0, size 33 bytes
// name confidence: 0.4 (already carries this name; standard MSVC 7.1 std::vector<T>::size()
//   for T = hwreq_string_pair, sizeof(T) = 0x38)
// rewrite confidence: 0.6 (trivial, standard library code)
// evidence: types/shell.h msvc_std_vector (first 0x04, last 0x08); out/phase4/shell_types_notes.md
//   "57b5b0 vector::size". Element stride 0x38 matches hwreq_string_pair, size 0x38.
// register convention: ECX = this (const msvc_std_vector *).
// blam-cc: hwreq_device_list_size(const msvc_std_vector *this /*ECX*/)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

int32_t hwreq_device_list_size(const msvc_std_vector *self)
{
    if (self->first == 0) {
        return 0;
    }
    return ((int32_t)self->last - (int32_t)self->first) / 0x38;
}

#if 0
Original Ghidra decompilation (0x57b5b0):

int hwreq_device_list_size(void)

{
  int in_ECX;

  if (*(int *)(in_ECX + 4) == 0) {
    return 0;
  }
  return (*(int *)(in_ECX + 8) - *(int *)(in_ECX + 4)) / 0x38;
}
#endif
