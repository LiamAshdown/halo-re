// autopatch_current_version_string_get  (Ghidra: autopatch_current_version_string_get, already named)
// address 0x578190, size 38 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md summary: "Writes the hardcoded current game build
// version string (\"01.00.10.0621\") into a caller buffer." The four dwords decode exactly to
// that ASCII text plus a NUL.
// register convention: destination buffer in EAX (in_EAX, unresolved register read).
// blam-cc: EAX -> out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

// blam-cc: EAX -> out
// Writes the hardcoded current game build version string ("01.00.10.0621") into out (a 14-byte
// buffer, including the NUL).
void autopatch_current_version_string_get(char *out)
{
    int32_t i;
    static const char version[] = "01.00.10.0621";
    for (i = 0; i <= 13; i++) {
        out[i] = version[i];
    }
}

#if 0
Original Ghidra decompilation (0x578190):

void autopatch_current_version_string_get(void)

{
  undefined4 *in_EAX;

  *in_EAX = 0x302e3130;
  in_EAX[1] = 0x30312e30;
  in_EAX[2] = 0x3236302e;
  *(undefined2 *)(in_EAX + 3) = 0x31;
  return;
}
#endif
