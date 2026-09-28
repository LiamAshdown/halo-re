// hwreq_d3dcaps_field_resolve  (Ghidra: hwreq_d3dcaps_field_resolve, already named)
// address 0x578ff0, size 1712 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: verified against the raw disassembly (objdump -M intel -d --start-address=0x578ff0
// --stop-address=0x579690 bin/halo.exe): every `mov edx,<addr>` before each call to
// hwreq_token_match_keyword (0x578fa0), and every `mov esi,[edi+off]` field load that follows a
// successful match, were read directly, then the addr resolved against .rdata and the off
// matched against d3d_caps9 (types/rasterizer.h) with parser->caps at hwreq_parser+0x508. All 42
// D3DCAPS9 fields (Caps..PixelShaderVersion) plus cpuspeed/ram/videoram/subsysid/revision/guid/
// driver/os check out exactly, including the documented holes (MaxTextureHeight, the guard-band
// floats, MaxPointSize are skipped).
// register convention: parser in EAX.
// UNSURE, important: this function and hwreq_parser_evaluate_condition (0x579690) are ONE
// physical routine in the binary -- 0x579690 is not a call target (its pack shows callers=0) and
// every successful match here falls straight through into the operator-parsing code at 0x579690
// with no `ret` in between; the only independent exit from this function's own code is the
// "no keyword matched" case, which the disassembly shows jumping directly into the *epilogue* of
// the 0x579690 routine (returning the string at 0x672400, "Unknown value"). This rewrite makes
// that fallthrough an explicit call to hwreq_parser_evaluate_condition, which is behaviourally
// identical and lets each Ghidra function address get its own C function as the task requires.
// The 42-entry field table replaces 42 near-identical copy-pasted if/else arms; match order and
// per-field behaviour are unchanged (hwreq_token_match_keyword has no side effects beyond the
// peek, so checking the same keywords via a loop instead of nested ifs is observably identical).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern uint32_t hwreq_token_match_keyword(const char *keyword, hwreq_parser *parser); // 0x00578fa0
extern const char *hwreq_parser_evaluate_condition(hwreq_parser *parser, int32_t kind,
                                                     uint32_t value); // 0x00579690

typedef struct d3dcaps_field_entry {
    const char *keyword;
    uint32_t caps_offset; // byte offset within d3d_caps9 (parser->caps)
} d3dcaps_field_entry;

// Order matches the disassembly exactly (Caps at caps+0x08 through PixelShaderVersion at
// caps+0xcc); see d3d_caps9 in types/rasterizer.h for each field's name.
static const d3dcaps_field_entry k_d3dcaps_fields[] = {
    { "Caps", 0x08 }, { "Caps2", 0x0c }, { "Caps3", 0x10 },
    { "PresentationIntervals", 0x14 }, { "CursorCaps", 0x18 }, { "DevCaps", 0x1c },
    { "PrimitiveMiscCaps", 0x20 }, { "RasterCaps", 0x24 }, { "ZCmpCaps", 0x28 },
    { "SrcBlendCaps", 0x2c }, { "DestBlendCaps", 0x30 }, { "AlphaCmpCaps", 0x34 },
    { "ShadeCaps", 0x38 }, { "TextureCaps", 0x3c }, { "TextureFilterCaps", 0x40 },
    { "CubeTextureFilterCaps", 0x44 }, { "VolumeTextureFilterCaps", 0x48 },
    { "TextureAddressCaps", 0x4c }, { "VolumeTextureAddressCaps", 0x50 }, { "LineCaps", 0x54 },
    { "MaxTextureWidth", 0x58 }, { "MaxVolumeExtent", 0x60 }, { "MaxTextureRepeat", 0x64 },
    { "MaxTextureAspectRatio", 0x68 }, { "MaxAnisotropy", 0x6c }, { "StencilCaps", 0x88 },
    { "FVFCaps", 0x8c }, { "TextureOpCaps", 0x90 }, { "MaxTextureBlendStages", 0x94 },
    { "MaxSimultaneousTextures", 0x98 }, { "VertexProcessingCaps", 0x9c },
    { "MaxActiveLights", 0xa0 }, { "MaxUserClipPlanes", 0xa4 },
    { "MaxVertexBlendMatrices", 0xa8 }, { "MaxVertexBlendMatrixIndex", 0xac },
    { "MaxPrimitiveCount", 0xb4 }, { "MaxVertexIndex", 0xb8 }, { "MaxStreams", 0xbc },
    { "MaxStreamStride", 0xc0 }, { "VertexShaderVersion", 0xc4 },
    { "MaxVertexShaderConst", 0xc8 }, { "PixelShaderVersion", 0xcc }
};
#define k_d3dcaps_field_count (sizeof(k_d3dcaps_fields) / sizeof(k_d3dcaps_fields[0]))

