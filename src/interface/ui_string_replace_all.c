// ui_string_replace_all  (Ghidra: ui_string_replace_all, already named)
// address 0x49be10, size 489 bytes
// name confidence: 0.55   rewrite confidence: 0.4
// evidence: matches the given name; cdecl, all three stack parameters recognized by Ghidra.
// Replaces every occurrence of `search` with `replacement` inside the wide string pointed to by
// `*buffer`, growing the buffer via heap_reallocate when the replacement is longer than the
// search string, and returns the number of replacements made (-1 on allocation failure, 0 if
// `*buffer` is empty or absent).
// register convention: cdecl, all three parameters recognized by Ghidra.
// UNSURE: the growth path's heap_reallocate call shows no visible "old pointer" argument in
// Ghidra (only the new size); by the time that call is reached the local Ghidra tracks as the
// buffer base has already been advanced past the counted occurrences by the preceding scan, so it
// cannot be the real "old" argument. Modeled as `*buffer` (the caller's original, unmoved
// pointer), which is the only value that is actually correct to realloc.
// TYPES-GAP: wcslen (0x625b7a) is used across many modules as wide strlen with no agreed
// name yet; kept as wcslen per that cross-module precedent.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>

extern heap *widget_memory_pool; // 0x006926c4

extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80
extern uint32_t wcslen(const uint16_t *s); // 0x625b7a, wide strlen
extern wchar_t *_wcsstr(const wchar_t *string, const wchar_t *needle); // CRT
extern void *_memmove(void *dest, const void *source, uint32_t size);

// Replaces every occurrence of `search` inside `*buffer` with `replacement`, growing `*buffer`
// through the widget heap if needed. Returns the number of replacements, or -1 if growth failed.
int32_t ui_string_replace_all(wchar_t *search, uint16_t *replacement, wchar_t **buffer)
{
    int32_t search_length;
    uint32_t replacement_length;
    int32_t total_length; // wchar count including the terminating NUL
    int32_t count = 0;
    wchar_t *original;
    wchar_t *base;
    wchar_t *match;

    if (buffer == (wchar_t **)0 || (original = *buffer) == (wchar_t *)0) {
        return 0;
    }

    search_length = (int32_t)wcslen((const uint16_t *)search);
    replacement_length = wcslen(replacement);
    total_length = (int32_t)wcslen((const uint16_t *)original) + 1;

    if (search_length < (int32_t)replacement_length) {
        // Replacement grows the string: count occurrences first, then reallocate and copy.
        match = _wcsstr(original, search);
        if (match == (wchar_t *)0) {
            return 0;
        }
        do {
            count = count + 1;
            match = _wcsstr(match + search_length, search);
        } while (match != (wchar_t *)0);

        base = (wchar_t *)heap_reallocate(original,
                                          ((replacement_length - search_length) * count + total_length) * 2,
                                          widget_memory_pool);
        if (base == (wchar_t *)0) {
            return -1;
        }
        match = _wcsstr(base, search);
        if (match != (wchar_t *)0) {
            do {
                uint16_t *src;
                wchar_t *dst;
                uint32_t words;
                uint32_t tail_bytes;

                tail_bytes = (uint32_t)((total_length - (int32_t)((match - base))) - search_length) * 2;
                _memmove(match + replacement_length, match + search_length, tail_bytes);

                src = replacement;
                dst = match;
                for (words = replacement_length >> 1; words != 0; words = words - 1) {
                    *(uint32_t *)dst = *(uint32_t *)src;
                    src += 2;
                    dst += 2;
                }
                if ((replacement_length & 1) != 0) {
                    *dst = (wchar_t)*src;
                }

                total_length = total_length + (replacement_length - search_length);
                match = _wcsstr(base, search);
            } while (match != (wchar_t *)0);
        }
        *buffer = base;
    } else {
        // Replacement is the same length or shorter: rewrite in place, shrinking as we go.
        match = _wcsstr(original, search);
        if (match == (wchar_t *)0) {
            return 0;
        }
        do {
            uint16_t *src = replacement;
            wchar_t *dst = match;
            uint32_t words;

            count = count + 1;
            for (words = replacement_length >> 1; words != 0; words = words - 1) {
                *(uint32_t *)dst = *(uint32_t *)src;
                src += 2;
                dst += 2;
            }
            if ((replacement_length & 1) != 0) {
                *dst = (wchar_t)*src;
            }
            if (search_length > (int32_t)replacement_length) {
                uint32_t tail_bytes = (uint32_t)((total_length - (int32_t)((match - original))) -
                                                  (int32_t)replacement_length) * 2;
                _memmove(match + replacement_length, match + search_length, tail_bytes);
                total_length = total_length - (search_length - replacement_length);
            }
            match = _wcsstr(original, search);
        } while (match != (wchar_t *)0);
        return count;
    }
    return count;
}

