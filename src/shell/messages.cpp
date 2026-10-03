#include "halo/shell/messages.hpp"

namespace halo::shell {

/**
 * Looks the message up by id. The ids are stable identifiers shared with the engine's error paths and with the text
 * ids carried in network messages; the wording is ours.
 */
const char *shell_message(uint32_t id)
{
    switch (id) {
    case 0x65: return "There is not enough memory (RAM or virtual memory) to run the game. Close other programs and try again.";
    case 0x66: return "The processor is slower than the game requires. The game may run poorly.";
    case 0x67: return "The video hardware is below the minimum the game recommends. The game may run poorly.";
    case 0x68: return "The video driver has not been tested with this game. A newer driver may be available from the hardware maker.";
    case 0x69: return "The video driver is known to have serious problems with this game. Install a newer driver.";
    case 0x6a: return "The game did not exit correctly last time. Starting in safe mode is recommended.";
    case 0x6b: return "DirectX 9 is required but could not be found. Install DirectX 9 and run the game again.";
    case 0x6c: return "The video hardware has less video memory than the game recommends. The game may run poorly.";
    case 0x6d: return "There is less than 100 MB of free disk space. Free some space to play.";
    case 0x77: return "Exception!";
    case 0x78: return "Gathering Exception Data...";
    case 0x79: return "DirectDraw could not be initialised. Hardware acceleration may be disabled; run DXDIAG.";
    case 0x7b: return "DirectSound could not be initialised.";
    case 0x7c: return "DirectInput could not be initialised.";
    case 0x7d: return "SHFolder.dll is missing. Reinstalling the system components is recommended.";
    case 0x7f: return "Halo - Warning";
    case 0x80: return "Halo - Fatal Error";
    case 0x81: return "Direct3D could not be initialised. Hardware acceleration may be disabled; run DXDIAG.";
    case 0x83: return "Windowed mode needs a 32-bit colour setting. Change the desktop colour depth and run the game again.";
    case 0x87: return "The control key was held while the game started, so safe mode was requested.";
    case 0x89: return "A game file is missing or damaged. Reinstalling the game is recommended.";
    case 0x8b: return "The player's profile files could not be accessed. Check that the profile folder exists and can be written.";
    case 0x8d: return "The anti-aliasing override set in the graphics control panel is not supported. Set it to application controlled.";
    case 0x8e: return "The sound driver has not been tested with this game. A newer driver may be available.";
    case 0x8f: return "The sound driver is known to have serious problems with this game. Install a newer driver.";
    case 0x91: return "Autobalance: team change request denied";
    case 0x92: return "Another copy of the game is already running on this machine.";
    case 0x93: return "The video card looks like a prototype. Only retail video cards are supported.";
    case 0xa0: return "The product key is invalid.";
    default: return nullptr;
    }
}

/**
 * The command-line switches the engine understands, shown for -help and -?.
 */
const char *shell_usage_text()
{
    return "-window / -windowed   Run in a window\n"
           "-nosound              Disable all sound\n"
           "-novideo              Disable video playback\n"
           "-nojoystick           Disable joysticks and gamepads\n"
           "-nonetwork            Disable networking\n"
           "-nowinkey             Disable the Windows key\n"
           "-safemode             Start with minimal graphics options\n"
           "-width640             Use the 640-wide layout\n"
           "-screenshot           Enable screenshots\n"
           "-ip <address>         Server address to connect to\n"
           "-port <port>          Server port\n"
           "-cport <port>         Client port\n"
           "-checkfpu             Check the floating point state\n"
           "-timedemo             Run the time demo\n";
}

}  // namespace halo::shell
