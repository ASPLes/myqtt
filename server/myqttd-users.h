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

#ifndef __MYQTTD_USERS_H__
#define __MYQTTD_USERS_H__

#include <myqttd.h>

MyQttdUsers * myqttd_users_load (MyQttdCtx  * ctx, 
				 MyQttConn  * conn,
				 const char * path);

axl_bool      myqttd_users_do_auth (MyQttdCtx    * ctx,
				    MyQttdUsers  * users,
				    MyQttConn    * conn,
				    const char   * username, 
				    const char   * password,
				    const char   * client_id);

axl_bool      myqttd_users_register_backend (MyQttdCtx          * ctx,
					     const char         * backend_type,
					     MyQttdUsersLoadDb    loadBackend,
					     MyQttdUsersExists    userExists,
					     MyQttdUsersAuthUser  authUser,
					     MyQttdUsersUnloadDb  unloadBackend,
					     axlPointer           extensionPtr,
					     axlPointer           extensionPtr2,
					     axlPointer           extensionPtr3,
					     axlPointer           extensionPtr4);

void          myqttd_users_free (MyQttdUsers * users);
					     

#endif
