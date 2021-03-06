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

#include <py_myqtt.h>

/* signal handling */
#include <signal.h>

#if defined(ENABLE_PY_MYQTT_LOG)
/** 
 * @brief Variable used to track if environment variables associated
 * to log were checked. 
 */
axl_bool _py_myqtt_log_checked = axl_false;

/** 
 * @brief Boolean variable that tracks current console log status. By
 * default it is disabled.
 */
axl_bool _py_myqtt_log_enabled = axl_false;

/** 
 * @brief Boolean variable that tracks current second level console
 * log status. By default it is disabled.
 */
axl_bool _py_myqtt_log2_enabled = axl_false;

/** 
 * @brief Boolean variable that tracks current console color log
 * status. By default it is disabled.
 */
axl_bool _py_myqtt_color_log_enabled = axl_false;
#endif


/** 
 * @brief Allows to start a listener running on the address and port
 * specified.
 */
static PyObject * py_myqtt_create_listener (PyObject * self, PyObject * args, PyObject * kwds)
{
	const char         * host          = NULL;
	const char         * port          = NULL;
	PyObject           * py_myqtt_ctx  = NULL;
	MyQttConn          * listener      = NULL;
	PyObject           * py_listener;

	/* now parse arguments */
	static char        * kwlist[] = {"ctx", "host", "port", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "Oss", kwlist, &py_myqtt_ctx, &host, &port))
		return NULL;

	/* create a listener */
	listener = myqtt_listener_new (
		/* context */
		py_myqtt_ctx_get (py_myqtt_ctx),
		/* host and port */
		host, port, NULL, NULL, NULL);

	py_myqtt_log (PY_MYQTT_DEBUG, "creating listener using: %s:%s (%p, status: %d)", host, port,
		       listener, myqtt_conn_is_ok (listener, axl_false));

	/* do not check if the connection is ok, to return a different
	   value. Rather return a PyMyQttConn in all cases
	   allowing the user to call to .is_ok () */

	/* create the listener and acquire a reference to the
	 * PyMyQttCtx */
	py_listener =  py_myqtt_conn_create (
			/* connection reference wrapped */
			listener, 
			/* acquire a reference */
			axl_true,
			/* close ref on variable collect */
			axl_true);

	/* handle exception */
	py_myqtt_handle_and_clear_exception (py_listener);

	py_myqtt_log (PY_MYQTT_DEBUG, "py_listener running at: %s:%s (refs: %d, id: %d)", 
		       myqtt_conn_get_host (listener),
		       myqtt_conn_get_port (listener),
		       myqtt_conn_ref_count (listener),
		       myqtt_conn_get_id (listener));

	return py_listener;
}

MyQttCtx * py_myqtt_wait_listeners_ctx = NULL;

void py_myqtt_wait_listeners_sig_handler (int _signal)
{
	MyQttCtx * ctx = py_myqtt_wait_listeners_ctx;

	/* nullify context */
	py_myqtt_wait_listeners_ctx = NULL;

	py_myqtt_log (PY_MYQTT_DEBUG, "received signal %d, unlocking (ctx: %p)", _signal, ctx);
	myqtt_listener_unlock (ctx);
	return;
}

/** 
 * @brief Implementation of myqtt.wait_listeners which blocks the
 * caller until all myqtt library is stopped.
 */
static PyObject * py_myqtt_wait_listeners (PyObject * self, PyObject * args, PyObject * kwds)
{
	
	PyObject           * py_myqtt_ctx    = NULL;
	axl_bool             unlock_on_signal = axl_false;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", "unlock_on_signal", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "O|i", kwlist, &py_myqtt_ctx, &unlock_on_signal))
		return NULL;

	if (! py_myqtt_ctx_check (py_myqtt_ctx))
		return NULL;

	/* received context, increase its reference during the wait
	   operation */
	Py_INCREF (py_myqtt_ctx);

	/* configure signal handling to detect signal emision */
	if (unlock_on_signal) {
		/* record ctx */
		py_myqtt_wait_listeners_ctx = py_myqtt_ctx_get (py_myqtt_ctx);

		/* configure handler */
		signal (SIGTERM, py_myqtt_wait_listeners_sig_handler);
		signal (SIGQUIT, py_myqtt_wait_listeners_sig_handler);
		signal (SIGINT,  py_myqtt_wait_listeners_sig_handler);
	} /* end if */

	/* allow other threads to enter into the python space */
	Py_BEGIN_ALLOW_THREADS

	/* call to wait for listeners */
	py_myqtt_log (PY_MYQTT_DEBUG, "waiting listeners to finish: %p", py_myqtt_ctx_get (py_myqtt_ctx));
	myqtt_listener_wait (py_myqtt_ctx_get (py_myqtt_ctx));
	py_myqtt_log (PY_MYQTT_DEBUG, "wait for listeners ended, returning");

	/* restore thread state */
	Py_END_ALLOW_THREADS

	Py_DECREF (py_myqtt_ctx);
	
	/* return none */
	Py_INCREF (Py_None);
	return Py_None;
}

