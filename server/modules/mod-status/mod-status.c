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

#include <myqtt-conn-private.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

MyQttdCtx * ctx = NULL;
axlDoc    * mod_status_conf = NULL;

/** MyQttdUsersUnloadDb **/
void __mod_status_unload (MyQttdCtx * ctx, 
			      axlPointer  _backend)
{
	return;
}


axl_bool __mod_status_get_subscription (axlPointer key, axlPointer data, axlPointer user_data)
{
	axlDoc     * doc = user_data;
	axlNode    * node;
	const char * topic = key;
	int          qos   = PTR_TO_INT (data);
	axlNode    * root;

	/* create node and add to the document */
	node = axl_node_create ("sub");
	axl_node_set_attribute (node, "topic", topic);
	axl_node_set_attribute_ref (node, axl_strdup ("qos"), axl_strdup_printf ("%d", qos));
	
	/* add to the document */
	root = axl_doc_get (doc, "/reply");
	axl_node_set_child (root, node);
	
	return axl_false; /* keep iterating over all items */
}

MyQttPublishCodes __mod_status_on_publish (MyQttdCtx * ctx,       MyQttdDomain * domain,  
					   MyQttCtx  * myqtt_ctx, MyQttConn    * conn, 
					   MyQttMsg  * msg,       axlPointer user_data)
{
	axlDoc * doc;
	char   * content;
	int      size;


	if (axl_cmp ("myqtt/my-status/get-subscriptions", myqtt_msg_get_topic (msg))) {
		/* create empty document */
		doc = axl_doc_parse ("<reply></reply>", 17, NULL);
		if (doc == NULL)
			return MYQTT_PUBLISH_OK; /* failed to create document, skip operation */

		/* lock and unlock */
		myqtt_mutex_lock (&conn->op_mutex);
		axl_hash_foreach (conn->subs, __mod_status_get_subscription, doc);
		axl_hash_foreach (conn->wild_subs, __mod_status_get_subscription, doc);
		myqtt_mutex_unlock (&conn->op_mutex);

		/* I've got the entire document, dump it into to send
		 * it back to the client */
		if (! axl_doc_dump_pretty (doc, &content, &size, 4)) {
			/* printf ("%s:%d -- axl_doc_free (%p)\n", __AXL_FILE__, __AXL_LINE__, doc); */
			axl_doc_free (doc);
			return MYQTT_PUBLISH_OK; /* failed to create document, skip operation */
		}
		
		/* release document */
		/* printf ("%s:%d -- axl_doc_free (%p)\n", __AXL_FILE__, __AXL_LINE__, doc); */
		axl_doc_free (doc);

		/* send document */
		if (! myqtt_conn_pub (conn, myqtt_msg_get_topic (msg), content, size, MYQTT_QOS_0, axl_false, 0)) {
			axl_free (content);
			return MYQTT_PUBLISH_OK; /* failed to send content to the client, skip operation */
		} /* end if */

		/* release content */
		axl_free (content);
		
		return MYQTT_PUBLISH_DISCARD; /* notify engine to forget about this message */
	} /* end if */

	return MYQTT_PUBLISH_OK; /* report we don't know anything
				  * about this message, let it go
				  * through myqttd engine */
}

/** 
 * @brief Init function, perform all the necessary code to register
 * handlers, configure MyQtt, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  mod_status_init (MyQttdCtx * _ctx)
{
	char     * config;
	axlError * err = NULL;

	/* configure the module */
	MYQTTD_MOD_PREPARE (_ctx);

	/* add default location if the document wasn't found */
	myqtt_support_add_domain_search_path_ref (MYQTTD_MYQTT_CTX(ctx), axl_strdup ("mod-status"),
						  myqtt_support_build_filename (myqttd_sysconfdir (ctx), "myqtt", "status", NULL));
	config = myqtt_support_domain_find_data_file (MYQTTD_MYQTT_CTX (_ctx), "mod-status", "status.conf");
	if (config == NULL) {
		error ("Unable to find status.conf file under expected locations, failed to activate mod-Status support (try checking %s/myqtt/status/status.conf)",
		       myqttd_sysconfdir (ctx));
		return axl_false;
	} /* end if */

	/* try to load configuration */
	mod_status_conf = axl_doc_parse_from_file (config, &err);
	if (mod_status_conf == NULL) {
		error ("Unable to load configuration from %s, axl_doc_parse_from_file failed: %s",
		       config, axl_error_get (err));
		axl_free (config);
		axl_error_free (err);
		return axl_false;
	} /* end if */
	axl_free (config);

	/* configure API */
	myqttd_ctx_add_on_publish (ctx, __mod_status_on_publish, NULL);

	return axl_true;
}

/** 
 * @brief Close function called once the myqttd server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void mod_status_close (MyQttdCtx * ctx)
{
	axlDoc * doc = mod_status_conf;
	mod_status_conf = NULL;
	/* printf ("%s:%d -- axl_doc_free (%p)\n", __AXL_FILE__, __AXL_LINE__, doc); */
	axl_doc_free (doc);

	/* for now nothing */
	return;
}

/** 
 * @brief The reconf function is used by myqttd to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
static void mod_status_reconf (MyQttdCtx * ctx) {
	/* for now nothing */
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the myqttd will lookup to load the rest of items.
 */
MyQttdModDef module_def = {
	"mod-status",
	"Status module for various sentences that provides real-time status to requesting clients",
	mod_status_init,
	mod_status_close,
	mod_status_reconf,
	NULL,
};

END_C_DECLS
