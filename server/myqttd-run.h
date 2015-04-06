/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2015 Advanced Software Production Line, S.L.
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
#ifndef __MYQTTD_RUN_H__
#define __MYQTTD_RUN_H__

#include <myqttd.h>

/** 
 * \addtogroup myqttd_run
 * @{
 */

int  myqttd_run_config    (MyQttdCtx * ctx);

void myqttd_run_cleanup   (MyQttdCtx * ctx);

/** 
 * @brief Shutdown and closes the connection.
 * @param conn The connection to shutdown and close.
 */
#define TBC_FAST_CLOSE(conn) do{	                                           \
	error ("shutdowing connection id=%d..", myqtt_conn_get_id (conn));  \
        myqtt_conn_set_close_socket (conn, axl_true);                       \
	myqtt_conn_shutdown (conn);                                         \
	myqtt_conn_close    (conn);                                         \
        conn = NULL;                                                               \
	} while (0);

axl_bool myqttd_run_check_no_load_module (MyQttdCtx  * ctx, 
					  const char * module_to_check);

/** 
 * @}
 */

#endif
