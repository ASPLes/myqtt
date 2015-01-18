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

#ifndef __MYQTTD_PROCESS_H__
#define __MYQTTD_PROCESS_H__

#include <myqttd.h>

void              myqttd_process_init         (MyQttdCtx * ctx, 
					       axl_bool        reinit);

void              myqttd_process_create_child (MyQttdCtx       * ctx, 
					       MyQttConn       * conn,
					       axl_bool          handle_reply,
					       char            * serverName,
					       MyQttMsg        * msg);

void              myqttd_process_kill_childs  (MyQttdCtx * ctx);

int               myqttd_process_child_count  (MyQttdCtx * ctx);

axlList         * myqttd_process_child_list (MyQttdCtx * ctx);

MyQttdChild     * myqttd_process_child_by_id (MyQttdCtx * ctx, int pid);

axl_bool          myqttd_process_child_exists  (MyQttdCtx * ctx, int pid);

void              myqttd_process_check_for_finish (MyQttdCtx * ctx);

void              myqttd_process_cleanup      (MyQttdCtx * ctx);

/* internal API */
axl_bool myqttd_process_parent_notify (MyQttdLoop    * loop, 
				       MyQttdCtx     * ctx,
				       int             descriptor, 
				       axlPointer      ptr, 
				       axlPointer      ptr2);

void              myqttd_process_set_file_path (const char * path);

void              myqttd_process_set_child_cmd_prefix (const char * cmd_prefix);

axl_bool          __myqttd_process_create_parent_connection (MyQttdChild * child);

MyQttConn * __myqttd_process_handle_connection_received (MyQttdCtx         * ctx, 
							 MyQttdChild       * child,
							 MYQTT_SOCKET        socket, 
							 char               * conn_status);

char *           myqttd_process_connection_status_string (axl_bool          handle_reply,
							  const char      * serverName,
							  int               has_tls,
							  int               fix_server_name,
							  const char      * remote_host,
							  const char      * remote_port,
							  const char      * remote_host_ip,
							  /* this must be the last attribute */
							  int               skip_conn_recover);

void             myqttd_process_connection_recover_status (char            * ancillary_data,
							   axl_bool        * handle_reply,
							   const char     ** serverName,
							   int             * fix_server_name,
							   const char     ** remote_host,
							   const char     ** remote_port,
							   const char     ** remote_host_ip,
							   int             * has_tls);

axl_bool myqttd_process_send_socket (MYQTT_SOCKET     socket, 
				     MyQttdChild    * child, 
				     const char     * ancillary_data, 
				     int              size);

#endif
