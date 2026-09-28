// autopatch_get_proxy_settings  (Ghidra: autopatch_get_proxy_settings, already named)
// address 0x576f40, size 639 bytes (0x576f40..0x5771be, single `ret`)
// name confidence: 0.7   rewrite confidence: 0.75
// evidence: objdump -d -M intel 0x576f40..0x5771bf, used instead of the decompile because Ghidra
//   merged the two Win32 records on the stack into one tangle (it reads WINHTTP_PROXY_INFO's
//   string pointers out of the WINHTTP_AUTOPROXY_OPTIONS slots and aims WideCharToMultiByte at
//   the wrong buffer). The real frame, relative to esp after the four register pushes:
//     +0x010 DWORD  query_length (0x3ff)
//     +0x014 WINHTTP_AUTOPROXY_OPTIONS { dwFlags = 1 (AUTO_DETECT), dwAutoDetectFlags = 3
//            (DHCP | DNS_A), lpszAutoConfigUrl = 0, lpvReserved = 0, dwReserved = 0,
//            fAutoLogonIfChallenged = 1 }   (writes at 0x577056..0x57708f)
//     +0x02c WINHTTP_PROXY_INFO { dwAccessType, lpszProxy (+0x30), lpszProxyBypass (+0x34) }
//     +0x038 char proxy_list[0x400]
//     +0x438 INTERNET_PROXY_INFO buffer[0x400] (lpszProxy at +0x43c)
//   Strings: 0x672118 "winhttp.dll", 0x672100 "WinHttpGetProxyForUrl", 0x6720f4 "WinHttpOpen",
//   0x6720e0 "WinHttpCloseHandle", 0x6720d0 L"HaloPC", 0x6720a4 L"http://www.bungie.net",
//   0x6720a0 " ;", 0x672098 "http=", 0x672090 "http://". InternetQueryOptionA is reached through
//   the wininet delay-load slot at 0x0069ffd0; 0x26 is INTERNET_OPTION_PROXY. strstr is the
//   CRT strstr (src/input/input_joystick_axis_name_to_index.c names it so).
//   Sole caller: autopatch_proxy_initialize (0x5771c0), which hands the returned string to the
//   HTTP client setup.
// Not Blam engine logic (Win32 proxy discovery for the autopatch HTTP client), but it is a real
// Bungie-written function in the networking range, so it is rewritten like the rest.
// UNSURE: the name of the 0x100-byte result buffer at 0x007227d0 (this file's own name).
// register convention: no parameters.
//   // blam-cc: none -> returns char * (always autopatch_proxy_server)

#include "crt.h"
#include "win32.h"
#include <ctype.h>
#include "tags.h"
#include "memory.h"
#include "math.h"

typedef struct internet_proxy_info {       // Win32 INTERNET_PROXY_INFO
    uint32_t access_type;
    const char *proxy;
    const char *proxy_bypass;
} internet_proxy_info;

typedef struct winhttp_autoproxy_options { // Win32 WINHTTP_AUTOPROXY_OPTIONS
    uint32_t flags;
    uint32_t auto_detect_flags;
    const uint16_t *auto_config_url;
    void *reserved_pointer;
    uint32_t reserved;
    int32_t auto_logon_if_challenged;
} winhttp_autoproxy_options;

typedef struct winhttp_proxy_info {        // Win32 WINHTTP_PROXY_INFO
    uint32_t access_type;
    uint16_t *proxy;
    uint16_t *proxy_bypass;
} winhttp_proxy_info;

typedef void *(__stdcall *winhttp_open_proc)(const uint16_t *agent, uint32_t access_type,
    const uint16_t *proxy, const uint16_t *bypass, uint32_t flags);
typedef int32_t (__stdcall *winhttp_get_proxy_for_url_proc)(void *session, const uint16_t *url,
    winhttp_autoproxy_options *options, winhttp_proxy_info *proxy_info);
typedef int32_t (__stdcall *winhttp_close_handle_proc)(void *handle);

extern char autopatch_proxy_server[0x100]; // 0x007227d0, 0x007228cf is its last byte

extern int32_t InternetQueryOptionA(void *internet, uint32_t option, void *buffer,
    uint32_t *buffer_length); // 0x0069ffd0 wininet delay-load slot

static const uint16_t k_agent_halopc[] = { 'H', 'a', 'l', 'o', 'P', 'C', 0 }; // 0x6720d0
static const uint16_t k_bungie_url[] = { 'h', 't', 't', 'p', ':', '/', '/', 'w', 'w', 'w', '.',
    'b', 'u', 'n', 'g', 'i', 'e', '.', 'n', 'e', 't', 0 };                       // 0x6720a4

