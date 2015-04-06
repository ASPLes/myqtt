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

#include <myqttd.h>
#include <stdlib.h>
#include <syslog.h>

/* local include */
#include <myqttd-ctx-private.h>

/** 
 * @brief Init the myqttd log module.
 */
void myqttd_log_init (MyQttdCtx * ctx)
{
	/* get current myqttd configuration */
	axlDoc  * doc = myqttd_config_get (ctx);
	axlNode * node;

	/* check log reporting */
	node = axl_doc_get (doc, "/myqtt/global-settings/log-reporting");
	if (node == NULL) {
		abort_error ("Unable to find log configuration <myqttd/global-settings/log-reporting>");
		return;
	} /* end if */

	/* check enabled attribute */
	if (! HAS_ATTR (node, "enabled")) {
		abort_error ("Missing attribute 'enabled' located at <myqttd/global-settings/log-reporting>. Unable to determine if log is enabled");
		return;
	}

	/* check if log reporting is enabled or not */
	if (! HAS_ATTR_VALUE (node, "enabled", "yes")) {
		msg ("log reporting to file disabled");
		return;
	}

	/* check for syslog usage */
	ctx->use_syslog = HAS_ATTR_VALUE (node, "use-syslog", "yes");
	msg ("Checking for usage of syslog %d", ctx->use_syslog);
	if (ctx->use_syslog) {
		/* open syslog */
		openlog ("myqttd", LOG_PID, LOG_DAEMON);
		msg ("Using syslog facility for logging");
		return;
	} /* end if */

	/* open all logs */
	node      = axl_node_get_child_called (node, "general-log");
	/* check permission access */

	ctx->general_log = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (ctx->general_log == -1) {
		abort_error ("unable to open general log: %s", ATTR_VALUE (node, "file"));
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	/* open error logs */
	node      = axl_node_get_child_called (node, "error-log");
	ctx->error_log = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (ctx->error_log == -1) {
		abort_error ("unable to open error log: %s", ATTR_VALUE (node, "file"));
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	/* open access log */
	node      = axl_node_get_child_called (node, "access-log");
	ctx->access_log  = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (ctx->access_log == -1) {
		abort_error ("unable to open access log: %s", ATTR_VALUE (node, "file"));
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);

	node      = axl_node_get_child_called (node, "myqtt-log");
	ctx->myqtt_log  = open (ATTR_VALUE (node, "file"), O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (ctx->myqtt_log == -1) {
		abort_error ("unable to open myqtt log: %s", ATTR_VALUE (node, "file"));
	} else {
		msg ("opened log: %s", ATTR_VALUE (node, "file"));
	} /* end if */
	node      = axl_node_get_parent (node);
	
	return;
}

/** 
 * @brief Allows to configure the provided file descriptor to receive
 * logs produced in the context of type parameter. The configuration
 * is perfomed on the context provided.
 *
 * @param ctx MyQttd context to be configured.
 * @param type The log context to be reconfigured.
 * @param descriptor The descriptor to be configured.
 */
void      myqttd_log_configure (MyQttdCtx * ctx,
				    LogReportType   type,
				    int             descriptor)
{
	v_return_if_fail (ctx);
	switch (type) {
	case LOG_REPORT_GENERAL:
		/* configure new general log descriptor */
		ctx->general_log = descriptor;
		break;
	case LOG_REPORT_ERROR:
	case LOG_REPORT_WARNING:
		/* configure new error log descriptor */
		ctx->error_log = descriptor;
		break;
	case LOG_REPORT_ACCESS:
		/* configure new access log descriptor */
		ctx->access_log = descriptor;
		break;
	case LOG_REPORT_MYQTT:
		/* configure new myqtt log descriptor */
		ctx->myqtt_log = descriptor;
		break;
	} /* end switch */
	return;
}


axl_bool __myqttd_log_manager_transfer_content (MyQttdLoop * loop, 
						    MyQttdCtx  * ctx,
						    int              descriptor,
						    axlPointer       ptr,
						    axlPointer       ptr2)
{
	int     size;
	int     size_written;
	char    buffer[4097];
	int     output_sink = PTR_TO_INT (ptr);

	switch (output_sink) {
	case LOG_REPORT_GENERAL:
		output_sink = ctx->general_log;
		break;
	case LOG_REPORT_ERROR:
	case LOG_REPORT_WARNING:
		output_sink = ctx->error_log;
		break;
	case LOG_REPORT_ACCESS:
		output_sink = ctx->access_log;
		break;
	case LOG_REPORT_MYQTT:
		output_sink = ctx->myqtt_log;
		break;
	default:
		/* send to default output sink */
		output_sink = ctx->general_log;
	} /* end switch */

	/* read content */
	size = read (descriptor, buffer, 4096);
			
	/* check closed socket (child process finished) */
	if (size <= 0) 
		return axl_false;
			
	/* transfer content to the associated socket */
	buffer[size] = 0;
	size_written = write (output_sink, buffer, size);
	if (size_written != size) {
		error ("failed to write log received from child, content differs (%d != %d), error was: %s", 
		       size, size_written,
		       myqtt_errno_get_last_error ());
	} /* end if */

	return axl_true;
}

/** 
 * @internal This function allows to start the log manager that will
 * control file descriptors to logs opened and will control pipes to
 * child process to write content received.
 */
void myqttd_log_manager_start (MyQttdCtx * ctx)
{
	/* check if log is enabled */
	if (! myqttd_log_is_enabled (ctx)) {
		msg ("myqttd log disabled");
		return;
	}

	/* skip starting log manager if we are using syslog */
	if (ctx->use_syslog)
		return;

	/* crear manager */
	ctx->log_manager = myqttd_loop_create (ctx);

	msg ("log manager started");
	return;
}

/** 
 * @internal Function used to register new descriptors that are
 * connected to local logs making all content received from this
 * descriptor to be redirected to the local file designated by type.
 */
void      myqttd_log_manager_register (MyQttdCtx * ctx,
					   LogReportType   type,
					   int             descriptor)
{
	v_return_if_fail (ctx);

	/* create log descriptor linking the descriptor received to
	 * the current descriptor managing the context signaled by
	 * type */
	msg ("register fd: %d (type: %d)", descriptor, type);
	switch (type) {
	case LOG_REPORT_GENERAL:
		/* configure general log watcher */
		myqttd_loop_watch_descriptor (ctx->log_manager,
						  descriptor,
						  __myqttd_log_manager_transfer_content,
						  INT_TO_PTR (LOG_REPORT_GENERAL),
						  NULL);
		break;
	case LOG_REPORT_ERROR:
	case LOG_REPORT_WARNING:
		/* configure error log watcher */
		myqttd_loop_watch_descriptor (ctx->log_manager,
						  descriptor,
						  __myqttd_log_manager_transfer_content,
						  INT_TO_PTR (LOG_REPORT_ERROR),
						  NULL);
		break;
	case LOG_REPORT_ACCESS:
		/* configure access log watcher */
		myqttd_loop_watch_descriptor (ctx->log_manager,
						  descriptor,
						  __myqttd_log_manager_transfer_content,
						  INT_TO_PTR(LOG_REPORT_ACCESS),
						  NULL);
		break;
	case LOG_REPORT_MYQTT:
		/* configure myqtt log watcher */
		myqttd_loop_watch_descriptor (ctx->log_manager,
						  descriptor,
						  __myqttd_log_manager_transfer_content,
						  INT_TO_PTR(LOG_REPORT_MYQTT),
						  NULL);
		break;
	} /* end switch */
	return;
}


/** 
 * @internal macro that allows to report a message to the particular
 * log, appending date information.
 */
void REPORT (axl_bool use_syslog, LogReportType type, int log, const char * message, va_list args, const char * file, int line) 
{
	/* get myqttd context */
	time_t             time_val;
	char             * time_str;
	char             * string;
	char             * string2;
	char             * result;
	int                length;
	int                length2;
	int                total;

	if (use_syslog) {
		string = axl_strdup_printfv (message, args);
		if (string == NULL)
			return;
		if (type == LOG_REPORT_ERROR) {
			syslog (LOG_ERR, "**ERROR**: %s", string);
		} else if (type == LOG_REPORT_WARNING) {
			syslog (LOG_INFO, "warning: %s", string);
		} else if (type == LOG_REPORT_ACCESS) {
			syslog (LOG_INFO, "access: %s", string);
		} else if (type == LOG_REPORT_MYQTT) {
			syslog (LOG_INFO, "myqtt: %s", string);
		} else {
 		        syslog (LOG_INFO, "info: %s", string);
		}
		axl_free (string);
		return;
	} /* end if */

	/* do not report if log description is not defined */
	if (log < 0)
		return;

	/* create timestamp */
	time_val = time (NULL);
	time_str = axl_strdup (ctime (&time_val));
	if (time_str == NULL)
		return;
	time_str [strlen (time_str) - 1] = 0;

	/* write stamp */
	string = axl_strdup_printf ("%s [%d] (%s:%d) ", time_str, getpid (), file, line);
	axl_free (time_str);
	if (string == NULL)
		return;
	length = strlen (string);

	/* create message */
	string2 = axl_strdup_printfv (message, args);
	if (string2 == NULL) {
		axl_free (string);
		return;
	}

	length2 = strlen (string2);

	/* build final log message */
	total  = length + length2 + 1;
	result = axl_new (char, total + 1);
	if (result == NULL) {
		axl_free (string);
		axl_free (string2);
		return;
	}

	memcpy (result, string, length);
	memcpy (result + length, string2, length2);
	memcpy (result + length + length2, "\n", 1);

	axl_free (string);
	axl_free (string2);
	
	/* write content: do it in a single operation to avoid mixing
	 * content from different logs at the log file. */
	if (write (log, result, total) == -1) {
		axl_free (result);
		return;
	}
	
	/* release memory used */
	axl_free (result);
	return;
} 

/** 
 * @brief Reports a single line to the particular log, configured by
 * "type".
 * 
 * @param type The log to select for reporting. The function do not
 * support reporting at the same call to several targets. You must
 * call one time for each target to report.
 *
 * @param message The message to report.
 */
void myqttd_log_report (MyQttdCtx   * ctx,
			    LogReportType     type, 
			    const char      * message, 
			    va_list           args,
			    const char      * file,
			    int               line)
{
	/* according to the type received report */
	if ((type & LOG_REPORT_GENERAL) == LOG_REPORT_GENERAL) 
		REPORT (ctx->use_syslog, LOG_REPORT_GENERAL, ctx->general_log, message, args, file, line);
	
	/* handle error and warning through the same log file */
	if ((type & LOG_REPORT_ERROR) == LOG_REPORT_ERROR) 
		REPORT (ctx->use_syslog, LOG_REPORT_ERROR, ctx->error_log, message, args, file, line);
	if ((type & LOG_REPORT_WARNING) == LOG_REPORT_WARNING) 
		REPORT (ctx->use_syslog, LOG_REPORT_WARNING, ctx->error_log, message, args, file, line);
	
	if ((type & LOG_REPORT_ACCESS) == LOG_REPORT_ACCESS) 
		REPORT (ctx->use_syslog, LOG_REPORT_ACCESS, ctx->access_log, message, args, file, line);

	if ((type & LOG_REPORT_MYQTT) == LOG_REPORT_MYQTT) {
		REPORT (ctx->use_syslog, LOG_REPORT_MYQTT, ctx->myqtt_log, message, args, file, line);
	}
	return;
}

/** 
 * @brief Allows to check if the log to file is enabled on the
 * provided context.
 *
 * @param ctx The context where file log is checked to be enabled or
 * not.
 */
axl_bool   myqttd_log_is_enabled    (MyQttdCtx * ctx)
{
	axlDoc  * config;
	axlNode * node;

	/* check context received */
	if (ctx == NULL)
		return axl_false;
	
	/* get configuration */
	config = myqttd_config_get (ctx);
	node   = axl_doc_get (config, "/myqtt/global-settings/log-reporting");

	/* check value returned */
	return myqttd_config_is_attr_positive (ctx, node, "enabled");
}

void __myqttd_log_close (MyQttdCtx * ctx)
{

	/* check if we are running with syslog support */
	if (ctx->use_syslog) {
		closelog ();
		return;
	}

	/* close the general log */
	if (ctx->general_log >= 0)
		close (ctx->general_log);
	ctx->general_log = -1;

	/* close the error log */
	if (ctx->error_log >= 0)
		close (ctx->error_log);
	ctx->error_log = -1;

	/* close the access log */
	if (ctx->access_log >= 0)
		close (ctx->access_log);
	ctx->access_log = -1;

	/* close myqtt log */
	if (ctx->myqtt_log >= 0)
		close (ctx->myqtt_log);
	ctx->myqtt_log = -1;
	return;
}

void __myqttd_log_reopen (MyQttdCtx * ctx)
{
	msg ("Reload received, reopening log references..");

	/* call to close all logs opened at this moment */
	__myqttd_log_close (ctx);

	/* call to open again */
	myqttd_log_init (ctx);

	msg ("Log reopening finished..");

	return;
}

/** 
 * @internal
 * @brief Stops and dealloc all resources hold by the module.
 */
void myqttd_log_cleanup (MyQttdCtx * ctx)
{
	/* call to close current logs */
	__myqttd_log_close (ctx);

	/* now finish log manager */
	myqttd_loop_close (ctx->log_manager, axl_true);
	ctx->log_manager = NULL;

	return;
}


