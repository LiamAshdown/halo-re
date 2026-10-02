// rasterizer_shader_environment_build_technique_table  (Ghidra: already named)
// address 0x526930, size 953 bytes
// name confidence: 0.65   rewrite confidence: 0.9
// evidence: raw disassembly (phase 4 review). The earlier file lost the effect every lookup runs
//   against (0x530120 takes it in EDI) and swapped the table sizes of the reflection and plain
//   stages. Each stage formats "<Kind>%s" against the 0x80 byte stride suffix table 0x0069c750,
//   resolves it with rasterizer_shader_technique_for_name in the effect of that kind and stores
//   the handle (0 included); the first failure clears the result and skips every later stage.
//   Effects: 116 EnvironmentNo, 117 SelfIllumination, 118 ChangeColor, 119 Multipurpose, 120
//   Reflection, 121 No and No..SelfIllumination. The EnvironmentNo and No stages use suffixes
//   0..5 and 12..17 (entries 6 and up skip 0x300 bytes).
//   Table sizes below ps_1_4 / from ps_1_4: EnvironmentNo 6/12, SelfIllumination 12/24,
//   ChangeColor 12/24, Multipurpose 12/24, Reflection 12/24, No 6/12; the No%sSelfIllumination
//   stage (6 entries into 0x006e1950, the second half of the plain table) runs only on ps_1_1..1_3.
// register convention: none. Returns 1 when every technique was found.
// blam-cc: none
// UNSURE: the suffix table 0x0069c750 is only known through the three strings the string list
//   shows ("MaskDetailBeforeReflectionBiasedMultiply" ...); its size is not bounded here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern d3d_caps9 rasterizer_caps;                                    // 0x007c10c0
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern int32_t environment_techniques_multipurpose[24];              // 0x006e1780
extern int32_t environment_techniques_no[12];                        // 0x006e17f4
extern int32_t environment_techniques_self_illumination[24];         // 0x006e18d8
extern int32_t environment_techniques_plain[12];                     // 0x006e1938 (+6 is 0x006e1950)
extern int32_t environment_techniques_reflection[24];                // 0x006e1968
extern int32_t environment_techniques_change_color[24];              // 0x006e19d0
extern const char rasterizer_shader_technique_name_suffixes[][0x80]; // 0x0069c750

extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693 CRT
// blam-cc: EDI -> effect, stack -> name
extern void *rasterizer_shader_technique_for_name(void *effect, const char *name); // 0x530120

// One stage: returns 0 at the first name that does not resolve.
static uint8_t build_stage(int32_t *table, int32_t count, int effect_index, const char *format, uint8_t skip_middle)
{
    char name[128];
    int32_t i;

    for (i = 0; i < count; i++) {
        int32_t suffix = (skip_middle && i >= 6) ? i + 6 : i;   // + 0x300 bytes from entry 6 on
        void *technique;

        sprintf(name, format, rasterizer_shader_technique_name_suffixes[suffix]);
        technique = rasterizer_shader_technique_for_name((void *)(uintptr_t)rasterizer_effects[effect_index].effect, name);
        table[i] = (int32_t)(uintptr_t)technique;
        if (technique == NULL) {
            return 0;
        }
    }
    return 1;
}

uint8_t rasterizer_shader_environment_build_technique_table(void)
{
    uint8_t ps_1_4 = rasterizer_caps.pixel_shader_version >= 0xffff0104;
    int32_t short_count = ps_1_4 ? 0xc : 6;
    int32_t long_count = ps_1_4 ? 0x18 : 0xc;
    uint8_t ok;

    ok = build_stage(environment_techniques_no, short_count, 116, "EnvironmentNo%s", 1);
    ok = ok && build_stage(environment_techniques_self_illumination, long_count, 117, "SelfIllumination%s", 0);
    ok = ok && build_stage(environment_techniques_change_color, long_count, 118, "ChangeColor%s", 0);
    ok = ok && build_stage(environment_techniques_multipurpose, long_count, 119, "Multipurpose%s", 0);
    ok = ok && build_stage(environment_techniques_reflection, long_count, 120, "Reflection%s", 0);
    ok = ok && build_stage(environment_techniques_plain, short_count, 121, "No%s", 1);
    if (rasterizer_caps.pixel_shader_version >= 0xffff0101 && rasterizer_caps.pixel_shader_version < 0xffff0104 && ok) {
        ok = build_stage(&environment_techniques_plain[6], short_count, 121, "No%sSelfIllumination", 0);
    }
    return ok;
}

#if 0
Original Ghidra decompilation (0x526930):

char rasterizer_shader_environment_build_technique_table(void)

