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
#if defined(ENABLE_TLS_SUPPORT)
#include <myqtt-tls.h>
#endif

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

MyQttdCtx * ctx = NULL;

/** 
 * @internal Definition to capture the code that signals when to apply
 * global acls: before or after user acls.
 */
typedef enum {
	MOD_AUTH_XML_APPLY_BEFORE = 1,
	MOD_AUTH_XML_APPLY_AFTER  = 2
} ModAuthXmlWhenToApply;

typedef struct _ModAuthXmlBackend {

	char         * full_path;
	axlDoc       * doc;

	axl_bool       anonymous;
	/*
	 * <myqtt-users password-format='plain|md5|sha1'>
	 * 0 - plain : password as is, clear text
	 * 1 - md5   : password md5 encoded
	 * 2 - sha1  : password md5 encoded
	 */
	int            password_format;
	axlNode      * global_acls;
	
	MyQttPublishCodes     no_match_policy;
	ModAuthXmlWhenToApply when_to_apply;
	MyQttPublishCodes     deny_action;

} ModAuthXmlBackend;

/** Implementation for MyQttdUsersLoadDb **/
axlPointer __mod_auth_xml_load (MyQttdCtx    * ctx,
				MyQttdDomain * domain,
				MyQttConn    * conn, 
				const char   * path)
{
	char              * full_path = myqtt_support_build_filename (path, "users.xml", NULL);
	axlDoc            * doc;
	axlNode           * node;
	ModAuthXmlBackend * backend;
	axlError          * err = NULL;
	const char        * value;
	
	if (full_path == NULL)
		return NULL;

	/* check if the document exists */
	if (! myqtt_support_file_test (full_path, FILE_EXISTS))
		return NULL; /* this is not an error, it might be
			      * another backend type */

	/* load document */
	doc = axl_doc_parse_from_file (full_path, &err);
	if (doc == NULL) {
		error ("Failed to load document %s, error was: %s", full_path, axl_error_get (err));
		axl_free (full_path);
		axl_error_free (err);
		return NULL;
	} /* end if */

	/* never report a global reference here because this function
	   is called to activate a particular backend for a particular
	   domain. */
	backend = axl_new (ModAuthXmlBackend, 1);
	if (backend == NULL) {
		axl_free (full_path);
		return NULL;
	} /* end if */

	/* get references */
	backend->full_path = full_path;
	backend->doc       = doc;

	/* configure anonymous status */
	node = axl_doc_get (doc, "/myqtt-users");
	backend->anonymous        = HAS_ATTR_VALUE (node, "anonymous", "yes");

	/* configure password format indication */
	if (HAS_ATTR_VALUE (node, "password-format", "md5"))
		backend->password_format = 1;
	else if (HAS_ATTR_VALUE (node, "password-format", "sha1"))
		backend->password_format = 2;
	else {
		/* just for clarity, if nothing is configured or
		   "plain", password format is: plain (clear text) */
		backend->password_format = 0;
	} /* end if */

	/* now get current global configuration */
	backend->global_acls = axl_doc_get (backend->doc, "/myqtt-users/global-acls");
	if (backend->global_acls) {
		
		/* get no match policy to quickly haccess it */
		value = ATTR_VALUE (backend->global_acls, "no-match-policy");
		if (axl_cmp (value, "close"))
			backend->no_match_policy = MYQTT_PUBLISH_CONN_CLOSE;
		else if (axl_cmp (value, "discard") || axl_cmp (value, "deny"))
			backend->no_match_policy = MYQTT_PUBLISH_DISCARD;
		else if (axl_cmp (value, "ok") || axl_cmp (value, "allow"))
			backend->no_match_policy = MYQTT_PUBLISH_OK;
		else
			backend->no_match_policy = MYQTT_PUBLISH_OK; /* by default ok for no-match-policy */
		
		/* and deny action if defined */
		value = ATTR_VALUE (backend->global_acls, "deny-action");
		if (axl_cmp (value, "close"))
			backend->deny_action = MYQTT_PUBLISH_CONN_CLOSE;
		else if (axl_cmp (value, "discard") || axl_cmp (value, "deny"))
			backend->deny_action = MYQTT_PUBLISH_DISCARD;
		else if (axl_cmp (value, "ok") || axl_cmp (value, "allow"))
			backend->deny_action = MYQTT_PUBLISH_OK;
		else
			backend->deny_action = MYQTT_PUBLISH_DISCARD; /* by default discard */
		
		/* and when to apply if defined */
		value = ATTR_VALUE (backend->global_acls, "when-to-apply");
		if (axl_cmp (value, "before"))
			backend->when_to_apply = MOD_AUTH_XML_APPLY_BEFORE;
		else if (axl_cmp (value, "after"))
			backend->when_to_apply = MOD_AUTH_XML_APPLY_AFTER;
		else
			backend->when_to_apply = MOD_AUTH_XML_APPLY_BEFORE; /* by default apply before */
		
	} /* end if */

	/* report backend */
	return backend;
}

