#include "halo/shell/hardware.hpp"
#include "halo/shell/layout.hpp"
#include "halo/shell/diagnostics.hpp"
#include "halo/shell/system.hpp"
#include <dsound.h>
#include "halo/core/link.hpp"
#include "halo/shell/vars.hpp"
#include "halo/shell/api.hpp"

static auto &physical_memory = halo::link::ref<uint32_t>(halo::shell::vars().physical_memory);
static auto &cpu_speed = halo::link::ref<uint32_t>(halo::shell::vars().cpu_speed);
static auto &display_adapter_count = halo::link::ref<uint32_t>(halo::shell::vars().display_adapter_count);
static auto &display_adapters = halo::link::ref<shell_display_adapter [k_shell_maximum_display_adapters]>(halo::shell::vars().display_adapters);
static auto &sound_devices = halo::link::ref<shell_sound_device [k_shell_maximum_sound_devices]>(halo::shell::vars().sound_devices);
static auto &sound_device_count = halo::link::ref<uint32_t>(halo::shell::vars().sound_device_count);
static auto &selected_sound_device = halo::link::ref<int32_t>(halo::shell::vars().selected_sound_device);
static auto &iid_direct_draw7 = halo::link::ref<const uint32_t [4]>(halo::shell::vars().iid_direct_draw7);
static auto &dsdevid_default_playback = halo::link::ref<const uint32_t [4]>(halo::shell::vars().dsdevid_default_playback);
static auto &clsid_dxdiag_provider = halo::link::ref<const uint32_t [4]>(halo::shell::vars().clsid_dxdiag_provider);
static auto &iid_dxdiag_provider = halo::link::ref<const uint32_t [4]>(halo::shell::vars().iid_dxdiag_provider);
static auto &dxdiag_sound_device_child_name = halo::link::ref<const uint16_t []>(halo::shell::vars().dxdiag_sound_device_child_name);

#define SOUND_DEVICE_HARDWARE_ID_NFORCE "pci\ven_10de&dev_01b0&subsys_37301462&rev_c2\3&13c0b0c5&0&28"

namespace {

constexpr int k_text_buffer_length = 256;

}  // namespace