#if 0
Original Ghidra decompilation (0x49be10):

int ui_string_replace_all(wchar_t *param_1,undefined4 *param_2,undefined4 *param_3)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  wchar_t *pwVar4;
  wchar_t *pwVar5;
  uint uVar6;
  int iVar7;
  undefined4 *puVar8;
  wchar_t *pwVar9;
  int local_10;

  local_10 = 0;
  iVar7 = 0;
  if ((param_3 != (undefined4 *)0x0) && (pwVar5 = (wchar_t *)*param_3, pwVar5 != (wchar_t *)0x0)) {
    iVar1 = FUN_00625b7a(param_1);
    uVar2 = FUN_00625b7a(param_2);
    iVar3 = FUN_00625b7a(pwVar5);
    iVar3 = iVar3 + 1;
    iVar7 = local_10;
    if (iVar1 < (int)uVar2) {
      pwVar5 = _wcsstr(pwVar5,param_1);
      if (pwVar5 != (wchar_t *)0x0) {
        do {
          iVar7 = local_10 + 1;
          pwVar5 = _wcsstr(pwVar5 + iVar1,param_1);
          local_10 = iVar7;
        } while (pwVar5 != (wchar_t *)0x0);
        if (0 < iVar7) {
          pwVar5 = (wchar_t *)heap_reallocate(((uVar2 - iVar1) * iVar7 + iVar3) * 2);
          if (pwVar5 == (wchar_t *)0x0) {
            return -1;
          }
          pwVar4 = _wcsstr(pwVar5,param_1);
          if (pwVar4 != (wchar_t *)0x0) {
            do {
              _memmove(pwVar4 + uVar2,pwVar4 + iVar1,
                       ((iVar3 - ((int)pwVar4 - (int)pwVar5 >> 1)) - iVar1) * 2);
              puVar8 = param_2;
              for (uVar6 = (uVar2 & 0x7fffffff) >> 1; uVar6 != 0; uVar6 = uVar6 - 1) {
                *(undefined4 *)pwVar4 = *puVar8;
                puVar8 = puVar8 + 1;
                pwVar4 = pwVar4 + 2;
              }
              for (uVar6 = uVar2 * 2 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
                *(undefined1 *)pwVar4 = *(undefined1 *)puVar8;
                puVar8 = (undefined4 *)((int)puVar8 + 1);
                pwVar4 = (wchar_t *)((int)pwVar4 + 1);
              }
              iVar3 = iVar3 + (uVar2 - iVar1);
              pwVar4 = _wcsstr(pwVar5,param_1);
            } while (pwVar4 != (wchar_t *)0x0);
          }
          *param_3 = pwVar5;
        }
      }
    }
    else {
      pwVar4 = _wcsstr(pwVar5,param_1);
      if (pwVar4 != (wchar_t *)0x0) {
        do {
          local_10 = local_10 + 1;
          puVar8 = param_2;
          pwVar9 = pwVar4;
          for (uVar6 = (uVar2 & 0x7fffffff) >> 1; uVar6 != 0; uVar6 = uVar6 - 1) {
            *(undefined4 *)pwVar9 = *puVar8;
            puVar8 = puVar8 + 1;
            pwVar9 = pwVar9 + 2;
          }
          for (uVar6 = uVar2 * 2 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
            *(undefined1 *)pwVar9 = *(undefined1 *)puVar8;
            puVar8 = (undefined4 *)((int)puVar8 + 1);
            pwVar9 = (wchar_t *)((int)pwVar9 + 1);
          }
          if (0 < (int)(iVar1 - uVar2)) {
            _memmove(pwVar4 + uVar2,pwVar4 + iVar1,
                     ((iVar3 - ((int)pwVar4 - (int)pwVar5 >> 1)) - uVar2) * 2);
            iVar3 = iVar3 - (iVar1 - uVar2);
          }
          pwVar4 = _wcsstr(pwVar5,param_1);
        } while (pwVar4 != (wchar_t *)0x0);
        return local_10;
      }
    }
  }
  return iVar7;
}
#endif