/** Implemenation for MyQttdUsersExists **/
axl_bool     __mod_auth_xml_user_exists (MyQttdCtx    * ctx,
					 MyQttdDomain * domain,
					 axl_bool       domain_selected,
					 MyQttdUsers  * users,
					 MyQttConn    * conn, 
					 axlPointer     _backend,
					 const char   * client_id, 
					 const char   * user_name)
{
	ModAuthXmlBackend * backend    = _backend;
	axlNode           * node       = axl_doc_get (backend->doc, "/myqtt-users/user");
	int                 check_mode = 0;

	if (backend->anonymous) {
		return axl_true; /* user exists */
	} /* end if */

	/* check type of checking we have to implement */
	check_mode = myqttd_support_check_mode (user_name, client_id);
	if (check_mode == 0) {
		/* report failure because username and client id is not defined */
		return axl_false;
	} /* end if */

	while (node) {
		switch (check_mode) {
		case 3:
			/* user and client id */
			if (axl_cmp (client_id, ATTR_VALUE (node, "id")) && 
			    axl_cmp (user_name, ATTR_VALUE (node, "username")))
				return axl_true;
			break;
		case 2:
			/* client id */
			if (axl_cmp (client_id, ATTR_VALUE (node, "id")))
				return axl_true;
			break;
		case 1:
			/* user */
			if (axl_cmp (user_name, ATTR_VALUE (node, "username")))
				return axl_true;
			break;
		} /* end switch */

		/* get next <user> node */
		node = axl_node_get_next_called (node, "user");
	} /* end while */

	/* user does not exists for this backend */
	return axl_false;
}

