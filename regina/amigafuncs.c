/*
 *  Amiga REXX functions for regina
 *  Copyright � 2002, Staf Verhaegen
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "rexx.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

typedef struct _amiga_tsd_t {
  proclevel amilevel;
} amiga_tsd_t;


/* Init amiga specific thread data, this function is called during initialisation
 * of the thread specific data
 */
int init_amigaf ( tsd_t *TSD )
{
  amiga_tsd_t *atsd = MallocTSD( sizeof(amiga_tsd_t) );
  
  if (atsd==NULL) return 0;

  /* Allocate later because systeminfo is not initialized at the moment */
  atsd->amilevel = NULL;
  TSD->ami_tsd = (void *)atsd;

  return 1;
}


/*
 * Support functions for the ARexx IO functions
 */
/* setamilevel will change the environment to the variables used for open files */
static proclevel setamilevel( tsd_t *TSD )
{
  amiga_tsd_t *atsd = (amiga_tsd_t *)TSD->ami_tsd;
  proclevel oldlevel = TSD->currlevel;
  
  if (atsd->amilevel==NULL)
  {
    atsd->amilevel = newlevel( TSD, NULL );
  }

  TSD->currlevel = atsd->amilevel;
  
  return oldlevel;
}


/* getfile will return the FILE pointer of given name */
static FILE *getfile( tsd_t *TSD, const streng *name )
{
  proclevel oldlevel = setamilevel( TSD );
  const streng *s;
  char *txt;
  FILE *file=NULL;

  if ( isvariable( TSD, name ) )
  {
    s = getvalue( TSD, name, 0 );
    txt = str_of( TSD, s );
    sscanf( txt, "%p", &file );
    FreeTSD( txt );
  }

  TSD->currlevel = oldlevel;
  
  return file;
}


/* addfile: store the FILE pointer in a given name */
static void addfile( tsd_t *TSD, const streng *name, FILE *file )
{
  proclevel oldlevel = setamilevel( TSD );
  char txt[20];
  streng *s;

  sprintf( txt, "%p", (void *)file );
  s = Str_cre_TSD( TSD, txt );
  setvalue( TSD, name, s );
  
  TSD->currlevel = oldlevel;
}


/* rmfile: remove a given of open files list */
static void rmfile( tsd_t *TSD, const streng *name )
{
  amiga_tsd_t *atsd = (amiga_tsd_t *)TSD->ami_tsd;
  proclevel oldlevel = setamilevel( TSD );
  
  TSD->currlevel = atsd->amilevel;
  
  drop_var( TSD, name );
  
  TSD->currlevel = oldlevel;
}


/*
 * Implementation of the ARexx IO functions
 * See general documentation for more information
 * Functions implemented: OPEN, CLOSE, READCH, READLN, WRITECH, WRITELN, EOF, SEEK
 */
streng *arexx_open( tsd_t *TSD, cparamboxptr parm1 )
{
  cparamboxptr parm2, parm3;
  char *filename;
  FILE *file;
  int mode;
  static const char* modestrings[] = {
    "w",
    "r+",
    "a"
  };

  checkparam( parm1, 2, 3, "OPEN" );
  parm2 = parm1->next;
  parm3 = parm2->next;
  
  file = getfile( TSD, parm1->value );
  if ( file!=NULL )
  {
    return int_to_streng( TSD, 0 );
  }

  filename = str_of( TSD, parm2->value );
  
  if ( parm3==NULL
       || parm3->value==NULL
       || parm3->value->len==0 )
    mode=0;
  else switch( getoptionchar( TSD, parm3->value, "OPEN", 3, "WRA" ) )
  {
    case 'W':
      mode=0;
      break;
    
    case 'R':
      mode=1;
      break;
      
    case 'A':
      mode=2;
      break;
      
    default:
      assert(0);
      break;
  }

  file = fopen( filename, modestrings[mode] );
  FreeTSD( filename );

  if ( file==NULL )
  {
    return int_to_streng( TSD, 0 );
  }
  
  addfile( TSD, parm1->value, file );
  return int_to_streng( TSD, 1);
}


