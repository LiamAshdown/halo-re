// message_delta_metrics_dump  (Ghidra: message_delta_metrics_dump, already named)
// address 0x4ec3d0, size 120 bytes
// name confidence: 0.55   rewrite confidence: 0.35
// evidence: strings "message metrics", "%s\\%s %s", "Message Summary.txt",
// "Wrote network message metrics to %s"; out/phase4/networking_functions.md summary. This
// function has no callers anywhere in the 454-function module list, so it is dead code in this
// build (or called only from outside the module).
// register convention: an unresolved string in ESI (unaff_ESI); the format string
// "%s\\%s %s" takes three substitutions but the decompilation shows only two ("message
// metrics", the "Message Summary.txt"-shaped global), so unaff_ESI is almost certainly the third,
// register-forwarded argument.
// blam-cc: ESI -> suffix (UNSURE name)
// UNSURE: unaff_ESI's own contribution -- its length is computed and then discarded before the
// snprintf call, matching a caller that checked it for non-NULL/non-empty first; transcribed as
// an unused local exactly as decompiled, with the register value passed as the format's third
// substitution.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char message_delta_metrics_filename_suffix[]; // 0x0069b24c, UNSURE: e.g. a map/scenario name

extern int32_t snprintf(char *dest, uint32_t count, const char *format, ...);
extern void console_printf_verbose(const char *format, ...); // logging helper, other module

// blam-cc: ESI -> suffix (UNSURE name)
// Builds the output path for a network-message-metrics dump ("message metrics\<suffix> <name>")
// and logs a confirmation that it was written. No caller of this function exists anywhere in the
// 454-function networking module, so the actual dump/write step this path would feed is not
// present here.
void message_delta_metrics_dump(char *suffix)
{
    char path[260];

    if (suffix != 0) {
        int32_t len;
        for (len = 0; suffix[len] != 0; len++) {
        }
        (void)len; // UNSURE: computed but never used, matching the original
    }
    snprintf(path, 0x104, "%s\\%s %s", "message metrics", message_delta_metrics_filename_suffix, suffix);
    console_printf_verbose("Wrote network message metrics to %s", path);
}

#if 0
Original Ghidra decompilation (0x4ec3d0):

void message_delta_metrics_dump(void)

{
  char cVar1;
  char *unaff_ESI;
  char local_104 [260];

  if (unaff_ESI != (char *)0x0) {
    do {
      cVar1 = *unaff_ESI;
      unaff_ESI = unaff_ESI + 1;
    } while (cVar1 != '\0');
  }
  __snprintf(local_104,0x104,"%s\\%s %s","message metrics",&DAT_0069b24c);
  FUN_00496a80("Wrote network message metrics to %s",local_104);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
