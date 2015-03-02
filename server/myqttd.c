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
#if defined(ENABLE_TERMIOS)
# include <termios.h>
# include <sys/stat.h>
# include <unistd.h>
# define MYQTTD_TERMINAL "/dev/tty"
#endif

#include <myqttd.h>


#if defined(AXL_OS_UNIX)
/* used by fchmod */
# include <sys/types.h>
# include <sys/stat.h>

#endif
#include <unistd.h>

/* local include */
#include <myqttd-ctx-private.h>

#if defined(__COMPILING_MYQTTD__) && defined(__GNUC__)
/* make happy gcc compiler */
int fsync (int fd);
#endif

/** 
 * \defgroup myqttd MyQttd: general facilities, initialization, etc
 */

/** 
 * \addtogroup myqttd
 * @{
 */

void __myqttd_thread_pool_conf (MyQttdCtx * ctx)
{
	int        max_limit   = 40;
	int        step_period = 5;
	int        step_add    = 1;
	int        value;

	/* get max limit for the pool */
	value = myqttd_config_get_number (ctx, "/myqtt/global-settings/thread-pool", "max-limit");
	if (value > 0)
		max_limit = value;

	/* get step period */
	value = myqttd_config_get_number (ctx, "/myqtt/global-settings/thread-pool", "step-period");
	if (value > 0)
		step_period = value;

	/* get step-add */
	value = myqttd_config_get_number (ctx, "/myqtt/global-settings/thread-pool", "step-add");
	if (value > 0)
		step_add = value;

	/* configure the pool */
	msg ("Setting thread pool to max-limit=%d, step-add=%d, step-period=%d", max_limit, step_add, step_period);
	myqtt_thread_pool_setup (ctx->myqtt_ctx, max_limit, step_add, step_period, axl_true);

	return;
}

/* configure here back log */
void __myqttd_server_backlog (MyQttdCtx * ctx)
{
	int        backlog     = 50;
	int        value;

	/* get max limit for the pool */
	value = myqttd_config_get_number (ctx, "/myqtt/global-settings/server-backlog", "value");
	if (value > 0)
		backlog = value;

	/* configure backlog */
	msg ("Configuring server TCP backlog: %d", backlog);
	myqtt_conf_set (ctx->myqtt_ctx, MYQTT_LISTENER_BACKLOG, backlog, NULL);
	return;
}

/* configure serveral limits */
void __myqttd_acquire_limits (MyQttdCtx * ctx)
{
	int        value;

	/* get global child limit */
	value = myqttd_config_get_number (ctx, "/myqtt/global-settings/global-child-limit", "value");
	if (value > 0)
		ctx->global_child_limit = value;
	else
		ctx->global_child_limit = 100;

	/* get global child limit */
	value = myqttd_config_get_number (ctx, "/myqtt/global-settings/max-incoming-complete-frame-limit", "value");
	if (value > 0)
		ctx->max_complete_flag_limit = value;
	else
		ctx->max_complete_flag_limit = 32768;

	return;
}

/** 
 * @brief Starts myqttd execution, initializing all libraries
 * required by the server application.
 *
 * A call to \ref myqttd_exit is required before exit.
 */
axl_bool  myqttd_init (MyQttdCtx   * ctx, 
		       MyQttCtx    * myqtt_ctx,
		       const char  * config)
{
	/* no initialization done if null reference received */
	if (ctx == NULL) {
	        abort_error ("Received a null myqttd context, failed to init the myqttd");
		return axl_false;
	} /* end if */
	if (config == NULL) {
		abort_error ("Received a null reference to the myqttd configuration, failed to init the myqttd");
		return axl_false;
	} /* end if */

	/* if a null value is received for the myqtt context, create
	 * a new empty one */
	if (myqtt_ctx == NULL) {
		msg2 ("creating a new myqtt context because a null value was received..");
		myqtt_ctx = myqtt_ctx_new ();
	} /* end if */

	/* check if the context is not started, and start it */
	if (! myqtt_init_check (myqtt_ctx)) {
		msg ("Calling to init MyQttCtx object (found to be uninitialized reference received");
		if (! myqtt_init_ctx (myqtt_ctx)) {
			abort_error ("Unable to initialize MyQtt context (for reference not initialized received)");
			return axl_false;
		} /* end if */
	} /* end if */

	/* get current process id */
	ctx->pid = getpid ();

	/* init myqttd internals */
	myqtt_mutex_create (&ctx->exit_mutex);

	/* configure the myqtt context created */
	myqttd_ctx_set_myqtt_ctx (ctx, myqtt_ctx);

	/* configure lookup domain for myqttd data */
	myqtt_support_add_domain_search_path_ref (myqtt_ctx, axl_strdup ("myqttd-data"), 
						   myqtt_support_build_filename (MYQTTD_DATADIR, "myqttd", NULL));
#if defined(AXL_OS_WIN32)
	/* make myqttd to add the path ../data to the search list
	 * under windows as it is organized this way */
	myqtt_support_add_domain_search_path     (myqtt_ctx, "myqttd-data", MYQTTD_DATADIR);
#endif
	myqtt_support_add_domain_search_path     (myqtt_ctx, "myqttd-data", ".");

	/* load current myqttd configuration */
	if (! myqttd_config_load (ctx, config)) {
		/* unable to load configuration */
		return axl_false;
	}

	/* configure here back log */
	__myqttd_server_backlog (ctx);

	/* init myqttd-mediator.c: init this module before others
	   to acchieve notifications and push events  */
	/* myqttd_mediator_init (ctx); */

	/*** not required to initialize axl library, already done by myqtt ***/
	msg ("myqttd internal init ctx: %p, myqtt ctx: %p", ctx, myqtt_ctx);

	/* db list */
	/* if (! myqttd_db_list_init (ctx)) {
		abort_error ("failed to init the myqttd db-list module");
		return axl_false;
		} */ /* end if */ 

	/* init myqttd-module.c */
	myqttd_module_init (ctx);

	/* init myqttd-proces.c: reinit=axl_false */
	myqttd_process_init (ctx, axl_false);

	/* configure thread pool here */
	__myqttd_thread_pool_conf (ctx);

	/* configure serveral limits */
	__myqttd_acquire_limits (ctx);

	/* init profile path module: this initialization must be done
	 * before calling to myqttd_run_config to avoid third
	 * party modules to install handler with higher priority. */
	/* if (! myqttd_ppath_init (ctx)) {
		return axl_false;
		} */ /* end if */

	/* init connection manager */
	myqttd_conn_mgr_init (ctx);

	/* init ok */
	return axl_true;
} /* end if */

/** 
 * @brief Function that performs a reload operation for the current
 * myqttd instance (represented by the provided MyQttdCtx).
 * 
 * @param ctx The myqttd context representing a running instance
 * that must reload.
 *
 * @param value The signal number caught. This value is optional since
 * reloading can be triggered not only by a signal received (i.e.
 * SIGHUP).
 */
void     myqttd_reload_config       (MyQttdCtx * ctx, int value)
{
	/* get myqttd context */
	int   already_notified = axl_false;
	
	msg ("caught HUP signal, reloading configuration");
	/* reconfigure signal received, notify myqttd modules the
	 * signal */
	myqtt_mutex_lock (&ctx->exit_mutex);
	if (already_notified) {
		myqtt_mutex_unlock (&ctx->exit_mutex);
		return;
	}
	already_notified = axl_true;

	/* call to reload logs */
	__myqttd_log_reopen (ctx);

	/* reload myqttd here, before modules
	 * reloading */
	/* myqttd_db_list_reload_module (); */
	
	/* reload modules */
	myqttd_module_notify_reload_conf (ctx);
	myqtt_mutex_unlock (&ctx->exit_mutex);

	return;
} 

