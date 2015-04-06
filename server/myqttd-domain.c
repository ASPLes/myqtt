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

#include <myqttd-domain.h>
#include <myqttd-ctx-private.h>
#include <myqtt-ctx-private.h>


void myqttd_domain_free (axlPointer _domain)
{
	MyQttdDomain * domain = _domain;

	/* release memory */
 	axl_free (domain->name);
 	axl_free (domain->storage_path);
	axl_free (domain->users_db);

	/* check and stop context if it is started */
	if (domain->initialized)  
		myqtt_exit_ctx (domain->myqtt_ctx, axl_true);

	myqttd_users_free (domain->users);

	/* destroy mutex */
	myqtt_mutex_destroy (&domain->mutex);

	axl_free (domain);

	return;
}

/** 
 * @internal Initialize domain module on the provided server MyQttdCtx
 *
 * @param ctx The context where the operation takes place.
 *
 * @return axl_true in the case the module was initialized, othewise
 * axl_false is returned.
 */
axl_bool   myqttd_domain_init (MyQttdCtx  * ctx)
{
	
	/* init module */
	ctx->domains = myqtt_hash_new_full (axl_hash_string, 
					    axl_hash_equal_string,
					    NULL, /* name is a reference to the domain */
					    myqttd_domain_free);
	return axl_true;
}

/** 
 * @brief Allows to add a new domain with the provide initial data.
 *
 * @param ctx The context where the domain will be added.
 *
 * @param storage_path The storage path used by this domain to hold
 * messages in transit.
 *
 * @param user_db The storage path where user databse configuration is
 * found.
 */
axl_bool   myqttd_domain_add  (MyQttdCtx  * ctx, 
			       const char * name, 
			       const char * storage_path, 
			       const char * user_db,
			       const char * use_settings)
{
	MyQttdDomain * domain;

	/* check input parameters */
	if (ctx == NULL || name == NULL || storage_path == NULL || user_db == NULL)
		return axl_false;

	/* ensure first the is no domain with the same name */
	if (myqtt_hash_exists (ctx->domains, (axlPointer) name)) {
		error ("Unable to add domain %s because it is already registered", name);
		return axl_false; /* unable to add domain because it does exists */
	} /* end if */

	/* it does not exists, add it */
	domain = axl_new (MyQttdDomain, 1);
	if (domain == NULL)
		return axl_false; /* failed to allocate memory for domain, unable to add it */

	/* copy content */
	domain->ctx          = ctx;
	domain->name         = axl_strdup (name);
	domain->storage_path = axl_strdup (storage_path);
	domain->users_db     = axl_strdup (user_db);
	domain->use_settings = use_settings;
	/* reference to the settings configured */
	if (use_settings) {
		domain->settings = myqtt_hash_lookup (ctx->domain_settings, (axlPointer) use_settings);
		if (! domain->settings)
			error ("ERROR: failed to find settings (%s)", use_settings);
	} /* end if */

	myqtt_mutex_create (&domain->mutex);

	/* add it into the domain hashes */
	myqtt_hash_insert (ctx->domains, domain->name, domain);
	
	/* report added */
	return axl_true;
}

/** 
 * @brief Allows to find a domain by name (label configured at
 * configuration file).
 *
 * @param ctx The context where the operation takes place
 *
 * @param name The domain name that is being looked for (<domain name='' />).
 */
MyQttdDomain    * myqttd_domain_find_by_name (MyQttdCtx   * ctx,
					      const char  * name)
{
	if (ctx == NULL || name == NULL)
		return NULL;

	/* report domain found */
	return myqtt_hash_lookup (ctx->domains, (axlPointer) name);
}

/** 
 * @brief Allows to find the domain associated to the given
 * serverName. This server name can be received because SNI indication
 * by TLS or through the Websocket + WebSocket/TLS interface. 
 *
 * @param server_Name The server name that is being requested.
 *
 * @return The domain found or NULL if nothing was found.
 */
