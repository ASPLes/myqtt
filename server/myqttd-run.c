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
#include <myqtt-ctx-private.h>
#include <myqtt-conn-private.h>

void __myqttd_run_log_handler (const char       * file,
			       int                line,
			       MyQttDebugLevel   log_level,
			       const char       * message,
			       va_list            args,
			       axlPointer         user_data)
{
	MyQttdCtx * ctx = user_data;
	if (log_level != MYQTT_LEVEL_CRITICAL)
		return;
	error ("(myqtt) %s:%d: %s", file, line, message); 
	return;
}

/** 
 * \defgroup myqttd_run MyQttd runtime: runtime checkings 
 */

/** 
 * \addtogroup myqttd_run
 * @{
 */

/** 
 * @internal Function used to check a module name or a base module
 * name to be not loaded by a no-load directive.
 */
axl_bool myqttd_run_check_no_load_module (MyQttdCtx * ctx, 
					      const char    * module_to_check)
{
	axlDoc     * doc = myqttd_config_get (ctx);
	axlNode    * node;
	int          length;
	const char * module_name;

	node = axl_doc_get (doc, "/myqtt/modules/no-load/module");
	while (node != NULL) {

		/* check module name without extension */
		length      = 0;
		while (module_to_check[length] && module_to_check[length] != '.')
			length++;

		/* check length */
		module_name = ATTR_VALUE (node, "name");
		if (length == strlen (module_name))  {
			
			/* check restriction without name */
			msg ("checking %s with %s", module_name, module_to_check);
			if (axl_memcmp (module_name, module_to_check, length))
				return axl_false;
		} /* end if */
		
		/* get next module */
		node = axl_node_get_next_called (node, "module");
	} /* end while */

	/* no restriction found */
	return axl_true;
}

/** 
 * @brief Tries to load all modules found at the directory already
 * located. In fact the function searches for xml files that points to
 * modules to be loaded.
 * 
 * @param ctx MyQttd context.
 *
 * @param path The path that was used to open the dirHandle.
 *
 * @param dirHandle The directory that will be inspected for modules.
 *
 */
void myqttd_run_load_modules_from_path (MyQttdCtx * ctx, const char * path, DIR * dirHandle)
{
	struct dirent    * entry;
	char             * fullpath = NULL;
	const char       * location;
	axlDoc           * doc;
	axlError         * error;
	char             * temp;
				      


	/* get the first entry */
	entry = readdir (dirHandle);
	while (entry != NULL) {
		/* nullify full path and doc */
		fullpath = NULL;
		doc      = NULL;
		error    = NULL;

		/* check for non valid directories */
		if (axl_cmp (entry->d_name, ".") ||
		    axl_cmp (entry->d_name, "..")) {
			goto next;
		} /* end if */
		
		fullpath = myqtt_support_build_filename (path, entry->d_name, NULL);
		if (! myqtt_support_file_test (fullpath, FILE_IS_REGULAR))
			goto next;

		/* check if the fullpath is ended with .xml */
		if (strlen (fullpath) < 5 || ! axl_cmp (fullpath + (strlen (fullpath) - 4), ".xml")) {
			msg2 ("skiping file %s which do not end with .xml", fullpath);
			goto next;
		} /* end if */

		/* notify file found */
		msg ("possible module pointer found: %s", fullpath);		
		
		/* check its xml format */
		doc = axl_doc_parse_from_file (fullpath, &error);
		if (doc == NULL) {
			wrn ("file %s does not appear to be a valid xml file (%s), skipping..", fullpath, axl_error_get (error));
			goto next;
		}
		
		msg ("Attempting to load mod myqttd pointer: %s", fullpath);

		/* check module basename to be not loaded */
		location = ATTR_VALUE (axl_doc_get_root (doc), "location");
		temp     = myqttd_file_name (location);
		if (! myqttd_run_check_no_load_module (ctx, temp)) {
			axl_free (temp);
			wrn ("module %s skipped by location", location);
			goto next;
		}
		axl_free (temp);

		/* load the module man!!! */
		myqttd_module_open_and_register (ctx, location);

	next:
		/* free the document */
		axl_doc_free (doc);
		
		/* free the error */
		axl_error_free (error);

		/* free full path */
		axl_free (fullpath);

		/* get the next entry */
		entry = readdir (dirHandle);
	} /* end while */

	return;
}

/** 
 * @internal Loads all paths from the configuration, calling to load all
 * modules inside those paths.
 * 
 * @param doc The myqttd run time configuration.
 */