/** 
 * @brief Performs all operations required to cleanup myqttd
 * runtime execution (calling to all module cleanups).
 */
void myqttd_exit (MyQttdCtx * ctx, 
		      axl_bool        free_ctx,
		      axl_bool        free_myqtt_ctx)
{
	MyQttCtx * myqtt_ctx;

	/* do not perform any change if a null context is received */
	v_return_if_fail (ctx);

	msg ("Finishing myqttd up (MyQttCtx: %p)..", ctx);

	/* check to kill childs */
	myqttd_process_kill_childs (ctx);

	/* terminate all modules */
	myqttd_config_cleanup (ctx);

	/* unref all connections (before calling to terminate myqtt) */
	myqttd_conn_mgr_cleanup (ctx);

	/* get the myqtt context assocaited */
	myqtt_ctx = myqttd_ctx_get_myqtt_ctx (ctx);

	/* terminate profile path */
	/* myqttd_ppath_cleanup (ctx); */

	/* close modules before terminating myqtt so they still can
	 * use the Myqtt API to terminate its function. */
	myqttd_module_notify_close (ctx);

	/* terminate myqttd db list module at this point to avoid
	 * modules referring to db-list to lost references */
	/* myqttd_db_list_cleanup (ctx); */
	
	/* terminate myqttd mediator module */
	/* myqttd_mediator_cleanup (ctx); */

	/* do not release the context (this is done by the caller) */
	myqttd_log_cleanup (ctx);

	/* release child if defined */
	myqttd_child_unref (ctx->child);

	/* terminate myqtt */
	msg ("now, terminate myqtt library after myqttd cleanup..");
	myqtt_exit_ctx (myqtt_ctx, free_myqtt_ctx);

	/* terminate run module */
	myqttd_run_cleanup (ctx);

	/* cleanup process module */
	myqttd_process_cleanup (ctx);

	/* termiante proxy loop (if started) */
	myqttd_loop_close (ctx->proxy_loop, axl_true);

	/* free mutex */
	myqtt_mutex_destroy (&ctx->exit_mutex);

	/* free ctx */
	if (free_ctx)
		myqttd_ctx_free (ctx);

	return;
}

/** 
 * @internal Simple macro to check if the console output is activated
 * or not.
 */
#define CONSOLE if (ctx->console_enabled || ignore_debug) fprintf

/** 
 * @internal Simple macro to check if the console output is activated
 * or not.
 */
#define CONSOLEV if (ctx->console_enabled || ignore_debug) vfprintf



/** 
 * @internal function that actually handles the console error.
 *
 * @param ignore_debug Allows to configure if the debug configuration
 * must be ignored (bypassed) and drop the log. This can be used to
 * perform logging for important messages.
 */
void myqttd_error (MyQttdCtx * ctx, axl_bool ignore_debug, const char * file, int line, const char * format, ...)
{
	/* get myqttd context */
	va_list            args;

	/* do not print if NULL is received */
	if (format == NULL || ctx == NULL)
		return;
	
	/* check extended console log */
	if (ctx->console_debug3 || ignore_debug) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stderr, "(proc:%d) [\e[1;31merr\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stderr, "(proc:%d) [err] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stderr, "\e[1;31mE: \e[0m");
		} else
#endif
			CONSOLE (stderr, "E: ");
	} /* end if */

	va_start (args, format);

	/* report to the console */
	CONSOLEV (stderr, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	myqttd_log_report (ctx, LOG_REPORT_ERROR, format, args, file, line);

	va_end (args);
	va_start (args, format);

	myqttd_log_report (ctx, LOG_REPORT_GENERAL, format, args, file, line);
	
	va_end (args);

	CONSOLE (stderr, "\n");
	
	fflush (stderr);
	
	return;
}

/** 
 * @brief Allows to check if the debug is activated (\ref msg type).
 * 
 * @return axl_true if activated, otherwise axl_false is returned.
 */
axl_bool  myqttd_log_enabled (MyQttdCtx * ctx)
{
	/* get myqttd context */
	v_return_val_if_fail (ctx, axl_false);

	return ctx->console_debug;
}

/** 
 * @brief Allows to activate the myqttd console log (by default
 * disabled).
 * 
 * @param ctx The myqttd context to configure.  @param value The
 * value to configure to enable/disable console log.
 */
void myqttd_log_enable       (MyQttdCtx * ctx, 
				  int  value)
{
	v_return_if_fail (ctx);

	/* configure the value */
	ctx->console_debug = value;

	/* update the global console activation */
	ctx->console_enabled     = ctx->console_debug || ctx->console_debug2 || ctx->console_debug3;

	return;
}




/** 
 * @brief Allows to check if the second level debug is activated (\ref
 * msg2 type).
 * 
 * @return axl_true if activated, otherwise axl_false is returned.
 */
axl_bool  myqttd_log2_enabled (MyQttdCtx * ctx)
{
	/* get myqttd context */
	v_return_val_if_fail (ctx, axl_false);
	
	return ctx->console_debug2;
}

/** 
 * @brief Allows to activate the second level console log. This level
 * of debug automatically activates the previous one. Once activated
 * it provides more information to the console.
 * 
 * @param ctx The myqttd context to configure.
 * @param value The value to configure.
 */
void myqttd_log2_enable      (MyQttdCtx * ctx,
				  int  value)
{
	v_return_if_fail (ctx);

	/* set the value */
	ctx->console_debug2 = value;

	/* update the global console activation */
	ctx->console_enabled     = ctx->console_debug || ctx->console_debug2 || ctx->console_debug3;

	/* makes implicit activations */
	if (ctx->console_debug2)
		ctx->console_debug = axl_true;

	return;
}

/** 
 * @brief Allows to check if the third level debug is activated (\ref
 * msg2 with additional information).
 * 
 * @return axl_true if activated, otherwise axl_false is returned.
 */
axl_bool  myqttd_log3_enabled (MyQttdCtx * ctx)
{
	/* get myqttd context */
	v_return_val_if_fail (ctx, axl_false);

	return ctx->console_debug3;
}

/** 
 * @brief Allows to activate the third level console log. This level
 * of debug automatically activates the previous one. Once activated
 * it provides more information to the console.
 * 
 * @param ctx The myqttd context to configure.
 * @param value The value to configure.
 */
void myqttd_log3_enable      (MyQttdCtx * ctx,
				  int  value)
{
	v_return_if_fail (ctx);

	/* set the value */
	ctx->console_debug3 = value;

	/* update the global console activation */
	ctx->console_enabled     = ctx->console_debug || ctx->console_debug2 || ctx->console_debug3;

	/* makes implicit activations */
	if (ctx->console_debug3)
		ctx->console_debug2 = axl_true;

	return;
}

/** 
 * @brief Allows to configure if the console log produced is colorfied
 * according to the status reported (red: (error,criticals), yellow:
 * (warning), green: (info, debug).
 * 
 * @param ctx The myqttd context to configure.
 *
 * @param value The value to configure. This function could take no
 * effect on system where ansi values are not available.
 */
void myqttd_color_log_enable (MyQttdCtx * ctx,
				  int             value)
{
	v_return_if_fail (ctx);

	/* configure the value */
	ctx->console_color_debug = value;

	return;
}

/** 
 * @internal function that actually handles the console msg.
 */
