// shell_parse_config_txt  (Ghidra: shell_parse_config_txt, already named)
// address 0x57d410, size 969 bytes
// name confidence: 0.65  rewrite confidence: 0.75
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Loads and parses
//   config.txt, applying recognized system-requirement and shader/driver-management settings and
//   reporting unknown or malformed properties." Every global matches types/shell.h's config_* /
//   required_* / graphics_* field list by address; out/phase4/shell_types_notes.md documents the
//   d3d_adapter_identifier9 / d3d_caps9 offsets this function fills via GetAdapterIdentifier /
//   GetDeviceCaps and the config property table it walks.
// register convention: Ghidra recovered zero parameters and only "in_EDX" (the IDirect3D9 *,
//   confirmed by out/phase4/shell_types_notes.md). objdump shows a SECOND, completely dropped
//   parameter: "mov edi,ecx; mov esi,edx" at entry, and edi (the ECX parameter) is then pushed as
//   GetAdapterIdentifier's and GetDeviceCaps's Adapter argument -- so ECX is the adapter index,
//   not a hidden this. blam-cc: adapter_index in ECX, d3d in EDX.
// Review fix: get_error_message (vtable +0x3c, 0x5788e0) takes no argument (a plain ret). The
//   "push ebx" (ebx == 1) at 0x57d509 before that call is the third argument of the fatal error
//   call that follows, scheduled early by the compiler; it was misread as an argument.
// Review fix: the property setter call (0x57d5f2..0x57d611) is wrapped in __try /
//   __except(EXCEPTION_EXECUTE_HANDLER) (scope table 0x673068); a setter that faults counts as a
//   failed setter (xor al,al at 0x57d61c) and produces the "Error in config.txt" message.
// Correction: the fatal-error call here has 3 arguments, not the 2 the Ghidra decompile for THIS
// function showed -- "push ebx" (ebx == 1 from the GetDeviceCaps DeviceType setup, still live)
// immediately precedes "push eax; push 0xffffffff; call 0x57ea70; add esp,0xc" (0xc == 3 dwords).
// shell_display_fatal_error_dialog 0x57ea70's own Ghidra decompile confirms a 3-parameter
// signature (resource_id, message, is_fatal); its third argument here is 1.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "interface.h"

// The IDirect3D9 slot typedefs (d3d9_get_adapter_identifier_fn / d3d9_get_device_caps_fn) and the
// hwreq_parser_vtable slot typedefs (hwreq_*_fn) are in types/shell.h.

extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b CRT
extern int32_t sscanf(const char *buffer, const char *format, ...); // 0x626572 CRT
extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693 CRT
extern hwreq_parser *hwreq_parser_create(void); // 0x57b4c0
extern void config_reset_system_requirements(void); // 0x57cfe0
extern int32_t shell_display_fatal_error_dialog(uint32_t resource_id, const char *help_text, int32_t is_fatal); // 0x57ea70

extern large_integer graphics_driver_version; // 0x00722ba0
extern uint32_t video_memory;                 // 0x00722bb0
extern uint32_t display_adapter_count;        // 0x00722bb4
extern shell_display_adapter display_adapters[k_shell_maximum_display_adapters]; // 0x006efdc0
extern hwreq_parser *hardware_requirements;   // 0x006effdc
extern shell_sound_device sound_devices[k_shell_maximum_sound_devices]; // 0x006ef9b0
extern int32_t selected_sound_device;         // 0x006effd0
extern uint32_t physical_memory;              // 0x00722ba8
extern uint32_t cpu_speed;                    // 0x00722bac
extern char *graphics_vendor_name;            // 0x00722b90
extern char *graphics_device_name;            // 0x00722b94
extern uint32_t graphics_device_id;           // 0x00722b98
extern uint32_t graphics_vendor_id;           // 0x00722b9c
extern shell_config_property config_properties[k_shell_config_property_count]; // 0x0069fe40
extern char config_unknown_property_text[k_shell_config_message_length]; // 0x00722f58
extern char config_error_text[k_shell_config_message_length];            // 0x00722e58
extern int32_t config_use_fixed_function;          // 0x00722b34
extern int32_t config_disable_driver_management;   // 0x00722b38
extern int32_t config_disable_buffering;           // 0x00722b54
extern int32_t config_force_shader;                // 0x00722b64
extern int32_t config_min_max_blend_op_is_broken;  // 0x00722b7c
extern int32_t config_maximum_resolution;          // 0x0069fe3c
extern int32_t config_disable_render_targets;      // 0x00722b70
extern int32_t config_disable_specular;            // 0x00722b6c
extern int32_t required_cpu_speed;                // 0x006effe0
extern int32_t required_memory;                   // 0x006effcc
extern int32_t required_video_memory;             // 0x006effc8
extern int32_t required_directx_build;            // 0x006effd8
extern int32_t required_disk_space;               // 0x006effd4
extern int32_t safe_mode;                          // 0x007196f4 (32 bit BOOL), -safemode; also set by the fatal error dialog