streng *arexx_close( tsd_t *TSD, cparamboxptr parm1 )
{
  FILE *file;
  
  checkparam( parm1, 1, 1, "CLOSE" );
  
  file = getfile( TSD, parm1->value );
  if ( file==NULL )
    return int_to_streng( TSD, 0 );

  fclose( file );
  rmfile( TSD, parm1->value );
  
  return int_to_streng( TSD, 1 );
}


streng *arexx_writech( tsd_t *TSD, cparamboxptr parm1 )
{
  cparamboxptr parm2;
  char *txt;
  FILE *file;
  int count;
  
  checkparam( parm1, 2, 2, "WRITECH" );
  parm2 = parm1->next;

  file = getfile( TSD, parm1->value );
  if ( file==NULL )
    exiterror( ERR_INCORRECT_CALL, 27, "WRITECH", tmpstr_of( TSD, parm1->value ));
  
  txt = str_of( TSD, parm2->value );
  count = fprintf( file, "%s", txt );
  FreeTSD( txt );
  
  return int_to_streng( TSD, count );
}


streng *arexx_writeln( tsd_t *TSD, cparamboxptr parm1 )
{
  cparamboxptr parm2;
  char *txt;
  FILE *file;
  int count;
  
  checkparam( parm1, 2, 2, "WRITELN" );
  parm2 = parm1->next;

  file = getfile( TSD, parm1->value );
  if ( file==NULL )
    exiterror( ERR_INCORRECT_CALL, 27, "WRITELN", tmpstr_of( TSD, parm1->value ) );

  txt = str_of( TSD, parm2->value );
  count = fprintf(file, "%s\n", txt);
  FreeTSD( txt );

  return int_to_streng( TSD, count );
}


streng *arexx_seek( tsd_t *TSD, cparamboxptr parm1 )
{
  cparamboxptr parm2, parm3;
  FILE *file;
  int pos, error, wench;
  long offset;
  
  checkparam( parm1, 2, 3, "SEEK" );
  parm2 = parm1->next;
  parm3 = parm2->next;
  
  file = getfile( TSD, parm1->value );
  if ( file==NULL )
    exiterror( ERR_INCORRECT_CALL, 27, "SEEK", tmpstr_of( TSD, parm1->value ) );
  
  offset = streng_to_int( TSD, parm2->value, &error );
  if (error)
    exiterror( ERR_INCORRECT_CALL, 11, "SEEK", 2, tmpstr_of( TSD, parm2->value ) );
  
  if ( parm3==NULL
       || parm3->value==NULL
       || parm3->value->len == 0 )
    wench = SEEK_CUR;
  else switch( getoptionchar( TSD, parm3->value, "SEEK", 3, "CBE" ) )
  {
    case 'C':
      wench = SEEK_CUR;
      break;
      
    case 'B':
      wench = SEEK_SET;
      break;
      
    case 'E':
      wench = SEEK_END;
      break;
      
    default:
      assert(0);
      break;
  }
  
  pos = fseek( file, offset, wench );
  return int_to_streng( TSD, pos );
}


streng *arexx_readch( tsd_t *TSD, cparamboxptr parm1 )
{
  cparamboxptr parm2;
  FILE *file;
  
  checkparam( parm1, 1, 2, "READCH");
  parm2 = parm1->next;
  
  file = getfile( TSD, parm1->value );
  if ( file==NULL )
    exiterror( ERR_INCORRECT_CALL, 27, "READCH", tmpstr_of( TSD, parm1->value ) );
  
  if ( parm2==NULL )
  {
    char buffer[2] = { 0, 0 };
    
    buffer[0] = getc( file );
    
    return Str_cre_TSD( TSD, buffer );
  }
  else
  {
    int count, error;
    char *buffer;
    streng *ret;
    
    count = streng_to_int( TSD, parm2->value, &error );
    
    if ( error || count<=0 )
      exiterror( ERR_INVALID_INTEGER, 14, "READCH", 2, tmpstr_of( TSD, parm2->value ) );
    
    buffer = malloc( count + 1 );

    count = fread( buffer, 1, count, file );
    buffer[count + 1] = 0;
    
    ret = Str_cre_TSD( TSD, buffer );
    free(buffer);
    return ret;
  }
}


