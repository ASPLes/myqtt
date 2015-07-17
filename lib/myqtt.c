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

/** 
 * \defgroup myqtt MyQtt Init: init, core and common functions
 */

/** 
 * \addtogroup myqtt
 * @{
 */

/* global includes */
#include <myqtt.h>
#include <signal.h>

/* private include */
#include <myqtt-ctx-private.h>

#define LOG_DOMAIN "myqtt"

/* Ugly hack to have access to vsnprintf function (secure form of
 * vsprintf where the output buffer is limited) but unfortunately is
 * not available in ANSI C. This is only required when compile myqtt
 * with log support */
#if defined(ENABLE_MYQTT_LOG)
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

#if !defined(AXL_OS_WIN32)
void __myqtt_sigpipe_do_nothing (int _signal)
{
	/* do nothing sigpipe handler to be able to manage EPIPE error
	 * returned by write. read calls do not fails because we use
	 * the myqtt reader process that is waiting for changes over
	 * a connection and that changes include remote peer
	 * closing. So, EPIPE (or receive SIGPIPE) can't happen. */
	

	/* the following line is to ensure ancient glibc version that
	 * restores to the default handler once the signal handling is
	 * executed. */
	signal (SIGPIPE, __myqtt_sigpipe_do_nothing);
	return;
}
#endif

/** 
 * @brief Allows to get current status for log debug info to console.
 * 
 * @param ctx The context that is required to return its current log
 * activation configuration.
 * 
 * @return axl_true if console debug is enabled. Otherwise axl_false.
 */
axl_bool      myqtt_log_is_enabled (MyQttCtx * ctx)
{
#ifdef ENABLE_MYQTT_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return axl_false;

	/* check if the debug function was already checked */
	if (! ctx->debug_checked) {
		/* not checked, get the value and flag as checked */
		ctx->debug         = myqtt_support_getenv_int ("MYQTT_DEBUG") > 0;
		ctx->debug_checked = axl_true;
	} /* end if */

	/* return current status */
	return ctx->debug;

#else
	return axl_false;
#endif
}

/** 
 * @brief Allows to get current status for second level log debug info
 * to console.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return axl_true if console debug is enabled. Otherwise axl_false.
 */
axl_bool      myqtt_log2_is_enabled (MyQttCtx * ctx)
{
#ifdef ENABLE_MYQTT_LOG	

	/* no context, no log */
	if (ctx == NULL)
		return axl_false;

	/* check if the debug function was already checked */
	if (! ctx->debug2_checked) {
		/* not checked, get the value and flag as checked */
		ctx->debug2         = myqtt_support_getenv_int ("MYQTT_DEBUG2") > 0;
		ctx->debug2_checked = axl_true;
	} /* end if */

	/* return current status */
	return ctx->debug2;

#else
	return axl_false;
#endif
}

/** 
 * @brief Enable console myqtt log.
 *
 * You can also enable log by setting MYQTT_DEBUG environment
 * variable to 1.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param status axl_true enable log, axl_false disables it.
 */
void     myqtt_log_enable       (MyQttCtx * ctx, axl_bool      status)
{
#ifdef ENABLE_MYQTT_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return;

	ctx->debug         = status;
	ctx->debug_checked = axl_true;
	return;
#else
	/* just return */
	return;
#endif
}

/** 
 * @brief Enable console second level myqtt log.
 * 
 * You can also enable log by setting MYQTT_DEBUG2 environment
 * variable to 1.
 *
 * Activating second level debug also requires to call to \ref
 * myqtt_log_enable (axl_true). In practical terms \ref
 * myqtt_log_enable (axl_false) disables all log reporting even
 * having \ref myqtt_log2_enable (axl_true) enabled.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param status axl_true enable log, axl_false disables it.
 */
void     myqtt_log2_enable       (MyQttCtx * ctx, axl_bool      status)
{
#ifdef ENABLE_MYQTT_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return;

	ctx->debug2 = status;
	ctx->debug2_checked = axl_true;
	return;
#else
	/* just return */
	return;
#endif
}


/** 
 * @brief Allows to check if the color log is currently enabled.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return axl_true if enabled, axl_false if not.
 */
axl_bool      myqtt_color_log_is_enabled (MyQttCtx * ctx)
{
#ifdef ENABLE_MYQTT_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return axl_false;
	if (! ctx->debug_color_checked) {
		ctx->debug_color_checked = axl_true;
		ctx->debug_color         = myqtt_support_getenv_int ("MYQTT_DEBUG_COLOR") > 0;
	} /* end if */

	/* return current result */
	return ctx->debug_color;
#else
	/* always return axl_false */
	return axl_false;
#endif
	
}


