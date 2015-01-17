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

#ifndef __MYQTTD_CONN_MGR_H__
#define __MYQTTD_CONN_MGR_H__

/* internal use API */
void myqttd_conn_mgr_init          (MyQttdCtx    * ctx);

void myqttd_conn_mgr_cleanup       (MyQttdCtx * ctx);

/* public API */
axlList *  myqttd_conn_mgr_conn_list   (MyQttdCtx            * ctx, 
					MyQttPeerRole          role,
					const char           * filter);

axlList *  myqttd_conn_mgr_conn_list   (MyQttdCtx            * ctx, 
					MyQttPeerRole          role,
					const char           * filter);

int        myqttd_conn_mgr_count       (MyQttdCtx            * ctx);

axl_bool   myqttd_conn_mgr_proxy_on_parent (MyQttConn * conn);

int        myqttd_conn_mgr_setup_proxy_on_parent (MyQttdCtx * ctx, MyQttConn * conn);

MyQttConn * myqttd_conn_mgr_find_by_id (MyQttdCtx * ctx,
					int         conn_id);

/* private API */
void myqttd_conn_mgr_on_close (MyQttConn * conn, 
			       axlPointer         user_data);


#endif 
