;
; @(#) $Header$
;
; BGUI library
; libtag.asm
;
; (C) Copyright 1998 Manuel Lemos.
; (C) Copyright 1996-1997 Ian J. Einman.
; (C) Copyright 1993-1996 Jaba Development.
; (C) Copyright 1993-1996 Jan van den Baard.
; All Rights Reserved.
;
; $Log$
; Revision 1.1  1998/02/25 17:08:54  mlemos
; Ian sources
;
;
;

         SECTION TEXT,CODE

         XREF  _LibID
         XREF  _LibName
         XREF  _LibInit

;
;  Do nothing when run as an executable.
;
         MOVEQ.L #0,D0
         RTS

ROMTag:                 DC.W  $4AFC    ; RT_MATCHWORD
         DC.L  ROMTag      ; RT_MATCHTAG
         DC.L  EndCode         ; RT_ENDSKIP
         DC.B  0     ; RT_FLAGS
         DC.B  41    ; RT_VERSION
         DC.B  9     ; RT_TYPE   (NT_LIBRARY)
         DC.B  0     ; RT_PRI
         DC.L  _LibName ; RT_NAME
         DC.L  _LibID   ; RT_IDSTRING
         DC.L  _LibInit ; RT_INIT

EndCode:
         END