/** 
 * @brief Enable console color log. 
 *
 * Note that this doesn't enable logging, just selects color messages if
 * logging is already enabled, see myqtt_log_enable().
 *
 * This is mainly useful to hunt messages using its color: 
 *  - red:  errors, critical 
 *  - yellow: warnings
 *  - green: info, debug
 *
 * You can also enable color log by setting MYQTT_DEBUG_COLOR
 * environment variable to 1.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param status axl_true enable color log, axl_false disables it.
 */
void     myqtt_color_log_enable (MyQttCtx * ctx, axl_bool      status)
{

#ifdef ENABLE_MYQTT_LOG
	/* no context, no log */
	if (ctx == NULL)
		return;
	ctx->debug_color_checked = status;
	ctx->debug_color = status;
	return;
#else
	return;
#endif
}

/** 
 * @brief Allows to configure which levels will be filtered from log
 * output. This can be useful to only show debug, warning or critical
 * messages (or any mix).
 *
 * For example, to only show critical, pass filter_string = "debug,
 * warning". To show warnings and criticals, pass filter_string =
 * "debug".
 *
 * To disable any filtering, use filter_string = NULL.
 *
 * @param ctx The myqtt context that will be configured with a log filter.
 *
 * @param filter_string The filter string to be used. You can separate
 * allowed values as you wish. Allowed filter items are: debug,
 * warning, critical.
 *
 */
void     myqtt_log_filter_level (MyQttCtx * ctx, const char * filter_string)
{
	v_return_if_fail (ctx);

	/* set that debug filter was configured */
	ctx->debug_filter_checked = axl_true;

	/* enable all levels */
	if (filter_string == NULL) {
		ctx->debug_filter_is_enabled = axl_false;
		return;
	} /* end if */

	/* add each filter bit */
	if (strstr (filter_string, "debug"))
		ctx->debug_filter |= MYQTT_LEVEL_DEBUG;
	if (strstr (filter_string, "warning"))
		ctx->debug_filter |= MYQTT_LEVEL_WARNING;
	if (strstr (filter_string, "critical"))
		ctx->debug_filter |= MYQTT_LEVEL_CRITICAL;

	/* set as enabled */
	ctx->debug_filter_is_enabled = axl_true;
	return;
}

/** 
 * @brief Allows to check if current MYQTT_DEBUG_FILTER is enabled. 
 * @param ctx The context where the check will be implemented.
 *
 * @return axl_true if log filtering is enabled, otherwise axl_false
 * is returned.
 */ 
axl_bool    myqtt_log_filter_is_enabled (MyQttCtx * ctx)
{
	char * value;
	v_return_val_if_fail (ctx, axl_false);
	if (! ctx->debug_filter_checked) {
		/* set as checked */
		ctx->debug_filter_checked = axl_true;
		value = myqtt_support_getenv ("MYQTT_DEBUG_FILTER");
		myqtt_log_filter_level (ctx, value);
		axl_free (value);
	}

	/* return current status */
	return ctx->debug_filter_is_enabled;
}

/** 
 * @brief Allows to get a myqtt configuration, providing a valid
 * myqtt item.
 * 
 * The function requires the configuration item that is required and a
 * valid reference to a variable to store the result. 
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param item The configuration item that is being returned.
 *
 * @param value The variable reference required to fill the result.
 * 
 * @return The function returns axl_true if the configuration item is
 * returned. 
 */
axl_bool       myqtt_conf_get             (MyQttCtx      * ctx,
					    MyQttConfItem   item, 
					    int            * value)
{
#if defined(AXL_OS_WIN32)

#elif defined(AXL_OS_UNIX)
	/* variables for nix world */
	struct rlimit _limit;
#endif	
	/* do common check */
	v_return_val_if_fail (ctx,   axl_false);
	v_return_val_if_fail (value, axl_false);

	/* no context, no configuration */
	if (ctx == NULL)
		return axl_false;

	/* clear value received */
	*value = 0;

#if defined (AXL_OS_WIN32)
#elif defined(AXL_OS_UNIX)
	/* clear not filled result */
	_limit.rlim_cur = 0;
	_limit.rlim_max = 0;	
#endif

	switch (item) {
	case MYQTT_SOFT_SOCK_LIMIT:
#if defined (AXL_OS_WIN32)
		/* return the soft sock limit */
		*value = ctx->__myqtt_conf_soft_sock_limit;
		return axl_true;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "failed to get current soft limit: %s", myqtt_errno_get_last_error ());
			return axl_false;
		} /* end if */

		/* return current limit */
		*value = _limit.rlim_cur;
		return axl_true;
#endif		
	case MYQTT_HARD_SOCK_LIMIT:
#if defined (AXL_OS_WIN32)
		/* return the hard sockt limit */
		*value = ctx->__myqtt_conf_hard_sock_limit;
		return axl_true;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "failed to get current soft limit: %s", myqtt_errno_get_last_error ());
			return axl_false;
		} /* end if */

		/* return current limit */
		*value = _limit.rlim_max;
		return axl_true;
