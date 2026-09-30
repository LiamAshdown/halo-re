// network_join_status_text_update  (Ghidra: FUN_004db4c0; renamed, no prior name)
// address 0x4db4c0, size 359 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Updates the on-screen join/connect
// status text ('Loading', 'Connecting', or an animated 'Connecting...') and the associated UI
// state machine based on an integer mode selector").
// register convention: the mode selector arrives in EAX (in_EAX), the client pointer in ESI
// (unaff_ESI). // blam-cc: EAX -> mode, ESI -> client
// UNSURE: none.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern void console_printf_verbose(const char *format, ...); // 0x496a80
extern int32_t interface_loading_screen_progress; // 0x00718f90
extern int32_t join_ui_state; // 0x00718f8c
extern int16_t network_game_mode; // 0x00719720
extern int32_t interface_loading_screen_request_id; // 0x0068e688
extern const char network_ellipsis_dots[]; // 0x00697e7c, "................"


// blam-cc: EAX -> mode, ESI -> client
void network_join_status_text_update(int32_t mode, network_client_globals *client)
{
    network_connection_attempt_state *attempt = &client->connect_attempt;
    char dots[17];
    int32_t count;

    if (mode == 0) {
        attempt->elapsed_counter = 0;
        console_printf_verbose("Connecting");
        interface_loading_screen_progress = 0;
        join_ui_state = 5;
    } else if (mode == 1) {
        memset(dots, 0, sizeof(dots));
        count = attempt->elapsed_counter + 1;
        attempt->elapsed_counter = count;
        if (count < 0) {
            count = 0;
        } else if (count > 0x10) {
            count = 0x10;
        }
        strncpy(dots, network_ellipsis_dots, count);
        console_printf_verbose("Connecting%s", dots);
        interface_loading_screen_progress = attempt->elapsed_counter;
        if (join_ui_state != 1 && join_ui_state != 2) {
            if (join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
                return;
            }
            join_ui_state = 6;
            return;
        }
    } else {
        attempt->elapsed_counter = 0;
        console_printf_verbose("Loading");
        interface_loading_screen_progress = 0;
        if (network_game_mode == 2) {
            if (join_ui_state != 1) {
                if (join_ui_state != 2 && join_ui_state == 4) {
                    interface_loading_screen_request_id = -1;
                }
                join_ui_state = 8;
                return;
            }
        } else if (join_ui_state != 1 && join_ui_state != 2) {
            if (join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
                interface_loading_screen_progress = 0;
                return;
            }
            join_ui_state = 7;
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4db4c0):

void FUN_004db4c0(void)

{
  int in_EAX;
  size_t _Count;
  int unaff_ESI;
  char local_14 [20];

  if (in_EAX == 0) {
    *(undefined4 *)(unaff_ESI + 0xae8) = 0;
    FUN_00496a80("Connecting");
    DAT_00718f90 = 0;
    DAT_00718f8c = 5;
  }
  else if (in_EAX == 1) {
    local_14[0] = '\0';
    local_14[1] = '\0';
    local_14[2] = '\0';
    local_14[3] = '\0';
    local_14[0xc] = '\0';
    local_14[0xd] = '\0';
    local_14[0xe] = '\0';
    local_14[0xf] = '\0';
    local_14[4] = '\0';
    local_14[5] = '\0';
    local_14[6] = '\0';
    local_14[7] = '\0';
    _Count = *(int *)(unaff_ESI + 0xae8) + 1;
    local_14[8] = '\0';
    local_14[9] = '\0';
    local_14[10] = '\0';
    local_14[0xb] = '\0';
    local_14[0x10] = 0;
    *(size_t *)(unaff_ESI + 0xae8) = _Count;
    if ((int)_Count < 0) {
      _Count = 0;
    }
    else if (0x10 < (int)_Count) {
      _Count = 0x10;
    }
    _strncpy(local_14,s__________________00697e7c,_Count);
    FUN_00496a80("Connecting%s",local_14);
    DAT_00718f90 = *(undefined4 *)(unaff_ESI + 0xae8);
    if ((DAT_00718f8c != 1) && (DAT_00718f8c != 2)) {
      if (DAT_00718f8c == 4) {
        DAT_0068e688 = 0xffffffff;
        return;
      }
      DAT_00718f8c = 6;
      return;
    }
  }
  else {
    *(undefined4 *)(unaff_ESI + 0xae8) = 0;
    FUN_00496a80("Loading");
    DAT_00718f90 = 0;
    if (DAT_00719720 == 2) {
      if (DAT_00718f8c != 1) {
        if ((DAT_00718f8c != 2) && (DAT_00718f8c == 4)) {
          DAT_0068e688 = 0xffffffff;
        }
        DAT_00718f8c = 8;
        return;
      }
    }
    else if ((DAT_00718f8c != 1) && (DAT_00718f8c != 2)) {
      if (DAT_00718f8c == 4) {
        DAT_0068e688 = 0xffffffff;
        DAT_00718f90 = 0;
        return;
      }
      DAT_00718f8c = 7;
      return;
    }
  }
  return;
}
#endif