void myqttd_msg (MyQttdCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get myqttd context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;

	/* do not print if NULL is received */
	if (format == NULL || ctx == NULL)
		return;

	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;32mmsg\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [msg] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;32mI: \e[0m");
		} else
#endif
			CONSOLE (stdout, "I: ");
	} /* end if */
	
	va_start (args, format);
	
	/* report to console */
	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	myqttd_log_report (ctx, LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console access
 */
void  myqttd_access   (MyQttdCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get myqttd context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;

	/* do not print if NULL is received */
	if (format == NULL || ctx == NULL)
		return;

	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;32mmsg\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [msg] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;32mI: \e[0m");
		} else
#endif
			CONSOLE (stdout, "I: ");
	} /* end if */
	
	va_start (args, format);
	
	/* report to console */
	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	myqttd_log_report (ctx, LOG_REPORT_ACCESS, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console msg (second level debug)
 */
void myqttd_msg2 (MyQttdCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get myqttd context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;

	/* do not print if NULL is received */
	if (format == NULL || ctx == NULL)
		return;

	/* check second level debug */
	if (! ctx->console_debug2)
		return;

	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;32mmsg\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [msg] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;32mI: \e[0m");
		} else
#endif
			CONSOLE (stdout, "I: ");
	} /* end if */
	va_start (args, format);
	
	/* report to console */
	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	myqttd_log_report (ctx, LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console wrn.
 */
void myqttd_wrn (MyQttdCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get myqttd context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;

	/* do not print if NULL is received */
	if (format == NULL || ctx == NULL)
		return;
	
	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;33m!!!\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [!!!] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;33m!: \e[0m");
		} else
#endif
			CONSOLE (stdout, "!: ");
	} /* end if */
	
	va_start (args, format);

	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	myqttd_log_report (ctx, LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);
	va_start (args, format);

	myqttd_log_report (ctx, LOG_REPORT_WARNING, format, args, file, line);

	va_end (args);

	CONSOLE (stdout, "\n");
	
	fflush (stdout);
	
	return;
}

/** 
 * @internal function that actually handles the console wrn_sl.
 */
void myqttd_wrn_sl (MyQttdCtx * ctx, const char * file, int line, const char * format, ...)
{
	/* get myqttd context */
	va_list            args;
	axl_bool           ignore_debug = axl_false;

	/* do not print if NULL is received */
	if (format == NULL || ctx == NULL)
		return;
	
	/* check extended console log */
	if (ctx->console_debug3) {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "(proc:%d) [\e[1;33m!!!\e[0m] (%s:%d) ", ctx->pid, file, line);
		} else
#endif
			CONSOLE (stdout, "(proc:%d) [!!!] (%s:%d) ", ctx->pid, file, line);
	} else {
#if defined(AXL_OS_UNIX)	
		if (ctx->console_color_debug) {
			CONSOLE (stdout, "\e[1;33m!: \e[0m");
		} else
#endif
			CONSOLE (stdout, "!: ");
	} /* end if */
	
	va_start (args, format);

	CONSOLEV (stdout, format, args);

	va_end (args);
	va_start (args, format);

	/* report to log */
	myqttd_log_report (ctx, LOG_REPORT_ERROR | LOG_REPORT_GENERAL, format, args, file, line);

	va_end (args);

	fflush (stdout);
	
	return;
}

/** 
 * @brief Provides the same functionality like myqtt_support_file_test,
 * but allowing to provide the file path as a printf like argument.
 * 
 * @param format The path to be checked.
 * @param test The test to be performed. 
 * 
 * @return axl_true if all test returns axl_true. Otherwise axl_false
 * is returned. Note that if format is NULL, the function will always
 * return axl_false.
 */
axl_bool  myqttd_file_test_v (const char * format, MyQttFileTest test, ...)
{
	va_list   args;
	char    * path;
	int       result;

	if (format == NULL)
		return axl_false;

	/* open arguments */
	va_start (args, test);

	/* get the path */
	path = axl_strdup_printfv (format, args);

	/* close args */
	va_end (args);

	/* do the test */
	result = myqtt_support_file_test (path, test);
	axl_free (path);

	/* return the test */
	return result;
}


/** 
 * @brief Creates the directory with the path provided.
 * 
 * @param path The directory to create.
 * 
 * @return axl_true if the directory was created, otherwise axl_false is
 * returned.
 */
axl_bool   myqttd_create_dir  (const char * path)
{
	/* check the reference */
	if (path == NULL)
		return axl_false;
	
	/* create the directory */
#if defined(AXL_OS_WIN32)
	return (_mkdir (path) == 0);
#else
	return (mkdir (path, 0770) == 0);
#endif
}

/**
 * @brief Allows to remove the selected file pointed by the path
 * provided.
 *
 * @param path The path to the file to be removed.
 *
 * @return axl_true if the file was removed, otherwise axl_false is
 * returned.
 */ 
axl_bool myqttd_unlink              (const char * path)
{
	if (path == NULL)
		return axl_false;

	/* remove the file */
#if defined(AXL_OS_WIN32)
	return (_unlink (path) == 0);
#else
	return (unlink (path) == 0);
#endif
}

/** 
 * @brief Allows to get last modification date for the file provided.
 * 
 * @param file The file to check for its modification time.
 * 
 * @return The last modification time for the file provided. The
 * function return -1 if it fails.
 */
long myqttd_last_modification (const char * file)
{
	struct stat status;

	/* check file received */
	if (file == NULL)
		return -1;

	/* get stat */
	if (stat (file, &status) == 0)
		return (long) status.st_mtime;
	
	/* failed to get stats */
	return -1;
}

/** 
 * @brief Allows to check if the provided file path is a full path
 * (not releative). The function is meant to be portable.
 * 
 * @param file The file to check if it is a full path file name.
 * 
 * @return axl_true if the file is a full path, otherwise axl_false is
 * returned.
 */
axl_bool      myqttd_file_is_fullpath (const char * file)
{
	/* check the value received */
	if (file == NULL)
		return axl_false;
#if defined(AXL_OS_UNIX)
	if (file != NULL && (strlen (file) >= 1) && file[0] == '/')
		return axl_true;
#elif defined(AXL_OS_WIN32)
	if (file != NULL && (strlen (file) >= 3) && file[1] == ':' && (file[2] == '/' || file[2] == '\\'))
		return axl_true;
#endif	
	/* the file is not a full path */
	return axl_false;
}


/** 
 * @brief Allows to get the base dir associated to the provided path.
 * 
 * @param path The path that is required to return its base part.
 * 
 * @return Returns the base dir associated to the function. You
 * must deallocate the returning value with axl_free.
 */
char   * myqttd_base_dir            (const char * path)
{
	int    iterator;
	axl_return_val_if_fail (path, NULL);

	/* start with string length */
	iterator = strlen (path) - 1;

	/* lookup for the back-slash */
	while ((iterator >= 0) && 
	       ((path [iterator] != '/') && path [iterator] != '\\'))
		iterator--;

	/* check if the file provided doesn't have any base dir */
	if (iterator == -1) {
		/* return base dir for default location */
		return axl_strdup (".");
	}

	/* check the special case where the base path is / */
	if (iterator == 0 && path [0] == '/')
		iterator = 1;

	/* copy the base dir found */
	return axl_stream_strdup_n (path, iterator);
}

/** 
 * @brief Allows to get the file name associated to the provided path.
 * 
 * @param path The path that is required to return its base value.
 * 
 * @return Returns the base dir associated to the function. You msut
 * deallocate the returning value with axl_free.
 */
char   * myqttd_file_name           (const char * path)
{
	int    iterator;
	axl_return_val_if_fail (path, NULL);

	/* start with string length */
	iterator = strlen (path) - 1;

	/* lookup for the back-slash */
	while ((iterator >= 0) && ((path [iterator] != '/') && (path [iterator] != '\\')))
		iterator--;

	/* check if the file provided doesn't have any file part */
	if (iterator == -1) {
		/* return the an empty file part */
		return axl_strdup (path);
	}

	/* copy the base dir found */
	return axl_strdup (path + iterator + 1);
}

