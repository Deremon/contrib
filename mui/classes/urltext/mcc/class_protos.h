#ifndef _CLASS_PROTOS_H
#define _CLASS_PROTOS_H

/* init.c */
SAVEDS ASM ULONG query ( REG (d0 )LONG which );
void ASM freeBase ( REG (a0 )struct UrltextBase *base );
BOOL ASM initBase ( REG (a0 )struct UrltextBase *base );

/* class.c */
BOOL ASM initMCC ( REG (a0 )struct UrltextBase *base );

/* utils */
char ASM getKeyChar ( REG (a0 )STRPTR string );
int STDARGS snprintf ( char *buf , int size , char *fmt , ...);

/* loc.c */
STRPTR ASM getString ( REG (d0 )ULONG id );

#endif /* _CLASS_PROTOS_H */
