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

#include <main-common.h>
#include <myqttd-ctx-private.h>

/** 
 * @internal Common function used by myqttd server and logger
 * process to check and enable debug options.
 */ 
void main_common_enable_debug_options (MyQttdCtx * ctx, 
				       MyQttCtx  * myqtt_ctx)
{
	/* configure context debug according to values received */
	myqttd_log_enable  (ctx, exarg_is_defined ("debug"));
	myqttd_log2_enable (ctx, exarg_is_defined ("debug2"));
	myqttd_log3_enable (ctx, exarg_is_defined ("debug3"));

	/* enable myqtt debug: do this at this place because
	 * turbulece_init makes a call to myqtt_init */
	myqtt_log_enable  (myqtt_ctx, exarg_is_defined ("myqtt-debug"));
	myqtt_log2_enable (myqtt_ctx, exarg_is_defined ("myqtt-debug2"));
	if (exarg_is_defined ("myqtt-debug-color")) {
		myqtt_log_enable       (myqtt_ctx, axl_true);
		myqtt_color_log_enable (myqtt_ctx, axl_true);
	} /* end if */

	/* check console color debug */
	myqttd_color_log_enable (ctx, exarg_is_defined ("color-debug"));

	/* enable --debug option if it is found to be defined
	 * --color-debug */
	if (exarg_is_defined ("color-debug"))
		myqttd_log_enable (ctx, axl_true);

	/* configure no unmap module if defined */
	if (exarg_is_defined ("no-unmap-modules")) {
		msg ("Setting no unmap modules = True");
		myqttd_module_set_no_unmap_modules (axl_true);
	}

	/* configure no unmap module if defined */
	msg ("Setting wait for pool threads = %s", exarg_is_defined ("wait-thread-pool") ? "True" : "False");
	myqtt_conf_set (myqtt_ctx, MYQTT_SKIP_THREAD_POOL_WAIT, ! exarg_is_defined ("wait-thread-pool"), NULL);

	/* configure child cmd prefix */
	if (exarg_is_defined ("child-cmd-prefix")) {
		msg ("Setting child cmd prefix: %s", exarg_get_string ("child-cmd-prefix"));
		myqttd_process_set_child_cmd_prefix (exarg_get_string ("child-cmd-prefix"));
	}

	return;
}

/**  
 * @internal Common function used by myqttd server and logger
 * process to get configuration file location.
 */ 
char * main_common_get_config_location (MyQttdCtx * ctx, 
					MyQttCtx  * myqtt_ctx)
{
	char * config;

	/* init the myqtt support module to allow finding the
	 * configuration file */
	myqtt_support_init (myqtt_ctx);

	/* configure lookup domain, and load configuration file */
	myqtt_support_add_domain_search_path_ref (myqtt_ctx, axl_strdup ("myqtt-conf"), 
						   myqtt_support_build_filename (SYSCONFDIR, "myqtt", NULL));
	myqtt_support_add_domain_search_path     (myqtt_ctx, "myqtt-conf", ".");

	/* find the configuration file */
	if (exarg_is_defined ("config")) {
		/* get the configuration defined at the command line */
		config = axl_strdup (exarg_get_string ("config"));
	} else {
		/* get the default configuration defined at
		 * compilation time */
		config = myqtt_support_domain_find_data_file (myqtt_ctx, "myqtt-conf", "myqtt.conf");
	} /* end if */

	/* load main turb */
	if (config == NULL) {
		abort_error ("Unable to find myqtt.conf file at the default location: %s/myqtt/myqtt.conf", SYSCONFDIR);
		return NULL;
	} else 
		msg ("using configuration file: %s", config);
	return config;
}
