// string_erase  (already named; task-provided)
// address 0x57bd80, size 117 bytes
// name confidence: 0.5 (already carries this name; standard MSVC 7.1
//   std::basic_string<char>::erase(size_type pos, size_type count))
// rewrite confidence: 0.55 (standard library code, confirmed against the decompilation)
// evidence: types/shell.h msvc_std_string.
// register convention: ECX = this, stack arguments = pos, count.
// blam-cc: string_erase(msvc_std_string *this /*ECX*/, uint32_t pos /*stack*/, uint32_t count /*stack*/)
// UNSURE: FUN_00638e74 (`_Xlen`/`_Xran`) and `_memmove` are opaque externs, not rewritten here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void string_throw_out_of_range(void); // 0x638e74, _Xran: throws out_of_range("invalid string position")
extern void *_memmove(void *dest, const void *src, uint32_t count); // 0x6236f0

void string_erase(msvc_std_string *this, uint32_t pos, uint32_t count)
{
    uint32_t remaining;
    char *buffer;
    uint32_t new_size;

    if (this->size < pos) {
        string_throw_out_of_range();
    }

    remaining = this->size - pos;
    if (remaining < count) {
        count = remaining;
    }

    if (count == 0) {
        return;
    }

    buffer = (this->capacity > 0xf) ? (char *)this->buffer.heap_buffer : this->buffer.inline_buffer;
    _memmove(buffer + pos, buffer + pos + count, remaining - count);

    new_size = this->size - count;
    this->size = new_size;

    buffer = (this->capacity > 0xf) ? (char *)this->buffer.heap_buffer : this->buffer.inline_buffer;
    buffer[new_size] = 0;
}

#if 0
Original Ghidra decompilation (0x57bd80):

void string_erase(uint param_1,uint param_2)

{
  undefined4 *puVar1;
  uint uVar2;
  int iVar3;
  int in_ECX;
  undefined4 *puVar4;
  undefined4 *puVar5;

  if (*(uint *)(in_ECX + 0x14) < param_1) {
    FUN_00638e74();
  }
  uVar2 = *(int *)(in_ECX + 0x14) - param_1;
  if (uVar2 < param_2) {
    param_2 = uVar2;
  }
  if (param_2 != 0) {
    puVar5 = (undefined4 *)(in_ECX + 4);
    puVar4 = puVar5;
    puVar1 = puVar5;
    if (0xf < *(uint *)(in_ECX + 0x18)) {
      puVar4 = (undefined4 *)*puVar5;
      puVar1 = (undefined4 *)*puVar5;
    }
    _memmove((void *)((int)puVar4 + param_1),(void *)((int)puVar1 + param_2 + param_1),
             uVar2 - param_2);
    iVar3 = *(int *)(in_ECX + 0x14) - param_2;
    *(int *)(in_ECX + 0x14) = iVar3;
    if (0xf < *(uint *)(in_ECX + 0x18)) {
      puVar5 = (undefined4 *)*puVar5;
    }
    *(undefined1 *)((int)puVar5 + iVar3) = 0;
  }
  return;
}
#endif