MyQttdDomain * myqttd_domain_find_by_serverName (MyQttdCtx * ctx, const char * server_Name)
{
	/* for now, not implemented */
	return NULL;
}

typedef struct __MyQttdDomainFindData {
	MyQttConn    * conn;
	const char   * username;
	const char   * client_id;
	const char   * password;
	MyQttdDomain * result;
} MyQttdDomainFindData;

/** 
 * @internal Function finding to find a domain given a username and a
 * client id.
 */
axl_bool __myqttd_domain_find_by_username_client_id_foreach (axlPointer _name, 
							     axlPointer _domain, 
							     axlPointer user_data)
{
	/* item received */
	MyQttdDomainFindData   * data    = user_data;
	MyQttdDomain           * domain  = _domain;
	const char             * name    = _name;

	/* context */
	MyQttdCtx              * ctx     = domain->ctx;

	/* parameters */
	/* MyQttdDomain ** __domain  = user_data; */
	const char             * username  = data->username;
	const char             * client_id = data->client_id;
	const char             * password  = data->password;
	MyQttConn              * conn      = data->conn;

	msg ("Checking domain: %s, username=%s, client_id=%s", name, myqttd_ensure_str (username), myqttd_ensure_str (client_id));
	/* ensure we have loaded users object for this domain */
	if (domain->users == NULL) {
		myqtt_mutex_lock (&domain->mutex);
		if (domain->users == NULL) {
			/* load users database object from path */
			domain->users = myqttd_users_load (ctx, conn, domain->users_db);
			if (domain->users == NULL) {
				error ("Failed to load database from %s for domain %s", domain->users_db, domain->name);
				myqtt_mutex_unlock (&domain->mutex);
				return axl_true; /* stop searching, we failed to load database */
			} /* end if */
		} /* end if */
		myqtt_mutex_unlock (&domain->mutex);
	} /* end if */

	/* reached this point, domain has a users database backend
	   loaded, now check if this domain recognizes */
	if (myqttd_domain_do_auth (ctx, domain, conn, username, password, client_id)) {
		msg ("Authentication: %s, username=%s, client_id=%s : login ok", 
		     name, myqttd_ensure_str (username), myqttd_ensure_str (client_id));
		/* report domain to the caller */
		data->result = domain;
		return axl_true; /* domain found, report it to the caller */
	} /* end if */
		
	return axl_false; /* keep on searching */
}

/** 
 * @brief Allows to find the domain associated with the username +
 * clien_id or just client_id or username when some of them are NULL.
 *
 * @param ctx The context where where operation is taking place.
 *
 * @param conn The connection where the find operation is taking place.
 *
 * @param username The username to use to select the domain.
 *
 * @param client_id The client id to use to select the domain.
 *
 * @return A reference to the domain found or NULL if it is not found.
 */
MyQttdDomain * myqttd_domain_find_by_username_client_id (MyQttdCtx  * ctx,
							 MyQttConn  * conn,
							 const char * username, 
							 const char * client_id,
							 const char * password)
{
	MyQttdDomain         * domain = NULL;
	MyQttdDomainFindData * data;

	data = axl_new (MyQttdDomainFindData, 1);
	if (data == NULL)
		return NULL;

	/* grab references */
	data->username  = username;
	data->client_id = client_id;
	data->password  = password;
	data->conn      = conn;

	/* for each domain, check username and client id */
	myqtt_hash_foreach (ctx->domains, __myqttd_domain_find_by_username_client_id_foreach, data);

	/* grab reference result and release data */
	domain = data->result;
	axl_free (data);

	return domain;
}

/** 
 * @brief Allows to find the corresponding domain for the given
 * username, client id and serverName.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param conn The connection (optional reference) where the operation is taking place.
 *
 * @param username The user name to lookfor (optional)
 *
 * @param client_id The client id provided by the remote connecting device.
 *
 * @param server_name Optional server name indication to locate the
 * domain.
 *
 * @return The domain reference object or NULL if it failed to find a valid domain.
 * 
 */
