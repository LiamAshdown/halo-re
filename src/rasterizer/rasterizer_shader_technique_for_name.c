// rasterizer_shader_technique_for_name  (Ghidra: rasterizer_shader_technique_for_name, already named)
// address 0x530120, size 143 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: cea-pdb name match on "%s_ps_%d_%d"; same GetTechniqueByName(0x34)/ValidateTechnique
//   (0xf4) version-degrading loop as rasterizer_dx9_shaders_init_effect.c, generalized to a
//   caller-supplied name prefix and a caller-supplied effect object (all 3 callers --
//   rasterizer_screen_flash_init_shaders, rasterizer_screen_effect_init_shaders and
//   rasterizer_shader_environment_build_technique_table -- pass Ghidra's `unaff_EDI` with no
//   visible setup, i.e. it really is a plain register parameter here, not corruption).
// Phase 4 review fix: ID3DXEffect::GetTechniqueByName (+0x34) takes only the name (the push
//   sequence at 0x530178 is name, effect); the earlier file passed an extra parent handle.
// register convention: EDI -> effect, stack -> name. // blam-cc: EDI -> effect, stack -> name
// UNSURE: on a total failure (no "<name>_ps_<major>_<minor>" validates) this returns whatever the
//   very last GetTechniqueByName call returned, which is not necessarily NULL; not corrected.

#include "crt.h"
#include "tags.h"
#include "math.h"
#include "rasterizer.h"
#include <stdint.h> // uintptr_t

extern d3d_caps9 rasterizer_caps; // 0x007c10c0

typedef int32_t (__stdcall *d3dx_get_by_name_fn)(void *effect, const char *name);
typedef int32_t (__stdcall *d3dx_validate_technique_fn)(void *effect, void *technique);

// VERIFIED against disassembly 0x530120..0x5301ae (2026-09-30): version split (0x7c118c = caps.PixelShaderVersion, major = bits
//   8..15, minor = bits 0..7), loop order/reset to minor 9, sprintf argument order, both stdcall vtable calls (+0x34 by name,
//   +0xf4 validate) and the hr >= 0 test match. A difftest "died" needs a real ID3DXEffect behind EDI.
// Finds and validates the best pixel-shader technique named "<name>_ps_<major>_<minor>" supported
// by the current pixel-shader-model version, degrading the minor then major version number until
// one validates.
void *rasterizer_shader_technique_for_name(void *effect, const char *name)
{
    void **vt = *(void ***)effect;
    char full_name[128];
    void *technique = 0;
    int32_t major, minor;
    int32_t found = 0;

    major = (rasterizer_caps.pixel_shader_version >> 8) & 0xff;
    minor = rasterizer_caps.pixel_shader_version & 0xff;
    for (; !found && major >= 0; major--, minor = 9) {
        for (; !found && minor >= 0; minor--) {
            sprintf(full_name, "%s_ps_%d_%d", name, major, minor);
            technique = (void *)(uintptr_t)((d3dx_get_by_name_fn)vt[0x34 / 4])(effect, full_name);
            if (technique != 0) {
                int32_t hr = ((d3dx_validate_technique_fn)vt[0xf4 / 4])(effect, technique);
                found = hr >= 0;
            }
        }
    }
    return technique;
}

#if 0
Original Ghidra decompilation (0x530120):

int rasterizer_shader_technique_for_name(undefined4 param_1)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  int *unaff_EDI;
  bool bVar5;
  char local_80 [128];

  uVar3 = DAT_007c118c >> 8 & 0xff;
  iVar4 = 0;
  uVar1 = DAT_007c118c & 0xff;
  bVar5 = false;
  do {
    if (bVar5) {
      return iVar4;
    }
    do {
      if (bVar5) break;
      _sprintf(local_80,"%s_ps_%d_%d",param_1,uVar3,uVar1);
      iVar4 = (**(code **)(*unaff_EDI + 0x34))();
      if (iVar4 != 0) {
        iVar2 = (**(code **)(*unaff_EDI + 0xf4))();
        bVar5 = -1 < iVar2;
      }
      uVar1 = uVar1 - 1;
    } while (-1 < (int)uVar1);
    uVar3 = uVar3 - 1;
    uVar1 = 9;
    if ((int)uVar3 < 0) {
      return iVar4;
    }
  } while( true );
}
#endif
