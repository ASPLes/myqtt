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
	axl_free (domain->use_settings);

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
 * @brief Allows to add/update a domain with the provide initial data.
 *
 * @param ctx The context where the domain will be added.
 *
 * @param storage_path The storage path used by this domain to hold
 * messages in transit.
 *
 * @param user_db The storage path where user databse configuration is
 * found.
 *
 * @param is_active Flags the domain as activate (usable).
 *
 * @return axl_true when the domain was added or updated, otherwise axl_false is returned.
 */
axl_bool   myqttd_domain_add  (MyQttdCtx  * ctx, 
			       const char * name, 
			       const char * storage_path, 
			       const char * user_db,
			       const char * use_settings,
			       axl_bool     is_active)
{
	MyQttdDomain * domain;

	/* check input parameters */
	if (ctx == NULL || name == NULL || storage_path == NULL || user_db == NULL)
		return axl_false;

	/* ensure first the is no domain with the same name */
	if (myqtt_hash_exists (ctx->domains, (axlPointer) name)) {
		if (! ctx->started) {
			error ("Unable to add domain %s because it is already registered", name);
			return axl_false; /* unable to add domain because it does exists */
		} /* end if */
	} /* end if */

	/* reached this point, ctx->started == axl_true, so we are reloading */
	domain = myqtt_hash_lookup (ctx->domains, (axlPointer) name);
	if (! domain) {
	
		/* it does not exists, add it */
		domain = axl_new (MyQttdDomain, 1);
		if (domain == NULL)
			return axl_false; /* failed to allocate memory for domain, unable to add it */

		/* init mutex */
		myqtt_mutex_create (&domain->mutex);

		/* copy content */
		domain->ctx          = ctx;

		/* setup initial name */
		domain->name         = axl_strdup (name);

		/* add it into the domain hashes */
		myqtt_hash_insert (ctx->domains, domain->name, domain);
		
	} /* end if */

	/* storage path */
	if (domain->storage_path)
		axl_free (domain->storage_path);
	domain->storage_path = axl_strdup (storage_path);

	/* users db */
	if (domain->users_db)
		axl_free (domain->users_db);
	domain->users_db     = axl_strdup (user_db);

	/* setup new domain settings */
	if (domain->use_settings)
		axl_free (domain->use_settings);
	domain->use_settings = axl_strdup (use_settings);

	/* configure active */
	domain->is_active    = is_active;
	
	/* reference to the settings configured */
	if (use_settings) {
		domain->settings = myqtt_hash_lookup (ctx->domain_settings, (axlPointer) use_settings);
		if (! domain->settings)
			error ("ERROR: failed to find settings (%s)", use_settings);
	} /* end if */

	/* report added */
	return axl_true;
}

/** 
 * @brief Allows to find an active domain by name (label configured at
 * configuration file).
 *
 * @param ctx The context where the operation takes place
 *
 * @param name The domain name that is being looked for (<domain name='' />).
 *
 * @return A reference to the domain object (\ref MyQttdDomain) or
 * NULL if it is not found or it is not active.
 */
MyQttdDomain    * myqttd_domain_find_by_name (MyQttdCtx   * ctx,
					      const char  * name)
{
	MyQttdDomain * domain;
	
	if (ctx == NULL || name == NULL)
		return NULL;

	/* report domain found */
	domain =  myqtt_hash_lookup (ctx->domains, (axlPointer) name);
	if (domain && ! domain->is_active)
		return NULL; /* domain is disabled, so report NULL */

	/* report reference found */
	return domain;
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
	return myqttd_domain_find_by_name (ctx, server_Name);
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

	/* context */
	MyQttdCtx              * ctx     = domain->ctx;

	/* parameters */
	/* MyQttdDomain ** __domain  = user_data; */
	const char             * username  = data->username;
	const char             * client_id = data->client_id;
	const char             * password  = data->password;
	MyQttConn              * conn      = data->conn;

	/* check if the domain is active */
	if (domain && ! domain->is_active)
		return axl_false; /* keep on searching */

	/* Reached this point, domain has a users database backend
	   loaded, now check if this domain recognizes.  Configure
	   domain_selected = axl_false becaus this domain was not
	   selected previously but we are attempting to auth this user
	   against this domain. */
	if (myqttd_domain_do_auth (ctx, domain, /* domain_selected */ axl_false,
				   conn, username, password, client_id)) {
		/* report domain to the caller */
		data->result = domain;
		return axl_true; /* domain found, report it to the caller */
	} /* end if */
		
	return axl_false; /* keep on searching */
}