#endif		
	case MYQTT_LISTENER_BACKLOG:
		/* return current backlog value */
		*value = ctx->backlog;
		return axl_true;
	case MYQTT_SKIP_THREAD_POOL_WAIT:
		*value = ctx->skip_thread_pool_wait;
		return axl_true;
	default:
		/* configuration found, return axl_false */
		myqtt_log (MYQTT_LEVEL_CRITICAL, "found a requested for a non existent configuration item");
		return axl_false;
	} /* end if */

	return axl_true;
}

/** 
 * @brief Allows to configure the provided item, with either the
 * integer or the string value, according to the item configuration
 * documentation.
 * 
 * @param ctx The context where the configuration will take place.
 *
 * @param item The item configuration to be set.
 *
 * @param value The integer value to be configured if applies.
 *
 * @param str_value The string value to be configured if applies.
 * 
 * @return axl_true if the configuration was done properly, otherwise
 * axl_false is returned.
 */
axl_bool       myqtt_conf_set             (MyQttCtx      * ctx,
					    MyQttConfItem   item, 
					    int              value, 
					    const char     * str_value)
{
#if defined(AXL_OS_WIN32)

#elif defined(AXL_OS_UNIX)
	/* variables for nix world */
	struct rlimit _limit;
#endif	
	/* do common check */
	v_return_val_if_fail (ctx,   axl_false);
	v_return_val_if_fail (value, axl_false);

#if defined (AXL_OS_WIN32)
#elif defined(AXL_OS_UNIX)
	/* clear not filled result */
	_limit.rlim_cur = 0;
	_limit.rlim_max = 0;	
#endif

	switch (item) {
	case MYQTT_SOFT_SOCK_LIMIT:
#if defined (AXL_OS_WIN32)
		/* check soft limit received */
		if (value > ctx->__myqtt_conf_hard_sock_limit)
			return axl_false;

		/* configure new soft limit */
		ctx->__myqtt_conf_soft_sock_limit = value;
		return axl_true;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "failed to get current soft limit: %s", myqtt_errno_get_last_error ());
			return axl_false;
		} /* end if */

		/* configure new value */
		_limit.rlim_cur = value;

		/* set new limit */
		if (setrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "failed to set current soft limit: %s", myqtt_errno_get_last_error ());
			return axl_false;
		} /* end if */

		return axl_true;
#endif		
	case MYQTT_HARD_SOCK_LIMIT:
#if defined (AXL_OS_WIN32)
		/* current it is not possible to configure hard sock
		 * limit */
		return axl_false;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "failed to get current soft limit: %s", myqtt_errno_get_last_error ());
			return axl_false;
		} /* end if */

		/* configure new value */
		_limit.rlim_max = value;
		
		/* if the hard limit gets lower than the soft limit,
		 * make the lower limit to be equal to the hard
		 * one. */
		if (_limit.rlim_max < _limit.rlim_cur)
			_limit.rlim_max = _limit.rlim_cur;

		/* set new limit */
		if (setrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "failed to set current hard limit: %s", myqtt_errno_get_last_error ());
			return axl_false;
		} /* end if */
		
		return axl_true;
#endif		
	case MYQTT_LISTENER_BACKLOG:
		/* return current backlog value */
		ctx->backlog = value;
		return axl_true;
	case MYQTT_SKIP_THREAD_POOL_WAIT:
		ctx->skip_thread_pool_wait = value;
		return axl_true;
	default:
		/* configuration found, return axl_false */
		myqtt_log (MYQTT_LEVEL_CRITICAL, "found a requested for a non existent configuration item");
		return axl_false;
	} /* end if */

	return axl_true;
}

/** 
 * @brief If activate the console debug, it may happen that some
 * messages are mixed because several threads are producing them at
 * the same time.
 *
 * This function allows to make all messages logged to acquire a mutex
 * before continue. It is by default disabled because it has the
 * following considerations:
 * 
 * - Acquiring a mutex allow to clearly separate messages from
 * different threads, but has a great perform penalty (only if log is
 * activated).
 *
 * - Acquiring a mutex could make the overall system to act in a
 * different way because the threading is now globally synchronized by
 * all calls done to the log. That is, it may be possible to not
 * reproduce a thread race condition if the log is activated with
 * mutex acquire.
 * 
 * @param ctx The context that is going to be configured.
 * 
 * @param status axl_true to acquire the mutex before logging, otherwise
 * log without locking the context mutex.
 *
 * You can use \ref myqtt_log_is_enabled_acquire_mutex to check the
 * mutex status.
 */
void     myqtt_log_acquire_mutex    (MyQttCtx * ctx, 
				      axl_bool    status)
{
	/* get current context */
	v_return_if_fail (ctx);

	/* configure status */
	ctx->use_log_mutex = axl_true;
}

/** 
 * @brief Allows to check if the log mutex acquire is activated.
 *
 * @param ctx The context that will be required to return its
 * configuration.
 * 
 * @return Current status configured.
 */