/*
 * @brief Allows to get the next line read from the user. The function
 * return an string allocated.
 * 
 * @return An string allocated or NULL if nothing was received.
 */
char * myqttd_io_get (char * prompt, MyqttdIoFlags flags)
{
#if defined(ENABLE_TERMIOS)
	struct termios current_set;
	struct termios new_set;
	int input;
	int output;

	/* buffer declaration */
	char   buffer[1024];
	char * result = NULL;
	int    iterator;
	
	/* try to read directly from the tty */
	if ((input = output = open (MYQTTD_TERMINAL, O_RDWR)) < 0) {
		/* if fails to open the terminal, use the standard
		 * input and standard error */
		input  = STDIN_FILENO;
		output = STDERR_FILENO;
	} /* end if */

	/* print the prompt if defined */
	if (prompt != NULL) {
		/* write the prompt */
		if (write (output, prompt, strlen (prompt)) == -1)
			fsync (output);
		else
			fsync (output);
	}

	/* check to disable echo */
	if (flags & DISABLE_STDIN_ECHO) {
		if (input != STDIN_FILENO && (tcgetattr (input, &current_set) == 0)) {
			/* copy to the new set */
			memcpy (&new_set, &current_set, sizeof (struct termios));
			
			/* configure new settings */
			new_set.c_lflag &= ~(ECHO | ECHONL);
			
			/* set this values to the current input */
			tcsetattr (input, TCSANOW, &new_set);
		} /* end if */
	} /* end if */

	iterator = 0;
	memset (buffer, 0, 1024);
	/* get the next character */
	while ((iterator < 1024) && (read (input, buffer + iterator, 1) == 1)) {
		
		if (buffer[iterator] == '\n') {
			/* remove trailing \n */
			buffer[iterator] = 0;
			result = axl_strdup (buffer);

			break;
		} /* end if */


		/* update the iterator */
		iterator++;
	} /* end while */

	/* return terminal settings if modified */
	if (flags & DISABLE_STDIN_ECHO) {
		if (input != STDIN_FILENO) {
			
			/* set this values to the current input */
			tcsetattr (input, TCSANOW, &current_set);

			/* close opened file descriptor */
			close (input);
		} /* end if */
	} /* end if */

	/* do not return anything from this point */
	return result;
#else
	return NULL;
#endif
}

/** 
 * @internal Allows to get current system path configured for the
 * provided path_name or NULL if it fails.
 *
 */
const char * __myqttd_system_path (MyQttdCtx * ctx, const char * path_name)
{
	axlNode * node;
	if (ctx == NULL || ctx->config == NULL)
		return NULL;
	/* get the first node <path> */
	node = axl_doc_get (ctx->config, "/myqtt/global-settings/system-paths/path");
	if (node == NULL)
		return NULL;
	while (node != NULL) {
		/* check if the node has the path name configuration
		   we are looking */
		if (HAS_ATTR_VALUE (node, "name", path_name))
			return ATTR_VALUE (node, "value");

		/* get next node */
		node = axl_node_get_next_called (node, "path");
	} /* end while */

	return NULL;
}

/** 
 * @brief Allows to get the SYSCONFDIR path provided at compilation
 * time. This is configured when the libmyqttd.{dll,so} is built,
 * ensuring all pieces uses the same SYSCONFDIR value. See also \ref
 * myqttd_datadir.
 *
 * The SYSCONFDIR points to the base root directory where all
 * configuration is found. Under unix system it is usually:
 * <b>/etc</b>. On windows system it is usually configured to:
 * <b>../etc</b>. Starting from that directory is found the rest of
 * configurations:
 * 
 *  - etc/myqtt/myqtt.conf
 *  - etc/myqtt/sasl/sasl.conf
 *
 * @param ctx The myqttd ctx with the associated configuration
 * where we are getting the sysconfdir. If NULL is provided, default
 * value is returned.
 *
 * @return The path currently configured by default or the value
 * overrided on the configuration.
 */
const char    * myqttd_sysconfdir     (MyQttdCtx * ctx)
{
	const char * path;

	/* return current configuration */
	path = __myqttd_system_path (ctx, "sysconfdir");
	if (path != NULL) 
		return path;
	return SYSCONFDIR;
	
}

/** 
 * @brief Allows to get the MYQTTD_DATADIR path provided at compilation
 * time. This is configured when the libmyqttd.{dll,so} is built,
 * ensuring all pieces uses the same data dir value. 
 *
 * The MYQTTD_DATADIR points to the base root directory where data files
 * are located (mostly dtd files). Under unix system it is usually:
 * <b>/usr/share/myqtt</b>. On windows system it is usually
 * configured to: <b>../data</b>.
 *
 * @param ctx The myqttd ctx with the associated configuration
 * where we are getting the sysconfdir. If NULL is provided, default
 * value is returned.
 *
 * @return The path currently configured by default or the value
 * overrided on the configuration.
 */
const char    * myqttd_datadir        (MyQttdCtx  * ctx)
{
	const char * path;

	/* return current configuration */
	path = __myqttd_system_path (ctx, "datadir");
	if (path != NULL) 
		return path;
	return MYQTTD_DATADIR;
}

/** 
 * @brief Allows to get the MYQTTD_RUNTIME_DATADIR path provided at
 * compilation time. This is configured when the
 * libmyqttd.{dll,so} is built, ensuring all pieces uses the same
 * runtime datadir value. 
 *
 * The MYQTTD_RUNTIME_DATADIR points to the base directory where runtime data files
 * are located (for example unix socket files). 
 * 
 * Under unix system it is usually: <b>/var/lib</b>. On windows system
 * it is usually configured to: <b>../run-time</b>. Inside that
 * directory is found a directory called "myqttd" where runtime
 * content is found.
 *
 * @param ctx The myqttd ctx with the associated configuration
 * where we are getting the sysconfdir. If NULL is provided, default
 * value is returned.
 *
 * @return The path currently configured by default or the value
 * overrided on the configuration.
 */
const char    * myqttd_runtime_datadir        (MyQttdCtx * ctx)
{
	const char * path;

	/* return current configuration */
	path = __myqttd_system_path (ctx, "runtime_datadir");
	if (path != NULL) 
		return path;
	return MYQTTD_RUNTIME_DATADIR;
}

/** 
 * @brief Allows to get current temporal directory.
 *
 * @param ctx The myqttd ctx where the configuration will be checked.
 *
 * @return The path to the temporal directory.
 */
const char    * myqttd_runtime_tmpdir  (MyQttdCtx * ctx)
{
#if defined(AXL_OS_UNIX)
	return "/tmp";
#elif defined(AXL_OS_WIN32)
	return "c:/windows/temp";
#endif
}


/** 
 * @brief Allows to check if the provided value contains a decimal
 * number.
 *
 * @param value The decimal number to be checked.
 *
 * @return axl_true in the case a decimal value is found otherwise
 * axl_false is returned.
 */
axl_bool  myqttd_is_num  (const char * value)
{
	int iterator = 0;
	while (iterator < value[iterator]) {
		/* check value on each position */
		if (! isdigit (value[iterator]))
			return axl_false;

		/* next position */
		iterator++;
	}

	/* is a number */
	return axl_true;
}

#if defined(AXL_OS_UNIX)

#define SYSTEM_ID_CONSUME_UNTIL_ZERO(line, fstab, delimiter)                       \
	if (fread (line + iterator, 1, 1, fstab) != 1 || line[iterator] == 0) {    \
	      fclose (fstab);                                                      \
	      return axl_false;                                                    \
	}                                                                          \
        if (line[iterator] == delimiter) {                                         \
	      line[iterator] = 0;                                                  \
	      break;                                                               \
	}                                                                          

