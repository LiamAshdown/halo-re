// shell_build_product_id_string  (Ghidra: FUN_0057f3f0; renamed per types/shell.h's own header
//   comment, which already calls this "shell_build_product_id_string 0x57f3f0")
// address 0x57f3f0, size 317 bytes
// name confidence: 0.55  rewrite confidence: 0.6
// evidence: out/phase4/shell_functions.md: "Builds a formatted identifier string derived from
//   the registered Windows Product ID and a CryptoAPI hash, used elsewhere (e.g. crash/support
//   reporting)." types/shell.h's digital_product_id documents every field this function reads
//   (size/version header validated against 0xa4/3/0, hashed_key at +0x38 as the SHA-1 input,
//   citing this function's own address 0x57f4c7) and the product id string result buffer
//   (0x00722bd8, 0x80 bytes, "" on failure via the shared empty-string constant 0x0065512c).
// register convention: plain __cdecl, no parameters (Ghidra's own recognized signature).
//   extract_product_id_digits 0x57f360 takes the product id string in EBX; compute_sha1_hash_
//   first_qword 0x57f330 takes data in EDX, length in ECX, output in ESI (both per
//   out/phase4/shell_types_notes.md).
// blam-cc: (no arguments)
// UNSURE: RegQueryValueExA is called here with a buffer capacity of 0x400 (k_shell_path_length *
//   4) while the destination is the 0xa4-byte digital_product_id struct itself -- objdump
//   confirms the original passes 0x400, not sizeof(digital_product_id), so a DigitalProductID
//   registry value larger than 0xa4 bytes would overflow the stack buffer here exactly as it did
//   in the original binary; reproduced as-is rather than "fixed", per the no-invented-behaviour
//   rule.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693 CRT
extern int32_t extract_product_id_digits(const char *product_id); // 0x57f360, blam-cc: product_id in EBX
extern uint8_t compute_sha1_hash(const uint8_t *data, uint32_t length, uint8_t *digest_out); // 0x57f2a0
extern uint8_t compute_sha1_hash_first_qword(const uint8_t *data, uint32_t length, uint32_t *output); // 0x57f330,
                                                                                                        // blam-cc:
                                                                                                        // data in
                                                                                                        // EDX, length
                                                                                                        // in ECX,
                                                                                                        // output in
                                                                                                        // ESI

extern char product_id_string[k_product_id_string_length]; // 0x00722bd8
extern char k_empty_string;                                   // 0x0065512c, the shared "" constant other code returns
extern uint32_t crypt_provider;                              // 0x00722bcc

// Reads the Windows DigitalProductID registry value, validates its header (size 0xa4, version
// 3.0), and formats it into product_id_string as "<unknown_20>,<product id digits>,0,<hashed
// key prefix>" for crash/support reporting. Returns product_id_string on success, or the shared
// empty-string constant on any failure (missing/malformed registry value, or a CryptoAPI error).
char *shell_build_product_id_string(void)
{
    void *key;
    digital_product_id data;
    uint32_t data_size;
    uint8_t digest[20];
    int64_t hash_prefix;
    int32_t product_id_digits;

    product_id_string[0] = 0;
    data_size = 0x400; // see UNSURE above: larger than sizeof(digital_product_id)

    if (RegOpenKeyExA((HKEY)0x80000002 /* HKEY_LOCAL_MACHINE */, "Software\\Microsoft\\Microsoft Games\\Halo", 0,
                      0x20019, (PHKEY)&key) != 0) {
        return &k_empty_string;
    }
    if (RegQueryValueExA((HKEY)key, "DigitalProductID", 0, 0, (uint8_t *)&data, (LPDWORD)&data_size) != 0) {
        RegCloseKey((HKEY)key);
        return &k_empty_string;
    }
    RegCloseKey((HKEY)key);

    if (data.size != k_digital_product_id_size || data.major_version != k_digital_product_id_major_version ||
        data.minor_version != 0) {
        return &k_empty_string;
    }

    product_id_digits = extract_product_id_digits(data.product_id);

    if (CryptAcquireContextA((HCRYPTPROV *)&crypt_provider, 0, 0, 1 /* PROV_RSA_FULL */, 0xf0000000 /* CRYPT_VERIFYCONTEXT */) ==
        0) {
        return &k_empty_string;
    }
    if (compute_sha1_hash(data.hashed_key, k_digital_product_id_hashed_bytes, digest) == 0) {
        return &k_empty_string;
    }
    if (compute_sha1_hash_first_qword(digest, sizeof(digest), (uint32_t *)&hash_prefix) == 0) {
        return &k_empty_string;
    }

    sprintf(product_id_string, "%05d,%09d,0,% 19.19I64d", data.unknown_20, product_id_digits, hash_prefix);
    CryptReleaseContext(crypt_provider, 0);
    return product_id_string;
}

#if 0
Original Ghidra decompilation (0x57f3f0):

undefined1 * FUN_0057f3f0(void)

{
  LSTATUS LVar1;
  undefined4 uVar2;
  BOOL BVar3;
  int iVar4;
  HKEY local_c8;
  undefined4 local_c4;
  undefined4 local_c0;
  DWORD local_bc;
  BYTE local_b8 [20];
  int local_a4;
  short local_a0;
  short local_9e;
  undefined4 local_84;
  BYTE local_70 [112];

  DAT_00722bd8 = 0;
  local_bc = 0x400;
  local_c4 = 0;
  local_c0 = 0;
  LVar1 = RegOpenKeyExA((HKEY)&DAT_80000002,"Software\\Microsoft\\Microsoft Games\\Halo",0,0x20019,
                        &local_c8);
  if (LVar1 == 0) {
    LVar1 = RegQueryValueExA(local_c8,"DigitalProductID",(LPDWORD)0x0,(LPDWORD)0x0,(LPBYTE)&local_a4
                             ,&local_bc);
    if (LVar1 == 0) {
      RegCloseKey(local_c8);
      if (((local_a4 == 0xa4) && (local_a0 == 3)) && (local_9e == 0)) {
        uVar2 = FUN_0057f360();
        BVar3 = CryptAcquireContextA(&DAT_00722bcc,(LPCSTR)0x0,(LPCSTR)0x0,1,0xf0000000);
        if (BVar3 != 0) {
          iVar4 = compute_sha1_hash(local_70,0xf,local_b8);
          if ((char)iVar4 != '\0') {
            iVar4 = FUN_0057f330();
            if (iVar4 != 0) {
              _sprintf(&DAT_00722bd8,"%05d,%09d,0,% 19.19I64d",local_84,uVar2,local_c4,local_c0);
              CryptReleaseContext(DAT_00722bcc,0);
              return &DAT_00722bd8;
            }
          }
        }
        return &DAT_0065512c;
      }
    }
    else {
      RegCloseKey(local_c8);
    }
  }
  return &DAT_0065512c;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