/** 
 * @brief Implementation of myqtt.unlock_listeners which blocks the
 * caller until all myqtt library is stopped.
 */
static PyObject * py_myqtt_unlock_listeners (PyObject * self, PyObject * args, PyObject * kwds)
{
	
	PyObject           * py_myqtt_ctx = NULL;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "O", kwlist, &py_myqtt_ctx))
		return NULL;
	
	if (! py_myqtt_ctx_check (py_myqtt_ctx))
		return NULL;

	/* unlock the caller */
	py_myqtt_log (PY_MYQTT_DEBUG, "unlocking listeners: %p", py_myqtt_ctx_get (py_myqtt_ctx));
	myqtt_listener_unlock (py_myqtt_ctx_get (py_myqtt_ctx));
	
	/* return none */
	Py_INCREF (Py_None);
	return Py_None;
}

void py_myqtt_decref (PyObject * obj)
{
	Py_XDECREF (obj);
	return;
}

static PyMethodDef py_myqtt_methods[] = { 
	/* create_listener */
	{"create_listener", (PyCFunction) py_myqtt_create_listener, METH_VARARGS | METH_KEYWORDS,
	 "Wrapper of the set of functions that allows to create a MQTT listener. The function returns a new myqtt.Connection that represents a listener running on the port and address provided."},
	/* wait_listeners */
	{"wait_listeners", (PyCFunction) py_myqtt_wait_listeners, METH_VARARGS | METH_KEYWORDS,
	 "Direct wrapper for myqtt_listener_wait. This function is optional and it is used at the listener side to make the main thread to not finish after all myqtt initialization."},
	{"unlock_listeners", (PyCFunction) py_myqtt_unlock_listeners, METH_VARARGS | METH_KEYWORDS,
	 "Direct wrapper for myqtt_listener_unlock. This function allows to unlock the thread that is blocked at myqtt.wait_listeners."},
	{NULL, NULL, 0, NULL}   /* sentinel */
}; 

/** 
 * @internal Function that inits all myqtt modules and classes.
 */ 
PyMODINIT_FUNC  initlibpy_myqtt_10 (void)
{
	PyObject * module;

	/** 
	 * NOTE: it seems the previous call is not the appropriate way
	 * but there are relevant people that do not think so:
	 *
	 * http://fedoraproject.org/wiki/Features/PythonEncodingUsesSystemLocale
	 *
	 * Our appreciation is that python should take care of the
	 * current system locale to translate unicode content into
	 * const char strings, for those Py_ParseTuple and Py_BuildArg
	 * using s and z, rather forcing people to get into these
	 * hacks which are problematic. 
	 */
	PyUnicode_SetDefaultEncoding ("UTF-8");

	/* call to initilize threading API and to acquire the lock */
	PyEval_InitThreads();

	/* register myqtt module */
	module = Py_InitModule3 ("libpy_myqtt_10", py_myqtt_methods, 
				 "Base module that include core MQTT elements implemented by the MyQtt base library");
	if (module == NULL) 
		return;

	/* call to register all myqtt modules and types */
	init_myqtt_ctx          (module);
	init_myqtt_conn         (module);
	init_myqtt_async_queue  (module);
	init_myqtt_msg          (module);
	init_myqtt_handle       (module);
	init_myqtt_conn_opts    (module);

	return;
}

/** 
 * @brief Allows to get current log enabled status.
 *
 * @return axl_true if log is enabled, otherwise axl_false is returned.
 */
axl_bool py_myqtt_log_is_enabled (void)
{
#if defined(ENABLE_PY_MYQTT_LOG)
	/* if log is not checked, check environment variables */
	if (! _py_myqtt_log_checked) {
		_py_myqtt_log_checked = axl_true;

		/* check for PY_MYQTT_DEBUG */
		_py_myqtt_log_enabled = myqtt_support_getenv_int ("PY_MYQTT_DEBUG") > 0;

		/* check for PY_MYQTT_DEBUG_COLOR */
		_py_myqtt_color_log_enabled = myqtt_support_getenv_int ("PY_MYQTT_DEBUG_COLOR") > 0;
	} /* end if */

	return _py_myqtt_log_enabled;
#else
	return axl_false;
#endif
}

