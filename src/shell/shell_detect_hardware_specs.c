// shell_detect_hardware_specs  (Ghidra: shell_detect_hardware_specs, already named)
// address 0x57d880, size 2190 bytes
// name confidence: 0.6  rewrite confidence: 0.7
// evidence: out/phase4/shell_functions.md ("Detects the host's memory size, CPU speed, and
//   installed DirectDraw/sound hardware") and types/shell.h (shell_sound_device,
//   shell_display_adapter, physical_memory / cpu_speed / video_memory). Called three times from
//   shell_winmain (0x541532, 0x541788, 0x54179b).
// Rewritten from objdump 0x57d880..0x57e10d, not from the decompile: Ghidra lost track of the
//   stack across the COM calls (every IDirectDraw7 / IDxDiag call is a stdcall through a vtable),
//   which scrambled the locals and produced garbage wide string "literals". The facts pinned from
//   the disassembly:
//   - CPU speed: QueryPerformanceFrequency / 4 is added to the current counter (__alldiv at
//     0x57d91e, divisor 4), rdtsc is read, the counter is polled until it passes that target
//     (signed 64 bit compare 0x57d94c..0x57d960), rdtsc is read again, and the tick difference is
//     divided by 250000 (0x3d090, __alldiv at 0x57d995): a quarter second sample in MHz. The
//     process runs REALTIME_PRIORITY_CLASS (0x100) and the thread TIME_CRITICAL (15) around it.
//   - Each display adapter: DirectDrawCreateEx(entry 0 ? NULL : &adapter.guid, &dd,
//     IID_IDirectDraw7 0x0064e2bc, NULL), SetCooperativeLevel(NULL, DDSCL_NORMAL 8) (+0x50) and
//     four GetAvailableVidMem (+0x5c) calls. The total is not reset between calls, so a failing
//     call re-offers the previous total (reproduced).
//   - DxDiag: CoCreateInstance(CLSID_DxDiagProvider 0x006728d4, NULL, CLSCTX_INPROC_SERVER,
//     IID_IDxDiagProvider 0x006728c4), Initialize (+0x0c), GetRootContainer (+0x10),
//     GetChildContainer(L"DxDiag_DirectSound.DxDiag_SoundDevices") (+0x14),
//     GetNumberOfChildContainers (+0x0c), then per device GetChildContainer and GetProp (+0x20)
//     of szGuidDeviceID, szDescription, szHardwareID and szDriverVersion.
// Original quirks reproduced as observed (each is visible in the disassembly):
//   - The child container name pointer starts at 0x00672578 (L"0") and advances by 2 bytes per
//     device (add edi,2 at 0x57e0c9). Device 0 is looked up as "0"; later devices get the empty
//     string (0x0067257a..) and, from device 4 on, the tail of the next literal. Only the first
//     sound device is ever read in practice.
//   - The child pointer is cleared once before the loop (0x57dd20), not per device.
//   - "*nforce" is compared as the whole lower-cased hardware id (8 bytes including the
//     terminator). The replacement id string at 0x006724dc was written with "\v" and "\3"
//     escapes in the source, so its bytes are "pci" 0x0b "en_10de..." 0x03 "&13c0b0c5&0&28"; the
//     "ven_" search then fails and vendor_id stays 0 for that device.
//   - The "rev_" id is parsed from 7 characters past the match (lea edx,[eax+7] at 0x57dff1),
//     like "subsys_", not 4.
//   - In szDriverVersion only the fourth sscanf field is pre-cleared (0x57e06f).
// register convention: no arguments. hex_string_to_uint 0x57d7f0 takes the string in EDX;
//   hex_string_to_bytes 0x57d830 takes the destination in ECX and the source in EDX.

#include "crt.h"
#include "win32.h"
#include <dsound.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif



extern int32_t hex_string_to_uint(char *string);                               // 0x57d7f0, string in EDX
extern void hex_string_to_bytes(uint8_t *dest, const char *source);            // 0x57d830, dest in ECX, source in EDX
extern int32_t shell_display_fatal_error_dialog(uint32_t resource_id, uint32_t help_text_or_id, int32_t is_fatal); // 0x57ea70
extern int32_t __stdcall shell_display_adapter_enumerate_callback(void *guid, char *description,
                                                                  char *driver_name, void *context,
                                                                  void *monitor); // 0x57d370, not a Ghidra function

