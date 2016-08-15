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

#include <myqtt-tls.h>

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

MyQttdCtx * ctx             = NULL;
axlDoc    * mod_ssl_conf    = NULL;
axl_bool    __mod_ssl_debug = axl_false;  /* set <mod-ssl debug='yes'>
					   * .. to enable this, at
					   * /etc/myqtt/ssl/ssl.conf */

/** MyQttdUsersUnloadDb **/
void __mod_ssl_unload (MyQttdCtx * ctx, 
		       axlPointer  _backend)
{
	return;
}

axlNode * __mod_ssl_get_default_certificate (MyQttdCtx * ctx, MyQttCtx * my_ctx)
{
	axlNode * node;

	node = axl_doc_get (mod_ssl_conf, "/mod-ssl/certificates/cert");
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
	return axl_doc_get (mod_ssl_conf, "/mod-ssl/certificates/cert");
}

axlNode *  __mod_ssl_get_certificate_node_by_name (MyQttdCtx * ctx, const char * serverName)
{
	axlNode * node;

	node = axl_doc_get (mod_ssl_conf, "/mod-ssl/certificates/cert");
	if (node == NULL)
		return NULL;
	while (node) {
		if (axl_cmp (serverName, ATTR_VALUE (node, "serverName"))) {
			/* node found, report it */
			return node;
		} /* end if */

		/* get next <cert> node */
		node = axl_node_get_next_called (node, "cert");
	} /* end while */

	/* reached this point no certificate was found, so report NULL */
	return NULL;
}


MyQttConn * __mod_ssl_start_listener (MyQttdCtx * ctx, MyQttCtx * my_ctx, axlNode * port_node, 
				      const char * bind_addr, const char * port, axlPointer user_data)
{
	MyQttConn   * listener;
	axlNode     * node;
	const char  * crt, * key, * chain;

	/* create listener on the port indicated */
	listener = myqtt_tls_listener_new (
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

	/* configure default certificates from store */
	node = __mod_ssl_get_default_certificate (ctx, my_ctx);
	if (node) {
		/* configure certificates */
		crt   = ATTR_VALUE (node, "crt");
		key   = ATTR_VALUE (node, "key");
		chain = ATTR_VALUE (node, "chain");
		msg ("mod-ssl: configuring default crt=%s, key=%s, chain=%s", crt, key, chain);
		     
		if (! myqtt_tls_set_certificate (listener, crt, key, chain)) {
			error ("unable to configure certificates for TLS mqtt (myqtt_tls_set_certificate failed)..");
			return NULL;
		} /* end if */
	} else {
		wrn ("Unable to configure any certificate. No default certificate was found. __mod_ssl_get_default_certificate() reported NULL");
	} /* end if */

	return listener; /* basic configuration done */
}

char * __mod_ssl_sni_common_handler (MyQttCtx   * myqtt_ctx,
				     MyQttConn  * conn,
				     const char * serverName,
				     axlPointer   user_data,
				     const char * attr_name)
{
	MyQttdCtx * ctx = user_data;
	axlNode   * node;

	/* find certificate node by Name */
	node = __mod_ssl_get_certificate_node_by_name (ctx, serverName);
	if (! node) {
		wrn ("No certificate was found for serverName=%s, requeted by connecting ip=%s",
		     serverName, myqtt_conn_get_host_ip (conn));
		return NULL; /* no node certificate was found, finish
			      * here */
	} /* end if */

	if (! HAS_ATTR (node, attr_name)) {
		wrn ("No certificate was found for serverName=%s, requeted by connecting ip=%s, node %s attribute was not defined",
		     serverName, myqtt_conn_get_host_ip (conn), attr_name);
		return NULL; /* no crt attr was found, finish here */
	} /* end if */

	/* ok, now report certificate location */
	if (__mod_ssl_debug)
		msg ("Reporting certificate %s=%s for serverName=%s", attr_name, ATTR_VALUE (node, attr_name), serverName);

	/* report certificate */
	return axl_strdup (ATTR_VALUE (node, attr_name));
}

char * __mod_ssl_sni_certificate_handler (MyQttCtx   * myqtt_ctx,
					  MyQttConn  * conn,
					  const char * serverName,
					  axlPointer   user_data)
{
	/* find and report certificate according to serverName */
	return __mod_ssl_sni_common_handler (myqtt_ctx, conn, serverName, user_data, "crt");
}

char * __mod_ssl_sni_private_handler (MyQttCtx   * myqtt_ctx,
				      MyQttConn  * conn,
				      const char * serverName,
				      axlPointer   user_data)
{
	/* find and report private according to serverName */
	return __mod_ssl_sni_common_handler (myqtt_ctx, conn, serverName, user_data, "key");
}

char * __mod_ssl_chain_certificate_handler  (MyQttCtx   * myqtt_ctx,
					     MyQttConn  * conn,
					     const char * serverName,
					     axlPointer   user_data)
{
	/* find and report chain certificate according to serverName */
	return __mod_ssl_sni_common_handler (myqtt_ctx, conn, serverName, user_data, "chain");
}


/** 
 * @brief Init function, perform all the necessary code to register
 * handlers, configure MyQtt, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  mod_ssl_init (MyQttdCtx * _ctx)
{
	char     * config;
	axlError * err = NULL;
	axlNode  * node;

	/* configure the module */
	MYQTTD_MOD_PREPARE (_ctx);

	/* add default location if the document wasn't found */
	myqtt_support_add_domain_search_path_ref (MYQTTD_MYQTT_CTX(ctx), axl_strdup ("ssl"),
						  myqtt_support_build_filename (myqttd_sysconfdir (ctx), "myqtt", "ssl", NULL));
	config = myqtt_support_domain_find_data_file (MYQTTD_MYQTT_CTX (_ctx), "ssl", "ssl.conf");
	if (config == NULL) {
		error ("Unable to find ssl.conf file under expected locations, failed to activate TLS support (try checking %s/myqtt/ssl/ssl.conf)",
		       myqttd_sysconfdir (ctx));
		return axl_false;
	} /* end if */

	/* try to load configuration */
	mod_ssl_conf = axl_doc_parse_from_file (config, &err);
	if (config == NULL) {
		error ("Unable to load configuration from %s, axl_doc_parse_from_file failed: %s",
		       config, axl_error_get (err));
		axl_free (config);
		axl_error_free (err);
		return axl_false;
	} /* end if */
	axl_free (config);

	/* setup ssl debug */
	node            = axl_doc_get (mod_ssl_conf, "mod-ssl");
	__mod_ssl_debug = (node && HAS_ATTR_VALUE (node, "debug", "yes"));

	/* register listener activator: make myqttd server to support the following protocols: mqtt-tls, tls, ssl and mqtt-ssl */
	myqttd_ctx_add_listener_activator (ctx, "mqtt-tls", __mod_ssl_start_listener, NULL);
	myqttd_ctx_add_listener_activator (ctx, "tls", __mod_ssl_start_listener, NULL);
	myqttd_ctx_add_listener_activator (ctx, "ssl", __mod_ssl_start_listener, NULL);
	myqttd_ctx_add_listener_activator (ctx, "mqtt-ssl", __mod_ssl_start_listener, NULL);

	/* install handlers for SNI support: certificate selection
	 * based on Server Name Indication */
	myqtt_tls_listener_set_certificate_handlers (MYQTTD_MYQTT_CTX (ctx),
						     __mod_ssl_sni_certificate_handler,
						     __mod_ssl_sni_private_handler,
						     __mod_ssl_chain_certificate_handler,
						     ctx);
	
	return axl_true;
}

