{
    $Id$
    This file is part of the Free Pascal run time library.
    Copyright (c) 1999-2000 by Nils Sjoholm and Carl Eric Codere
    members of the Free Pascal development team
      Date conversion routine taken from SWAG

    See the file COPYING.FPC, included in this distribution,
    for details about the copyright.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 **********************************************************************}

Unit Dos;


{--------------------------------------------------------------------}
{ LEFT TO DO:                                                        }
{--------------------------------------------------------------------}
{ o DiskFree / Disksize don't work as expected                       }
{ o Implement SetDate and SetTime                                    }
{ o Implement EnvCount,EnvStr                                        }
{ o FindFirst should only work with correct attributes               }
{--------------------------------------------------------------------}




Interface

{$I os.inc}


Const
  {Bitmasks for CPU Flags}
  fcarry     = $0001;
  fparity    = $0004;
  fauxiliary = $0010;
  fzero      = $0040;
  fsign      = $0080;
  foverflow  = $0800;

  {Bitmasks for file attribute}
  readonly  = $01;
  hidden    = $02;
  sysfile   = $04;
  volumeid  = $08;
  directory = $10;
  archive   = $20;
  anyfile   = $3F;

  {File Status}
  fmclosed = $D7B0;
  fminput  = $D7B1;
  fmoutput = $D7B2;
  fminout  = $D7B3;


Type
  ComStr  = String[255];  { size increased to be more compatible with Unix}
  PathStr = String[255];  { size increased to be more compatible with Unix}
  DirStr  = String[255];  { size increased to be more compatible with Unix}
  NameStr = String[255];  { size increased to be more compatible with Unix}
  ExtStr  = String[255];  { size increased to be more compatible with Unix}



{
  filerec.inc contains the definition of the filerec.
  textrec.inc contains the definition of the textrec.
  It is in a separate file to make it available in other units without
  having to use the DOS unit for it.
}
{$i filerec.inc}
{$i textrec.inc}


Type

  SearchRec = Packed Record
    { watch out this is correctly aligned for all processors }
    { don't modify.                                          }
    { Replacement for Fill }
{0} AnchorPtr : Pointer;    { Pointer to the Anchorpath structure }
{4} Fill: Array[1..15] of Byte; {future use}
    {End of replacement for fill}
    Attr : BYTE;        {attribute of found file}
    Time : LongInt;     {last modify date of found file}
    Size : LongInt;     {file size of found file}
    Name : String[255]; {name of found file}
  End;


  DateTime = packed record
    Year: Word;
    Month: Word;
    Day: Word;
    Hour: Word;
    Min: Word;
    Sec: word;
  End;



Var
  DosError : integer;

{Interrupt}
{Procedure Intr(intno: byte; var regs: registers);
Procedure MSDos(var regs: registers);}

{Info/Date/Time}
Function  DosVersion: Word;
Procedure GetDate(var year, month, mday, wday: word);
Procedure GetTime(var hour, minute, second, sec100: word);
procedure SetDate(year,month,day: word);
Procedure SetTime(hour,minute,second,sec100: word);
Procedure UnpackTime(p: longint; var t: datetime);
Procedure PackTime(var t: datetime; var p: longint);

{Exec}
Procedure Exec(const path: pathstr; const comline: comstr);
Function  DosExitCode: word;

{Disk}
Function  DiskFree(drive: byte) : longint;
Function  DiskSize(drive: byte) : longint;
Procedure FindFirst(path: pathstr; attr: word; var f: searchRec);
Procedure FindNext(var f: searchRec);
Procedure FindClose(Var f: SearchRec);

{File}
Procedure GetFAttr(var f; var attr: word);
Procedure GetFTime(var f; var time: longint);
Function  FSearch(path: pathstr; dirlist: string): pathstr;
Function  FExpand(path: pathstr): pathstr;
Procedure FSplit(path: pathstr; var dir: dirstr; var name: namestr; var ext: extstr);

{Environment}
Function  EnvCount: longint;
Function  EnvStr(index: integer): string;
Function  GetEnv(envvar: string): string;

{Misc}
Procedure SetFAttr(var f; attr: word);
Procedure SetFTime(var f; time: longint);
Procedure GetCBreak(var breakvalue: boolean);
Procedure SetCBreak(breakvalue: boolean);
Procedure GetVerify(var verify: boolean);
Procedure SetVerify(verify: boolean);

{Do Nothing Functions}
Procedure SwapVectors;
Procedure GetIntVec(intno: byte; var vector: pointer);
Procedure SetIntVec(intno: byte; vector: pointer);
Procedure Keep(exitcode: word);

implementation

const
  DaysPerMonth :  Array[1..12] of ShortInt =
(031,028,031,030,031,030,031,031,030,031,030,031);
  DaysPerYear  :  Array[1..12] of Integer  =
(031,059,090,120,151,181,212,243,273,304,334,365);
  DaysPerLeapYear :    Array[1..12] of Integer  =
(031,060,091,121,152,182,213,244,274,305,335,366);
  SecsPerYear      : LongInt  = 31536000;
  SecsPerLeapYear  : LongInt  = 31622400;
  SecsPerDay       : LongInt  = 86400;
  SecsPerHour      : Integer  = 3600;
  SecsPerMinute    : ShortInt = 60;
  TICKSPERSECOND    = 50;



Type
    pClockData = ^tClockData;
    tClockData = packed Record
      sec   : Word;
      min   : Word;
      hour  : Word;
      mday  : Word;
      month : Word;
      year  : Word;
      wday  : Word;
    END;

    BPTR     = Longint;
    BSTR     = Longint;

  pMinNode = ^tMinNode;
  tMinNode = Packed Record
    mln_Succ,
    mln_Pred  : pMinNode;
  End;


    pMinList = ^tMinList;
    tMinList = Packed record
    mlh_Head        : pMinNode;
    mlh_Tail        : pMinNode;
    mlh_TailPred    : pMinNode;
    end;
{ *  List Node Structure.  Each member in a list starts with a Node * }

  pNode = ^tNode;
  tNode = Packed Record
    ln_Succ,                { * Pointer to next (successor) * }
    ln_Pred  : pNode;       { * Pointer to previous (predecessor) * }
    ln_Type  : Byte;
    ln_Pri   : Shortint;        { * Priority, for sorting * }
    ln_Name  : PCHAR;       { * ID string, null terminated * }
  End;  { * Note: Integer aligned * }



    pList = ^tList;
    tList = Packed record
    lh_Head     : pNode;
    lh_Tail     : pNode;
    lh_TailPred : pNode;
    lh_Type     : Byte;
    l_pad       : Byte;
    end;


   pMsgPort = ^tMsgPort;
    tMsgPort = Packed record
    mp_Node     : tNode;
    mp_Flags    : Byte;
    mp_SigBit   : Byte;     { signal bit number    }
    mp_SigTask  : Pointer;   { task to be signalled (TaskPtr) }
    mp_MsgList  : tList;     { message linked list  }
    end;


  pTask = ^tTask;
    tTask = Packed record
        tc_Node         : tNode;
        tc_Flags        : Byte;
        tc_State        : Byte;
        tc_IDNestCnt    : Shortint;         { intr disabled nesting         }
        tc_TDNestCnt    : Shortint;         { task disabled nesting         }
        tc_SigAlloc     : Cardinal;        { sigs allocated                }
        tc_SigWait      : Cardinal;        { sigs we are waiting for       }
        tc_SigRecvd     : Cardinal;        { sigs we have received         }
        tc_SigExcept    : Cardinal;        { sigs we will take excepts for }
        tc_TrapAlloc    : Word;        { traps allocated               }
        tc_TrapAble     : Word;        { traps enabled                 }
        tc_ExceptData   : Pointer;      { points to except data         }
        tc_ExceptCode   : Pointer;      { points to except code         }
        tc_TrapData     : Pointer;      { points to trap data           }
        tc_TrapCode     : Pointer;      { points to trap code           }
        tc_SPReg        : Pointer;      { stack pointer                 }
        tc_SPLower      : Pointer;      { stack lower bound             }
        tc_SPUpper      : Pointer;      { stack upper bound + 2         }
        tc_Switch       : Pointer;      { task losing CPU               }
        tc_Launch       : Pointer;      { task getting CPU              }
        tc_MemEntry     : tList;        { allocated memory              }
        tc_UserData     : Pointer;      { per task data                 }
    end;



    TDateStamp = packed record
        ds_Days         : Longint;      { Number of days since Jan. 1, 1978 }
        ds_Minute       : Longint;      { Number of minutes past midnight }
        ds_Tick         : Longint;      { Number of ticks past minute }
    end;
    PDateStamp = ^TDateStamp;



{ Returned by Examine() and ExInfo(), must be on a 4 byte boundary }

    PFileInfoBlock = ^TfileInfoBlock;
    TFileInfoBlock = packed record
        fib_DiskKey     : Longint;
        fib_DirEntryType : Longint;
                        { Type of Directory. If < 0, then a plain file.
                          If > 0 a directory }
        fib_FileName    : Array [0..107] of Char;
                        { Null terminated. Max 30 chars used for now }
        fib_Protection  : Longint;
                        { bit mask of protection, rwxd are 3-0. }
        fib_EntryType   : Longint;
        fib_Size        : Longint;      { Number of bytes in file }
        fib_NumBlocks   : Longint;      { Number of blocks in file }
        fib_Date        : TDateStamp; { Date file last changed }
        fib_Comment     : Array [0..79] of Char;
                        { Null terminated comment associated with file }
        fib_Reserved    : Array [0..35] of Char;
    end;

{ returned by Info(), must be on a 4 byte boundary }

    pInfoData = ^tInfoData;
    tInfoData = packed record
        id_NumSoftErrors        : Longint;      { number of soft errors on disk }
        id_UnitNumber           : Longint;      { Which unit disk is (was) mounted on }
        id_DiskState            : Longint;      { See defines below }
        id_NumBlocks            : Longint;      { Number of blocks on disk }
        id_NumBlocksUsed        : Longint;      { Number of block in use }
        id_BytesPerBlock        : Longint;
        id_DiskType             : Longint;      { Disk Type code }
        id_VolumeNode           : BPTR;         { BCPL pointer to volume node }
        id_InUse                : Longint;      { Flag, zero if not in use }
    end;


{ ------ Library Base Structure ---------------------------------- }
{  Also used for Devices and some Resources  }

    pLibrary = ^tLibrary;
    tLibrary = packed record
        lib_Node     : tNode;
        lib_Flags,
        lib_pad      : Byte;
        lib_NegSize,            {  number of bytes before library  }
        lib_PosSize,            {  number of bytes after library  }
        lib_Version,            {  major  }
        lib_Revision : Word;    {  minor  }
        lib_IdString : PCHAR;   {  ASCII identification  }
        lib_Sum      : LONGINT; {  the checksum itself  }
        lib_OpenCnt  : Word;    {  number of current opens  }
    end;                {  * Warning: size is not a longword multiple ! * }

       pAChain = ^tAChain;
       tAChain = packed record
        an_Child,
        an_Parent   : pAChain;
        an_Lock     : BPTR;
        an_Info     : tFileInfoBlock;
        an_Flags    : Shortint;
        an_String   : Array[0..0] of Char;   { FIX!! }
       END;


       pAnchorPath = ^tAnchorPath;
       tAnchorPath = packed record
        case integer of
        0 : (
        ap_First      : pAChain;
        ap_Last       : pAChain;
        );
        1 : (
        ap_Base,                    { pointer to first anchor }
        ap_Current    : pAChain;    { pointer to last anchor }
        ap_BreakBits,               { Bits we want to break on }
        ap_FoundBreak : Longint;    { Bits we broke on. Also returns ERROR_BREAK }
        ap_Flags      : Shortint;       { New use for extra Integer. }
        ap_Reserved   : Shortint;
        ap_Strlen     : Integer;       { This is what ap_Length used to be }
        ap_Info       : tFileInfoBlock;
        ap_Buf        : Array[0..0] of Char;     { Buffer for path name, allocated by user !! }
        { FIX! }
        );
       END;


    pCommandLineInterface = ^TCommandLineInterface;
    TCommandLineInterface = packed record
      cli_result2     : longint;    {* Value of IoErr from last command   *}
      cli_SetName     : BSTR;       {* Name of current directory             *}
      cli_CommandDir  : BPTR;       {* Head of the path locklist             *}
      cli_ReturnCode  : longint;    {* Return code from last command          *}
      cli_CommandName : BSTR;       {* Name of current command              *}
      cli_FailLevel   : longint;    {* Fail level (set by FAILAT)            *}
      cli_Prompt      : BSTR;       {* Current prompt (set by PROMPT)     *}
      cli_StandardInput: BPTR;      {* Default (terminal) CLI input       *}
      cli_CurrentInput : BPTR;      {* Current CLI input                       *}
      cli_CommandFile  : BSTR;      {* Name of EXECUTE command file       *}
      cli_Interactive  : longint;   {* Boolean; True if prompts required  *}
      cli_Background   : longint;   {* Boolean; True if CLI created by RUN*}
      cli_CurrentOutput: BPTR;      {* Current CLI output                   *}
      cli_DefautlStack : longint;   {* Stack size to be obtained in long words *}
      cli_StandardOutput : BPTR;    {* Default (terminal) CLI output          *}
      cli_Module       : BPTR;      {* SegList of currently loaded command*}
    END;

    {    structure used for multi-directory assigns. AllocVec()ed. }

       pAssignList = ^tAssignList;
       tAssignList = packed record
        al_Next : pAssignList;
        al_Lock : BPTR;
       END;

   pDosList = ^tDosList;
   tDosList = packed record
    dol_Next            : BPTR;           {    bptr to next device on list }
    dol_Type            : Longint;        {    see DLT below }
    dol_Task            : pMsgPort;       {    ptr to handler task }
    dol_Lock            : BPTR;
    case integer of
    0 : (
        dol_Handler : record
          dol_Handler    : BSTR;      {    file name to load IF seglist is null }
          dol_StackSize,              {    stacksize to use when starting process }
          dol_Priority,               {    task priority when starting process }
          dol_Startup    : Longint;   {    startup msg: FileSysStartupMsg for disks }
          dol_SegList,                {    already loaded code for new task }
          dol_GlobVec    : BPTR;      {    BCPL global vector to use when starting
                                 * a process. -1 indicates a C/Assembler
                                 * program. }
        end;
    );
    1 : (
        dol_Volume       : record
          dol_VolumeDate : tDateStamp; {    creation date }
          dol_LockList   : BPTR;       {    outstanding locks }
          dol_DiskType   : Longint;    {    'DOS', etc }
        END;
    );
    2 : (
        dol_assign       :  record
          dol_AssignName : PChar;         {    name for non-OR-late-binding assign }
          dol_List       : pAssignList;   {    for multi-directory assigns (regular) }
         END;
    dol_Name            : BSTR;           {    bptr to bcpl name }
    );
   END;


    TProcess = packed record
        pr_Task         : TTask;
        pr_MsgPort      : TMsgPort;      { This is BPTR address from DOS functions  }
{126}   pr_Pad          : Word;         { Remaining variables on 4 byte boundaries }
{128}   pr_SegList      : Pointer;      { Array of seg lists used by this process  }
{132}   pr_StackSize    : Longint;      { Size of process stack in bytes            }
{136}   pr_GlobVec      : Pointer;      { Global vector for this process (BCPL)    }
{140}   pr_TaskNum      : Longint;      { CLI task number of zero if not a CLI      }
{144}   pr_StackBase    : BPTR;         { Ptr to high memory end of process stack  }
{148}   pr_Result2      : Longint;      { Value of secondary result from last call }
{152}   pr_CurrentDir   : BPTR;         { Lock associated with current directory   }
{156}   pr_CIS          : BPTR;         { Current CLI Input Stream                  }
{160}   pr_COS          : BPTR;         { Current CLI Output Stream                 }
{164}   pr_ConsoleTask  : Pointer;      { Console handler process for current window}
{168}   pr_FileSystemTask : Pointer;    { File handler process for current drive   }
{172}   pr_CLI          : BPTR;         { pointer to ConsoleLineInterpreter         }
        pr_ReturnAddr   : Pointer;      { pointer to previous stack frame           }
        pr_PktWait      : Pointer;      { Function to be called when awaiting msg  }
        pr_WindowPtr    : Pointer;      { Window for error printing }
        { following definitions are new with 2.0 }
        pr_HomeDir      : BPTR;         { Home directory of executing program      }
        pr_Flags        : Longint;      { flags telling dos about process          }
        pr_ExitCode     : Pointer;      { code to call on exit of program OR NULL  }
        pr_ExitData     : Longint;      { Passed as an argument to pr_ExitCode.    }
        pr_Arguments    : PChar;        { Arguments passed to the process at start }
        pr_LocalVars    : TMinList;      { Local environment variables             }
        pr_ShellPrivate : Longint;      { for the use of the current shell         }
        pr_CES          : BPTR;         { Error stream - IF NULL, use pr_COS       }
    end;
    PProcess = ^TProcess;


CONST
    { DOS Lib Offsets }
    _LVOMatchFirst = -822;
    _LVOMatchNext  = -828;
    _LVOMatchEnd   = -834;
    _LVOCli        = -492;
    _LVOExecute    = -222;
    _LVOSystemTagList = -606;
    _LVOSetFileDate = -396;

    LDF_READ   = 1;
    LDF_DEVICES = 4;

    ERROR_NO_MORE_ENTRIES            = 232;
    FIBF_SCRIPT         = 64;  { program is a script              }
    FIBF_PURE           = 32;  { program is reentrant             }
    FIBF_ARCHIVE        = 16;  { cleared whenever file is changed }
    FIBF_READ           = 8;   { ignoed by old filesystem         }
    FIBF_WRITE          = 4;   { ignored by old filesystem        }
    FIBF_EXECUTE        = 2;   { ignored by system, used by shell }
    FIBF_DELETE         = 1;   { prevent file from being deleted  }

    SHARED_LOCK         = -2;

{******************************************************************************
                           --- Internal routines ---
******************************************************************************}


procedure CurrentTime(var Seconds, Micros : Longint);
Begin
 asm
    MOVE.L  A6,-(A7)
    MOVE.L  Seconds,a0
    MOVE.L  Micros,a1
    MOVE.L  _IntuitionBase,A6
    JSR -084(A6)
    MOVE.L  (A7)+,A6
 end;
end;


function Date2Amiga(date : pClockData) : Longint;
Begin
  asm
    MOVE.L  A6,-(A7)
    MOVE.L  date,a0
    MOVE.L  _UtilityBase,A6
    JSR -126(A6)
    MOVE.L  (A7)+,A6
    MOVE.L  d0,@RESULT
  end;
end;


procedure Amiga2Date(amigatime : Longint;
                     resultat : pClockData);
Begin
  asm
    MOVE.L  A6,-(A7)
    MOVE.L  amigatime,d0
    MOVE.L  resultat,a0
    MOVE.L  _UtilityBase,A6
    JSR -120(A6)
    MOVE.L  (A7)+,A6
  end;
end;

FUNCTION Examine(lock : BPTR; fileInfoBlock : pFileInfoBlock) : BOOLEAN;
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVE.L  lock,D1
    MOVE.L  fileInfoBlock,D2
    MOVEA.L _DOSBase,A6
    JSR -102(A6)
    MOVEA.L (A7)+,A6
    TST.L   D0
    BEQ.B   @end
    MOVE.B  #1,D0
    @end: MOVE.B  D0,@RESULT
  END;
END;

function Lock(const name : string;
           accessmode : Longint) : BPTR;
var
 buffer: Array[0..255] of char;
Begin
  move(name[1],buffer,length(name));
  buffer[length(name)]:=#0;
  asm
    MOVEM.L d2/a6,-(A7)
    LEA     buffer,a0
    MOVE.L  a0,d1
    MOVE.L  accessmode,d2
    MOVE.L  _DOSBase,A6
    JSR -084(A6)
    MOVEM.L (A7)+,d2/a6
    MOVE.L  d0,@RESULT
  end;
end;


procedure UnLock(lock : BPTR);
Begin
  asm
    MOVE.L  A6,-(A7)
    MOVE.L  lock,d1
    MOVE.L  _DOSBase,A6
    JSR -090(A6)
    MOVE.L  (A7)+,A6
  end;
end;

FUNCTION Info(lock : BPTR; parameterBlock : pInfoData) : BOOLEAN;
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVE.L  lock,D1
    MOVE.L  parameterBlock,D2
    MOVEA.L _DOSBase,A6
    JSR -114(A6)
    MOVEA.L (A7)+,A6
    TST.L   D0
    BEQ.B   @end
    MOVE.B  #1,D0
    @end:
     MOVE.B  D0,@RESULT
  END;
END;

FUNCTION NameFromLock(lock : BPTR; buffer : pCHAR; len : LONGINT) : BOOLEAN;
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVE.L  lock,D1
    MOVE.L  buffer,D2
    MOVE.L  len,D3
    MOVEA.L _DOSBase,A6
    JSR -402(A6)
    MOVEA.L (A7)+,A6
    TST.L   D0
    BEQ.B   @end
    MOVE.B  #1,D0
    @end: MOVE.B  D0,@RESULT
  END;
END;

FUNCTION GetVar(name : pCHAR; buffer : pCHAR; size : LONGINT; flags : LONGINT) : LONGINT;
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVE.L  name,D1
    MOVE.L  buffer,D2
    MOVE.L  size,D3
    MOVE.L  flags,D4
    MOVEA.L _DOSBase,A6
    JSR -906(A6)
    MOVEA.L (A7)+,A6
    MOVE.L  D0,@RESULT
  END;
END;

FUNCTION FindTask(name : pCHAR) : pTask;
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVEA.L name,A1
    MOVEA.L _ExecBase,A6
    JSR -294(A6)
    MOVEA.L (A7)+,A6
    MOVE.L  D0,@RESULT
  END;
END;

FUNCTION MatchFirst(pat : pCHAR; anchor : pAnchorPath) : LONGINT;
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVE.L  pat,D1
    MOVE.L  anchor,D2
    MOVEA.L _DOSBase,A6
    JSR -822(A6)
    MOVEA.L (A7)+,A6
    MOVE.L  D0,@RESULT
  END;
END;

FUNCTION MatchNext(anchor : pAnchorPath) : LONGINT;
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVE.L  anchor,D1
    MOVEA.L _DOSBase,A6
    JSR -828(A6)
    MOVEA.L (A7)+,A6
    MOVE.L  D0,@RESULT
  END;
END;

PROCEDURE MatchEnd(anchor : pAnchorPath);
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVE.L  anchor,D1
    MOVEA.L _DOSBase,A6
    JSR -834(A6)
    MOVEA.L (A7)+,A6
  END;
END;

FUNCTION Cli : pCommandLineInterface;
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVEA.L _DOSBase,A6
    JSR -492(A6)
    MOVEA.L (A7)+,A6
    MOVE.L  D0,@RESULT
  END;
END;

Function _Execute(p: pchar): longint;
 Begin
   asm
     move.l  a6,d6                 { save base pointer       }
     move.l  d2,-(sp)
     move.l  p,d1                  { command to execute      }
     clr.l   d2                    { No TagList for command  }
     move.l  _DosBase,a6
     jsr     _LVOSystemTagList(a6)
     move.l  (sp)+,d2
     move.l  d6,a6                 { restore base pointer    }
     move.l  d0,@RESULT
   end;
end;

FUNCTION LockDosList(flags : CARDINAL) : pDosList;
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVE.L  flags,D1
    MOVEA.L _DOSBase,A6
    JSR -654(A6)
    MOVEA.L (A7)+,A6
    MOVE.L  D0,@RESULT
  END;
END;


PROCEDURE UnLockDosList(flags : CARDINAL);
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVE.L  flags,D1
    MOVEA.L _DOSBase,A6
    JSR -660(A6)
    MOVEA.L (A7)+,A6
  END;
END;


FUNCTION NextDosEntry(dlist : pDosList; flags : CARDINAL) : pDosList;
BEGIN
  ASM
    MOVE.L  A6,-(A7)
    MOVE.L  dlist,D1
    MOVE.L  flags,D2
    MOVEA.L _DOSBase,A6
    JSR -690(A6)
    MOVEA.L (A7)+,A6
    MOVE.L  D0,@RESULT
  END;
END;


FUNCTION BADDR(bval : BPTR): POINTER;
BEGIN
    BADDR := POINTER( bval shl 2);
END;

function PasToC(var s: string): Pchar;
var i: integer;
begin
    i := Length(s) + 1;
    if i > 255 then
    begin
        Delete(s, 255, 1);      { ensure there is a spare byte }
        Dec(i)
    end;
    s[i]     := #0;
    PasToC := @s[1]
end;


Procedure AmigaToDt(SecsPast: LongInt; Var Dt: DateTime);
var
  cd : pClockData;
Begin
  New(cd);
  Amiga2Date(SecsPast,cd);
  Dt.sec   := cd^.sec;
  Dt.min   := cd^.min;
  Dt.hour  := cd^.hour;
  Dt.day   := cd^.mday;
  Dt.month := cd^.month;
  Dt.year  := cd^.year;
  Dispose(cd);
End;

Function DtToAmiga(DT: DateTime): LongInt;
var
  cd : pClockData;
  temp : Longint;
Begin
  New(cd);
  cd^.sec   := Dt.sec;
  cd^.min   := Dt.min;
  cd^.hour  := Dt.hour;
  cd^.mday  := Dt.day;
  cd^.month := Dt.month;
  cd^.year  := Dt.year;
  temp := Date2Amiga(cd);
  Dispose(cd);
  DtToAmiga := temp;
end;

Function SetProtection(const name: string; mask:longint): longint;
 var
  buffer : array[0..255] of char;
 Begin
   move(name[1],buffer,length(name));
   buffer[length(name)]:=#0;
   asm
      move.l  a6,d6
      lea     buffer,a0
      move.l  a0,d1
      move.l  mask,d2
      move.l  _DosBase,a6
      jsr     -186(a6)
      move.l  d6,a6
      move.l  d0,@RESULT
   end;
 end;


Function IsLeapYear(Source : Word) : Boolean;
Begin
  If (Source Mod 4 = 0) Then
    IsLeapYear := True
  Else
    IsLeapYear := False;
End;


Procedure Amiga2DateStamp(Date : LongInt; Var TotalDays,Minutes,Ticks: longint);
{ Converts a value in seconds past 1978 to a value in AMIGA DateStamp format }
{ Taken from SWAG and modified to work with the Amiga format - CEC           }
Var
  LocalDate : LongInt; Done : Boolean; X : ShortInt; TotDays : Integer;
  Y: Word;
  M: Word;
  D: Word;
  H: Word;
  Min: Word;
  S : Word;
Begin
  Y   := 1978; M := 1; D := 1; H := 0; Min := 0; S := 0;
  TotalDays := 0;
  Minutes := 0;
  Ticks := 0;
  LocalDate := Date;
  Done := False;
  While Not Done Do
  Begin
    If LocalDate >= SecsPerYear Then
    Begin
      Inc(Y,1);
      Dec(LocalDate,SecsPerYear);
      Inc(TotalDays,DaysPerYear[12]);
    End
    Else
      Done := True;
    If (IsLeapYear(Y+1)) And (LocalDate >= SecsPerLeapYear) And
       (Not Done) Then
    Begin
      Inc(Y,1);
      Dec(LocalDate,SecsPerLeapYear);
      Inc(TotalDays,DaysPerLeapYear[12]);
    End;
  End; { END WHILE }
  M := 1; D := 1;
  Done := False;
  TotDays := LocalDate Div SecsPerDay;
  { Total number of days }
  TotalDays := TotalDays + TotDays;
    Dec(LocalDate,TotDays*SecsPerDay);
  { Absolute hours since start of day }
  H := LocalDate Div SecsPerHour;
  { Convert to minutes }
  Minutes := H*60;
    Dec(LocalDate,(H * SecsPerHour));
  { Find the remaining minutes to add }
  Min := LocalDate Div SecsPerMinute;
    Dec(LocalDate,(Min * SecsPerMinute));
  Minutes:=Minutes+Min;
  { Find the number of seconds and convert to ticks }
  S := LocalDate;
  Ticks:=TICKSPERSECOND*S;
End;


  Function SetFileDate(name: string; p : pDateStamp): longint;
  var
    buffer : array[0..255] of char;
  Begin
    move(name[1],buffer,length(name));
    buffer[length(name)]:=#0;
     asm
       move.l a6,d6           { save base pointer }
       move.l d2,-(sp)        { save reserved reg }
       lea    buffer,a0
       move.l a0,d1
       move.l p,d2
       move.l _DosBase,a6
       jsr    _LVOSetFileDate(a6)
       move.l (sp)+,d2        { restore reserved reg }
       move.l d6,a6           { restore base pointer }
       move.l d0,@Result
     end;
  end;





{******************************************************************************
                           --- Dos Interrupt ---
******************************************************************************}

(*Procedure Intr (intno: byte; var regs: registers);
  Begin
  { Does not apply to Linux - not implemented }
  End;*)


Procedure SwapVectors;
  Begin
  { Does not apply to Linux - Do Nothing }
  End;


(*Procedure msdos(var regs : registers);
  Begin
  { ! Not implemented in Linux ! }
  End;*)


Procedure getintvec(intno : byte;var vector : pointer);
  Begin
  { ! Not implemented in Linux ! }
  End;


Procedure setintvec(intno : byte;vector : pointer);
  Begin
  { ! Not implemented in Linux ! }
  End;

{******************************************************************************
                        --- Info / Date / Time ---
******************************************************************************}

  Function DosVersion: Word;
   var p: pLibrary;
  Begin
    p:=pLibrary(_DosBase);
    DosVersion:= p^.lib_Version or (p^.lib_Revision shl 8);
  End;

Procedure GetDate(Var Year, Month, MDay, WDay: Word);
Var
  cd    : pClockData;
  mysec,
  tick  : Longint;
begin
  New(cd);
  CurrentTime(mysec,tick);
  Amiga2Date(mysec,cd);
  Year  := cd^.year;
  Month := cd^.month;
  MDay  := cd^.mday;
  WDay  := cd^.wday;
  Dispose(cd);
end;

Procedure SetDate(Year, Month, Day: Word);
  Begin
  { !! }
  End;

Procedure GetTime(Var Hour, Minute, Second, Sec100: Word);
Var
  mysec,
  tick    : Longint;
  cd      : pClockData;
begin
  New(cd);
  CurrentTime(mysec,tick);
  Amiga2Date(mysec,cd);
  Hour   := cd^.hour;
  Minute := cd^.min;
  Second := cd^.sec;
  Sec100 := 0;
  Dispose(cd);
END;


Procedure SetTime(Hour, Minute, Second, Sec100: Word);
  Begin
  { !! }
  End;

Procedure unpacktime(p : longint;var t : datetime);
Begin
  AmigaToDt(p,t);
End;


Procedure packtime(var t : datetime;var p : longint);
Begin
  p := DtToAmiga(t);
end;


{******************************************************************************
                               --- Exec ---
******************************************************************************}


Var
  LastDosExitCode: word;
  breakflag : Boolean;
  ver: Boolean;


Procedure Exec (Const Path: PathStr; Const ComLine: ComStr);
  var
   p : string;
   buf: array[0..255] of char;
   result : longint;
   MyLock : longint;
   i : Integer;
  Begin
   DosError := 0;
   LastdosExitCode := 0;
   p:=Path+' '+ComLine;
   { allow backslash as slash }
   for i:=1 to length(p) do
       if p[i]='\' then p[i]:='/';
   Move(p[1],buf,length(p));
   buf[Length(p)]:=#0;
   { Here we must first check if the command we wish to execute }
   { actually exists, because this is NOT handled by the        }
   { _SystemTagList call (program will abort!!)                 }

   { Try to open with shared lock                               }
   MyLock:=Lock(path,SHARED_LOCK);
   if MyLock <> 0 then
     Begin
        { File exists - therefore unlock it }
        Unlock(MyLock);
        result:=_Execute(buf);
        { on return of -1 the shell could not be executed }
        { probably because there was not enough memory    }
        if result = -1 then
          DosError:=8
        else
          LastDosExitCode:=word(result);
     end
   else
    DosError:=3;
  End;


Function DosExitCode: Word;
  Begin
    DosExitCode:=LastdosExitCode;
  End;


  Procedure GetCBreak(Var BreakValue: Boolean);
  Begin
   breakvalue:=breakflag;
  End;


 Procedure SetCBreak(BreakValue: Boolean);
  Begin
   breakflag:=BreakValue;
  End;


  Procedure GetVerify(Var Verify: Boolean);
   Begin
     verify:=ver;
   End;


 Procedure SetVerify(Verify: Boolean);
  Begin
    ver:=Verify;
  End;

{******************************************************************************
                               --- Disk ---
******************************************************************************}

{ How to solve the problem with this:       }
{  We could walk through the device list    }
{  at startup to determine possible devices }

const

  not_to_use_devs : array[0..12] of string =(
                   'DF0:',
                   'DF1:',
                   'DF2:',
                   'DF3:',
                   'PED:',
                   'PRJ:',
                   'PIPE:',
                   'RAM:',
                   'CON:',
                   'RAW:',
                   'SER:',
                   'PAR:',
                   'PRT:');

var
   deviceids : array[1..20] of byte;
   devicenames : array[1..20] of string[20];
   numberofdevices : Byte;

Function DiskFree(Drive: Byte): Longint;
Var
  MyLock      : BPTR;
  Inf         : pInfoData;
  Free        : Longint;
  myproc      : pProcess;
  OldWinPtr   : Pointer;
Begin
  Free := -1;
  { Here we stop systemrequesters to appear }
  myproc := pProcess(FindTask(nil));
  OldWinPtr := myproc^.pr_WindowPtr;
  myproc^.pr_WindowPtr := Pointer(-1);
  { End of systemrequesterstop }
  New(Inf);
  MyLock := Lock(devicenames[deviceids[Drive]],SHARED_LOCK);
  If MyLock <> 0 then begin
     if Info(MyLock,Inf) then begin
        Free := (Inf^.id_NumBlocks * Inf^.id_BytesPerBlock) -
                (Inf^.id_NumBlocksUsed * Inf^.id_BytesPerBlock);
     end;
     Unlock(MyLock);
  end;
  Dispose(Inf);
  { Restore systemrequesters }
  myproc^.pr_WindowPtr := OldWinPtr;
  diskfree := Free;
end;



Function DiskSize(Drive: Byte): Longint;
Var
  MyLock      : BPTR;
  Inf         : pInfoData;
  Size        : Longint;
  myproc      : pProcess;
  OldWinPtr   : Pointer;
Begin
  Size := -1;
  { Here we stop systemrequesters to appear }
  myproc := pProcess(FindTask(nil));
  OldWinPtr := myproc^.pr_WindowPtr;
  myproc^.pr_WindowPtr := Pointer(-1);
  { End of systemrequesterstop }
  New(Inf);
  MyLock := Lock(devicenames[deviceids[Drive]],SHARED_LOCK);
  If MyLock <> 0 then begin
     if Info(MyLock,Inf) then begin
        Size := (Inf^.id_NumBlocks * Inf^.id_BytesPerBlock);
     end;
     Unlock(MyLock);
  end;
  Dispose(Inf);
  { Restore systemrequesters }
  myproc^.pr_WindowPtr := OldWinPtr;
  disksize := Size;
end;




Procedure FindFirst(Path: PathStr; Attr: Word; Var f: SearchRec);
var
 buf: Array[0..255] of char;
 Anchor : pAnchorPath;
 Result : Longint;
 index : Integer;
 s     : string;
 j     : integer;
Begin
 DosError:=0;
 New(Anchor);
 {----- allow backslash as slash         -----}
 for index:=1 to length(path) do
   if path[index]='\' then path[index]:='/';
 { remove any dot characters and replace by their current }
 { directory equivalent.                                  }
 if pos('../',path) = 1 then
 { look for parent directory }
    Begin
       delete(path,1,3);
       getdir(0,s);
       j:=length(s);
       while (s[j] <> '/') AND (s[j] <> ':') AND (j > 0 ) do
         dec(j);
       if j > 0 then
         s:=copy(s,1,j);
       path:=s+path;
    end
 else
 if pos('./',path) = 1 then
 { look for current directory }
    Begin
       delete(path,1,2);
       getdir(0,s);
       if (s[length(s)] <> '/') and (s[length(s)] <> ':') then
          s:=s+'/';
       path:=s+path;
    end;
 {----- replace * by #? AmigaOs strings  -----}
 repeat
  index:= pos('*',Path);
  if index <> 0 then
   Begin
     delete(Path,index,1);
     insert('#?',Path,index);
   end;
 until index =0;
 {--------------------------------------------}
 FillChar(Anchor^,sizeof(TAnchorPath),#0);
 move(path[1],buf,length(path));
 buf[length(path)]:=#0;

 Result:=MatchFirst(@buf,Anchor);
 f.AnchorPtr:=Anchor;
 if Result = ERROR_NO_MORE_ENTRIES then
   DosError:=18
 else
 if Result <> 0 then
   DosError:=3;
 { If there is an error, deallocate }
 { the anchorpath structure         }
 if DosError <> 0 then
   Begin
     MatchEnd(Anchor);
     if assigned(Anchor) then
       Dispose(Anchor);
   end
 else
 {-------------------------------------------------------------------}
 { Here we fill up the SearchRec attribute, but we also do check     }
 { something else, if the it does not match the mask we are looking  }
 { for we should go to the next file or directory.                   }
 {-------------------------------------------------------------------}
   Begin
         with Anchor^.ap_Info do
          Begin
             f.Time := fib_Date.ds_Days * (24 * 60 * 60) +
             fib_Date.ds_Minute * 60 +
             fib_Date.ds_Tick div 50;
           {*------------------------------------*}
           {* Determine if is a file or a folder *}
           {*------------------------------------*}
           if fib_DirEntryType > 0 then
               f.attr:=f.attr OR DIRECTORY;

           {*------------------------------------*}
           {* Determine if Read only             *}
           {*  Readonly if R flag on and W flag  *}
           {*   off.                             *}
           {* Should we check also that EXEC     *}
           {* is zero? for read only?            *}
           {*------------------------------------*}
           if   ((fib_Protection and FIBF_READ) <> 0)
            AND ((fib_Protection and FIBF_WRITE) = 0)
           then
              f.attr:=f.attr or READONLY;
           f.Name := strpas(fib_FileName);
           f.Size := fib_Size;
         end; { end with }
   end;
End;


Procedure FindNext(Var f: SearchRec);
var
 Result: longint;
 Anchor : pAnchorPath;
Begin
 DosError:=0;
 Result:=MatchNext(f.AnchorPtr);
 if Result = ERROR_NO_MORE_ENTRIES then
   DosError:=18
 else
 if Result <> 0 then
   DosError:=3;
 { If there is an error, deallocate }
 { the anchorpath structure         }
 if DosError <> 0 then
   Begin
     MatchEnd(f.AnchorPtr);
     if assigned(f.AnchorPtr) then
       Dispose(f.AnchorPtr);
   end
 else
 { Fill up the Searchrec information     }
 { and also check if the files are with  }
 { the correct attributes                }
   Begin
         Anchor:=pAnchorPath(f.AnchorPtr);
         with Anchor^.ap_Info do
          Begin
             f.Time := fib_Date.ds_Days * (24 * 60 * 60) +
             fib_Date.ds_Minute * 60 +
             fib_Date.ds_Tick div 50;
           {*------------------------------------*}
           {* Determine if is a file or a folder *}
           {*------------------------------------*}
           if fib_DirEntryType > 0 then
               f.attr:=f.attr OR DIRECTORY;

           {*------------------------------------*}
           {* Determine if Read only             *}
           {*  Readonly if R flag on and W flag  *}
           {*   off.                             *}
           {* Should we check also that EXEC     *}
           {* is zero? for read only?            *}
           {*------------------------------------*}
           if   ((fib_Protection and FIBF_READ) <> 0)
            AND ((fib_Protection and FIBF_WRITE) = 0)
           then
              f.attr:=f.attr or READONLY;
           f.Name := strpas(fib_FileName);
           f.Size := fib_Size;
         end; { end with }
   end;
End;

    Procedure FindClose(Var f: SearchRec);
      begin
      end;

{******************************************************************************
                               --- File ---
******************************************************************************}

Procedure FSplit(path: pathstr; var dir: dirstr; var name: namestr; var ext: extstr);
var
   p1,i : longint;
begin
  { allow slash as backslash }
  for i:=1 to length(path) do
   if path[i]='\' then path[i]:='/';
  { get drive name }
  p1:=pos(':',path);
  if p1>0 then
    begin
       dir:=copy(path,1,p1);
       delete(path,1,p1);
    end
  else
    dir:='';
  { split the path and the name, there are no more path informtions }
  { if path contains no backslashes                                 }
  while true do
    begin
       p1:=pos('/',path);
       if p1=0 then
         break;
       dir:=dir+copy(path,1,p1);
       delete(path,1,p1);
    end;
  { try to find out a extension }
  p1:=pos('.',path);
  if p1>0 then
    begin
       ext:=copy(path,p1,4);
       delete(path,p1,length(path)-p1+1);
    end
  else
    ext:='';
  name:=path;
end;


Function FExpand(Path: PathStr): PathStr;
var
    FLock  : BPTR;
    buffer : array[0..255] of char;
    i :integer;
    j :integer;
    temp : string;
begin

   { allow backslash as slash }
    for i:=1 to length(path) do
       if path[i]='\' then path[i]:='/';

   temp:=path;
   if pos('../',temp) = 1 then
     delete(temp,1,3);
   if pos('./',temp) = 1 then
      delete(temp,1,2);
   {First remove all references to '/./'}
    while pos('/./',temp)<>0 do
      delete(temp,pos('/./',temp),3);
   {Now remove also all references to '/../' + of course previous dirs..}
    repeat
      i:=pos('/../',temp);
      {Find the pos of the previous dir}
      if i>1 then
        begin
          j:=i-1;
          while (j>1) and (temp[j]<>'/') do
             dec (j);{temp[1] is always '/'}
          delete(temp,j,i-j+4);
        end
      else
      if i=1 then  {i=1, so we have temp='/../something', just delete '/../'}
       delete(temp,1,4);
    until i=0;


    FLock := Lock(temp,-2);
    if FLock <> 0 then begin
       if NameFromLock(FLock,buffer,255) then begin
          Unlock(FLock);
          FExpand := strpas(buffer);
       end else begin
          Unlock(FLock);
          FExpand := '';
       end;
    end else FExpand := '';
end;


   Function  fsearch(path : pathstr;dirlist : string) : pathstr;
      var
         i,p1   : longint;
         s      : searchrec;
         newdir : pathstr;
      begin
      { No wildcards allowed in these things }
         if (pos('?',path)<>0) or (pos('*',path)<>0) then
           fsearch:=''
         else
           begin
              { allow slash as backslash }
              for i:=1 to length(dirlist) do
                if dirlist[i]='\' then dirlist[i]:='/';
              repeat
                p1:=pos(';',dirlist);
                if p1<>0 then
                 begin
                   newdir:=copy(dirlist,1,p1-1);
                   delete(dirlist,1,p1);
                 end
                else
                 begin
                   newdir:=dirlist;
                   dirlist:='';
                 end;
                if (newdir<>'') and (not (newdir[length(newdir)] in ['/',':'])) then
                 newdir:=newdir+'/';
                findfirst(newdir+path,anyfile,s);
                if doserror=0 then
                 newdir:=newdir+path
                else
                 newdir:='';
              until (dirlist='') or (newdir<>'');
              fsearch:=newdir;
           end;
      end;


Procedure getftime (var f; var time : longint);
{
    This function returns a file's date and time as the number of
    seconds after January 1, 1978 that the file was created.
}
var
    FInfo : pFileInfoBlock;
    FTime : Longint;
    FLock : Longint;
    Str   : String;
    i     : integer;
begin
    DosError:=0;
    FTime := 0;
    Str := StrPas(filerec(f).name);
    for i:=1 to length(Str) do
     if str[i]='\' then str[i]:='/';
    FLock := Lock(Str, SHARED_LOCK);
    IF FLock <> 0 then begin
        New(FInfo);
        if Examine(FLock, FInfo) then begin
             with FInfo^.fib_Date do
             FTime := ds_Days * (24 * 60 * 60) +
             ds_Minute * 60 +
             ds_Tick div 50;
        end else begin
             FTime := 0;
        end;
        Unlock(FLock);
        Dispose(FInfo);
    end
    else
     DosError:=6;
    time := FTime;
end;


  Procedure setftime(var f; time : longint);
   var
    DateStamp: pDateStamp;
    Str: String;
    i: Integer;
    Days, Minutes,Ticks: longint;
    FLock: longint;
  Begin
    new(DateStamp);
    Str := StrPas(filerec(f).name);
    for i:=1 to length(Str) do
     if str[i]='\' then str[i]:='/';
    { Check first of all, if file exists }
    FLock := Lock(Str, SHARED_LOCK);
    IF FLock <> 0 then
      begin
        Unlock(FLock);
        Amiga2DateStamp(time,Days,Minutes,ticks);
        DateStamp^.ds_Days:=Days;
        DateStamp^.ds_Minute:=Minutes;
        DateStamp^.ds_Tick:=Ticks;
        if SetFileDate(Str,DateStamp) <> 0 then
            DosError:=0
        else
            DosError:=6;
      end
    else
      DosError:=2;
    if assigned(DateStamp) then Dispose(DateStamp);
  End;

  Procedure getfattr(var f; var attr : word);
  var
    info : pFileInfoBlock;
    MyLock : Longint;
    flags: word;
    Str: String;
    i: integer;
  Begin
    DosError:=0;
    flags:=0;
    New(info);
    Str := StrPas(filerec(f).name);
    for i:=1 to length(Str) do
     if str[i]='\' then str[i]:='/';
    { open with shared lock to check if file exists }
    MyLock:=Lock(Str,SHARED_LOCK);
    if MyLock <> 0 then
      Begin
        Examine(MyLock,info);
        {*------------------------------------*}
        {* Determine if is a file or a folder *}
        {*------------------------------------*}
        if info^.fib_DirEntryType > 0 then
             flags:=flags OR DIRECTORY;

        {*------------------------------------*}
        {* Determine if Read only             *}
        {*  Readonly if R flag on and W flag  *}
        {*   off.                             *}
        {* Should we check also that EXEC     *}
        {* is zero? for read only?            *}
        {*------------------------------------*}
        if   ((info^.fib_Protection and FIBF_READ) <> 0)
         AND ((info^.fib_Protection and FIBF_WRITE) = 0)
         then
          flags:=flags OR ReadOnly;
        Unlock(mylock);
      end
    else
      DosError:=3;
    attr:=flags;
    Dispose(info);
  End;


Procedure setfattr (var f;attr : word);
  var
   flags: longint;
   MyLock : longint;
   str: string;
   i: integer;
  Begin
    DosError:=0;
    flags:=FIBF_WRITE;
    { open with shared lock }
    Str := StrPas(filerec(f).name);
    for i:=1 to length(Str) do
     if str[i]='\' then str[i]:='/';

    MyLock:=Lock(Str,SHARED_LOCK);

    { By default files are read-write }
    if attr AND ReadOnly <> 0 then
      { Clear the Fibf_write flags }
      flags:=FIBF_READ;


    if MyLock <> 0 then
     Begin
       Unlock(MyLock);
       if SetProtection(Str,flags) = 0 then
         DosError:=5;
     end
    else
      DosError:=3;
  End;



{******************************************************************************
                             --- Environment ---
******************************************************************************}

 Function EnvCount: Longint;
 { HOW TO GET THIS VALUE:                                }
 {   Each time this function is called, we look at the   }
 {   local variables in the Process structure (2.0+)     }
 {   And we also read all files in the ENV: directory    }
  Begin
  End;


 Function EnvStr(Index: Integer): String;
  Begin
    EnvStr:='';
  End;



function GetEnv(envvar : String): String;
var
   buffer : Pchar;
   bufarr : array[0..255] of char;
   strbuffer : array[0..255] of char;
   temp : Longint;
begin
   move(envvar[1],strbuffer,length(envvar));
   strbuffer[length(envvar)] := #0;
   buffer := @bufarr;
   temp := GetVar(strbuffer,buffer,255,$100);
   if temp = -1 then
      GetEnv := ''
   else GetEnv := StrPas(buffer);
end;


{******************************************************************************
                             --- Not Supported ---
******************************************************************************}

Procedure keep(exitcode : word);
  Begin
  { ! Not implemented in Linux ! }
  End;

procedure AddDevice(str : String);
begin
    inc(numberofdevices);
    deviceids[numberofdevices] := numberofdevices;
    devicenames[numberofdevices] := str;
end;

function MakeDeviceName(str : pchar): string;
var
   temp : string[20];
begin
   temp := strpas(str);
   temp := temp + ':';
   MakeDeviceName := temp;
end;

function IsInDeviceList(str : string): boolean;
var
   i : byte;
   theresult : boolean;
begin
   theresult := false;
   for i := low(not_to_use_devs) to high(not_to_use_devs) do
   begin
       if str = not_to_use_devs[i] then begin
          theresult := true;
          break;
       end;
   end;
   IsInDeviceList := theresult;
end;


function BSTR2STRING(s : BSTR): pchar;
begin
    BSTR2STRING := Pointer(Longint(BADDR(s))+1);
end;

procedure ReadInDevices;
var
   dl : pDosList;
   temp : pchar;
   str  : string[20];
begin
   dl := LockDosList(LDF_DEVICES or LDF_READ );
   repeat
      dl := NextDosEntry(dl,LDF_DEVICES );
      if dl <> nil then begin
         temp := BSTR2STRING(dl^.dol_Name);
         str := MakeDeviceName(temp);
         if not IsInDeviceList(str) then
              AddDevice(str);
      end;
   until dl = nil;
   UnLockDosList(LDF_DEVICES or LDF_READ );
end;

Begin
 DosError:=0;
 ver:=TRUE;
 breakflag:=TRUE;
 numberofdevices := 0;
 AddDevice('DF0:');
 AddDevice('DF1:');
 AddDevice('DF2:');
 AddDevice('DF3:');
 ReadInDevices;
End.

{
  $Log$
  Revision 1.1  2002/02/19 08:24:41  sasu
  Initial revision

  Revision 1.1  2000/07/13 06:30:25  michael
  + Initial import

  Revision 1.13  2000/02/09 16:59:28  peter
    * truncated log

  Revision 1.12  2000/01/07 16:41:28  daniel
    * copyright 2000

  Revision 1.11  2000/01/07 16:32:21  daniel
    * copyright 2000 added

}