MyQttdDomain    * myqttd_domain_find_by_indications (MyQttdCtx  * ctx,
						     MyQttConn  * conn,
						     const char * username,
						     const char * client_id,
						     const char * password,
						     const char * server_Name)
{
	MyQttdDomain * domain;

	/* find the domain by cache serverName */

	/* find the domain by cache username + client_id */

	/* find the domain by the cache client_id */

	/* find the domain by the cache username */

	/* reached this point, the item wasn't found by taking a lookt
	   at the different caches, try to find by the data */
	if (server_Name && strlen (server_Name) > 0) {
		/* get domain by serverName indicated */
		domain = myqttd_domain_find_by_serverName (ctx, server_Name);
		if (domain) 
			return domain;
	} /* end if */

	/* find by username + client id, or just client id or just username */
	domain = myqttd_domain_find_by_username_client_id (ctx, conn, username, client_id, password);
	if (domain)
		return domain;
		
	/* return NULL, no domain was found */
	error ("No domain was found username=%s client-id=%s server-name=%s : no domain was found to handle request",
	       username ? username : "", client_id ? client_id : "", server_Name ? server_Name : "");
	return NULL;
}

/** 
 * @brief Allows to authenticate the provided username, password and
 * client_id on the provided domain.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param domain The domain where the auth operation will take place.
 *
 * @param conn The connection where the authentication operation will
 * take place. This is an optional reference that may not be received.
 *
 * @param username The username to check.
 *
 * @param password The password to check.
 *
 * @param client_id The client id to check.
 *
 * @return axl_true if the authentication was ok, otherwise, axl_false
 * is returned.
 */
axl_bool          myqttd_domain_do_auth (MyQttdCtx    * ctx,
					 MyQttdDomain * domain,
					 MyQttConn    * conn,
					 const char   * username, 
					 const char   * password,
					 const char   * client_id)
{
	/* do a basic check operation for data received */
	if (ctx == NULL || domain == NULL || domain->users == NULL)
		return axl_false;

	/* now for the users database loaded, try to do a complete
	   auth operation */
	if (domain->users->backend->auth (ctx, conn, domain->users->backend_reference, client_id, username, password))
		return axl_true; /* authentication done */

	return axl_false;
}

axl_bool __myqttd_domain_count_foreach (axlPointer _name, axlPointer _domain, axlPointer user_data)
{
	/* item received */
	MyQttdDomain           * domain  = _domain;
	int                    * count   = user_data;

	/* check if the domain is initialized */
	if (domain->initialized)
		(*count) ++;
	return axl_false; /* don't stop searching */
}

/** 
 * @brief Allows to get the number of domains that are enabled right
 * now.
 *
 * @param ctx The context where the operation is taking place.
 *
 * @return Number of domains now enabled.
 */
int               myqttd_domain_count_enabled (MyQttdCtx * ctx)
{
	int count = 0;
	if (ctx == NULL || ctx->domains == NULL)
		return 0; /* no domains enabled */

	myqtt_hash_foreach (ctx->domains, __myqttd_domain_count_foreach, &count);
	return count;
}

/** 
 * @brief Allows to get the amount of connections the provide domain
 * have (users connected at this moment).
 *
 * @param domain The domain that is being checked for number of connections.
 *
 * @return Number of connections.
 */
int               myqttd_domain_conn_count (MyQttdDomain * domain)
{
	if (domain == NULL)
		return 0;
	if (! domain->initialized)
		return 0;

	/* list of connections currently watched */
	return axl_list_length (domain->myqtt_ctx->conn_list);
}

/** 
 * @internal Cleanups the domain module for this context.
 */
void myqttd_domain_cleanup (MyQttdCtx * ctx)
{
	MyQttHash * hash = ctx->domains;

	ctx->domains = NULL;
	/* release domains */
	myqtt_hash_destroy (hash);


	return;
}

