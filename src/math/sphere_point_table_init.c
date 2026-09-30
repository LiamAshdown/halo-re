// sphere_point_table_init  (Ghidra: sphere_point_table_init, already named)
// address 0x4cd0e0, size 131 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: out/phase4/math_types_notes.md "sphere_mesh" and "globals owned by the module":
//   builds sphere_point_table / sphere_point_table_count from a generated sphere_mesh, and
//   calls sphere_mesh_generate @0x4ca4b0 with subdivisions = 16 (mov eax, 0x10 at 0x4cd0ec,
//   register-passed and therefore invisible in the Ghidra call).
// register convention: __cdecl, no arguments.

#include "win32.h"
#include "tags.h"
#include "math.h"
#include "fn_math.h"


extern random_seed effect_random_seed;               // 0x00719cd4
extern real_point3d *sphere_point_table;             // 0x006b7af4
extern int16_t sphere_point_table_count;             // 0x006b7af8

// Initializes the global table of quasi-uniform unit-sphere sample points used for randomized
// directions: builds a 16-subdivision sphere mesh, copies its point array out into its own
// GlobalAlloc'd buffer, and frees the mesh (header, points and index buffer).
void sphere_point_table_init(void)
{
    sphere_mesh *mesh;
    real_point3d *points;
    int16_t i;

    effect_random_seed = random_seed_generate();
    mesh = sphere_mesh_generate(k_sphere_point_table_subdivisions);

    points = (real_point3d *)GlobalAlloc(0, (uint32_t)mesh->point_count * sizeof(real_point3d));
    sphere_point_table_count = mesh->point_count;
    sphere_point_table = points;

    for (i = 0; i < sphere_point_table_count; i++) {
        points[i] = mesh->points[i];
    }

    GlobalFree(mesh->points);
    GlobalFree(mesh->indices);
    GlobalFree(mesh);
}

#if 0
Original Ghidra decompilation (0x4cd0e0):

void __cdecl sphere_point_table_init(void)

{
  HGLOBAL hMem;
  HGLOBAL pvVar1;
  undefined4 *puVar2;
  short sVar3;
  undefined4 *puVar4;

  DAT_00719cd4 = random_seed_generate();
  hMem = (HGLOBAL)sphere_mesh_generate();
  pvVar1 = GlobalAlloc(0,*(short *)((int)hMem + 0xc) * 0xc);
  DAT_006b7af8 = *(short *)((int)hMem + 0xc);
  sVar3 = 0;
  DAT_006b7af4 = pvVar1;
  if (0 < DAT_006b7af8) {
    do {
      puVar4 = (undefined4 *)(*(int *)((int)hMem + 4) + sVar3 * 0xc);
      puVar2 = (undefined4 *)(sVar3 * 0xc + (int)pvVar1);
      *puVar2 = *puVar4;
      puVar2[1] = puVar4[1];
      sVar3 = sVar3 + 1;
      puVar2[2] = puVar4[2];
    } while (sVar3 < *(short *)((int)hMem + 0xc));
  }
  GlobalFree(*(HGLOBAL *)((int)hMem + 4));
  GlobalFree(*(HGLOBAL *)((int)hMem + 8));
  GlobalFree(hMem);
  return;
}
#endif