axl_bool __myqttd_get_system_id_info (MyQttdCtx * ctx, const char * value, int * system_id, const char * path)
{
	FILE * fstab;
	char   line[512];
	int    iterator;

	/* set invalid value */
	if (system_id)
		(*system_id) = -1;

	fstab = fopen (path, "r");
	if (fstab == NULL) {
		error ("Failed to open file %s", path);
		return axl_false;
	}
	
	/* now read the file */
keep_on_reading:
	iterator = 0;
	do {
		SYSTEM_ID_CONSUME_UNTIL_ZERO (line, fstab, ':');

		/* next position */
		iterator++;
	} while (axl_true);
	
	/* check user found */
	if (! axl_cmp (line, value)) {
		/* consume all content until \n is found */
		iterator = 0;
		do {
			if (fread (line + iterator, 1, 1, fstab) != 1 || line[iterator] == 0) {
				fclose (fstab);
				return axl_false;
			}
			if (line[iterator] == '\n') {
				goto keep_on_reading;
				break;
			} /* end if */
		} while (axl_true);
	} /* end if */

	/* found user */
	iterator = 0;
	/* get :x: */
	if ((fread (line, 1, 2, fstab) != 2) || !axl_memcmp (line, "x:", 2)) {
		fclose (fstab);
		return axl_false;
	}
	
	/* now get the id */
	iterator = 0;
	do {
		SYSTEM_ID_CONSUME_UNTIL_ZERO (line, fstab, ':');

		/* next position */
		iterator++;
	} while (axl_true);

	(*system_id) = atoi (line);

	fclose (fstab);
	return axl_true;
}
#endif

/** 
 * @brief Allows to get system user id or system group id from the
 * provided string. 
 *
 * If the string already contains the user id or group id, the
 * function returns its corresponding integet value. The function also
 * checks if the value (that should represent a user or group in some
 * way) is present on the current system. get_user parameter controls
 * if the operation should perform a user lookup or a group lookup.
 * 
 * @param ctx The myqttd context.
 * @param value The user or group to get system id.
 * @param get_user axl_true to signal the value to lookup user, otherwise axl_false to lookup for groups.
 *
 * @return The function returns the user or group id or -1 if it fails.
 */
int myqttd_get_system_id  (MyQttdCtx * ctx, const char * value, axl_bool get_user)
{
#if defined (AXL_OS_UNIX)
	int system_id = -1;

	/* get user and group id associated to the value provided */
	if (! __myqttd_get_system_id_info (ctx, value, &system_id, get_user ? "/etc/passwd" : "/etc/group"))
		return -1;

	/* return the user id or group id */
	msg ("Resolved %s:%s to system id %d", get_user ? "user" : "group", value, system_id);
	return system_id;
#endif
	/* nothing defined */
	return -1;
}

/** 
 * @brief Allows to change the owner (user and group) of the socket's
 * file associated.
 *
 * @param ctx The Myqttd context.
 *
 * @param file_name  The file name path to change its owner.
 *
 * @param user The user string representing the user to change. If no
 * user required to change, just pass empty string or NULL.
 *
 * @param group The group string representing the group to change. If
 * no group required to change, just pass empty string or NULL.
 *
 * @return axl_true if the operation was completed, otherwise false is
 * returned.
 */
axl_bool        myqttd_change_fd_owner (MyQttdCtx * ctx,
					    const char    * file_name,
					    const char    * user,
					    const char    * group)
{
#if defined(AXL_OS_UNIX)
	/* call to change owners */
	return chown (file_name, 
		       /* get user */
		       myqttd_get_system_id (ctx, user, axl_true), 
		       /* get group */
		       myqttd_get_system_id (ctx, group, axl_false)) == 0;
#endif
	return axl_true;
}

#if !defined(fchmod)
int fchmod(int fildes, mode_t mode);
#endif

/** 
 * @brief Allows to change the permissions of the socket's file associated.
 *
 * @param ctx The Myqttd context.
 *
 * @param file_name The file name path to change perms.
 *
 * @param mode The mode to configure.
 */ 
axl_bool        myqttd_change_fd_perms (MyQttdCtx * ctx,
					    const char    * file_name,
					    const char    * mode)
{
#if defined(AXL_OS_UNIX)
	mode_t value = strtol (mode, NULL, 8);
	/* call to change mode */
	return chmod (file_name, value) == 0;
#endif
	return axl_true;
}

/** 
 * @brief Implements a portable subsecond thread sleep operation. The
 * caller will be blocked during the provide period.
 *
 * @param ctx The context used during the operation.
 *
 * @param microseconds Amount of time to wait.
 *
 * 
 */
void            myqttd_sleep           (MyQttdCtx * ctx,
					    long            microseconds)
{
#if defined(AXL_OS_UNIX)
	struct timeval timeout;

	timeout.tv_sec  = 0;
	timeout.tv_usec = microseconds;
	
	/* get current start time */
	select (0, 0, 0, 0, &timeout);
	
#elif defined(AXL_OS_WIN32)
#error "Add support for win32"
#endif
	return;
}

/* @} */

/** 
 * \mainpage 
 *
 * \section intro Myqttd Introduction
 *
 * Myqttd is a general MQTT server server. This means you focus on
 * adding features to your server side application, letting Myqttd to
 * help you with (to name some of them):
 *
 * - The profile security (\ref profile_path_configuration "by using profile path"), that is, to ensure your
 *      profile is used in the exact combination sequence required.
 *
 * - To enable application authentication (\ref myqttd_mod_sasl "by using SASL profiles"), that
 *     is, you can use already tested and integrated SASL framework to
 *     add auth to your server side applications (including site particular sasl configuration, MySQL support...).
 *
 * - To secure connection transmissions in a transparent way to your
 *     application (\ref myqttd_mod_tls "by using TLS profile").
 *
 * - The \ref myqttd_execution_model "execution model" required by
 *     your application, that is, you can choose to handle your
 *     incoming requests with a single new process each time (like
 *     HTTP servers), or handling a set of logically related
 *     connections by the same child process, allowing security
 *     configurations like chroot, changing executing user and group,
 *     etc.
 *
 * Myqttd is written on top of MyQtt Library and it is extended
 * by modules written in C or python (for now) and can be accessed by
 * available MQTT toolkits.
 *
 * \section documentation Myqttd Documentation
 *
 * Myqttd documentation is separated into two sections:
 * administrators manuals (used by people that want to deploy and
 * maintain Myqttd and its applications) and the developer manual
 * which includes information on how to extend Myqttd:
 *
 * - \ref myqttd_administrator_manual
 * - \ref myqttd_developer_manual
 *
 * <h2>Contact us</h2>
 *
 * You can reach us at the Myqtt mailing list: at <a
 * href="http://lists.aspl.es/cgi-bin/mailman/listinfo/myqtt">myqtt
 * users</a> for any question and patches.
 *
 * If you are interested in getting commercial support, you can
 * contact us at: <a href="mailto:info@aspl.es?subject=Myqttd+Myqtt support">info@aspl.es</a>. For more information see:
 * http://www.aspl.es/myqtt/commercial.html
 */

