// compute_sha1_hash  (Ghidra: compute_sha1_hash, already named)
// address 0x57f2a0, size 140 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Computes the SHA-1
//   hash of param_2 bytes at param_1 using the CryptoAPI, writing the 20-byte digest to param_3."
//   Standard CryptCreateHash(CALG_SHA1=0x8004)/CryptHashData/CryptGetHashParam(HP_HASHVAL=2)
//   sequence against the module's shared crypt_provider (0x00722bcc).
// register convention: plain __cdecl (Ghidra's own recognized signature).
// blam-cc: data, length, digest_out (all stack parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t __stdcall CryptCreateHash(uint32_t provider, uint32_t algorithm_id, uint32_t key, uint32_t flags,
                                uint32_t *hash_out); // import 0x63a054
extern int32_t __stdcall CryptHashData(uint32_t hash, const uint8_t *data, uint32_t data_length, uint32_t flags); // import 0x63a05c
extern int32_t __stdcall CryptGetHashParam(uint32_t hash, uint32_t param, uint8_t *data, uint32_t *data_length,
                                  uint32_t flags); // import 0x63a048
extern int32_t __stdcall CryptDestroyHash(uint32_t hash); // import 0x63a058

extern uint32_t crypt_provider; // 0x00722bcc

// Computes the SHA-1 digest of `length` bytes at `data` using the module's shared CryptoAPI
// provider, writing the 20-byte result to `digest_out`. Returns 1 on success, 0 on any CryptoAPI
// failure (the hash object, if created, is always destroyed before returning).
uint8_t compute_sha1_hash(const uint8_t *data, uint32_t length, uint8_t *digest_out)
{
    uint32_t hash;
    uint32_t digest_length;
    uint8_t ok;

    hash = 0;
    ok = 0;
    if (CryptCreateHash(crypt_provider, 0x8004 /* CALG_SHA1 */, 0, 0, &hash) != 0) {
        if (CryptHashData(hash, data, length, 0) != 0) {
            digest_length = 0x14;
            ok = CryptGetHashParam(hash, 2 /* HP_HASHVAL */, digest_out, &digest_length, 0) != 0;
        }
    }
    if (hash != 0) {
        CryptDestroyHash(hash);
    }
    return ok;
}

#if 0
Original Ghidra decompilation (0x57f2a0):

int __cdecl compute_sha1_hash(BYTE *param_1,DWORD param_2,BYTE *param_3)

{
  BOOL BVar1;
  bool bVar2;
  HCRYPTHASH local_8;
  DWORD local_4;

  local_8 = 0;
  BVar1 = CryptCreateHash(DAT_00722bcc,0x8004,0,0,&local_8);
  bVar2 = false;
  if (BVar1 != 0) {
    BVar1 = CryptHashData(local_8,param_1,param_2,0);
    bVar2 = false;
    if (BVar1 != 0) {
      local_4 = 0x14;
      BVar1 = CryptGetHashParam(local_8,2,param_3,&local_4,0);
      bVar2 = BVar1 != 0;
    }
  }
  BVar1 = 0;
  if (local_8 != 0) {
    BVar1 = CryptDestroyHash(local_8);
  }
  return CONCAT31((int3)((uint)BVar1 >> 8),bVar2);
}
#endif
