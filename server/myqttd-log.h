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
#ifndef __MYQTTD_LOG_H__
#define __MYQTTD_LOG_H__

#include <myqttd.h>

void      myqttd_log_init         (MyQttdCtx * ctx);

typedef enum {LOG_REPORT_GENERAL = 1, 
	      LOG_REPORT_ACCESS  = 1 << 2, 
	      LOG_REPORT_MYQTT  = 1 << 3,
	      LOG_REPORT_ERROR   = 1 << 4,
	      LOG_REPORT_WARNING = 1 << 5
} LogReportType;

void      myqttd_log_report (MyQttdCtx * ctx,
				 LogReportType   type, 
				 const char    * message,
				 va_list         args,
				 const char    * file,
				 int             line);

void      myqttd_log_configure (MyQttdCtx * ctx,
				LogReportType   type,
				int             descriptor);

void      myqttd_log_manager_start (MyQttdCtx * ctx);

void      myqttd_log_manager_register (MyQttdCtx * ctx,
				       LogReportType   type,
				       int             descriptor);

axl_bool  myqttd_log_is_enabled    (MyQttdCtx * ctx);

void      myqttd_log_cleanup       (MyQttdCtx * ctx);

void      __myqttd_log_reopen      (MyQttdCtx * ctx);

#endif
