#ifndef _CLASS_PROTOS_H
#define _CLASS_PROTOS_H

/* init.c */
ULONG ASM SAVEDS query ( REG (d0 )LONG which );
void ASM freeBase ( void );
BOOL ASM initBase ( void );

/* snprintf.c */
int STDARGS snprintf ( char *buf , int size , char *fmt , ...);

/* loc.c */
void ASM initStrings ( void );
STRPTR ASM getString ( REG (d0 )ULONG id );
ULONG ASM getKeyChar ( REG (a0 )STRPTR string );

/* shape.c */
BOOL ASM initShape ( void );

/* class.c */
BOOL ASM initMCP ( void );

#endif /* _CLASS_PROTOS_H */