namespace halo::shell {

namespace {

void read_time_stamp_counter(large_integer *result)
{
    uint32_t tsc_low, tsc_high;
    __asm {
        rdtsc
        mov tsc_low, eax
        mov tsc_high, edx
    }
    result->parts.low_part = tsc_low;
    result->parts.high_part = (int32_t)tsc_high;
}

/**
 * Rounds a GetAvailableVidMem total up to the sizes graphics cards ship with.
 */
uint32_t round_video_memory(uint32_t bytes)
{
    if (bytes <= k_video_memory_16mb) {
        return align_up(bytes, k_video_memory_granule_small);
    }
    if (bytes <= k_video_memory_64mb) {
        return align_up(bytes, k_video_memory_granule_medium);
    }
    if (bytes > k_video_memory_2gb) {
        return k_video_memory_2gb;
    }
    return align_up(bytes, k_video_memory_granule_large);
}

constexpr Win32HardwareProbe k_win32_hardware_probe{};

}

/**
 * The hardware probe of the running platform.
 */
const HardwareProbe &HardwareProbe::current()
{
    return k_win32_hardware_probe;
}

/**
 * Measures physical memory, CPU clock, display adapter video memory and the sound devices.
 *
 * @address 0x57d880
 */
void Win32HardwareProbe::detect() const
{
    measure_physical_memory();
    measure_cpu_speed();
    detect_display_adapters();
    detect_sound_devices();
}

/**
 * Physical memory in MB, rounded up to 16 MB and clamped to 1 GB.
 */
void Win32HardwareProbe::measure_physical_memory()
{
    win32_memory_status memory_status;

    GlobalMemoryStatus((LPMEMORYSTATUS)&memory_status);
    if (memory_status.total_physical > k_shell_physical_memory_clamp) {
        memory_status.total_physical = k_shell_physical_memory_clamp;
    }
    physical_memory = ((memory_status.total_physical + k_physical_memory_round_bytes) >> 20) & k_physical_memory_megabyte_mask;
}

/**
 * CPU clock in MHz: rdtsc ticks over a quarter second of the performance counter, snapped to the usual
 * 33 / 50 / 66 / 100 steps.
 */
void Win32HardwareProbe::measure_cpu_speed()
{
    void *thread;
    void *process;
    int32_t thread_priority;
    uint32_t priority_class;
    large_integer frequency;
    large_integer target;
    large_integer counter;
    large_integer tsc_start;
    large_integer tsc_end;
    int64_t cycles;
    uint32_t speed;
    uint32_t remainder;

    thread = GetCurrentThread();
    process = GetCurrentProcess();
    thread_priority = GetThreadPriority(thread);
    priority_class = GetPriorityClass(process);
    SetPriorityClass(process, k_realtime_priority_class);
    SetThreadPriority(thread, 15);
    Sleep(100);
    QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
    QueryPerformanceCounter((LARGE_INTEGER *)&target);
    target.quad_part = target.quad_part + frequency.quad_part / 4;
    read_time_stamp_counter(&tsc_start);
    do {
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    } while (target.quad_part > counter.quad_part);
    read_time_stamp_counter(&tsc_end);
    cycles = tsc_end.quad_part - tsc_start.quad_part;
    SetThreadPriority(thread, thread_priority);
    SetPriorityClass(process, priority_class);

    speed = (uint32_t)(cycles / 250000);
    cpu_speed = speed;
    if (speed > 1000) {
        speed = ((speed + 50) / 100) * 100;
        cpu_speed = speed;
    }
    remainder = speed % 100;
    if (speed < 200) {
        if (remainder > 0x5f) {
            speed = speed + (100 - remainder);
            cpu_speed = speed;
        }
        if (remainder > 0x3d && remainder < 0x47) {
            speed = speed + (0x42 - remainder);
            cpu_speed = speed;
        }
        if (remainder > 0x2d && remainder < 0x37) {
            speed = speed + (0x32 - remainder);
            cpu_speed = speed;
        }
        if (remainder > 0x1c && remainder < 0x26) {
            cpu_speed = speed + (0x21 - remainder);
        }
    } else if (remainder > 0x54) {
        cpu_speed = speed + (100 - remainder);
    } else if (remainder > 0x3a) {
        cpu_speed = speed + (0x42 - remainder);
    } else if (remainder > 0x2a) {
        cpu_speed = speed + (0x32 - remainder);
    } else if (remainder > 0x10) {
        cpu_speed = speed + (0x21 - remainder);
    } else if (remainder < 0x10) {
        cpu_speed = speed - remainder;
    }
}

/**
 * DirectDraw callback that records one display adapter (guid for entries past the primary, driver
 * name when shorter than 0x1f characters). Returns 0 to stop once ten adapters are stored.
 *
 * @address 0x57d370
 */
int32_t __stdcall Win32HardwareProbe::enumerate_display_adapter(void *guid, char *description, char *driver_name,
                                                                void *context, void *monitor)
{
    uint32_t index = display_adapter_count;
    shell_display_adapter *adapter = &display_adapters[index];
    const char *s;
    int32_t i;

    if (index != 0) {
        const uint32_t *source = (const uint32_t *)guid;
        adapter->guid[0] = source[0];
        adapter->guid[1] = source[1];
        adapter->guid[2] = source[2];
        adapter->guid[3] = source[3];
    }
    for (s = driver_name; *s; s++) {
    }
    if ((uint32_t)(s - driver_name) < 0x1f) {
        i = 0;
        do {
            adapter->driver_name[i] = driver_name[i];
        } while (driver_name[i++] != 0);
    } else {
        adapter->driver_name[0] = 0;
    }
    display_adapter_count = index + 1;
    return index + 1 != k_shell_maximum_display_adapters;
}

/**
 * Enumerates the DirectDraw adapters and records the smallest non-zero available video memory total
 * over four capability masks, rounded to card sizes, for each.
 */
void Win32HardwareProbe::detect_display_adapters()
{
    DynamicLibrary ddraw("ddraw.dll");
    direct_draw_create_ex_fn direct_draw_create_ex;
    direct_draw_enumerate_ex_fn direct_draw_enumerate_ex;
    uint32_t adapter_index;
    direct_draw7 *direct_draw;
    ddscaps2 caps;
    uint32_t total_memory;
    uint32_t free_memory;
    uint32_t smallest;

    direct_draw_create_ex = (direct_draw_create_ex_fn)ddraw.symbol("DirectDrawCreateEx");
    if (direct_draw_create_ex == 0) {
        FatalError::show(k_string_display_unsupported, k_help_file_directx, 1);
    }
    direct_draw_enumerate_ex = (direct_draw_enumerate_ex_fn)ddraw.symbol("DirectDrawEnumerateExA");
    if (direct_draw_enumerate_ex == 0) {
        FatalError::show(k_string_display_unsupported, k_help_file_directx, 1);
    }
    direct_draw_enumerate_ex((void *)enumerate_display_adapter, 0, 1);

    for (adapter_index = 0; adapter_index < display_adapter_count; adapter_index++) {
        if (direct_draw_create_ex(adapter_index != 0 ? display_adapters[adapter_index].guid : 0, &direct_draw,
                                  iid_direct_draw7, 0) < 0) {
            FatalError::show(k_string_display_unsupported, k_help_file_directx, 1);
        }
        direct_draw->vtable->set_cooperative_level(direct_draw, 0, 8);

        caps.caps2 = 0;
        caps.caps3 = 0;
        caps.caps4 = 0;
        total_memory = 0;
        caps.caps = k_ddscaps_primary_video_memory;
        smallest = k_dword_none;
        direct_draw->vtable->get_available_vid_mem(direct_draw, &caps, &total_memory, &free_memory);
        if (total_memory > 0 && total_memory < k_dword_none) {
            smallest = total_memory;
        }
        caps.caps = k_ddscaps_local_texture_3d_device;
        direct_draw->vtable->get_available_vid_mem(direct_draw, &caps, &total_memory, &free_memory);
        if (total_memory > 0 && total_memory < smallest) {
            smallest = total_memory;
        }
        caps.caps = k_ddscaps_local_texture;
        direct_draw->vtable->get_available_vid_mem(direct_draw, &caps, &total_memory, &free_memory);
        if (total_memory > 0 && total_memory < smallest) {
            smallest = total_memory;
        }
        caps.caps = k_ddscaps_local_offscreen_plain;
        direct_draw->vtable->get_available_vid_mem(direct_draw, &caps, &total_memory, &free_memory);
        if (total_memory > 0 && total_memory < smallest) {
            smallest = total_memory;
        }

        display_adapters[adapter_index].video_memory = round_video_memory(smallest);
        direct_draw->vtable->release(direct_draw);
    }
}

/**
 * Reads description, GUID, hardware ids and driver version of one DxDiag sound device into record and
 * selects it when its GUID is the default playback device.
 */
void Win32HardwareProbe::read_sound_device(dxdiag_container *device, shell_sound_device *record,
                                           const uint32_t *default_guid, uint32_t device_index, win32_variant *variant)
{
    char text[k_text_buffer_length];
    char *match;
    int32_t version_a, version_b, version_c, version_d;

    device->vtable->get_prop(device, (const uint16_t *)L"szGuidDeviceID", variant);
    if (variant->type == 8) {
        WideCharToMultiByte(0, 0, (LPCWCH)variant->value, -1, text, sizeof(text), 0, 0);
        _strlwr(text);
        record->guid[0] = (uint32_t)HexParser::to_uint(text + 1);
        ((uint16_t *)record->guid)[2] = (uint16_t)HexParser::to_uint(text + 10);
        ((uint16_t *)record->guid)[3] = (uint16_t)HexParser::to_uint(text + 15);
        HexParser::to_bytes((uint8_t *)record->guid + 8, text + 20);
        HexParser::to_bytes((uint8_t *)record->guid + 10, text + 25);
        if (memcmp(record->guid, default_guid, 0x10) == 0) {
            selected_sound_device = (int32_t)device_index;
        }
        VariantClear((VARIANTARG *)variant);
    }

    device->vtable->get_prop(device, (const uint16_t *)L"szDescription", variant);
    if (variant->type == 8) {
        WideCharToMultiByte(0, 0, (LPCWCH)variant->value, -1, text, sizeof(text), 0, 0);
        strncpy(record->description, text, 0x1f);
        record->description[0x1f] = 0;
        VariantClear((VARIANTARG *)variant);
    }

    device->vtable->get_prop(device, (const uint16_t *)L"szHardwareID", variant);
    if (variant->type == 8) {
        WideCharToMultiByte(0, 0, (LPCWCH)variant->value, -1, text, sizeof(text), 0, 0);
        _strlwr(text);
        if (memcmp(text, "*nforce", 8) == 0) {
            memcpy(text, SOUND_DEVICE_HARDWARE_ID_NFORCE, 0x3b);
        }
        match = strstr(text, "ven_");
        record->vendor_id = match != 0 ? (uint32_t)HexParser::to_uint(match + 4) : 0;
        match = strstr(text, "dev_");
        record->device_id = match != 0 ? (uint32_t)HexParser::to_uint(match + 4) : 0;
        match = strstr(text, "subsys_");
        record->subsystem_id = match != 0 ? (uint32_t)HexParser::to_uint(match + 7) : 0;
        match = strstr(text, "rev_");
        record->revision = match != 0 ? (uint32_t)HexParser::to_uint(match + 7) : 0;
        VariantClear((VARIANTARG *)variant);
    }

    device->vtable->get_prop(device, (const uint16_t *)L"szDriverVersion", variant);
    if (variant->type == 8) {
        WideCharToMultiByte(0, 0, (LPCWCH)variant->value, -1, text, sizeof(text), 0, 0);
        version_d = 0;
        sscanf(text, "%d.%d.%d.%d", &version_a, &version_b, &version_c, &version_d);
        record->driver_version.parts.high_part = (version_a << 16) + version_b;
        record->driver_version.parts.low_part = (uint32_t)((version_c << 16) + version_d);
        VariantClear((VARIANTARG *)variant);
    }
}

/**
 * Lists the DxDiag sound devices (at most nine) into sound_devices and selects the default playback
 * device.
 */
void Win32HardwareProbe::detect_sound_devices()
{
    dxdiag_provider *provider;
    dxdiag_container *root;
    dxdiag_container *devices;
    dxdiag_container *device;
    dxdiag_init_params params;
    win32_variant variant;
    uint32_t default_guid[4];
    uint32_t device_index;
    const uint16_t *child_name;
    shell_sound_device *record;

    memset(sound_devices, 0, sizeof(sound_devices));
    sound_device_count = 0;
    selected_sound_device = 0;
    GetDeviceID((LPCGUID)dsdevid_default_playback, (LPGUID)default_guid);
    CoInitialize(0);

    provider = 0;
    root = 0;
    if (CoCreateInstance((REFCLSID)clsid_dxdiag_provider, 0, 1, (REFIID)iid_dxdiag_provider, (void **)&provider) < 0) {
        CoUninitialize();
        return;
    }

    params.size = sizeof(params);
    params.header_version = 0x6f;
    params.allow_whql_checks = 0;
    params.reserved = 0;
    provider->vtable->initialize(provider, &params);
    provider->vtable->get_root_container(provider, &root);

    if (root != 0) {
        devices = 0;
        device = 0;
        VariantInit((VARIANTARG *)&variant);
        root->vtable->get_child_container(root, (const uint16_t *)L"DxDiag_DirectSound.DxDiag_SoundDevices",
                                          (void **)&devices);
        if (devices != 0) {
            devices->vtable->get_number_of_child_containers(devices, &sound_device_count);
            if (sound_device_count > k_shell_sound_device_count_clamp) {
                sound_device_count = k_shell_sound_device_count_clamp;
            }

            child_name = dxdiag_sound_device_child_name;
            record = sound_devices;
            for (device_index = 0; device_index < sound_device_count; device_index++) {
                devices->vtable->get_child_container(devices, child_name, (void **)&device);
                if (device != 0) {
                    read_sound_device(device, record, default_guid, device_index, &variant);
                    device->vtable->release(device);
                }
                child_name = child_name + 1;
                record = record + 1;
            }
            devices->vtable->release(devices);
        }
        root->vtable->release(root);
    }
    provider->vtable->release(provider);
    CoUninitialize();
}

}