/** 
 * @brief Close function called once the myqttd server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void mod_ssl_close (MyQttdCtx * ctx)
{
	/* printf ("%s:%d -- axl_doc_free (%p)\n", __AXL_FILE__, __AXL_LINE__, mod_ssl_conf); */
	axl_doc_free (mod_ssl_conf);

	/* for now nothing */
	return;
}

/** 
 * @brief The reconf function is used by myqttd to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
static void mod_ssl_reconf (MyQttdCtx * ctx) {
	/* for now nothing */
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the myqttd will lookup to load the rest of items.
 */
MyQttdModDef module_def = {
	"mod-ssl",
	"TLS/SSL support for MyQttd server",
	mod_ssl_init,
	mod_ssl_close,
	mod_ssl_reconf,
	NULL,
};

END_C_DECLS

/** 
 * \page myqttd_mod_tls MyQttd mod-ssl configuration
 *
 * \section myqttd_mod_tls_intro Introduction to mod-ssl
 *
 * SSL/TLS configuration for MyQttD is handled through this module. In
 * essence, this module allows to configure the list of certificates
 * associated to the serverName (if available) so MyQttD can select
 * the right certificate according to the connecting client.
 *
 * 
 * \section myqttd_mod_ssl_enabling Enabling mod-ssl 
 *
 * The module expects to find a configuration file located, usually
 * located at <b>/etc/myqtt/ssl/ssl.conf</b>. If you do not have it,
 * you can use the example provided by running:
 *
 * \code
 * >> mv /etc/myqtt/ssl/ssl.example.conf /etc/myqtt/ssl/ssl.conf 
 * \endcode
 *
 * Before using it, you have to enable the module by running something like:
 *
 * \code
 * >> ln -s /etc/myqtt/mods-available/mod-ssl.xml /etc/myqtt/mods-enabled/mod-ssl.xml
 * >> service myqtt restart
 * \endcode
 *
 * \section myqttd_mod_ssl_configuring Configuring mod-ssl
 *
 * Let's assume we have the default configuration provided by ssl.example.conf:
 *
 * \htmlinclude ssl.example.conf.tmp
 *
 * This file (ssl.conf) includes a list of certificates associated to
 * the <b>serverName</b> (the common name requested through SNI, or
 * Host: header requested through the WebSocket bridge).
 *
 * The first certificate declared as <b>default="yes"</b> will be used
 * in the case no serverName matches. If no certificate is declared as
 * such, the first certificate on the list will be used as default
 * certificate.
 *
 * \section myqttd_mod_ssl_enable_debug Enabling mod-ssl debug
 *
 * You can enable debug flag so mod-ssl module will drop more logs
 * into the console and logs. For that just set <b>debug='yes'</b>
 * inside top <b>&lt;mod-ssl></b> node, at the ssl.conf, usually
 * located at /etc/myqtt/ssl/ssl.conf
 * 
 *
 */

