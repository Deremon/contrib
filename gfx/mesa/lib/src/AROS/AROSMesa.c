/* $Id$ */

/*
 * Mesa 3-D graphics library
 * Copyright (C) 1995  Brian Paul  (brianp@ssec.wisc.edu)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
$Log$
Revision 1.3  2005/01/12 13:09:01  NicJA
tidied remaining debug output

Revision 1.2  2005/01/12 11:11:40  NicJA
removed extremely broken "FAST" double bufering routines for the time being - will be placed into seperate files later..

Revision 1.1  2005/01/11 14:58:29  NicJA
AROSMesa 3.0

- Based on the official mesa 3 code with major patches to the amigamesa driver code to get it working.
- GLUT not yet started (ive left the _old_ mesaaux, mesatk and demos in for this reason)
- Doesnt yet work - the _db functions seem to be writing the data incorrectly, and color picking also seems broken somewhat - giving most things a blue tinge (those that are currently working)

Revision 1.16-a1.3  2004/06/29 00:32:39  NicJA
A few build related fixes (remove windows line endings in make scripts - python doesnt like them).

Revision 1.16-a1.2  2004/02/25 02:46:04  NicJA
updates updates updates... now builds with the standard aros mmakefile.src system - use make contrib-mesa2 to build

Revision 1.16-a1.1  2003/08/09 00:23:14  chodorowski
Amiga Mesa 2.2 ported to AROS by Nic Andrews. Build with 'make aros'. Not built by default.

 * Revision 1.16  1997/06/25  19:16:56  StefanZ
 * New drawing routines:
 * - Now in separate files
 * - cyberGfx added.
 *
 * Revision 1.15  1996/10/13  20:54:59  StefanZ
 * A few nasty bugfixes.
 *
 * Revision 1.14  1996/10/07  00:11:07  StefanZ
 * Mesa 2.0 Fixed
 *
 * Revision 1.13  1996/10/06  20:31:50  StefanZ
 * Source Bump before Mesa 2.0
 *
 * Revision 1.12  1996/08/14  22:17:32  StefanZ
 * New API to amigacalls (uses taglist)
 * Made it more flexible to add gfx-card support.
 * Fast doublebuff routines
 * minor bugfixes
 *
 * Revision 1.10    1996/06/12  13:06:00  StefanZ
 * BugFix and encatments by Jorge Acereda (JAM)
 *
 * Revision 1.9  1996/06/11  15:53:02   StefanZ
 * Little speedup, VerryFast colorallocation by Stefan Burst�m
 *
 * Revision 1.8  1996/06/01  23:57:47   StefanZ
 * Started to use RCS to keep track of code.
 *
 * Revision 1.7  1996/05/21  23:08:42   StefanZ
 * A few bug and enforcer fixes
 *
 * Revision 1.6  1996/04/29  22:14:31   StefanZ
 * BugFixes reported by by Daniel J�nsson
 *
 * Revision 1.5  1996/03/14  23:54:33   StefanZ
 * Doublebuffer & Tmprastport seams to work (big speed improvment)
 * a fastpolydraw is also implemented
 *
 * Revision 1.4  1996/03/07  16:55:04   StefanZ
 * Much of the code works now (RGB mode is simulated) Doublebuffers... (didn't work)
 *
 * Revision 1.3  1996/02/29  02:12:45   StefanZ
 * First sight of colors (even the right ones) maglight.c works
 *
 * Revision 1.2  1996/02/25  13:11:16   StefanZ
 * First working version. Draws everything with the same color
 * (Colormaping is now urgent needed)
 *
 * Revision 1.1  1996/02/23  22:01:15   StefanZ
 * Made changes to match latest version of ddsample 1.5
 *
 * Revision 1.0  1996/02/21  11:01:15   StefanZ
 * File created from ddsample.c ver 1.3 and amesa.c ver 1.5
 * in a brave atempt to rebuild the amiga version
 *
 */

/*
TODO:
Dynamic allocate the vectorbuffer for polydrawing. (memory improvment)
implement shared list.
fix resizing bug.
some native asm routine
fast asm line drawin in db mode
fast asm clear       in db mode
read buffer routines  in db-mode

IDEAS:
 Make the gl a sharedlibrary. (Have ben started look in /amiga)
*/

/*
 * Note that you'll usually have to flip Y coordinates since Mesa's
 * window coordinates start at the bottom and increase upward.  Most
 * window system's Y-axis increases downward
 *
 * See dd.h for more device driver info.
 * See the other device driver implementations for ideas.
 *
 */

#include "AROSMesa_intern.h"

#include <exec/memory.h>
#include <exec/types.h>
#ifdef __GNUC__
#ifndef __AROS__
#include <inline/exec.h>
#include <inline/intuition.h>
#include <inline/graphics.h>
#include <inline/utility.h>
#else
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#endif
#else
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#endif

