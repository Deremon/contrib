/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Regina initialization code.
    Lang: English
*/

#define AROS_ALMOST_COMPATIBLE
#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/alib.h>

#include "regina_intern.h"
#include "libdefs.h"

/****************************************************************************************/

#undef SysBase

/* Customize libheader.c */
#define LC_LIBBASESIZE        sizeof(LIBBASETYPE)

/* #define LC_NO_INITLIB    */
/* #define LC_NO_OPENLIB    */
/* #define LC_NO_CLOSELIB   */
/* #define LC_NO_EXPUNGELIB */

#include <libcore/libheader.c>

struct ExecBase* SysBase;
struct DosLibrary* DOSBase;
struct Library* aroscbase;
struct MinList *__regina_tsdlist = NULL;

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR ReginaBase)
{
    D(bug("Inside Init func of regina.library\n"));

    SysBase = LC_SYSBASE_FIELD(ReginaBase);
    if (!(DOSBase = OpenLibrary("dos.library",0))) return FALSE;

    if (!(aroscbase = OpenLibrary("arosc.library",41)))
    {
        CloseLibrary((struct Library *)DOSBase);
        return FALSE;
    }

    __regina_semaphorepool = CreatePool(MEMF_PUBLIC, 1024, 256);
  
    __regina_tsdlist = (struct MinList *)AllocPooled (__regina_semaphorepool, sizeof(struct MinList));
    NewList((struct List *)__regina_tsdlist);

    return TRUE;
}

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR ReginaBase)
{
    D(bug("Inside Open func of regina.library\n"));

    return TRUE;
}

/****************************************************************************************/

void  SAVEDS STDARGS LC_BUILDNAME(L_CloseLib) (LC_LIBHEADERTYPEPTR ReginaBase)
{
    D(bug("Inside Close func of regina.library\n"));
}

/****************************************************************************************/

void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR ReginaBase)
{
    D(bug("Inside Expunge func of regina.library\n"));

    DeletePool(__regina_semaphorepool);
    CloseLibrary(aroscbase);
    CloseLibrary((struct Library *)DOSBase);
}

/****************************************************************************************/
