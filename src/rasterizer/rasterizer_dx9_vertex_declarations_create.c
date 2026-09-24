// rasterizer_dx9_vertex_declarations_create  (Ghidra: rasterizer_dx9_vertex_declarations_create, already named)
// address 0x5301b0, size 897 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: functions.md summary; every 0x006e1aXX/0x006e1bXX global lands exactly on the
//   declaration/fvf/usage fields of rasterizer_vertex_declarations[0..19] (checked by address
//   arithmetic against types/rasterizer.h), one CreateVertexDeclaration (device +0x158) call per
//   slot (several slots share the same D3DVERTEXELEMENT9 array).
// register convention: none -- __cdecl, no arguments.
// Phase 4 review: 0x583dca is the D3DX import D3DXFVFFromDeclarator; it writes the FVF of the
//   processed model elements into declarations[15].fvf (+4), next to the literal FVFs 0x144 / 0x1c4
//   of declarations 17 / 18. The element arrays are typed d3d_vertex_element9 (types/rasterizer.h).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <string.h> // memset

extern void *rasterizer_device; // 0x0071d174
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern d3d_caps9 rasterizer_caps; // 0x007c10c0

// The D3DVERTEXELEMENT9 arrays in .rdata that describe each vertex declaration (declaration 19 is
// conditional); passed straight through to CreateVertexDeclaration.
extern const d3d_vertex_element9 vertex_elements_environment_uncompressed[];  // 0x0065e308 (declarations 0, 1)
extern const d3d_vertex_element9 vertex_elements_environment_lightmap[];      // 0x0065e2a0 (declarations 2, 3)
extern const d3d_vertex_element9 vertex_elements_model[];                     // 0x0065e220 (declarations 4, 5)
extern const d3d_vertex_element9 vertex_elements_dynamic[];                   // 0x0065e1d8 (declarations 6, 7)
extern const d3d_vertex_element9 vertex_elements_dynamic_screen[];             // 0x0065e1b8 (declaration 8)
extern const d3d_vertex_element9 vertex_elements_debug[];                      // 0x0065e168 (declaration 9)
extern const d3d_vertex_element9 vertex_elements_decal[];                      // 0x0065e180 (declaration 10)
extern const d3d_vertex_element9 vertex_elements_detail_object[];              // 0x0065e198 (declaration 11)
extern const d3d_vertex_element9 vertex_elements_environment_uncompressed_ff[];  // 0x0065e338 (declaration 12)
extern const d3d_vertex_element9 vertex_elements_environment_lightmap_ff[];    // 0x0065e2e0 (declaration 13)
extern const d3d_vertex_element9 vertex_elements_model_ff[];                   // 0x0065e260 (declaration 14)
extern const d3d_vertex_element9 vertex_elements_model_processed[];            // 0x0065e280 (declaration 15)
extern const d3d_vertex_element9 vertex_elements_unlit_zsprite[];              // 0x0065e1f8 (declaration 16)
extern const d3d_vertex_element9 vertex_elements_screen_transformed_lit[];     // 0x0065e380 (declaration 17)
extern const d3d_vertex_element9 vertex_elements_screen_transformed_lit_specular[]; // 0x0065e3a0 (declaration 18)
extern const d3d_vertex_element9 vertex_elements_environment_single_stream_ff[]; // 0x0065e358 (declaration 19, conditional)

typedef int32_t (*d3d_create_vertex_declaration_fn)(void *device, const void *elements, void **out_declaration);
extern int32_t D3DXFVFFromDeclarator(const d3d_vertex_element9 *elements, uint32_t *out_fvf); // 0x583dca D3DX

