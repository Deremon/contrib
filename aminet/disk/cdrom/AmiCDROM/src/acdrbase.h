#ifndef ACDRBASE_H
#define ACDRBASE_H

#include <dos/bptr.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <intuition/intuitionbase.h>

struct ACDRBase {
	struct Device device;
	struct ExecBase *SysBase;
	struct DOSBase *DOSBase;
	BPTR seglist;
	struct MsgPort port;   /* message port for the handler */
	struct MsgPort rport;  /* reply port of the handler */
	struct MsgPort prport; /* 2nd replyport (answer of packets) */
	struct IOFileSys *iofs;
	struct List process_list; /* list of packet style processes */
	void *(*GetData)(struct ACDRBase *);
#if 0
	struct Library *UtilityBase;
	struct IntuitionBase *IntuitionBase;
	struct Library *IconBase;
	struct Library *WorkbenchBase;
#endif
};

struct ProcNode {
	struct Node ln;
	struct Process *proc; /* packet style process */
	void *data;           /* data of the process (globals) */
};

#define expunge() AROS_LC0(BPTR, expunge, struct ACDRBase *, acdrbase, 3, acdr)

#endif /* ACDRBASE_H */

