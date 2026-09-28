// input_error_log_once  (Ghidra: already named)
// address 0x492150, size 66 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/input_functions.md summary "Formats and (de-duplicated by error code)
// logs a DirectInput failure message, given an HRESULT-like code and a printf-style
// description."; global 0x0068e544 is input_last_error (input.h globals list, "-1 initially").
// Ghidra recovered a full __cdecl signature, so no register-convention guesswork is needed.
// register convention: __cdecl, both arguments on the stack (error_code, format)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <stdarg.h>

extern int32_t input_last_error; // 0x0068e544

// Formats description (printf-style, with any varargs) into a scratch buffer, but only the
// first time error_code is seen; repeats of the same HRESULT-like code are suppressed. Every
// call site in this build passes a literal description with no varargs, so the formatted text
// is only ever the description itself, but the varargs plumbing is preserved.
void input_error_log_once(int32_t error_code, char *description, ...)
{
    char message[4092];
    va_list args;

    if (error_code != input_last_error) {
        input_last_error = error_code;
        va_start(args, description);
        vsprintf(message, description, args);
        va_end(args);
    }
}

#if 0
Original Ghidra decompilation (0x492150):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void __cdecl input_error_log_once(int param_1,char *param_2)

{
  char local_1000 [4092];
  undefined4 uStack_4;

  uStack_4 = 0x49215a;
  if (param_1 != DAT_0068e544) {
    DAT_0068e544 = param_1;
    _vsprintf(local_1000,param_2,&stack0x0000000c);
  }
  return;
}
#endif
