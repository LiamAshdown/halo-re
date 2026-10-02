// extract_product_id_digits  (Ghidra: FUN_0057f360; renamed per out/phase4/shell_types_notes.md:
//   "extract_product_id_digits 0x57f360: product id string in EBX")
// address 0x57f360, size 136 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/shell_types_notes.md: "The digit tables at 0x00672a68 (OEM: 12..15,
//   18..22) and 0x00672a90 (retail: 6..8, 10..15) index product_id." objdump byte dump of both
//   tables confirms exactly these two 9-entry, 0-terminated offset arrays.
// register convention: out/phase4/shell_types_notes.md pins the product id string in EBX.
// blam-cc: product_id in EBX (only parameter).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t isdigit(int32_t c); // 0x62532f CRT
extern long atol(const char *string); // CRT atol (0x62589e: skips isspace, optional sign, decimal digits)

static const int32_t k_oem_digit_offsets[10] = {12, 13, 14, 15, 18, 19, 20, 21, 22, 0};
static const int32_t k_retail_digit_offsets[10] = {6, 7, 8, 10, 11, 12, 13, 14, 15, 0};

// Selects the OEM or retail digit-offset table by checking for "OEM" (case-insensitively) at
// product_id[6..8], then copies the digit at each table offset into a scratch buffer, stopping
// at the table's 0 terminator. If a table offset lands on a non-digit character first, the
// product id is malformed and this returns -1; otherwise returns the collected digits as a
// decimal number (atol_0062589e of the concatenated digit string).
int32_t extract_product_id_digits(const char *product_id)
{
    const int32_t *offsets;
    char digits[12];
    int32_t i;
    char c;

    if ((product_id[6] == 'O' || product_id[6] == 'o') && (product_id[7] == 'E' || product_id[7] == 'e') &&
        (product_id[8] == 'M' || product_id[8] == 'm')) {
        offsets = k_oem_digit_offsets;
    } else {
        offsets = k_retail_digit_offsets;
    }

    i = 0;
    while (offsets[i] != 0) {
        c = product_id[offsets[i]];
        if (!isdigit((int32_t)c)) {
            break;
        }
        digits[i] = c;
        i++;
    }

    if (offsets[i] != 0) {
        return -1;
    }
    digits[i] = 0;
    return atol(digits);
}

#if 0
Original Ghidra decompilation (0x57f360):

long FUN_0057f360(void)

{
  int iVar1;
  int iVar2;
  long lVar3;
  int unaff_EBX;
  int *piVar4;
  char *pcVar5;
  char local_c [12];

  if ((((*(char *)(unaff_EBX + 6) == 'O') || (*(char *)(unaff_EBX + 6) == 'o')) &&
      ((*(char *)(unaff_EBX + 7) == 'E' || (*(char *)(unaff_EBX + 7) == 'e')))) &&
     ((*(char *)(unaff_EBX + 8) == 'M' || (*(char *)(unaff_EBX + 8) == 'm')))) {
    piVar4 = &DAT_00672a68;
  }
  else {
    piVar4 = &DAT_00672a90;
  }
  iVar2 = *piVar4;
  pcVar5 = local_c;
  while ((iVar2 != 0 && (iVar2 = _isdigit((int)*(char *)(iVar2 + unaff_EBX)), iVar2 != 0))) {
    iVar1 = *piVar4;
    iVar2 = piVar4[1];
    piVar4 = piVar4 + 1;
    *pcVar5 = *(char *)(unaff_EBX + iVar1);
    pcVar5 = pcVar5 + 1;
  }
  if (*piVar4 != 0) {
    return -1;
  }
  *pcVar5 = '\0';
  lVar3 = _atol(local_c);
  return lVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
