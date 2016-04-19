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
#ifndef __MYQTTD_SUPPORT_H__
#define __MYQTTD_SUPPORT_H__

#include <myqttd.h>

char          * myqttd_support_get_backtrace (MyQttdCtx * ctx, int pid);

axl_bool        myqttd_support_smtp_send (MyQttdCtx     * ctx, 
					  const char    * mail_from,
					  const char    * mail_to,
					  const char    * subject,
					  const char    * body,
					  const char    * body_file,
					  const char    * smtp_server,
					  const char    * smtp_port);

axl_bool        myqttd_support_simple_smtp_send (MyQttdCtx     * ctx,
						 const char    * smtp_conf_id,
						 const char    * subject,
						 const char    * body,
						 const char    * body_file);

int             myqttd_support_check_mode (const char * username, const char * client_id);
					      

#endif
