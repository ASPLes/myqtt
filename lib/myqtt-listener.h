/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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
#ifndef __MYQTT_LISTENER_H__
#define __MYQTT_LISTENER_H__

#include <myqtt.h>

/**
 * \addtogroup myqtt_listener
 * @{
 */

MyQttConn * myqtt_listener_new             (MyQttCtx             * ctx,
					    const char           * host, 
					    const char           * port, 
					    MyQttConnOpts        * opts,
					    MyQttListenerReady     on_ready, 
					    axlPointer             user_data);

MyQttConn * myqtt_listener_new6            (MyQttCtx             * ctx,
					    const char           * host, 
					    const char           * port, 
					    MyQttConnOpts        * opts,
					    MyQttListenerReady     on_ready, 
					    axlPointer             user_data);

MYQTT_SOCKET     myqtt_listener_sock_listen      (MyQttCtx    * ctx,
						  const char  * host,
						  const char  * port,
						  axlError   ** error);

MYQTT_SOCKET     myqtt_listener_sock_listen6     (MyQttCtx    * ctx,
						  const char  * host,
						  const char  * port,
						  axlError   ** error);

void          myqtt_listener_accept_connections   (MyQttCtx       * ctx,
						   int              server_socket,
						   MyQttConn      * listener);

void          myqtt_listener_accept_connection    (MyQttConn * connection, 
						    axl_bool   send_greetings);

MYQTT_SOCKET  myqtt_listener_accept               (MYQTT_SOCKET server_socket);

void          myqtt_listener_complete_register    (MyQttConn * connection);

void          myqtt_listener_wait                 (MyQttCtx  * ctx);

void          myqtt_listener_unlock               (MyQttCtx  * ctx);

void          myqtt_listener_init                 (MyQttCtx  * ctx);

void          myqtt_listener_cleanup              (MyQttCtx  * ctx);

void          myqtt_listener_set_on_connection_accepted (MyQttCtx                  * ctx,
							 MyQttOnAcceptedConnection   on_accepted, 
							 axlPointer                   data);

axlPointer    myqtt_listener_set_port_sharing_handling (MyQttCtx               * ctx, 
							 const char              * local_addr,
							 const char              * local_port, 
							 MyQttPortShareHandler    handler,
							 axlPointer                user_data);

void          myqtt_listener_shutdown (MyQttConn * listener,
					axl_bool           also_created_conns);

/*** internal API ***/
MyQttConn * __myqtt_listener_initial_accept (MyQttCtx      * ctx,
					     MYQTT_SOCKET    client_socket, 
					     MyQttConn     * listener,
					     axl_bool        dont_register);

axl_bool __myqtt_listener_check_port_sharing (MyQttCtx * ctx, MyQttConn * connection);


/* @} */

#endif