// Finds the HTTP proxy the autopatch client should use: first the WinInet (Internet Explorer)
// setting, then WinHTTP auto-detection (WPAD) for http://www.bungie.net. From the proxy list it
// takes the "http=" entry (or the first entry), strips any leading "http://" prefixes and copies
// it into autopatch_proxy_server, which it always returns (empty when no proxy was found).
char *autopatch_get_proxy_settings(void)
{
    uint32_t query_length;
    winhttp_autoproxy_options options;
    winhttp_proxy_info proxy_info;
    char proxy_list[0x400];
    uint8_t query_buffer[0x400];
    void *winhttp;
    winhttp_get_proxy_for_url_proc get_proxy_for_url;
    winhttp_open_proc open;
    winhttp_close_handle_proc close_handle;
    void *session;
    char *token;
    char *proxy;
    char *cursor;
    int32_t token_count;
    int32_t i;

    for (i = 0; i < 0x400; i++) {
        query_buffer[i] = 0;
        proxy_list[i] = 0;
    }
    for (i = 0; i < 0x100; i++) {
        autopatch_proxy_server[i] = 0;
    }

    // 1. the WinInet proxy setting
    query_length = 0x3ff;
    if (InternetQueryOptionA(0, 0x26, query_buffer, &query_length) && query_length > 1 &&
        ((internet_proxy_info *)query_buffer)->proxy != 0) {
        strncpy(proxy_list, ((internet_proxy_info *)query_buffer)->proxy, 0x400);
        proxy_list[0x3ff] = 0;
    }

    // 2. WinHTTP auto-detection
    if (proxy_list[0] == 0) {
        winhttp = LoadLibraryA("winhttp.dll");
        if (winhttp != 0) {
            get_proxy_for_url = (winhttp_get_proxy_for_url_proc)GetProcAddress(winhttp, "WinHttpGetProxyForUrl");
            open = (winhttp_open_proc)GetProcAddress(winhttp, "WinHttpOpen");
            close_handle = (winhttp_close_handle_proc)GetProcAddress(winhttp, "WinHttpCloseHandle");
            if (get_proxy_for_url != 0 && open != 0 && close_handle != 0) {
                session = open(k_agent_halopc, 0, 0, 0, 0);
                if (session != 0) {
                    options.flags = 1;
                    options.auto_detect_flags = 3;
                    options.auto_config_url = 0;
                    options.reserved_pointer = 0;
                    options.reserved = 0;
                    options.auto_logon_if_challenged = 1;
                    if (get_proxy_for_url(session, k_bungie_url, &options, &proxy_info)) {
                        if (proxy_info.proxy != 0) {
                            if (proxy_info.proxy[0] != 0) {
                                WideCharToMultiByte(0, 0, proxy_info.proxy, -1, proxy_list, 0x400, 0, 0);
                                proxy_list[0x3ff] = 0;
                            }
                            if (proxy_info.proxy != 0) {
                                GlobalFree(proxy_info.proxy);
                            }
                        }
                        if (proxy_info.proxy_bypass != 0) {
                            GlobalFree(proxy_info.proxy_bypass);
                        }
                    }
                    close_handle(session);
                }
            }
            FreeLibrary(winhttp);
        }
        if (proxy_list[0] == 0) {
            return autopatch_proxy_server;
        }
    }

    // 3. pick the http entry out of "http=host:port;https=..." or a bare "host:port" list
    for (cursor = proxy_list; *cursor != 0; cursor++) {
        *cursor = (char)_tolower((uint8_t)*cursor);
    }

    token_count = 0;
    token = strtok(proxy_list, " ;");
    if (token == 0) {
        return autopatch_proxy_server;
    }
    do {
        token_count++;
        if (strstr(token, "http=") == token) {
            proxy = token + 5;
            if (proxy == 0) {   // kept from the original (`test esi,esi` at 0x57716d)
                return autopatch_proxy_server;
            }
            goto copy_proxy;
        }
        token = strtok(0, " ;");
    } while (token != 0);
    if (token_count <= 0) {
        return autopatch_proxy_server;
    }
    proxy = proxy_list; // no "http=" entry: the first token (strtok has already cut it off)

copy_proxy:
    while (strstr(proxy, "http://") == proxy) {
        proxy += 7;
    }
    strncpy(autopatch_proxy_server, proxy, 0x100);
    autopatch_proxy_server[0xff] = 0;
    return autopatch_proxy_server;
}

#if 0
Original Ghidra decompilation (0x576f40):

undefined4 * autopatch_get_proxy_settings(void)

