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
#include <myqttd.h>
#include <myqttd-ctx-private.h>

typedef struct _MyQttdUserLoadData {
	MyQttdCtx  * ctx;
	MyQttConn  * conn;
	const char * path;

	/* output variables */
	const char          * backend_type;
	axlPointer            backend_reference;
	MyQttdUsersBackend  * backend;
} MyQttdUserLoadData;


axl_bool __myqttd_users_load (axlPointer _key,
			      axlPointer _data,
			      axlPointer user_data)
{
	/* parameters received */
	MyQttdUserLoadData * data         = user_data;
	MyQttdCtx          * ctx          = data->ctx;

	/* reference to the backend type and the backend itself that is being checked */
	const char         * backend_type = _key;
	MyQttdUsersBackend * backend      = _data;
	
	/* try to load backend database */
	data->backend_reference = backend->load (data->ctx, data->conn, data->path);
	if (data->backend_reference) {
		/* backend loaded, report log */
		msg ("Found users database at %s:%s", backend_type, data->path);
		data->backend_type = backend_type;
		data->backend      = backend;
		return axl_true; /* report to stop searching */
	} /* end if */

	return axl_false; /* keep on searching */
}

/** 
 * @brief Allows to load a MyQttdUsers database reference from the
 * provided path.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param path 
 */
MyQttdUsers * myqttd_users_load (MyQttdCtx  * ctx, 
				 MyQttConn  * conn,
				 const char * path)
{
	
	MyQttdUsers        * users;
	MyQttdUserLoadData * data;

	if (ctx == NULL || conn == NULL || path == NULL)
		return NULL;

	/* reference to the data used during the load operation */
	data = axl_new (MyQttdUserLoadData, 1);
	if (data == NULL)
		return NULL;

	/* set values on data to be used by the foreach handler */
	data->conn = conn;
	data->ctx  = ctx;
	data->path = path;

	/* iterate and find a backend loading the path */
	msg ("Attempting to load users backend at %s (registered backends %d)",
	     path, myqtt_hash_size (ctx->auth_backends));
	myqtt_hash_foreach (ctx->auth_backends, __myqttd_users_load, data);

	if (data->backend == NULL || data->backend_type == NULL) {
		/* no backend was reported after foreach all registered handlers */
		axl_free (data);
		return NULL;
	} /* end if */

	/* build container for the result */
	users = axl_new (MyQttdUsers, 1);
	if (users == NULL) {
		axl_free (data);
		return NULL; 
	} /* end if */

	/* report backend reference and type */
	users->ctx               = ctx;
	users->backend           = data->backend;
	users->backend_type      = data->backend_type;
	users->backend_reference = data->backend_reference;
	users->unload            = data->backend->unload;

	/* release local data and report database */
	axl_free (data);
	return users;
}

/** 
 * @brief Allows to do an auth operation on the provided users backend
 * (users) in the context of the provided connection (optionally
 * provided) with credentials provided.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param users The users' backend where auth operation is requested.
 *
 * @param conn The connection where the auth operation is taking
 * place. This conection reference may not be provided (for example an
 * offline auth operation).
 *
 * @param username The username to check on the auth operation.
 *
 * @param password The password to check on the auth operation.
 *
 * @param client_id The client_id to check on the auth operation.
 *
 * @return axl_true if the auth operation finishes successfully,
 * otherwise, axl_false is returned.
 */
axl_bool      myqttd_users_do_auth (MyQttdCtx    * ctx,
				    MyQttdUsers  * users,
				    MyQttConn    * conn,
				    const char   * username, 
				    const char   * password,
				    const char   * client_id)
{
	if (users == NULL || ctx == NULL)
		return axl_false; /* minimum parameters not received */

	if (users->backend->auth (ctx, conn, users->backend_reference, client_id, username, password)) {
		return axl_true; /* auth operation ok */
	} /* end if */

	return axl_false; /* auth operation failed */
}

void __myqttd_users_release_backend_node (axlPointer _backend)
{
	MyQttdUsersBackend * backend = _backend;
	
	axl_free (backend->backend_type);
	axl_free (backend);
	return;
}

/** 
 * @brief Allows to register a new authentication backend. A backend
 * is a set of functions tha allows to load a backend and implement
 * different operations like exists or auth user so all these
 * operations can be delegated to a third party module. 
 *
 *
 * @param ctx The context where the operation will take place.
 *
 * @param backend_type A simple label to identify the backend registered.
 *
 * @param loadBackend The handler that should be used to load a
 * particular instance of this backend.
 *
 * @param userExists The handler that should be used to check for if a
 * user exists in the provided backend.
 *
 * @param authUser The handler that should be used to auth a
 * particular user against that backend.
 *
 * @param unloadBackend The handler that should be used to unload and
 * release the provided backend.
 *
 * @param extensionPtr Without use for now.
 *
 * @param extensionPtr2 Without use for now
 *
 * @param extensionPtr3 Without use for now
 *
 * @param extensionPtr4 Without use for now
 */
axl_bool      myqttd_users_register_backend (MyQttdCtx          * ctx,
					     const char         * backend_type,
					     MyQttdUsersLoadDb    loadBackend,
					     MyQttdUsersExists    userExists,
					     MyQttdUsersAuthUser  authUser,
					     MyQttdUsersUnloadDb  unloadBackend,
					     axlPointer           extensionPtr,
					     axlPointer           extensionPtr2,
					     axlPointer           extensionPtr3,
					     axlPointer           extensionPtr4)
{
	MyQttdUsersBackend * backend;

	/* check input parameters */
	if (backend_type == NULL || strlen (backend_type) == 0) 
		return axl_false;

	if (loadBackend   == NULL || 
	    userExists    == NULL || 
	    authUser      == NULL ||
	    unloadBackend == NULL)
		return axl_false;

	/* register backend */
	backend = axl_new (MyQttdUsersBackend, 1);
	if (backend == NULL)
		return axl_false;

	/* save references */
	backend->backend_type   = axl_strdup (backend_type);
	if (backend->backend_type == NULL) {
		axl_free (backend->backend_type);
		return axl_false;
	} /* end if */

	backend->load       = loadBackend;
	backend->exists     = userExists;
	backend->auth       = authUser;
	backend->unload     = unloadBackend;

	/* record it into the hash */
	msg ("Registering auth backend: %s", backend_type);
	myqtt_hash_replace_full (ctx->auth_backends,
				 backend->backend_type, NULL,
				 backend, __myqttd_users_release_backend_node);
	
	return axl_true;
}

/** 
 * @brief Allows to release resources used by the provided database of
 * users.
 *
 * @param users The database to release.
 */
void          myqttd_users_free (MyQttdUsers * users) {

	if (users == NULL)
		return;

	/* call to unload */
	if (users->backend_reference) 
		users->unload (users->ctx, users->backend_reference);

	axl_free (users);

	return;
}

