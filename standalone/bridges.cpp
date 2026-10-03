/**
 * standalone/bridges.cpp -- the few symbols whose meaning differs between the standalone exe and the hook harness
 * (harness/, where the rewritten C runs inside the retail game).
 *
 * D3DXCreateEffect: the game creates its effects through the 2003 D3DX statically linked into the original exe; the
 *   standalone uses the June 2010 D3DX instead, through standalone/d3dx_compat.cpp, which wraps the effect in the 2003
 *   interface layout the C calls. The cdecl symbol _D3DXCreateEffect is an alias of standalone_d3dx_create_effect.
 * code_address_<fn>: C that hands a function's address to Windows (an APC, a window procedure) or to the I/O queue
 *   declares code_address_<fn>; the harness binds it to the original function's address, the standalone to the C
 *   function -- through a stdcall adapter where the C is cdecl but the caller pops like the original (ret 12). The
 *   symbols keep their undecorated cdecl names (_code_address_<fn>) through /alternatename, so a bare forward is the
 *   very address of its target and an adapter is the address of the stdcall function below.
 */
#include <stdint.h>

extern "C" {

/**
 * An I/O completion routine (the original at 0x443b00 ends ret 12): Windows calls it with three stack arguments and
 * the routine pops them; the C function is cdecl, so the adapter forwards the same three arguments and returns
 * with the callee-popped convention.
 */
void cache_io_completion_routine(uint32_t error_code, uint32_t bytes_transferred, void *overlapped);

void __stdcall halo_bridge_cache_io_completion_routine(uint32_t error_code, uint32_t bytes_transferred, void *overlapped)
{
    cache_io_completion_routine(error_code, bytes_transferred, overlapped);
}

}

#pragma comment(linker, "/alternatename:_D3DXCreateEffect=_standalone_d3dx_create_effect")
#pragma comment(linker, "/alternatename:_code_address_cache_io_completion_routine=_halo_bridge_cache_io_completion_routine@12")
#pragma comment(linker, "/alternatename:_code_address_cache_io_sound_decode_thunk=_cache_io_sound_decode_thunk")
#pragma comment(linker, "/alternatename:_code_address_shell_window_procedure=_shell_window_procedure@16")
