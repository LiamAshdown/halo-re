// terminal_initialize  (Ghidra: terminal_initialize, already named)
// address 0x4963d0, size 80 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: out/phase4/interface_functions.md "Allocates the terminal-output data array and
// resets developer-console/terminal state globals."; types/interface.h's console_message
// (0x124 bytes, capacity 0x20) and the terminal/console globals list at the bottom of that
// header name every field written here.
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *terminal_messages;           // 0x006b2f00, "terminal output"
extern uint8_t terminal_initialized;             // 0x006b2efc
extern terminal_console *console_active;         // 0x006b2f0c
extern datum_index console_message_head;         // 0x006b2f04
extern datum_index console_message_tail;         // 0x006b2f08
extern int32_t console_caret_blink_time;         // 0x006b2f14
extern int32_t console_rcon_handle;              // 0x006b2f1c

extern data_array *data_new(int16_t element_size, char *name, int16_t maximum_count);
extern void data_delete_all(data_array *array);

// Allocates the terminal-output console_message data array (capacity 0x20) and resets the
// developer console/terminal globals to their empty state.
void terminal_initialize(void)
{
    terminal_messages = data_new(sizeof(console_message), (char *)"terminal output", 0x20);
    terminal_initialized = 1;
    terminal_messages->valid = 1;
    data_delete_all(terminal_messages);
    console_active = (terminal_console *)0;
    console_message_head = (datum_index)0xffffffff;
    console_message_tail = (datum_index)0xffffffff;
    console_caret_blink_time = 0;
    console_rcon_handle = (int32_t)0xffffffff;
}

#if 0
Original Ghidra decompilation (0x4963d0):

void __cdecl terminal_initialize(void)

{
  DAT_006b2f00 = data_new("terminal output",0x20);
  DAT_006b2efc = 1;
  *(undefined1 *)(DAT_006b2f00 + 0x24) = 1;
  data_delete_all();
  DAT_006b2f0c = 0;
  DAT_006b2f04 = 0xffffffff;
  DAT_006b2f08 = 0xffffffff;
  DAT_006b2f14 = 0;
  DAT_006b2f1c = 0xffffffff;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