extern uint32_t physical_memory;                                               // 0x00722ba8
extern uint32_t cpu_speed;                                                     // 0x00722bac
extern uint32_t display_adapter_count;                                         // 0x00722bb4
extern shell_display_adapter display_adapters[k_shell_maximum_display_adapters]; // 0x006efdc0
extern shell_sound_device sound_devices[k_shell_maximum_sound_devices];        // 0x006ef9b0
extern uint32_t sound_device_count;                                            // 0x006ef9a8
extern int32_t selected_sound_device;                                          // 0x006effd0

extern const uint32_t iid_direct_draw7[4];                                     // 0x0064e2bc IID_IDirectDraw7
extern const uint32_t dsdevid_default_playback[4];                             // 0x0064e23c DSDEVID_DefaultPlayback
extern const uint32_t clsid_dxdiag_provider[4];                                // 0x006728d4 CLSID_DxDiagProvider
extern const uint32_t iid_dxdiag_provider[4];                                  // 0x006728c4 IID_IDxDiagProvider
extern const uint16_t dxdiag_sound_device_child_name[];                        // 0x00672578 L"0", then zeros

#define SOUND_DEVICE_HARDWARE_ID_NFORCE "pci\ven_10de&dev_01b0&subsys_37301462&rev_c2\3&13c0b0c5&0&28" // 0x006724dc, 59 bytes; the \v and \3 escapes are the original bytes

static void shell_read_time_stamp_counter(large_integer *result)
{
#if defined(__GNUC__)
    uint32_t low, high;
    __asm__ volatile ("rdtsc" : "=a"(low), "=d"(high));
    result->parts.low_part = low;
    result->parts.high_part = (int32_t)high;
#else
    uint32_t tsc_low, tsc_high;
    __asm {
        rdtsc
        mov tsc_low, eax
        mov tsc_high, edx
    }
    result->parts.low_part = tsc_low;
    result->parts.high_part = (int32_t)tsc_high;
#endif
}

// Rounds a GetAvailableVidMem total up to the sizes graphics cards ship with.
static uint32_t shell_round_video_memory(uint32_t bytes)
{
    if (bytes <= 0x1000000) {
        return (bytes + 0x7fffff) & 0xff800000;          // up to 16 MB: next 8 MB
    }
    if (bytes <= 0x4000000) {
        return (bytes + 0x1ffffff) & 0xfe000000;         // up to 64 MB: next 32 MB
    }
    if (bytes > 0x80000000) {
        return 0x80000000;
    }
    return (bytes + 0x3ffffff) & 0xfc000000;             // next 64 MB
}

