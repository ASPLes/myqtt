/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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

#include <myqttd.h>

/* mysql flags */
#include <mysql.h>


/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

MyQttdCtx * ctx = NULL;

/** Implementation for MyQttdUsersLoadDb **/
axlPointer __mod_auth_mysql_load (MyQttdCtx  * ctx, 
				  MyQttConn  * conn, 
				  const char * path)
{
	return NULL;
}

/** Implemenation for MyQttdUsersExists **/
axl_bool     __mod_auth_mysql_user_exists (MyQttdCtx  * ctx, 
					 MyQttConn  * conn, 
					 axlPointer   _backend,
					 const char * client_id, 
					 const char * user_name)
{
	/* user does not exists for this backend */
	return axl_false;
}

/** Implementation for MyQttdUsersAuthUser **/
axl_bool     __mod_auth_mysql_auth_user (MyQttdCtx  * ctx, 
				       MyQttConn  * conn,
				       axlPointer   _backend,
				       const char * client_id, 
				       const char * user_name,
				       const char * password)
{
	/* user does not exists for this backend */
	return axl_false;
}

/** MyQttdUsersUnloadDb **/
void __mod_auth_mysql_unload (MyQttdCtx * ctx, 
			    axlPointer  _backend)
{

	return;
}

/** 
 * @brief Init function, perform all the necessary code to register
 * handlers, configure MyQtt, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  mod_auth_mysql_init (MyQttdCtx * _ctx)
{
	/* configure the module */
	MYQTTD_MOD_PREPARE (_ctx);

	/* install handlers to implement auth based on a simple xml
	   backend */
	if (! myqttd_users_register_backend (ctx, 
					     "mod-auth-xml",
					     __mod_auth_mysql_load,
					     __mod_auth_mysql_user_exists,
					     __mod_auth_mysql_auth_user,
					     __mod_auth_mysql_unload,
					     /* for future usage */
					     NULL, NULL, NULL, NULL)) {
		error ("Failed to install mod-auth-xml authentication handlers..");
		return axl_false;
	} /* end if */

	/* register on subscribe */
	/* myqttd_ctx_add_on_subscribe (ctx, __mod_auth_mysql_on_subscribe, NULL); */
	
	
	return axl_true;
}

/** 
 * @brief Close function called once the myqttd server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void mod_auth_mysql_close (MyQttdCtx * ctx)
{
	/* for now nothing */
	return;
}

/** 
 * @brief The reconf function is used by myqttd to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
static void mod_auth_mysql_reconf (MyQttdCtx * ctx) {
	/* for now nothing */
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the myqttd will lookup to load the rest of items.
 */
MyQttdModDef module_def = {
	"mod-auth-mysql",
	"Authentication backend support for auth-mysql",
	mod_auth_mysql_init,
	mod_auth_mysql_close,
	mod_auth_mysql_reconf,
	NULL,
};

END_C_DECLS


/** 
 * \page myqttd_mod_auth_mysql mod-auth-mysql Authentication and Acls plugin supported on MySQL server for MyQttd broker
 *
 * 
 * 
 *
 */