/** 
 * @brief Allows to find the domain associated with the username +
 * clien_id or just client_id or username when some of them are
 * NULL. 
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

MyQttdDomain * __myqttd_domain_auth_and_report (MyQttdCtx * ctx, MyQttdDomain * domain, axl_bool domain_selected,
						MyQttConn * conn, const char * serverName, const char * username, const char * password, const char * client_id)
{
	/* Domain found, implement authentication to ensure this
	 * domain is accesible by the connecting user providing the
	 * username+client_id+password credentials */
	
	/* domain found by client id or username, try to authenticate */
	if (myqttd_domain_do_auth (ctx, domain, axl_true, conn, username, password, client_id))
		return domain; /* domain found, report it to the caller */

	/* domain found but authentication failed */
	error ("Domain was found (%s) for authentication failed username=%s client-id=%s server-name=%s : domain found but authentication failed",
	       domain->name, username ? username : "", client_id ? client_id : "", serverName ? serverName : "");
	return NULL;
} 

/** 
 * @internal The following function implements client-id@<server-name>
 * detection: that is, allowing a remote MQTT client to connect using
 * clientid or username +  @<server-name> to indicate MyQtt engine which serverName has to be used.
 *
 * In the case @ is not detected in the auth_value (which can be
 * clientid or username), the function just report NULL (no domain can
 * be found because no @<server-name> indication was given).
 */
MyQttdDomain * myqttd_domain_find_by_auth_serverName (MyQttdCtx * ctx, const char * auth_value)
{
	int          iterator;
	const char * serverName;
	
	if (auth_value == NULL)
		return NULL;
	if (strstr (auth_value, "@") == NULL)
		return NULL;

	iterator = 0;
	while (auth_value[iterator]) {
		if (auth_value[iterator] == '@') {
			/* get reference to the server name */
			serverName = auth_value + iterator + 1;

			msg ("Found serverName=%s inside auth-value=%s", serverName, auth_value);

			/* try to find server name by name */
			return myqttd_domain_find_by_name (ctx, serverName);
		} /* end if */
		
		/* next position */
		iterator++;
	} /* end while */
	
	return NULL; /* no domain with the provided server name
		      * found */
}

/** 
 * @internal Gets the auth value removing the @<server-name> part (if any).
 */
