// rasterizer_dx9_vertex_declarations_release  (Ghidra: rasterizer_dx9_vertex_declarations_release, already named)
// address 0x530540, size 47 bytes
// name confidence: 0.65   rewrite confidence: 0.85
// evidence: functions.md summary ("Releases every vertex declaration created by
//   rasterizer_dx9_vertex_declarations_create and clears the table"); loop bound
//   0x006e1a90..0x006e1b80 is exactly k_rasterizer_vertex_type_count (20) declarations of 0xc bytes.
// register convention: none -- __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <string.h> // memset

extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90

// Releases every vertex declaration created by rasterizer_dx9_vertex_declarations_create and
// clears the table.
void rasterizer_dx9_vertex_declarations_release(void)
{
    int i;

    for (i = 0; i < k_rasterizer_vertex_type_count; i++) {
        void *declaration = (void *)rasterizer_vertex_declarations[i].declaration;
        if (declaration != 0) {
            ((void (__stdcall *)(void *))(*(void ***)declaration)[2])(declaration); // Release()
        }
    }
    memset(rasterizer_vertex_declarations, 0, sizeof(rasterizer_vertex_declarations));
}

#if 0
Original Ghidra decompilation (0x530540):

void __cdecl rasterizer_dx9_vertex_declarations_release(void)

{
  int *piVar1;
  int iVar2;
  int *piVar3;
  undefined4 *puVar4;

  piVar3 = &DAT_006e1a90;
  do {
    piVar1 = (int *)*piVar3;
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
    }
    piVar3 = piVar3 + 3;
  } while ((int)piVar3 < 0x6e1b80);
  puVar4 = &DAT_006e1a90;
  for (iVar2 = 0x3c; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  return;
}
#endif
