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
#include <myqttd.h>

/* local include */
#include <myqttd-ctx-private.h>

/** 
 * \defgroup myqttd_config Myqttd Config: files to access to run-time Myqttd Config
 */

/**
 * \addtogroup myqttd_config
 * @{
 */

/** 
 * @internal Iteration function used to find all <include> nodes.
 */
axl_bool myqttd_config_find_include_nodes (axlNode    * node, 
					       axlNode    * parent, 
					       axlDoc     * doc, 
					       axl_bool   * was_removed, 
					       axlPointer   ptr)
{
	/* check and add */
	if (NODE_CMP_NAME (node, "include")) {
		/* add to the list */
		axl_list_append (ptr, node);
	}
	

	return axl_true; /* never stop until the last node is
			  * visited */
}

axl_bool myqttd_config_load_expand_nodes (MyQttdCtx * ctx, axlDoc * doc)
{
	axlList    * include_nodes;
	int          iterator;
	axlNode    * node, * node2;
	axlDoc     * doc_aux;
	axlError   * error;
	const char * path;
	char       * full_path;

	/* directory reading */
	DIR           * dir;
	struct dirent * dirent;
	int             length;


	/* create the list and iterate over all nodes */
	include_nodes = axl_list_new (axl_list_always_return_1, NULL);
	axl_doc_iterate (doc, DEEP_ITERATION, myqttd_config_find_include_nodes, include_nodes);
	msg ("Found include nodes %d, expanding..", axl_list_length (include_nodes));

	/* next position */
	iterator = 0;
	while (iterator < axl_list_length (include_nodes)) {

		/* get the node to replace */
		node = axl_list_get_nth (include_nodes, iterator);

		/* try to load document or directory */
		if (HAS_ATTR (node, "src")) {
			error = NULL;
			path  = ATTR_VALUE (node, "src");
			msg ("Loading document from: %s", path);
			doc_aux = axl_doc_parse_from_file (path, &error);
			if (doc_aux == NULL) {
				wrn ("Failed to load document at %s, error was: %s", path, axl_error_get (error));
				axl_error_free (error);
			} else {
				/* ok, now replace the content from the document into the original document */
				node2 = axl_doc_get_root (doc_aux);
				axl_node_deattach (node2);

				/* and */
				axl_node_replace (node, node2, axl_true);

				/* associate data to the node so it is released all memory when finished */
				axl_node_annotate_data_full (node2, "doc", NULL, doc_aux, (axlDestroyFunc) axl_doc_free);
					
			} /* end if */

		} else if (HAS_ATTR (node, "dir")) {
			msg ("Opening directory: %s", ATTR_VALUE (node, "dir"));
			dir    = opendir (ATTR_VALUE (node, "dir"));
			
			while (dir && (dirent = readdir (dir))) {

				/* build path */
				full_path = axl_strdup_printf ("%s/%s", ATTR_VALUE (node, "dir"), dirent->d_name);

				/* check for regular file */
				if (! myqtt_support_file_test (full_path, FILE_IS_REGULAR)) {
					axl_free (full_path);
					continue;
				} /* end if */
					
				/* found regular file */
				msg ("Found regular file: %s..loading", full_path);
				length = strlen (full_path);
				if (full_path[length - 1] == '~') {
					wrn ("Skipping file %s", full_path);
					axl_free (full_path);
					continue;
				}

				/* load document */
				error   = NULL;
				doc_aux = axl_doc_parse_from_file (full_path, &error);
				if (doc_aux == NULL) {
					wrn ("Failed to load document at %s, error was: %s", full_path, axl_error_get (error));
					axl_error_free (error);
					axl_free (full_path);
					continue;
				} 
				axl_free (full_path);

				/* ok, now replace the content from the document into the original document */
				node2 = axl_doc_get_root (doc_aux);
				axl_node_deattach (node2);
					
				/* set the node content following reference node */
				axl_node_set_child_after (node, node2);

				/* associate data to the node so it is released all memory when finished */
				axl_node_annotate_data_full (node2, "doc", NULL, doc_aux, (axlDestroyFunc) axl_doc_free);
					
				/* next directory */
			}

			/* close directory */
			closedir (dir);
			
			/* remove include node */
			axl_node_remove (node, axl_true);
		}



		/* next iterator */
		iterator++;
	} /* end while */

	axl_list_free (include_nodes);
	return axl_true;
}


