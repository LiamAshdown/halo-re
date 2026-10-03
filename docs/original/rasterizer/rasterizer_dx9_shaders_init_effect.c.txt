// rasterizer_dx9_shaders_init_effect  (Ghidra: rasterizer_dx9_shaders_init_effect, already named)
// address 0x52f780, size 499 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: cea-pdb name match on {"TDefault_ps","Texture0","Texture1"}; the ID3DXEffect vtable
//   offsets are cross-checked against the ones already established elsewhere in this module
//   (0x24 GetParameterByName, 0x88 SetVector, 0xd0 SetTexture, 0xec SetTechnique, 0x100 Begin):
//   0x34 sits at the same per-4-byte index spacing as GetTechniqueByName, 0xf4 as ValidateTechnique
//   and 0xf8 as FindNextValidTechnique(NULL, &out) -- consistent with this function trying
//   "ps_<major>_<minor>" names in descending version order, validating each, and falling back to
//   "TDefault_ps"/"TDefault_no_ps" (or "fallback" when neither -deferred nor a captured caps flag
//   applies) before activating whatever technique it found with SetTechnique.
// register convention: EAX -> effect_index (rasterizer_effects[] slot to initialize).
// UNSURE: `config_safe_mode`/`config_force_shader` are read but not otherwise documented in this
//   module; from the branch shape (skip the whole version-probing loop, go straight to the
//   "fallback" technique name) they read as a forced-fallback flag and a specific sentinel value
//   (0x270d) that also selects the plain fallback path.

#include "crt.h"
#include "tags.h"
#include "math.h"
#include "rasterizer.h"


#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410
extern d3d_caps9 rasterizer_caps;                                    // 0x007c10c0
extern int32_t config_safe_mode; // 0x00722b60 nonzero forces the "fallback" technique name
extern int32_t config_force_shader; // 0x00722b64 config pixel shader version; 0x270d also forces "fallback"

typedef int32_t (__stdcall *d3dx_get_by_name_fn)(void *effect, void *parent, const char *name);
typedef int32_t (__stdcall *d3dx_validate_technique_fn)(void *effect, void *technique);
typedef int32_t (__stdcall *d3dx_find_next_valid_technique_fn)(void *effect, void *technique, void *out_technique);
typedef int32_t (__stdcall *d3dx_set_technique_fn)(void *effect, void *technique);

