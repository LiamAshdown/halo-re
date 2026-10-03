/**
 * standalone/bridges.cpp -- the few symbols that need a link-level alias in the standalone exe.
 *
 * D3DXCreateEffect: the game creates its effects through the 2003 D3DX statically linked into the original exe; the
 *   standalone uses the June 2010 D3DX instead, through standalone/d3dx_compat.cpp, which wraps the effect in the 2003
 *   interface layout the C calls. The cdecl symbol _D3DXCreateEffect is an alias of standalone_d3dx_create_effect.
 * code_address_<fn>: C that hands a function's address to Windows (an APC, a window procedure) or to the I/O queue
 *   declares code_address_<fn>; the standalone binds it to the C
 *   function -- through a stdcall adapter where the C is cdecl but the caller pops like the original (ret 12). The
 *   symbols keep their undecorated cdecl names (_code_address_<fn>) through /alternatename, so a bare forward is the
 *   very address of its target and an adapter is the address of the stdcall function below.
 */
#pragma comment(linker, "/alternatename:_D3DXCreateEffect=_standalone_d3dx_create_effect")
