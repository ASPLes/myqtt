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

#ifndef __MYQTTD_TYPES_H__
#define __MYQTTD_TYPES_H__

/** 
 * \defgroup myqttd_types Myqttd types: types used/exposed by MyQttd API
 */

/** 
 * \addtogroup myqttd_types
 * @{
 */

/** 
 * @brief Type that represents a myqttd module.
 */
typedef struct _MyQttdModule MyQttdModule;

/** 
 * @brief Type representing a child process created. Abstraction used
 * to store a set of data used around the child.
 */
typedef struct _MyQttdChild  MyQttdChild;

/** 
 * @brief Type representing a loop watching a set of files. See \ref myqttd_loop.
 */
typedef struct _MyQttdLoop MyQttdLoop;

/** 
 * @brief Type representing a single domain which a set of
 * configurations for a group of users.
 * 
 * Every domain is a logical group that provides support for a certain
 * list of users, client id, storage and acls applied to them, as well
 * as a set of resource limits and different rules to isolate them
 * from other domains and to control them.
 *
 */
typedef struct _MyQttdDomain MyQttdDomain;

/** 
 * @brief Set of handlers that are supported by modules. This handler
 * descriptors are used by some functions to notify which handlers to
 * call: \ref myqttd_module_notify.
 */
typedef enum {
	/** 
	 * @brief Module reload handler 
	 */
	MYQTTD_RELOAD_HANDLER = 1,
	/** 
	 * @brief Module close handler 
	 */
	MYQTTD_CLOSE_HANDLER  = 2,
	/** 
	 * @brief Module init handler 
	 */
	MYQTTD_INIT_HANDLER   = 3,
} MyQttdModHandler;

#endif

/** 
 * @}
 */
