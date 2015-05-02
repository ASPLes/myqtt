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

/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

MyQttdCtx * ctx = NULL;
axlDoc    * conf = NULL;

/** MyQttdUsersUnloadDb **/
void __mod_status_unload (MyQttdCtx * ctx, 
			      axlPointer  _backend)
{
	return;
}

/** 
 * @brief Init function, perform all the necessary code to register
 * profiles, configure MyQtt, and any other init task. The function
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
	config = myqtt_support_domain_find_data_file (MYQTTD_MYQTT_CTX (_ctx), "status", "status.conf");
	if (config == NULL) {
		error ("Unable to find ssl.conf file under expected locations, failed to activate mod-Status support (try checking %s/myqtt/status/status.conf)",
		       myqttd_sysconfdir (ctx));
		return axl_false;
	} /* end if */

	/* try to load configuration */
	conf = axl_doc_parse_from_file (config, &err);
	if (conf == NULL) {
		error ("Unable to load configuration from %s, axl_doc_parse_from_file failed: %s",
		       config, axl_error_get (err));
		axl_free (config);
		axl_error_free (err);
		return axl_false;
	} /* end if */
	axl_free (config);

	return axl_true;
}

/** 
 * @brief Close function called once the myqttd server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void mod_status_close (MyQttdCtx * ctx)
{
	axlDoc * doc = conf;
	conf = NULL;
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