#ifdef ADISP_CYBERGFX
#ifdef __GNUC__
#include <proto/cybergraphics.h>
#else
#include <inline/cybergraphics.h>
#endif
#endif

#include <GL/AROSMesa.h>

#ifdef ADISP_CYBERGFX
#include "AROSMesa_TC.h"
#endif

#ifdef ADISP_AGA
#include "AROSMesa_8bit.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>
#include "context.h"
#include "dd.h"
#include "xform.h"
#include "macros.h"
#include "vb.h"

/* #define DEBUGPRINT */

#define DEBUG 1
#include <aros/debug.h>

/**********************************************************************/
/*****                Internal Data                                         *****/
/**********************************************************************/

GLenum          LastError;                /* The last error generated*/
struct Library *CyberGfxBase = NULL;      /* optional base address for cybergfx */

/**********************************************************************/
/*****                  AROS/Mesa API Functions                  *****/
/**********************************************************************/
/*
 * Implement the client-visible AROS/Mesa interface functions defined
 * in Mesa/include/GL/AROSmesa.h
 *
 **********************************************************************/
/*
 * Implement the public AROS/Mesa interface functions defined
 * in Mesa/include/GL/AROSMesa.h
 */
 
struct arosmesa_visual *
AROSMesaCreateVisualTags(long Tag1, ...)
{
    return AROSMesaCreateVisual(&Tag1);
}

struct arosmesa_visual *
AROSMesaCreateVisual(register struct TagItem *tagList)
{
    struct arosmesa_visual *v=NULL;
    int  index_bits, redbits, greenbits, bluebits, alphabits;
    
    if (!(v = (struct arosmesa_visual *)AllocVec(sizeof(struct arosmesa_visual),MEMF_PUBLIC|MEMF_CLEAR))) return NULL;

    v->rgb_flag=GetTagData(AMA_RastPort,GL_TRUE,tagList);
    v->db_flag=GetTagData(AMA_DoubleBuf,GL_FALSE,tagList);
    v->alpha_flag=GetTagData(AMA_AlphaFlag,GL_FALSE,tagList);

    if (v->rgb_flag) {
        /* RGB(A) mode */
        redbits = greenbits = bluebits = 8;
        alphabits = 0;
        index_bits = 0;
    }
    else {
        /* color index mode */
        redbits = greenbits = bluebits = alphabits = 0;
        index_bits = 8;           /* @@@ TODO */
    }
    
    /* Create core visual */
    v->gl_visual = gl_create_visual( v->rgb_flag, 
          v->alpha_flag,
          v->db_flag,
          GL_FALSE,                             /* stereo */
          16,                                   /* depth_size */
          8,                                    /* stencil_size */
          16,                                   /* accum_size */
          index_bits,
          redbits, greenbits,
          bluebits, alphabits
          );

    return v;
}

void AROSMesaDestroyVisual( struct arosmesa_visual *v )
{
    gl_destroy_visual( v->gl_visual );
    FreeVec( v );
}

struct arosmesa_buffer *AROSMesaCreateBuffer( struct arosmesa_visual *visual,int windowid)
{
    struct arosmesa_buffer *b=NULL;
    
    if (!(b = (struct arosmesa_buffer *) AllocVec(sizeof(struct arosmesa_buffer),MEMF_PUBLIC|MEMF_CLEAR))) return NULL;
    
    b->gl_buffer = gl_create_framebuffer( visual->gl_visual);
    
    /* other stuff */
    
    return b;
}

void AROSMesaDestroyBuffer( struct arosmesa_buffer *b )
{
    gl_destroy_framebuffer( b->gl_buffer );
    FreeVec( b );
}

struct arosmesa_context *AROSMesaCreateContextTags(long Tag1, ...)
{
    return AROSMesaCreateContext(&Tag1);
}

