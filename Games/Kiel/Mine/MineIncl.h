#include "../prec.c"

extern struct ExecBase *SysBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Window *Window;
struct RastPort *rp;
struct IntuiMessage *msg;

ULONG iflags;

#define StopMsg() ModifyIDCMP(Window,0L)
#define ContMsg() ModifyIDCMP(Window,iflags)
#define clearwin() EraseRect(rp,0,0,Window->Width-5,Window->Height-11)

void open_lib()
{
  IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library",0L);
  GfxBase = (struct GfxBase *) OpenLibrary("graphics.library",0L);
}

void open_window(struct NewWindow *newwindow)
{
  Window = (struct Window *) OpenWindow(newwindow);
  rp=Window->RPort;
  iflags=Window->IDCMPFlags;
  ActivateWindow(Window);
}

void write_text(LONG x, LONG y, char *text, ULONG color)
{
SetAPen(rp,color);
  Move(rp, x, y);
  Text(rp, text, strlen(text));
}

void drawfield(LONG x1, LONG y1, LONG x2, LONG y2)
{
SetAPen(rp,2);
  Move(rp,x1,y2);
  Draw(rp,x1,y1);
  Draw(rp,x2,y1);
SetAPen(rp,1);
  Move(rp,x2,y1);
  Draw(rp,x2,y2);
  Draw(rp,x1,y2);
}

/* AROS unimplemented
void copypic(x1,y1,dx,dy,x2,y2)
{
ClipBlit(rp,x1,y1,rp,x2,y2,dx,dy,192);
} */

void WinSize(struct Window *Win, LONG x, LONG y)
{
LONG altx,alty;
  altx=Win->Width;
  alty=Win->Height;
  Win->LeftEdge=10;
  Win->TopEdge=10;
  SizeWindow(Win,x-altx,y-alty);
}

void close_window()
{
  CloseWindow(Window);
}

void close_lib()
{
  CloseLibrary((struct Library *)IntuitionBase);
  CloseLibrary((struct Library *)GfxBase);
}
