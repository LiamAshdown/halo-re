// cache_io_wait_for_flag  (Ghidra: FUN_00442ce0; renamed -- not Bungie-attested, chosen to
// describe what the code does. Skipped by out/phase4/cache_types_notes.md item 6 as a "generic
// Win32 alertable-wait helper"; written here because its single caller was otherwise forced to
// declare an invented prototype for it, and because the argument it actually takes -- a bare
// completion flag, not a completion record -- was being got wrong.)
// address 0x442ce0, size 44 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: the body is three instructions of evidence on its own -- `cmp BYTE PTR [esi],0` at
// entry, `mov al,BYTE PTR [esi]` on both exits -- so ESI is a uint8_t*, and the only call site,
// cache_file_slot_read_header @0x4435e0, passes `lea esi,[esp+0x13]` (0x443681), the same byte
// slot it initialized as the cache_io_completion::flag of the request it submitted and the same
// one it tests immediately after this returns (0x44368a). The 0xc0 the SleepEx result is
// compared against is WAIT_IO_COMPLETION, i.e. "an APC ran"; cache_io_completion_routine
// @0x443b00 is exactly such an APC and is what sets this flag.
// register convention: flag in ESI. No stack parameters.
//
// UNSURE: the loop exits on any SleepEx return other than WAIT_IO_COMPLETION, including a plain
// timeout return of 0 after the full 5 seconds, so the returned byte can be 0 -- a timeout is
// indistinguishable from a completed read except through that return value, which the one caller
// ignores in favour of reading the flag itself. Reproduced literally.

#include "tags.h"
#include "cache.h"

extern uint32_t __stdcall SleepEx(uint32_t milliseconds, int32_t alertable); // 0x0063a290 IAT

// blam-cc: flag in ESI
// Blocks the calling thread in an alertable wait until `*flag` is set by an IO completion APC.
// Returns as soon as the flag is already set, and otherwise sleeps alertably in 5-second slices
// for as long as each slice is cut short by an APC running. Returns the flag's final value.
uint8_t cache_io_wait_for_flag(uint8_t *flag)
{
    uint32_t wait_result;

    if (*flag != 0) {
        return *flag;
    }

    do {
        wait_result = SleepEx(5000, 1);
        if (wait_result != 0xc0) { // WAIT_IO_COMPLETION
            break;
        }
    } while (*flag == 0);

    return *flag;
}

#if 0
Original Ghidra decompilation (0x442ce0):

char FUN_00442ce0(void)

{
  DWORD DVar1;
  char *unaff_ESI;

  if (*unaff_ESI != '\0') {
    return *unaff_ESI;
  }
  do {
    DVar1 = SleepEx(5000,1);
    if (DVar1 != 0xc0) break;
  } while (*unaff_ESI == '\0');
  return *unaff_ESI;
}

Raw disassembly (0x442ce0-0x442d0b), objdump -d -M intel --start-address=0x442ce0
--stop-address=0x442d0c bin/halo.exe:

00442ce0: cmp BYTE PTR [esi],0x0
00442ce3: jne 0x442d09
00442ce5: push edi
00442ce6: mov edi,DWORD PTR ds:0x63a290    ; SleepEx
00442cec: lea esp,[esp+0x0]                ; alignment padding
00442cf0: push 0x1                         ; bAlertable = TRUE
00442cf2: push 0x1388                      ; 5000 ms
00442cf7: call edi
00442cf9: cmp eax,0xc0                     ; WAIT_IO_COMPLETION
00442cfe: jne 0x442d05
00442d00: cmp BYTE PTR [esi],0x0
00442d03: je 0x442cf0
00442d05: mov al,BYTE PTR [esi]
00442d07: pop edi
00442d08: ret
00442d09: mov al,BYTE PTR [esi]            ; the already-set fast path
00442d0b: ret
#endif