/** Implementation for MyQttdUsersAuthUser **/
axl_bool     __mod_auth_xml_auth_user (MyQttdCtx    * ctx,
				       MyQttdDomain * domain,
				       axl_bool       domain_selected,
				       MyQttdUsers  * users,
				       MyQttConn    * conn,
				       axlPointer     _backend,
				       const char   * client_id, 
				       const char   * user_name,
				       const char   * password)
{
	ModAuthXmlBackend * backend    = _backend;
	axlNode           * node       = axl_doc_get (backend->doc, "/myqtt-users/user");
	int                 check_mode = 0;
	axl_bool            result;
#if defined(ENABLE_TLS_SUPPORT)
	char              * temp;
#endif

	/* check to authorize user if the is we have anonymous option
	 * enabled */
	if (backend->anonymous) {
		return axl_true; /* user exists */
	} /* end if */

	/* check type of checking we have to implement */
	check_mode = myqttd_support_check_mode (user_name, client_id);
	if (check_mode == 0) {
		/* report failure because username and client id is not defined */
		return axl_false;
	} /* end if */

	while (node) {
		switch (check_mode) {
		case 3:
			/* user and client id (and password) */
			if (axl_cmp (client_id, ATTR_VALUE (node, "id")) && 
			    axl_cmp (user_name, ATTR_VALUE (node, "username"))) {

				/* check password here */
				result = axl_false;
				switch (backend->password_format) {
				case 0:
					/* plain */
					result = axl_cmp (password, ATTR_VALUE (node, "password"));
					break;
#if defined(ENABLE_TLS_SUPPORT)
				case 1:
					/* md5 */
					temp   = myqtt_tls_get_digest (MYQTT_MD5, ATTR_VALUE (node, "password"));
					result = axl_cmp (password, temp);
					axl_free (temp);
					break;
				case 2:
					/* sha1 */
					temp   = myqtt_tls_get_digest (MYQTT_SHA1, ATTR_VALUE (node, "password"));
					result = axl_cmp (password, temp);
					axl_free (temp);
					break;
				} /* end switch */
#endif

				/* check authentication to store node
				 * for later use and report result */
				if (result) {
					/* anotate this node into the connection */
					myqtt_conn_set_data (conn, "mod:auth:xml:user-node", node);
				} /* end if */
				
				return result;
			}
			break;
		case 2:
			/* client id */
			if (axl_cmp (client_id, ATTR_VALUE (node, "id"))) {
				
				/* anotate this node into the connection */
				myqtt_conn_set_data (conn, "mod:auth:xml:user-node", node);
				
				return axl_true;
			}
			break;
		case 1:
			/* user */
			if (axl_cmp (user_name, ATTR_VALUE (node, "username"))) {
				
				/* anotate this node into the connection */
				myqtt_conn_set_data (conn, "mod:auth:xml:user-node", node);
				
				return axl_true;
			} /* end if */
			break;
		} /* end switch */

		/* get next <user> node */
		node = axl_node_get_next_called (node, "user");
	} /* end while */

	/* user does not exists for this backend */
	return axl_false;
}

/** MyQttdUsersUnloadDb **/
void __mod_auth_xml_unload (MyQttdCtx * ctx, 
			    axlPointer  _backend)
{
	ModAuthXmlBackend * backend = _backend;
	
	/* release full path and internal document holding database */
	axl_free (backend->full_path);
	/* printf ("%s:%d -- axl_doc_free (%p)\n", __AXL_FILE__, __AXL_LINE__, backend->doc); */
	axl_doc_free (backend->doc);
	axl_free (backend);

	return;
}

axl_bool __mod_auth_xml_on_publish_acl_deny (ModAuthXmlBackend * backend, axlNode * acl, MyQttMsg * msg, MyQttConn * conn) {

	if (acl == NULL)
		return axl_true; /* not denied */

	while (acl) {

		/* printf ("ACL: checking %s == %s\n", ATTR_VALUE (acl, "topic"), myqtt_msg_get_topic (msg)); */
		if (axl_cmp (ATTR_VALUE (acl, "topic"), myqtt_msg_get_topic (msg))) {
			
			/* acl found, see policy */
			if (strstr ("w", ATTR_VALUE (acl, "mode"))) {
				/* found write acl in PUBLISH acl, so allow it */
				return axl_true;
			}
			
			/* acl found, see policy */
			if (strstr ("publish", ATTR_VALUE (acl, "mode"))) {
				/* found write acl in PUBLISH acl, so allow it */
				return axl_true;
			} 

			/* acl found, see policy */
			if ((strstr ("publish0", ATTR_VALUE (acl, "mode")) && myqtt_msg_get_qos (msg) == MYQTT_QOS_0) ||
			    (strstr ("publish1", ATTR_VALUE (acl, "mode")) && myqtt_msg_get_qos (msg) == MYQTT_QOS_1) ||
			    (strstr ("publish2", ATTR_VALUE (acl, "mode")) && myqtt_msg_get_qos (msg) == MYQTT_QOS_2)) {
				/* found write acl in PUBLISH acl, so allow it */
				return axl_true;
			}

			/* reached this point, acl was found, but it
			 * does not allow PUBLISH (write) */
			error ("Denied PUBLISH operation from %s:%s (client-id: %s, user: %s) because acl %s (%s)",
			       myqtt_conn_get_host (conn), myqtt_conn_get_port (conn),
			       myqtt_conn_get_client_id (conn), myqtt_conn_get_username (conn) ? myqtt_conn_get_username (conn) : "<not defined>",
			       myqtt_msg_get_topic (msg),
			       backend->full_path);
			
			return axl_false; /** DENIED **/
			
		} /* end while */

		
		/* call to get next acl node */
		acl = axl_node_get_next_called (acl, "acl");
		
	} /* end while */
	
	return axl_true; /* not denied */
}