struct arosmesa_context *
AROSMesaCreateContext(register struct TagItem *tagList)
{
    /* Create a new AROS/Mesa context */
    /* Be sure to initialize the following in the core Mesa context: */
    /* DrawBuffer, ReadBuffer */    /* @@@ IMPLEMENTERA ???*/

/*  GLfloat redscale,greenscale,bluescale,alphascale; */
/*  int         I; */
/*	int drawMode; */
    struct arosmesa_context *c=NULL;

    /* try to open cybergraphics.library */
    if (CyberGfxBase==NULL)
    {
        CyberGfxBase = OpenLibrary((UBYTE*)"cybergraphics.library",0);
    }

    /* allocate arosmesa_context struct initialized to zeros */
    if (!(c = (struct arosmesa_context *) AllocVec(sizeof(struct arosmesa_context),MEMF_PUBLIC|MEMF_CLEAR)))
    {
            LastError=AMESA_OUT_OF_MEM;
            return(NULL);
    }

    if(!(c->visual = (struct arosmesa_visual *)GetTagData(AMA_Visual,NULL,tagList)))
    {
        if (!(c->visual = AROSMesaCreateVisual(tagList)))
        {
            LastError=AMESA_OUT_OF_MEM;
            return NULL;
        }
        c->flags=c->flags||0x1;
    }

    if(!(c->buffer = (struct arosmesa_buffer *)GetTagData(AMA_Buffer,NULL,tagList)))
    {
        if(!(c->buffer = AROSMesaCreateBuffer( c->visual,GetTagData(AMA_WindowID,1,tagList))))
        {
            LastError=AMESA_OUT_OF_MEM;
            return NULL;
        }
        c->flags=c->flags||0x2;
    }

    c->share=(struct arosmesa_context *)GetTagData(AMA_ShareGLContext,NULL,tagList);
    struct arosmesa_context *sharetmp = c->share;
    c->gl_ctx = gl_create_context(  c->visual->gl_visual,
          sharetmp ? sharetmp->gl_ctx : NULL,
          (void *) c, GL_TRUE);

/*  drawMode=GetTagData(AMA_DrawMode,AMESA_AGA,tagList);  */
/* Disabled for now .. (nicja)
#ifdef AMESA_DOUBLEBUFFFAST
    if(c->visual->db_flag==GL_TRUE)
    {
#ifdef DEBUGPRINT
D(bug("[AMESA] : AROSMesaCreateContext -> doublebuffered\n"));
#endif
        if (CyberGfxBase)
        {
            if (!(arosTC_Standard_init_db(c,tagList)))
            {
                gl_destroy_context( c->gl_ctx );
                FreeVec( c );
                return NULL;
            }
        }
        else
        {
//          if (aros8bit_Standard_init_db(c,tagList))   Not realy finished/working
            if (!(aros8bit_Standard_init(c,tagList)))
            {
                gl_destroy_context( c->gl_ctx );
                FreeVec( c );
                return NULL;
            }
        }
    }
    else  // allways fallback on AGA when unknown drawmode
#endif*/
    {
#ifdef DEBUGPRINT
D(bug("[AMESA] : AROSMesaCreateContext -> unbuffered\n"));
#endif
        if (CyberGfxBase)
        {
            if (!(arosTC_Standard_init(c,tagList)))
            {
                gl_destroy_context( c->gl_ctx );
                FreeVec( c );
                return NULL;
            }
        }
        else
        {
            if (!(aros8bit_Standard_init(c,tagList)))
            {
                gl_destroy_context( c->gl_ctx );
                FreeVec( c );
                return NULL;
            }
        }
    }
    if (c->gl_ctx) c->gl_ctx->Driver.UpdateState = c->InitDD;
    return c;
  
}

void AROSMesaDestroyContext(register struct arosmesa_context *c )
{
    /* destroy a AROS/Mesa context */

    (*c->Dispose)( c );

    if(c->flags&&0x1) AROSMesaDestroyVisual(c->visual);
    if(c->flags&&0x2) AROSMesaDestroyBuffer(c->buffer);
    
    gl_destroy_context( c->gl_ctx );
    FreeVec( c );
}

void AROSMesaMakeCurrent(register struct arosmesa_context *amesa,register struct arosmesa_buffer *b )
{
    /* Make the specified context the current one */
    /* the order of operations here is very important! */

    if (amesa && b) {
        gl_make_current( amesa->gl_ctx,b->gl_buffer );

        (*amesa->InitDD)(amesa->gl_ctx);                            /* Call Driver_init_routine */

        if (amesa->gl_ctx->Viewport.Width==0) {
#ifdef DEBUGPRINT
D(bug("[AMESA] : AROSMesaMakeCurrent: call glViewport(0,0,%d,%d)", amesa->width, amesa->height));
#endif
            glViewport( 0, 0, amesa->width, amesa->height );
        }
	else
        {
            gl_make_current( NULL,NULL);
        }
    }
}

void AROSMesaSwapBuffers(register struct arosmesa_context *amesa)
{                /* copy/swap back buffer to front if applicable */
    (*amesa->SwapBuffer)( amesa );
}

/* This is on the drawingboard */
/* Mostly for when future changes the library is still intact*/

BOOL AROSMesaSetDefs(register struct TagItem *tagList)
{
/*
    struct TagItem *tag=NULL;
            
    tagValue=AMA_DrawMode;
    tag = FindTagItem(tagValue,tagList);
#ifdef DEBUGPRINT
    if (tag)
D(bug("[AMESA] : AROSMesaSetDefs: Tag=0x%x, is 0x%x/n",tagValue,tag->ti_Data));
    else
D(bug("[AMESA] : AROSMesaSetDefs: Tag=0x%x is not specified/n",tagValue));
#endif
*/
    return FALSE;
}

/*
 Maybe a report error routine ??? like:
*/

GLenum AROSMesaReportError(register struct arosmesa_context *c )
{
    GLenum error;
    error=LastError;
    LastError=0;
    return(error);
}

#undef DEBUGPRINT
