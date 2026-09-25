// hs_doc  (Ghidra: hs_doc, already named)
// address 0x484270, size 136 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: CEA-PDB string match ("hs_doc.txt"); writes every hs_function_definitions entry's
// formatted signature and info string to hs_doc.txt. The inline byte-copy loop that duplicates
// each info string onto the stack before the second fprintf is a compiled strcpy (info is
// fprintf'd directly the first time, so the copy has no effect beyond matching hs_help_print_
// function's identical pattern); rewritten as strcpy.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include <stdio.h>
#include <string.h>

// fopen: <stdio.h>, resolved to the game CRT at 0x624186 // 0x00624186, lib:crt (_fsopen wrapper), not this module
extern void hs_format_function_signature(int16_t function_index, char *out); // 0x00484300, this batch

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58

// Writes the signature and documentation string of every registered HS script function to
// hs_doc.txt.
void hs_doc(void)
{
    FILE *file;
    int16_t i;
    char buffer[2048];

    file = fopen("hs_doc.txt", "w");
    for (i = 0; i < k_hs_function_count; i = i + 1) {
        hs_format_function_signature(i, buffer);
        fprintf(file, "%s\r\n", buffer);
        strcpy(buffer, hs_function_definitions[i]->info);
        fprintf(file, "%s\r\n\r\n", buffer);
    }
    fclose(file);
}

#if 0
Original Ghidra decompilation (0x484270):

void __cdecl hs_doc(void)

{
  char cVar1;
  FILE *_File;
  char *pcVar2;
  char *pcVar3;
  undefined **ppuVar4;
  short sVar5;
  char local_800 [2048];

  _File = (FILE *)FUN_00624186("hs_doc.txt",0x660030);
  sVar5 = 0;
  ppuVar4 = &PTR_DAT_00688b58;
  do {
    hs_format_function_signature();
    _fprintf(_File,"%s\r\n",local_800);
    pcVar2 = *(char **)(*ppuVar4 + 0x10);
    pcVar3 = local_800;
    do {
      cVar1 = *pcVar2;
      pcVar2 = pcVar2 + 1;
      *pcVar3 = cVar1;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
    _fprintf(_File,"%s\r\n\r\n",local_800);
    sVar5 = sVar5 + 1;
    ppuVar4 = ppuVar4 + 1;
  } while (sVar5 < 0x20a);
  _fclose(_File);
  return;
}
#endif