/*
 * @internal function that loads the configuration from the provided
 * path, expanding all nodes and reporting the document to the caller,
 * without setting it as official anywhere.
 *
 * @return A reference to the loaded document (axlDoc) or NULL if it fails. 
 */
axlDoc * __myqttd_config_load_from_file (MyQttdCtx * ctx, const char * config)
{
	axlError * err    = NULL;
	axlDoc   * result;

	/* check null value */
	if (config == NULL) {
		error ("config file not defined, failed to load configuration from file");
		return NULL;
	} /* end if */
	
	/* load the file */
	result = axl_doc_parse_from_file (config, &err);
	if (result == NULL) {
		error ("unable to load file (%s), it seems a xml error: %s", 
		       config, axl_error_get (err));

		/* free resources */
		axl_error_free (err);

		/* call to finish myqttd */
		return NULL;

	} /* end if */

	/* drop a message */
	msg ("file %s loaded, ok", config);

	/* now process inclusions */
	if (! myqttd_config_load_expand_nodes (ctx, result)) {
		error ("Failed to expand nodes, unable to load configuration file from %s", config);
		axl_doc_free (result);
		return NULL;
	} /* end if */

	/* report configuration load */
	return result;
}

/** 
 * @internal Loads the myqttd main file, which has all definitions to make
 * myqttd to start.
 * 
 * @param config The configuration file to load by the provided
 * myqttd context.
 * 
 * @return axl_true if the configuration file looks ok and it is
 * syncatically correct.
 */
axl_bool  myqttd_config_load (MyQttdCtx * ctx, const char * config)
{
	axlDoc     * doc;

	/* call to load document */
	doc = __myqttd_config_load_from_file (ctx, config);
	if (! doc)
		return axl_false; /* failed to load document */
		

	/* get a reference to the configuration path used for this context */
	ctx->config_path = axl_strdup (config);

	/* load the file */
	ctx->config      = doc;
	
	return axl_true;
}

/** 
 * @brief Allows to get the configuration loaded at the startup. The
 * function will always return a configuration object. 
 *
 * @param ctx The context where the operation is taking place.
 * 
 * @return A reference to the axlDoc having all the configuration
 * loaded.
 */
axlDoc * myqttd_config_get (MyQttdCtx * ctx)
{
	/* null for the caller if a null is received */
	v_return_val_if_fail (ctx, NULL);

	/* return current reference */
	return ctx->config;
}

/** 
 * @brief Allows to configure the provided name and value on the
 * provided path inside the myqttd config.
 *
 * @param ctx The myqttd context where the configuration will be
 * modified.
 *
 * @param path The xml path to the configuration to be modified.
 *
 * @param attr_name The attribute name to be modified on the selected
 * path node.
 *
 * @param attr_value The attribute value to be modified on the
 * selected path node. 
 *
 * @return axl_true if the value was configured, otherwise axl_false
 * is returned (telling the value wasn't configured mostly because the
 * path is wrong or the node does not exists or any of the values
 * passed to the function is NULL).
 */
axl_bool            myqttd_config_set      (MyQttdCtx * ctx,
					    const char    * path,
					    const char    * attr_name,
					    const char    * attr_value)
{
	axlNode * node;

	/* check values received */
	v_return_val_if_fail (ctx && path && attr_name && attr_value, axl_false);

	msg ("Setting value %s=%s at path %s (%s)", attr_name, attr_value, path, attr_name);

	/* get the node */
	node = axl_doc_get (ctx->config, path);
	if (node == NULL) {
		wrn ("  Path %s was not found in config (%p)", path, ctx->config);
		return axl_false;
	} /* end if */

	/* set attribute */
	axl_node_remove_attribute (node, attr_name);
	axl_node_set_attribute (node, attr_name, attr_value);
	
	return axl_true;
}

/** 
 * @brief Ensures that the provided attribute is defined on the
 * provided node and to fail if it doesn't.
 *
 * DO NOT USE this function after having the server started. This
 * function is meant to be used during initialization to ensure
 * everything is ok and in place.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param node The node to be checked.
 *
 * @param attr_name The xml node attr name to check.
 *
 * The function does not return anything because it takes full control
 * of the caller in the case that if a failure is found, the function
 * will abort. Otherwise, the caller will recover control after the
 * function finishes.
 */
void            myqttd_config_ensure_attr (MyQttdCtx * ctx, axlNode * node, const char * attr_name)
{
	const char * value = ATTR_VALUE (node, attr_name);
	char * dump = NULL;
	int    size;

	if (value == NULL || strlen (value) == 0) {
		axl_node_dump (node, &dump, &size);
		abort_error ("You must define a '%s' attribute inside the node %s", attr_name, dump);
		axl_free (dump);
		return;
	} /* end if */

	return;
}