// Measures physical memory (MB, rounded up to 16 MB, at most 1 GB), the CPU clock (MHz, snapped
// to the usual 33 / 50 / 66 / 100 steps), the video memory of every DirectDraw adapter the
// enumeration callback recorded, and the DxDiag sound devices (description, device GUID,
// hardware ids, driver version), selecting the one whose GUID is the default playback device.
void shell_detect_hardware_specs(void)
{
    win32_memory_status memory_status;
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
    void *ddraw_module;
    direct_draw_create_ex_fn direct_draw_create_ex;
    direct_draw_enumerate_ex_fn direct_draw_enumerate_ex;
    uint32_t adapter_index;
    direct_draw7 *direct_draw;
    ddscaps2 caps;
    uint32_t total_memory;
    uint32_t free_memory;
    uint32_t smallest;
    uint32_t default_guid[4];
    dxdiag_provider *provider;
    dxdiag_container *root;
    dxdiag_container *devices;
    dxdiag_container *device;
    dxdiag_init_params params;
    win32_variant variant;
    uint32_t device_index;
    const uint16_t *child_name;
    shell_sound_device *record;
    char text[0x100];
    char *match;
    int32_t version_a, version_b, version_c, version_d;

    // physical memory
    GlobalMemoryStatus((LPMEMORYSTATUS)&memory_status);
    if (memory_status.total_physical > k_shell_physical_memory_clamp) {
        memory_status.total_physical = k_shell_physical_memory_clamp;
    }
    physical_memory = ((memory_status.total_physical + 0xffffff) >> 20) & 0xff0;

    // CPU clock: rdtsc ticks over a quarter second of the performance counter
    thread = GetCurrentThread();
    process = GetCurrentProcess();
    thread_priority = GetThreadPriority(thread);
    priority_class = GetPriorityClass(process);
    SetPriorityClass(process, 0x100);   // REALTIME_PRIORITY_CLASS
    SetThreadPriority(thread, 15);      // THREAD_PRIORITY_TIME_CRITICAL
    Sleep(100);
    QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
    QueryPerformanceCounter((LARGE_INTEGER *)&target);
    target.quad_part = target.quad_part + frequency.quad_part / 4;
    shell_read_time_stamp_counter(&tsc_start);
    do {
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    } while (target.quad_part > counter.quad_part);
    shell_read_time_stamp_counter(&tsc_end);
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
        // below 200 MHz every matching rule applies in turn (the sum is rewritten each time)
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

    // DirectDraw adapters and their video memory
    ddraw_module = LoadLibraryA("ddraw.dll");
    direct_draw_create_ex = (direct_draw_create_ex_fn)GetProcAddress((HMODULE)ddraw_module, "DirectDrawCreateEx");
    if (direct_draw_create_ex == 0) {
        shell_display_fatal_error_dialog(0x79, (uint32_t)((const char *)0x7a), 1);
    }
    direct_draw_enumerate_ex = (direct_draw_enumerate_ex_fn)GetProcAddress((HMODULE)ddraw_module, "DirectDrawEnumerateExA");
    if (direct_draw_enumerate_ex == 0) {
        shell_display_fatal_error_dialog(0x79, (uint32_t)((const char *)0x7a), 1);
    }
    direct_draw_enumerate_ex((void *)shell_display_adapter_enumerate_callback, 0,
                             1 /* DDENUM_ATTACHEDSECONDARYDEVICES */);

    for (adapter_index = 0; adapter_index < display_adapter_count; adapter_index++) {
        if (direct_draw_create_ex(adapter_index != 0 ? display_adapters[adapter_index].guid : 0, &direct_draw,
                                  iid_direct_draw7, 0) < 0) {
            shell_display_fatal_error_dialog(0x79, (uint32_t)((const char *)0x7a), 1);
        }
        direct_draw->vtable->set_cooperative_level(direct_draw, 0, 8 /* DDSCL_NORMAL */);

        caps.caps2 = 0;
        caps.caps3 = 0;
        caps.caps4 = 0;
        total_memory = 0;
        caps.caps = 0x4200;             // DDSCAPS_VIDEOMEMORY | DDSCAPS_PRIMARYSURFACE
        smallest = 0xffffffff;
        direct_draw->vtable->get_available_vid_mem(direct_draw, &caps, &total_memory, &free_memory);
        if (total_memory > 0 && total_memory < 0xffffffff) {
            smallest = total_memory;
        }
        caps.caps = 0x10007000;         // DDSCAPS_LOCALVIDMEM | VIDEOMEMORY | 3DDEVICE | TEXTURE
        direct_draw->vtable->get_available_vid_mem(direct_draw, &caps, &total_memory, &free_memory);
        if (total_memory > 0 && total_memory < smallest) {
            smallest = total_memory;
        }
        caps.caps = 0x10005000;         // DDSCAPS_LOCALVIDMEM | VIDEOMEMORY | TEXTURE
        direct_draw->vtable->get_available_vid_mem(direct_draw, &caps, &total_memory, &free_memory);
        if (total_memory > 0 && total_memory < smallest) {
            smallest = total_memory;
        }
        caps.caps = 0x10004040;         // DDSCAPS_LOCALVIDMEM | VIDEOMEMORY | OFFSCREENPLAIN
        direct_draw->vtable->get_available_vid_mem(direct_draw, &caps, &total_memory, &free_memory);
        if (total_memory > 0 && total_memory < smallest) {
            smallest = total_memory;
        }

        display_adapters[adapter_index].video_memory = shell_round_video_memory(smallest);
        direct_draw->vtable->release(direct_draw);
    }
    FreeLibrary((HMODULE)ddraw_module);

    // DxDiag sound devices
    memset(sound_devices, 0, sizeof(sound_devices));
    sound_device_count = 0;
    selected_sound_device = 0;
    GetDeviceID((LPCGUID)dsdevid_default_playback, (LPGUID)default_guid);
    CoInitialize(0);

    provider = 0;
    root = 0;
    if (CoCreateInstance((REFCLSID)clsid_dxdiag_provider, 0, 1 /* CLSCTX_INPROC_SERVER */, (REFIID)iid_dxdiag_provider,
                         (void **)&provider) < 0) {
        CoUninitialize();
        return;
    }

    params.size = sizeof(params);
    params.header_version = 0x6f;       // DXDIAG_DX9_SDK_VERSION
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
                    // szGuidDeviceID "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
                    device->vtable->get_prop(device, (const uint16_t *)L"szGuidDeviceID", &variant);
                    if (variant.type == 8 /* VT_BSTR */) {
                        WideCharToMultiByte(0, 0, (LPCWCH)variant.value, -1, text, sizeof(text), 0, 0);
                        _strlwr(text);
                        record->guid[0] = (uint32_t)hex_string_to_uint(text + 1);
                        ((uint16_t *)record->guid)[2] = (uint16_t)hex_string_to_uint(text + 10);
                        ((uint16_t *)record->guid)[3] = (uint16_t)hex_string_to_uint(text + 15);
                        hex_string_to_bytes((uint8_t *)record->guid + 8, text + 20);
                        hex_string_to_bytes((uint8_t *)record->guid + 10, text + 25);
                        if (memcmp(record->guid, default_guid, 0x10) == 0) {
                            selected_sound_device = (int32_t)device_index;
                        }
                        VariantClear((VARIANTARG *)&variant);
                    }

                    device->vtable->get_prop(device, (const uint16_t *)L"szDescription", &variant);
                    if (variant.type == 8) {
                        WideCharToMultiByte(0, 0, (LPCWCH)variant.value, -1, text, sizeof(text), 0, 0);
                        strncpy(record->description, text, 0x1f);
                        record->description[0x1f] = 0;
                        VariantClear((VARIANTARG *)&variant);
                    }

                    // szHardwareID "pci\ven_xxxx&dev_xxxx&subsys_xxxxxxxx&rev_xx..."
                    device->vtable->get_prop(device, (const uint16_t *)L"szHardwareID", &variant);
                    if (variant.type == 8) {
                        WideCharToMultiByte(0, 0, (LPCWCH)variant.value, -1, text, sizeof(text), 0, 0);
                        _strlwr(text);
                        if (memcmp(text, "*nforce", 8) == 0) {
                            memcpy(text, SOUND_DEVICE_HARDWARE_ID_NFORCE, 0x3b);
                        }
                        match = strstr(text, "ven_");
                        record->vendor_id = match != 0 ? (uint32_t)hex_string_to_uint(match + 4) : 0;
                        match = strstr(text, "dev_");
                        record->device_id = match != 0 ? (uint32_t)hex_string_to_uint(match + 4) : 0;
                        match = strstr(text, "subsys_");
                        record->subsystem_id = match != 0 ? (uint32_t)hex_string_to_uint(match + 7) : 0;
                        match = strstr(text, "rev_");
                        record->revision = match != 0 ? (uint32_t)hex_string_to_uint(match + 7) : 0; // +7, see header
                        VariantClear((VARIANTARG *)&variant);
                    }

                    // szDriverVersion "a.b.c.d"
                    device->vtable->get_prop(device, (const uint16_t *)L"szDriverVersion", &variant);
                    if (variant.type == 8) {
                        WideCharToMultiByte(0, 0, (LPCWCH)variant.value, -1, text, sizeof(text), 0, 0);
                        version_d = 0;
                        sscanf(text, "%d.%d.%d.%d", &version_a, &version_b, &version_c, &version_d);
                        record->driver_version.parts.high_part = (version_a << 16) + version_b;
                        record->driver_version.parts.low_part = (uint32_t)((version_c << 16) + version_d);
                        VariantClear((VARIANTARG *)&variant);
                    }

                    device->vtable->release(device);
                }
                child_name = child_name + 1;    // 2 bytes, see header
                record = record + 1;
            }
            devices->vtable->release(devices);
        }
        root->vtable->release(root);
    }
    provider->vtable->release(provider);
    CoUninitialize();
}

