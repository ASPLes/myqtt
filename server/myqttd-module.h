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
#ifndef __MYQTTD_MODULE_H__
#define __MYQTTD_MODULE_H__

#include <myqttd.h>

void               myqttd_module_init        (MyQttdCtx * ctx);

MyQttdModule     * myqttd_module_open        (MyQttdCtx * ctx, 
					      const char    * module);

void               myqttd_module_unload      (MyQttdCtx * ctx,
						  const char    * module);

const char       * myqttd_module_name        (MyQttdModule * module);

ModInitFunc        myqttd_module_get_init    (MyQttdModule * module);

ModCloseFunc       myqttd_module_get_close   (MyQttdModule * module);

axl_bool           myqttd_module_exists      (MyQttdModule * module);

axl_bool           myqttd_module_register    (MyQttdModule * module);

void               myqttd_module_unregister  (MyQttdModule * module);

MyQttdModule     * myqttd_module_open_and_register (MyQttdCtx * ctx, 
						    const char * location);

void               myqttd_module_skip_unmap  (MyQttdCtx * ctx, 
					      const char * mod_name);

void               myqttd_module_free        (MyQttdModule  * module);

axl_bool           myqttd_module_notify      (MyQttdCtx         * ctx, 
					      MyQttdModHandler    handler,
					      axlPointer              data,
					      axlPointer              data2,
					      axlPointer              data3);

void               myqttd_module_notify_reload_conf (MyQttdCtx * ctx);

void               myqttd_module_notify_close (MyQttdCtx * ctx);

void               myqttd_module_set_no_unmap_modules (axl_bool status);

void               myqttd_module_cleanup      (MyQttdCtx * ctx);

#endif
