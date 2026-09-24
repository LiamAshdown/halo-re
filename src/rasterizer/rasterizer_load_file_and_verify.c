// rasterizer_load_file_and_verify  (Ghidra: rasterizer_load_file_and_verify, already named)
// address 0x5199f0, size 195 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: a plain CreateFileA/GetFileSize/GlobalAlloc/ReadFile sequence followed by
//   rasterizer_resource_file_verify_signature; matches its own name exactly.
// register convention: out buffer pointer / out size in the two recognized parameters, filename
//   in in_ECX. // blam-cc: ECX -> path, stack -> (out_buffer, out_size)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *CreateFileA(const char *path, uint32_t access, uint32_t share, void *security,
                          uint32_t creation, uint32_t flags, void *template_file); // Win32
extern uint32_t GetFileSize(void *file, uint32_t *high); // Win32
extern void *GlobalAlloc(uint32_t flags, uint32_t bytes); // Win32
extern int32_t ReadFile(void *file, void *buffer, uint32_t bytes_to_read, uint32_t *bytes_read, void *overlapped); // Win32
extern int32_t CloseHandle(void *file); // Win32
extern void *GlobalFree(void *mem); // Win32

extern uint8_t rasterizer_resource_file_verify_signature(uint8_t *buffer, uint32_t size); // 0x519980 (this session)

// blam-cc: ECX -> path, stack -> (out_buffer, out_size)
// Reads the whole file `path` into a newly GlobalAlloc'd buffer, verifies it with
// rasterizer_resource_file_verify_signature, and returns the buffer and size on success (1);
// frees the buffer and returns 0 on any failure.
uint32_t rasterizer_load_file_and_verify(void **out_buffer, uint32_t *out_size, const char *path)
{
    void *file;
    uint32_t size;
    void *buffer;

    *out_buffer = (void *)0;
    *out_size = 0;

    file = CreateFileA(path, 0x80000000, 0, (void *)0, 3, 0x8000000, (void *)0);
    if (file == (void *)0xffffffff) {
        return 0;
    }

    size = GetFileSize(file, (uint32_t *)0);
    if (size == 0xffffffff) {
        CloseHandle(file);
        return 0;
    }

    buffer = GlobalAlloc(0, size);
    if (buffer != (void *)0) {
        uint32_t bytes_read;
        int32_t ok = ReadFile(file, buffer, size, &bytes_read, (void *)0);
        if (ok != 0) {
            CloseHandle(file);
            if (rasterizer_resource_file_verify_signature((uint8_t *)buffer, size) == 0) {
                GlobalFree(buffer);
                return 0;
            }
            *out_buffer = buffer;
            *out_size = size;
            return 1;
        }
        GlobalFree(buffer);
    }
    CloseHandle(file);
    return 0;
}

#if 0
Original Ghidra decompilation (0x5199f0):

undefined4 rasterizer_load_file_and_verify(undefined4 *param_1,DWORD *param_2)

{
  DWORD *pDVar1;
  char cVar2;
  HANDLE hFile;
  DWORD dwBytes;
  HGLOBAL lpBuffer;
  BOOL BVar3;
  LPCSTR in_ECX;

  pDVar1 = param_2;
  *param_1 = 0;
  *param_2 = 0;
  hFile = CreateFileA(in_ECX,0x80000000,0,(LPSECURITY_ATTRIBUTES)0x0,3,0x8000000,(HANDLE)0x0);
  if (hFile == (HANDLE)0xffffffff) {
    return 0;
  }
  dwBytes = GetFileSize(hFile,(LPDWORD)0x0);
  if (dwBytes == 0xffffffff) {
    CloseHandle(hFile);
    return 0;
  }
  lpBuffer = GlobalAlloc(0,dwBytes);
  if (lpBuffer != (HGLOBAL)0x0) {
    BVar3 = ReadFile(hFile,lpBuffer,dwBytes,(LPDWORD)&param_2,(LPOVERLAPPED)0x0);
    if (BVar3 != 0) {
      CloseHandle(hFile);
      cVar2 = FUN_00519980();
      if (cVar2 == '\0') {
        GlobalFree(lpBuffer);
        return 0;
      }
      *param_1 = lpBuffer;
      *pDVar1 = dwBytes;
      return 1;
    }
    GlobalFree(lpBuffer);
  }
  CloseHandle(hFile);
  return 0;
}
#endif