/** 
 * @brief Allows to get current second level log enabled status.
 *
 * @return axl_true if log is enabled, otherwise axl_false is returned.
 */
axl_bool py_myqtt_log2_is_enabled (void)
{
#if defined(ENABLE_PY_MYQTT_LOG)
	return _py_myqtt_log2_enabled;
#else
	return axl_false;
#endif
}

/** 
 * @brief Allows to get current color log enabled status.
 *
 * @return axl_true if color log is enabled, otherwise axl_false is returned.
 */
axl_bool py_myqtt_color_log_is_enabled (void)
{
#if defined(ENABLE_PY_MYQTT_LOG)
	return _py_myqtt_color_log_enabled;
#else
	return axl_false;
#endif
}

/** 
 * @internal Internal common log implementation to support several
 * levels of logs.
 * 
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The level of the log
 * @param message The message 
 * @param args Arguments for the message.
 */
void _py_myqtt_log_common (const char          * file,
			    int                   line,
			    PyMyQttLog           log_level,
			    const char          * message,
			    va_list               args)
{
#if defined(ENABLE_PY_MYQTT_LOG)
	/* if not PY_MYQTT_DEBUG FLAG, do not output anything */
	if (!py_myqtt_log_is_enabled ()) 
		return;

#if defined (__GNUC__)
	if (py_myqtt_color_log_is_enabled ()) 
		fprintf (stdout, "\e[1;36m(proc %d)\e[0m: ", getpid ());
	else 
#endif /* __GNUC__ */
		fprintf (stdout, "(proc %d): ", getpid ());
		
		
	/* drop a log according to the level */
#if defined (__GNUC__)
	if (py_myqtt_color_log_is_enabled ()) {
		switch (log_level) {
		case PY_MYQTT_DEBUG:
			fprintf (stdout, "(\e[1;32mdebug\e[0m) ");
			break;
		case PY_MYQTT_WARNING:
			fprintf (stdout, "(\e[1;33mwarning\e[0m) ");
			break;
		case PY_MYQTT_CRITICAL:
			fprintf (stdout, "(\e[1;31mcritical\e[0m) ");
			break;
		}
	} else {
#endif /* __GNUC__ */
		switch (log_level) {
		case PY_MYQTT_DEBUG:
			fprintf (stdout, "(debug) ");
			break;
		case PY_MYQTT_WARNING:
			fprintf (stdout, "(warning) ");
			break;
		case PY_MYQTT_CRITICAL:
			fprintf (stdout, "(critical) ");
			break;
		}
#if defined (__GNUC__)
	} /* end if */
#endif
	
	/* drop a log according to the domain */
	(file != NULL) ? fprintf (stdout, "%s:%d ", file, line) : fprintf (stdout, ": ");
	
	/* print the message */
	vfprintf (stdout, message, args);
	
	fprintf (stdout, "\n");
	
	/* ensure that the log is dropped to the console */
	fflush (stdout);
#endif /* end ENABLE_PY_MYQTT_LOG */

	/* return */
	return;
}

/** 
 * @internal Log function used by py_myqtt to notify all messages that are
 * generated by the core. 
 *
 * Do no use this function directly, use <b>py_myqtt_log</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the operation will be performed.
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _py_myqtt_log (const char          * file,
		     int                   line,
		     PyMyQttLog           log_level,
		     const char          * message,
		     ...)
{

#ifndef ENABLE_PY_MYQTT_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* call to common implementation */
	va_start (args, message);
	_py_myqtt_log_common (file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}

/** 
 * @internal Log function used by py_myqtt to notify all second level
 * messages that are generated by the core.
 *
 * Do no use this function directly, use <b>py_myqtt_log2</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the log will be dropped.
 * @param file The file that contains that fired the log.
 * @param line The line where the log was produced.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _py_myqtt_log2 (const char          * file,
		      int                   line,
		      PyMyQttLog           log_level,
		      const char          * message,
		      ...)
{

#ifndef ENABLE_PY_MYQTT_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* if not PY_MYQTT_DEBUG2 FLAG, do not output anything */
	if (!py_myqtt_log2_is_enabled ()) {
		return;
	} /* end if */
	
	/* call to common implementation */
	va_start (args, message);
	_py_myqtt_log_common (file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}

/** 
 * @internal Handler used to notify exception catched and handled by
 * py_myqtt_handle_and_clear_exception.
 */
