/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation version 2.0 of the
 *  License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is GPL software 
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
#ifndef __TEST_01_H__
#define __TEST_01_H__

MyQttCtx        * common_init_ctx (void);

MyQttdCtx       * common_init_ctxd (MyQttCtx * myqtt_ctx, const char * config);

void              common_queue_message_received (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data);

void              common_queue_message_received_only_one (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data);

axl_bool          common_connect_send_and_check (MyQttCtx   * myqtt_ctx, 
					   const char * client_id, const char * user, const char * password,
					   const char * topic,     const char * message, 
					   const char * check_reply,
					   MyQttQos qos, 
					   axl_bool skip_error_reporting);

MyQttConn       * common_connect_and_subscribe (MyQttCtx * myqtt_ctx, const char * client_id, 
					  const char * topic, 
					  MyQttQos qos, axl_bool skip_error_reporting);

void              common_close_conn_and_ctx (MyQttConn * conn);

axl_bool          common_send_msg (MyQttConn * conn, const char * topic, const char * message, MyQttQos qos);

void              common_configure_reception_queue_received (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data);

MyQttAsyncQueue * common_configure_reception (MyQttConn * conn);

axl_bool          common_receive_and_check (MyQttAsyncQueue * queue, const char * topic, const char * message, MyQttQos qos, axl_bool skip_fail_if_null);




#endif
