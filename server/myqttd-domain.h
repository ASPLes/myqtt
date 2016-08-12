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

#ifndef __MYQTTD_DOMAIN_H__
#define __MYQTTD_DOMAIN_H__

#include <myqttd.h>

axl_bool          myqttd_domain_init (MyQttdCtx  * ctx);

axl_bool          myqttd_domain_add  (MyQttdCtx  * ctx, 
				      const char * name, 
				      const char * storage_path, 
				      const char * user_db,
				      const char * use_settings,
				      axl_bool     is_active);

const char      * myqttd_domain_get_name (MyQttdDomain * domain);

long              myqttd_domain_get_month_message_quota (MyQttdDomain * domain);

long              myqttd_domain_get_day_message_quota (MyQttdDomain * domain);

MyQttdDomain    * myqttd_domain_find_by_name (MyQttdCtx   * ctx,
					      const char  * name);

MyQttdDomain    * myqttd_domain_find_by_indications (MyQttdCtx  * ctx,
						     MyQttConn  * conn,
						     const char * username,
						     const char * client_id,
						     const char * password,
						     const char * server_Name);

axl_bool          myqttd_domain_do_auth (MyQttdCtx    * ctx,
					 MyQttdDomain * domain,
					 axl_bool       domain_selected,
					 MyQttConn    * conn,
					 const char   * username, 
					 const char   * password,
					 const char   * client_id);

int               myqttd_domain_count_enabled (MyQttdCtx * ctx);

int               myqttd_domain_conn_count (MyQttdDomain * domain);

int               myqttd_domain_conn_count_all (MyQttdDomain * domain);

MyQttdUsers     * myqttd_domain_get_users_backend (MyQttdDomain * domain);

void              myqttd_domain_cleanup (MyQttdCtx * ctx);

#endif
