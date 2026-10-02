// network_log_path_resolve  (Ghidra: FUN_004e40a0; renamed -- see evidence)
// address 0x4e40a0, size 79 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md's own summary ("builds a default server/profile
// name string") does not match the code: the only work here is zeroing a static 0x104 buffer,
// asking security_check_write_access() whether the process can write to the requested location,
// and formatting a caller-supplied string into the buffer with a plain "%s" (twice, once gated
// on the access check and once unconditionally if the buffer is still empty -- both calls format
// the identical source string, so the second is only ever reached when the first branch was
// skipped by the access check). Its only two call sites (this batch's network_banlist_save at
// 0x4e3380 and a foreign network_stats_summary_log_open at 0x440670, see
// src/networking/network_stats_summary_log_open.c) both use its result immediately as an fopen
// path, which is what the name reflects.
// register convention: no formal parameters recognized by Ghidra, but network_banlist_save's own
// disassembly (`mov esi,0x71c308` immediately before `call 0x4e40a0`, and this function's body
// doing `push esi` as the "%s" vararg) proves the caller-supplied string arrives in ESI.
//   // blam-cc: ESI -> requested_path
// UNSURE: the two snprintf calls format the exact same ESI value in both branches; the "falling
// back to a second source" a low-confidence summary suggested is not what the retail binary
// does, so this rewrite keeps the literal double check-and-retry shape without inventing a
// second source. UNSURE: the exact text of the fopen mode string at 0x0065fd30, reused here by
// this function's one caller in this batch (not captured by string extraction; assumed to be a
// plain text mode such as "wt"). UNSURE: network_stats_summary_log_open.c's own extern for this
// function was written before ESI's role was known and declares it as taking no arguments; that
// file is outside this batch and is not corrected here.

#include "crt.h"
#include "tags.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t network_log_path_buffer[0x104]; // 0x006b85b8
extern char network_log_path_format[];         // 0x0065efec, UNSURE: assumed to be "%s"

extern int32_t security_check_write_access(void); // 0x542840, foreign module

// blam-cc: ESI -> requested_path
// Zeroes the shared path buffer, then formats requested_path into it with "%s" if the process
// has write access to it; if the buffer is still empty afterward (either the access check
// failed, or requested_path formatted to nothing), formats it in again unconditionally. Always
// returns the shared buffer.
char *network_log_path_resolve(char *requested_path) // blam-cc: ESI -> requested_path
{
    network_log_path_buffer[0] = 0;
    if (security_check_write_access() != 0) {
        _snprintf((char *)network_log_path_buffer, 0x104, network_log_path_format, requested_path);
    }
    if (network_log_path_buffer[0] == 0) {
        _snprintf((char *)network_log_path_buffer, 0x104, network_log_path_format, requested_path);
    }
    return (char *)network_log_path_buffer;
}

#if 0
Original Ghidra decompilation (0x4e40a0), from tools/pack.py 0x4e40a0:

undefined1 * FUN_004e40a0(void)

{
  int iVar1;

  DAT_006b85b8 = '\0';
  iVar1 = security_check_write_access();
  if (iVar1 != 0) {
    __snprintf(&DAT_006b85b8,0x104,"%s");
  }
  if (DAT_006b85b8 == '\0') {
    __snprintf(&DAT_006b85b8,0x104,"%s");
  }
  return &DAT_006b85b8;
}

Disassembly (objdump -d -M intel, bin/halo.exe) confirms the ESI-carried vararg:
  4e40a0: mov byte ptr [0x6b85b8],0
  4e40a7: call security_check_write_access
  4e40ac: test eax,eax
  4e40ae: je 0x4e40c8
  4e40b0: push esi
  4e40b1: push 0x65efec
  4e40b6: push 0x104
  4e40bb: push 0x6b85b8
  4e40c0: call __snprintf
  4e40c8: mov al,[0x6b85b8]
  4e40cd: test al,al
  4e40cf: jne 0x4e40e9
  4e40d1: push esi
  4e40d2: push 0x65efec
  4e40d7: push 0x104
  4e40dc: push 0x6b85b8
  4e40e1: call __snprintf
  4e40e9: mov eax,0x6b85b8
  4e40ee: ret
And the caller (network_banlist_save, 0x4e3380):
  4e3386: push 0x65fd30      ; fopen mode, staged early for the later fopen call
  4e338b: mov esi,0x71c308   ; ESI = &network_banlist_full_path, this function's real argument
  4e3390: call 0x4e40a0
  4e3395: push eax
  4e3396: call 0x624186      ; fopen(path=eax, mode=0x65fd30)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