// Creates the full set of Direct3D vertex declarations (and their per-format stride/usage
// constants) used by every geometry draw path in the rasterizer, returning false if any creation
// call fails.
uint8_t rasterizer_dx9_vertex_declarations_create(void)
{
    void **vt;
    d3d_create_vertex_declaration_fn create_decl;
    int32_t hr[19];
    uint8_t ok;
    int i;

    memset(rasterizer_vertex_declarations, 0, sizeof(rasterizer_vertex_declarations));

    vt = *(void ***)rasterizer_device;
    create_decl = (d3d_create_vertex_declaration_fn)vt[0x158 / 4];

    hr[0] = create_decl(rasterizer_device, vertex_elements_environment_uncompressed,
                        (void **)&rasterizer_vertex_declarations[0].declaration);
    hr[1] = create_decl(rasterizer_device, vertex_elements_environment_uncompressed,
                        (void **)&rasterizer_vertex_declarations[1].declaration);
    hr[2] = create_decl(rasterizer_device, vertex_elements_environment_lightmap,
                        (void **)&rasterizer_vertex_declarations[2].declaration);
    hr[3] = create_decl(rasterizer_device, vertex_elements_environment_lightmap,
                        (void **)&rasterizer_vertex_declarations[3].declaration);
    hr[4] = create_decl(rasterizer_device, vertex_elements_model,
                        (void **)&rasterizer_vertex_declarations[4].declaration);
    hr[5] = create_decl(rasterizer_device, vertex_elements_model,
                        (void **)&rasterizer_vertex_declarations[5].declaration);
    hr[6] = create_decl(rasterizer_device, vertex_elements_dynamic,
                        (void **)&rasterizer_vertex_declarations[6].declaration);
    hr[7] = create_decl(rasterizer_device, vertex_elements_dynamic,
                        (void **)&rasterizer_vertex_declarations[7].declaration);
    hr[8] = create_decl(rasterizer_device, vertex_elements_dynamic_screen,
                        (void **)&rasterizer_vertex_declarations[8].declaration);
    hr[9] = create_decl(rasterizer_device, vertex_elements_debug,
                        (void **)&rasterizer_vertex_declarations[9].declaration);
    hr[10] = create_decl(rasterizer_device, vertex_elements_decal,
                        (void **)&rasterizer_vertex_declarations[10].declaration);
    hr[11] = create_decl(rasterizer_device, vertex_elements_detail_object,
                        (void **)&rasterizer_vertex_declarations[11].declaration);
    hr[12] = create_decl(rasterizer_device, vertex_elements_environment_uncompressed_ff,
                        (void **)&rasterizer_vertex_declarations[12].declaration);
    hr[13] = create_decl(rasterizer_device, vertex_elements_environment_lightmap_ff,
                        (void **)&rasterizer_vertex_declarations[13].declaration);
    hr[14] = create_decl(rasterizer_device, vertex_elements_model_ff,
                        (void **)&rasterizer_vertex_declarations[14].declaration);
    hr[15] = create_decl(rasterizer_device, vertex_elements_model_processed,
                        (void **)&rasterizer_vertex_declarations[15].declaration);
    hr[16] = create_decl(rasterizer_device, vertex_elements_unlit_zsprite,
                        (void **)&rasterizer_vertex_declarations[16].declaration);
    hr[17] = create_decl(rasterizer_device, vertex_elements_screen_transformed_lit,
                        (void **)&rasterizer_vertex_declarations[17].declaration);
    hr[18] = create_decl(rasterizer_device, vertex_elements_screen_transformed_lit_specular,
                        (void **)&rasterizer_vertex_declarations[18].declaration);

    ok = 1;
    for (i = 0; i < 19; i++) {
        if (hr[i] < 0) {
            ok = 0;
        }
    }

    rasterizer_vertex_declarations[14].usage = 8;
    rasterizer_vertex_declarations[13].usage = 8;
    rasterizer_vertex_declarations[12].usage = 8;
    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        rasterizer_vertex_declarations[6].usage = 0x218;
        rasterizer_vertex_declarations[7].usage = 0x218;
        rasterizer_vertex_declarations[8].usage = 0x218;
        rasterizer_vertex_declarations[9].usage = 0x218;
        rasterizer_vertex_declarations[0].usage = 0x18;
        rasterizer_vertex_declarations[1].usage = 0x18;
        rasterizer_vertex_declarations[2].usage = 0x18;
        rasterizer_vertex_declarations[3].usage = 0x18;
        rasterizer_vertex_declarations[4].usage = 0x18;
        rasterizer_vertex_declarations[5].usage = 0x18;
        rasterizer_vertex_declarations[10].usage = 0x18;
        rasterizer_vertex_declarations[11].usage = 0x18;
        rasterizer_vertex_declarations[16].usage = 0x18;
    } else {
        rasterizer_vertex_declarations[0].usage = 8;
        rasterizer_vertex_declarations[1].usage = 8;
        rasterizer_vertex_declarations[2].usage = 8;
        rasterizer_vertex_declarations[3].usage = 8;
        rasterizer_vertex_declarations[4].usage = 8;
        rasterizer_vertex_declarations[5].usage = 8;
        rasterizer_vertex_declarations[6].usage = 0x208;
        rasterizer_vertex_declarations[7].usage = 0x208;
        rasterizer_vertex_declarations[8].usage = 0x208;
        rasterizer_vertex_declarations[9].usage = 0x208;
        rasterizer_vertex_declarations[10].usage = 8;
        rasterizer_vertex_declarations[11].usage = 8;
        rasterizer_vertex_declarations[16].usage = 8;
    }
    rasterizer_vertex_declarations[15].usage = 0x208;
    rasterizer_vertex_declarations[17].usage = 0x208;
    rasterizer_vertex_declarations[18].usage = 0x208;

    D3DXFVFFromDeclarator(vertex_elements_model_processed, &rasterizer_vertex_declarations[15].fvf);

    rasterizer_vertex_declarations[17].fvf = 0x144;
    rasterizer_vertex_declarations[18].fvf = 0x1c4;

    if (rasterizer_caps.max_streams < 2) {
        int32_t hr19 = create_decl(rasterizer_device, vertex_elements_environment_single_stream_ff,
                                   (void **)&rasterizer_vertex_declarations[19].declaration);
        ok = (hr19 >= 0) && ok;
        rasterizer_vertex_declarations[19].usage = 8;
    }

    return ok;
}