// Loads and parses config.txt against the given IDirect3D9 adapter (via the hardware
// requirements script parser), applying every recognized "flag = value" property to the
// matching config_properties setter (fatal-erroring on a parse failure), then reading the
// script's Requirements section into the required_* globals -- forcing a fixed safe-mode
// configuration first if -safemode is active. Returns NULL on success, or the address of a
// static message buffer describing an unknown or malformed config.txt property.
char *shell_parse_config_txt(uint32_t adapter_index, d3d9_interface *d3d)
{
    d3d_adapter_identifier9 identifier;
    d3d_caps9 caps;
    uint32_t i;
    uint32_t flag_count;
    uint32_t requirement_count;
    char *name;
    char *value;
    uint8_t found;
    uint32_t property_index;
    uint8_t ok;
    char *error_message;
    int32_t directx_scratch;
    void **d3d_vtable;
    hwreq_parser_vtable *vt;

    d3d_vtable = (void **)d3d->vtable;
    ((d3d9_get_adapter_identifier_fn)d3d_vtable[5])(d3d, adapter_index, 0, &identifier);
    ((d3d9_get_device_caps_fn)d3d_vtable[14])(d3d, adapter_index, 1 /* D3DDEVTYPE_HAL */, &caps);

    graphics_driver_version = identifier.driver_version;
    video_memory = display_adapters[0].video_memory;

    for (i = 0; i < display_adapter_count; i++) {
        if (__stricmp(display_adapters[i].driver_name, identifier.device_name) == 0) {
            video_memory = display_adapters[i].video_memory;
        }
    }

    hardware_requirements = hwreq_parser_create();
    vt = (hwreq_parser_vtable *)hardware_requirements->vtable;

    ok = ((hwreq_parse_fn)vt->parse)(hardware_requirements, "config.txt", &sound_devices[selected_sound_device],
                                      &identifier, &caps, physical_memory, video_memory, cpu_speed);
    if (ok == 0) {
        error_message = ((hwreq_get_string_fn)vt->get_error_message)(hardware_requirements);
        shell_display_fatal_error_dialog(0xffffffff, error_message, 1);
    }

    graphics_vendor_name = ((hwreq_get_string_fn)vt->get_graphics_vendor_name)(hardware_requirements);
    graphics_device_name = ((hwreq_get_string_fn)vt->get_graphics_device_name)(hardware_requirements);
    graphics_device_id = identifier.device_id;
    graphics_vendor_id = identifier.vendor_id;

    config_reset_system_requirements();

    flag_count = ((hwreq_get_count_fn)vt->get_flag_count)(hardware_requirements);
    for (i = 0; i < flag_count; i++) {
        name = ((hwreq_get_indexed_string_fn)vt->get_flag_name)(hardware_requirements, i);
        value = ((hwreq_get_indexed_string_fn)vt->get_flag_value)(hardware_requirements, i);

        found = 0;
        for (property_index = 0; property_index < k_shell_config_property_count; property_index++) {
            if (__stricmp((const char *)config_properties[property_index].name, name) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            sprintf(config_unknown_property_text, "Unknown property in config.txt '%s'", name);
            return config_unknown_property_text;
        }

#if defined(_MSC_VER)
        __try {
            ok = ((shell_config_property_setter)config_properties[property_index].setter)(value);
        } __except (1) {
            ok = 0;
        }
#else
        ok = ((shell_config_property_setter)config_properties[property_index].setter)(value);
#endif
        if (!ok) {
            sprintf(config_error_text, "Error in config.txt '%s'='%s'", name, value);
            return config_error_text;
        }
    }

    if (safe_mode != 0) {
        config_use_fixed_function = 1;
        config_disable_driver_management = 1;
        config_disable_buffering = 1;
        config_force_shader = 9999;
        config_min_max_blend_op_is_broken = 1;
        config_maximum_resolution = 800;
        config_disable_render_targets = 1;
        config_disable_specular = 1;
    }

    required_cpu_speed = k_shell_required_cpu_speed_default;
    required_memory = k_shell_required_memory_default;
    required_video_memory = k_shell_required_video_memory_default;
    required_directx_build = k_shell_required_directx_build_default;
    required_disk_space = k_shell_required_disk_space_default;

    requirement_count = ((hwreq_get_count_fn)vt->get_requirement_count)(hardware_requirements);
    for (i = 0; i < requirement_count; i++) {
        name = ((hwreq_get_indexed_string_fn)vt->get_requirement_name)(hardware_requirements, i);
        value = ((hwreq_get_indexed_string_fn)vt->get_requirement_value)(hardware_requirements, i);

        if (__stricmp(name, "CpuSpeed") == 0) {
            sscanf(value, "%d", &required_cpu_speed);
        }
        if (__stricmp(name, "Memory") == 0) {
            sscanf(value, "%d", &required_memory);
        }
        if (__stricmp(name, "VideoMemory") == 0) {
            sscanf(value, "%d", &required_video_memory);
        }
        if (__stricmp(name, "DirectX") == 0) {
            sscanf(value, "%d.%d.%d.%d", &directx_scratch, &directx_scratch, &directx_scratch, &required_directx_build);
        }
        if (__stricmp(name, "DiskSpace") == 0) {
            sscanf(value, "%d", &required_disk_space);
        }
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x57d410):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined * shell_parse_config_txt(void)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  undefined4 uVar4;
  char *_Str2;
  char *_Src;
  int *in_EDX;
  uint uVar5;
  uint uVar6;
  undefined4 *puVar7;
  undefined1 local_5b0 [1024];
  char local_1b0 [32];
  undefined4 local_190;
  undefined4 local_18c;
  undefined4 local_188;
  undefined4 local_184;
  undefined1 local_164 [304];
  uint local_34;
  undefined1 local_30 [4];
  undefined4 local_2c;
  uint local_28;
  char local_21;
  char *local_20;
  undefined1 *local_1c;
  void *local_14;
  undefined1 *puStack_10;
  undefined *puStack_c;
  undefined4 local_8;

  local_8 = 0xffffffff;
  puStack_c = &DAT_00673068;
  puStack_10 = &LAB_00628dfc;
  local_14 = ExceptionList;
  local_1c = &stack0xfffffa44;
  ExceptionList = &local_14;
  (**(code **)(*in_EDX + 0x14))();
  (**(code **)(*in_EDX + 0x38))();
  _DAT_00722ba0 = local_190;
  _DAT_00722ba4 = local_18c;
  DAT_00722bb0 = DAT_006efdf0;
  uVar5 = 0;
  if (DAT_00722bb4 != 0) {
    puVar7 = &DAT_006efdf0;
    do {
      iVar3 = __stricmp((char *)(puVar7 + -8),local_1b0);
      if (iVar3 == 0) {
        DAT_00722bb0 = *puVar7;
      }
      uVar5 = uVar5 + 1;
      puVar7 = puVar7 + 0xd;
    } while (uVar5 < DAT_00722bb4);
  }
  DAT_006effdc = (int *)hwreq_parser_create();
  cVar2 = (**(code **)*DAT_006effdc)
                    ("config.txt",&DAT_006ef9b0 + DAT_006effd0 * 0x1a,local_5b0,local_164,
                     DAT_00722ba8,DAT_00722bb0,DAT_00722bac);
  if (cVar2 == '\0') {
    uVar4 = (**(code **)(*DAT_006effdc + 0x3c))(1);
    shell_display_fatal_error_dialog(0xffffffff,uVar4);
  }
  DAT_00722b90 = (**(code **)(*DAT_006effdc + 0x2c))();
  DAT_00722b94 = (**(code **)(*DAT_006effdc + 0x28))();
  DAT_00722b98 = local_184;
  DAT_00722b9c = local_188;
  config_reset_system_requirements();
  local_28 = (**(code **)(*DAT_006effdc + 0x10))();
  uVar5 = 0;
  do {
    local_34 = uVar5;
    if (local_28 <= uVar5) {
      if (DAT_007196f4 != 0) {
        DAT_00722b34 = 1;
        DAT_00722b38 = 1;
        DAT_00722b54 = 1;
        DAT_00722b64 = 9999;
        DAT_00722b7c = 1;
        DAT_0069fe3c = 800;
        DAT_00722b70 = 1;
        DAT_00722b6c = 1;
      }
      DAT_006effe0 = 0x2dd;
      DAT_006effcc = 0x80;
      DAT_006effc8 = 0x20;
      _DAT_006effd8 = 0x386;
      DAT_006effd4 = 100;
      local_28 = (**(code **)(*DAT_006effdc + 0x1c))();
      uVar5 = 0;
      if (local_28 != 0) {
        do {
          _Str2 = (char *)(**(code **)(*DAT_006effdc + 0x20))(uVar5);
          _Src = (char *)(**(code **)(*DAT_006effdc + 0x24))(uVar5);
          iVar3 = __stricmp("CpuSpeed",_Str2);
          if (iVar3 == 0) {
            _sscanf(_Src,"%d",&DAT_006effe0);
          }
          iVar3 = __stricmp("Memory",_Str2);
          if (iVar3 == 0) {
            _sscanf(_Src,"%d",&DAT_006effcc);
          }
          iVar3 = __stricmp("VideoMemory",_Str2);
          if (iVar3 == 0) {
            _sscanf(_Src,"%d",&DAT_006effc8);
          }
          iVar3 = __stricmp("DirectX",_Str2);
          if (iVar3 == 0) {
            _sscanf(_Src,"%d.%d.%d.%d",local_30,local_30,local_30,&DAT_006effd8);
          }
          iVar3 = __stricmp("DiskSpace",_Str2);
          if (iVar3 == 0) {
            _sscanf(_Src,"%d",&DAT_006effd4);
          }
          uVar5 = uVar5 + 1;
        } while (uVar5 < local_28);
      }
      ExceptionList = local_14;
      return (undefined *)0x0;
    }
    local_20 = (char *)(**(code **)(*DAT_006effdc + 0x14))(uVar5);
    local_2c = (**(code **)(*DAT_006effdc + 0x18))(uVar5);
    bVar1 = false;
    uVar6 = 0;
    do {
      iVar3 = __stricmp((&PTR_s_ForceShader_0069fe40)[uVar6 * 2],local_20);
      if (iVar3 == 0) {
        bVar1 = true;
        break;
      }
      uVar6 = uVar6 + 1;
    } while (uVar6 < 0x1c);
    if (!bVar1) {
      _sprintf(&DAT_00722f58,"Unknown property in config.txt \'%s\'",local_20);
      ExceptionList = local_14;
      return &DAT_00722f58;
    }
    local_8 = 0;
    local_21 = (**(code **)(uVar6 * 8 + 0x69fe44))(local_2c);
    local_8 = 0xffffffff;
    if (local_21 == '\0') {
      _sprintf(&DAT_00722e58,"Error in config.txt \'%s\'=\'%s\'",local_20,local_2c);
      ExceptionList = local_14;
      return &DAT_00722e58;
    }
    uVar5 = uVar5 + 1;
  } while( true );
}
#endif
