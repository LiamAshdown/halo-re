// sound_resume  (Ghidra: FUN_005481a0)
// address 0x5481a0, size 91 bytes
// name confidence: 0.45   rewrite confidence: 0.9
// evidence: the mirror of sound_pause (0x548170): clears sound_paused (0x00725202), calls
//   sound_driver.set_paused(0) through vtable+0x28 and restarts sound_time (0x0072520c) from
//   time_query_performance_counter_ms (0x449210). Before that it re-applies the EAX setting
//   through sound_driver_set_eax_enabled (0x548200, CL = force = 1). shell_window_procedure
//   calls it on activation and restore, and 0x54119b tail-jumps to it. Written from objdump -d
//   0x5481a0..0x5481fb.
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern uint8_t shell_window_proc_bypass;      // 0x00721e8d
extern uint8_t directsound_eax_enabled;       // 0x00746121
extern uint8_t directsound_eax_available;     // 0x00746120
extern uint8_t sound_paused;                  // 0x00725202
extern sound_driver *current_sound_driver;    // 0x00725208
extern int32_t sound_time;                    // 0x0072520c

extern void sound_driver_set_eax_enabled(uint8_t eax_enabled, uint8_t force); // 0x548200, blam-cc: stack, CL
extern int32_t time_query_performance_counter_ms(void); // 0x449210

// Re-applies the EAX setting and, if the sound engine was paused, unpauses the driver and
// restarts the sound clock. Does nothing while shell_window_proc_bypass is set.
void sound_resume(void)
{
    if (shell_window_proc_bypass != 0) {
        return;
    }

    sound_driver_set_eax_enabled((directsound_eax_enabled != 0 && directsound_eax_available != 0) ? 1 : 0, 1);

    if (sound_paused != 0) {
        sound_paused = 0;
        if (current_sound_driver != 0) {
            current_sound_driver->set_paused(0);
        }
        sound_time = time_query_performance_counter_ms();
    }
}

#if 0
Original Ghidra decompilation (0x5481a0):

void FUN_005481a0(void)
{
  undefined4 uVar1;
  if (DAT_00721e8d == '\0') {
    if ((DAT_00746121 == '\0') || (DAT_00746120 == '\0')) uVar1 = 0; else uVar1 = 1;
    sound_driver_set_eax_enabled(uVar1);
    if (DAT_00725202 != '\0') {
      DAT_00725202 = '\0';
      if (DAT_00725208 != 0) (**(code **)(DAT_00725208 + 0x28))(0);
      DAT_0072520c = time_query_performance_counter_ms();
    }
  }
  return;
}
#endif
