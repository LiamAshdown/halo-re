// saved_game_create_custom_variant  (Ghidra: FUN_0053bb50, renamed)
// address 0x53bb50, size 288 bytes
// name confidence: 0.45   rewrite confidence: 0.5
// evidence: out/phase4/saved_games_functions.md summary "Creates a new custom playlist/
// game-variant profile file seeded from the classic-slayer default options and a caller-supplied
// name, rolling back the write failure." saved_game_create_slot is called with a hardcoded type
// of 1 (_saved_game_type_game_variant), not param_1, and param_1 is otherwise never read anywhere
// in the function; kept as an unused parameter rather than dropped, since the calling convention
// is otherwise unknown (UNSURE).
// Several call arguments are register-dropped by Ghidra: saved_game_open_file_by_handle's own
// handle argument (EAX, established throughout this module) must be the handle
// saved_game_create_slot just returned, since nothing else plausible is live; likewise both
// saved_game_delete_by_handle() rollback calls (EDI, established in saved_game_delete_by_handle.c)
// and file_reference_seek()/file_reference_write() (established ESI/EDX-ECX/EAX-ECX-ESI
// conventions) all operate on that same handle/ref pair. `_Dest`, used unassigned by Ghidra for
// the wcsncpy destination, is read as the same buffer the classic-slayer defaults were just
// copied into (local_2008, i.e. &file.variant), matching game_variant::name at offset 0 and the
// terminator write (local_1fda) immediately after it at name[23].
// register convention: __cdecl, plain stack arguments (param_1 [unused], name).
// reconciled: R37 game_variant.unknown_94 -> uint16 variant_flags (bit 0 built-in, high byte default index)

#include "crt.h"
#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern game_variant *game_engine_variant_defaults_classic_slayer(game_variant *out); // 0x463c40
extern void game_variant_sanitize_options(game_variant *variant); // 0x466730; blam-cc: ECX -> variant
extern void crc32_update(uint32_t *checksum, const void *data, uint32_t size); // 0x4d02d0
extern uint32_t saved_game_create_slot(uint16_t type, uint16_t *name); // 0x53c660, this module
extern uint8_t saved_game_open_file_by_handle(int32_t handle, file_reference_record *out_ref); // 0x53c9f0, this module
extern uint8_t saved_game_delete_by_handle(int32_t handle); // 0x53c960, this module
extern uint8_t file_reference_seek(int32_t offset, file_reference_record *ref); // 0x5558f0, this module
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // 0x555a90, this module
extern uint8_t file_reference_close(file_reference_record *ref); // 0x555890, this module

// blam-cc: __cdecl, plain stack arguments (unused, name)
// Creates a new game-variant slot named name, opens its file, fills it with the classic-slayer
// default options (variant_flags's built-in bit cleared, since this is a user-created variant),
// sanitizes it, stamps name over its name field, crcs and writes it, and rolls back (deletes the
// slot) if the seek or write fails. Returns the new slot's packed handle, or 0xffffffff on any
// failure.
uint32_t saved_game_create_custom_variant(uint32_t unused, uint16_t *name)
{
    uint32_t handle;
    file_reference_record ref;
    game_variant_file file;
    game_variant *defaults_ptr;
    uint8_t opened;
    uint8_t seeked;
    uint8_t written;

    (void)unused; // UNSURE: unused, see header comment

    handle = saved_game_create_slot(1, name);
    if (handle == 0xffffffff) {
        return 0xffffffff;
    }

    opened = saved_game_open_file_by_handle((int32_t)handle, &ref);
    if (opened != 0) {
        memset(&file, 0, sizeof(file));
        defaults_ptr = game_engine_variant_defaults_classic_slayer((game_variant *)&file.variant);
        memcpy(&file.variant, defaults_ptr, sizeof(file.variant));
        file.variant.variant_flags = (int16_t)((uint16_t)file.variant.variant_flags & 0xfffe);
        game_variant_sanitize_options(&file.variant);
        wcsncpy((wchar_t *)file.variant.name, (const wchar_t *)name, 0x17);
        file.variant.name[0x17] = 0;
        file.checksum = 0xffffffff;
        crc32_update(&file.checksum, &file.variant, sizeof(file.variant));

        seeked = file_reference_seek(0, &ref);
        if (seeked == 0 || (written = file_reference_write(&ref, &file, sizeof(file)), written == 0)) {
            saved_game_delete_by_handle((int32_t)handle);
            handle = 0xffffffff;
        }
        file_reference_close(&ref);
        return handle;
    }
    saved_game_delete_by_handle((int32_t)handle);
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x53bb50):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Type propagation algorithm not settling */

uint FUN_0053bb50(undefined4 param_1,wchar_t *param_2)

{
  char cVar1;
  uint uVar2;
  undefined4 *extraout_EAX;
  int iVar3;
  wchar_t *_Dest;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined1 local_21b0 [272];
  undefined1 local_20a0 [152];
  undefined1 local_2008 [3];
  undefined1 auStack_2005 [43];
  undefined2 local_1fda;
  byte local_1f74;
  undefined4 local_1f70 [2009];
  undefined4 uStack_c;

  uStack_c = 0x53bb60;
  uVar2 = saved_game_create_slot(1,param_2);
  if (uVar2 == 0xffffffff) {
    return 0xffffffff;
  }
  cVar1 = saved_game_open_file_by_handle(local_21b0);
  if (cVar1 != '\0') {
    local_2008[0] = 0;
    puVar4 = (undefined4 *)((int)local_2008 + 1);
    for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    *(undefined2 *)puVar4 = 0;
    *(undefined1 *)((int)puVar4 + 2) = 0;
    game_engine_variant_defaults_classic_slayer(local_20a0);
    puVar4 = extraout_EAX;
    puVar5 = (undefined4 *)local_2008;
    for (iVar3 = 0x26; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    local_1f74 = local_1f74 & 0xfe;
    game_variant_sanitize_options();
    _wcsncpy(_Dest,param_2,0x17);
    local_1fda = 0;
    local_1f70[0] = 0xffffffff;
    crc32_update(local_1f70,local_2008,0x98);
    cVar1 = file_reference_seek();
    if ((cVar1 == '\0') || (cVar1 = file_reference_write(), cVar1 == '\0')) {
      saved_game_delete_by_handle();
      uVar2 = 0xffffffff;
    }
    file_reference_close();
    return uVar2;
  }
  saved_game_delete_by_handle();
  return 0xffffffff;
}
#endif
