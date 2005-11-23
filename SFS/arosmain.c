#include <proto/dos.h>
#include <proto/exec.h>
#include <aros/asmcall.h>
#include <dos/bptr.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <exec/memory.h>
#include <utility/tagitem.h>

#include <stddef.h>
#include <string.h> 

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1
#include <aros/debug.h>

#include "asfsbase.h"
#include "globals.h"
#include "aros_stuff.h"
#include "locks.h"

LONG mainprogram(struct ExecBase *);
void *ASFS_GetData(struct ASFSBase *);

static const ULONG sizes[]=
{
    0,
    offsetof(struct ExAllData,ed_Type),
    offsetof(struct ExAllData,ed_Size),
    offsetof(struct ExAllData,ed_Prot),
    offsetof(struct ExAllData,ed_Days),
    offsetof(struct ExAllData,ed_Comment),
    offsetof(struct ExAllData,ed_OwnerUID),
    sizeof(struct ExAllData)
};

#undef SysBase
/* entry point for the packet style process */
AROS_UFH3(void, ASFSOldEntry,
    AROS_UFHA(STRPTR, argstr, A0),
    AROS_UFHA(ULONG, arglen, D0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

D(bug("ASFSOldEntry\n"));

    /* Wait until global for this process is setup */
    Wait(SIGBREAKF_CTRL_F);

D(bug("ASFSOldEntry calling main program with SysBase=%p Globals=%p\n",SysBase,globals));

    mainprogram(SysBase);
    AROS_USERFUNC_EXIT
}

/*
	every packet style process needs it's own global variable
	we may loose it on every task switch so set it up here
	before switching the task
*/
AROS_UFH1(void, ASFS_Launch,
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT
    if (globals)
    {
	    globals = ASFS_GetData(globals->asfsbase);
    }
    AROS_USERFUNC_EXIT
}

#ifdef SysBase
#	undef SysBase
#endif
#define SysBase asfsbase->SysBase
#ifdef DOSBase
#	undef DOSBase
#endif
#define DOSBase asfsbase->DOSBase

void sendPacket(struct ASFSBase *asfsbase, struct DosPacket *packet, struct MsgPort *mp)
{
    packet->dp_Link->mn_Node.ln_Name = (STRPTR)packet;
    packet->dp_Port = &asfsbase->prport;
    PutMsg(mp,packet->dp_Link);
    WaitPort(&asfsbase->prport);	/* wait for reply */
    GetMsg(&asfsbase->prport);
}

void AddFileSystemTask(struct ASFSBase *asfsbase, struct IOFileSys *iofs)
{
    struct ASFSDeviceInfo   *device;
    struct Process 	    *proc;
    struct Message 	     msg;
    struct DosPacket 	     packet;
    struct ProcNode 	    *node;
    struct Globals 	    *newglobal;
    struct TagItem 	     tags[]=
    {
        {NP_Entry  , (IPTR)ASFSOldEntry    	    	},
        {NP_Name   , (IPTR)"ASFS - packet style Process"},
        {TAG_END   , 0     	    	    	    	}
    };

D(bug("AddFileSystemTask\n"));

    device = AllocMem(sizeof(struct ASFSDeviceInfo),MEMF_PUBLIC | MEMF_CLEAR);
    if (device)
    {
	newglobal = AllocMem(sizeof(struct Globals), MEMF_PUBLIC | MEMF_CLEAR);
	if (newglobal)
	{
        newglobal->sysBase = SysBase;
        
	    node = AllocMem(sizeof(struct ProcNode), MEMF_PUBLIC | MEMF_CLEAR);
	    if (node)
	    {
		Forbid();
		AddTail(&asfsbase->process_list, &node->ln);
		Permit();
		
		node->data = newglobal;
		
		proc = CreateNewProc(tags);
		if (proc)
		{
		    Forbid();
		    proc->pr_Task.tc_Launch = ASFS_Launch;
		    proc->pr_Task.tc_Flags |= TF_LAUNCH;
		    Permit();

		    node->proc = proc;
		    device->taskmp = &proc->pr_MsgPort;
		    packet.dp_Link = &msg;
		    packet.dp_Arg2 = (IPTR)NULL;
//		    packet.dp_Arg3 = &device->pseudoDevNode;
		    device->fssm.fssm_Unit = iofs->io_Union.io_OpenDevice.io_Unit;
		    device->fssm.fssm_Device = MKBADDR(iofs->io_Union.io_OpenDevice.io_DeviceName);
		    device->fssm.fssm_Environ = MKBADDR((struct DosEnvec *)iofs->io_Union.io_OpenDevice.io_Environ);
		    device->fssm.fssm_Flags = 0;
		    packet.dp_Arg3 = (IPTR)&device->fssm;
		    newglobal->asfsbase = asfsbase;
		    newglobal->device = device;

		    if (!globals) globals = newglobal;

		    /* Indicate to proc that global is setup for it and
		       it can continue running */
		    Signal(&proc->pr_Task, SIGBREAKF_CTRL_F);

		    sendPacket(asfsbase, &packet, device->taskmp);
		    iofs->io_DosError = packet.dp_Res2;

		    if (packet.dp_Res1 == DOSTRUE)
		    {
			device->global = newglobal;
			device->rootfh.device = device;
			iofs->IOFS.io_Unit=(struct Unit *)&device->rootfh;
			return;
		    }
			
		} /* if (proc) */
		else
		{
		    iofs->io_DosError = ERROR_NO_FREE_STORE;
		}
		FreeMem(node, sizeof(struct ProcNode));

	    } /* if (node) */
	    else
	    {
		iofs->io_DosError = ERROR_NO_FREE_STORE;
	    }
	    FreeMem(newglobal, sizeof(struct Globals));
		
	} /* if (newglobal) */
	else
	{
	    iofs->io_DosError = ERROR_NO_FREE_STORE;
	}
	FreeMem(device, sizeof(struct ASFSDeviceInfo));
	
    } /* if (device) */
    else
    {
#warning "maybe use another error here"
	iofs->io_DosError = ERROR_NO_FREE_STORE;
    }
    
    iofs->IOFS.io_Unit=0;
}

void ASFS_work(struct ASFSBase *asfsbase)
{
    struct IOFileSys 	*iofs;
    struct ASFSHandle 	*asfshandle;
    struct Message  	 msg;
    struct DosPacket 	 packet;
    LONG    	    	 retval;
    LONG    	    	 error;

    asfsbase->port.mp_SigBit = SIGBREAKB_CTRL_F;
    asfsbase->port.mp_Flags = PA_SIGNAL;
    asfsbase->prport.mp_SigBit = AllocSignal(-1);    /* should never fail! */
    packet.dp_Link = &msg;
    
    for (;;)
    {
        while ((iofs=(struct IOFileSys *)GetMsg(&asfsbase->port)))
        {
            D(bug("[asfs] got command %ld\n",iofs->IOFS.io_Command));
            error=0;
            asfshandle = (struct ASFSHandle *)iofs->IOFS.io_Unit;
            if (asfshandle)
                globals = asfshandle->device->global;
#ifdef DEBUG
#if DEBUG!=0
            else
            {
                    if (iofs->IOFS.io_Command != (UWORD)-1)
                        bug("[asfs] no acdrhandle!!!\n");
            }
#endif
#endif
            switch (iofs->IOFS.io_Command)
            {
            case (UWORD)-1: /* new device to handle */
                /* send startupMsg() to handler() */
                AddFileSystemTask(asfsbase, iofs);
                PutMsg(&asfsbase->rport, &iofs->IOFS.io_Message);
                continue;

            case (UWORD)-2: /* kill handler for a device */
                iofs->io_DosError = ERROR_NOT_IMPLEMENTED;
                PutMsg(&asfsbase->rport, &iofs->IOFS.io_Message);
                continue;

            case (UWORD)-3: /* got a packet - forward it */
                sendPacket(asfsbase, iofs->io_PacketEmulation, asfshandle->device->taskmp);
                error = 0;
                break;

            case FSA_OPEN:
                {
                struct ASFSHandle *new;
D(bug("[asfs] open: %s\n", iofs->io_Union.io_OPEN_FILE.io_Filename));

                    new = AllocMem(sizeof(struct ASFSHandle), MEMF_PUBLIC | MEMF_CLEAR);
                    if (new)
                    {
		    	struct ExtFileLock dummyfl;
		    	void *handle;
			
		    	if (!(asfshandle->flags & AHF_IS_LOCK))
			{
			    dummyfl.link = asfshandle->handle;
			    handle = &dummyfl;
			}
			else
			{
			    handle = asfshandle->handle;
			}
                        packet.dp_Type = ACTION_LOCATE_OBJECT;
                        packet.dp_Arg1 =
                            (asfshandle ==  &asfshandle->device->rootfh) ?
                            0 :
                            (IPTR)MKBADDR(handle);
                        packet.dp_Arg2 =(IPTR)iofs->io_Union.io_OPEN_FILE.io_Filename;
                        packet.dp_Arg3 = 
                            (iofs->io_Union.io_OPEN.io_FileMode == FMF_LOCK) ?
                            EXCLUSIVE_LOCK :
                            SHARED_LOCK;

D(bug("[asfs] open: ACTION_LOCATE_OBJECT %x %x %x\n", packet.dp_Arg1, packet.dp_Arg2, packet.dp_Arg3));

                        sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                        error = packet.dp_Res2;
                        if (error == 0)
                        {
                            new->handle = (void *)packet.dp_Res1;
                            new->flags |= AHF_IS_LOCK;
                            new->device = asfshandle->device;
D(bug("[asfs] open: lock = %p\n", new->handle));
                        }
                        else
                        {
                            FreeMem(new, sizeof(struct ASFSHandle));
                            new = 0;
                        }
                        iofs->IOFS.io_Unit=(struct Unit *)new;
                    }
                    else
                        error = ERROR_NO_FREE_STORE;
                }
                break;

            case FSA_OPEN_FILE:
                {
                struct ASFSHandle *new;
                ULONG mode=iofs->io_Union.io_OPEN_FILE.io_FileMode;
D(bug("[asfs] openfile: %s, %lx, %lx\n", iofs->io_Union.io_OPEN_FILE.io_Filename, mode, iofs->io_Union.io_OPEN_FILE.io_Protection));
                    if (
                            (mode == FMF_MODE_OLDFILE) ||
                            (mode == FMF_READ)
                        )
                        packet.dp_Type = ACTION_FINDINPUT;
                    else if (mode == FMF_MODE_READWRITE)
                        packet.dp_Type = ACTION_FINDUPDATE;
                    else if (mode == FMF_MODE_NEWFILE)
                        packet.dp_Type = ACTION_FINDOUTPUT;
                    else
                    {
                        error = ERROR_BAD_NUMBER;
                        break; /* switch statement */
                    }
                    new = AllocMem(sizeof(struct ASFSHandle), MEMF_PUBLIC | MEMF_CLEAR);
                    if (new)
                    {
                    struct FileHandle fh=
                    {
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
                        0,{0,0},
#endif
                        0,
                        (UBYTE *)-1,(UBYTE *)-1,
                        0,0,0,0,0
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
                        ,0
#endif
                    };
		    
                        packet.dp_Arg1 = (IPTR)MKBADDR(&fh);
                        packet.dp_Arg2 =
                            (asfshandle ==  &asfshandle->device->rootfh) ?
                            0 :
                            (IPTR)MKBADDR(asfshandle->handle);
                        packet.dp_Arg3 = (IPTR)iofs->io_Union.io_OPEN_FILE.io_Filename;
                        sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                        if (packet.dp_Res1)
                        {
                            new->handle = (void *)fh.fh_Arg1;
                            new->flags |= AHF_IS_FH;
                            new->device = asfshandle->device;
D(bug("[asfs] openfile: handle = %lp\n", new->handle));
                            error = 0;
                        }
                        else
                        {
                            FreeMem(new, sizeof(struct ASFSHandle));
                            new = 0;
                            error = packet.dp_Res2;
                        }
                        iofs->IOFS.io_Unit=(struct Unit *)new;
                    }
                    else
                        error = ERROR_NO_FREE_STORE;
                }
                break;

            case FSA_CLOSE:
                if (asfshandle->flags & AHF_IS_LOCK)
                {
                    packet.dp_Type = ACTION_FREE_LOCK;
                    packet.dp_Arg1 = (IPTR)asfshandle->handle;
D(bug("[asfs] close: lock=%p\n", asfshandle->handle));
                }
                else
                {
                    packet.dp_Type = ACTION_END;
                    packet.dp_Arg1 = (IPTR)asfshandle->handle;
D(bug("[asfs] close: handle=%p\n", asfshandle->handle));
                }
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                error = packet.dp_Res2;
                FreeMem(asfshandle, sizeof(struct ASFSHandle));
                break;

            case FSA_READ:
                packet.dp_Type = ACTION_READ;
                packet.dp_Arg1 = (IPTR)asfshandle->handle;
                packet.dp_Arg2 = (IPTR)iofs->io_Union.io_READ.io_Buffer;
                packet.dp_Arg3 = (IPTR)iofs->io_Union.io_READ.io_Length;
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                iofs->io_Union.io_READ.io_Length = packet.dp_Res1;
                error = packet.dp_Res2;
                break;

            case FSA_WRITE:
                packet.dp_Type = ACTION_WRITE;
                packet.dp_Arg1 = (IPTR)asfshandle->handle;
                packet.dp_Arg2 = (IPTR)iofs->io_Union.io_WRITE.io_Buffer;
                packet.dp_Arg3 = (IPTR)iofs->io_Union.io_WRITE.io_Length;
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                iofs->io_Union.io_WRITE.io_Length = packet.dp_Res1;
                error = packet.dp_Res2;
                break;

            case FSA_SEEK:
                packet.dp_Type = ACTION_SEEK;
                packet.dp_Arg1 = (IPTR)asfshandle->handle;
                packet.dp_Arg2 = (IPTR)iofs->io_Union.io_SEEK.io_Offset;
                packet.dp_Arg3 = (IPTR)iofs->io_Union.io_SEEK.io_SeekMode;
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                iofs->io_Union.io_SEEK.io_Offset = packet.dp_Res1;
                error = packet.dp_Res2;
                break;

            case FSA_SET_FILE_SIZE:
                packet.dp_Type = ACTION_SET_FILE_SIZE;
                packet.dp_Arg1 = (IPTR)asfshandle->handle;
                packet.dp_Arg2 = (IPTR)iofs->io_Union.io_SET_FILE_SIZE.io_Offset;
                packet.dp_Arg3 = (IPTR)iofs->io_Union.io_SET_FILE_SIZE.io_SeekMode;
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                if (packet.dp_Res1)
                    error = 0;
                else
                    error = packet.dp_Res2;
                break;

            case FSA_FILE_MODE:
                packet.dp_Type = ACTION_CHANGE_MODE;
                packet.dp_Arg2 = (IPTR)asfshandle->handle;
                if (asfshandle->flags & AHF_IS_LOCK)
                {
                    packet.dp_Arg1 = CHANGE_LOCK;
                    packet.dp_Arg3 = (IPTR)iofs->io_Union.io_FILE_MODE.io_FileMode;
                }
                else
                {
                    packet.dp_Arg2 = CHANGE_FH;
                    packet.dp_Arg3 =
                        (
                            iofs->io_Union.io_FILE_MODE.io_FileMode & FMF_LOCK ?
                                EXCLUSIVE_LOCK :
                                SHARED_LOCK
                        );
                }
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                break;

            case FSA_IS_FILESYSTEM:
                packet.dp_Type = ACTION_IS_FILESYSTEM;
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                iofs->io_Union.io_IS_FILESYSTEM.io_IsFilesystem = packet.dp_Res1;
                error = packet.dp_Res2;
                break;
		
            case FSA_EXAMINE:
                {
                    struct FileInfoBlock fib;

                    packet.dp_Type = (asfshandle->flags & AHF_IS_LOCK) ? ACTION_EXAMINE_OBJECT : ACTION_EXAMINE_FH;
#ifdef DEBUG
#if DEBUG!=0
if ((asfshandle->flags & AHF_IS_LOCK)==0)
bug("[asfs] examine called on a lock!!!\n");
#endif
#endif
D(bug("[asfs] examine: lock=%p\n", asfshandle->handle));
                    packet.dp_Arg1 = (IPTR)asfshandle->handle;
                    packet.dp_Arg2 = (IPTR)MKBADDR(&fib);
                    sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                    error = packet.dp_Res2;

D(bug("[asfs] examine: error=%d, packet.dp_Res1=%p\n", error, packet.dp_Res1));
		    
                    if (packet.dp_Res1)
                    {
                	struct ExAllData    *ead = iofs->io_Union.io_EXAMINE.io_ead;
                	ULONG 	    	     size = iofs->io_Union.io_EXAMINE.io_Size;
                	ULONG 	    	     mode = iofs->io_Union.io_EXAMINE.io_Mode;
                	ULONG 	    	     len;
                	STRPTR      	     next,end;

                        next=(STRPTR)ead+sizes[mode];
                        end=(STRPTR)ead+size;
                        if (next<=end)
                        {
                            iofs->io_DirPos = fib.fib_DiskKey;
//kprintf("****************acdr examine: pos = %lx\n", iofs->io_DirPos);
D(bug("[asfs] examine: pos=%d, mode=%d\n", iofs->io_DirPos, mode));

                            switch (mode)
                            {
                            case ED_OWNER:
                                ead->ed_OwnerUID = fib.fib_OwnerUID;
                                ead->ed_OwnerGID = fib.fib_OwnerGID;
D(bug("[asfs] examine: UID=%d, GID=%d\n", ead->ed_OwnerUID, ead->ed_OwnerGID));


                            case ED_COMMENT:
                                len = AROS_BSTR_strlen(fib.fib_Comment)+1;
                                if ((next+len)>end)
                                {
                                    error = ERROR_BUFFER_OVERFLOW;
                                    break; /* break switch statement */
                                }
                                ead->ed_Comment = next;
                                CopyMem(AROS_BSTR_ADDR(fib.fib_Comment), ead->ed_Comment, len);
                                next += len;
D(bug("[asfs] examine: comment=%s\n", ead->ed_Comment));

                            case ED_DATE:
                                ead->ed_Days = fib.fib_Date.ds_Days;
                                ead->ed_Mins = fib.fib_Date.ds_Minute;
                                ead->ed_Ticks = fib.fib_Date.ds_Tick;
D(bug("[asfs] examine: date=%d %d %d\n", ead->ed_Days, ead->ed_Mins, ead->ed_Ticks));

                            case ED_PROTECTION:
                                ead->ed_Prot = fib.fib_Protection;
D(bug("[asfs] examine: protection=%08x\n", ead->ed_Prot));

                            case ED_SIZE:
                                ead->ed_Size = fib.fib_Size;
D(bug("[asfs] examine: size=%d\n", ead->ed_Size));

                            case ED_TYPE:
                                ead->ed_Type = fib.fib_EntryType;

                            case ED_NAME:
                                len = AROS_BSTR_strlen(fib.fib_FileName)+1;
                                if ((next+len)>end)
                                {
                                    error = ERROR_BUFFER_OVERFLOW;
                                    break; /* switch statement */
                                }
                                ead->ed_Name = next;
                                CopyMem(AROS_BSTR_ADDR(fib.fib_FileName), ead->ed_Name, len);
                                ead->ed_Name[len]=0;
                                next += len;
D(bug("[asfs] examine: name=%s ([0]=%x)\n", ead->ed_Name, ead->ed_Name[0]));

                            case 0:
                                ead->ed_Next = 0;
                                error = 0;
D(bug("[asfs] examine: no error\n"));
                                break;
				
                            default:
                                error = ERROR_BAD_NUMBER;
D(bug("[asfs] examine: ERROR_BAD_NUMBER\n"));
				break;
                            }
D(bug("[asfs] the next one\n"));
                        } /* if (next<=end) */
                        else
                        {
                            error = ERROR_BUFFER_OVERFLOW;
                        }
			
                    } /* if (packet.dp_Res1) */
		    
                } /**/
                break;

            case FSA_EXAMINE_NEXT:
//kprintf("****************acdr examineNext: lock = %lx\n", acdrhandle->handle);
                packet.dp_Type = ACTION_EXAMINE_NEXT;
                packet.dp_Arg1 = (IPTR)asfshandle->handle;
                packet.dp_Arg2 = (IPTR)BADDR(iofs->io_Union.io_EXAMINE_NEXT.io_fib);
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
//kprintf("****************acdr examinenext: pos = %lx\n", iofs->io_Union.io_EXAMINE_NEXT.io_fib->fib_DiskKey);
                if (packet.dp_Res1)
                    error = 0;
                else
                    error = packet.dp_Res2;
                break;

            case FSA_CREATE_DIR:
                {
                struct ASFSHandle *new;

                    new = AllocMem(sizeof(struct ASFSHandle), MEMF_PUBLIC|MEMF_CLEAR);
                    if (new)
                    {
                        packet.dp_Type = ACTION_CREATE_DIR;
                        packet.dp_Arg1 =
                            (asfshandle ==  &asfshandle->device->rootfh) ?
                            0 :
                            (IPTR)MKBADDR(asfshandle->handle);
                        packet.dp_Arg2=(IPTR)iofs->io_Union.io_CREATE_DIR.io_Filename;
                        sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                        error = packet.dp_Res2;
                        if (error == 0)
                        {
                            new->handle = (void *)packet.dp_Res1;
                            new->flags |= AHF_IS_LOCK;
                            new->device = asfshandle->device;
//kprintf("****************acdr-create dir: lock = %lx\n", new->handle);
                        }
                        else
                        {
                            FreeMem(new, sizeof(struct ASFSHandle));
                            new = 0;
                        }
                        iofs->IOFS.io_Unit=(struct Unit *)new;
                    }
                    else
                        error = ERROR_NO_FREE_STORE;
                }
                break;

            case FSA_CREATE_HARDLINK:
            case FSA_CREATE_SOFTLINK:
                packet.dp_Type = ACTION_MAKE_LINK;
                packet.dp_Arg1 = 
                    (asfshandle ==  &asfshandle->device->rootfh) ?
                    0 :
                    (IPTR)MKBADDR(asfshandle->handle);
                if (iofs->IOFS.io_Command == FSA_CREATE_HARDLINK)
                {
                    packet.dp_Arg2 =
                        (IPTR)iofs->io_Union.io_CREATE_HARDLINK.io_Filename;
                    packet.dp_Arg3 =
                        (IPTR)iofs->io_Union.io_CREATE_HARDLINK.io_OldFile;
                    packet.dp_Arg4 = LINK_HARD;
                }
                else
                {
                    packet.dp_Arg2 =
                        (IPTR)iofs->io_Union.io_CREATE_SOFTLINK.io_Filename;
                    packet.dp_Arg3 =
                        (IPTR)iofs->io_Union.io_CREATE_SOFTLINK.io_Reference;
                    packet.dp_Arg4 = LINK_SOFT;
                }
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                if (packet.dp_Res1)
                    error = 0;
                else
                    error = packet.dp_Res2;
                break;

            case FSA_READ_SOFTLINK:
                packet.dp_Type = ACTION_READ_LINK;
                packet.dp_Arg1 = 
                    (asfshandle ==  &asfshandle->device->rootfh) ?
                    0 :
                    (IPTR)MKBADDR(asfshandle->handle);
                packet.dp_Arg2 = 0;
#warning "VERY FIXME: READ_LINK missing parameter"
                packet.dp_Arg3 = (IPTR)iofs->io_Union.io_READ_SOFTLINK.io_Buffer;
                packet.dp_Arg4 = (IPTR)iofs->io_Union.io_READ_SOFTLINK.io_Size;
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                if (packet.dp_Res1 == -2)
                    error = ERROR_BUFFER_OVERFLOW;
                else if (packet.dp_Res1 == -1)
                    error = packet.dp_Res2;
                else
                {
                    error = 0;
                    iofs->io_Union.io_READ_SOFTLINK.io_Size = packet.dp_Res1;
                }
                break;

            case FSA_RENAME:
                packet.dp_Type = ACTION_RENAME_OBJECT;
                packet.dp_Arg1 =
                    (asfshandle ==  &asfshandle->device->rootfh) ?
                    0 :
                    (IPTR)MKBADDR(asfshandle->handle);
                packet.dp_Arg2 =(IPTR)MKBADDR(iofs->io_Union.io_RENAME.io_Filename);
                packet.dp_Arg3 = 0;
#warning "VERY FIXME: RENAME missing parameter"
                packet.dp_Arg4 = (IPTR)MKBADDR(iofs->io_Union.io_RENAME.io_NewName);
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                if (packet.dp_Res1)
                    error = 0;
                else
                    error = packet.dp_Res2;
                break;

            case FSA_DELETE_OBJECT:
                packet.dp_Type = ACTION_DELETE_OBJECT;
                packet.dp_Arg1 =
                    (asfshandle ==  &asfshandle->device->rootfh) ?
                    0 :
                    (IPTR)MKBADDR(asfshandle->handle);
                packet.dp_Arg2 =
                    (IPTR)MKBADDR(iofs->io_Union.io_DELETE_OBJECT.io_Filename);
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                if (packet.dp_Res1)
                    error = 0;
                else
                    error = packet.dp_Res2;
                break;

            case FSA_SET_COMMENT:
                packet.dp_Type = ACTION_SET_COMMENT;
                packet.dp_Arg2 =
                    (asfshandle ==  &asfshandle->device->rootfh) ?
                    0 :
                    (IPTR)MKBADDR(asfshandle->handle);
                packet.dp_Arg3 =
                    (IPTR)MKBADDR(iofs->io_Union.io_SET_COMMENT.io_Filename);
#warning "TODO: check if really BSTR"
                packet.dp_Arg4 = (IPTR)iofs->io_Union.io_SET_COMMENT.io_Comment;
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                if (packet.dp_Res1)
                    error = 0;
                else
                    error = packet.dp_Res2;
                break;

            case FSA_SET_PROTECT:
                packet.dp_Type = ACTION_SET_PROTECT;
                packet.dp_Arg2 =
                    (asfshandle ==  &asfshandle->device->rootfh) ?
                    0 :
                    (IPTR)MKBADDR(asfshandle->handle);
                packet.dp_Arg3 =
                    (IPTR)MKBADDR(iofs->io_Union.io_SET_PROTECT.io_Filename);
                packet.dp_Arg4 = iofs->io_Union.io_SET_PROTECT.io_Protection;
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                if (packet.dp_Res1)
                    error = 0;
                else
                    error = packet.dp_Res2;
                break;

            case FSA_SET_DATE:
                packet.dp_Type = ACTION_SET_DATE;
                packet.dp_Arg2 =
                    (asfshandle ==  &asfshandle->device->rootfh) ?
                    0 :
                    (IPTR)MKBADDR(asfshandle->handle);
                packet.dp_Arg3 =
                    (IPTR)MKBADDR(iofs->io_Union.io_SET_DATE.io_Filename);
                packet.dp_Arg4 = (IPTR)&iofs->io_Union.io_SET_DATE.io_Date;
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                if (packet.dp_Res1)
                    error = 0;
                else
                    error = packet.dp_Res2;
                break;

            case FSA_INHIBIT:
                packet.dp_Type = ACTION_INHIBIT;
                packet.dp_Arg1 = iofs->io_Union.io_INHIBIT.io_Inhibit;
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                if (packet.dp_Res1)
                {
#if 0
                    if (globals->DevList && (iofs->io_Union.io_INHIBIT.io_Inhibit == DOSFALSE))
                    {
#warning "I hope volumenode is always valid"
                        global->DevList->dl_Device = iofs->IOFS.io_Device;
                        global->DevList->dl_Unit = (struct Unit *)&acdrhandle->device->rootfh;
                    }
#endif
                    error = 0;
                }
                else
                    error = ERROR_UNKNOWN;
                break;

            case FSA_FORMAT:
                packet.dp_Type = ACTION_FORMAT;
                packet.dp_Arg1 =
                    (IPTR)MKBADDR(iofs->io_Union.io_FORMAT.io_VolumeName);
                packet.dp_Arg2 = iofs->io_Union.io_FORMAT.io_DosType;
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                if (packet.dp_Res1)
                    error = 0;
                else
                    error = ERROR_UNKNOWN;
                break;

            case FSA_RELABEL:
                packet.dp_Type = ACTION_RENAME_DISK;
                packet.dp_Arg1 =(IPTR)MKBADDR(iofs->io_Union.io_RELABEL.io_NewName);
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                if (packet.dp_Res1)
                    error = 0;
                else
                    error = ERROR_UNKNOWN;
                break;
 
            case FSA_DISK_INFO:
                packet.dp_Type = ACTION_DISK_INFO;
                packet.dp_Arg1 = (IPTR)MKBADDR(iofs->io_Union.io_INFO.io_Info);
                sendPacket(asfsbase, &packet, asfshandle->device->taskmp);
                error = packet.dp_Res2;
                break;

#if 0
            case FSA_ALL_OTHER_MESSAGES:
                prepare packet;
                sendPacket(acdrbase, &packet, acdrhandle->device->taskmp);
                error = packet.dp_Res2;
                newhandle->oldfh=packet.dp_Res1;
                iofs->IOFS.io_Unit=(struct Unit *)newhandle;
                break;
#endif

            default:
                D(bug("[acdr] unkown fsa %d\n", iofs->IOFS.io_Command));
                retval = DOSFALSE;
                error = ERROR_ACTION_NOT_KNOWN;
		break;
		
            } /* switch (iofs->IOFS.io_Command) */
	    
            iofs->io_DosError = error;
            ReplyMsg(&iofs->IOFS.io_Message);
	    
        } /* ((iofs=(struct IOFileSys *)GetMsg(&acdrbase->port))) */
	
        Wait(1<<asfsbase->port.mp_SigBit);
	
    } /* for (;;) */
}

void *ASFS_GetData(struct ASFSBase *asfsbase)
{
    struct Process *proc;
    struct ProcNode *node;

    proc = (struct Process *)FindTask(NULL);
    node = (struct ProcNode *)asfsbase->process_list.lh_Head;
    
    while (node->ln.ln_Succ)
    {
	if (node->proc == proc)
	    return node->data;
	node = (struct ProcNode *)node->ln.ln_Succ;
    }
    return NULL;
}