MyQttPublishCodes __mod_auth_xml_report (MyQttdCtx * ctx, MyQttMsg * msg, MyQttConn * conn, MyQttPublishCodes code)
{
	if (code == MYQTT_PUBLISH_DISCARD)
		error ("Discarding PUBLISH (%s) from %s:%s (due to acl)", myqtt_msg_get_topic (msg), myqtt_conn_get_host (conn), myqtt_conn_get_port (conn));
	else if (code == MYQTT_PUBLISH_CONN_CLOSE)
		error ("Closing connection after PUBLISH (%s) from %s:%s (due to acl)", myqtt_msg_get_topic (msg), myqtt_conn_get_host (conn), myqtt_conn_get_port (conn));  

	return code;
}

MyQttPublishCodes __mod_auth_xml_on_publish (MyQttdCtx * ctx,       MyQttdDomain * domain, 
					     MyQttCtx  * myqtt_ctx, MyQttConn    * conn, 
					     MyQttMsg  * msg,       axlPointer     user_data)
{

	MyQttdUsers       * users = myqttd_domain_get_users_backend (domain);
	ModAuthXmlBackend * backend;
	axlNode           * node;

	if (users == NULL) {
		error ("Connection close on publish because Users' backend is not available..");
		return MYQTT_PUBLISH_CONN_CLOSE;
	} /* end if */

	/* call to get backed associated to this domain */
	backend = myqttd_users_get_backend_ref (users);
	if (backend == NULL) {
		error ("Connection close on publish because User's backend is not available from myqttd_users_get_backend_ref ()");
		return __mod_auth_xml_report (ctx, msg, conn, MYQTT_PUBLISH_CONN_CLOSE);
	} /* end if */

	/** GLOBAL: apply global acls before users' acls */
	if (backend->when_to_apply == MOD_AUTH_XML_APPLY_BEFORE) {
		
		/* do acl apply before applying users' acls */
		if (! __mod_auth_xml_on_publish_acl_deny (backend, axl_node_get_child_called (backend->global_acls, "acl"), msg, conn))
			return __mod_auth_xml_report (ctx, msg, conn, backend->deny_action);
		
	} /* end if */

	/* find user and apply its acls if defined */
	node = myqtt_conn_get_data (conn, "mod:auth:xml:user-node");

	/* USER: apply acls if defined for the provided user node */
	if (! __mod_auth_xml_on_publish_acl_deny (backend, axl_node_get_child_called (node, "acl"), msg, conn))
		return __mod_auth_xml_report (ctx, msg, conn, backend->deny_action);


	/** GLOBAL: apply global acls after users' acls */
	if (backend->when_to_apply == MOD_AUTH_XML_APPLY_AFTER) {
		
		/* do acl apply after applying users' acls */
		if (! __mod_auth_xml_on_publish_acl_deny (backend, axl_node_get_child_called (backend->global_acls, "acl"), msg, conn))
			return __mod_auth_xml_report (ctx, msg, conn, backend->deny_action);
		
	} /* end if */

	/* report publish is ok so return default deny policy */
	return __mod_auth_xml_report (ctx, msg, conn, backend->no_match_policy);
}

/** 
 * @brief Init function, perform all the necessary code to register
 * handlers, configure MyQtt, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  mod_auth_xml_init (MyQttdCtx * _ctx)
{
	/* configure the module */
	MYQTTD_MOD_PREPARE (_ctx);

	/* install handlers to implement auth based on a simple xml
	   backend */
	if (! myqttd_users_register_backend (ctx, 
					     "mod-auth-xml",
					     __mod_auth_xml_load,
					     __mod_auth_xml_user_exists,
					     __mod_auth_xml_auth_user,
					     __mod_auth_xml_unload,
					     /* for future usage */
					     NULL, NULL, NULL, NULL)) {
		error ("Failed to install mod-auth-xml authentication handlers..");
		return axl_false;
	} /* end if */

	/* register an on publish (write, publish, publish0, publish1, publish2) */
	myqttd_ctx_add_on_publish (ctx, __mod_auth_xml_on_publish, NULL);

	/* register on subscribe */
	/* myqttd_ctx_add_on_subscribe (ctx, __mod_auth_xml_on_subscribe, NULL); */
	
	
	return axl_true;
}

