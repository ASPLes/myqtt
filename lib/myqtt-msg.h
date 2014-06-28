/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2014 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build MQTT enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/mqtt
 *                        http://www.aspl.es/myqtt
 */
#ifndef __MYQTT_MSG_H__
#define __MYQTT_MSG_H__

#include <myqtt.h>

/**
 * \addtogroup myqtt_msg
 * @{
 */

MyQttMsg    * myqtt_msg_get_next              (MyQttConn   * conn);

MyQttMsgType  myqtt_msg_get_type              (MyQttMsg    * msg);

const char  * myqtt_msg_get_type_str          (MyQttMsg    * msg);

axl_bool      myqtt_msg_send_raw              (MyQttConn            * conn, 
					       const unsigned char  * msg, 
					       int                    msg_size);

int           myqtt_msg_receive_raw           (MyQttConn          * conn, 
					       unsigned char      * buffer, 
					       int                  maxlen);

unsigned char        * myqtt_msg_build        (MyQttCtx     * ctx,
					       MyQttMsgType   type,
					       axl_bool       dup,
					       MyQttQos       qos,
					       axl_bool       retain,
					       int          * size,
					       ...);

void          myqtt_msg_free_build            (MyQttCtx * ctx, 
					       unsigned char * msg_build, 
					       int size);

axl_bool      myqtt_msg_ref                   (MyQttMsg * msg);

void          myqtt_msg_unref                 (MyQttMsg * msg);

int           myqtt_msg_ref_count             (MyQttMsg * msg);

void          myqtt_msg_free                  (MyQttMsg * msg);

int           myqtt_msg_get_id                (MyQttMsg * msg);

int           myqtt_msg_get_payload_size      (MyQttMsg * msg);

const void *  myqtt_msg_get_payload           (MyQttMsg * msg);


/*** INTERNAL API: don't use it, it may change ***/
axl_bool myqtt_msg_encode_remaining_length (MyQttCtx * ctx, unsigned char * input, int value, int * out_position);

int      myqtt_msg_decode_remaining_length (MyQttCtx * ctx, unsigned char * input, int * out_position);

/* @} */

#endif
