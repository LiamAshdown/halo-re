// string_grow_reserve  (already named; task-provided)
// address 0x57c6d0, size 274 bytes (0x57c6d0..0x57c7e1; 115 bytes in Ghidra's own count), the real function extends through
//   0x57c7df (see UNSURE -- 0x57c76c is not a separate function)
// name confidence: 0.55 (already carries this name; standard MSVC 7.1
//   std::basic_string<char>::_Grow(size_type new_capacity, size_type preserve_count):
//   1.5x-growth capacity policy, allocates a new buffer, copies the preserved prefix into it,
//   frees the old heap buffer if any, and installs the new buffer/capacity/size)
// rewrite confidence: 0.45 (the capacity-growth arithmetic and buffer swap are confirmed
//   against objdump; this required reading past Ghidra's own function boundary, see UNSURE)
// evidence: types/shell.h msvc_std_string.
// register convention (confirmed via objdump): ECX = this, stack arguments = new_capacity
//   (param_1, recognized by Ghidra), preserve_count (a 2nd stack argument at [ebp+0xc] that
//   Ghidra's own decompile of this function never recognized -- read only after the
//   malloc call, past where Ghidra stopped tracking it as a parameter).
// UNSURE: Ghidra split this function's tail into a second, separately-named "function",
//   string_copy_into_new_buffer 0x57c76c -- but objdump shows the only path into 0x57c76c is a
//   fall-through `jmp` from this function's own body (0x57c741) or a "load a fixed address into
//   EAX then `ret`" trampoline (0x57c75d) that lands on it, never a `call`. 0x57c76c has no
//   prologue of its own and is not a function; this file covers the address range 0x57c6d0..
//   0x57c7df in full and no separate file is written for 0x57c76c (recorded in
//   out/phase4/orphans_notes.md).
// UNSURE: the 1.5x growth arithmetic uses a magic-number multiply in the binary
//   (0xaaaaaaab, i.e. division by 3 via reciprocal multiplication); reproduced here with plain
//   integer division, which is arithmetically equivalent for every value the string library
//   would pass through it.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"


// blam-cc: ECX -> this, stack -> new_capacity, preserve_count
void string_grow_reserve(msvc_std_string *this, uint32_t new_capacity, uint32_t preserve_count)
{
    uint32_t capacity = new_capacity | 0xf;
    char *new_buffer;
    char *terminator;

    if (capacity != 0xffffffff) {
        uint32_t current_capacity = this->capacity;
        uint32_t half = current_capacity >> 1;
        if (capacity / 3 < half && current_capacity <= (0xfffffffe - half)) {
            capacity = half + current_capacity;
        }
    } else {
        capacity = new_capacity; // original request, pre-|0xf (0xffffffff case is left alone)
    }

    new_buffer = (char *)malloc(capacity + 1);

    if (preserve_count != 0) {
        const char *old_buffer = (this->capacity < 0x10) ? this->buffer.inline_buffer : (const char *)this->buffer.heap_buffer;
        uint32_t i;
        for (i = 0; i < preserve_count; i++) {
            new_buffer[i] = old_buffer[i];
        }
    }

    if (this->capacity > 0xf) {
        free((void *)this->buffer.heap_buffer);
    }

    this->buffer.inline_buffer[0] = 0;
    this->buffer.heap_buffer = (uint32_t)new_buffer;
    this->capacity = capacity;
    this->size = preserve_count;

    terminator = (capacity >= 0x10) ? new_buffer : this->buffer.inline_buffer;
    terminator[preserve_count] = 0;
}

#if 0
Original Ghidra decompilation, both halves (0x57c6d0 and its fall-through continuation 0x57c76c):

void string_grow_reserve(uint param_1)

{
  uint uVar1;
  int in_ECX;
  uint uVar2;
  uint uVar3;
  void *pvStack_10;
  undefined1 *puStack_c;
  undefined4 local_8;

  puStack_c = &LAB_00639340;
  pvStack_10 = ExceptionList;
  uVar3 = param_1 | 0xf;
  if (uVar3 != 0xffffffff) {
    uVar1 = *(uint *)(in_ECX + 0x18);
    uVar2 = uVar1 >> 1;
    param_1 = uVar3;
    if ((uVar3 / 3 < uVar2) && (uVar1 <= -uVar2 - 2)) {
      param_1 = uVar2 + uVar1;
    }
  }
  local_8 = 0;
  ExceptionList = &pvStack_10;
  operator_new(param_1 + 1);
  string_copy_into_new_buffer();
  return;
}

void string_copy_into_new_buffer(void)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  undefined4 *in_EDX;
  uint unaff_EBX;
  int unaff_EBP;
  int unaff_ESI;
  undefined4 *puVar4;
  undefined4 *puVar5;

  uVar2 = *(uint *)(unaff_EBP + 0xc);
  *(undefined4 *)(unaff_EBP + -4) = 0xffffffff;
  iVar3 = 0;
  if (uVar2 != 0) {
    if (*(uint *)(unaff_ESI + 0x18) < 0x10) {
      puVar4 = (undefined4 *)(unaff_ESI + 4);
    }
    else {
      puVar4 = *(undefined4 **)(unaff_ESI + 4);
    }
    puVar5 = in_EDX;
    for (uVar1 = uVar2 >> 2; uVar1 != 0; uVar1 = uVar1 - 1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
      *(undefined1 *)puVar5 = *(undefined1 *)puVar4;
      puVar4 = (undefined4 *)((int)puVar4 + 1);
      puVar5 = (undefined4 *)((int)puVar5 + 1);
    }
    unaff_ESI = *(int *)(unaff_EBP + -0x14);
    iVar3 = *(int *)(unaff_EBP + 0xc);
  }
  if (0xf < *(uint *)(unaff_ESI + 0x18)) {
    _free(*(void **)(unaff_ESI + 4));
    in_EDX = *(undefined4 **)(unaff_EBP + 8);
    iVar3 = *(int *)(unaff_EBP + 0xc);
  }
  puVar4 = (undefined4 *)(unaff_ESI + 4);
  *(undefined1 *)puVar4 = 0;
  *puVar4 = in_EDX;
  *(uint *)(unaff_ESI + 0x18) = unaff_EBX;
  *(int *)(unaff_ESI + 0x14) = iVar3;
  if (0xf < unaff_EBX) {
    puVar4 = in_EDX;
  }
  *(undefined1 *)((int)puVar4 + iVar3) = 0;
  ExceptionList = *(void **)(unaff_EBP + -0xc);
  return;
}
#endif
