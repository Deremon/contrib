{
  $Id$
}
unit initc;

interface

{ 0.99.12 had a bug that initialization/finalization only worked for
  objfpc,delphi mode }
{$ifdef VER0_99_12}
  {$mode objfpc}
{$endif}

 {$LINKLIB cygwin}
 {$linklib kernel32}

{ this unit is just ment to run
  startup code to get C code to work correctly PM }


implementation

{$i textrec.inc}


procedure cygwin_crt0(p : pointer);cdecl;external;

{
procedure do_global_dtors;cdecl;external;
 this does not work because
 do_global_dtors is a static C function PM
 it is inserted into the atexit chain,
 but how do we call this from FPC ???
 it seems to be done in exit function
 but that one ends with _exit that is system dependent !! }

{ avoid loading of cygwin _exit code
  so that exit returns }
procedure _exit(status : longint);cdecl;
begin
end;

procedure C_exit(status : longint);popstack;external name '_exit';

const
   STD_INPUT_HANDLE = $fffffff6;
   STD_OUTPUT_HANDLE = $fffffff5;
   STD_ERROR_HANDLE = $fffffff4;

function GetStdHandle(nStdHandle:DWORD):longint;external 'kernel32' name 'GetStdHandle';

procedure UpdateStdHandle(var t:TextRec;var stdHandle:longint;newHandle:longint);
{ Check if the stdHandle is the same as the one in the TextRec, then
  also update the TextRec }
begin
  if t.Handle=stdHandle then
   t.Handle:=newHandle;
  stdHandle:=newHandle;
end;


initialization
  cygwin_crt0(nil);
{ Reinitialize std handles that can be changed }
  UpdateStdHandle(TextRec(Input),StdInputHandle,GetStdHandle(STD_INPUT_HANDLE));
  UpdateStdHandle(TextRec(Output),StdOutputHandle,GetStdHandle(STD_OUTPUT_HANDLE));
  TextRec(StdOut).Handle:=StdOutputHandle;
  UpdateStdHandle(TextRec(Stderr),StdErrorHandle,GetStdHandle(STD_ERROR_HANDLE));

finalization
{ should we pass exit code ?
  its apparently only used by _exit so it doesn't matter PM }
  C_exit(0);

end.
{
  $Log$
  Revision 1.1  2002/02/19 08:26:25  sasu
  Initial revision

  Revision 1.1.2.1  2000/12/30 17:49:48  peter
    * update std handles after initializing c

}