/** 
 * \page myqttd_administrator_manual Myqttd Administrator manual
 *
 * <b>Section 1: Installation notes</b>
 *
 *   - \ref installing_myqttd 
 *
 * <b>Section 2: Myqttd configuration</b>
 *
 *   - \ref configuring_myqttd
 *   - \ref myqttd_config_location
 *   - \ref myqttd_ports
 *   - \ref myqttd_smtp_notifications
 *   - \ref myqttd_configuring_log_files
 *   - \ref myqttd_db_list_management "2.7 Myqttd Db-List management"
 *   - \ref myqttd_configure_system_paths
 *   - \ref myqttd_configure_splitting
 *
 * <b>Section 3: MQTT management</b>
 *
 *   - \ref profile_path_configuration
 *   - \ref profile_path_flags_supported_by_allow_and_if_sucess
 *   - \ref profile_path_expressions_examples
 *   - \ref myqttd_execution_model
 *   - \ref myqttd_starting_without_profiles
 *   - \ref myqttd_profile_path_search
 *
 * <b>Section 4: Myqttd module management</b> 
 *
 *   - \ref myqttd_modules_configuration
 *   - \ref myqttd_modules_filtering
 *   - \ref myqttd_modules_activation
 *   - \ref myqttd_mod_sasl   "4.4 mod-sasl: SASL support for Myqttd (auth services)"
 *   - \ref myqttd_mod_python "4.6 mod-python: python language support for Myqttd"
 *   - \ref myqttd_mod_tls    "4.7 mod-tls: TLS support for Myqttd (secure connections)"
 *   - \ref myqttd_mod_radmin "4.8 mod-radmin: support for query runtime status"
 *
 * \section installing_myqttd 1.1 Installing Myqttd
 *
 * <b>Myqttd dependencies</b>
 * 
 * Myqttd have the following required dependencies:
 *
 * <ol>
 *  <li> <b>Myqtt Library 1.1</b>: which provides the MQTT engine (located at: http://www.aspl.es/myqtt).</li>
 *
 *  <li> <b>Axl Library:</b> which provides all XML services (located at: http://www.aspl.es/xml).</li>
 * </ol>
 *
 * The following optional dependencies are not required to built
 * Myqttd, but provides really useful features.
 *
 * <ol> 
 *
 * <li><b>Libpcre3:</b> a library that provides perl expressions
 * support. It is used by all expressions used by the myqttd
 * configuration file. If not provided all expressions provided will
 * be matched as is.
 *
 * This regular expression library was written by Philip Hazel, and
 * copyrighted by the University of Cambridge, England.
 *
 * The software is available at:
 * ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre </li>
 *
 *  <li><b>OpenSSL:</b> a library that provides TLS support. If not
 *  provided mod-tls won't be available. 
 *
 *  The software is available at: http://www.openssl.org </li>
 *  
 * <li><b>GNU SASL:</b> a library that provides SASL support. If not
 * provided  won't be available.
 *
 * The software is available at: ftp://alpha.gnu.org/pub/gnu/gsasl </li>
 *
 *</ol>
 *
 * In many cases, GNU SASL, Libpcre3 and OpenSSL are already available
 * on your platform. Check your distribution documentation.  
 *
 *
 * <h3>Building Myqttd from source on POSIX / Unix environments</h3>
 * 
 * It is assumed you have a platform that have autoconf, libtool, and
 * the rest of GNU tools to build software.
 *
 * Get the source code at the download page: http://www.aspl.es/myqtt/downloads.html
 * 
 * Myqttd is compiled following the standard autoconf-compatible
 * procedure:
 *
 * \code
 * >> tar xzvf myqttd-X.X.X-bXXXX.gXXXX.tar.gz
 * >> cd myqttd-X.X.X-bXXXX.gXXXX
 * >> ./configure
 * >> make
 * >> make install
 * \endcode
 *
 * <h3>Once finished, next step</h3>
 *
 * Now you must configure your Myqttd installation. Check the
 * following section.
 * 
 * \section configuring_myqttd 2.1 Myqttd configuration
 * 
 * \section myqttd_config_location 2.2 How myqttd is configured (configuration file location)
 *   
 * Myqttd is configured through XML 1.0 files. The intention is
 * provide an easy and extensible way to configure myqttd,
 * allowing third parties to build tools to update/reconfigure it. It
 * is also provided a DTD to ensure that configuration file created is
 * properly constructed at a minimum syntax level.
 * 
 * According to your installation, the main myqttd configuration
 * file should be found at the location provided by the following
 * command: 
 *
 * \include myqttd-config-location.txt
 * 
 * Alternatively you can provide your own configuration file by using
 * the <b>--config</b> option: 
 *
 * \code
 *  >> myqttd --conf my-myqttd.conf
 * \endcode
 *
 * Myqttd main configuration file includes several global
 * sections: 
 *
 * <ol>
 *  <li><b>&lt;global-settings></b>: This main section includes several
 *  run time configuration for the MQTT server: TCP ports, listener
 *  address, log files, crash handling, etc.
 *  </li>
 *
 *  <li>
 *    <b>&lt;modules></b>: Under this section is mainly configured
 * directories holding modules.
 *  </li>
 *
 * <li><b>&lt;profile-path-configuration></b>: Under this section is
 *  provided the profile path configuration, an administrative
 *  configuration which allows to mix, sequence, and select profiles
 *  to be provided to client peers according to several run time
 *  values such remote address, profiles already initiated, etc..
 *  </li>
 *
 * </ol>
 *
 * \section myqttd_ports 2.3 Myqttd addresses and ports used
 *
 * Ports and addresses used by Myqttd to listen are configured at
 * the <b>&lt;global-settings></b> section. Here is an example:
 * 
 * \htmlinclude global-settings.xml-tmp
 *
 * Previous example will make Myqttd to listen on ports 3206 and
 * 44010 for all addresses that are known for the server hosting
 * myqttd (0.0.0.0). Myqttd will understand this section
 * listening on all addresses provided, for all ports.
 *
 * Alternatively, during development or when it is found a myqttd
 * bug, it is handy to configure the default action to take on server
 * crash (bad signal received). This is done by configuring the
 * following:
 *
 * \htmlinclude on-bad-signal.xml-tmp
 *
 * The <b>"hold"</b> option is really useful for real time debugging
 * because it hangs right after the signal is received. This allows to
 * launch the debugger and attach to the process to see what's
 * happening. 
 *
 * Another useful option is <b>"backtrace"</b> which produces a
 * backtrace report (of the current process) saving it into a
 * file. This allows to save some error evidences and then let the
 * process to finish. This option combined with mail-to attribute is a
 * powerful debug option.
 *
 * Inside production environment it is recommended <b>"ignore"</b>.
 *
 * \section myqttd_smtp_notifications 2.4 Receiving SMTP notification on failures and error conditions
 *
 * Myqttd includes a small SMTP client that allows to report
 * critical or interesting conditions. For example, this is used to
 * report backtraces on critical signal received. 
 *
 * This configuration is found and declarted at the <b><global-settings></b>
 * section. Here is an example:
 *
 * \htmlinclude notify-failures.xml-tmp
 *
 * It is possible to have more than one <b><smtp-server></b>
 * declared. They are later used/referenced either through <b>id</b>
 * attribute or because the declaration is flagged with an
 * <b>is-default=yes</b>. 
 *
 * \section myqttd_configuring_log_files 2.5 Configuring myqttd log files
 *
 * Myqttd logs is sent to a set of files that are configured at
 * the <b>&lt;global-settings></b> section:
 *
 * \htmlinclude log-reporting.xml-tmp
 *
 * These files hold logs for general information
 *  (<b>&lt;general-log></b>), error information
 *  (<b>&lt;error-log></b>), client peer access information
 *  (<b>&lt;access-log></b>) and myqtt engine log produced by its
 *  execution (<b>&lt;myqtt-log></b>).
 *
 * Apart from the myqtt log (<b>&lt;myqtt-log></b>) the rest of
 * files contains the information that is produced by all calls done
 * to the following library functions: <b>msg</b>, <b>msg2</b>,
 * <b>wrn</b> and <b>error</b>. 
 *
 * By default, Myqttd server is started with no console
 * output. All log is sent to previous log files. The second
 * destination available is the console output. 
 *
 * Four command line options controls logs produced to the console by
 * Myqttd and tools associated:
 *
 * <ol>
 *  
 * <li><p><b>--debug</b>: activates the console log, showing main
 * messages. Myqttd tools have this option implicitly activated. </p></li>
 *
 * <li><p> <b>--debug2</b>: activates implicitly the <b>--debug</b>
 * option and shows previous messages plus new messages that are
 * considered to have more details.</p></li>
 *
 * <li><p><b>--debug3</b>: makes log output activated to include more
 * details at the place it was launched (file and line), process pid,
 * etc.</p></li>
 *
 * <li><p><b>--color-debug</b>: whenever previous options are activated, if
 * this one is used, the console log is colored according to the
 * kind of message reported: green (messages), red (error messages),
 * yellow (warning messages).</p></li>
 *
 * </ol>
 * 
 * If previous options are used Myqttd will register a log into
 * the appropriate file but also will send the same log to the
 * console.
 *
 * In the case you want to handle all logs through syslog, just
 * declare as follows. Note that once syslog is enabled, general-log,
 * error-log, access-log and myqtt-log declarations will be ignored,
 * making all information to be sent to syslog.
 *
 * \htmlinclude log-reporting.syslog.xml-tmp
 *
 * \section myqttd_configure_system_paths 2.7 Alter default myqttd base system paths
 *
 * By default Myqttd has 3 built-in system paths used to locate
 * configuration files (<b>sysconfdir</b>), find data files (<b>datadir</b>) and directories used at run
 * time (<b>runtime datadir</b>) to implement internal functions.
 *
 * To show default values configured on your myqttd use:
 * \code
 * >> myqttd --conf-location
 * \endcode
 *
 * However, these three system paths can also be overrided by a
 * configuration placed at the global section. Here is an example:
 *
 * \htmlinclude override-system-paths.xml-tmp
 *
 * Currently, accepted system paths are:
 *
 * - <b>sysconfdir</b>: base dir where myqttd.conf file will be located (${sysconfdir}/myqtt/myqtt.conf).
 * - <b>datadir</b>: base dir where static myqttd data files are located (${datadir}/myqtt).
 * - <b>runtime_datadir</b>: base directory where run time files are created (${runtime_datadir}/myqtt).
 *
 * Additionally many modules inside Myqttd and Myqtt Library find
 * configuration and data files by call to
 * myqtt_support_domain_find_data_file. That function works by
 * finding the provided file under a particular search domain. Each
 * module has it own search domain. To add new search elements into a
 * particular domain to make your files available to each particular
 * module then use the syntax found in the example above.
 *
 * \section myqttd_configure_splitting 2.8 Splitting myqttd configuration
 *
 * As we saw, myqttd has a main configuration file which is
 * <b>myqttd.conf</b>. However, it is possible to split this file
 * into several files or even directories with additional files.
 *
 * In the case you want to move some configuration into a separate
 * file, just place the following:
 * 
 * \htmlinclude include-from-file.xml-tmp
 *
 * Then the <b>&lt;include /></b> node will be replaced with the content found inside <b>file-with-config.conf</b>. 
 *
 * <b>NOTE: </b> even having this feature, the resuling
 * myqttd.conf file after replacing all <b>includes</b> must be a
 * properly formated myqttd.conf file.
 *
 * It is also possible to load a set of configuration files from a
 * directory. This is useful for profile paths where all of them are
 * stored separated into /etc/myqtt/profile.d directory. That's
 * why the following declaration inside <profile-path-configuration>:
 *
 * \htmlinclude include-from-dir.xml-tmp
 *
 * Previous declaration import all content from files found in
 * <b>/etc/myqtt/profile.d</b> replacing the <b>include</b> node.
 *
 * \section profile_path_expressions_examples 3.3 Profile path configuration: expression examples
 *
 * For the case an expression is required to match source or
 * destination the following expressions area available:
 *
 * - <b>192.168.0.*</b> allows to match everything that matches the network IP part 192.168.0.X.
 *
 * - <b>not 192.168.0.*</b> allows to match everything that isn't the network IP part 192.168.0.X.
 *
 * - <b>192.168.1.132, 192.168.1.134</b> allows to match a list of ips.
 *
 * - <b>192.168.1.0, 192.168.0.*</b> allows to match a list of wilcard ips.
 *
 * - <b>not 192.168.1.0, 192.168.0.*</b> allows to inverse the previous match.
 *
 * \section myqttd_execution_model 3.4 Myqttd execution model (process security)
 *
 * It is posible to configure Myqttd, through profile path
 * configuration, to handle connections in the same master process or
 * using child processes. Here is a detailed list:
 *
 * - <b>Single master process: </b> by default if no <b>separate=yes</b> flag
 *     is used at any profile path then all connections will be
 *     handled by the same single process.
 *
 * - <b>Handling at independent childs: </b> if <b>separate=yes</b> is
 *     configured on a profile path, once a connection matches it, a
 *     child process is created to handle it. In this context it is
 *     possible to configure running user (uid) or chroot to improve
 *     application separation.
 *
 * - <b>Handling at same childs: </b> because creating one child for
 *     each connection received may be resource expensive or it is required to share the same context (process) across connections to the same profile path, <b>reuse=yes</b>
 *     flag is provided so connections matching same profile path are
 *     handled by the same child process. 
 *
 * By default, when Myqttd is stopped, all created childs are
 * killed. This is configured with <b><kill-childs-on-exit value="yes" /></b>
 * inside <global-settings> node.
 *
 * \section myqttd_starting_without_profiles 3.5 Making myqttd to start without profiles defined
 *
 * By default Myqttd checks after module start up (init method) if
 * there are at least one profile to serve. It is found that no
 * profile is still defined, Myqttd will refuse to continue. This
 * is done to prevent starting Myqttd with a wrong configuration. 
 *
 * To control or change this behaviour check
 * <b><allow-start-without-profiles value="yes or no" /></b> inside
 * <global-settings> node.
 *
 * \section myqttd_profile_path_search 3.6 Declaring additional search paths inside profile paths
 *
 * Some modules, like mod-sasl, mod-python, etc, may use a search path
 * to find configuration files. This search path uses default values
 * (profile path working directory or global system configuration
 * directory) but it is possible to declare alternative paths for
 * those files by using <b>&lt;search></b> node declarations inside
 * profile path.
 *
 * Here is an example:
 *
 * \htmlinclude search-path-example.xml-tmp
 *
 * The detail about what domain and what files are found with those
 * declarations is module especific. Check the particular module
 * documentation to find out how to use this.
 *
 * \section myqttd_modules_configuration 4.1 Myqttd modules configuration
 * 
 * Modules loaded by myqttd are found at the directories
 * configured in the <b>&lt;modules></b> section. Here is an
 * example:
 *
 * \htmlinclude tbc-modules.xml-tmp
 * 
 * Every directory configured contains myqttd xml module pointers
 * having the following content: 
 *
 * \htmlinclude module-conf.xml-tmp
 * 
 * Each module have its own configuration file, which should use XML
 * as default configuration format. 
 *
 * \section myqttd_modules_filtering 4.2 Myqttd module filtering
 *
 * It is possible to configure Myqttd to skip some module so it is
 * not loaded. This is done by adding a <b><no-load /></b> declaration
 * with the set of modules to be skipped. This is done inside <b><modules /></b> section:
 *
 * \htmlinclude module-skip.xml-tmp
 *
 * \section myqttd_modules_activation 4.3 Enable a myqttd module
 *
 * To enable a myqttd module, just make the module pointer file to be
 * available in one of the directories listed inside <b>&lt;modules></b>. This is
 * usually done as follows:
 * <ul>
 *
 *   <li>On Windows: just copy the module pointer .xml file into the mods-enabled and restart myqttd.</li>
 *
 *   <li>On Unix: link the module pointer .xml file like this
 *   (assuming you want to enable mod-sasl and mods available and
 *   enabled folders are located at /etc/myqtt):
 *
 *    \code
 *    >> ln -s /etc/myqtt/mods-enabled/mod-sasl.xml /etc/myqtt/mods-available/mod-sasl.xml
 *    \endcode
 *   ..and restart myqttd.
 *   </li>
 * </ul>
 * 
 *   
 *
 */