axl_bool      myqtt_log_is_enabled_acquire_mutex (MyQttCtx * ctx)
{
	/* get current context */
	v_return_val_if_fail (ctx, axl_false);
	
	/* configure status */
	return ctx->use_log_mutex;
}

/** 
 * @brief Allows to configure an application handler that will be
 * called for each log produced by the myqtt engine.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param handler A reference to the handler to configure or NULL to
 * disable the notification.
 *
 * @param user_data User defined pointer passed into into the handler
 * provided when called.
 */
void     myqtt_log_set_handler      (MyQttCtx        * ctx, 
				     MyQttLogHandler   handler,
				     axlPointer        user_data)
{
	/* get current context */
	v_return_if_fail (ctx);
	
	/* configure status */
	ctx->debug_handler = handler;
	ctx->debug_handler_user_data = user_data;
}

/** 
 * @brief Allows to instruct myqtt to send string logs already
 * formated into the log handler configured (myqtt_log_set_handler).
 *
 * This will make myqtt to expand string arguments (message and
 * args), causing the argument \ref MyQttLogHandler message argument
 * to receive full content. In this case, args argument will be
 * received as NULL.
 *
 * @param ctx The myqtt context to configure.
 *
 * @param prepare_string axl_true to prepare string received by debug
 * handler, otherwise use axl_false to leave configured default
 * behaviour.
 */
void     myqtt_log_set_prepare_log  (MyQttCtx         * ctx,
				      axl_bool            prepare_string)
{
	v_return_if_fail (ctx);
	ctx->prepare_log_string = prepare_string;
}

/** 
 * @brief Allows to get current log handler configured. By default no
 * handler is configured so log produced by the myqtt execution is
 * dropped to the console.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return The handler configured (or NULL) if no handler was found
 * configured.
 */
MyQttLogHandler     myqtt_log_get_handler      (MyQttCtx * ctx)
{
	/* get current context */
	v_return_val_if_fail (ctx, NULL);
	
	/* configure status */
	return ctx->debug_handler;
}


/** 
 * @internal Internal common log implementation to support several levels
 * of logs.
 * 
 * @param ctx The context where the operation will be performed.
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The level of the log
 * @param message The message 
 * @param args Arguments for the message.
 */
