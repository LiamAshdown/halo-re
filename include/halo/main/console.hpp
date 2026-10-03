#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::main {

/**
 * Developer console: input, autocomplete, command dispatch and output.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct Console {
    static void chimera__exec_init(void);
    static void autocomplete_command(void);
    static uint32_t command_context_mask(uint32_t context_flags);
    static void deactivate(void);
    static uint8_t exec_file_run(const char *file_name);
    static void initialize(void);
    static uint32_t paste_clipboard_text(void);
    static char process_command(char *command_line, uint32_t context_flags);
    static uint8_t process_key_events(void);
    static void process_rcon_command(int32_t rcon_handle, char *command_line);
    static void toggle(void);
};

}
