{
    $Id$
    This file is part of the Free Pascal run time library.
    Copyright (c) 1999-2000 by the Free Pascal development team

    See the file COPYING.FPC, included in this distribution,
    for details about the copyright.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 **********************************************************************}
{$mode objfpc}
unit Sockets;

Interface

  Uses
     windows,winsock;

  Const
     AF_MAX          = WinSock.AF_MAX;
     PF_MAX          = AF_MAX;

{$i socketsh.inc}

Implementation

{ Include filerec and textrec structures }
{$i filerec.inc}
{$i textrec.inc}

{******************************************************************************
                          Basic Socket Functions
******************************************************************************}

Function socket(Domain,SocketType,Protocol:Longint):Longint;
begin
  Socket:=WinSock.Socket(Domain,SocketType,ProtoCol);
  if Socket<0 then
    SocketError:=WSAGetLastError
  else
    SocketError:=0;
end;

Function Send(Sock:Longint;Var Addr;AddrLen,Flags:Longint):Longint;
begin
  Send:=WinSock.Send(Sock,Addr,AddrLen,Flags);
  if Send<0 then
    SocketError:=WSAGetLastError
  else
    SocketError:=0;
end;

Function Recv(Sock:Longint;Var Addr;AddrLen,Flags:Longint):Longint;
begin
  Recv:=WinSock.Recv(Sock,Addr,AddrLen,Flags);
  if Recv<0 then
    SocketError:=WSAGetLastError
  else
    SocketError:=0;
end;

Function Bind(Sock:Longint;Var Addr;AddrLen:Longint):Boolean;

  var
     l : longint;

begin
  l:=WinSock.Bind(Sock,WinSock.PSockAddr(@Addr),AddrLen);
  if l<0 then
    begin
       SocketError:=WSAGetLastError;
       Bind:=false;
    end
  else
    begin
       SocketError:=0;
       Bind:=true;
    end;
end;

Function Listen(Sock,MaxConnect:Longint):Boolean;

  var
     l : longint;

begin
  l:=WinSock.Listen(Sock,MaxConnect);
  if l<0 then
    begin
       SocketError:=WSAGetLastError;
       Listen:=false;
    end
  else
    begin
       SocketError:=0;
       Listen:=true;
    end;
end;

Function Accept(Sock:Longint;Var Addr;Var Addrlen:Longint):Longint;
begin
  Accept:=WinSock.Accept(Sock,WinSock.PSockAddr(@Addr),plongint(@AddrLen));
  if Accept<0 then
    SocketError:=WSAGetLastError
  else
    SocketError:=0;
end;

Function Connect(Sock:Longint;Var Addr;Addrlen:Longint):Boolean;

begin
  Connect:=WinSock.Connect(Sock,WinSock.TSockAddr(Addr),AddrLen)=0;
  if not Connect then
    SocketError:=WSAGetLastError
  else
    SocketError:=0;
end;

Function Shutdown(Sock:Longint;How:Longint):Longint;
begin
  ShutDown:=WinSock.ShutDown(Sock,How);
  if ShutDown<0 then
    SocketError:=WSAGetLastError
  else
    SocketError:=0;
end;

Function GetSocketName(Sock:Longint;Var Addr;Var Addrlen:Longint):Longint;
begin
  GetSocketName:=WinSock.GetSockName(Sock,WinSock.TSockAddr(Addr),AddrLen);
  if GetSocketName<0 then
    SocketError:=WSAGetLastError
  else
    SocketError:=0;
end;

Function GetPeerName(Sock:Longint;Var Addr;Var Addrlen:Longint):Longint;
begin
  GetPeerName:=WinSock.GetPeerName(Sock,WinSock.TSockAddr(Addr),AddrLen);
  if GetPeerName<0 then
    SocketError:=WSAGetLastError
  else
    SocketError:=0;
end;

Function SetSocketOptions(Sock,Level,OptName:Longint;Var OptVal;optlen:longint):Longint;
begin
  SetSocketOptions:=WinSock.SetSockOpt(Sock,Level,OptName,OptVal,OptLen);
  if SetSocketOptions<0 then
    SocketError:=WSAGetLastError
  else
    SocketError:=0;
end;

Function GetSocketOptions(Sock,Level,OptName:Longint;Var OptVal;Var optlen:longint):Longint;
begin
  GetSocketOptions:=WinSock.GetSockOpt(Sock,Level,OptName,OptVal,OptLen);
  if GetSocketOptions<0 then
    SocketError:=WSAGetLastError
  else
    SocketError:=0;
end;

Function SocketPair(Domain,SocketType,Protocol:Longint;var Pair:TSockArray):Longint;
begin
  // SocketPair:=SocketCall(Socket_Sys_SocketPair,Domain,SocketType,Protocol,longint(@Pair),0,0);
end;


{ mimic the linux fdWrite/fdRead calls for the file/text socket wrapper }
function fdWrite(handle : longint;var bufptr;size : dword) : dword;
begin
  fdWrite := WinSock.send(handle, bufptr, size, 0);
  if fdWrite = SOCKET_ERROR then
  begin
    SocketError := WSAGetLastError;
    fdWrite := 0;
  end
  else
    SocketError := 0;
end;

function fdRead(handle : longint;var bufptr;size : dword) : dword;
  var
     d : dword;

  begin
     if ioctlsocket(handle,FIONREAD,@d) = SOCKET_ERROR then
       begin
         SocketError:=WSAGetLastError;
         fdRead:=0;
         exit;
       end;
     if d>0 then
       begin
         if size>d then
           size:=d;
         fdRead := WinSock.recv(handle, bufptr, size, 0);
         if fdRead = SOCKET_ERROR then
         begin
           SocketError:= WSAGetLastError;
           fdRead := 0;
         end else
           SocketError:=0;
       end
     else
       SocketError:=0;
  end;


{$i sockets.inc}

{ winsocket stack needs an init. and cleanup code }
var
  wsadata : twsadata;

initialization
  WSAStartUp($2,wsadata);
finalization
  WSACleanUp;
end.
{
  $Log$
  Revision 1.1  2002/02/19 08:26:27  sasu
  Initial revision

  Revision 1.1.2.2  2000/07/28 08:36:16  sg
  * Applied patch by Markus Kaemmerer: Fixes fdRead and fdWrite

  Revision 1.1.2.1  2000/07/28 06:33:27  sg
  * Applied patch to "Connect" by Markus Kaemmerer: WinSock.Connect returns
    zero when it succeeded, and not vice versa.

  Revision 1.1  2000/07/13 06:31:22  michael
  + Initial import

  Revision 1.7  2000/06/21 22:26:47  pierre
   * use ioctlsocket in fdRead

  Revision 1.6  2000/06/19 13:32:18  michael
  + Corrected GetSocketOptions

  Revision 1.5  2000/02/09 16:59:34  peter
    * truncated log

  Revision 1.4  2000/01/07 16:41:52  daniel
    * copyright 2000

  Revision 1.3  2000/01/07 16:32:34  daniel
    * copyright 2000 added

}