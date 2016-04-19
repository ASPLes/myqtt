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