void _myqtt_log_common (MyQttCtx        * ctx,
			 const       char * file,
			 int                line,
			 MyQttDebugLevel   log_level,
			 const char       * message,
			 va_list            args)
{

#ifndef ENABLE_MYQTT_LOG
	/* do no operation if not defined debug */
	return;
#else
	/* log with mutex */
	int    use_log_mutex = axl_false;
	char * log_string;
	struct timeval stamp;
	char   buffer[1024];

	/* if not MYQTT_DEBUG FLAG, do not output anything */
	if (! myqtt_log_is_enabled (ctx)) {
		return;
	} /* end if */

	if (ctx == NULL) {
		goto ctx_not_defined;
	}

	/* check if debug is filtered */
	if (myqtt_log_filter_is_enabled (ctx)) {
		/* if the filter removed the current log level, return */
		if ((ctx->debug_filter & log_level) == log_level)
			return;
	} /* end if */

	/* acquire the mutex so multiple threads will not mix their
	 * log messages together */
	use_log_mutex = ctx->use_log_mutex;
	if (use_log_mutex) 
		myqtt_mutex_lock (&ctx->log_mutex);

	if( ctx->debug_handler) {
		if (ctx->prepare_log_string) {
			/* pass the string already prepared */
			log_string = axl_strdup_printfv (message, args);
			ctx->debug_handler (file, line, log_level, log_string, NULL, ctx->debug_handler_user_data);
			axl_free (log_string);
		} else {
			/* call a custom debug handler if one has been set */
			ctx->debug_handler (file, line, log_level, message, args, ctx->debug_handler_user_data);
		} /* end if */

	} else {
		/* printout the process pid */
	ctx_not_defined:

		/* get current stamp */
		gettimeofday (&stamp, NULL);

		/* print the message */
		vsnprintf (buffer, 1023, message, args);
				
	/* drop a log according to the level */
#if defined (__GNUC__)
		if (myqtt_color_log_is_enabled (ctx)) {
			switch (log_level) {
			case MYQTT_LEVEL_DEBUG:
				fprintf (stdout, "\e[1;36m(%d.%d proc %d)\e[0m: (\e[1;32mdebug\e[0m) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			case MYQTT_LEVEL_WARNING:
				fprintf (stdout, "\e[1;36m(%d.%d proc %d)\e[0m: (\e[1;33mwarning\e[0m) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			case MYQTT_LEVEL_CRITICAL:
				fprintf (stdout, "\e[1;36m(%d.%d proc %d)\e[0m: (\e[1;31mcritical\e[0m) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			}
		} else {
#endif /* __GNUC__ */
			switch (log_level) {
			case MYQTT_LEVEL_DEBUG:
				fprintf (stdout, "(%d.%d proc %d): (debug) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			case MYQTT_LEVEL_WARNING:
				fprintf (stdout, "(%d.%d proc %d): (warning) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			case MYQTT_LEVEL_CRITICAL:
				fprintf (stdout, "(%d.%d proc %d): (critical) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			}
#if defined (__GNUC__)
		} /* end if */
#endif
		/* ensure that the log is dropped to the console */
		fflush (stdout);
		
	} /* end if (ctx->debug_handler) */

	/* check to release the mutex if defined the context */
	if (use_log_mutex) 
		myqtt_mutex_unlock (&ctx->log_mutex);

#endif /* end ENABLE_MYQTT_LOG */


	/* return */
	return;
}

/** 
 * @internal Log function used by myqtt to notify all messages that are
 * generated by the core. 
 *
 * Do no use this function directly, use <b>myqtt_log</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the operation will be performed.
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _myqtt_log (MyQttCtx        * ctx,
		  const       char * file,
		  int                line,
		  MyQttDebugLevel   log_level,
		  const char       * message,
		  ...)
{

#ifndef ENABLE_MYQTT_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* call to common implementation */
	va_start (args, message);
	_myqtt_log_common (ctx, file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}

/** 
 * @internal Log function used by myqtt to notify all second level
 * messages that are generated by the core.
 *
 * Do no use this function directly, use <b>myqtt_log2</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the log will be dropped.
 * @param file The file that contains that fired the log.
 * @param line The line where the log was produced.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _myqtt_log2 (MyQttCtx        * ctx,
		   const       char * file,
		   int                line,
		   MyQttDebugLevel   log_level,
		   const char       * message,
		  ...)
{

#ifndef ENABLE_MYQTT_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* if not MYQTT_DEBUG2 FLAG, do not output anything */
	if (!myqtt_log2_is_enabled (ctx)) {
		return;
	} /* end if */
	
	/* call to common implementation */
	va_start (args, message);
	_myqtt_log_common (ctx, file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}

/**
 * \defgroup myqtt MyQtt: Main MyQtt Library Module (initialization and exit stuff)
 */

/**
 * \addtogroup myqtt
 * @{
 */


/** 
 * @brief Context based myqtt library init. Allows to init the myqtt
 * library status on the provided context object (\ref MyQttCtx).
 *
 * To init myqtt library use: 
 * 
 * \code
 * MyQttCtx * ctx;
 * 
 * // create an empty context 
 * ctx = myqtt_ctx_new ();
 *
 * // init the context
 * if (! myqtt_init_ctx (ctx)) {
 *      printf ("failed to init the library..\n");
 * } 
 *
 * // do API calls before this function 
 * 
 * // terminate the context 
 * myqtt_exit_exit (ctx);
 *
 * // release the context 
 * myqtt_ctx_free (ctx);
 * \endcode
 * 
 * @param ctx An already created context where the library
 * initialization will take place.
 * 
 * @return axl_true if the context was initialized, otherwise axl_false is
 * returned.
 *
 * NOTE: This function is not thread safe, that is, calling twice from
 * different threads on the same object will cause improper
 * results. You can use \ref myqtt_init_check to ensure if you
 * already initialized the context.
 */
axl_bool    myqtt_init_ctx (MyQttCtx * ctx)
{
	int          thread_num;
	int          soft_limit;

	v_return_val_if_fail (ctx, axl_false);

	/* check if the library was already initialized */
	if (myqtt_init_check (ctx)) 
		return axl_true;

	/**** myqtt_io.c: init io module */
	myqtt_io_init (ctx);

	/**** myqtt.c: init global mutex *****/
	myqtt_mutex_create (&ctx->msg_id_mutex);
	myqtt_mutex_create (&ctx->connection_id_mutex);
	myqtt_mutex_create (&ctx->listener_mutex);
	myqtt_mutex_create (&ctx->listener_unlock);
	myqtt_mutex_create (&ctx->exit_mutex);
	myqtt_mutex_create (&ctx->port_share_mutex);

	/* init connection module */
	myqtt_conn_init (ctx);

	/* init myqtt support module on the context provided: 
	 * 
	 * A list containing all search paths with its domains to
	 * perform lookups, and its associated mutex to avoid the list
	 * to be corrupted by several access.
	 */
	myqtt_support_init (ctx);

#if ! defined(AXL_OS_WIN32)
	/* install sigpipe handler */
	signal (SIGPIPE, __myqtt_sigpipe_do_nothing);
#endif

#if defined(AXL_OS_WIN32)
	/* init winsock API */
	myqtt_log (MYQTT_LEVEL_DEBUG, "init winsocket for windows");
	if (! myqtt_win32_init (ctx))
		return axl_false;
#endif

	/* init axl library */
	axl_init ();

	/* add default paths */
#if defined(AXL_OS_UNIX)
	myqtt_log (MYQTT_LEVEL_DEBUG, "configuring context to use: %p", ctx);
	myqtt_support_add_search_path_ref (ctx, myqtt_support_build_filename ("libmyqtt-1.1", NULL ));
	myqtt_support_add_search_path_ref (ctx, myqtt_support_build_filename (PACKAGE_TOP_DIR, "libmyqtt-1.1", "data", NULL));
	myqtt_support_add_search_path_ref (ctx, myqtt_support_build_filename (PACKAGE_TOP_DIR, "data", NULL));
#endif
	/* do not use the add_search_path_ref version to force myqtt
	   library to perform a local copy path */
	myqtt_support_add_search_path (ctx, ".");
	
	/* before starting, check if we are using select(2) system
	 * call method, to adequate the number of sockets that can
	 * *really* handle the FD_* function family, to the number of
	 * sockets that is allowed to handle the process. This is to
	 * avoid race conditions cause to properly create a
	 * connection, which is later not possible to handle at the
	 * select(2) I/O subsystem. */
	if (myqtt_io_waiting_get_current (ctx) == MYQTT_IO_WAIT_SELECT) {
		/* now check if the current process soft limit is
		 * allowed to handle more connection than
		 * MYQTT_FD_SETSIZE */
		myqtt_conf_get (ctx, MYQTT_SOFT_SOCK_LIMIT, &soft_limit);
		myqtt_log (MYQTT_LEVEL_DEBUG, "select mechanism selected, reconfiguring current socket limit: soft_limit=%d > %d..",
			    soft_limit, MYQTT_FD_SETSIZE);
		if (soft_limit > (MYQTT_FD_SETSIZE - 1)) {
			/* decrease the limit to avoid funny
			 * problems. it is not required to be
			 * privilege user to run the following
			 * command. */
			myqtt_conf_set (ctx, MYQTT_SOFT_SOCK_LIMIT, (MYQTT_FD_SETSIZE - 1), NULL);
			myqtt_log (MYQTT_LEVEL_WARNING, 
				    "found select(2) I/O configured, which can handled up to %d fds, reconfigured process with that value",
				    MYQTT_FD_SETSIZE -1);
		} /* end if */
	} 

	/* init reader subsystem */
	myqtt_log (MYQTT_LEVEL_DEBUG, "starting myqtt reader..");
	if (! myqtt_reader_run (ctx))
		return axl_false;
	
	/* init writer subsystem */
	/* myqtt_log (MYQTT_LEVEL_DEBUG, "starting myqtt writer..");
	   myqtt_writer_run (); */

	/* init sequencer subsystem */
	myqtt_log (MYQTT_LEVEL_DEBUG, "starting myqtt sequencer..");
	if (! myqtt_sequencer_run (ctx))
		return axl_false;
	
	/* init thread pool (for query receiving) */
	thread_num = myqtt_thread_pool_get_num ();
	myqtt_log (MYQTT_LEVEL_DEBUG, "starting myqtt thread pool: (%d threads the pool have)..",
		    thread_num);
	myqtt_thread_pool_init (ctx, thread_num);

	/* flag this context as initialized */
	ctx->myqtt_initialized = axl_true;

	/* register the myqtt exit function */
	return axl_true;
}

/** 
 * @brief Allows to check if the provided MyQttCtx is initialized
 * (\ref myqtt_init_ctx).
 * @param ctx The context to be checked for initialization.
 * @return axl_true if the context was initialized, otherwise axl_false is returned.
 */
axl_bool myqtt_init_check (MyQttCtx * ctx)
{
	if (ctx == NULL || ! ctx->myqtt_initialized) {
		return axl_false;
	}
	return axl_true;
}


/** 
 * @brief Terminates the myqtt library execution on the provided
 * context.
 *
 * Stops all internal myqtt process and all allocated resources
 * associated to the context. It also close all connection that where
 * not closed until call this function.
 *
 * This function is reentrant, allowing several threads to call \ref
 * myqtt_exit_ctx function at the same time. Only one thread will
 * actually release resources allocated.
 *
 * NOTE2: This function isn't designed to dealloc all resources
 * associated to the context and used by the myqtt engine
 * function. According to the particular control structure used by
 * your application, you must first terminate using any MyQtt API and
 * then call to \ref myqtt_exit_ctx. In other words, this function
 * won't close all your opened connections.
 * 
 * @param ctx The context to terminate. The function do not dealloc
 * the context provided. 
 *
 * @param free_ctx Allows to signal the function if the context
 * provided must be deallocated (by calling to \ref myqtt_ctx_free).
 *
 */
void myqtt_exit_ctx (MyQttCtx * ctx, axl_bool  free_ctx)
{
	int            iterator;
	axlDestroyFunc func;

	/* check context is initialized */
	if (! myqtt_init_check (ctx))
		return;

	/* check if the library is already started */
	if (ctx == NULL || ctx->myqtt_exit)
		return;

	myqtt_log (MYQTT_LEVEL_DEBUG, "shutting down myqtt library, MyQttCtx %p", ctx);

	myqtt_mutex_lock (&ctx->exit_mutex);
	if (ctx->myqtt_exit) {
		myqtt_mutex_unlock (&ctx->exit_mutex);
		return;
	}
	/* flag other waiting functions to do nothing */
	myqtt_mutex_lock (&ctx->ref_mutex);
	ctx->myqtt_exit = axl_true;
	myqtt_mutex_unlock (&ctx->ref_mutex);
	
	/* unlock */
	myqtt_mutex_unlock  (&ctx->exit_mutex);

	/* flag the thread pool to not accept more jobs */
	myqtt_thread_pool_being_closed (ctx);

	/* stop myqtt writer */
	/* myqtt_writer_stop (); */

	/* stop myqtt reader process */
	myqtt_reader_stop (ctx);

	/* stop myqtt sequencer */
	myqtt_sequencer_stop (ctx);

	/* clean up myqtt modules */
	myqtt_log (MYQTT_LEVEL_DEBUG, "shutting down myqtt xml subsystem");

	/* Cleanup function for the XML library. */
	myqtt_log (MYQTT_LEVEL_DEBUG, "shutting down xml library");
	axl_end ();

	/* unlock listeners */
	myqtt_log (MYQTT_LEVEL_DEBUG, "unlocking myqtt listeners");
	myqtt_listener_unlock (ctx);

	myqtt_log (MYQTT_LEVEL_DEBUG, "myqtt library stopped");

	/* stop the myqtt thread pool: 
	 * 
	 * In the past, this call was done, however, it is showed that
	 * user applications on top of myqtt that wants to handle
	 * signals, emitted to all threads running (including the pool)
	 * causes many non-easy to solve problem related to race
	 * conditions.
	 * 
	 * At the end, to release the thread pool is not a big
	 * deal. */
	myqtt_thread_pool_exit (ctx); 

	/* cleanup connection module */
	myqtt_conn_cleanup (ctx); 

	/* cleanup listener module */
	myqtt_listener_cleanup (ctx);

	/* cleanup myqtt_support module */
	myqtt_support_cleanup (ctx);

	/* destroy global mutex */
	myqtt_mutex_destroy (&ctx->msg_id_mutex);
	myqtt_mutex_destroy (&ctx->connection_id_mutex);
	myqtt_mutex_destroy (&ctx->listener_mutex);
	myqtt_mutex_destroy (&ctx->listener_unlock);
	myqtt_mutex_destroy (&ctx->port_share_mutex);

	/* release port share handlers (if any) */
	axl_list_free (ctx->port_share_handlers);

	/* lock/unlock to avoid race condition */
	myqtt_mutex_lock  (&ctx->exit_mutex);
	myqtt_mutex_unlock  (&ctx->exit_mutex);
	myqtt_mutex_destroy (&ctx->exit_mutex);

	/* call to activate ctx cleanups */
	if (ctx->cleanups) {
		/* acquire lock */
		myqtt_mutex_lock (&ctx->ref_mutex);

		iterator = 0;
		while (iterator < axl_list_length (ctx->cleanups)) {
			/* get clean up function */
			func = axl_list_get_nth (ctx->cleanups, iterator);

			/* call to clean */
			myqtt_mutex_unlock (&ctx->ref_mutex);
			func (ctx);
			myqtt_mutex_lock (&ctx->ref_mutex);

			/* next iterator */
			iterator++;
		} /* end while */

		/* terminate list */
		axl_list_free (ctx->cleanups);
		ctx->cleanups = NULL; 

		/* release lock */
		myqtt_mutex_unlock (&ctx->ref_mutex);
	} /* end if */

#if defined(AXL_OS_WIN32)
	WSACleanup (); 
	myqtt_log (MYQTT_LEVEL_DEBUG, "shutting down WinSock2(tm) API");
#endif
   
	/* release the ctx */
	if (free_ctx)
		myqtt_ctx_free2 (ctx, "end ctx");

	return;
}

/** 
 * @brief Allows to check if myqtt engine started on the provided
 * context is finishing (a call to \ref myqtt_exit_ctx was done).
 *
 * @param ctx The context to check if it is exiting.
 *
 * @return axl_true in the case the context is finished, otherwise
 * axl_false is returned. The function also returns axl_false when
 * NULL reference is received.
 */
axl_bool myqtt_is_exiting           (MyQttCtx * ctx)
{
	axl_bool result;
	if (ctx == NULL)
		return axl_false;
	myqtt_mutex_lock (&ctx->ref_mutex);
	result = ctx->myqtt_exit;
	myqtt_mutex_unlock (&ctx->ref_mutex);
	return result;
}

/** 
 * @internal Allows to extract a particular bit from a byte given the
 * position.
 *
 *    +------------------------+
 *    | 7  6  5  4  3  2  1  0 | position
 *    +------------------------+
 */
int myqtt_get_bit (unsigned char byte, int position) {
	return ( ( byte & (1 << position) ) >> position);
}

/** 
 * @internal Allows to set a particular bit on the first position of
 * the buffer provided.
 *
 *    +------------------------+
 *    | 7  6  5  4  3  2  1  0 | position
 *    +------------------------+
 */
void myqtt_set_bit (unsigned char * buffer, int position) {
	buffer[0] |= (1 << position);
	return;
}

void myqtt_show_byte (MyQttCtx * ctx, unsigned char byte, const char * label) {
	
	myqtt_log (MYQTT_LEVEL_DEBUG, "  byte (%s) = %d %d %d %d  %d %d %d %d",
		   label,
		   myqtt_get_bit (byte, 7),
		   myqtt_get_bit (byte, 6),
		   myqtt_get_bit (byte, 5),
		   myqtt_get_bit (byte, 4),
		   myqtt_get_bit (byte, 3),
		   myqtt_get_bit (byte, 2),
		   myqtt_get_bit (byte, 1),
		   myqtt_get_bit (byte, 0));
	return;
}

unsigned char * myqtt_int2bin (int a, unsigned char *buffer, int buf_size) {
	int i;

	buffer += (buf_size - 1);
	
	for (i = 31; i >= 0; i--) {
		*buffer-- = (a & 1) + '0';
		
		a >>= 1;
	}
	
	return buffer;
}

#define BUF_SIZE 33

void myqtt_int2bin_print (MyQttCtx * ctx, int value) {
	
	unsigned char buffer[BUF_SIZE];
	buffer[BUF_SIZE - 1] = '\0';

	myqtt_int2bin (value, buffer, BUF_SIZE - 1);
	
	myqtt_log (MYQTT_LEVEL_DEBUG, "%d = %s", value, buffer);

	return;
}

/** 
 * @internal Allows to get the 16 bit integer located at the buffer
 * pointer.
 *
 * @param buffer The buffer pointer to extract the 16bit integer from.
 *
 * @return The 16 bit integer value found at the buffer pointer.
 */
int    myqtt_get_16bit (const unsigned char * buffer)
{
	int high_part = buffer[0] << 8;
	int low_part  = buffer[1] & 0x000000ff;

	return (high_part | low_part) & 0x000000ffff;
}

/** 
 * @internal Allows to get the 8bit integer located at the buffer
 * pointer.
 *
 * @param buffer The buffer pointer to extract the 8bit integer from.
 *
 * @erturn The 8 bit integer value found at the buffer pointer.
 */
int    myqtt_get_8bit  (const unsigned char * buffer)
{
	return buffer[0] & 0x00000000ff;
}

/** 
 * @internal Allows to set the 16 bit integer value into the 2 first
 * bytes of the provided buffer.
 *
 * @param value The value to be configured in the buffer.
 *
 * @param buffer The buffer where the content will be placed.
 */
void   myqtt_set_16bit (int value, unsigned char * buffer)
{
	buffer[0] = (value & 0x0000ff00) >> 8;
	buffer[1] = value & 0x000000ff;
	
	return;
}

/** 
 * @internal Allows to set the 32 bit integer value into the 4 first
 * bytes of the provided buffer.
 *
 * @param value The value to be configured in the buffer.
 *
 * @param buffer The buffer where the content will be placed.
 */
void   myqtt_set_32bit (int value, unsigned char * buffer)
{
	buffer[0] = (value & 0x00ff000000) >> 24;
	buffer[1] = (value & 0x0000ff0000) >> 16;
	buffer[2] = (value & 0x000000ff00) >> 8;
	buffer[3] =  value & 0x00000000ff;

	return;
}

/** 
 * @internal Allows to get a 32bits integer value from the buffer.
 *
 * @param buffer The buffer where the integer will be retreived from.
 *
 * @return The integer value reported by the buffer.
 */
int    myqtt_get_32bit (const unsigned char * buffer)
{
	int part1 = (int)(buffer[0] & 0x0ff) << 24;
	int part2 = (int)(buffer[1] & 0x0ff) << 16;
	int part3 = (int)(buffer[2] & 0x0ff) << 8;
	int part4 = (int)(buffer[3] & 0x0ff);

	return part1 | part2 | part3 | part4;
}

/** 
 * @brief Portable subsecond sleep. Suspends the calling thread during
 * the provided amount of time.
 *
 * @param microseconds The amount of time to wait.
 */
void        myqtt_sleep (long microseconds)
{
#if defined(AXL_OS_UNIX)
	usleep (microseconds);
	return;
#elif defined(AXL_OS_WIN32)
	Sleep (microseconds / 1000);
	return;
#endif
}

/* @} */


