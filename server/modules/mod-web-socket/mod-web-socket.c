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

#include <myqtt-web-socket.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

MyQttdCtx * ctx = NULL;
axlDoc    * mod_web_socket_conf = NULL;

/** MyQttdUsersUnloadDb **/
void __mod_web_socket_unload (MyQttdCtx * ctx, 
			      axlPointer  _backend)
{
	return;
}

axlNode * __mod_web_socket_get_default_certificate (MyQttdCtx * ctx, MyQttCtx * my_ctx)
{
	axlNode * node;

	node = axl_doc_get (mod_web_socket_conf, "/mod-web-socket/certificates/cert");
	if (node == NULL)
		return NULL;
	while (node) {
		/* return first node flagged as default */
		if (HAS_ATTR_VALUE (node, "default", "yes"))
			return node;

		/* get next <cert> node */
		node = axl_node_get_next_called (node, "cert");
	} /* end while */

	/* reached this point no certificate is flagged as default,
	   then return first */
	return axl_doc_get (mod_web_socket_conf, "/mod-web-socket/certificates/cert");
}


MyQttConn * __mod_web_socket_start_listener (MyQttdCtx  * ctx, MyQttCtx * my_ctx, axlNode * port_node, 
					     const char * bind_addr, const char * port, axlPointer user_data)
{
	const char * mode     = ATTR_VALUE (port_node, "proto");
	noPollCtx  * nopoll_ctx;
	noPollConn * nopoll_listener = NULL;
	axlNode    * node;

	/* get or create a noPollCtx */
	nopoll_ctx   = myqtt_web_socket_get_ctx (my_ctx);
	if (nopoll_ctx == NULL)
		nopoll_ctx  = nopoll_ctx_new ();
	/* end */

	if (axl_cmp (mode, "mqtt-ws") || axl_cmp (mode, "ws")) {
		/* enable listener without SSL */
		nopoll_listener = nopoll_listener_new (nopoll_ctx, bind_addr, port);
	} else { 
		/* create listener */
		nopoll_listener = nopoll_listener_tls_new (nopoll_ctx, bind_addr, port);

		/* configure default certificates from store */
		node = __mod_web_socket_get_default_certificate (ctx, my_ctx);
		if (node) {
			/* set certificates */
			msg ("Setting certificate crt=%s key=%s", ATTR_VALUE (node, "crt"), ATTR_VALUE (node, "key"));
			if (! nopoll_listener_set_certificate (nopoll_listener, ATTR_VALUE (node, "crt"), ATTR_VALUE (node, "key"), NULL)) {
				error ("unable to configure certificates for TLS websocket..\n");
				return NULL;
			} /* end if */
		} /* end if */
	} /* end if */

	/* check listener created */
	if (! nopoll_conn_is_ok (nopoll_listener)) {
		error ("failed to start WebSocket listener at %s:%s..\n", bind_addr, port);
		return NULL;
	} /* end if */

	/* now start listener */
	return myqtt_web_socket_listener_new (my_ctx, nopoll_listener, NULL, NULL, NULL);
}

/** 
 * @brief Init function, perform all the necessary code to register
 * handlers, configure MyQtt, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  mod_web_socket_init (MyQttdCtx * _ctx)
{
	char     * config;
	axlError * err = NULL;

	/* configure the module */
	MYQTTD_MOD_PREPARE (_ctx);

	/* add default location if the document wasn't found */
	myqtt_support_add_domain_search_path_ref (MYQTTD_MYQTT_CTX(ctx), axl_strdup ("web-socket"),
						  myqtt_support_build_filename (myqttd_sysconfdir (ctx), "myqtt", "web-socket", NULL));
	config = myqtt_support_domain_find_data_file (MYQTTD_MYQTT_CTX (_ctx), "web-socket", "web-socket.conf");
	if (config == NULL) {
		error ("Unable to find web-socket.conf file under expected locations, failed to activate Web-Socket support (try checking %s/myqtt/web-socket/web-socket.conf)",
		       myqttd_sysconfdir (ctx));
		return axl_false;
	} /* end if */

	/* try to load configuration */
	mod_web_socket_conf = axl_doc_parse_from_file (config, &err);
	if (mod_web_socket_conf == NULL) {
		error ("Unable to load configuration from %s, axl_doc_parse_from_file failed: %s",
		       config, axl_error_get (err));
		axl_free (config);
		axl_error_free (err);
		return axl_false;
	} /* end if */
	axl_free (config);

	/* regster listener activator */
	myqttd_ctx_add_listener_activator (ctx, "mqtt-ws", __mod_web_socket_start_listener, NULL);
	myqttd_ctx_add_listener_activator (ctx, "mqtt-wss", __mod_web_socket_start_listener, NULL);
	myqttd_ctx_add_listener_activator (ctx, "ws", __mod_web_socket_start_listener, NULL);
	myqttd_ctx_add_listener_activator (ctx, "wss", __mod_web_socket_start_listener, NULL);
	
	return axl_true;
}

/** 
 * @brief Close function called once the myqttd server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void mod_web_socket_close (MyQttdCtx * ctx)
{
	axlDoc * doc = mod_web_socket_conf;
	mod_web_socket_conf = NULL;
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
static void mod_web_socket_reconf (MyQttdCtx * ctx) {
	/* for now nothing */
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the myqttd will lookup to load the rest of items.
 */
MyQttdModDef module_def = {
	"mod-web-socket",
	"WebSocket support for MyQttd server",
	mod_web_socket_init,
	mod_web_socket_close,
	mod_web_socket_reconf,
	NULL,
};

END_C_DECLS