char * __myqttd_domain_auth_strip_serverName (MyQttdCtx * ctx, const char * auth_value)
{
	int iterator;
	char * result;
	
	if (auth_value == NULL)
		return NULL;
	if (strstr (auth_value, "@") == NULL)
		return axl_strdup (auth_value);

	iterator = 0;
	while (auth_value[iterator]) {
		if (auth_value[iterator] == '@') {
			result = axl_strdup (auth_value);
			result[iterator] = 0;
			return result; /* report auth value updated */
		} /* end if */
		
		/* next position */
		iterator++;
	} /* end while */
	
	return axl_strdup (auth_value); /* never reached */
	
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
						     const char * serverName)
{
	MyQttdDomain * domain;
	char         * aux_client_id;
	char         * aux_username;
	

	/* find the domain by cache serverName */

	/* find the domain by cache username + client_id */

	/* find the domain by the cache client_id */

	/* find the domain by the cache username */

	/* reached this point, the item wasn't found by taking a lookt
	   at the different caches, try to find by the data */

	/* by server name indication inside client_id and username: client_id@<server-name> or username@<server-name> */
	domain = myqttd_domain_find_by_auth_serverName (ctx, client_id);
	if (! domain) {
		/* if it fails, try with username */
		domain = myqttd_domain_find_by_auth_serverName (ctx, username);
	} /* end if */
	
	if (domain) {

		/* prepare username and client id */
		aux_client_id = __myqttd_domain_auth_strip_serverName (ctx, client_id);
		aux_username  = __myqttd_domain_auth_strip_serverName (ctx, username);
		
		/* domain found by serverName indication, try to do a final (domain_selected=axl_true) authentication */
		domain        = __myqttd_domain_auth_and_report (ctx, domain, axl_true, conn, serverName,
								 aux_username, password,
								 aux_client_id);
		
		/* release prepared values */
		axl_free (aux_client_id);
		axl_free (aux_username);
		return domain;
	} /* end if */
	

	/* by server name */
	if (serverName && strlen (serverName) > 0) {

		msg ("Finding domain by serverName=%s", serverName ? serverName : "");

		/* get domain by serverName indicated */
		domain = myqttd_domain_find_by_serverName (ctx, serverName);
		if (domain) {
			/* domain found by serverName indication, try to do a final (domain_selected=axl_true) authentication */
			return __myqttd_domain_auth_and_report (ctx, domain, axl_true, conn, serverName, username, password, client_id);
		} /* end if */

	} /* end if */

	/* find by username + client id, or just client id or just username */
	domain = myqttd_domain_find_by_username_client_id (ctx, conn, username, client_id, password);
	if (domain)
		return domain;

	/* return NULL, no domain was found */
	error ("No domain was found username=%s client-id=%s server-name=%s : no domain was found to handle request",
	       username ? username : "", client_id ? client_id : "", serverName ? serverName : "");
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
 * @param domain_selected External indication to signal auth backend
 * that the domain was already selected or not. Once a domain is
 * <b>selected</b>, it is assumed that any auth operation in this
 * domain with the given username/client_id/password will be final: no
 * other domain will be checked. This is important in the sense that
 * MyQttd engine try to find the domain by various methods, and one of
 * them is by checking auth over all domains. However, because in that
 * context the search for domain is done by using auth operations,
 * then the auth backend may want to disable certain mechanism (like anonymous login) in the case <b>domain_selected == axl_false</b>.
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
					 axl_bool       domain_selected,
					 MyQttConn    * conn,
					 const char   * username, 
					 const char   * password,
					 const char   * client_id)
{
	/* do a basic check operation for data received */
	if (ctx == NULL || domain == NULL)
		return axl_false;
	
	/* avoid checking template or skeleton domains */
	if (strstr (domain->name, "dont-use-please"))
	        return axl_false; 

	msg ("Checking domain: %s, username=%s, client_id=%s", domain->name, myqttd_ensure_str (username), myqttd_ensure_str (client_id));
	/* ensure we have loaded users object for this domain */
	if (domain->users == NULL) {
		myqtt_mutex_lock (&domain->mutex);
		if (domain->users == NULL) {
			/* load users database object from path */
			domain->users = myqttd_users_load (ctx, domain, conn, domain->users_db);
			if (domain->users == NULL) {
				error ("Failed to load database from %s for domain %s", domain->users_db, domain->name);
				myqtt_mutex_unlock (&domain->mutex);

				/* report axl_false to indicate login failure for this domain backend */
				return axl_false; /* stop searching, we failed to load database */
			} /* end if */
		} /* end if */
		myqtt_mutex_unlock (&domain->mutex);
	} /* end if */

	/* now for the users database loaded, try to do a complete
	   auth operation */
	if (myqttd_users_do_auth (ctx, domain, domain_selected, domain->users, conn, username, password, client_id)) {
		msg ("Authentication: %s, username=%s, client_id=%s : login ok", 
		     domain->name, myqttd_ensure_str (username), myqttd_ensure_str (client_id));
		return axl_true; /* authentication done */
	} /* end if */

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
 * @return Number of connections (without including listener
 * connections). If you want to return count for all connections
 * (accepted and listeners) use \ref myqttd_domain_conn_count_all
 */
int               myqttd_domain_conn_count (MyQttdDomain * domain)
{
	if (domain == NULL)
		return 0;
	if (! domain->initialized)
		return 0;

	/* list of connections currently watched plus the amount of
	 * connections are about to be watched because they are
	 * waiting in the readers queue to be accepted. */
	return axl_list_length (domain->myqtt_ctx->conn_list) + myqtt_async_queue_items (domain->myqtt_ctx->reader_queue);
}

/** 
 * @brief Allows to get the amount of connections the provide domain
 * have (users connected at this moment) plus listener connections.
 *
 * @param domain The domain that is being checked for number of connections.
 *
 * @return Number of connections  including listener
 * connections.
 */
int               myqttd_domain_conn_count_all (MyQttdDomain * domain)
{
	if (domain == NULL)
		return 0;
	if (! domain->initialized)
		return 0;
	if (! domain->myqtt_ctx)
		return 0;

	/* list of connections currently watched */
	return myqttd_domain_conn_count (domain) + axl_list_length (domain->myqtt_ctx->srv_list);
}

/** 
 * @brief Allows to get internal domain name attribute. 
 *
 * @param domain where the operatation takes place.
 *
 * @return A reference to the domain name or NULL if it fails.
 */
const char      * myqttd_domain_get_name (MyQttdDomain * domain)
{
	if (domain == NULL)
		return NULL;
	return domain->name;
}

/** 
 * @brief Allows to get current month message quota currently
 * configured for the given domain, according to its settings.
 *
 * @param domain A reference to the domain where the operation takes place.
 *
 * @return Current cuota or -1 if no cuota was configured. The
 * function returns 0 if NULL domain reference is received.
 */ 
long              myqttd_domain_get_month_message_quota (MyQttdDomain * domain)
{
	/* check for null reference */
	if (domain == NULL)
		return 0;

	/* check input values and report currently configured value */
	if (domain && domain->settings)
		return domain->settings->month_message_quota;
	
	return -1;
}
	
/** 
 * @brief Allows to get current day message quota currently
 * configured for the given domain, according to its settings.
 *
 * @param domain A reference to the domain where the operation takes place.
 *
 * @return Current cuota or -1 if no cuota was configured. The
 * function returns 0 if NULL domain reference is received.
 */ 
long              myqttd_domain_get_day_message_quota (MyQttdDomain * domain)
{
	/* check for null reference */
	if (domain == NULL)
		return 0;

	/* check input values and report currently configured value */
	if (domain && domain->settings)
		return domain->settings->day_message_quota;
	
	return -1;
}

/** 
 * @brief Allows to get currently loaded users backend for the
 * provided domain (or NULL if nothing is still loaded).
 *
 * @param domain The domain where the operation is going to happen.
 *
 * @return A reference to the MyQttdUsers object that represents the user's backend.
 */
MyQttdUsers     * myqttd_domain_get_users_backend (MyQttdDomain * domain)
{
	/* report domain users backend */
	return domain->users;
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