{
  byte bVar1;
  HMODULE hModule;
  FARPROC pFVar2;
  FARPROC pFVar3;
  FARPROC pFVar4;
  int iVar5;
  char *pcVar6;
  char *pcVar7;
  byte *pbVar8;
  int iVar9;
  byte *pbVar10;
  undefined4 *puVar11;
  uint local_830;
  undefined4 uStack_82c;
  undefined4 uStack_828;
  undefined4 uStack_824;
  LPCWSTR pWStack_820;
  HGLOBAL pvStack_81c;
  undefined4 uStack_818;
  undefined1 auStack_814 [12];
  byte local_808;
  undefined4 local_807;
  undefined1 uStack_419;
  undefined1 local_409;
  undefined1 local_408;
  undefined1 local_407 [1027];

  local_408 = 0;
  puVar11 = (undefined4 *)local_407;
  for (iVar9 = 0xff; iVar9 != 0; iVar9 = iVar9 + -1) {
    *puVar11 = 0;
    puVar11 = puVar11 + 1;
  }
  *(undefined2 *)puVar11 = 0;
  *(undefined1 *)((int)puVar11 + 2) = 0;
  local_808 = 0;
  puVar11 = &local_807;
  for (iVar9 = 0xff; iVar9 != 0; iVar9 = iVar9 + -1) {
    *puVar11 = 0;
    puVar11 = puVar11 + 1;
  }
  *(undefined2 *)puVar11 = 0;
  *(undefined1 *)((int)puVar11 + 2) = 0;
  puVar11 = &DAT_007227d0;
  for (iVar9 = 0x40; iVar9 != 0; iVar9 = iVar9 + -1) {
    *puVar11 = 0;
    puVar11 = puVar11 + 1;
  }
  local_830 = 0x3ff;
  iVar9 = InternetQueryOptionA(0,0x26,&local_408,&local_830);
  if (((iVar9 != 0) && (1 < local_830)) && ((char *)local_407._3_4_ != (char *)0x0)) {
    _strncpy((char *)&local_808,(char *)local_407._3_4_,0x400);
    local_409 = 0;
  }
  if (local_808 == 0) {
    hModule = LoadLibraryA("winhttp.dll");
    if (hModule != (HMODULE)0x0) {
      pFVar2 = GetProcAddress(hModule,"WinHttpGetProxyForUrl");
      pFVar3 = GetProcAddress(hModule,"WinHttpOpen");
      pFVar4 = GetProcAddress(hModule,"WinHttpCloseHandle");
      if (((pFVar2 != (FARPROC)0x0) && (pFVar3 != (FARPROC)0x0)) &&
         ((pFVar4 != (FARPROC)0x0 && (iVar9 = (*pFVar3)(L"HaloPC",0,0,0,0), iVar9 != 0)))) {
        uStack_824 = 0;
        pWStack_820 = (LPCWSTR)0x0;
        pvStack_81c = (HGLOBAL)0x0;
        uStack_82c = 1;
        uStack_828 = 3;
        uStack_818 = 1;
        iVar5 = (*pFVar2)(iVar9,L"http://www.bungie.net",&uStack_82c,auStack_814);
        if (iVar5 != 0) {
          if (pWStack_820 != (LPCWSTR)0x0) {
            if (*pWStack_820 != L'\0') {
              WideCharToMultiByte(0,0,pWStack_820,-1,(LPSTR)&uStack_818,0x400,(LPCSTR)0x0,
                                  (LPBOOL)0x0);
              uStack_419 = 0;
            }
            if (pWStack_820 != (LPCWSTR)0x0) {
              GlobalFree(pWStack_820);
            }
          }
          if (pvStack_81c != (HGLOBAL)0x0) {
            GlobalFree(pvStack_81c);
          }
        }
        (*pFVar4)(iVar9);
      }
      FreeLibrary(hModule);
    }
    if (local_808 == 0) {
      return &DAT_007227d0;
    }
  }
  iVar9 = 0;
  pbVar10 = &local_808;
  bVar1 = local_808;
  while (bVar1 != 0) {
    iVar5 = _tolower((uint)*pbVar10);
    *pbVar10 = (byte)iVar5;
    pbVar8 = pbVar10 + 1;
    pbVar10 = pbVar10 + 1;
    bVar1 = *pbVar8;
  }
  pcVar6 = _strtok((char *)&local_808," ;");
  if (pcVar6 != (char *)0x0) {
    do {
      iVar9 = iVar9 + 1;
      pcVar7 = (char *)FUN_00625430(pcVar6,"http=");
      if (pcVar7 == pcVar6) {
        pbVar10 = (byte *)(pcVar6 + 5);
        if (pbVar10 == (byte *)0x0) {
          return &DAT_007227d0;
        }
        goto LAB_00577171;
      }
      pcVar6 = _strtok((char *)0x0," ;");
    } while (pcVar6 != (char *)0x0);
    if (0 < iVar9) {
      pbVar10 = &local_808;
LAB_00577171:
      pbVar8 = (byte *)FUN_00625430(pbVar10,"http://");
      if (pbVar8 == pbVar10) {
        do {
          pbVar10 = pbVar10 + 7;
          pbVar8 = (byte *)FUN_00625430(pbVar10,"http://");
        } while (pbVar8 == pbVar10);
      }
      _strncpy((char *)&DAT_007227d0,(char *)pbVar10,0x100);
      DAT_007228cf = 0;
    }
  }
  return &DAT_007227d0;
}
#endif
