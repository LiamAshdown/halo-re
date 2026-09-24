// rasterizer_create_game_window  (Ghidra: rasterizer_create_game_window, already named)
// address 0x515930, size 490 bytes
// name confidence: 0.7   rewrite confidence: 0.7
// evidence: registers a window class and creates/centers the game's main window via the
//   standard Win32 sequence (RegisterClassExA, GetDesktopWindow/GetWindowRect/AdjustWindowRect,
//   CreateWindowExA), reporting a message box on failure; matches its own name and the
//   functions.md summary exactly.
// register convention: window height in in_EAX, width in unaff_EBX (unresolved register read).
//   // blam-cc: EAX -> height, unaff_EBX -> width
// UNSURE: WNDCLASSEXA/RECT are modeled locally (standard Win32 layouts, pinned by the 0x30 byte
//   / 12 dword zero loop matching WNDCLASSEXA exactly) since the project headers do not carry
//   Win32 SDK types.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"


extern void *rasterizer_window_proc;    // 0x007461d0, WNDPROC
extern uint32_t rasterizer_window_style; // 0x0069c6a4
extern void *rasterizer_hinstance;      // 0x007461c0
extern char rasterizer_window_class_name[]; // 0x007461d4
extern char rasterizer_window_title[];      // 0x00746214
extern void *rasterizer_hwnd;               // 0x007461c4
extern void *shell_module_handle;                                   // 0x00722bb8 HINSTANCE
extern void *rasterizer_window_icon_bitmap; // 0x0071d188, HBITMAP
extern void *rasterizer_window_icon_dc;     // 0x0071d184, HDC

extern void *LoadIconA(void *hinstance, const char *name);
extern void *LoadCursorA(void *hinstance, const char *name);
extern int32_t RegisterClassExA(win32_wndclassexa *wc);
extern void *GetDesktopWindow(void);
extern int32_t GetWindowRect(void *hwnd, win32_rect *rect);
extern int32_t AdjustWindowRect(win32_rect *rect, uint32_t style, int32_t menu);
extern void *CreateWindowExA(uint32_t ex_style, const char *class_name, const char *title, uint32_t style,
                              int32_t x, int32_t y, int32_t w, int32_t h, void *parent, void *menu,
                              void *hinstance, void *param);
extern uint32_t GetLastError(void);
extern uint32_t FormatMessageA(uint32_t flags, const void *source, uint32_t message_id, uint32_t language_id,
                                char **buffer, uint32_t size, void *arguments);
extern int32_t MessageBoxA(void *hwnd, const char *text, const char *caption, uint32_t type);
extern int32_t UnregisterClassA(const char *class_name, void *hinstance);
extern void *LocalFree(void *mem);
extern void *LoadBitmapA(void *hinstance, const char *name);
extern void *GetDC(void *hwnd);
extern void *CreateCompatibleDC(void *hdc);
extern void *SelectObject(void *hdc, void *object);
extern int32_t SetForegroundWindow(void *hwnd);
extern int32_t SetActiveWindow(void *hwnd);
extern void *SetFocus(void *hwnd);
extern int32_t ShowWindow(void *hwnd, int32_t cmd_show);

// blam-cc: EAX -> height, unaff_EBX -> width
// Registers the game window class and creates/centers a `width` by `height` window on the
// primary desktop, reporting a message box and returning 0 on failure. On success, loads the
// taskbar icon bitmap (best-effort, ignored on failure) and brings the window to the front.
uint32_t rasterizer_create_game_window(int32_t height, int32_t width)
{
    win32_wndclassexa wc;
    win32_rect rect;
    void *hwnd;
    uint32_t *fields = (uint32_t *)&wc;
    int i;

    for (i = 0; i < 0xc; i++) {
        fields[i] = 0;
    }

    wc.window_procedure = (uint32_t)rasterizer_window_proc;
    rasterizer_window_style = 0xcf0000; // WS_OVERLAPPEDWINDOW
    wc.size = 0x30;
    wc.style = 0x40; // CS_DBLCLKS
    wc.class_extra = 0;
    wc.window_extra = 0;
    wc.instance = (uint32_t)rasterizer_hinstance;
    wc.icon = (uint32_t)LoadIconA(rasterizer_hinstance, (const char *)0x66);
    wc.small_icon = (uint32_t)LoadIconA(rasterizer_hinstance, (const char *)0x66);
    wc.cursor = (uint32_t)LoadCursorA((void *)0, (const char *)0x7f00); // IDC_ARROW
    wc.background_brush = 0;
    wc.menu_name = 0;
    wc.class_name = (uint32_t)rasterizer_window_class_name;
    RegisterClassExA(&wc);

    GetWindowRect(GetDesktopWindow(), &rect);
    rect.top = (uint32_t)((rect.bottom - rect.top) - height) >> 1;
    rect.bottom = rect.top + height;
    rect.left = (uint32_t)((rect.right - rect.left) - width) >> 1;
    rect.right = rect.left + width;
    AdjustWindowRect(&rect, rasterizer_window_style, 0);

    hwnd = CreateWindowExA(0, rasterizer_window_class_name, rasterizer_window_title,
                            rasterizer_window_style, rect.left, rect.top, rect.right - rect.left,
                            rect.bottom - rect.top, GetDesktopWindow(), (void *)0, (void *)wc.instance, (void *)0);
    if (hwnd == (void *)0) {
        char *message_buffer = (char *)0;
        uint32_t message_id = GetLastError();
        FormatMessageA(0x1300, (const void *)0, message_id, 0x400, &message_buffer, 0, (void *)0);
        MessageBoxA((void *)0, message_buffer, "ERROR - failed to create window", 0x40);
        UnregisterClassA(rasterizer_window_class_name, rasterizer_hinstance);
        LocalFree(message_buffer);
        return 0;
    }

    rasterizer_hwnd = hwnd;
    rasterizer_window_icon_bitmap = LoadBitmapA((void *)shell_module_handle, (const char *)0x86);
    if (rasterizer_window_icon_bitmap != (void *)0) {
        void *hdc = GetDC(hwnd);
        rasterizer_window_icon_dc = CreateCompatibleDC(hdc);
        SelectObject(rasterizer_window_icon_dc, rasterizer_window_icon_bitmap);
    }

    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    SetFocus(hwnd);
    ShowWindow(hwnd, 5); // SW_SHOW
    return 1;
}