{
  bool bVar1;
  int iVar2;
  char cVar3;
  char cVar4;
  int iVar5;
  char *pcVar6;
  int iVar7;
  int local_98;
  int local_94;
  int local_90;
  int local_8c;
  int local_88;
  int local_84;
  char local_80 [128];
  
  if (DAT_007c118c < 0xffff0104) {
    local_8c = 6;
    local_94 = 0xc;
    local_84 = 0xc;
    local_90 = 0xc;
    local_88 = 0xc;
    local_98 = 6;
  }
  else {
    local_8c = 0xc;
    local_94 = 0x18;
    local_84 = 0x18;
    local_90 = 0x18;
    local_88 = 0x18;
    local_98 = 0xc;
  }
  iVar7 = 0;
  iVar5 = 0;
  do {
    if (local_8c <= iVar7) {
      bVar1 = true;
      goto LAB_00526a08;
    }
    iVar2 = iVar5 + 0x300;
    if (iVar7 < 6) {
      iVar2 = iVar5;
    }
    _sprintf(local_80,"EnvironmentNo%s",s_MaskDetailBeforeReflectionBiased_0069c750 + iVar2);
    iVar2 = rasterizer_shader_technique_for_name(local_80);
    (&DAT_006e17f4)[iVar7] = iVar2;
    iVar5 = iVar5 + 0x80;
    iVar7 = iVar7 + 1;
  } while (iVar2 != 0);
  bVar1 = false;
LAB_00526a08:
  iVar5 = 0;
  if (bVar1) {
    pcVar6 = s_MaskDetailBeforeReflectionBiased_0069c750;
    do {
      if (local_94 <= iVar5) {
        bVar1 = true;
        goto LAB_00526a7f;
      }
      _sprintf(local_80,"SelfIllumination%s",pcVar6);
      iVar7 = rasterizer_shader_technique_for_name(local_80);
      (&DAT_006e18d8)[iVar5] = iVar7;
      pcVar6 = pcVar6 + 0x80;
      iVar5 = iVar5 + 1;
    } while (iVar7 != 0);
  }
  bVar1 = false;
LAB_00526a7f:
  iVar5 = 0;
  if (bVar1) {
    pcVar6 = s_MaskDetailBeforeReflectionBiased_0069c750;
    do {
      if (local_84 <= iVar5) {
        bVar1 = true;
        goto LAB_00526afc;
      }
      _sprintf(local_80,"ChangeColor%s",pcVar6);
      iVar7 = rasterizer_shader_technique_for_name(local_80);
      (&DAT_006e19d0)[iVar5] = iVar7;
      pcVar6 = pcVar6 + 0x80;
      iVar5 = iVar5 + 1;
    } while (iVar7 != 0);
  }
  bVar1 = false;
LAB_00526afc:
  iVar5 = 0;
  if (bVar1) {
    pcVar6 = s_MaskDetailBeforeReflectionBiased_0069c750;
    do {
      if (local_90 <= iVar5) {
        bVar1 = true;
        goto LAB_00526b7b;
      }
      _sprintf(local_80,"Multipurpose%s",pcVar6);
      iVar7 = rasterizer_shader_technique_for_name(local_80);
      (&DAT_006e1780)[iVar5] = iVar7;
      pcVar6 = pcVar6 + 0x80;
      iVar5 = iVar5 + 1;
    } while (iVar7 != 0);
  }
  bVar1 = false;
LAB_00526b7b:
  iVar5 = 0;
  if (bVar1) {
    pcVar6 = s_MaskDetailBeforeReflectionBiased_0069c750;
    do {
      if (local_88 <= iVar5) {
        cVar3 = '\x01';
        goto LAB_00526bf2;
      }
      _sprintf(local_80,"Reflection%s",pcVar6);
      iVar7 = rasterizer_shader_technique_for_name(local_80);
      (&DAT_006e1968)[iVar5] = iVar7;
      pcVar6 = pcVar6 + 0x80;
      iVar5 = iVar5 + 1;
    } while (iVar7 != 0);
  }
  cVar3 = '\0';
LAB_00526bf2:
  iVar5 = 0;
  if (cVar3 != '\0') {
    iVar7 = 0;
    do {
      if (local_98 <= iVar5) break;
      iVar2 = iVar7 + 0x300;
      if (iVar5 < 6) {
        iVar2 = iVar7;
      }
      _sprintf(local_80,"No%s",s_MaskDetailBeforeReflectionBiased_0069c750 + iVar2);
      if (cVar3 == '\0') {
LAB_00526c6c:
        cVar3 = '\0';
      }
      else {
        iVar2 = rasterizer_shader_technique_for_name(local_80);
        (&DAT_006e1938)[iVar5] = iVar2;
        if (iVar2 == 0) goto LAB_00526c6c;
        cVar3 = '\x01';
      }
      iVar5 = iVar5 + 1;
      iVar7 = iVar7 + 0x80;
    } while (cVar3 != '\0');
  }
  if (((0xffff0100 < DAT_007c118c) && (DAT_007c118c < 0xffff0104)) && (iVar5 = 0, cVar3 != '\0')) {
    pcVar6 = s_MaskDetailBeforeReflectionBiased_0069c750;
    cVar4 = cVar3;
    do {
      if (local_98 <= iVar5) {
        return cVar4;
      }
      _sprintf(local_80,"No%sSelfIllumination",pcVar6);
      if (cVar4 == '\0') {
LAB_00526cd6:
        cVar4 = '\0';
      }
      else {
        iVar7 = rasterizer_shader_technique_for_name(local_80);
        (&DAT_006e1950)[iVar5] = iVar7;
        if (iVar7 == 0) goto LAB_00526cd6;
        cVar4 = '\x01';
      }
      iVar5 = iVar5 + 1;
      pcVar6 = pcVar6 + 0x80;
      cVar3 = '\0';
    } while (cVar4 != '\0');
  }
  return cVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
