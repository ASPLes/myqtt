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


#include <myqttd.h>
#include <myqttd-ctx-private.h>

/* include inline dtd */
#include <mod-myqttd.dtd.h>

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

	node = axl_doc_get (doc, "/myqttd/modules/no-load/module");
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
		
		fullpath = vortex_support_build_filename (path, entry->d_name, NULL);
		if (! vortex_support_file_test (fullpath, FILE_IS_REGULAR))
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
		
		/* now validate the file found */
		if (! axl_dtd_validate (doc, ctx->module_dtd, &error)) {
			wrn ("file %s does is not a valid myqttd module pointer: %s", fullpath,
			     axl_error_get (error));
			goto next;
		} /* end if */

		msg ("loading mod myqttd pointer: %s", fullpath);

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

	directory = axl_doc_get (doc, "/myqttd/modules/directory");
	if (directory == NULL) {
		msg ("no module directories were configured, nothing loaded");
		return;
	}

	/* check every module */
	while (directory != NULL) {
		/* get the directory */
		path = ATTR_VALUE (directory, "src");
		
		/* try to open the directory */
		if (vortex_support_file_test (path, FILE_IS_DIR | FILE_EXISTS)) {
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

axl_bool myqttd_run_config_start_listeners (MyQttdCtx * ctx, axlDoc * doc)
{
	axlNode          * listener;
	axl_bool           at_least_one_listener = axl_false;
	axlNode          * name;
	axlNode          * port;
	VortexConnection * conn_listener;
	VortexCtx        * vortex_ctx = myqttd_ctx_get_vortex_ctx (ctx);

	/* check if this is a child process (it has no listeners, only
	 * master process do) */
	if (ctx->child)
		return axl_true;

	/* get reference to the first listener defined */
	listener = axl_doc_get (doc, "/myqttd/global-settings/listener");
	while (listener != NULL) {

		/* get the listener name configuration */
		name = axl_node_get_child_called (listener, "name");
		
		/* get ports to be allocated */
		port = axl_doc_get (doc, "/myqttd/global-settings/ports/port");
		while (port != NULL) {

			/* start the listener */
			conn_listener = vortex_listener_new (
				/* the context where the listener will
				 * be started */
				vortex_ctx,
				/* listener name */
				axl_node_get_content (name, NULL),
				/* port to use */
				axl_node_get_content (port, NULL),
				/* on ready callbacks */
				NULL, NULL);
			
			/* check the listener started */
			if (! vortex_connection_is_ok (conn_listener, axl_false)) {
				/* unable to start the server configuration */
				error ("unable to start listener at %s:%s...", 
				       /* server name */
				       axl_node_get_content (name, NULL),
				       /* server port */
				       axl_node_get_content (port, NULL));

				goto next;
			} /* end if */

			msg ("started listener at %s:%s (id: %d, socket: %d)...",
			     axl_node_get_content (name, NULL),
			     axl_node_get_content (port, NULL),
			     vortex_connection_get_id (conn_listener), vortex_connection_get_socket (conn_listener));

			/* flag that at least one listener was
			 * created */
			at_least_one_listener = axl_true;
		next:
			/* get the next port */
			port = axl_node_get_next_called (port, "port");
			
		} /* end while */
		
		/* get the next listener */
		listener = axl_node_get_next_called (listener, "listener");

	} /* end if */

	if (! at_least_one_listener) {
		error ("Unable to start myqttd, no listener configuration was started, either due to configuration error or to startup problems. Terminating..");
		return axl_false;
	}

	/* listeners ok */
	return axl_true;
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
	VortexCtx        * vortex_ctx = myqttd_ctx_get_vortex_ctx (ctx);

	/* mod myqttd dtd */
	char             * features   = NULL;
	char             * string_aux;
#if defined(AXL_OS_UNIX)
	/* required by the vortex_conf_set hard/soft socket limit. */
	int                int_aux;
#endif
	axlError         * error;

	/* check ctx received */
	if (ctx == NULL) 
		return axl_false;

	/* check log configuration */
	node = axl_doc_get (doc, "/myqttd/global-settings/log-reporting");
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
	node       = axl_doc_get (doc, "/myqttd/global-settings/connections/max-connections");

#if defined(AXL_OS_UNIX)
	/* NOTE: currently, vortex_conf_set do not allows to configure
	 * the hard/soft connection limit on windows platform. Once done,
	 * this section must be public (remove
	 * defined(AXL_OS_UNIX)). */

	/* check if node is defined since it is an optional configuration */
	if (node != NULL) {
		/* set hard limit */
		string_aux = (char*) ATTR_VALUE (node, "hard-limit");
		int_aux    = strtol (string_aux, NULL, 10);
		if (! vortex_conf_set (vortex_ctx, VORTEX_HARD_SOCK_LIMIT, int_aux, NULL)) {
			error ("failed to set hard limit to=%s (int value=%d), error: %s, terminating myqttd..",
			       string_aux, int_aux, vortex_errno_get_last_error ());
		} else {
			msg ("configured max connections hard limit to: %s", string_aux);
		} /* end if */

		/* set soft limit */
		string_aux = (char*) ATTR_VALUE (node, "soft-limit");
		int_aux    = strtol (string_aux, NULL, 10);
		if (! vortex_conf_set (vortex_ctx, VORTEX_SOFT_SOCK_LIMIT, int_aux, NULL)) {
			error ("failed to set soft limit to=%s (int value=%d), terminating myqttd..",
			       string_aux, int_aux);
		} else {
			msg ("configured max connections soft limit to: %s", string_aux);			
		} /* end if */
	} /* end if */
#endif 

	node = axl_doc_get (doc, "/myqttd/global-settings/tls-support");
	if (HAS_ATTR_VALUE (node, "enabled", "yes")) {
		/* enable sasl support */
		/* myqttd_tls_enable (); */
	} /* end if */

	/* found dtd file */
	ctx->module_dtd = axl_dtd_parse (MOD_MYQTTD_DTD, -1, &error);
	if (ctx->module_dtd == NULL) {
		error ("unable to load mod-myqttd.dtd file: %s", axl_error_get (error));
		axl_error_free (error);
		return axl_false;
	} /* end if */

	/* check features here */
	node = axl_doc_get (doc, "/myqttd/features");
	if (node != NULL) {
		
		/* get first node posibily containing a feature */
		node = axl_node_get_first_child (node);
		while (node != NULL) {
			/* check for supported features */
			if (NODE_CMP_NAME (node, "request-x-client-close") && HAS_ATTR_VALUE (node, "value", "yes")) {
				string_aux = features;
				features   = axl_concat (string_aux, string_aux ? " x-client-close" : "x-client-close");
				axl_free (string_aux);
				msg ("feature found: x-client-close");
			} /* end if */

			/* ENTER HERE new features */

			/* process next feature */
			node = axl_node_get_next (node);

		} /* end while */

		/* now set features (if defined) */
		if (features != NULL) {
			vortex_greetings_set_features (vortex_ctx, features);
			axl_free (features);
		}

	} /* end if */

	/* load search paths */
	node = axl_doc_get (doc, "/myqttd/global-settings/system-paths/search");
	while (node) {
		/* add search path */
		msg ("Adding domain (%s) search path: %s", ATTR_VALUE (node, "domain"), ATTR_VALUE (node, "path"));
		vortex_support_add_domain_search_path (TBC_VORTEX_CTX (ctx), ATTR_VALUE (node, "domain"), ATTR_VALUE (node, "path"));

		/* get next search node */
		node = axl_node_get_next_called (node, "search");
	} /* end if */

	/* now load all modules found */
	myqttd_run_load_modules (ctx, doc);

	/* now check for profiles already activated */
	if (vortex_profiles_registered (vortex_ctx) == 0) {
		/* check for <allow-start-without-profiles> */
		if (myqttd_config_is_attr_negative (ctx, 
							axl_doc_get (doc, "/myqttd/global-settings/allow-start-without-profiles"),
							"value")) {
			abort_error ("unable to start myqttd server, no profile was registered into the vortex engine either by configuration or modules");
			return axl_false;
		} /* end if */
	} /* end if */

	/* start here log manager */
	myqttd_log_manager_start (ctx);

	/* get the first listener configuration */
	if (! myqttd_run_config_start_listeners (ctx, doc))
		return axl_false;

	/* myqttd started properly */
	return axl_true;
}

/** 
 * @internal Allows to cleanup the module.
 */
void myqttd_run_cleanup (MyQttdCtx * ctx)
{
	/* cleanup module dtd */
	axl_dtd_free (ctx->module_dtd);
	ctx->module_dtd = NULL;
	return;
}


/** 
 * @}
 */
