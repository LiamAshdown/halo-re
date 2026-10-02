// string_replace_all_in_place  (Ghidra: FUN_00496df0, unnamed)
// address 0x496df0, size 158 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: sole caller chimera__console_out_copy @0x496e90 calls this twice, immediately after
// strncpy(local_buffer, in_EAX, 0x100) -- cdecl strncpy returns its destination pointer in EAX,
// which is why the buffer to edit arrives here as an unrecognized (in_EAX) register argument
// instead of a third stack parameter. It replaces every occurrence of the search token with the
// replacement token in place, shifting the tail of the buffer with memmove; unlike the wide,
// reallocating ui_string_replace_all @0x49be10, this version assumes the caller's buffer already
// has room and never grows it.
// register convention: buffer to edit in EAX (in_EAX, unresolved register read), search token
// and replacement token as the two recognized stack parameters.
// blam-cc: EAX -> buffer, then (search, replacement) on the stack

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


// blam-cc: EAX -> buffer, then (search, replacement) on the stack
// Replaces every occurrence of `search` inside `buffer` with `replacement`, in place. Assumes
// the caller's buffer already has room for the (possibly longer) result; does not reallocate.
// UNSURE: the tail-move size is computed from a one-past-the-NUL pointer taken from buffer's
// length before the loop starts, not recomputed per iteration; if replacement is a different
// length than search this is preserved verbatim from the original (not "fixed" here), and the
// original also unconditionally strlen()s buffer before the NULL check below ever runs.
void string_replace_all_in_place(char *buffer, char *search, char *replacement)
{
    uint32_t search_length;
    uint32_t replacement_length;
    char *buffer_end; // one past buffer's NUL, computed once before any replacement happens
    char *cursor;

    search_length = strlen(search);
    replacement_length = strlen(replacement);
    buffer_end = buffer + strlen(buffer) + 1;

    cursor = buffer;
    if (buffer != (char *)0) {
        while ((cursor = strstr(cursor, search)) != (char *)0) {
            uint32_t tail_size = (uint32_t)(buffer_end - cursor) - 1;

            memmove(cursor, replacement, replacement_length);
            memmove(cursor + replacement_length, cursor + search_length, tail_size);
        }
    }
}

#if 0
Original Ghidra decompilation (0x496df0):

void FUN_00496df0(char *param_1,char *param_2)

{
  char cVar1;
  char *in_EAX;
  char *pcVar2;
  char *pcVar3;
  char *pcVar4;
  uint uVar5;
  uint uVar6;
  char *pcVar7;
  char *pcVar8;

  pcVar2 = in_EAX;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  pcVar3 = param_1;
  do {
    cVar1 = *pcVar3;
    pcVar3 = pcVar3 + 1;
  } while (cVar1 != '\0');
  pcVar4 = param_2;
  do {
    cVar1 = *pcVar4;
    pcVar4 = pcVar4 + 1;
  } while (cVar1 != '\0');
  uVar5 = (int)pcVar4 - (int)(param_2 + 1);
  pcVar4 = in_EAX;
  if (in_EAX != (char *)0x0) {
    while (pcVar4 = (char *)FUN_00625430(pcVar4,param_1), pcVar4 != (char *)0x0) {
      pcVar7 = param_2;
      pcVar8 = pcVar4;
      for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
        *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
        pcVar7 = pcVar7 + 4;
        pcVar8 = pcVar8 + 4;
      }
      for (uVar6 = uVar5 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
        *pcVar8 = *pcVar7;
        pcVar7 = pcVar7 + 1;
        pcVar8 = pcVar8 + 1;
      }
      _memmove(pcVar4 + uVar5,pcVar4 + ((int)pcVar3 - (int)(param_1 + 1)),
               (size_t)(in_EAX + (int)(pcVar2 + (-(int)pcVar4 - (int)(in_EAX + 1)))));
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
