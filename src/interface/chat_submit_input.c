// chat_submit_input  (Ghidra: chat_submit_input, already named)
// address 0x4aa9b0, size 210 bytes
// name confidence: 0.5 (existing Ghidra name)   rewrite confidence: 0.5
// evidence: matches the given name and cc; strings "oEditbox", "text"; shares the embedded GUI
// library table with chimera__chat_open.c/chat_close.c.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>

extern uint8_t chat_dialog_open; // 0x006b3858

extern void *chat_gui_root_handle;    // 0x00721ea4
extern chat_gui_find_object_fn chat_gui_find_object; // 0x00721eb8
extern void *chat_gui_find_object_arg; // 0x0069c698
extern chat_gui_find_child_fn chat_gui_find_child;    // 0x00721ecc
extern chat_gui_get_property_string_fn chat_gui_get_property_string; // 0x00721ee0
extern chat_gui_release_fn chat_gui_release;          // 0x00721ec8

extern int32_t chat_default_team_channel(void); // 0x4ab1e0
extern int32_t FUN_00625b7a(const uint16_t *s); // 0x625b7a, UNSURE: appears to be wcslen
extern void chimera__chat_out(uint8_t team_index); // 0x4aab00
extern void chat_close(void); // 0x4aa900

// Reads the text typed into the open chat editbox and, if the default team channel is valid
// and the box is non-empty, sends it (truncated to 254 wide characters) via chimera__chat_out
// before closing the dialog.
void chat_submit_input(void)
{
    if (chat_dialog_open == 0) {
        return;
    }

    {
        int32_t team_index = chat_default_team_channel();
        if (team_index != -1) {
            const wchar_t *text = 0;
            void *gui_object = chat_gui_find_object(chat_gui_root_handle, chat_gui_find_object_arg);
            if (gui_object != 0) {
                void *editbox = chat_gui_find_child(gui_object, L"oEditbox");
                if (editbox != 0) {
                    text = chat_gui_get_property_string(editbox, L"text");
                }
                chat_gui_release(gui_object);

                if (text != 0 && *text != 0) {
                    wchar_t buffer[256];
                    uint32_t length = FUN_00625b7a((const uint16_t *)text);
                    size_t count = (length < 0xff) ? length : 0xfe;
                    wcsncpy(buffer, text, count);
                    buffer[count] = 0;
                    chimera__chat_out((uint8_t)team_index);
                }
            }
        }
        chat_close();
    }
}

#if 0
Original Ghidra decompilation (0x4aa9b0):

void __cdecl chat_submit_input(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  wchar_t *_Source;
  size_t _Count;
  wchar_t awStack_200 [256];

  if (DAT_006b3858 != '\0') {
    iVar1 = chat_default_team_channel();
    if (iVar1 != -1) {
      _Source = (wchar_t *)0x0;
      iVar2 = (*DAT_00721eb8)(DAT_00721ea4,PTR_PTR_0069c698);
      if (iVar2 != 0) {
        iVar3 = (*DAT_00721ecc)(iVar2,L"oEditbox");
        if (iVar3 != 0) {
          _Source = (wchar_t *)(*DAT_00721ee0)(iVar3,L"text");
        }
        (*DAT_00721ec8)(iVar2);
        if ((_Source != (wchar_t *)0x0) && (*_Source != L'\0')) {
          uVar4 = FUN_00625b7a(_Source);
          if (uVar4 < 0xff) {
            _Count = FUN_00625b7a(_Source);
          }
          else {
            _Count = 0xfe;
          }
          _wcsncpy(awStack_200,_Source,_Count);
          awStack_200[_Count] = L'\0';
          chimera__chat_out((char)iVar1);
        }
      }
    }
    chat_close();
  }
  return;
}
#endif
