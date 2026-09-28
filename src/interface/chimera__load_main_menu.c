// chimera__load_main_menu  (Ghidra: chimera__load_main_menu, already named)
// address 0x4989f0, size 184 bytes
// name confidence: 0.7   rewrite confidence: 0.5
// evidence: matches the given name exactly; loads "ui\shell\main_menu\main_menu", tearing down a
// pending loading-thread state and resyncing input timing first when a "reload" flag
// (DAT_006926c8) is set, then surfaces any pending error and starts the title music.
// chimera__load_ui_widget takes 7 stack arguments (phase-4 review, objdump 0x497a70); called
// here with no parent and -1 for the three history arguments, as pushed at 0x498a47.
// register convention: no register-passed arguments.
// UNSURE: input_queue_sample_time_update, player_profile_check_storage_and_defaults (0x492210, 0x49c680) are declared void(void); their own
// decompiles were not read in this session.
// TYPES-GAP: DAT_006e35c0 (a command-line-ish string pointer compared against "xdemo") and
// DAT_00692af8 are not documented anywhere in types/interface.h.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint8_t main_menu_reload_pending; // 0x006926c8, UNSURE name
extern char *shell_command_line;    // 0x006e35c0, TYPES-GAP, UNSURE name
extern uint8_t ui_input_batch_mode;      // 0x00718fc5
extern uint8_t loading_thread_result;    // 0x00718fc0
extern loading_thread_record *loading_thread; // 0x00718fbc
extern int16_t network_join_error_code;        // 0x00718fa4
extern uint8_t main_menu_music_pending;  // 0x00718fc6
extern datum_index cached_saved_game_something; // 0x00692af8, TYPES-GAP, UNSURE name

extern void input_time_base_resync(void); // 0x48b470
extern void input_queue_sample_time_update(void); // 0x492210, UNSURE
extern void player_profile_check_storage_and_defaults(void); // 0x49c680, UNSURE
extern void widget_close_all(void); // 0x498650
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, 7 stack args (objdump)
extern void display_error(int16_t error_string_index, int32_t unknown, uint8_t modal, uint8_t is_error); // 0x498f20
extern void main_menu_play_title_music(void); // 0x4993e0
extern void virtual_keyboard_initialize(void); // 0x4a88f0

// Loads and opens the main menu UI widget: if a reload is pending, tears down any tracked
// loading-thread state and resyncs input timing (checking, but not acting on, whether the
// command line names the demo build); always resets the first-person weapon interface and
// closes every open widget first, then opens the main menu, surfaces any pending generic UI
// error, starts the title music if it is not already pending, and (re)initializes the virtual
// keyboard.
void chimera__load_main_menu(void)
{
    ui_input_batch_mode = 0;
    if (main_menu_reload_pending == 1) {
        if (shell_command_line != (char *)0) {
            _stricmp(shell_command_line, "xdemo");
        }
        ui_input_batch_mode = 1;
        loading_thread_result = 0;
        loading_thread = (loading_thread_record *)0;
        player_profile_check_storage_and_defaults();
        ui_input_batch_mode = 0;
        input_time_base_resync();
    }
    input_queue_sample_time_update();
    widget_close_all();
    chimera__load_ui_widget("ui\\shell\\main_menu\\main_menu", (datum_index)-1, (widget_instance *)0, 0xffff,
                            (datum_index)-1, (datum_index)-1, -1);
    if (network_join_error_code != -1) {
        display_error(network_join_error_code, -1, 1, 0);
        network_join_error_code = -1;
    }
    if (main_menu_music_pending == 0) {
        main_menu_play_title_music();
    }
    cached_saved_game_something = (datum_index)-1;
    virtual_keyboard_initialize();
    main_menu_reload_pending = 0;
}

#if 0
Original Ghidra decompilation (0x4989f0):

void chimera__load_main_menu(void)

{
  DAT_00718fc5 = 0;
  if (DAT_006926c8 == '\x01') {
    if (DAT_006e35c0 != (char *)0x0) {
      __stricmp(DAT_006e35c0,"xdemo");
    }
    DAT_00718fc5 = 1;
    DAT_00718fc0 = 0;
    DAT_00718fbc = 0;
    FUN_0049c680();
    DAT_00718fc5 = 0;
    input_time_base_resync();
  }
  FUN_00492210();
  widget_close_all();
  chimera__load_ui_widget
            ("ui\\shell\\main_menu\\main_menu",0xffffffff,0,0xffffffff,0xffffffff,0xffffffff,
             0xffffffff);
  if (DAT_00718fa4 != -1) {
    display_error(DAT_00718fa4,-1,'\x01','\0');
    DAT_00718fa4 = -1;
  }
  if (DAT_00718fc6 == '\0') {
    main_menu_play_title_music();
  }
  DAT_00692af8 = 0xffffffff;
  virtual_keyboard_initialize();
  DAT_006926c8 = 0;
  return;
}
#endif