/** 
 * @brief Close function called once the myqttd server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void mod_auth_xml_close (MyQttdCtx * ctx)
{
	/* for now nothing */
	return;
}

/** 
 * @brief The reconf function is used by myqttd to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
static void mod_auth_xml_reconf (MyQttdCtx * ctx) {
	/* for now nothing */
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the myqttd will lookup to load the rest of items.
 */
MyQttdModDef module_def = {
	"mod-auth-xml",
	"Authentication backend support for auth-xml",
	mod_auth_xml_init,
	mod_auth_xml_close,
	mod_auth_xml_reconf,
	NULL,
};

END_C_DECLS


/** 
 * \page myqttd_mod_auth_xml mod-auth-xml Authentication and Acls plugin supported on XML files for MyQttd broker
 *
 * \section myqttd_mod_auth_xml_index Index
 *
 * - \ref myqttd_mod_auth_xml_intro
 * - \ref myqttd_mod_auth_xml_enabling
 * - \ref myqttd_mod_auth_xml_configuring
 *
 * \section myqttd_mod_auth_xml_intro Introduction to mod-auth-xml
 *
 * Authentication support for MyQttD is delegated to auth backends that are registered at run-time via \ref myqttd_users_register_backend
 *
 * This module provides an authentication backend supported through
 * xml-files. Its function is pretty simple. The module expects to
 * find a configuration file <b>users.xml</b> inside the
 * <b>users-db</b> directive associated to the domain that is being
 * checked for activation/authentication.
 *
 * \section myqttd_mod_auth_xml_enabling Enabling mod-auth-xml 
 *
 * Don't forget to enable the module by running something like:
 *
 * \code
 * >> ln -s /etc/myqtt/mods-available/mod-auth-xml.xml /etc/myqtt/mods-enabled/mod-auth-xml.xml
 * >> service myqtt restart
 * \endcode
 *
 * \section myqttd_mod_auth_xml_configuring Configuring mod-auth-xml
 *
 * Let's suppose we have the following declaration inside your MyQttD
 * configuration, located at the <b>myqtt-domains</b> section:
 *
 * \htmlinclude myqtt-domain.example.xml-tmp
 *
 * Ok, assuming this configuration, inside the following directory
 * <b>/var/lib/myqtt-dbs/example.com</b> you have to place a file
 * called <b>users.xml</b>, hence a full path of:
 *
 * - /var/lib/myqtt-dbs/example.com/users.xml
 *
 * Inside this directory, you have to follow the next example to
 * declare users:
 *
 * \htmlinclude users.example.xml-tmp
 * 
 * As you can see, you can declare allowed users following the next format:
 *
 * - <b>&lt;user id="test_01" /></b>   -- to just support accepting connections from MQTT clients identifying themselves as "test_01" (client_id value)
 * - <b>&lt;user id="test_02" username="login" password="user-password" /></b>  -- to authorize users providing that set of values
 *
 * \note It is very important to understand that you can configure just <b>client-id</b> by using the <b>id</b> declaration, or <b>client-id + username + password</b>. If you just configure <b>username + password</b> it will not work.
 *
 * There is a declaration for <b>password-format</b> in the header:
 *
 * - <b>&lt;myqtt-users password-format="plain"></b>
 *
 * This attr allows to configure the format expected for the password. Allowed values are:
 *
 * - <b>plain</b> : passwords stored as is (not recommended)
 * - <b>md5</b> : passwords stored in md5
 * - <b>sha1</b> : passwords stored in sha1
 * 
 *
 * \note It is possible to use same database for different domains.
 * 
 * 
 * \section myqttd_mod_auth_xml_anonymous Configuring anonymous login
 *
 * <b>mod-auth-xml</b> also allows configure a MyQttd domain to work
 * with anoymous login/connection scheme. Note that enabling this
 * option will make the domain configured as such to "catch" all
 * connections reached to this domain.
 *
 * To configure anonymous support create a users.xml with the following format:
 *
 * \htmlinclude anonymous.example.xml-tmp
 *
 * Note that enabling this option will accept any connection reaching
 * this domain no matter if it provides a user/password or not.
 *
 * \section myqttd_mod_auth_global_acls Configuring global acls for all users inside your domain
 *
 * <b>mod-auth-xml</b> includes support to configure global acls and
 * user level acls. Here is how it is declared a basic global acl
 * configuration inside the users.xml file (see
 * <b>&lt;global-acls></b> section):
 *
 * \htmlinclude ../server/reg-test-18/users/users.xml-tmp
 *
 * As you can see, there is a new <b>&lt;global-acls></b> section that
 * includes different configurations and a list of acls that are
 * applied one after another, with the following indications:
 *
 * - <b>when-to-apply</b>: indicates if these global acls should be
 *     configured <b>before</b> or <b>after</b> user defined acls. In
 *     the case you want to be sure global acls applies first and
 *     overrides users' acls. In the case you want your users' acls to
 *     apply first then configure here <b>after</b>. This allows you configure different policies, for example, <i>"server global acls decides first"</i> (an alias for <b>before</b>) or <i>"users' acls decide first"</i> (an alias for <b>after</b>).
 *
 * - <b>no-match-policy</b>: this is the indication to instruct the
 *     server what to do if the chain of acls is ended without any
 *     result, that is, no acl denied or allowed the given
 *     operation. If you want your server to accept only what's
 *     configured use <b>close</b>, <b>discard</b> or <b>deny</b>
 *     ---they are now explained. If you want your server to accept
 *     everything that was not matched by any acl use <b>ok</b> or <b>allow</b>.
 *
 * - <b>deny-action</b>: as it name indicates, it instructs the server
 *     what is the deny action to be taken in the case a deny acl
 *     matches (either global or user level). You can use same values
 *     as <b>no-match-policy</b> <i>This <b>deny-action</b> applies to
 *     all acls defined in this users.xml backend, including global
 *     and users' acls. </i>
 *
 * Here is the list of actions:
 *
 * - <b>close</b>: if applied, causes the event connection to be closed.
 *
 * - <b>discard</b>: if applied, causes the PUBLISH, SUBSCRIBE and UNSUBSCRIBE operation to be silently discarded. This is the default value for <b>deny-action</b> if nothing is configured.
 *
 * - <b>deny</b>: alias for <b>discard</b>
 *
 * - <b>ok</b>: if applied, causes the given operation to be accepted. This is the default value for <b>no-match-policy</b> if nothing is configured.
 *
 * - <b>allow</b>: alias for <b>ok</b>
 *
 * Then, either inside a <b>&lt;global-acls></b> or inside <b>&lt;user></b> ---according to where you want the acl to be applied--- you declare the <b>&lt;acl></b> node with the following attributes:
 *
 * <ul>
 *   <li><i><b>mode</b></i> is the acl mode, which supports a list of comma separated values to allow:
 *     <ul>
 *       <li><b>r</b> : Allows unsubscribing, subscribing and receiving PUBLISH updates</li>
 *       <li><b>w</b> : Allows sending PUBLISH, including qos 0, 1, 2</li>
 *       <li><b>publish</b> : Alias for w</li>
 *       <li><b>subscribe</b> : Allows subscribing</li>
 *       <li><b>publish0</b> : Allows sending PUBLISH qos0 requests</li>
 *       <li><b>publish1</b> : Allows sending PUBLISH qos1 requests</li>
 *       <li><b>publish2</b> : Allows sending PUBLISH qos2 requests</li>
 *     </ul>
 *   <li><i><b>topic</b></i> the topic to allow by this acl, including MQTT topics wildcards.
 * </ul>
 *
 * 
 * 
 *
 */
