// chimera__chat_open  (Ghidra: chimera__chat_open, already named)
// address 0x4aa700, size 512 bytes
// name confidence: 0.6 (existing Ghidra name)   rewrite confidence: 0.35
// evidence: matches the given name and cc (__cdecl); strings "ui\\multiplayer_game_text",
// "oPrompt", "text", "oEditbox"; types/interface.h's note that the chat dialog is driven
// through the embedded GUI library's function-pointer table at 0x00721ea4..0x00721ee8 (an
// "oListbox"-style control), reused here for an "oPrompt"/"oEditbox" dialog; src/game/
// established empty_string (0x00660c34).
// UNSURE: the embedded GUI library's function table is not named anywhere in this pass; each
// slot is declared with the signature its call site implies and a generic name. UNSURE:
// DAT_006b7020 (a second "chat busy" gate alongside DAT_006b3858) is not documented.
// register convention: __cdecl, chat_scope as the recognized parameter (0 = all, 1 = team,
// 2 = vehicle).
// reconciled: R08 0x006b7020 chat_busy -> main.h console_globals_data.active (the console-open byte; byte read unchanged)

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "main.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t chat_dialog_open;      // 0x006b3858, UNSURE name
extern console_globals console_globals_data;             // 0x006b7020, main.h; +0x00 active = console open (R08)
extern int32_t chat_scope_active;     // 0x006b385c, -1 when no chat dialog is open
extern wchar_t empty_string;          // 0x00660c34

// UNSURE: the embedded GUI library's own object handle/table pointer and its function slots.
extern void *chat_gui_root_handle;                          // 0x00721ea4
extern chat_gui_find_object_fn chat_gui_find_object;         // 0x00721eb8
extern void *chat_gui_find_object_arg;                       // 0x0069c698
extern chat_gui_find_child_fn chat_gui_find_child;            // 0x00721ecc
extern chat_gui_set_focus_fn chat_gui_set_focus;              // 0x00721ed4
extern chat_gui_set_property_string_fn keystone_control_set_attribute; // 0x00721ee4
extern chat_gui_set_property_int_fn chat_gui_set_property_int; // 0x00721ee8
extern chat_gui_set_state_fn chat_gui_set_state;              // 0x00721edc
extern chat_gui_release_fn chat_gui_release;                  // 0x00721ec8
extern uint8_t chat_gui_active;                               // 0x00721eec

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern uint16_t *text_string_list_get_string(void); // 0x5578c0, UNSURE signature, called with no visible arguments
extern uint8_t game_engine_get_teams_enabled(void); // 0x462bf0, UNSURE signature
extern void input_keyboard_set_capture_mode(void); // 0x48b650, UNSURE signature, not in this module's range
extern datum_index player_get_vehicle(datum_index player_index); // 0x4ab170, blam-cc: ECX player_index; the vehicle of the player unit or -1
extern int32_t chat_default_team_channel(void); // 0x4ab1e0

// Opens the multiplayer chat input dialog for the requested scope: 0 = all, 1 = team (falls
// back to "all" if teams are disabled), 2 = vehicle (falls back to "team", then "all"),
// populating its prompt text from the ui\multiplayer_game_text tag when available.
void chimera__chat_open(int32_t chat_scope)
{
    const void *prompt_text;
    void *gui_object;
    void *child;

    if (chat_dialog_open != 0 || console_globals_data.active != 0 || chat_gui_find_object == 0) {
        return;
    }

    chat_scope_active = -1;

    if (chat_scope == 0) {
all_scope:
        {
            datum_index tag_id = tag_lookup(0x75737472 /* ustr */, (char *)"ui\\multiplayer_game_text");
            prompt_text = (tag_id == (datum_index)-1) ? (const void *)&empty_string
                                                       : (const void *)text_string_list_get_string();
            chat_scope_active = 0;
        }
    } else if (chat_scope == 1) {
        if (!game_engine_get_teams_enabled()) {
            goto all_scope;
        }
        goto team_scope;
    } else if (chat_scope == 2) {
        if (!game_engine_get_teams_enabled()) {
            goto all_scope;
        }
        {
            int32_t unit_index = chat_default_team_channel();
            int32_t player_index = player_get_vehicle((datum_index)unit_index);
            if (player_index != -1) {
                datum_index tag_id = tag_lookup(0x75737472 /* ustr */, (char *)"ui\\multiplayer_game_text");
                prompt_text = (tag_id == (datum_index)-1) ? (const void *)&empty_string
                                                           : (const void *)text_string_list_get_string();
                chat_scope_active = 2;
                goto gui_setup; // LAB_004aa826
            }
        }
team_scope:
        {
            datum_index tag_id;
            chat_scope_active = 1;
            tag_id = tag_lookup(0x75737472 /* ustr */, (char *)"ui\\multiplayer_game_text");
            prompt_text = (tag_id == (datum_index)-1) ? (const void *)&empty_string
                                                       : (const void *)text_string_list_get_string();
            if (chat_scope_active == -1) {
                return;
            }
        }
    } else {
        chat_scope_active = -1;
        return;
    }

gui_setup: // LAB_004aa826
    chat_gui_active = 1;
    gui_object = chat_gui_find_object(chat_gui_root_handle, chat_gui_find_object_arg);
    if (gui_object != 0) {
        child = chat_gui_find_child(gui_object, (const uint16_t *)L"oPrompt");
        if (child != 0) {
            keystone_control_set_attribute(child, (const uint16_t *)L"text", prompt_text);
        }
        child = chat_gui_find_child(gui_object, (const uint16_t *)L"oEditbox");
        if (child != 0) {
            int32_t zero[2] = {0, 0};
            chat_gui_set_focus(gui_object, child);
            keystone_control_set_attribute(child, (const uint16_t *)L"text", &empty_string);
            chat_gui_set_property_int(child, 0x201, 0, zero);
        }
        chat_gui_set_state(gui_object, 5);
        chat_gui_release(gui_object);
    }
    chat_dialog_open = 1;
    input_keyboard_set_capture_mode();
}