/** 
 * \page myqttd_developer_manual Myqttd Developer manual
 *
 * <b>Section 1: Creating myqttd modules (C language)</b>
 *
 *   - \ref myqttd_developer_manual_creating_modules
 *   - \ref myqttd_developer_manual_creating_modules_manually
 *   - \ref myqttd_developer_manual_using_tbc_mod_gen
 *
 * <b>Section 2: Creating python apps (mod-python enabled)</b>
 *
 *  - \ref  myqttd_mod_python_writing_apps
 *
 * <b>Section 3: Myqtt 1.1 API</b>
 *
 *  Because Myqttd extends and it is built on top of Myqtt
 *  Library 1.1, it is required to keep in mind and use Myqtt
 *  API. Here is the reference:
 *
 *  - <a class="el" href="http://fact.aspl.es/files/af-arch/myqtt-1.1/html/index.html">Myqtt Library 1.1 Documentation Center</a>
 *
 * <b>Section 4: Myqttd API</b>
 *
 *  The following is the API exposed to myqttd modules and
 *  tools. This is only useful for myqttd developers.
 *
 *  - \ref myqttd
 *  - \ref myqttd_moddef
 *  - \ref myqttd_config
 *  - \ref myqttd_db_list
 *  - \ref myqttd_conn_mgr
 *  - \ref myqttd_handlers
 *  - \ref myqttd_types
 *  - \ref myqttd_ctx
 *  - \ref myqttd_run
 *  - \ref myqttd_expr
 *  - \ref myqttd_loop
 *  - \ref myqttd_mediator
 *  - \ref myqttd_module
 *  - \ref myqttd_ppath
 *  - \ref myqttd_support
 *
 * \section myqttd_developer_manual_creating_modules How Myqttd module works
 *
 * Myqttd is a listener application built on top of <a
 * href="http://www.aspl.es/myqtt">Myqtt Library</a>, which reads a
 * set of \ref configuring_myqttd "configuration files" to start
 * at some selected ports, etc, and then load all modules installed.
 *
 * These modules could implement new MQTT features that extend MyQttd
 * internal function.
 *
 * Myqttd core is really small. The rest of features are added as
 * modules. For example, Myqttd SASL support is a module which is
 * configurable to use a particular user database and, with the help
 * of some tools (<b>tbc-sasl-conf</b>), you can manage users that are allowed.
 *
 * In fact, Myqttd know anything about SASL because this is delegated to mod-sasl. \ref
 * myqttd_mod_sasl "Myqttd SASL module" installs and
 * configures SASL profiles provided by Myqtt, and using \ref
 * profile_path_configuration "Profile Path" (a Myqttd core
 * feature), the security provisioning is meet.
 *
 * Myqttd module form is fairly simple. It contains the following handlers (defined at \ref MyqttdModDef):
 * <ol>
 *
 *  <li>Init (\ref ModInitFunc): A handler called by Myqttd to start the module. Here
 *  the developer must place all calls required to install/configure a
 *  profile, init global variables, etc.</li>
 *
 *  <li>Close (\ref ModCloseFunc): Called by Myqttd to stop a module. Here the
 *  developer must stop and dealloc all resources used by its
 *  module.</li>
 *
 *  <li>Reconf (\ref ModReconfFunc): Called by Myqttd when a HUP signal is
 *  received. This is a notification that the module should reload its
 *  configuration files and start to behave as they propose.</li>
 *
 *  <li>Profile path selected (\ref ModPPathSelected): Called by Myqttd when the profile path for a connection was selected.</li>
 *
 * </ol>
 *
 * \section myqttd_developer_manual_creating_modules_manually Creating a module from the scratch (dirty way)
 *
 * Maybe the easiest way to start writing a Myqttd Module is to
 * take a look into mod-test source code. This module does anything
 * but is maintained across releases to contain all handlers required
 * and a brief help. You can use it as an official reference. A module
 * is at minimum composed by the following tree files:
 *
 * - <b>mod-test.c</b>: base module source code: \ref myqttd_mod_test_c "mod-test.c" | <a href="https://dolphin.aspl.es/svn/publico/af-arch/trunk/myqtt/modules/mod-test/mod-test.c"><b>[TXT]</b></a>
 * - <b>Makefile.am</b>: optional automake file used to build the module: <a href="https://dolphin.aspl.es/svn/publico/af-arch/trunk/myqtt/modules/mod-test/Makefile.am"><b>[TXT]</b></a>
 * - <b>mod-test.xml.in</b>: xml module pointer, a file that is installed at the Myqttd modules dir to load the module: <a href="https://dolphin.aspl.es/svn/publico/af-arch/trunk/myqtt/modules/mod-test/mod-test.xml.in"><b>[TXT]</b></a>
 *
 * \section myqttd_developer_manual_using_tbc_mod_gen Using tbc-mod-gen to create the module (recommended)
 *
 * This tool allows to create a XML template that is used to produce
 * the module output. Here is an example:
 *
 * First we create a xml empty module template:
 *
 * \code
 * >> mkdir template
 * >> cd template
 * >> tbc-mod-gen --template --enable-autoconf --out-dir .
 * I: producing a template definition at the location provided
 * I: creating file:             ./template.xml
 * I: template created: OK
 * \endcode
 *
 * Now you should rename the file <b>template.xml</b> to something more
 * appropriate and edit the template content, changing the module name
 * and its description. Do not change the content of init, close and
 * reconf nodes for now:
 *
 * \htmlinclude template.xml-tmp
 *
 * Now, do the following to compile the content and produce a module
 * that is compatible with the Myqttd interface, and it is full
 * ready to be compiled and installed:
 *
 * \code 
 *  >> tbc-mod-gen --compile template.xml --out-dir . --enable-autoconf
 *  I: creating file:             ./mod_template.c
 *  I: creating file:             ./Makefile.am
 *  I: found autoconf support files request..
 *  I: creating file:             ./autogen.sh
 *  I: making executable:         ./autogen.sh
 *  I: creating file:             ./configure.ac
 *  I: creating file:             ./gen-code
 *  I: making executable:         ./gen-code
 *  I: mod_template created!
 * \endcode
 *
 * Now take a look into the files created, specially
 * <b>mod_template.c</b>. Once you are ready, type the following to build the
 * module:
 *
 * \code
 *  >> ./autogen.sh
 *  >> make
 *  >> make install
 * \endcode
 *
 * If you are new to autotools, you have to now that the first command
 * (autogen.sh) is only execute once. Next times you can run
 * <b>./configure</b> which have the same effect and run faster. The
 * <b>autogen.sh</b> command is executed to bootstrap the project, adding all
 * missing files.
 *
 * Now you are ready to complete the module with your particular
 * code. For that, you'll have to use the Myqttd API and specially
 * the Myqtt API. <a href="http://www.aspl.es/myqtt/doc.html">See document section for more details.</a>
 */

/** 
 * \page myqttd_mod_test_c mod-test.c source code
 *
 * You can copy and paste the following code to start a myqttd
 * module. This code is checked (through compilation) against
 * Myqttd source code.
 *
 * \include mod-test.c
 */