static uint32_t hwreq_cstr_length(const char *s)
{
    const char *p = s;
    while (*p != '\0') p++;
    return (uint32_t)(p - s);
}

// Matches the current token against the set of recognized D3DCAPS9 (and cpuspeed) field names
// used by the hardware-requirements script, resolving it to an internal field descriptor for a
// subsequent comparison, then parses and evaluates that comparison via
// hwreq_parser_evaluate_condition.
const char *hwreq_d3dcaps_field_resolve(hwreq_parser *parser)
{
    uint32_t i;
    int32_t kind;
    uint32_t value;
    const uint8_t *caps_base;

    while (*(char *)parser->cursor == ' ' || *(char *)parser->cursor == '\t') {
        parser->cursor++;
    }

    kind = k_hwreq_condition_value;

    if (hwreq_token_match_keyword("cpuspeed", parser)) {
        value = parser->cpu_speed;
        parser->cursor += hwreq_cstr_length("cpuspeed");
        return hwreq_parser_evaluate_condition(parser, kind, value);
    }
    if (hwreq_token_match_keyword("ram", parser)) {
        value = parser->memory;
        parser->cursor += hwreq_cstr_length("ram");
        return hwreq_parser_evaluate_condition(parser, kind, value);
    }

    caps_base = (const uint8_t *)&parser->caps;
    for (i = 0; i < k_d3dcaps_field_count; i++) {
        if (hwreq_token_match_keyword(k_d3dcaps_fields[i].keyword, parser)) {
            value = *(const uint32_t *)(caps_base + k_d3dcaps_fields[i].caps_offset);
            parser->cursor += hwreq_cstr_length(k_d3dcaps_fields[i].keyword);
            return hwreq_parser_evaluate_condition(parser, kind, value);
        }
    }

    if (hwreq_token_match_keyword("videoram", parser)) {
        value = parser->video_memory;
        parser->cursor += hwreq_cstr_length("videoram");
        return hwreq_parser_evaluate_condition(parser, kind, value);
    }
    if (hwreq_token_match_keyword("subsysid", parser)) {
        value = parser->adapter.subsystem_id;
        parser->cursor += hwreq_cstr_length("subsysid");
        return hwreq_parser_evaluate_condition(parser, kind, value);
    }
    if (hwreq_token_match_keyword("revision", parser)) {
        value = parser->adapter.revision;
        parser->cursor += hwreq_cstr_length("revision");
        return hwreq_parser_evaluate_condition(parser, kind, value);
    }
    if (hwreq_token_match_keyword("guid", parser)) {
        parser->cursor += hwreq_cstr_length("guid");
        return hwreq_parser_evaluate_condition(parser, k_hwreq_condition_guid, 0);
    }
    if (hwreq_token_match_keyword("driver", parser)) {
        parser->cursor += hwreq_cstr_length("driver");
        return hwreq_parser_evaluate_condition(parser, k_hwreq_condition_driver, 0);
    }
    if (hwreq_token_match_keyword("os", parser)) {
        // Detects the running OS family/build and maps it to a hwreq_os value; the RHS keyword
        // (e.g. "winxp") is parsed and compared against this by hwreq_parser_evaluate_condition.
        os_version_info_a version;
        uint32_t detected;

        version.size = 0x94;
        GetVersionExA((LPOSVERSIONINFOA)&version);

        // Verified against objdump 0x579628..0x57967f: NT (platform_id==2) picks winxp/win2k by
        // build number when major_version==5, else defaults to winxp; Win9x (platform_id!=2)
        // buckets dwBuildNumber's low 16 bits against the real Win95/98/98SE/ME build numbers.
        if (version.platform_id == 2 /* VER_PLATFORM_WIN32_NT */) {
            if (version.major_version == 5) {
                detected = (version.build_number >= 0xa28) ? k_hwreq_os_winxp : k_hwreq_os_win2k;
            } else {
                detected = k_hwreq_os_winxp;
            }
        } else {
            uint32_t build = version.build_number & 0xffff;
            if (build > 0x8ae) detected = k_hwreq_os_winme;         // > 2222
            else if (build > 0x7ce) detected = k_hwreq_os_win98se;  // 1999..2222
            else if (build > 0x3b6) detected = k_hwreq_os_win98;    // 951..1998
            else detected = k_hwreq_os_win95;                       // <= 950
        }

        parser->cursor += hwreq_cstr_length("os");
        return hwreq_parser_evaluate_condition(parser, k_hwreq_condition_os, detected);
    }

    return "Unknown value";
}

