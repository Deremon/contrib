/*
 *  The Regina Rexx Interpreter
 *  Copyright (C) 2000  Mark Hessling <M.Hessling@qut.edu.au>
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

/*
 * $Id$
 */

#include "rexxbif.h"            /* C functions that mimic REXX BIFs */

void showerror( int err, int suberr, char *tmpl, ...);
int init_external_queue( const tsd_t *TSD );
void term_external_queue( void );
int get_default_port_number( void );
int get_default_server_address( void );
int connect_to_rxstack( tsd_t *TSD, int portno, streng *server_name, int server_address ) ;
int disconnect_from_rxstack( const tsd_t *TSD, int sock );
int parse_queue( tsd_t *TSD, streng *queue, streng **server_name, int *server_address, int *portno );
int send_command_to_rxstack( const tsd_t *TSD, int sock, char *action, char *str, int len );
streng *read_result_from_rxstack( const tsd_t *TSD, int sock, int result_size );
int delete_queue_from_rxstack( const tsd_t *TSD, int sock, streng *queue_name );
int set_queue_in_rxstack( const tsd_t *TSD, int sock, streng *queue_name, streng **result );
int get_number_in_queue_from_rxstack( const tsd_t *TSD, int sock );
int get_queue_from_rxstack( const tsd_t *TSD, int sock, streng **result );
int create_queue_on_rxstack( const tsd_t *TSD, int sock, streng *queue, streng **result );
int get_line_from_rxstack( const tsd_t *TSD, int sock, streng **result );
int queue_line_lifo_to_rxstack( const tsd_t *TSD, int sock, streng *line );
int queue_line_fifo_to_rxstack( const tsd_t *TSD, int sock, streng *line );
int get_length_from_header( const tsd_t *TSD, streng *header );

#define RXSTACK_EXIT                'X'
#define RXSTACK_EXIT_STR            "X"
#define RXSTACK_KILL                'Z'
#define RXSTACK_KILL_STR            "Z"
#define RXSTACK_QUEUE_FIFO          'F'
#define RXSTACK_QUEUE_FIFO_STR      "F"
#define RXSTACK_QUEUE_LIFO          'L'
#define RXSTACK_QUEUE_LIFO_STR      "L"
#define RXSTACK_CREATE_QUEUE        'C'
#define RXSTACK_CREATE_QUEUE_STR    "C"
#define RXSTACK_DELETE_QUEUE        'D'
#define RXSTACK_DELETE_QUEUE_STR    "D"
#define RXSTACK_EMPTY_QUEUE         'E'
#define RXSTACK_EMPTY_QUEUE_STR     "E"
#define RXSTACK_PULL                'P'
#define RXSTACK_PULL_STR            "P"
#define RXSTACK_SET_QUEUE           'S'
#define RXSTACK_SET_QUEUE_STR       "S"
#define RXSTACK_GET_QUEUE           'G'
#define RXSTACK_GET_QUEUE_STR       "G"
#define RXSTACK_NUMBER_IN_QUEUE     'N'
#define RXSTACK_NUMBER_IN_QUEUE_STR "N"
#define RXSTACK_WRITE_QUEUE         'W'
#define RXSTACK_WRITE_QUEUE_STR     "W"
#define RXSTACK_UNKNOWN             '?'
#define RXSTACK_UNKNOWN_STR         "?"
#define RXSTACK_HEADER_SIZE         7

#ifndef RXSOCKET
# define RXSOCKET 5757
#endif