void myqttd_run_load_modules (MyQttdCtx * ctx, axlDoc * doc)
{
	axlNode     * directory;
	const char  * path;
	DIR         * dirHandle;

	directory = axl_doc_get (doc, "/myqtt/modules/directory");
	if (directory == NULL) {
		msg ("no module directories were configured, nothing loaded");
		return;
	}

	/* check every module */
	while (directory != NULL) {
		/* get the directory */
		path = ATTR_VALUE (directory, "src");
		
		/* try to open the directory */
		if (myqtt_support_file_test (path, FILE_IS_DIR | FILE_EXISTS)) {
			dirHandle = opendir (path);
			if (dirHandle == NULL) {
				wrn ("unable to open mod directory '%s' (%s), skiping to the next", strerror (errno), path);
				goto next;
			} /* end if */
		} else {
			wrn ("skiping mod directory: %s (not a directory or do not exists)", path);
			goto next;
		}

		/* directory found, now search for modules activated */
		msg ("found mod directory: %s", path);
		myqttd_run_load_modules_from_path (ctx, path, dirHandle);
		
		/* close the directory handle */
		closedir (dirHandle);
	next:
		/* get the next directory */
		directory = axl_node_get_next_called (directory, "directory");
		
	} /* end while */

	return;
}

MyQttConn * __myqttd_run_start_mqtt_listener (MyQttdCtx * ctx, MyQttCtx * my_ctx, axlNode * port_node, 
					      const char * bind_addr, const char * port, axlPointer user_data)
{
	return myqtt_listener_new (
		 /* the context where the listener will
		  * be started */
		  my_ctx,
		  /* listener name */
		  bind_addr ? bind_addr : "0.0.0.0",
		  /* port to use */
		  port,
		  /* opts */
		  NULL,
		  /* on ready callbacks */
		  NULL, NULL);
}


axl_bool myqttd_run_config_start_listeners (MyQttdCtx * ctx, axlDoc * doc)
{
	axl_bool           at_least_one_listener = axl_false;
	axlNode          * port;
	MyQttConn        * conn_listener;
	MyQttCtx         * myqtt_ctx = myqttd_ctx_get_myqtt_ctx (ctx);
	const char       * proto;
	const char       * bind_addr;
	int                port_val;

	/* refrence to the listener activator */
	MyQttdListenerActivatorData * activator;

	/* check if this is a child process (it has no listeners, only
	 * master process do) */
	if (ctx->child)
		return axl_true;

	/* add default listener activator */
	myqttd_ctx_add_listener_activator (ctx, "mqtt", __myqttd_run_start_mqtt_listener, NULL);

	/* get ports to be allocated */
	port = axl_doc_get (doc, "/myqtt/global-settings/ports/port");
	while (port != NULL) {

		/* get bind addr */
		bind_addr = ATTR_VALUE (port, "bind-addr");
		/* check protocol that must be running in the declared port */
		proto     = ATTR_VALUE (port, "proto");
		port_val  = myqtt_support_strtod (axl_node_get_content (port, NULL), NULL);

		if (proto == NULL) {
			/* No 'proto' declaration, try to figure out which protocol we should run here. 
			   For now, declare protocol as default */
			proto = "mqtt";
			if (port_val == 1883)
				proto = "mqtt"; /* this is not needed, but for completeness */
			else if (port_val == 8883)
				proto = "mqtt-tls";
		} /* end if */

		/* set default value */
		if (bind_addr == NULL)
			bind_addr = "0.0.0.0";
		
		/* check if proto is defined in the list of
		   listener activators */
		activator = myqtt_hash_lookup (ctx->listener_activators, (axlPointer) proto);
		if (activator == NULL) {
			error ("No listener activator was found for proto %s, skipping starting listener at %s:%d",
			       proto, bind_addr ? bind_addr : "", port_val);
			goto next;
		} /* end if */

		/* call to activate listener */
		conn_listener = activator->listener_activator (ctx, myqtt_ctx, port, bind_addr, axl_node_get_content (port, NULL), activator->user_data);
		
		/* check the listener started */
		if (! myqtt_conn_is_ok (conn_listener, axl_false)) {
			/* unable to start the server configuration */
			error ("unable to start listener at %s:%s...", 
			       /* server name */
			       bind_addr,
			       /* server port */
			       axl_node_get_content (port, NULL));
			
			goto next;
		} /* end if */
		
		msg ("started listener at %s:%s (id: %d, socket: %d, proto: %s)...",
		     bind_addr ? bind_addr : "0.0.0.0",
		     axl_node_get_content (port, NULL),
		     myqtt_conn_get_id (conn_listener), myqtt_conn_get_socket (conn_listener), proto);
		
		/* flag that at least one listener was
		 * created */
		at_least_one_listener = axl_true;
	next:
		/* get the next port */
		port = axl_node_get_next_called (port, "port");
		
	} /* end while */
	
	if (! at_least_one_listener) {
		error ("Unable to start myqttd, no listener configuration was started, either due to configuration error or to startup problems. Terminating..");
		return axl_false;
	} /* end if */

	/* listeners ok */
	return axl_true;
}

