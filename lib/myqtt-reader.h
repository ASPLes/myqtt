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
#ifndef __MYQTT_READER_H__
#define __MYQTT_READER_H__

#include <myqtt.h>

void myqtt_reader_watch_listener              (MyQttCtx        * ctx,
						MyQttConn * listener);

void myqtt_reader_watch_connection            (MyQttCtx        * ctx,
						MyQttConn * connection);

void myqtt_reader_unwatch_connection          (MyQttCtx        * ctx,
						MyQttConn * connection);

int  myqtt_reader_connections_watched         (MyQttCtx        * ctx);

int  myqtt_reader_run                         (MyQttCtx * ctx);

void myqtt_reader_stop                        (MyQttCtx * ctx);

int  myqtt_reader_notify_change_io_api        (MyQttCtx * ctx);

void myqtt_reader_notify_change_done_io_api   (MyQttCtx * ctx);

axl_bool  myqtt_reader_invoke_msg_received    (MyQttCtx  * ctx,
					       MyQttConn * connection,
					       MyQttMsg  * msg);

void        __myqtt_reader_prepare_wait_reply (MyQttConn * conn, int packet_id);

MyQttMsg  * __myqtt_reader_get_reply          (MyQttConn * conn, int packet_id, int timeout);

/* internal API */
typedef void (*MyQttForeachFunc) (MyQttConn * conn, axlPointer user_data);
typedef void (*MyQttForeachFunc3) (MyQttConn * conn, 
				    axlPointer         user_data, 
				    axlPointer         user_data2,
				    axlPointer         user_data3);

MyQttAsyncQueue * myqtt_reader_foreach       (MyQttCtx            * ctx,
						MyQttForeachFunc      func,
						axlPointer             user_data);

void               myqtt_reader_foreach_offline (MyQttCtx           * ctx,
						  MyQttForeachFunc3    func,
						  axlPointer            user_data,
						  axlPointer            user_data2,
						  axlPointer            user_data3);

void               myqtt_reader_restart (MyQttCtx * ctx);

#endif
