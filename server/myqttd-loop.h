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
#ifndef __MYQTTD_LOOP_H__
#define __MYQTTD_LOOP_H__

#include <myqttd.h>

/** 
 * \addtogroup myqttd_loop
 * @{
 */

MyQttdLoop * myqttd_loop_create (MyQttdCtx * ctx);

MyQttdCtx  * myqttd_loop_ctx    (MyQttdLoop * loop);

void             myqttd_loop_set_read_handler (MyQttdLoop        * loop,
					       MyQttdLoopOnRead    on_read,
					       axlPointer              ptr,
					       axlPointer              ptr2);

void             myqttd_loop_watch_descriptor (MyQttdLoop        * loop,
					       int                     descriptor,
					       MyQttdLoopOnRead    on_read,
					       axlPointer              ptr,
					       axlPointer              ptr2);

void             myqttd_loop_unwatch_descriptor (MyQttdLoop        * loop,
						 int                     descriptor,
						 axl_bool                wait_until_unwatched);

int              myqttd_loop_watching (MyQttdLoop * loop);

void             myqttd_loop_close (MyQttdLoop * loop, 
				    axl_bool        notify);

/** 
 * @}
 */

#endif
