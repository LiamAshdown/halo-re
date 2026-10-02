// saved_game_allocate_new_slot  (Ghidra: saved_game_allocate_new_slot, already named)
// address 0x53ca80, size 221 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: already named by Ghidra/CEA. out/phase4/saved_games_functions.md summary "Finds the
// next free save-game slot (up to 999) and creates it with an auto-generated localized display
// name from a string-table tag." Confirmed against objdump 0x53ca80..0x53cb5e: EBX (register) is
// the out wide-name pointer; tag_lookup is called with EDI = the 'ustr' tag_group fourcc
// (0x75737472) and the string literal pushed on the stack; string_format_wide_va_bounded takes
// (out_buffer, format_string, number) on the stack plus a max-count of 0x7f in EDX.
// The format string is string 2 of the ui\saved_game_file_strings list when the list has
// more than 2 strings (0x53cae4..0x53cb08: [data] > 2, then [data+4] + 0x28, its size at +0
// and pointer at +0xc), else L"<missing string>" at 0x00671fac; the NUL store into the
// shared tag string is in the binary. The first rewrite indexed tag_data + 0x28 directly and
// skipped the strings.pointer load.
// Phase 4 review (objdump 0x53cab0..0x53cb59): XCreateSaveGame gets out_name in EAX and a
// separate zeroed 0x100-byte stack buffer as out_path (the first rewrite aliased out_name);
// the loop keeps going while the query returns 0 and the number is below 999, and out_name
// is cleared when the counter reaches 999 (the first rewrite stopped one short and never
// cleared it).
// register convention: out wide-name buffer in EBX. No stack arguments.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char savegames_directory[0x100]; // 0x00721549
extern uint16_t missing_string_text[]; // 0x00671fac, the characters of L"<missing string>" (an array, not a pointer: mov reg,0x671fac; src/game uses the same name)
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, EDX count
extern uint32_t XCreateSaveGame(const uint16_t *save_game_name, const char *root_path, int32_t mode, char *out_path,
    uint32_t out_path_size); // 0x551710, blam-cc: EAX save_game_name (src/game/XCreateSaveGame.c: validity_token)

// blam-cc: out wide-name buffer in EBX
// Looks up the ui\\saved_game_file_strings ustr tag and, for each candidate slot number 1..999,
// formats "<string> <number>" (bounded to 0x7f characters) into out_name and asks
// XCreateSaveGame (mode 3, an existence query) whether that name is free. Stops at the first
// free number; clears out_name if all 999 are taken (or the tag lookup failed).
void saved_game_allocate_new_slot(uint16_t *out_name)
{
    datum_index tag_index;
    UnicodeStringList *string_list;
    UnicodeStringListString *string_entry;
    uint16_t *format_string;
    uint32_t string_size;
    int32_t number;
    int32_t next_number;
    uint32_t create_result;
    char scratch_path[0x100];

    out_name[0] = 0;
    tag_index = tag_lookup('ustr', (char *)"ui\\saved_game_file_strings");
    if (tag_index != -1) {
        memset(scratch_path, 0, sizeof(scratch_path));
        number = 0;
        do {
            string_list = (UnicodeStringList *)tag_instances[tag_index & 0xffff].data;
            format_string = missing_string_text;
            if (2 < (int32_t)string_list->strings.count) {
                // string 2 of the list: strings.pointer + 2 * 0x14 (mov eax,[eax+4] / add eax,0x28)
                string_entry = (UnicodeStringListString *)string_list->strings.pointer + 2;
                string_size = string_entry->string.size;
                if (0 < (int32_t)string_size) {
                    format_string = (uint16_t *)string_entry->string.pointer;
                    *(uint16_t *)((uint8_t *)format_string + (string_size >> 1) * 2 - 2) = 0;
                }
            }
            next_number = number + 1;
            string_format_wide_va_bounded(0x7f, out_name, format_string, next_number); // EDX = 0x7f
            out_name[0x7f] = 0;
            create_result = XCreateSaveGame(out_name, savegames_directory, 3, scratch_path, 0x100);
            if (create_result != 0) {
                break;
            }
            number = next_number;
        } while (number < 999);
        if (number == 999) {
            out_name[0] = 0;
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x53ca80):

void saved_game_allocate_new_slot(void)

{
  int *piVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  undefined2 *unaff_EBX;
  undefined4 *puVar7;
  undefined1 local_108;
  undefined4 local_107;

  *unaff_EBX = 0;
  uVar4 = tag_lookup("ui\\saved_game_file_strings");
  if (uVar4 != 0xffffffff) {
    local_108 = 0;
    puVar7 = &local_107;
    for (iVar6 = 0x3f; iVar6 != 0; iVar6 = iVar6 + -1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    *(undefined2 *)puVar7 = 0;
    *(undefined1 *)((int)puVar7 + 2) = 0;
    iVar6 = 0;
    do {
      piVar1 = *(int **)((uVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (2 < *piVar1) {
        iVar2 = piVar1[1];
        uVar3 = *(uint *)(iVar2 + 0x28);
        if (0 < (int)uVar3) {
          *(undefined2 *)(*(int *)(iVar2 + 0x34) + -2 + (uVar3 & 0xfffffffe)) = 0;
        }
      }
      iVar2 = iVar6 + 1;
      string_format_wide_va_bounded();
      unaff_EBX[0x7f] = 0;
      iVar5 = XCreateSaveGame(&DAT_00721549,3,&local_108,0x100);
    } while ((iVar5 == 0) && (iVar6 = iVar2, iVar2 < 999));
    if (iVar6 == 999) {
      *unaff_EBX = 0;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
