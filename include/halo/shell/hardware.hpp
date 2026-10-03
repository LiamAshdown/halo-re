#pragma once

#include "halo/shell/platform.hpp"

namespace halo::shell {

/**
 * Measures the machine the game runs on and fills the hardware globals: physical memory, CPU speed,
 * video memory per display adapter and the sound device list.
 */
class HardwareProbe {
public:
    virtual void detect() const = 0;

    /**
     * The hardware probe of the running platform.
     */
    static const HardwareProbe &current();
};

/**
 * Windows implementation of the hardware probe: GlobalMemoryStatus, a timed rdtsc loop, DirectDraw for
 * the display adapters and DxDiag for the sound devices.
 */
class Win32HardwareProbe final : public HardwareProbe {
public:
    void detect() const override;

    static int32_t __stdcall enumerate_display_adapter(void *guid, char *description, char *driver_name, void *context,
                                                       void *monitor);

private:
    static void measure_physical_memory();
    static void measure_cpu_speed();
    static void detect_display_adapters();
    static void detect_sound_devices();
    static void read_sound_device(dxdiag_container *device, shell_sound_device *record, const uint32_t *default_guid,
                                  uint32_t device_index, win32_variant *variant);
};

}