#if 0
Original Ghidra decompilation (0x4aa700):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl chimera__chat_open(int chat_scope)

{
  char cVar1;
  int iVar2;
  undefined *puVar3;
  int iVar4;
  undefined4 uStack_8;
  undefined4 uStack_4;

  if (DAT_006b3858 != '\0') {
    return;
  }
  if (DAT_006b7020 != '\0') {
    return;
  }
  if (DAT_00721eb8 == (code *)0x0) {
    return;
  }
  DAT_006b385c = 0xffffffff;
  if (chat_scope == 0) {
LAB_004aa7b6:
    iVar2 = tag_lookup("ui\\multiplayer_game_text");
    if (iVar2 == -1) {
      puVar3 = &DAT_00660c34;
      DAT_006b385c = 0;
    }
    else {
      puVar3 = (undefined *)text_string_list_get_string();
      DAT_006b385c = 0;
    }
  }
  else {
    if (chat_scope == 1) {
      cVar1 = game_engine_get_teams_enabled();
      if (cVar1 == '\0') goto LAB_004aa7b6;
    }
    else {
      if (chat_scope != 2) {
        DAT_006b385c = 0xffffffff;
        return;
      }
      cVar1 = game_engine_get_teams_enabled();
      if (cVar1 == '\0') goto LAB_004aa7b6;
      chat_default_team_channel();
      iVar2 = player_index_from_unit_index();
      if (iVar2 != -1) {
        iVar2 = tag_lookup("ui\\multiplayer_game_text");
        if (iVar2 == -1) {
          puVar3 = &DAT_00660c34;
          DAT_006b385c = 2;
        }
        else {
          puVar3 = (undefined *)text_string_list_get_string();
          DAT_006b385c = 2;
        }
        goto LAB_004aa826;
      }
    }
    DAT_006b385c = 1;
    iVar2 = tag_lookup("ui\\multiplayer_game_text");
    if (iVar2 == -1) {
      puVar3 = &DAT_00660c34;
    }
    else {
      puVar3 = (undefined *)text_string_list_get_string();
    }
    if (DAT_006b385c == -1) {
      return;
    }
  }
LAB_004aa826:
  _DAT_00721eec = 1;
  iVar2 = (*DAT_00721eb8)(DAT_00721ea4,PTR_PTR_0069c698);
  if (iVar2 != 0) {
    iVar4 = (*DAT_00721ecc)(iVar2,L"oPrompt");
    if (iVar4 != 0) {
      (*DAT_00721ee4)(iVar4,L"text",puVar3);
    }
    iVar4 = (*DAT_00721ecc)(iVar2,L"oEditbox");
    if (iVar4 != 0) {
      uStack_8 = 0;
      uStack_4 = 0;
      (*DAT_00721ed4)(iVar2,iVar4);
      (*DAT_00721ee4)(iVar4,L"text",&DAT_00660c34);
      (*DAT_00721ee8)(iVar4,0x201,0,&uStack_8);
    }
    (*DAT_00721edc)(iVar2,5);
    (*DAT_00721ec8)(iVar2);
  }
  DAT_006b3858 = 1;
  FUN_0048b650();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