void myqtt_run_load_domain (MyQttdCtx * ctx, axlDoc * doc, axlNode * node)
{
	/* get settings for this domain: see MyQttdDomain definition
	 * at myqttd-types.h and myqttd-ctx-private.h */
	const char * name         = ATTR_VALUE (node, "name");
	const char * storage      = ATTR_VALUE (node, "storage");
	const char * users_db     = ATTR_VALUE (node, "users-db");
	const char * use_settings = ATTR_VALUE (node, "use-settings");

	/* do some logging */
	msg ("Loading domain, name=%s, storage=%s, users-db=%s, use-settings=%s", 
	     name ? name : "", storage ? storage : "", users_db ? users_db : "", use_settings ? use_settings : "");

	/* check definition name to be present */
	myqttd_config_ensure_attr (ctx, node, "name");
	myqttd_config_ensure_attr (ctx, node, "storage");
	myqttd_config_ensure_attr (ctx, node, "users-db");

	/* now ensure storage is present and initialize it */
	if (! myqttd_domain_add (ctx, name, storage, users_db, use_settings)) {
		/* report failure */
		error ("Unable to add domain name='%s'", name);

		/* fail here because clean start is enabled? */
	} /* end if */

	return;
}

axl_bool myqttd_run_domains_load (MyQttdCtx * ctx, axlDoc * doc)
{
	axlNode * node;
	node = axl_doc_get (doc, "/myqtt/myqtt-domains/domain");
	if (node == NULL) {
		abort_error ("Unable to find any domain declaration inside %s file, under the xml section /myqtt/myqtt-domains/domain. Without a single domain declaration this server cannot accept connections");
		return axl_false;
	} /* end if */
	
	while (node) {
		/* record this domain into domains table for future
		 * usage and prepare configuration */
		myqtt_run_load_domain (ctx, doc, node);
		
		/* next next domain node */
		node = axl_node_get_next_called (node, "domain");
	} /* end if while */
	
	/* reached this point, everything was ok */
	return axl_true;
}

/** 
 * @internal Function to handle on publish messages at MyQttd (server)
 * level.
 */