// Finds and validates the best pixel-shader technique named "ps_<major>_<minor>" supported by the
// current pixel-shader-model version (degrading the minor, then major, version number until one
// validates), falls back to "TDefault_ps"/"TDefault_no_ps" (or "fallback" when forced) if none do,
// activates it with SetTechnique, then caches the effect's Texture0..Texture3 parameter handles.
int32_t rasterizer_dx9_shaders_init_effect(int32_t effect_index)
{
    void *effect;
    void **vt;
    char name[64];
    void *technique;
    int32_t hr;
    int32_t major, minor;
    int32_t found;
    int i;
    static const char *texture_param_names[4] = { "Texture0", "Texture1", "Texture2", "Texture3" };

    effect = (void *)rasterizer_effects[effect_index].effect;
    vt = *(void ***)effect;
    technique = 0;
    found = 0;

    if (config_safe_mode == 0 && config_force_shader != 0x270d) {
        major = (rasterizer_caps.pixel_shader_version >> 8) & 0xff;
        minor = rasterizer_caps.pixel_shader_version & 0xff;
        for (; !found && major >= 0; major--, minor = 9) {
            for (; !found && minor >= 0; minor--) {
                sprintf(name, "ps_%d_%d", major, minor);
                technique = (void *)((d3dx_get_by_name_fn)vt[0x34 / 4])(effect, 0, name);
                if (technique != 0) {
                    hr = ((d3dx_validate_technique_fn)vt[0xf4 / 4])(effect, technique);
                    found = hr >= 0;
                }
            }
        }
        if (found) {
            // Activate the ps_<major>_<minor> technique found above.
            hr = ((d3dx_set_technique_fn)vt[0xec / 4])(effect, technique);
        } else {
            // Fall back to the default technique name. Note this path does NOT call SetTechnique:
            // if GetTechniqueByName found "TDefault_ps"/"TDefault_no_ps" it only validates that
            // handle; if it did not, FindNextValidTechnique(NULL, &out) is called for its
            // documented side effect of making the first valid technique current, its own output
            // handle is discarded, and the still-zero `technique` is what gets "validated" next.
            // Transcribed exactly as the original compiled it, not corrected.
            const char *default_name = (rasterizer_caps.pixel_shader_version < 0xffff0101)
                                            ? "TDefault_no_ps" : "TDefault_ps";
            sprintf(name, default_name);
            technique = (void *)((d3dx_get_by_name_fn)vt[0x34 / 4])(effect, 0, name);
            if (technique == 0) {
                void *next_technique;
                hr = ((d3dx_find_next_valid_technique_fn)vt[0xf8 / 4])(effect, 0, &next_technique);
                if (hr < 0) {
                    return 0;
                }
            }
            hr = ((d3dx_validate_technique_fn)vt[0xf4 / 4])(effect, technique);
        }
        if (hr < 0) {
            return 0;
        }
        found = 1;
    } else {
        sprintf(name, "fallback");
        technique = (void *)((d3dx_get_by_name_fn)vt[0x34 / 4])(effect, 0, name);
        found = technique != 0;
        if (!found) {
            return 0;
        }
    }

    for (i = 0; i < 4; i++) {
        rasterizer_effects[effect_index].texture_handles[i] =
            (uint32_t)((d3dx_get_by_name_fn)vt[0x24 / 4])(effect, 0, texture_param_names[i]);
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x52f780):

bool rasterizer_dx9_shaders_init_effect(void)

{
  bool bVar1;
  int in_EAX;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  uint uVar5;
  uint uVar6;
  bool bVar7;
  char *_Format;
  char local_40 [64];

  iVar2 = 0;
  bVar7 = false;
  bVar1 = false;
  if ((DAT_00722b60 == 0) && (DAT_00722b64 != 0x270d)) {
    uVar5 = DAT_007c118c >> 8 & 0xff;
    uVar6 = DAT_007c118c & 0xff;
    do {
      if (bVar7) goto LAB_0052f8ad;
      do {
        if (bVar7) break;
        _sprintf(local_40,"ps_%d_%d",uVar5,uVar6);
        iVar2 = (**(code **)(*(int *)(&DAT_0069d410)[in_EAX * 8] + 0x34))
                          ((int *)(&DAT_0069d410)[in_EAX * 8],local_40);
        bVar7 = bVar1;
        if (iVar2 != 0) {
          iVar3 = (**(code **)(*(int *)(&DAT_0069d410)[in_EAX * 8] + 0xf4))
                            ((int *)(&DAT_0069d410)[in_EAX * 8],iVar2);
          bVar7 = -1 < iVar3;
        }
        uVar6 = uVar6 - 1;
        bVar1 = bVar7;
      } while (-1 < (int)uVar6);
      uVar5 = uVar5 - 1;
      uVar6 = 9;
    } while (-1 < (int)uVar5);
    if (bVar7) {
LAB_0052f8ad:
      iVar2 = (**(code **)(*(int *)(&DAT_0069d410)[in_EAX * 8] + 0xec))
                        ((int *)(&DAT_0069d410)[in_EAX * 8],iVar2);
    }
    else {
      if (DAT_007c118c < 0xffff0101) {
        _Format = "TDefault_no_ps";
      }
      else {
        _Format = "TDefault_ps";
      }
      _sprintf(local_40,_Format);
      iVar2 = (**(code **)(*(int *)(&DAT_0069d410)[in_EAX * 8] + 0x34))
                        ((int *)(&DAT_0069d410)[in_EAX * 8],local_40);
      if ((iVar2 == 0) &&
         (iVar3 = (**(code **)(*(int *)(&DAT_0069d410)[in_EAX * 8] + 0xf8))
                            ((int *)(&DAT_0069d410)[in_EAX * 8],0,&stack0xffffffb4), iVar3 < 0)) {
        return false;
      }
      iVar2 = (**(code **)(*(int *)(&DAT_0069d410)[in_EAX * 8] + 0xf4))
                        ((int *)(&DAT_0069d410)[in_EAX * 8],iVar2);
    }
    if (iVar2 < 0) {
      return false;
    }
    bVar7 = true;
  }
  else {
    _sprintf(local_40,"fallback");
    iVar2 = (**(code **)(*(int *)(&DAT_0069d410)[in_EAX * 8] + 0x34))
                      ((int *)(&DAT_0069d410)[in_EAX * 8],local_40);
    bVar7 = iVar2 != 0;
    if (!bVar7) {
      return bVar7;
    }
  }
  iVar2 = in_EAX * 0x20;
  uVar4 = (**(code **)(*(int *)(&DAT_0069d410)[in_EAX * 8] + 0x24))
                    ((int *)(&DAT_0069d410)[in_EAX * 8],0,"Texture0");
  *(undefined4 *)(&DAT_0069d418 + iVar2) = uVar4;
  uVar4 = (**(code **)(*(int *)(&DAT_0069d410)[in_EAX * 8] + 0x24))
                    ((int *)(&DAT_0069d410)[in_EAX * 8],0,"Texture1");
  *(undefined4 *)(&DAT_0069d41c + iVar2) = uVar4;
  uVar4 = (**(code **)(*(int *)(&DAT_0069d410)[in_EAX * 8] + 0x24))
                    ((int *)(&DAT_0069d410)[in_EAX * 8],0,"Texture2");
  *(undefined4 *)(&DAT_0069d420 + iVar2) = uVar4;
  uVar4 = (**(code **)(*(int *)(&DAT_0069d410)[in_EAX * 8] + 0x24))
                    ((int *)(&DAT_0069d410)[in_EAX * 8],0,"Texture3");
  *(undefined4 *)(&DAT_0069d424 + iVar2) = uVar4;
  return bVar7;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