#if 0
Original Ghidra decompilation (0x515930):

undefined4 rasterizer_create_game_window(void)

{
  int in_EAX;
  HWND pHVar1;
  DWORD dwMessageId;
  HDC hdc;
  int iVar2;
  int unaff_EBX;
  WNDCLASSEXA *pWVar3;
  DWORD dwLanguageId;
  HMENU hMenu;
  LPCSTR *lpBuffer;
  HINSTANCE hInstance;
  DWORD nSize;
  tagRECT *lpRect;
  LPVOID lpParam;
  va_list *Arguments;
  LPCSTR local_44;
  tagRECT local_40;
  WNDCLASSEXA local_30;

  pWVar3 = &local_30;
  for (iVar2 = 0xc; iVar2 != 0; iVar2 = iVar2 + -1) {
    pWVar3->cbSize = 0;
    pWVar3 = (WNDCLASSEXA *)&pWVar3->style;
  }
  local_30.lpfnWndProc = DAT_007461d0;
  DAT_0069c6a4 = 0xcf0000;
  local_30.cbSize = 0x30;
  local_30.style = 0x40;
  local_30.cbClsExtra = 0;
  local_30.cbWndExtra = 0;
  local_30.hInstance = DAT_007461c0;
  local_30.hIcon = LoadIconA(DAT_007461c0,&DAT_00000066);
  local_30.hIconSm = LoadIconA(DAT_007461c0,&DAT_00000066);
  local_30.hCursor = LoadCursorA((HINSTANCE)0x0,&DAT_00007f00);
  local_30.hbrBackground = (HBRUSH)0x0;
  local_30.lpszMenuName = (LPCSTR)0x0;
  local_30.lpszClassName = &DAT_007461d4;
  RegisterClassExA(&local_30);
  lpRect = &local_40;
  pHVar1 = GetDesktopWindow();
  GetWindowRect(pHVar1,lpRect);
  local_40.top = (uint)((local_40.bottom - local_40.top) - in_EAX) >> 1;
  local_40.bottom = local_40.top + in_EAX;
  local_40.left = (uint)((local_40.right - local_40.left) - unaff_EBX) >> 1;
  local_40.right = local_40.left + unaff_EBX;
  AdjustWindowRect(&local_40,DAT_0069c6a4,0);
  lpParam = (LPVOID)0x0;
  hMenu = (HMENU)0x0;
  hInstance = local_30.hInstance;
  pHVar1 = GetDesktopWindow();
  pHVar1 = CreateWindowExA(0,&DAT_007461d4,&DAT_00746214,DAT_0069c6a4,local_40.left,local_40.top,
                           local_40.right - local_40.left,local_40.bottom - local_40.top,pHVar1,
                           hMenu,hInstance,lpParam);
  if (pHVar1 == (HWND)0x0) {
    Arguments = (va_list *)0x0;
    nSize = 0;
    lpBuffer = &local_44;
    dwLanguageId = 0x400;
    dwMessageId = GetLastError();
    FormatMessageA(0x1300,(LPCVOID)0x0,dwMessageId,dwLanguageId,(LPSTR)lpBuffer,nSize,Arguments);
    MessageBoxA((HWND)0x0,local_44,"ERROR - failed to create window",0x40);
    UnregisterClassA(&DAT_007461d4,DAT_007461c0);
    LocalFree(local_44);
    return 0;
  }
  DAT_007461c4 = pHVar1;
  DAT_0071d188 = LoadBitmapA(DAT_00722bb8,&DAT_00000086);
  if (DAT_0071d188 != (HBITMAP)0x0) {
    hdc = GetDC(pHVar1);
    DAT_0071d184 = CreateCompatibleDC(hdc);
    SelectObject(DAT_0071d184,DAT_0071d188);
  }
  SetForegroundWindow(pHVar1);
  SetActiveWindow(pHVar1);
  SetFocus(pHVar1);
  ShowWindow(pHVar1,5);
  return 1;
}
#endif
