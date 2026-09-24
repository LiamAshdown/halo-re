// game_state_allocate_buffer  (Ghidra: game_state_allocate_buffer, already named)
// address 0x5385f0, size 158 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: out/phase4/saved_games_functions.md; strings "savegame.bin" and "%s\\%s"; matches
// out/phase4/saved_games_types_notes.md's "0x440000 arena" note (game_state_startup pushes
// 0x400000 and passes 0x40000 in ECX here) and the global list (game_state_snapshot_source ==
// map_memory, game_state_size, game_state_write_buffer, persistent-storage/core path strings,
// game_state_write_event, save-thread creation). objdump confirms 0x00670abc = "core".
// register convention: extra_size in ECX (in_ECX); cpu_size as the recognized stack parameter
// (param_1); returns map_memory (game_state_base) in EAX.
// reconciled: R10 profile_directory is char[0x105] (k_profile_directory_storage_size; shell zeroes 0x41 dwords + 1 byte at 0x540ef9)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern void *map_memory; // 0x006ac548
extern uint8_t *game_state_snapshot_source; // 0x006e2dec
extern uint8_t game_state_write_buffer_allocated; // 0x006e2de8
extern uint32_t game_state_size; // 0x006e2df0
extern uint8_t *game_state_write_buffer; // 0x006e2de4
extern char profile_directory[0x105]; // 0x006ac900
extern char game_state_persistent_storage_path[0x100]; // 0x006e2dfc
extern char game_state_core_directory[0x100]; // 0x006e2efc
extern uint8_t game_state_write_in_progress; // 0x006e3000
extern void *game_state_write_event; // 0x006e2ffc

extern void *GlobalAlloc(uint32_t flags, uint32_t bytes);
extern void *CreateEventA(void *security_attributes, int32_t manual_reset, int32_t initial_state, const char *name);
extern int __snprintf(char *dest, uint32_t count, const char *format, ...);
extern uint32_t __beginthread(void (*start_address)(void *), uint32_t stack_size, void *arg_list);
extern void game_state_save_thread_proc(void); // 0x538980

// blam-cc: cpu_size as the recognized stack parameter, extra_size in ECX
// Allocates the game-state working buffer (cpu_size + extra_size bytes), builds the
// savegame.bin and core-directory path strings under the active profile directory, and
// starts the background save-writer thread. Returns map_memory, the source the buffer is
// snapshotted from.
void *game_state_allocate_buffer(int32_t cpu_size, int32_t extra_size)
{
    void *base;

    base = map_memory;
    game_state_snapshot_source = map_memory;
    game_state_size = cpu_size + extra_size;
    game_state_write_buffer_allocated = 1;
    game_state_write_buffer = (uint8_t *)GlobalAlloc(0, game_state_size);
    __snprintf(game_state_persistent_storage_path, 0xff, "%s\\%s", profile_directory, "savegame.bin");
    __snprintf(game_state_core_directory, 0xff, "%s\\%s", profile_directory, "core");
    game_state_write_in_progress = 0;
    game_state_write_event = CreateEventA(0, 0, 0, 0);
    __beginthread((void (*)(void *))game_state_save_thread_proc, 0x1000, 0);
    return base;
}

#if 0
Original Ghidra decompilation (0x5385f0):

undefined4 game_state_allocate_buffer(int param_1)

{
  undefined4 uVar1;
  int in_ECX;

  uVar1 = DAT_006ac548;
  DAT_006e2dec = DAT_006ac548;
  DAT_006e2df0 = param_1 + in_ECX;
  DAT_006e2de8 = 1;
  DAT_006e2de4 = GlobalAlloc(0,DAT_006e2df0);
  __snprintf(&DAT_006e2dfc,0xff,"%s\\%s",&DAT_006ac900,"savegame.bin");
  __snprintf(&DAT_006e2efc,0xff,"%s\\%s",&DAT_006ac900,&DAT_00670abc);
  DAT_006e3000 = 0;
  DAT_006e2ffc = CreateEventA((LPSECURITY_ATTRIBUTES)0x0,0,0,(LPCSTR)0x0);
  __beginthread(game_state_save_thread_proc,0x1000,(void *)0x0);
  return uVar1;
}
#endif
