/*  Turbulence BEEP application server
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
 *  turbulence based solutions, etc, contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº10, Edificio Alius A, Despacho 102
 *         Alcala de Henares, 28802 (MADRID)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/turbulence
 */
#include <main-common.h>
#include <turbulence-ctx-private.h>

/** 
 * @internal Common function used by turbulence server and logger
 * process to check and enable debug options.
 */ 
void main_common_enable_debug_options (TurbulenceCtx * ctx, 
				       VortexCtx     * vortex_ctx)
{
	/* configure context debug according to values received */
	turbulence_log_enable  (ctx, exarg_is_defined ("debug"));
	turbulence_log2_enable (ctx, exarg_is_defined ("debug2"));
	turbulence_log3_enable (ctx, exarg_is_defined ("debug3"));

	/* enable vortex debug: do this at this place because
	 * turbulece_init makes a call to vortex_init */
	vortex_log_enable  (vortex_ctx, exarg_is_defined ("vortex-debug"));
	vortex_log2_enable (vortex_ctx, exarg_is_defined ("vortex-debug2"));
	if (exarg_is_defined ("vortex-debug-color")) {
		vortex_log_enable       (vortex_ctx, axl_true);
		vortex_color_log_enable (vortex_ctx, axl_true);
	} /* end if */

	/* check console color debug */
	turbulence_color_log_enable (ctx, exarg_is_defined ("color-debug"));

	/* enable --debug option if it is found to be defined
	 * --color-debug */
	if (exarg_is_defined ("color-debug"))
		turbulence_log_enable (ctx, axl_true);

	/* configure no unmap module if defined */
	if (exarg_is_defined ("no-unmap-modules")) {
		msg ("Setting no unmap modules = True");
		turbulence_module_set_no_unmap_modules (axl_true);
	}

	/* configure no unmap module if defined */
	msg ("Setting wait for pool threads = %s", exarg_is_defined ("wait-thread-pool") ? "True" : "False");
	vortex_conf_set (vortex_ctx, VORTEX_SKIP_THREAD_POOL_WAIT, ! exarg_is_defined ("wait-thread-pool"), NULL);

	/* configure child cmd prefix */
	if (exarg_is_defined ("child-cmd-prefix")) {
		msg ("Setting child cmd prefix: %s", exarg_get_string ("child-cmd-prefix"));
		turbulence_process_set_child_cmd_prefix (exarg_get_string ("child-cmd-prefix"));
	}

	return;
}

/**  
 * @internal Common function used by turbulence server and logger
 * process to get configuration file location.
 */ 
char * main_common_get_config_location (TurbulenceCtx * ctx, 
					VortexCtx     * vortex_ctx)
{
	char * config;

	/* init the vortex support module to allow finding the
	 * configuration file */
	vortex_support_init (vortex_ctx);

	/* configure lookup domain, and load configuration file */
	vortex_support_add_domain_search_path_ref (vortex_ctx, axl_strdup ("turbulence-conf"), 
						   vortex_support_build_filename (SYSCONFDIR, "turbulence", NULL));
	vortex_support_add_domain_search_path     (vortex_ctx, "turbulence-conf", ".");

	/* find the configuration file */
	if (exarg_is_defined ("config")) {
		/* get the configuration defined at the command line */
		config = axl_strdup (exarg_get_string ("config"));
	} else {
		/* get the default configuration defined at
		 * compilation time */
		config = vortex_support_domain_find_data_file (vortex_ctx, "turbulence-conf", "turbulence.conf");
	} /* end if */

	/* load main turb */
	if (config == NULL) {
		abort_error ("Unable to find turbulence.conf file at the default location: %s/turbulence/turbulence.conf", SYSCONFDIR);
		return NULL;
	} else 
		msg ("using configuration file: %s", config);
	return config;
}