#if 0
Original Ghidra decompilation (0x578ff0):


char * hwreq_d3dcaps_field_resolve(void)

{
  short sVar1;
  int iVar2;
  char cVar3;
  int in_EAX;
  int iVar4;
  short *psVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  byte *pbVar11;
  byte *pbVar12;
  bool bVar13;
  bool bVar14;
  int iStack_a4;
  int iStack_a0;
  int iStack_9c;
  int iStack_98;
  _OSVERSIONINFOA local_94;
  
  uVar10 = 0;
  iVar8 = 0;
  while( true ) {
    cVar3 = **(char **)(in_EAX + 8);
    if ((cVar3 != ' ') && (cVar3 != '\t')) break;
    *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
  }
  cVar3 = hwreq_token_match_keyword();
  if (cVar3 == '\0') {
    cVar3 = hwreq_token_match_keyword();
    if (cVar3 == '\0') {
      cVar3 = hwreq_token_match_keyword();
      if (cVar3 == '\0') {
        cVar3 = hwreq_token_match_keyword();
        if (cVar3 == '\0') {
          cVar3 = hwreq_token_match_keyword();
          if (cVar3 == '\0') {
            cVar3 = hwreq_token_match_keyword();
            if (cVar3 == '\0') {
              cVar3 = hwreq_token_match_keyword();
              if (cVar3 == '\0') {
                cVar3 = hwreq_token_match_keyword();
                if (cVar3 == '\0') {
                  cVar3 = hwreq_token_match_keyword();
                  if (cVar3 == '\0') {
                    cVar3 = hwreq_token_match_keyword();
                    if (cVar3 == '\0') {
                      cVar3 = hwreq_token_match_keyword();
                      if (cVar3 == '\0') {
                        cVar3 = hwreq_token_match_keyword();
                        if (cVar3 == '\0') {
                          cVar3 = hwreq_token_match_keyword();
                          if (cVar3 == '\0') {
                            cVar3 = hwreq_token_match_keyword();
                            if (cVar3 == '\0') {
                              cVar3 = hwreq_token_match_keyword();
                              if (cVar3 == '\0') {
                                cVar3 = hwreq_token_match_keyword();
                                if (cVar3 == '\0') {
                                  cVar3 = hwreq_token_match_keyword();
                                  if (cVar3 == '\0') {
                                    cVar3 = hwreq_token_match_keyword();
                                    if (cVar3 == '\0') {
                                      cVar3 = hwreq_token_match_keyword();
                                      if (cVar3 == '\0') {
                                        cVar3 = hwreq_token_match_keyword();
                                        if (cVar3 == '\0') {
                                          cVar3 = hwreq_token_match_keyword();
                                          if (cVar3 == '\0') {
                                            cVar3 = hwreq_token_match_keyword();
                                            if (cVar3 == '\0') {
                                              cVar3 = hwreq_token_match_keyword();
                                              if (cVar3 == '\0') {
                                                cVar3 = hwreq_token_match_keyword();
                                                if (cVar3 == '\0') {
                                                  cVar3 = hwreq_token_match_keyword();
                                                  if (cVar3 == '\0') {
                                                    cVar3 = hwreq_token_match_keyword();
                                                    if (cVar3 == '\0') {
                                                      cVar3 = hwreq_token_match_keyword();
                                                      if (cVar3 == '\0') {
                                                        cVar3 = hwreq_token_match_keyword();
                                                        if (cVar3 == '\0') {
                                                          cVar3 = hwreq_token_match_keyword();
                                                          if (cVar3 == '\0') {
                                                            cVar3 = hwreq_token_match_keyword();
                                                            if (cVar3 == '\0') {
                                                              cVar3 = hwreq_token_match_keyword();
                                                              if (cVar3 == '\0') {
                                                                cVar3 = hwreq_token_match_keyword();
                                                                if (cVar3 == '\0') {
                                                                  cVar3 = hwreq_token_match_keyword
                                                                                    ();
                                                                  if (cVar3 == '\0') {
                                                                    cVar3 = 
                                                  hwreq_token_match_keyword();
                                                  if (cVar3 == '\0') {
                                                    cVar3 = hwreq_token_match_keyword();
                                                    if (cVar3 == '\0') {
                                                      cVar3 = hwreq_token_match_keyword();
                                                      if (cVar3 == '\0') {
                                                        cVar3 = hwreq_token_match_keyword();
                                                        if (cVar3 == '\0') {
                                                          cVar3 = hwreq_token_match_keyword();
                                                          if (cVar3 == '\0') {
                                                            cVar3 = hwreq_token_match_keyword();
                                                            if (cVar3 == '\0') {
                                                              cVar3 = hwreq_token_match_keyword();
                                                              if (cVar3 == '\0') {
                                                                cVar3 = hwreq_token_match_keyword();
                                                                if (cVar3 == '\0') {
                                                                  cVar3 = hwreq_token_match_keyword
                                                                                    ();
                                                                  if (cVar3 == '\0') {
                                                                    cVar3 = 
                                                  hwreq_token_match_keyword();
                                                  if (cVar3 == '\0') {
                                                    cVar3 = hwreq_token_match_keyword();
                                                    if (cVar3 == '\0') {
                                                      cVar3 = hwreq_token_match_keyword();
                                                      if (cVar3 == '\0') {
                                                        cVar3 = hwreq_token_match_keyword();
                                                        if (cVar3 == '\0') {
                                                          cVar3 = hwreq_token_match_keyword();
                                                          if (cVar3 == '\0') {
                                                            cVar3 = hwreq_token_match_keyword();
                                                            if (cVar3 == '\0') {
                                                              cVar3 = hwreq_token_match_keyword();
                                                              if (cVar3 == '\0') {
                                                                cVar3 = hwreq_token_match_keyword();
                                                                if (cVar3 == '\0') {
                                                                  return "Unknown value";
                                                                }
                                                                local_94.dwOSVersionInfoSize = 0x94;
                                                                GetVersionExA(&local_94);
                                                                if (local_94.dwPlatformId == 2) {
                                                                  uVar10 = 5;
                                                                  if ((local_94.dwMajorVersion == 5)
                                                                     && (local_94.dwBuildNumber <
                                                                         0xa28)) {
                                                                    uVar10 = 4;
                                                                  }
                                                                }
                                                                else {
                                                                  uVar9 = local_94.dwBuildNumber &
                                                                          0xffff;
                                                                  uVar10 = 3;
                                                                  if (uVar9 < 0x8af) {
                                                                    uVar10 = 2;
                                                                  }
                                                                  if (uVar9 < 1999) {
                                                                    uVar10 = 1;
                                                                  }
                                                                  if (uVar9 < 0x3b7) {
                                                                    uVar10 = 0;
                                                                  }
                                                                }
                                                                iVar8 = 3;
                                                                iVar4 = *(int *)(in_EAX + 8) + 2;
                                                              }
                                                              else {
                                                                iVar8 = 2;
                                                                iVar4 = *(int *)(in_EAX + 8) + 6;
                                                              }
                                                            }
                                                            else {
                                                              iVar8 = 1;
                                                              iVar4 = *(int *)(in_EAX + 8) + 4;
                                                            }
                                                          }
                                                          else {
                                                            uVar10 = *(uint *)(in_EAX + 0x4f0);
                                                            iVar4 = *(int *)(in_EAX + 8) + 8;
                                                          }
                                                        }
                                                        else {
                                                          uVar10 = *(uint *)(in_EAX + 0x4ec);
                                                          iVar4 = *(int *)(in_EAX + 8) + 8;
                                                        }
                                                      }
                                                      else {
                                                        uVar10 = *(uint *)(in_EAX + 0xb8);
                                                        iVar4 = *(int *)(in_EAX + 8) + 8;
                                                      }
                                                    }
                                                    else {
                                                      uVar10 = *(uint *)(in_EAX + 0x5d4);
                                                      iVar4 = *(int *)(in_EAX + 8) + 0x12;
                                                    }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5d0);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0x14;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5cc);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0x13;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5c8);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0xf;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5c4);
                                                    iVar4 = *(int *)(in_EAX + 8) + 10;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5c0);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0xe;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5bc);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0x11;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5b4);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0x19;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5b0);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0x16;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5ac);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0x11;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5a8);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0xf;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5a4);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0x14;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x5a0);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0x17;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x59c);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0x15;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x598);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0xd;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x594);
                                                    iVar4 = *(int *)(in_EAX + 8) + 7;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x590);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0xb;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x574);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0xd;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x570);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0x15;
                                                  }
                                                  }
                                                  else {
                                                    uVar10 = *(uint *)(in_EAX + 0x56c);
                                                    iVar4 = *(int *)(in_EAX + 8) + 0x10;
                                                  }
                                                }
                                                else {
                                                  uVar10 = *(uint *)(in_EAX + 0x568);
                                                  iVar4 = *(int *)(in_EAX + 8) + 0xf;
                                                }
                                              }
                                              else {
                                                uVar10 = *(uint *)(in_EAX + 0x560);
                                                iVar4 = *(int *)(in_EAX + 8) + 0xf;
                                              }
                                            }
                                            else {
                                              uVar10 = *(uint *)(in_EAX + 0x55c);
                                              iVar4 = *(int *)(in_EAX + 8) + 8;
                                            }
                                          }
                                          else {
                                            uVar10 = *(uint *)(in_EAX + 0x558);
                                            iVar4 = *(int *)(in_EAX + 8) + 0x18;
                                          }
                                        }
                                        else {
                                          uVar10 = *(uint *)(in_EAX + 0x554);
                                          iVar4 = *(int *)(in_EAX + 8) + 0x12;
                                        }
                                      }
                                      else {
                                        uVar10 = *(uint *)(in_EAX + 0x550);
                                        iVar4 = *(int *)(in_EAX + 8) + 0x17;
                                      }
                                    }
                                    else {
                                      uVar10 = *(uint *)(in_EAX + 0x54c);
                                      iVar4 = *(int *)(in_EAX + 8) + 0x15;
                                    }
                                  }
                                  else {
                                    uVar10 = *(uint *)(in_EAX + 0x548);
                                    iVar4 = *(int *)(in_EAX + 8) + 0x11;
                                  }
                                }
                                else {
                                  uVar10 = *(uint *)(in_EAX + 0x544);
                                  iVar4 = *(int *)(in_EAX + 8) + 0xb;
                                }
                              }
                              else {
                                uVar10 = *(uint *)(in_EAX + 0x540);
                                iVar4 = *(int *)(in_EAX + 8) + 9;
                              }
                            }
                            else {
                              uVar10 = *(uint *)(in_EAX + 0x53c);
                              iVar4 = *(int *)(in_EAX + 8) + 0xc;
                            }
                          }
                          else {
                            uVar10 = *(uint *)(in_EAX + 0x538);
                            iVar4 = *(int *)(in_EAX + 8) + 0xd;
                          }
                        }
                        else {
                          uVar10 = *(uint *)(in_EAX + 0x534);
                          iVar4 = *(int *)(in_EAX + 8) + 0xc;
                        }
                      }
                      else {
                        uVar10 = *(uint *)(in_EAX + 0x530);
                        iVar4 = *(int *)(in_EAX + 8) + 8;
                      }
                    }
                    else {
                      uVar10 = *(uint *)(in_EAX + 0x52c);
                      iVar4 = *(int *)(in_EAX + 8) + 10;
                    }
                  }
                  else {
                    uVar10 = *(uint *)(in_EAX + 0x528);
                    iVar4 = *(int *)(in_EAX + 8) + 0x11;
                  }
                }
                else {
                  uVar10 = *(uint *)(in_EAX + 0x524);
                  iVar4 = *(int *)(in_EAX + 8) + 7;
                }
              }
              else {
                uVar10 = *(uint *)(in_EAX + 0x520);
                iVar4 = *(int *)(in_EAX + 8) + 10;
              }
            }
            else {
              uVar10 = *(uint *)(in_EAX + 0x51c);
              iVar4 = *(int *)(in_EAX + 8) + 0x15;
            }
          }
          else {
            uVar10 = *(uint *)(in_EAX + 0x518);
            iVar4 = *(int *)(in_EAX + 8) + 5;
          }
        }
        else {
          uVar10 = *(uint *)(in_EAX + 0x514);
          iVar4 = *(int *)(in_EAX + 8) + 5;
        }
      }
      else {
        uVar10 = *(uint *)(in_EAX + 0x510);
        iVar4 = *(int *)(in_EAX + 8) + 4;
      }
    }
    else {
      uVar10 = *(uint *)(in_EAX + 0xb4);
      iVar4 = *(int *)(in_EAX + 8) + 3;
    }
  }
  else {
    uVar10 = *(uint *)(in_EAX + 0xb0);
    iVar4 = *(int *)(in_EAX + 8) + 8;
  }
  *(int *)(in_EAX + 8) = iVar4;
  while( true ) {
    cVar3 = **(char **)(in_EAX + 8);
    if ((cVar3 != ' ') && (cVar3 != '\t')) break;
    *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
  }
  psVar5 = *(short **)(in_EAX + 8);
  sVar1 = *psVar5;
  if (sVar1 == 0x3d3d) {
    psVar5 = psVar5 + 1;
    uVar9 = 0;
  }
  else if (sVar1 == 0x3d21) {
    psVar5 = psVar5 + 1;
    uVar9 = 1;
  }
  else if (sVar1 == 0x3e3c) {
    psVar5 = psVar5 + 1;
    uVar9 = 1;
  }
  else if (sVar1 == 0x3e3d) {
    psVar5 = psVar5 + 1;
    uVar9 = 4;
  }
  else if (sVar1 == 0x3c3d) {
    psVar5 = psVar5 + 1;
    uVar9 = 5;
  }
  else if (sVar1 == 0x3d3c) {
    psVar5 = psVar5 + 1;
    uVar9 = 5;
  }
  else if (sVar1 == 0x3d3e) {
    psVar5 = psVar5 + 1;
    uVar9 = 4;
  }
  else {
    cVar3 = (char)*psVar5;
    if (cVar3 == '=') {
      psVar5 = (short *)((int)psVar5 + 1);
      uVar9 = 0;
    }
    else {
      if (cVar3 == '>') {
        uVar9 = 2;
      }
      else if (cVar3 == '<') {
        uVar9 = 3;
      }
      else {
        if (cVar3 != '&') {
          return "Unknown operator";
        }
        uVar9 = 6;
      }
      psVar5 = (short *)((int)psVar5 + 1);
    }
  }
  *(short **)(in_EAX + 8) = psVar5;
  while( true ) {
    cVar3 = **(char **)(in_EAX + 8);
    if ((cVar3 != ' ') && (cVar3 != '\t')) break;
    *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
  }
  if (iVar8 == 1) {
    if (1 < uVar9) {
      return "Only == or != allowed";
    }
    iVar8 = hwreq_token_parse_hex_id();
    if ((iVar8 != -1) && (iStack_a4 = hwreq_token_parse_hex_id(), iStack_a4 != -1)) {
      iStack_a4 = iVar8 * 0x10000 + iStack_a4;
      cVar3 = **(char **)(in_EAX + 8);
      *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
      if ((cVar3 == '-') &&
         (((iStack_a0 = hwreq_token_parse_hex_id(), iStack_a0 != -1 &&
           (cVar3 = **(char **)(in_EAX + 8), *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1,
           cVar3 == '-')) && (iVar8 = hwreq_token_parse_hex_id(), iVar8 != -1)))) {
        iStack_a0 = iVar8 * 0x10000 + iStack_a0;
        cVar3 = **(char **)(in_EAX + 8);
        *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
        if ((((cVar3 == '-') && (iStack_9c = hwreq_token_parse_hex_id_byteswap(), iStack_9c != -1))
            && (cVar3 = **(char **)(in_EAX + 8), *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1
               , cVar3 == '-')) && (iVar8 = hwreq_token_parse_hex_id_byteswap(), iVar8 != -1)) {
          iStack_9c = iVar8 * 0x10000 + iStack_9c;
          iVar8 = hwreq_token_parse_hex_id_byteswap();
          if ((iVar8 != -1) && (iVar4 = hwreq_token_parse_hex_id_byteswap(), iVar4 != -1)) {
            iStack_98 = iVar4 * 0x10000 + iVar8;
            iVar8 = 0x10;
            if (uVar9 != 0) {
              bVar13 = false;
              iVar4 = 0;
              bVar14 = true;
              pbVar11 = (byte *)&iStack_a4;
              pbVar12 = (byte *)(in_EAX + 0x4f4);
              do {
                if (iVar8 == 0) break;
                iVar8 = iVar8 + -1;
                bVar13 = *pbVar11 < *pbVar12;
                bVar14 = *pbVar11 == *pbVar12;
                pbVar11 = pbVar11 + 1;
                pbVar12 = pbVar12 + 1;
              } while (bVar14);
              if (!bVar14) {
                iVar4 = (1 - (uint)bVar13) - (uint)(bVar13 != 0);
              }
              return (char *)(uint)(iVar4 != 0);
            }
            bVar13 = false;
            iVar4 = 0;
            bVar14 = true;
            pbVar11 = (byte *)&iStack_a4;
            pbVar12 = (byte *)(in_EAX + 0x4f4);
            do {
              if (iVar8 == 0) break;
              iVar8 = iVar8 + -1;
              bVar13 = *pbVar11 < *pbVar12;
              bVar14 = *pbVar11 == *pbVar12;
              pbVar11 = pbVar11 + 1;
              pbVar12 = pbVar12 + 1;
            } while (bVar14);
            if (!bVar14) {
              iVar4 = (1 - (uint)bVar13) - (uint)(bVar13 != 0);
            }
            return (char *)(uint)(iVar4 == 0);
          }
        }
      }
    }
    return "Invalid GUID";
  }
  if (iVar8 == 2) {
    iVar8 = hwreq_token_parse_number();
    if ((iVar8 == -1) ||
       (cVar3 = **(char **)(in_EAX + 8), *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1,
       cVar3 != '.')) {
LAB_005799f7:
      return "Invalid driver number";
    }
    iVar4 = hwreq_token_parse_number();
    cVar3 = **(char **)(in_EAX + 8);
    iVar4 = iVar4 + iVar8 * 0x10000;
    *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
    if ((cVar3 != '.') ||
       ((iVar8 = hwreq_token_parse_number(), iVar8 == -1 ||
        (cVar3 = **(char **)(in_EAX + 8), *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1,
        cVar3 != '.')))) goto LAB_005799f7;
    iVar6 = hwreq_token_parse_number();
    uVar10 = *(uint *)(in_EAX + 0x4dc);
    iVar2 = *(int *)(in_EAX + 0x4e0);
    uVar7 = iVar6 + iVar8 * 0x10000;
    switch(uVar9) {
    case 0:
      if ((uVar10 == uVar7) && (iVar2 == iVar4)) {
        return (char *)0x1;
      }
      break;
    case 1:
      if ((uVar10 != uVar7) || (iVar2 != iVar4)) {
        return (char *)0x1;
      }
      break;
    case 2:
      if ((iVar4 <= iVar2) && ((iVar4 < iVar2 || (uVar7 < uVar10)))) {
        return (char *)0x1;
      }
      break;
    case 3:
      if ((iVar2 <= iVar4) && ((iVar2 < iVar4 || (uVar10 < uVar7)))) {
        return (char *)0x1;
      }
      break;
    case 4:
      if ((iVar4 <= iVar2) && ((iVar4 < iVar2 || (uVar7 <= uVar10)))) {
        return (char *)0x1;
      }
      break;
    case 5:
      if (iVar2 <= iVar4) {
        if (iVar2 < iVar4) {
          return (char *)0x1;
        }
        if (uVar10 <= uVar7) {
          return (char *)0x1;
        }
      }
      break;
    default:
      goto switchD_00579953_default;
    }
    return (char *)0x0;
  }
  if (iVar8 == 3) {
    cVar3 = hwreq_token_match_keyword();
    if (cVar3 == '\0') {
      cVar3 = hwreq_token_match_keyword();
      if (cVar3 != '\0') {
        uVar7 = 1;
        goto LAB_00579a20;
      }
      cVar3 = hwreq_token_match_keyword();
      if (cVar3 == '\0') {
        cVar3 = hwreq_token_match_keyword();
        if (cVar3 == '\0') {
          cVar3 = hwreq_token_match_keyword();
          if (cVar3 == '\0') {
            cVar3 = hwreq_token_match_keyword();
            if (cVar3 == '\0') {
              return "Unknown OS";
            }
            uVar7 = 5;
            iVar8 = *(int *)(in_EAX + 8) + 5;
            goto LAB_00579a26;
          }
          uVar7 = 4;
        }
        else {
          uVar7 = 3;
        }
        goto LAB_00579a20;
      }
      uVar7 = 2;
      iVar8 = *(int *)(in_EAX + 8) + 7;
    }
    else {
      uVar7 = 0;
LAB_00579a20:
      iVar8 = *(int *)(in_EAX + 8) + 5;
    }
LAB_00579a26:
    *(int *)(in_EAX + 8) = iVar8;
  }
  else {
    uVar7 = hwreq_token_parse_number();
    if (uVar7 == 0xffffffff) {
      return "Number expected";
    }
  }
  switch(uVar9) {
  case 0:
    return (char *)(uint)(uVar10 == uVar7);
  case 1:
    return (char *)(uint)(uVar10 != uVar7);
  case 2:
    return (char *)(uint)(uVar7 < uVar10);
  case 3:
    return (char *)(uint)(uVar10 < uVar7);
  case 4:
    return (char *)(uint)(uVar7 <= uVar10);
  case 5:
    return (char *)(uint)(uVar10 <= uVar7);
  case 6:
    return (char *)(uint)((uVar7 & uVar10) != 0);
  }
switchD_00579953_default:
  return "Invalid";
}
#endif