PyMyQttExceptionHandler py_myqtt_exception_handler = NULL;

/** 
 * @brief Allows to check, handle and clear exception state.
 */ 
axl_bool py_myqtt_handle_and_clear_exception (PyObject * py_conn)
{
	PyObject * ptype      = NULL;
	PyObject * pvalue     = NULL;
	PyObject * ptraceback = NULL;
	PyObject * list;
	PyObject * string;
	PyObject * mod;
	int        iterator;
	char     * str;
	char     * str_aux;
	axl_bool   found_error = axl_false;


	/* check exception */
	if (PyErr_Occurred()) {
		/* notify error found */
		found_error = axl_true;

		py_myqtt_log (PY_MYQTT_CRITICAL, "found exception...handling..");

		/* fetch exception state */
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		/* import traceback module */
		mod = PyImport_ImportModule("traceback");
		if (! mod) {
			py_myqtt_log (PY_MYQTT_CRITICAL, "failed to import traceback module, printing error to console");
			/* print exception */
			PyErr_Print ();
			goto clean_up;
		} /* end if */

		/* list of backtrace items */
		py_myqtt_log (PY_MYQTT_CRITICAL, "formating exception: ptype:%p  pvalue:%p  ptraceback:%p",
			       ptype, pvalue, ptraceback);

		/* check ptraceback */
		if (ptraceback == NULL) {
			ptraceback = Py_None;
			Py_INCREF (Py_None);
		} /* end if */

		/* check pvalue */
		if (pvalue == NULL) {
			pvalue = Py_None;
			Py_INCREF (Py_None);
		} /* end if */

		list     = PyObject_CallMethod (mod, "format_exception", "OOO", ptype,  pvalue, ptraceback);
		iterator = 0;
		if (py_myqtt_exception_handler)
			str      = axl_strdup ("");
		else
			str      = axl_strdup ("PyMyQtt found exception inside: \n");
		while (iterator < PyList_Size (list)) {
			/* get the string */
			string  = PyList_GetItem (list, iterator);

			str_aux = str;
			str     = axl_strdup_printf ("%s%s", str_aux, PyString_AsString (string));
			axl_free (str_aux);

			/* next iterator */
			iterator++;
		}

		/* drop a log */
		if (py_myqtt_exception_handler) {
			/* remove trailing \n */
			str[strlen (str) - 1] = 0;

			/* clear all % to avoid printf problems to the
			   handler */
			iterator = 0;
			while (str[iterator])  {
				if (str[iterator] == '%')
					str[iterator] = '#';
				iterator++;
			} /* end while */

			/* allow other threads to enter into the python space */
			Py_BEGIN_ALLOW_THREADS

			py_myqtt_exception_handler (str);

			/* restore thread state */
			Py_END_ALLOW_THREADS
			

#if defined(ENABLE_PY_MYQTT_LOG)
		} else if (_py_myqtt_log_enabled) {
			str[strlen (str) - 1] = 0;
			py_myqtt_log (PY_MYQTT_CRITICAL, str);
#endif
		} else {
			fprintf (stdout, "%s", str);
		}
		/* free message */
		axl_free (str);

		/* create an empty string \n */
		Py_XDECREF (list);
		Py_DECREF (mod);


	clean_up:
		/* call to finish retrieved vars .. */
		Py_XDECREF (ptype);
		Py_XDECREF (pvalue);
		Py_XDECREF (ptraceback);

		/* check connection reference and its role to close it
		 * in the case we are working at the listener side */
		if (py_conn && (myqtt_conn_get_role (py_myqtt_conn_get (py_conn)) != MyQttRoleInitiator)) {
			/* shutdown connection due to unhandled exception found */
			py_myqtt_log (PY_MYQTT_CRITICAL, "shutting down connection due to unhandled exception found");
			Py_DECREF ( py_myqtt_conn_shutdown (PY_MYQTT_CONN (py_conn)) );
		}
		

	} /* end if */

	/* clear exception */
	PyErr_Clear ();

	return found_error;
}

/** 
 * @brief Allows to configure a handler that will receive the
 * exception message created. This is useful inside the context of
 * PyMyQtt and Turbulence because exception propagated from python
 * into the c space are handled by \ref
 * py_myqtt_handle_and_clear_exception which support notifying the
 * entire exception message on the handler here configured.
 *
 * @param handler The handler to configure to receive all exception
 * handling.
 */
void     py_myqtt_set_exception_handler (PyMyQttExceptionHandler handler)
{
	/* configure the handler */
	py_myqtt_exception_handler = handler;

	return;
}
