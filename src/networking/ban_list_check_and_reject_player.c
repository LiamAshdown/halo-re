// ban_list_check_and_reject_player  (Ghidra: ban_list_check_and_reject_player, already named)
// address 0x4e3820, size 98 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md; the literal "Rejecting banned player %s (%s)."
// string; ban_list_entry (indefinite/expiry_time). Disassembly (objdump -d -M intel) shows the
// key argument arrives in EDI (spilled by the function's own `push edi` prologue slot, which is
// exactly the stack argument ban_list_find_by_name.c reads), stays untouched throughout, and is
// the second vararg of the final chimera__console_out call; the found entry pointer (ESI) is the
// first vararg, printed as "%s" via its leading `name` field.
// register convention: EDI -> key (see evidence above).
//   // blam-cc: EDI -> key
// UNSURE: FUN_006267d1 is a foreign (< this module's start) helper that Ghidra's decompile
// treats as returning a float80 and compares against 0.0; from its inputs (two time_t-shaped
// values) it is almost certainly a signed subtraction wrapper (the __allmul/__alldiv-family
// idiom noted elsewhere in this codebase), so this rewrite reads it as a plain integer compare
// (now < expiry_time) rather than reproducing an x87 compare.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <time.h>

extern ban_list_entry *ban_list_find_by_name(char *key); // this batch, 0x4e37d0
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Looks up `key` (a CD-key hash) in the ban list; if found and either indefinite or not yet
// expired, prints a rejection message and returns 1 (reject), otherwise returns 0 (allow).
uint8_t ban_list_check_and_reject_player(char *key) // blam-cc: EDI -> key
{
    ban_list_entry *entry;
    time_t now;

    entry = ban_list_find_by_name(key);
    if (entry != 0) {
        if (entry->indefinite == 0) {
            time(&now);
            if (now < entry->expiry_time) {
                return 0;
            }
        }
        chimera__console_out((ColorARGB *)0, (char *)"Rejecting banned player %s (%s).", entry, key);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4e3820), from tools/pack.py 0x4e3820:

undefined1 ban_list_check_and_reject_player(void)

{
  int iVar1;
  undefined1 uVar2;
  float10 fVar3;
  __time32_t local_4;

  uVar2 = 0;
  iVar1 = ban_list_find_by_name();
  if (iVar1 != 0) {
    if (*(char *)(iVar1 + 0x30) == '\0') {
      FID_conflict___time32(&local_4);
      fVar3 = (float10)FUN_006267d1(local_4,*(undefined4 *)(iVar1 + 0x34));
      if ((float10)0.0 <= fVar3) {
        return 0;
      }
    }
    uVar2 = 1;
    chimera__console_out("Rejecting banned player %s (%s).",iVar1);
  }
  return uVar2;
}

Disassembly (objdump -d -M intel, bin/halo.exe) confirms EDI carries the key argument and is the
call's second vararg:
  4e3820: push ecx           ; stack-space padding, value unused
  4e3821: push ebx
  4e3822: push esi
  4e3823: push edi           ; spills EDI = key; this exact slot is ban_list_find_by_name's arg
  4e3826: call ban_list_find_by_name
  ...
  4e3863: push edi           ; key (third/last pushed = second vararg)
  4e3864: push esi           ; entry (second pushed = first vararg)
  4e3865: push 0x66d6f8      ; "Rejecting banned player %s (%s)."
  4e386e: call chimera__console_out
#endif