MyQttPublishCodes __myqttd_run_on_publish_msg (MyQttCtx * myqtt_ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer _domain)
{
	MyQttdDomain        * domain   = _domain;
	MyQttdCtx           * ctx      = domain->ctx;
	int                   iterator = 0;
	MyQttdOnPublishData * data;
	MyQttPublishCodes     code;

	while (iterator < axl_list_length (ctx->on_publish_handlers)) {
		/* next data */
		data = axl_list_get_nth (ctx->on_publish_handlers, iterator);
		if (data != NULL && data->on_publish != NULL) {
			/* get the code reported by this on publish */
			code = data->on_publish (domain->ctx, domain, myqtt_ctx, conn, msg, data->user_data);
			if (code == MYQTT_PUBLISH_DISCARD) {
				/* discard message because one of the handlers discarded the message */
				return code;
			} /* end if */

			/* rest of codes for now, ignored */
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end while */
	
	return MYQTT_PUBLISH_OK; /* allow publish */
}

axl_bool __myqttd_run_on_header_msg (MyQttCtx * myqtt_ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer _domain)
{
	MyQttdDomain        * domain   = _domain;
	MyQttdCtx           * ctx      = domain->ctx;
	
	/* check if domain has settings and message limit */
	if (domain->initialized && domain->use_settings) {
		if (domain->settings && domain->settings->message_size_limit > 0) {
			if (myqtt_msg_get_payload_size (msg) > domain->settings->message_size_limit) {
				error ("Message size limit exceeded (%d > %d) : closing connection", myqtt_msg_get_payload_size (msg), domain->settings->message_size_limit);
				return axl_false; /* avoid message reception and close connection */
			} /* end if */
		} /* end if */
	} /* end if */

	return axl_true; /* report message accepted */
}

axl_bool __myqttd_run_on_store_msg (MyQttCtx * myqtt_ctx, MyQttConn * conn, 
				    const char * client_identifier, 
				    int packet_id, MyQttQos qos, 
				    unsigned char * app_msg, int app_msg_size, axlPointer _domain)
{
	MyQttdDomain        * domain   = _domain;
	MyQttdCtx           * ctx      = domain->ctx;
	int                   value    = 0;
	char                * key;

	/* check if domain has settings and message limit */
	if (domain->initialized && domain->use_settings) {
		if (domain->settings && domain->settings->storage_quota_limit > 0) {

			key   = axl_strdup_printf ("%s_str:qt", client_identifier);
			value = PTR_TO_INT (myqtt_ctx_get_data (myqtt_ctx, key));

			/* get current storage quota used */
			if (value == 0) 
				value = myqtt_storage_queued_messages_quota_offline (myqtt_ctx, client_identifier);
			if (value + app_msg_size > (domain->settings->storage_quota_limit * 1024)) {
				error ("Quota exceeded (%d > %d) qos %d, app_msg_size %d, packet_id %d : rejecting storing message for %s (domain %s)",
				       value + app_msg_size, domain->settings->storage_quota_limit * 1024,
				       qos, app_msg_size, packet_id, client_identifier, domain->name);
				axl_free (key);
				return axl_false;
			} /* end if */

			/* save values */
			value += app_msg_size;
			myqtt_ctx_set_data_full (myqtt_ctx, key, INT_TO_PTR (value), axl_free, NULL);
			
			/* store operation allowed */
		} /* end if */
	} /* end if */

	return axl_true; /* store operation allowed */
}

void __myqttd_run_on_release_msg (MyQttCtx * myqtt_ctx, MyQttConn * conn, 
				  const char * client_identifier, 
				  int packet_id, MyQttQos qos, 
				  unsigned char * app_msg, int app_msg_size, axlPointer _domain)
{
	MyQttdDomain        * domain   = _domain;
	int                   value    = 0;
	char                * key;
	
	/* check if domain has settings and message limit */
	if (domain->initialized && domain->use_settings) {
		if (domain->settings && domain->settings->storage_quota_limit > 0) {

			/* get key and value */
			key   = axl_strdup_printf ("%s_str:qt", client_identifier);
			value = PTR_TO_INT (myqtt_ctx_get_data (myqtt_ctx, key));

			if (value == 0) {
				/* get current storage quota used of nothing was found cached */
				value = myqtt_storage_queued_messages_quota_offline (myqtt_ctx, client_identifier);
			} /* end if */

			/* save values */
			value -= app_msg_size;
			if (value > 0) {
				/* update stored valued */
				myqtt_ctx_set_data_full (myqtt_ctx, key, INT_TO_PTR (value), axl_free, NULL);
			} else {
				/* delete key because it is not going
				 * to be stored */
				myqtt_hash_remove (myqtt_ctx->data, key);
				axl_free (key);
			}
			
			/* release operation accounted */
		} /* end if */
	} /* end if */

	return;
}

void __myqttd_init_domain_context (MyQttdCtx * ctx, MyQttdDomain * domain)
{
	int      subs;
	axl_bool debug_was_not_requested;

	if (domain->initialized)
		return;

	/* init context */
	domain->myqtt_ctx = myqtt_ctx_new ();

	/* init this context */
	if (! myqtt_init_ctx (domain->myqtt_ctx)) {
		myqtt_exit_ctx (domain->myqtt_ctx, axl_true);
		domain->myqtt_ctx = NULL;
		return;
	} /* end if */

	/* enable debug as it is in the parent context */
	debug_was_not_requested = PTR_TO_INT (myqttd_ctx_get_data (ctx, "debug-was-not-requested"));
	if (debug_was_not_requested) {
		/* enable log but to catch critical errors */
		myqtt_log_enable (domain->myqtt_ctx, axl_true); 
		myqtt_log_set_prepare_log (domain->myqtt_ctx, axl_true);
		myqtt_log_set_handler (domain->myqtt_ctx, __myqttd_run_log_handler, ctx);
	} else {
		/* enable debug was denabled in parent */
		myqtt_log_enable       (domain->myqtt_ctx, myqtt_log_is_enabled (ctx->myqtt_ctx));
		myqtt_log2_enable      (domain->myqtt_ctx, myqtt_log2_is_enabled (ctx->myqtt_ctx));
		myqtt_color_log_enable (domain->myqtt_ctx, myqtt_color_log_is_enabled (ctx->myqtt_ctx));
	}

	/* configure storage path */
	msg ("Setting storage path=%s for domain=%s", domain->storage_path, domain->name);
	if (! myqtt_storage_set_path (domain->myqtt_ctx, domain->storage_path, 4096)) {
		error ("Unable to configure storage path at %s, myqtt_storage_set_path failed", domain->storage_path);
		myqtt_exit_ctx (domain->myqtt_ctx, axl_true);
		domain->myqtt_ctx = NULL;
		return;
	} /* end if */

	/* call to load local storage first (before an incoming
	 * connection) */
	msg ("Loading storage myqtt_ctx=%p", domain->myqtt_ctx);
	subs = myqtt_storage_load (domain->myqtt_ctx);
	msg ("Finished, found %d client ids recovered...", subs);

	/* configure on publish msg */
	myqtt_ctx_set_on_publish (domain->myqtt_ctx, __myqttd_run_on_publish_msg, domain);

	/* configure on header msg */
	myqtt_ctx_set_on_header (domain->myqtt_ctx, __myqttd_run_on_header_msg, domain);

	/* configure store and release */
	myqtt_ctx_set_on_store (domain->myqtt_ctx, __myqttd_run_on_store_msg, domain);
	myqtt_ctx_set_on_release (domain->myqtt_ctx, __myqttd_run_on_release_msg, domain);

	/* get reference to the domain settings (if any) */
	domain->settings = myqtt_hash_lookup (ctx->domain_settings, (axlPointer) domain->use_settings);

	/* flag domain as initialized */
	domain->initialized = axl_true;

	return;
}

void myqttd_run_watch_after_unwatch (MyQttCtx * _ctx, MyQttConn * conn, axlPointer ptr)
{
	MyQttdDomain * domain     = ptr;

	/* register the connection into the new handler */
	myqtt_reader_watch_connection (domain->myqtt_ctx, conn);

	/* resend messages queued */
	if (myqtt_storage_queued_messages (domain->myqtt_ctx, conn) > 0) {
		/* we have pending messages, order to deliver them */
		myqtt_storage_queued_flush (domain->myqtt_ctx, conn);
	} /* end if */

	return;
}

MyQttConnAckTypes myqttd_run_send_connection_to_domain (MyQttdCtx * ctx, MyQttConn * conn, 
							MyQttCtx  * myqtt_ctx, MyQttdDomain * domain,
							const char * username, const char * client_id, const char * server_Name) {

	int         connections;
	MyQttConn * conn2;

	/* ensure context is initialized */
	if (! domain->initialized) {
		/* lock and unlock */
		myqtt_mutex_lock (&domain->mutex);
		if (! domain->initialized) {
			/* call to initiate context */
			__myqttd_init_domain_context (ctx, domain);
			if (domain->myqtt_ctx == NULL) {
				/* unable to initializae context */
				myqtt_mutex_unlock (&domain->mutex);
				return axl_false;
			} /* end if */
		} /* end if */
		myqtt_mutex_unlock (&domain->mutex);
	} /* end if */

	if (! domain->initialized) 
		return MYQTT_CONNACK_SERVER_UNAVAILABLE;

	/*** PHASE 1: check connections to the domain ***/
	if (domain->initialized && domain->use_settings) {
		if (domain->settings) {
			/* get current connections (plus 1, as we are simulating handling it) */
			connections = axl_list_length (domain->myqtt_ctx->conn_list) + axl_list_length (domain->myqtt_ctx->srv_list) + 1;
			/* msg ("Connections: %d/%d (%s -- %p)", connections, domain->settings->conn_limit,
			   domain->use_settings, domain->settings); */

			/* checking limits */
			if (connections > domain->settings->conn_limit) {
				wrn ("Connection rejected for username=%s client-id=%s server-name=%s domain=%s settings=%s : connection limit reached %d/%d",
				     myqttd_ensure_str (username), myqttd_ensure_str (client_id), myqttd_ensure_str (server_Name), domain->name, 
				     domain->use_settings, connections, domain->settings->conn_limit);
				return MYQTT_CONNACK_REFUSED;
			} /* end if */
		} else {
			/* report failure in configuration */
			wrn ("Found domain %s with settings configured %s but it wasn't found (did you miss to define it or to point to it properly)",
			     domain->name, domain->use_settings);
		} /* end if */
	} /* end if */

	/*** PHASE 2: init session storage for the connection (if any) ***/
	/* init storage if it has session */
	if (! conn->clean_session) {
		if (! myqtt_storage_init (domain->myqtt_ctx, conn, MYQTT_STORAGE_ALL)) {
			error ("Unable to init storage service for provided client identifier '%s', unable to accept connection", conn->client_identifier);
			return MYQTT_CONNACK_SERVER_UNAVAILABLE;
		} /* end if */

		if (! myqtt_storage_session_recover (domain->myqtt_ctx, conn)) {
			error ("Failed to recover session for the provided connection, unable to accept connection");
			return MYQTT_CONNACK_SERVER_UNAVAILABLE;
		} /* end if */

		/* session recovered, now remove offline subscriptions */
		__myqtt_reader_move_offline_to_online (domain->myqtt_ctx, conn);
	} /* end if */

	/*** PHASE 3: update client id hashes ***/
	/* register client identifier */
	myqtt_mutex_lock (&domain->myqtt_ctx->client_ids_m);

	/* init client ids hashes if it wasn't */
	if (domain->myqtt_ctx->client_ids == NULL)
		domain->myqtt_ctx->client_ids = axl_hash_new (axl_hash_string, axl_hash_equal_string);

	/** Ensure that no client connects with same id unless
	    drop_conn_same_client_id is enabled. According to MQTT
	    standard ([MQTT-3.1.4-2], page * 12, section 3.1.4
	    response, it states that by default the * server must
	    disconnect previous. */
	conn2 = axl_hash_get (domain->myqtt_ctx->client_ids, (axlPointer) conn->client_identifier);
	if (conn2) {
		/* connection with same client identifier found, now
		   check if we have to reply */
		if (! domain->settings->drop_conn_same_client_id) {
			/* client id found, reject it */
			myqtt_mutex_unlock (&domain->myqtt_ctx->client_ids_m);
			error ("Rejected CONNECT request because client id %s is already in use, denying connect", conn->client_identifier);
			return MYQTT_CONNACK_IDENTIFIER_REJECTED;
		} /* end if */

		/* reached this point, we have to drop previous
		   connection */
		wrn ("Replacing conn-id=%d by conn-id=%d because client id %s was found, dropping old connection", 
		     conn2->id, conn->id, conn->client_identifier);
		myqtt_conn_shutdown (conn2);
	} /* end if */

	axl_hash_insert_full (domain->myqtt_ctx->client_ids, 
			      axl_strdup (conn->client_identifier), axl_free,
			      conn, NULL);
	myqtt_mutex_unlock (&domain->myqtt_ctx->client_ids_m);

	/* remove registry from parent context */
	myqtt_mutex_lock (&myqtt_ctx->client_ids_m);
	axl_hash_remove (myqtt_ctx->client_ids, conn->client_identifier);
	myqtt_mutex_unlock (&myqtt_ctx->client_ids_m);

	/* un register this connection from current reader */
	myqtt_reader_unwatch_connection (ctx->myqtt_ctx, conn, myqttd_run_watch_after_unwatch, domain);

	/* enable domain and send connection in an async manner */
	return MYQTT_CONNACK_ACCEPTED;
}


MyQttConnAckTypes  myqttd_run_handle_on_connect (MyQttCtx * myqtt_ctx, MyQttConn * conn, axlPointer user_data)
{
	MyQttdDomain * domain;
	MyQttdCtx    * ctx = user_data;

	/* check support for auth operations */
	const char   * username     = myqtt_conn_get_username (conn);
	const char   * password     = myqtt_conn_get_password (conn);
	const char   * client_id    = myqtt_conn_get_client_id (conn);
	const char   * server_Name  = myqtt_conn_get_server_name (conn);

	/* conn limits */
	MyQttConnAckTypes    codes;

	/* find the domain that can handle this connection and do the AUTH operation at once */
	domain = myqttd_domain_find_by_indications (ctx, conn, username, client_id, password, server_Name);
	if (domain == NULL) {
		error ("Login failed for username=%s client-id=%s server-name=%s : no domain was found to handle request",
		       myqttd_ensure_str (username), myqttd_ensure_str (client_id), myqttd_ensure_str (server_Name));
		return MYQTT_CONNACK_IDENTIFIER_REJECTED;
	} /* end if */

	/*** IMPORTANT NOTE HERE: about myqttd_domain_find_by_indications
	 *    
	 *   if domain was found, we can consider a login ok unless we
	 *   find a failure sending the connection to the domain, but
	 *   that failure is more for technical reasons (limits, etc)
	 *   rather than a login failure .
	 * 
	 ***/

	/* activate domain to have it working */
	codes = myqttd_run_send_connection_to_domain (ctx, conn, myqtt_ctx, domain, username, client_id, server_Name);
	if (codes != MYQTT_CONNACK_ACCEPTED) {
		error ("Login failed for username=%s client-id=%s server-name=%s : failed to send connection to the corresponding domain",
		       username ? username : "", client_id ? client_id : "", server_Name ? server_Name : "");
		return codes;
	} /* end if */

	/* reached this point, the connecting user is enabled and authenticated */
	msg ("Connection accepted for username=%s client-id=%s server-name=%s conn-id=%d : selected domain=%s",
	     myqttd_ensure_str (username), myqttd_ensure_str (client_id), myqttd_ensure_str (server_Name), conn->id, domain->name);
	
	/* report connection accepted */
	return codes;
} /* end if */

void __myqttd_run_get_value_from_node (MyQttdCtx * ctx, axlNode * node, const char * _type, int * value, int _default)
{
	/* set default value */
	(*value)       = _default;
	if (node) {
		if (axl_cmp (_type, "boolean"))
			(*value) = myqttd_config_is_attr_positive (ctx, node, "value");
		if (axl_cmp (_type, "int"))
			(*value) = myqtt_support_strtod (ATTR_VALUE (node, "value"), NULL);
	}
	return;
}

void __myqttd_run_get_value (MyQttdCtx * ctx, axlDoc * doc, const char * const_path, const char * _type, int * value, int _default)
{
	axlNode * node = axl_doc_get (doc, const_path);

	/* set default value */
	__myqttd_run_get_value_from_node (ctx, node, _type, value, _default);
	return;
}

void __myqttd_run_get_value_by_node (MyQttdCtx * ctx, axlNode * node, const char * child_name, const char * _type, int * value, int _default)
{
	axlNode * _node = axl_node_get_child_called (node, child_name);

	/* set default value */
	__myqttd_run_get_value_from_node (ctx, _node, _type, value, _default);
	return;
}

axl_bool myqttd_run_domain_settings_load (MyQttdCtx * ctx, axlDoc * doc)
{
	axlNode             * node;
	MyQttdDomainSetting * setting;
	const char          * name;

	/* default settings */
	if (! ctx->default_setting)
		ctx->default_setting = axl_new (MyQttdDomainSetting, 1);
	if (! ctx->domain_settings)
		ctx->domain_settings = myqtt_hash_new (axl_hash_string, axl_hash_equal_string);

	/* require auth */
	__myqttd_run_get_value (ctx, doc, "/myqtt/domain-settings/global-settings/require-auth", "boolean", &(ctx->default_setting->require_auth), axl_true);
	/* restrict-ids */
	__myqttd_run_get_value (ctx, doc, "/myqtt/domain-settings/global-settings/restrict-ids", "boolean", &(ctx->default_setting->restrict_ids), axl_false);

	/* restrict-ids */
	__myqttd_run_get_value (ctx, doc, "/myqtt/domain-settings/global-settings/drop-conn-same-client-id", "boolean", &(ctx->default_setting->drop_conn_same_client_id), axl_false);

	/* conn-limit */
	__myqttd_run_get_value (ctx, doc, "/myqtt/domain-settings/global-settings/conn-limit", "int", &(ctx->default_setting->conn_limit), -1);
	/* message-size-limit */
	__myqttd_run_get_value (ctx, doc, "/myqtt/domain-settings/global-settings/message-size-limit", "int", &(ctx->default_setting->message_size_limit), -1);
	/* storage-messages-limit */
	__myqttd_run_get_value (ctx, doc, "/myqtt/domain-settings/global-settings/storage-messages-limit", "int", &(ctx->default_setting->storage_messages_limit), -1);
	/* storage-quota-limit */
	__myqttd_run_get_value (ctx, doc, "/myqtt/domain-settings/global-settings/storage-quota-limit", "int", &(ctx->default_setting->storage_quota_limit), -1);

	/* get first definition */
	node = axl_doc_get (doc, "/myqtt/domain-settings/domain-setting");
	while (node != NULL) {

		/* ensure it has name and value */
		name = ATTR_VALUE (node, "name");
		if (! name || strlen (name) == 0) {
			/* get next module */
			node = axl_node_get_next_called (node, "domain-setting");
			continue;
		} /* end if */

		/* get name and store this setting */
		setting = (MyQttdDomainSetting *) myqtt_hash_lookup (ctx->domain_settings, (axlPointer) name);

		/* create setting storage */
		if (setting == NULL) {
			/* create node */
			setting = axl_new (MyQttdDomainSetting, 1);
			if (setting == NULL)
				break;

			/* save setting into hash */
			myqtt_hash_replace_full (ctx->domain_settings, (axlPointer) name, NULL, setting, axl_free);
		} /* end if */

		/* require auth */
		__myqttd_run_get_value_by_node (ctx, node, "require-auth", "boolean", &(setting->require_auth),
						ctx->default_setting->require_auth);
		/* restrict-ids */
		__myqttd_run_get_value_by_node (ctx, node, "restrict-ids", "boolean", &(setting->restrict_ids),
						ctx->default_setting->restrict_ids);
		/* drop-conn-same-client-id */
		__myqttd_run_get_value_by_node (ctx, node, "drop-conn-same-client-id", "boolean", &(setting->drop_conn_same_client_id),
						ctx->default_setting->drop_conn_same_client_id);
		/* conn-limit */
		__myqttd_run_get_value_by_node (ctx, node, "conn-limit", "int", &(setting->conn_limit),
						ctx->default_setting->conn_limit);
		/* message-size-limit */
		__myqttd_run_get_value_by_node (ctx, node, "message-size-limit", "int", &(setting->message_size_limit),
						ctx->default_setting->message_size_limit);
		/* storage-messages-limit */
		__myqttd_run_get_value_by_node (ctx, node, "storage-messages-limit", "int", &(setting->storage_messages_limit),
						ctx->default_setting->storage_messages_limit);
		/* storage-quota-limit */
		__myqttd_run_get_value_by_node (ctx, node, "storage-quota-limit", "int", &(setting->storage_quota_limit),
						ctx->default_setting->storage_quota_limit);

		/* get next module */
		node = axl_node_get_next_called (node, "domain-setting");
	} /* end while */

	return axl_true; /* domains loaded */
}


/** 
 * @internal Takes current configuration, and starts all settings
 * required to run the server.
 * 
 * Later, all modules will be loaded adding profile configuration.
 *
 * @return axl_false if the function is not able to properly start
 * myqttd or the configuration will produce bad results.
 */
int  myqttd_run_config    (MyQttdCtx * ctx)
{
	/* get the document configuration */
	axlDoc           * doc        = myqttd_config_get (ctx);
	axlNode          * node;
	MyQttCtx         * myqtt_ctx = myqttd_ctx_get_myqtt_ctx (ctx);

	/* mod myqttd dtd */
	char             * string_aux;
#if defined(AXL_OS_UNIX)
	/* required by the myqtt_conf_set hard/soft socket limit. */
	int                int_aux;
#endif

	/* check ctx received */
	if (ctx == NULL) 
		return axl_false;

	/* check log configuration */
	node = axl_doc_get (doc, "/myqtt/global-settings/log-reporting");
	if (HAS_ATTR_VALUE (node, "enabled", "yes")) {
		/* init the log module */
		myqttd_log_init (ctx);
		
		/** NOTE: do not move log reporting initialization
		 * from here even knowing some logs (at the very
		 * begin) will not be registered. This is because this
		 * is the very earlier place where log can be
		 * initializad: configuration file was red and clean
		 * start configuration is also read. */
	} /* end if */

	/* configure max connection settings here */
	node       = axl_doc_get (doc, "/myqtt/global-settings/connections/max-connections");

#if defined(AXL_OS_UNIX)
	/* NOTE: currently, myqtt_conf_set do not allows to configure
	 * the hard/soft connection limit on windows platform. Once done,
	 * this section must be public (remove
	 * defined(AXL_OS_UNIX)). */

	/* check if node is defined since it is an optional configuration */
	if (node != NULL) {
		/* set hard limit */
		string_aux = (char*) ATTR_VALUE (node, "hard-limit");
		int_aux    = strtol (string_aux, NULL, 10);
		if (! myqtt_conf_set (myqtt_ctx, MYQTT_HARD_SOCK_LIMIT, int_aux, NULL)) {
			error ("failed to set hard limit to=%s (int value=%d), error: %s, terminating myqttd..",
			       string_aux, int_aux, myqtt_errno_get_last_error ());
		} else {
			msg ("configured max connections hard limit to: %s", string_aux);
		} /* end if */

		/* set soft limit */
		string_aux = (char*) ATTR_VALUE (node, "soft-limit");
		int_aux    = strtol (string_aux, NULL, 10);
		if (! myqtt_conf_set (myqtt_ctx, MYQTT_SOFT_SOCK_LIMIT, int_aux, NULL)) {
			error ("failed to set soft limit to=%s (int value=%d), terminating myqttd..",
			       string_aux, int_aux);
		} else {
			msg ("configured max connections soft limit to: %s", string_aux);			
		} /* end if */
	} /* end if */
#endif 

	/* now load all modules found */
	myqttd_run_load_modules (ctx, doc);

	/* start here log manager */
	myqttd_log_manager_start (ctx);

	/* get the first listener configuration */
	if (! myqttd_run_config_start_listeners (ctx, doc))
		return axl_false;

	/* load domain settings */
	if (! myqttd_run_domain_settings_load (ctx, doc))
		return axl_false;

	/* now, recognize domains supported */
	if (! myqttd_run_domains_load (ctx, doc))
		return axl_false;

	/* now install auth handler to accept conections and to
	 * redirect them to the right domain */
	myqtt_ctx_set_on_connect (ctx->myqtt_ctx, myqttd_run_handle_on_connect, ctx);

	/* myqttd started properly */
	return axl_true;
}

/** 
 * @internal Allows to cleanup the module.
 */
void myqttd_run_cleanup (MyQttdCtx * ctx)
{
	return;
}


/** 
 * @}
 */