/** 
 * @brief Allows to check if an xml attribute is positive, that is,
 * have 1, true or yes as value.
 *
 * @param ctx The myqttd context.
 *
 * @param node The node to check for positive attribute value.
 *
 * @param attr_name The node attribute name to check for positive
 * value.
 *
 * @return axl_true if positive value is found, otherwise axl_false is returned.
 */
axl_bool        myqttd_config_is_attr_positive (MyQttdCtx     * ctx,
						axlNode       * node,
						const char    * attr_name)
{
	if (ctx == NULL || node == NULL)
		return axl_false;

	/* check for yes, 1 or true */
	if (HAS_ATTR_VALUE (node, attr_name, "yes"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "1"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "true"))
		return axl_true;

	/* none of them was found */
	return axl_false;
}

/** 
 * @brief Allows to check if an xml attribute is positive, that is,
 * have 1, true or yes as value.
 *
 * @param ctx The myqttd context.
 *
 * @param node The node to check for positive attribute value.
 *
 * @param attr_name The node attribute name to check for positive
 * value.
 *
 * @return axl_true if negative value is found, otherwise axl_false is returned.
 */
axl_bool        myqttd_config_is_attr_negative (MyQttdCtx * ctx,
						    axlNode       * node,
						    const char    * attr_name)
{
	if (ctx == NULL || node == NULL)
		return axl_false;

	/* check for yes, 1 or true */
	if (HAS_ATTR_VALUE (node, attr_name, "no"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "0"))
		return axl_true;
	if (HAS_ATTR_VALUE (node, attr_name, "false"))
		return axl_true;

	/* none of them was found */
	return axl_false;
}


/** 
 * @brief Allows to check if the attribute name under the provided xml path, exists.
 *
 * @param ctx The myqttd context where to get the configuration value.
 *
 * @param path The path to the node where the config is found.
 *
 * @param attr_name The attribute name to be checked.
 *
 * @return The function returns axl_true/axl_false according to the attribute name existence under the given xml path.
 */
axl_bool        myqttd_config_exists_attr (MyQttdCtx     * ctx,
					   const char    * path,
					   const char    * attr_name)
{
	axlNode * node;

	/* check values received */
	v_return_val_if_fail (ctx && path && attr_name, -2);

	/* get the node */
	node = axl_doc_get (ctx->config, path);
	if (node == NULL) 
		return axl_false;

	/* now get the value */
	return HAS_ATTR (node, attr_name);
}

/** 
 * @brief Allows to get the value found on provided config path at the
 * selected attribute.
 *
 * @param ctx The myqttd context where to get the configuration value.
 *
 * @param path The path to the node where the config is found.
 *
 * @param attr_name The attribute name to be returned as a number.
 *
 * @return The function returns the value configured or -1 in the case
 * the configuration is wrong. The function returns -2 in the case
 * path, ctx or attr_name are NULL. The function returns -3 in the
 * case the path is not found so the user can take default action.
 */
int             myqttd_config_get_number (MyQttdCtx * ctx, 
					      const char    * path,
					      const char    * attr_name)
{
	axlNode * node;
	int       value;
	char    * error = NULL;

	/* check values received */
	v_return_val_if_fail (ctx && path && attr_name, -2);

	msg ("Getting value at path %s (%s)", path, attr_name);

	/* get the node */
	node = axl_doc_get (ctx->config, path);
	if (node == NULL) {
		wrn ("  Path %s was not found in config (%p)", path, ctx->config);
		return -3;
	}

	msg ("  Translating value to a number %s=%s", attr_name, ATTR_VALUE (node, attr_name));

	/* now get the value */
	value = myqtt_support_strtod (ATTR_VALUE (node, attr_name), &error);
	if (error && strlen (error) > 0) 
		return -1;
	return value;
}

/** 
 * @internal Cleanups the myqttd config module. This is called by
 * Myqttd itself on exit.
 */
void myqttd_config_cleanup (MyQttdCtx * ctx)
{
	/* do not operate */
	if (ctx == NULL)
		return;

	/* free previous state */
	if (ctx->config)
		axl_doc_free (ctx->config);
	ctx->config = NULL;
	if (ctx->config_path)
		axl_free (ctx->config_path);
	ctx->config_path = NULL;

	return;
} 

/* @} */
