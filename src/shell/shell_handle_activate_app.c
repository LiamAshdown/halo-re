// shell_handle_activate_app  (Ghidra: FUN_005410d0)
// address 0x5410d0, size 267 bytes
// name confidence: 0.5 (shell_window_procedure's only call, for WM_ACTIVATEAPP with
//   BL = (wparam == 0))   rewrite confidence: 0.85
// evidence: objdump -d 0x5410d0..0x5411da. Records the new state in
//   shell_application_inactive (0x00721e8c), pauses sound (sound_pause 0x548170, or the inline
//   set_paused(1) when there is no full-screen device) and unacquires DirectInput on
//   deactivation, acquires it on activation, resets input, minimizes (SW_MINIMIZE 6) or
//   restores (SW_RESTORE 9) the full-screen window, closes chat on deactivation, and resumes
//   sound on activation -- by tail-jumping to sound_resume (0x5481a0) when the full-screen
//   device exists, or with the inline copy of sound_resume's second half otherwise.
// register convention: the flag in BL, no stack arguments.
//   // blam-cc: BL -> inactive

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"
#include "fn_shell.h"
#include "fn_input.h"

extern uint8_t shell_application_inactive;   // 0x00721e8c
extern uint8_t shell_window_proc_bypass;     // 0x00721e8d
extern uint8_t rasterizer_fullscreen;        // 0x0071d16c
extern void *rasterizer_device;              // 0x0071d174
extern void *shell_window;                   // 0x007461c4
extern uint8_t sound_paused;                 // 0x00725202
extern sound_driver *current_sound_driver;   // 0x00725208
extern int32_t sound_time;                   // 0x0072520c


extern void chat_close(void);                             // 0x4aa900
extern int32_t time_query_performance_counter_ms(void);   // 0x449210

// blam-cc: BL -> inactive
// Reacts to the application losing or regaining focus.
void shell_handle_activate_app(uint8_t inactive)
{
    uint8_t fullscreen_device;

    if (shell_application_inactive == inactive) {
        return;
    }
    shell_application_inactive = inactive;

    if (inactive == 0) {
        input_directinput_acquire_devices();
    } else {
        if (shell_window_proc_bypass == 0) {
            if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
                sound_pause();
            } else if (sound_paused != 1) {
                sound_paused = 1;
                if (current_sound_driver != 0) {
                    current_sound_driver->set_paused(1);
                }
            }
        }
        input_directinput_unacquire_devices();
    }
    input_reset_state_and_axis_configs();

    if (shell_window != 0) {
        fullscreen_device = rasterizer_fullscreen != 0 && rasterizer_device != 0;
        if (fullscreen_device) {
            ShowWindow(shell_window, inactive != 0 ? 6 : 9); // SW_MINIMIZE / SW_RESTORE
        } else if (inactive == 0) {
            ShowWindow(shell_window, 9); // SW_RESTORE
        }
    }
    if (inactive != 0) {
        chat_close();
        return;
    }

    if (shell_window_proc_bypass != 0) {
        return;
    }
    if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
        sound_resume(); // 0x54119b: jmp 0x5481a0
        return;
    }
    if (sound_paused != 0) {
        sound_paused = 0;
        if (current_sound_driver != 0) {
            current_sound_driver->set_paused(0);
        }
        sound_time = time_query_performance_counter_ms();
    }
}

#if 0
Original Ghidra decompilation (0x5410d0):

void FUN_005410d0(void)
{
  char unaff_BL;
  if (DAT_00721e8c == unaff_BL) return;
  DAT_00721e8c = unaff_BL;
  if (unaff_BL == '\0') {
    input_directinput_acquire_devices();
  }
  else {
    if (DAT_00721e8d == '\0') {
      if ((DAT_0071d16c != '\0') && (DAT_0071d174 != 0)) {
        sound_pause();
        input_directinput_unacquire_devices();
        goto LAB_0054113d;
      }
      if ((DAT_00725202 != '\x01') && (DAT_00725202 = '\x01', DAT_00725208 != 0)) {
        (**(code **)(DAT_00725208 + 0x28))(1);
      }
    }
    input_directinput_unacquire_devices();
  }
LAB_0054113d:
  input_reset_state_and_axis_configs();
  if (DAT_007461c4 == (HWND)0x0) {
LAB_00541173:
    if (unaff_BL == '\0') goto LAB_00541180;
  }
  else {
    if ((DAT_0071d16c != '\0') && (DAT_0071d174 != 0)) {
      ShowWindow(DAT_007461c4,(-(uint)(unaff_BL != '\0') & 0xfffffffd) + 9);
      goto LAB_00541173;
    }
    if (unaff_BL == '\0') {
      ShowWindow(DAT_007461c4,9);
      goto LAB_00541180;
    }
  }
  chat_close();
  if (unaff_BL != '\0') return;
LAB_00541180:
  if (DAT_00721e8d == '\0') {
    if ((DAT_0071d16c != '\0') && (DAT_0071d174 != 0)) {
      FUN_005481a0();
      return;
    }
    if (DAT_00725202 != '\0') {
      DAT_00725202 = '\0';
      if (DAT_00725208 != 0) (**(code **)(DAT_00725208 + 0x28))(0);
      DAT_0072520c = time_query_performance_counter_ms();
    }
  }
  return;
}
#endif