#if 0
Original Ghidra decompilation (0x5301b0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

bool __cdecl rasterizer_dx9_vertex_declarations_create(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  int iVar17;
  int iVar18;
  int iVar19;
  bool bVar20;
  undefined4 *puVar21;

  puVar21 = &DAT_006e1a90;
  for (iVar19 = 0x3c; iVar19 != 0; iVar19 = iVar19 + -1) {
    *puVar21 = 0;
    puVar21 = puVar21 + 1;
  }
  iVar19 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e308,&DAT_006e1a90);
  iVar1 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e308,&DAT_006e1a9c);
  iVar2 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e2a0,&DAT_006e1aa8);
  iVar3 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e2a0,&DAT_006e1ab4);
  iVar4 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e220,&DAT_006e1ac0);
  iVar5 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e220,&DAT_006e1acc);
  iVar6 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e1d8,&DAT_006e1ad8);
  iVar7 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e1d8,&DAT_006e1ae4);
  iVar8 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e1b8,&DAT_006e1af0);
  iVar9 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e168,&DAT_006e1afc);
  iVar10 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e180,&DAT_006e1b08);
  iVar11 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e198,&DAT_006e1b14);
  iVar12 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e338,&DAT_006e1b20);
  iVar13 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e2e0,&DAT_006e1b2c);
  iVar14 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e260,&DAT_006e1b38);
  iVar15 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e280,&DAT_006e1b44);
  iVar16 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e1f8,&DAT_006e1b50);
  iVar17 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e380,&DAT_006e1b5c);
  iVar18 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e3a0,&DAT_006e1b68);
  bVar20 = -1 < iVar18 &&
           (-1 < iVar17 &&
           (-1 < iVar16 &&
           (-1 < iVar15 &&
           (-1 < iVar14 &&
           (-1 < iVar13 &&
           (-1 < iVar12 &&
           (-1 < iVar11 &&
           (-1 < iVar10 &&
           (-1 < iVar9 &&
           (-1 < iVar8 &&
           (-1 < iVar7 &&
           (-1 < iVar6 &&
           (-1 < iVar5 &&
           (-1 < iVar4 && (-1 < iVar3 && (-1 < iVar2 && (-1 < iVar1 && -1 < iVar19)))))))))))))))));
  _DAT_006e1b40 = 8;
  _DAT_006e1b34 = 8;
  _DAT_006e1b28 = 8;
  if (DAT_007c118c < 0xffff0101) {
    DAT_006e1ae0 = 0x218;
    _DAT_006e1aec = 0x218;
    DAT_006e1af8 = 0x218;
    _DAT_006e1b04 = 0x218;
    DAT_006e1a98 = 0x18;
    _DAT_006e1aa4 = 0x18;
    _DAT_006e1ab0 = 0x18;
    _DAT_006e1abc = 0x18;
    _DAT_006e1ac8 = 0x18;
    _DAT_006e1ad4 = 0x18;
    DAT_006e1b10 = 0x18;
    DAT_006e1b1c = 0x18;
    DAT_006e1b58 = 0x18;
  }
  else {
    DAT_006e1a98 = 8;
    _DAT_006e1aa4 = 8;
    _DAT_006e1ab0 = 8;
    _DAT_006e1abc = 8;
    _DAT_006e1ac8 = 8;
    _DAT_006e1ad4 = 8;
    DAT_006e1ae0 = 0x208;
    _DAT_006e1aec = 0x208;
    DAT_006e1af8 = 0x208;
    _DAT_006e1b04 = 0x208;
    DAT_006e1b10 = 8;
    DAT_006e1b1c = 8;
    DAT_006e1b58 = 8;
  }
  _DAT_006e1b4c = 0x208;
  _DAT_006e1b64 = 0x208;
  _DAT_006e1b70 = 0x208;
  FUN_00583dca(&DAT_0065e280,&DAT_006e1b48);
  _DAT_006e1b60 = 0x144;
  _DAT_006e1b6c = 0x1c4;
  if (DAT_007c117c < 2) {
    iVar19 = (**(code **)(*DAT_0071d174 + 0x158))(DAT_0071d174,&DAT_0065e358,&DAT_006e1b74);
    bVar20 = -1 < iVar19 && bVar20;
    _DAT_006e1b7c = 8;
  }
  return bVar20;
}
#endif
