#ifdef USE_PROTO_INCLUDES
#include <libraries/gadtools.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>

#include <string.h>
#include <stdio.h>

#endif

#include "filexstructs.h"
#include "allprotos.h"
#include "filexstrings.h"

static ULONG grabstart, grabend;

void GrabMemory( ULONG start, ULONG end, struct DisplayData *DD )
{
/*	if(start>=0xf00000)return;*/
/*	if(end>0xf00000)end=0xf00000;*/

	struct FileData *FD;

	if(start>end)
	{
		DisplayLocaleText(MSG_INFO_GRAB_ENDBEFORESTART);
		return;
	}
	if(start==end)
	{
		DisplayLocaleText(MSG_INFO_GRAB_STARTEQUALTOEND);
		return;
	}

	grabstart = start;
	grabend = end;

	FD = DD->FD;

	if( ElementZahl( &FD->DisplayList ) == 1 )
		FreeFD( FD );
	else
		Remove( &DD->FNode );

	FD = AllocFD( FD_GRAB );
	AddTail( &FD->DisplayList, &DD->FNode );
	DD->FD = FD;

	FD->Len = FD->RLen = end - start;
	FD->Mem = ( UBYTE * )start;

	if( DD->DI->Wnd )
	{
		CursorOff( DD );
		DD->SPos = 0;
		DD->CPos = 0;
		UpdateStatusZeile( DD );
		SetScrollerGadget( DD );
		RedrawDisplay( DD );
		CursorOn( DD );
	}
}

/*
 * int DoGrabWindow(void)
 *
 * GrabWindowAbfrage. Gibt zur�ck, wie Window verlassen wurde
 *                                   (Cancel=FALSE/Okay=TRUE)
 */

enum {
GD_GRAB_START,
GD_GRAB_END,
GD_GRAB_GRAB,
GD_GRAB_CANCEL
};

struct MyNewGadget GrabNewGadgets[] =
{
	{HEX_KIND, 0, 0, 0, MSG_GADGET_GRAB_START, 0, 8, 0, 9, 0},
	{HEX_KIND, 0, 0, 0, MSG_GADGET_GRAB_END, 0, 8, 0, 9, 0},

	{BUTTON_KIND, GP_LEFTBOTTOM, 0, 0, MSG_GADGET_OK, 0, 0, 0, 0, 0},
	{BUTTON_KIND, GP_RIGHTBOTTOM, 0, 0, MSG_GADGET_CANCEL, 0, 0, 0, 0, 0},
	{0}
};

static struct WindowData GrabWD =
{
	NULL,	NULL,	FALSE, NULL, NULL,
	0,0,
	&GrabNewGadgets[ 0 ], 4
};

static void DoGrabWndMsg(void)
{
	struct IntuiMessage	*m, Msg;
	struct Gadget *gad;

	WORD wie = -1;		/* Wie wurde Window verlassen ? */

	while(( m = GT_GetIMsg( GrabWD.Wnd->UserPort )))
	{
		CopyMem(( char * )m, ( char * )&Msg, (long)sizeof( struct IntuiMessage ));

		GT_ReplyIMsg( m );

		KeySelect( GrabWD.Gadgets, &Msg);

		gad=(struct Gadget *)Msg.IAddress;

		switch ( Msg.Class )
		{
			case	IDCMP_REFRESHWINDOW:
				GT_BeginRefresh( GrabWD.Wnd );
				GT_EndRefresh( GrabWD.Wnd, TRUE );
				break;

			case	IDCMP_VANILLAKEY:
				if( Msg.Code == 13)
				{
					wie = 1;
				}
				break;

			case	IDCMP_CLOSEWINDOW:
				wie = 0;
				break;

			case	IDCMP_GADGETUP:
				switch(gad->GadgetID)
				{
					case GD_GRAB_GRAB:
						wie = 1;
						break;

					case GD_GRAB_CANCEL:
						wie = 0;
						break;

					case GD_GRAB_START:
						if(!IsHexString(GetString(GrabWD.Gadgets[GD_GRAB_START])))
						{
							MyRequest( MSG_INFO_GLOBAL_ILLEGALCHARACTERS, ( ULONG )GetString( GrabWD.Gadgets[ GD_GRAB_START ]));
							ActivateGadget(GrabWD.Gadgets[GD_GRAB_START],GrabWD.Wnd,0);
						}
						else
							ActivateGadget(GrabWD.Gadgets[GD_GRAB_END],GrabWD.Wnd,0);
						break;

					case GD_GRAB_END:
						if(!IsHexString(GetString(GrabWD.Gadgets[GD_GRAB_END])))
						{
							MyRequest( MSG_INFO_GLOBAL_ILLEGALCHARACTERS, ( ULONG )GetString( GrabWD.Gadgets[ GD_GRAB_END ]));
							ActivateGadget(GrabWD.Gadgets[GD_GRAB_END],GrabWD.Wnd,0);
						}
						break;
				}
				break;
		}
	}

	if( wie == 1 )
	{
		ULONG start, end;

		stch_l( GetString( GrabWD.Gadgets[ GD_GRAB_START ] ), (LONG *)&start);
		stch_l( GetString( GrabWD.Gadgets[ GD_GRAB_END ] ), (LONG *)&end);

		GrabMemory( start, end, AktuDD );
	}

	if( wie != -1 )
	{
		MyRemoveSignal( 1L << GrabWD.Wnd->UserPort->mp_SigBit );
		NewCloseAWindow( &GrabWD );
	}
}

void OpenGrabWindow( void )
{
	long err;
	UBYTE GrabStartString[9], GrabEndString[9];

	if( GrabWD.Wnd )
	{
		ActivateWindow( GrabWD.Wnd );
		return;
	}

	if( AktuDD->FD->Typ == FD_GRAB )
	{
		grabstart = ( ULONG )AktuDD->FD->Mem;
		grabend = ( ULONG )AktuDD->FD->Mem + AktuDD->FD->Len;
	}

	if((grabstart==0)&&(grabend==0))
	{
		GrabNewGadgets[GD_GRAB_START].CurrentValue = 
		GrabNewGadgets[GD_GRAB_END  ].CurrentValue = 0;
	}
	else
	{
		GrabNewGadgets[GD_GRAB_START].CurrentValue = (LONG) GrabStartString;
		GrabNewGadgets[GD_GRAB_END  ].CurrentValue = (LONG) GrabEndString;

		sprintf(GrabStartString, "%08.lx", grabstart);
		sprintf(GrabEndString, "%08.lx", grabend);
	}

	if(( err = NewOpenAWindow( &GrabWD, GetStr( MSG_WINDOWTITLE_GRAB ))))
		MyRequest( MSG_INFO_GLOBAL_CANTOPENWINDOW, err);
	else
	{
		ActivateGadget( GrabWD.Gadgets[ GD_GRAB_START ], GrabWD.Wnd, 0);

		MyAddSignal( 1L << GrabWD.Wnd->UserPort->mp_SigBit, &DoGrabWndMsg );
	}
}
