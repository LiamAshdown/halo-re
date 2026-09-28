// saved_game_load_checkpoint  (Ghidra: saved_game_load_checkpoint, already named)
// address 0x539290, size 155 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/saved_games_functions.md; not present in out/functions.json (no Ghidra
// decompilation), rewritten directly from objdump. EAX (register-passed) is the requested
// checkpoint name: NULL or an empty string falls back to "autosave" (0x006709f4); "*" instead
// lists every checkpoint (autosaves included, newest first) via game_checkpoint_enumerate_files
// with the print callback (game_checkpoint_print_list_entry, 0x539110) and returns without
// loading anything; any other name is passed to saved_game_load_checkpoint_by_name as-is if it
// already contains "checkpoints\\" (strstr, substring search, matching
// game_checkpoint_enumerate_files' own use of it), otherwise formatted as
// "checkpoints\\<name>" first (format 0x66a4dc, confirmed by objdump). EDI throughout is the
// bare (unprefixed) name, which saved_game_load_checkpoint_by_name receives as its own hidden
// EDI parameter.
// register convention: name in EAX; no recognized stack parameters.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t saved_player_profile_slots_handle; // 0x00714dd4

extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory); // 0x53d080, blam-cc: handle in EAX, out buffer in ESI; bool in AL
extern int32_t game_checkpoint_enumerate_files(uint8_t include_autosaves, uint8_t sort_newest_first,
    checkpoint_enumerate_proc callback, void *user_data); // 0x538e70
extern uint8_t game_checkpoint_print_list_entry(int32_t index, const char *name, int32_t level_index,
    int32_t difficulty, int32_t game_time_ticks, const win32_systemtime *time, void *user_data); // 0x539110
extern uint8_t saved_game_load_checkpoint_by_name(char *name); // 0x5391a0

// blam-cc: name in EAX
// Resolves a checkpoint name and loads it: NULL/empty means "autosave", "*" instead prints the
// full checkpoint list and loads nothing, and any other name is loaded via
// saved_game_load_checkpoint_by_name (adding the "checkpoints\\" prefix if not already present).
uint8_t saved_game_load_checkpoint(char *name)
{
    char directory[264];
    char full_name[264];

    saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);

    if (name == 0 || *name == 0) {
        name = "autosave";
    } else if (*name == '*') {
        game_checkpoint_enumerate_files(1, 1, game_checkpoint_print_list_entry, 0);
        return 1;
    }

    if (strstr(name, "checkpoints\\") != 0) {
        return saved_game_load_checkpoint_by_name(name);
    }
    sprintf(full_name, "checkpoints\\%s", name);
    return saved_game_load_checkpoint_by_name(full_name); // 0x539315: only the stack name (EDI is not an input)
}

#if 0
Original disassembly (0x539290, no Ghidra export -- not in out/functions.json):

00539290:
  81 ec 00 02 00 00    sub    esp,0x200
  56                   push   esi
  57                   push   edi
  8b f8                mov    edi,eax                  ; edi = name (EAX param)
  a1 d4 4d 71 00       mov    eax,ds:0x714dd4
  8d b4 24 08 01 00 00 lea    esi,[esp+0x108]
  e8 d5 3d 00 00       call   0x53d080                  ; directory = FUN_0053d080(handle, esi)
  85 ff                test   edi,edi
  74 28                je     0x5392d7
  8a 07                mov    al,[edi]
  84 c0                test   al,al
  74 22                je     0x5392d7
  3c 2a                cmp    al,0x2a
  75 23                jne    0x5392dc
  6a 00                push   0x0
  68 10 91 53 00       push   0x539110                  ; game_checkpoint_print_list_entry
  6a 01                push   0x1
  6a 01                push   0x1
  e8 a7 fb ff ff       call   0x538e70                  ; game_checkpoint_enumerate_files
  83 c4 10             add    esp,0x10
  5f                   pop    edi
  b0 01                mov    al,0x1
  5e                   pop    esi
  81 c4 00 02 00 00    add    esp,0x200
  c3                   ret
005392d7:
  bf f4 09 67 00       mov    edi,0x6709f4              ; "autosave"
005392dc:
  68 00 0a 67 00       push   0x670a00                  ; "checkpoints\"
  57                   push   edi
  e8 49 c1 0e 00       call   0x625430                  ; FUN_00625430(edi, "checkpoints\")
  83 c4 08             add    esp,0x8
  85 c0                test   eax,eax
  74 14                je     0x539302
  8d 54 24 08          lea    edx,[esp+0x8]
  8b c7                mov    eax,edi
  2b d7                sub    edx,edi
  8a 08                mov    cl,[eax]
  88 0c 02             mov    [edx+eax],cl
  40                   inc    eax
  84 c9                test   cl,cl
  75 f6                jne    0x5392f6
  eb 13                jmp    0x539315
00539302:
  57                   push   edi
  8d 44 24 0c          lea    eax,[esp+0xc]
  68 dc a4 66 00       push   0x66a4dc                  ; "checkpoints\%s"
  50                   push   eax
  e8 81 a3 0e 00       call   0x623693                  ; _sprintf
  83 c4 0c             add    esp,0xc
00539315:
  8d 4c 24 08          lea    ecx,[esp+0x8]
  51                   push   ecx
  e8 81 fe ff ff       call   0x5391a0                  ; saved_game_load_checkpoint_by_name
  83 c4 04             add    esp,0x4
  5f                   pop    edi
  5e                   pop    esi
  81 c4 00 02 00 00    add    esp,0x200
  c3                   ret
#endif
