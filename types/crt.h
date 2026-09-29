#pragma once
/* crt.h -- the C runtime, from its own headers.

   Source files that call the C runtime include this instead of declaring runtime functions themselves (the
   decompiler's names for the game's statically linked CRT -- _strncpy, __stricmp, _fopen, operator_new -- are the
   standard ones: strncpy, _stricmp, fopen, malloc); the standalone link takes them from libcmt. */
#ifndef HALO_CRT_H
#define HALO_CRT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <time.h>
#include <locale.h>
#include <process.h>
#include <float.h>

#endif
