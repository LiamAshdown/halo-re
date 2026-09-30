// join_game_ticker_string_copy  (Ghidra: FUN_004b6160, still unnamed -> renamed)
// address 0x4b6160, size 63 bytes
// name confidence: 0.45   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("copies one of the localized 'join
// game' ticker status strings (selected by index) from a tag's string list into the caller's
// buffer"); out/phase2/networking/00.md's one visible call site is `FUN_004b6160(4);` -- a
// literal string index argument that this function's own decompile never names (it shows no
// parameters at all, only unaff_ESI/unaff_EBX for the output buffer and its capacity).
// register convention: output buffer in ESI (unaff_ESI), capacity in EBX (unaff_EBX);
// string_index is a genuine argument (proven by the call site) that this function's body never
// reads directly -- it is reconstructed here as an explicit stack parameter forwarded into
// text_string_list_get_string, matching the pattern already used for network_channel_attempt_connect's
// unused_param_1.
// UNSURE: text_string_list_get_string's real signature is not recovered; declared here as
// (tag_index, string_index) purely from this call site's evidence.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include <wchar.h>
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern int32_t tag_lookup(const char *path); // foreign, tags module
extern uint16_t *text_string_list_get_string(int32_t tag_index, int32_t string_index); // foreign, see UNSURE

// blam-cc: output buffer in ESI (unaff_ESI), capacity in EBX (unaff_EBX), string_index is a
// stack parameter
// Looks up the join-game ticker labels tag; if found, copies its string_index'th string into
// buffer (truncated to capacity - 1 characters, always NUL-terminated). Leaves buffer as an
// empty string if the tag is missing.
void join_game_ticker_string_copy(uint16_t *buffer, int32_t capacity, int32_t string_index)
{
    int32_t tag_index;
    uint16_t *source;

    *buffer = 0;
    tag_index = tag_lookup(
        "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_ticker_labels");
    if (tag_index != -1) {
        source = text_string_list_get_string(tag_index, string_index);
        wcsncpy(buffer, source, capacity - 1);
        buffer[capacity - 1] = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4b6160):

void FUN_004b6160(void)

{
  int iVar1;
  wchar_t *_Source;
  int unaff_EBX;
  wchar_t *unaff_ESI;

  *unaff_ESI = L'\0';
  iVar1 = tag_lookup(
                    "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_ticker_labels"
                    );
  if (iVar1 != -1) {
    _Source = (wchar_t *)text_string_list_get_string();
    _wcsncpy(unaff_ESI,_Source,unaff_EBX - 1);
    unaff_ESI[unaff_EBX + -1] = L'\0';
  }
  return;
}
#endif