streng *arexx_readln( tsd_t *TSD, cparamboxptr parm )
{
  FILE *file;
  char buffer[1001];
  
  checkparam( parm, 1, 1, "READLN");
  
  file = getfile( TSD, parm->value );
  if ( file==NULL )
    exiterror( ERR_INCORRECT_CALL, 27, "READLN", tmpstr_of( TSD, parm->value ) );
  
  fgets( buffer, 1001, file );
  if ( buffer[strlen(buffer)-1]=='\n' )
    buffer[strlen(buffer)-1]=0;
  
  return Str_cre_TSD( TSD, buffer );
}


streng *arexx_eof( tsd_t *TSD, cparamboxptr parm )
{
  FILE *file;
  
  checkparam( parm, 1, 1, "EOF" );
  
  file = getfile( TSD, parm->value );
  if ( file==NULL )
    exiterror( ERR_INCORRECT_CALL, 27, "EOF", tmpstr_of( TSD, parm->value ) );
  
  return int_to_streng( TSD, feof( file )!=0 );
}


/* 
 * Implementation of the additional conversion functions from ARexx
 * Functions: B2C, C2B
 */
streng *arexx_b2c( tsd_t *TSD, cparamboxptr parm )
{
  parambox parm2;
  streng *ret;
  
  checkparam( parm, 1, 1, "B2C" );

  parm2.next = NULL;
  parm2.value = std_b2x( TSD, parm );
  
  ret = std_x2c( TSD, &parm2 );
  Free_string_TSD( TSD, parm2.value );
  
  return ret;
}


streng *arexx_c2b( tsd_t *TSD, cparamboxptr parm )
{
  parambox parm2;
  streng *ret;
  
  checkparam( parm, 1, 1, "B2C" );

  parm2.next = NULL;
  parm2.value = std_c2x( TSD, parm );
  
  ret = std_x2b( TSD, &parm2 );
  Free_string_TSD( TSD, parm2.value );
  
  return ret;
}


/*
 * Implementation of the bitwise function from ARexx
 * Functions: BITCHG, BITCLR, BITSET, BITTST
 */
streng *arexx_bitchg( tsd_t *TSD, cparamboxptr parm1 )
{
  cparamboxptr parm2;
  streng *ret;
  int bit, error, byte;
  div_t dt;
  
  checkparam( parm1, 2, 2, "BITCHG" );
  parm2 = parm1->next;
  
  bit = streng_to_int( TSD, parm2->value, &error );
  if (error || bit<0 )
    exiterror( ERR_INCORRECT_CALL, 13, "BITCHG", 2, tmpstr_of( TSD, parm2->value ) );
  
  dt = div( bit, 8 );

  byte = parm1->value->len-dt.quot-1;
  if ( byte<0 )
    exiterror( ERR_INCORRECT_CALL, 0 );
  
  ret = Str_dup_TSD( TSD, parm1->value );
  ret->value[byte]^=(char)(1<<dt.rem);
  return ret;
}


streng *arexx_bitclr( tsd_t *TSD, cparamboxptr parm1 )
{
  cparamboxptr parm2;
  streng *ret;
  int bit, error, byte;
  div_t dt;
  
  checkparam( parm1, 2, 2, "BITCLR" );
  parm2 = parm1->next;
  
  bit = streng_to_int( TSD, parm2->value, &error );
  if (error)
    exiterror( ERR_INCORRECT_CALL, 13, "BITCLR", 2, tmpstr_of( TSD, parm2->value ) );
  
  dt = div( bit, 8 );
  
  byte = parm1->value->len-dt.quot-1;
  if ( byte<0 )
    exiterror( ERR_INCORRECT_CALL, 0 );
  
  ret = Str_dup_TSD( TSD, parm1->value );
  ret->value[byte]&=~(char)(1<<dt.rem);
  return ret;
}


