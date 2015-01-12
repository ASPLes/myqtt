/*  Myqttd BEEP application server
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; version 2.1 of the
 *  License.
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
 *  For commercial support on build BEEP enabled solutions, supporting
 *  myqttd based solutions, etc, contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº10, Edificio Alius A, Despacho 102
 *         Alcala de Henares, 28802 (MADRID)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/mqtt
 */
#include <myqttd.h>

/* local include */
#include <myqttd-ctx-private.h>

/* include local DTD */
#include <myqttd-config.dtd.h>

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

void myqttd_config_load_expand_nodes (MyQttdCtx * ctx)
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
	axl_doc_iterate (ctx->config, DEEP_ITERATION, myqttd_config_find_include_nodes, include_nodes);
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
	return;
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
	axlError   * error;
	axlDtd     * dtd_file;

	/* check null value */
	if (config == NULL) {
		error ("config file not defined, terminating myqttd");
		return axl_false;
	} /* end if */

	/* get a reference to the configuration path used for this context */
	ctx->config_path = axl_strdup (config);

	/* load the file */
	ctx->config = axl_doc_parse_from_file (config, &error);
	if (ctx->config == NULL) {
		error ("unable to load file (%s), it seems a xml error: %s", 
		       config, axl_error_get (error));

		/* free resources */
		axl_error_free (error);

		/* call to finish myqttd */
		return axl_false;

	} /* end if */

	/* drop a message */
	msg ("file %s loaded, ok", config);

	/* now process inclusions */
	myqttd_config_load_expand_nodes (ctx);

	/* found dtd file */
	dtd_file = axl_dtd_parse (MYQTTD_CONFIG_DTD, -1, &error);
	if (dtd_file == NULL) {
		axl_doc_free (ctx->config);
		error ("unable to load DTD to validate myqttd configuration, error: %s", axl_error_get (error));
		axl_error_free (error);
		return axl_false;
	} /* end if */

	if (! axl_dtd_validate (ctx->config, dtd_file, &error)) {
		abort_error ("unable to validate server configuration, something is wrong: %s", 
			     axl_error_get (error));

		/* free and set a null reference */
		axl_doc_free (ctx->config);
		ctx->config = NULL;

		axl_error_free (error);
		return axl_false;
	} /* end if */

	msg ("server configuration is valid..");
	
	/* free resources */
	axl_dtd_free (dtd_file);

	return axl_true;
}

/** 
 * @brief Allows to get the configuration loaded at the startup. The
 * function will always return a configuration object. 
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
 * @brief Allows to check if an xml attribute is positive, that is,
 * have 1, true or yes as value.
 *
 * @param ctx The myqttd context.
 *
 * @param node The node to check for positive attribute value.
 *
 * @param attr_name The node attribute name to check for positive
 * value.
 */
axl_bool        myqttd_config_is_attr_positive (MyQttdCtx * ctx,
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