#if 0
Original Ghidra decompilation (0x57d880), unreliable past the DirectDraw loop (see header):

void shell_detect_hardware_specs(void)

{
  undefined8 uVar1;
  HANDLE hThread;
  HANDLE hProcess;
  int iVar2;
  DWORD dwPriorityClass;
  HMODULE hModule;
  FARPROC pFVar3;
  uint uVar4;
  HRESULT HVar5;
  int iVar6;
  uint uVar7;
  undefined4 unaff_EBP;
  undefined2 *puVar8;
  undefined **unaff_ESI;
  undefined **ppuVar9;
  byte *pbVar10;
  undefined4 *puVar11;
  HMODULE *ppHVar12;
  byte *pbVar13;
  undefined4 *puVar14;
  bool bVar15;
  bool bVar16;
  longlong lVar17;
  _MEMORYSTATUS local_120 [9];

  GlobalMemoryStatus(local_120);
  if (0x40000000 < local_120[0].dwTotalPhys) {
    local_120[0].dwTotalPhys = 0x40000000;
  }
  DAT_00722ba8 = local_120[0].dwTotalPhys + 0xffffff >> 0x14 & 0xff0;
  hThread = GetCurrentThread();
  hProcess = GetCurrentProcess();
  iVar2 = GetThreadPriority(hThread);
  dwPriorityClass = GetPriorityClass(hProcess);
  SetPriorityClass(hProcess,0x100);
  SetThreadPriority(hThread,0xf);
  Sleep(100);
  QueryPerformanceFrequency(&local_160);
  QueryPerformanceCounter(&local_168);
  lVar17 = __alldiv();
  local_168 = (LARGE_INTEGER)(lVar17 + CONCAT44(local_168.s.HighPart,local_168.s.LowPart));
  uVar1 = rdtsc();
  local_14c = (undefined1 *)((ulonglong)uVar1 >> 0x20);
  local_150 = (uint)uVar1;
  do {
    QueryPerformanceCounter(&local_160);
  } while (CONCAT44(local_160.s.HighPart,local_160.s.LowPart) < (longlong)local_168);
  uVar1 = rdtsc();
  bVar15 = (uint)uVar1 < local_150;
  local_150 = (uint)uVar1 - local_150;
  local_14c = (undefined1 *)(((int)((ulonglong)uVar1 >> 0x20) - (int)local_14c) - (uint)bVar15);
  SetThreadPriority(hThread,iVar2);
  SetPriorityClass(hProcess,dwPriorityClass);
  DAT_00722bac = __alldiv();
  if (1000 < DAT_00722bac) {
    DAT_00722bac = ((DAT_00722bac + 0x32) / 100) * 100;
  }
  uVar7 = DAT_00722bac % 100;
  if (DAT_00722bac < 200) {
    if (0x5f < uVar7) {
      DAT_00722bac = DAT_00722bac + (100 - uVar7);
    }
    if ((0x3d < uVar7) && (uVar7 < 0x47)) {
      DAT_00722bac = DAT_00722bac + (0x42 - uVar7);
    }
    if ((0x2d < uVar7) && (uVar7 < 0x37)) {
      DAT_00722bac = DAT_00722bac + (0x32 - uVar7);
    }
    if ((0x1c < uVar7) && (uVar7 < 0x26)) {
      DAT_00722bac = DAT_00722bac + (0x21 - uVar7);
    }
  }
  else if (uVar7 < 0x55) {
    if (uVar7 < 0x3b) {
      if (uVar7 < 0x2b) {
        if (uVar7 < 0x11) {
          if (uVar7 < 0x10) {
            DAT_00722bac = DAT_00722bac - uVar7;
          }
        }
        else {
          DAT_00722bac = DAT_00722bac + (0x21 - uVar7);
        }
      }
      else {
        DAT_00722bac = DAT_00722bac + (0x32 - uVar7);
      }
    }
    else {
      DAT_00722bac = DAT_00722bac + (0x42 - uVar7);
    }
  }
  else {
    DAT_00722bac = DAT_00722bac + (100 - uVar7);
  }
  hModule = LoadLibraryA("ddraw.dll");
  local_16c = hModule;
  local_158 = GetProcAddress(hModule,"DirectDrawCreateEx");
  if (local_158 == (FARPROC)0x0) {
    shell_display_fatal_error_dialog();
  }
  pFVar3 = GetProcAddress(hModule,"DirectDrawEnumerateExA");
  if (pFVar3 == (FARPROC)0x0) {
    shell_display_fatal_error_dialog();
  }
  (*pFVar3)();
  ... (the remainder of the decompile, 330 lines of scrambled COM calls, is omitted; the full
  text is in python tools/pack.py 0x57d880. It calls the DirectDraw vtable at +0x50 / +0x5c / +8,
  FreeLibrary, clears 0x104 dwords at 0x006ef9b0, GetDeviceID, CoInitialize, CoCreateInstance,
  the DxDiag vtables at +0xc / +0x10 / +0x14 / +0x20 / +8, WideCharToMultiByte, __strlwr,
  hex_string_to_uint, hex_string_to_bytes, _strncpy, FUN_00625430 (strstr) against "ven_",
  "dev_", "subsys_" and "rev_", _sscanf "%d.%d.%d.%d", VariantClear and CoUninitialize.)
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