streng *arexx_bitset( tsd_t *TSD, cparamboxptr parm1 )
{
  cparamboxptr parm2;
  streng *ret;
  int bit, error, byte;
  div_t dt;
  
  checkparam( parm1, 2, 2, "BITSET" );
  parm2 = parm1->next;
  
  bit = streng_to_int( TSD, parm2->value, &error );
  if (error)
    exiterror( ERR_INCORRECT_CALL, 13, "BITSET", 2, tmpstr_of( TSD, parm2->value ) );
  
  dt = div( bit, 8 );
  
  byte = parm1->value->len-dt.quot-1;
  if ( byte<0 )
    exiterror( ERR_INCORRECT_CALL, 0 );
  
  ret = Str_dup_TSD( TSD, parm1->value );
  ret->value[byte]|=(char)(1<<dt.rem);
  return ret;
}


streng *arexx_bittst( tsd_t *TSD, cparamboxptr parm1 )
{
  cparamboxptr parm2;
  streng *ret;
  int bit, error, byte;
  div_t dt;
  
  checkparam( parm1, 2, 2, "BITTST" );
  parm2 = parm1->next;
  
  bit = streng_to_int( TSD, parm2->value, &error );
  if (error)
    exiterror( ERR_INCORRECT_CALL, 13, "BITTST", 2, tmpstr_of( TSD, parm2->value ) );
  
  dt = div( bit, 8 );
  
  byte = parm1->value->len-dt.quot-1;
  if ( byte<0 )
    exiterror( ERR_INCORRECT_CALL, 0 );
  
  ret = int_to_streng( TSD, (parm1->value->value[byte] & (char)(1<<dt.rem))!=0 );
  return ret;
}


/*
 * Some more misc. ARexx functions
 * Functions: COMPRESS, HASH, RANDU, TRIM, UPPER
 */
streng *arexx_hash( tsd_t *TSD, cparamboxptr parm1 )
{
  unsigned char *uc;
  int i, sum=0;
  
  checkparam( parm1, 1, 1, "HASH" );

  uc = (unsigned char *)parm1->value->value;
  for( i=0; i<parm1->value->len; i++)
  {
    sum = (sum + uc[i]) & 255;
  }
  
  return int_to_streng( TSD, sum );
}


streng *arexx_compress( tsd_t *TSD, cparamboxptr parm1 )
{
  const char *match;
  int i, start;
  streng *ret;
  
  checkparam( parm1, 1, 2, "COMPRESS" );

  match = ( parm1->next!=NULL ) ? str_of( TSD, parm1->next->value ) : " ";

  ret = Str_dup_TSD( TSD, parm1->value );
  for( i=start=0; i<ret->len; i++ )
  {
    /* Copy char if not found */
    if ( strchr( match, ret->value[i] )==NULL )
    {
      ret->value[start] = ret->value[i];
      start++;
    }
  }
  ret->len = start;

  if ( parm1->next!=NULL )
    FreeTSD( (char *)match );
  
  return ret;
}


static const streng T_str = { 1, 1, "T" };
static const parambox T_parm = { NULL, 0, &T_str };

streng *arexx_trim( tsd_t *TSD, cparamboxptr parm1 )
{
  checkparam( parm1, 1, 1, "TRIM" );

  parm1->next = &T_parm;
  
  return std_strip( TSD, parm1 );
}


streng *arexx_upper( tsd_t *TSD, cparamboxptr parm1 )
{
  int i;
  streng *ret;
  
  checkparam( parm1, 1, 1, "UPPER" );
  
  ret = Str_dup_TSD( TSD, parm1->value );

   return Str_upper( ret );
}


streng *arexx_randu( tsd_t *TSD, cparamboxptr parm1 )
{
  int error, seed;
  char text[30];
  
  checkparam( parm1, 0, 1, "RANDU" );
  
  if ( parm1!=NULL && parm1->value!=NULL )
  {
    seed = streng_to_int( TSD, parm1->value, &error );
    if (error)
      exiterror( ERR_INVALID_INTEGER, 11, "RANDU", 1, tmpstr_of( TSD, parm1->value ) );
    
    srand48( (long int)seed );
  }
  
  sprintf( text, "%f", drand48() );
  return Str_cre_TSD( TSD, text );
}
