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

#include <py_myqtt_ctx.h>

struct _PyMyQttCtx {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the myqtt context */
	MyQttCtx * ctx;

	/* flags if the PyMyQttCtx is pending to exit */
	axl_bool    exit_pending;
};

axlHash      * py_ctx_refs = NULL;
MyQttMutex    py_ctx_refs_mutex;

/** 
 * @brief Allows to get the MyQttCtx type definition found inside the
 * PyMyQttCtx encapsulation.
 *
 * @param py_myqtt_ctx The PyMyQttCtx that holds a reference to the
 * inner MyQttCtx.
 *
 * @return A reference to the inner MyQttCtx.
 */
MyQttCtx * py_myqtt_ctx_get (PyObject * py_myqtt_ctx)
{
	if (py_myqtt_ctx == NULL)
		return NULL;
	
	/* return current context created */
	return ((PyMyQttCtx *)py_myqtt_ctx)->ctx;
}

/* open handler (last handler that is found running) */
#define PY_MYQTT_WATCHER_HANDLER_HASH  "py:vo:wa:ha"
#define PY_MYQTT_WATCHER_NOTIFIER      "py:vo:wa:no"
#define PY_MYQTT_WATCHER_NOTIFIER_DATA "py:vo:wa:nod"

typedef struct _PyMyQttHandlerWatcher {
	char    * handler_string;
	int       stamp;
} PyMyQttHandlerWatcher;

void py_myqtt_ctx_release_handler_watcher (axlPointer ptr)
{
	PyMyQttHandlerWatcher * watcher = ptr;

	axl_free (watcher->handler_string);
	axl_free (watcher);
	
	return;
}

/** 
 * @internal Allows to track a handler started
 */
void        py_myqtt_ctx_record_start_handler (MyQttCtx * ctx, PyObject * handler)
{
	PyObject                * obj;
	struct timeval            stamp;
	char                    * buffer;
	Py_ssize_t                buffer_size;
	PyMyQttHandlerWatcher  * watcher;
	MyQttHash              * hash = myqtt_ctx_get_data (ctx, PY_MYQTT_WATCHER_HANDLER_HASH);

	/* no hash no record */
	if (hash == NULL)
		return;

	/* record handler position */
	watcher = axl_new (PyMyQttHandlerWatcher, 1);
	if (watcher == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to acquire memory to record handler start, skipping");
		return;
	} /* end if */

	/* build an string representation from the handler */
	obj = PyObject_Str (handler);
	if (obj == NULL) {
		axl_free (watcher);
		return;
	}
	if (PyString_AsStringAndSize (obj, &buffer, &buffer_size) == -1) {
		Py_DECREF (obj);
		axl_free (watcher);
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to record handler started because it wasn't possible to build an string representation");
		return;
	} /* end if */
	Py_DECREF (obj);

	/* record handler started */
	watcher->handler_string = axl_strdup (buffer);

	gettimeofday (&stamp, NULL);
	watcher->stamp = (int) stamp.tv_sec;
	
	/* record value */
	/* py_myqtt_log (PY_MYQTT_DEBUG, "Started handler %s and stamp %d", watcher->handler_string, (int) watcher->stamp);  */

	/* record */
	myqtt_hash_replace_full (hash, handler, NULL, watcher, py_myqtt_ctx_release_handler_watcher);

	return;
}

void        py_myqtt_ctx_record_close_handler (MyQttCtx * ctx, PyObject * handler)
{
	MyQttHash * hash = myqtt_ctx_get_data (ctx, PY_MYQTT_WATCHER_HANDLER_HASH);

	/* no hash no record */
	if (hash == NULL)
		return;

	/* delete handler */
	myqtt_hash_remove (hash, handler);
	return;
}

axl_bool py_myqtt_ctx_handler_watcher_foreach (axlPointer key, axlPointer data, axlPointer user_data, axlPointer user_data2, axlPointer _ctx)
{
	PyMyQttHandlerWatcher * watcher         = data;
	int                      watching_period = PTR_TO_INT (user_data2);
	struct  timeval        * stamp           = user_data;
	PyMyQttTooLongNotifier  notifier;
	axlPointer               notifier_data;
	char                   * report_msg;
	MyQttCtx              * ctx = _ctx;

	if ((stamp->tv_sec - watcher->stamp) > watching_period) {
		py_myqtt_log (PY_MYQTT_WARNING, "handler '%s' is taking too long to finish, it being running %d seconds..",
			       watcher->handler_string, (stamp->tv_sec - watcher->stamp));

		/* get references to notifiers */
		notifier      = myqtt_ctx_get_data (ctx, PY_MYQTT_WATCHER_NOTIFIER);
		notifier_data = myqtt_ctx_get_data (ctx, PY_MYQTT_WATCHER_NOTIFIER_DATA);

		if (notifier) {
			/* build message */
			report_msg = axl_strdup_printf ("handler '%s' is taking too long to finish, it being running %d seconds..",
							watcher->handler_string, (stamp->tv_sec - watcher->stamp));
			/* notify caller */
			notifier (report_msg, notifier_data);
			/* release */
			axl_free (report_msg);
		}
	} /* end if */

	return axl_false; /* do not stop */
}

axl_bool py_myqtt_ctx_handler_watcher (MyQttCtx *ctx, axlPointer user_data, axlPointer user_data2)
{
	int              watching_period  = PTR_TO_INT (user_data);
	MyQttHash     * hash             = myqtt_ctx_get_data (ctx, PY_MYQTT_WATCHER_HANDLER_HASH);
	struct timeval   current;
	
	/* py_myqtt_log (PY_MYQTT_WARNING, "checking for long running handlers, currently registered: %d..",
	   myqtt_hash_size (hash)); */
	if (myqtt_hash_size (hash) == 0)
		return axl_false; /* do not remove handler */

	/* get current stamp */
	gettimeofday (&current, NULL);

	/* foreach all registered handlers */
	myqtt_hash_foreach3 (hash, py_myqtt_ctx_handler_watcher_foreach, &current, INT_TO_PTR (watching_period), ctx);

	return axl_false; /* do not remove handler */
}

/** 
 * @internal Start a handler watcher to report user when a handler is
 * taking too long. In general, having watching_period equal to 3
 * seconds is a good value. This is because you handler shouldn't take
 * longer than that and, if that might happen, look for a solution:
 * start a task that regularly check the result in the future (python
 * subprocess has a nice poll() method to asynchounously check that).
 *
 * @param watching_period When to check if there are opened handler in seconds.
 */
void        py_myqtt_ctx_start_handler_watcher (MyQttCtx * ctx, int watching_period,
						 PyMyQttTooLongNotifier notifier, axlPointer notifier_data)
{

	MyQttHash * hash;

	/* check handler to be previously defined */
	if (myqtt_ctx_get_data (ctx, PY_MYQTT_WATCHER_HANDLER_HASH)) 
		return;

	/* init hash */
	hash = myqtt_hash_new (axl_hash_int, axl_hash_equal_int);
	myqtt_ctx_set_data_full (ctx, PY_MYQTT_WATCHER_HANDLER_HASH, hash, NULL, (axlDestroyFunc) myqtt_hash_unref);

	/* record notifier if defined */
	myqtt_ctx_set_data (ctx, PY_MYQTT_WATCHER_NOTIFIER, notifier);
	myqtt_ctx_set_data (ctx, PY_MYQTT_WATCHER_NOTIFIER_DATA, notifier_data);

	/* start handler */
	myqtt_thread_pool_new_event (ctx, (watching_period + 1) * 1000000, 
				      py_myqtt_ctx_handler_watcher, INT_TO_PTR (watching_period), NULL);
	return;
}

void py_myqtt_ctx_too_long_notifier_to_file (const char * msg_string, axlPointer data)
{
	const char * file = data;
	int          fd;
	time_t       rawtime;
	const char * value;

	py_myqtt_log (PY_MYQTT_DEBUG, "Logging into file: [%s], message [%s]", file, msg_string);
	
	/* open the file provided by the user */
	fd = open (file, O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (fd < 0) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "Failed to open log located at %s to log %s", file, msg_string);
		return;
	} /* end if */

	time ( &rawtime );
	value = ctime (&rawtime);
	if (write (fd, value, strlen (value) - 1) < 0)
		py_myqtt_log (PY_MYQTT_WARNING, "Unable to write info into log, errno: %s", myqtt_errno_get_error (errno));
	if (write (fd, "  ", 2) < 0)
		py_myqtt_log (PY_MYQTT_WARNING, "Unable to write info into log, errno: %s", myqtt_errno_get_error (errno));
	if (write (fd, msg_string, strlen (msg_string)) < 0)
		py_myqtt_log (PY_MYQTT_WARNING, "Unable to write info into log, errno: %s", myqtt_errno_get_error (errno));
	if (write (fd, "\n", 1) < 0)
		py_myqtt_log (PY_MYQTT_WARNING, "Unable to write info into log, errno: %s", myqtt_errno_get_error (errno));
	close (fd);

	return;
}

/** 
 * @internal Installs a too long notification handler that logs into
 * the provided file, all notifications found for handlers that are
 * taking more than the provided watching period measured in seconds.
 */
axl_bool        py_myqtt_ctx_log_too_long_notifications (MyQttCtx * ctx, int watching_period,
							  const char * file)
{
	int          fd;
	time_t       rawtime;
	const char * value;

	/* open the file provided by the user */
	fd = open (file, O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (fd < 0) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "Failed to open log located at %s, unable to enable long notifications", file);
		return axl_false;
	} /* end if */

	time ( &rawtime );
	value = ctime (&rawtime);
	if (write (fd, value, strlen (value) - 1) < 0)
		py_myqtt_log (PY_MYQTT_WARNING, "Unable to write info into log, errno: %s", myqtt_errno_get_error (errno));
	if (write (fd, "  ", 2) < 0)
		py_myqtt_log (PY_MYQTT_WARNING, "Unable to write info into log, errno: %s", myqtt_errno_get_error (errno));
	if (write (fd, "Too long notification watcher started\n", 38) < 0)
		py_myqtt_log (PY_MYQTT_WARNING, "Unable to write info into log, errno: %s", myqtt_errno_get_error (errno));
	close (fd);

	/* record file into ctx */
	file = strdup (file);
	myqtt_ctx_set_data_full (ctx, file, (axlPointer) file, NULL, axl_free);

	/* start handler watcher here */
	py_myqtt_ctx_start_handler_watcher (ctx, watching_period, py_myqtt_ctx_too_long_notifier_to_file, (axlPointer) file);	

	return axl_true;
}

static int py_myqtt_ctx_init_type (PyMyQttCtx *self, PyObject *args, PyObject *kwds)
{
    return 0;
}



/** 
 * @brief Function used to allocate memory required by the object myqtt.ctx
 */
static PyObject * py_myqtt_ctx_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyMyQttCtx *self;

	/* create the object */
	self = (PyMyQttCtx *)type->tp_alloc(type, 0);

	/* create the context */
	self->ctx = myqtt_ctx_new ();
	py_myqtt_log (PY_MYQTT_DEBUG, "created PyMyQttCtx referece %p (MyQttCtx %p)", self, self->ctx);

	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object myqtt.ctx
 */
static void py_myqtt_ctx_dealloc (PyMyQttCtx* self)
{
	py_myqtt_log (PY_MYQTT_DEBUG, "collecting myqtt.Ctx ref: %p (self->ctx: %p, count: %d)", self, self->ctx, myqtt_ctx_ref_count (self->ctx));

	/* check for pending exit */
	if (self->exit_pending) {
		py_myqtt_log (PY_MYQTT_DEBUG, "found myqtt.Ctx () exiting pending flag enabled, finishing context..");
		Py_DECREF ( py_myqtt_ctx_exit (self) );
	} /* end if */

	/* free ctx */
	myqtt_ctx_free (self->ctx);
	self->ctx = NULL;

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}

/** 
 * @brief Direct wrapper for the myqtt_init_ctx function. This method
 * receives a myqtt.ctx object and initializes it calling to
 * myqtt_init_ctx.
 */
static PyObject * py_myqtt_ctx_init (PyMyQttCtx* self)
{
	PyObject *_result;

	/* check to not reinitialize */
	if (self->exit_pending) {
		py_myqtt_log (PY_MYQTT_WARNING, "called to initialize a context twice");
		return Py_BuildValue ("i", axl_true);
	} /* end if */

	/* call to init context and build result value */
	self->exit_pending = myqtt_init_ctx (self->ctx);
	_result = Py_BuildValue ("i", self->exit_pending);

	return _result;
}

/** 
 * @brief Direct wrapper for the myqtt_init_ctx function. This method
 * receives a myqtt.ctx object and initializes it calling to
 * myqtt_init_ctx.
 */
PyObject * py_myqtt_ctx_exit (PyMyQttCtx* self)
{
	if (self->exit_pending) {
		py_myqtt_log (PY_MYQTT_DEBUG, "finishing myqtt.Ctx %p (self->ctx: %p)", self, self->ctx);

		/* let other threads to work after this */
		Py_BEGIN_ALLOW_THREADS

		/* call to finish context: do not dealloc ->ctx, this is
		   already done by the type deallocator */
		myqtt_exit_ctx (self->ctx, axl_false);

		/* restore thread state */
		Py_END_ALLOW_THREADS

		/* flag as exit done */
		self->exit_pending = axl_false;
	}

	/* return None */
	Py_INCREF (Py_None);
	return Py_None;
}

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_myqtt_ctx_get_attr (PyObject *o, PyObject *attr_name) {
	const char      * attr = NULL;
	PyObject        * result;
	PyMyQttCtx     * self = (PyMyQttCtx *) o;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	py_myqtt_log (PY_MYQTT_DEBUG, "received request to report channel attr name %s (self: %p)",
		       attr, o);

	if (axl_cmp (attr, "log")) {
		/* log attribute */
		return Py_BuildValue ("i", myqtt_log_is_enabled (self->ctx));
	} else if (axl_cmp (attr, "log2")) {
		/* log2 attribute */
		return Py_BuildValue ("i", myqtt_log2_is_enabled (self->ctx));
	} else if (axl_cmp (attr, "color_log")) {
		/* color_log attribute */
		return Py_BuildValue ("i", myqtt_color_log_is_enabled (self->ctx));
	} else if (axl_cmp (attr, "ref_count")) {
		/* ref counting */
		return Py_BuildValue ("i", myqtt_ctx_ref_count (self->ctx));
	} /* end if */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

/** 
 * @brief Implements attribute set operation.
 */
int py_myqtt_ctx_set_attr (PyObject *o, PyObject *attr_name, PyObject *v)
{
	const char      * attr = NULL;
	PyMyQttCtx     * self = (PyMyQttCtx *) o;
	axl_bool          boolean_value = axl_false;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return -1;

	if (axl_cmp (attr, "log")) {
		/* configure log */
		if (! PyArg_Parse (v, "i", &boolean_value))
			return -1;
		
		/* configure log */
		myqtt_log_enable (self->ctx, boolean_value);

		/* return operation ok */
		return 0;
	} else if (axl_cmp (attr, "log2")) {
		/* configure log2 */
		if (! PyArg_Parse (v, "i", &boolean_value))
			return -1;
		
		/* configure log */
		myqtt_log2_enable (self->ctx, boolean_value);

		/* return operation ok */
		return 0;

	} else if (axl_cmp (attr, "color_log")) {
		/* configure color_log */
		if (! PyArg_Parse (v, "i", &boolean_value))
			return -1;
		
		/* configure log */
		myqtt_color_log_enable (self->ctx, boolean_value);

		/* return operation ok */
		return 0;

	} /* end if */

	/* now implement generic setter */
	return PyObject_GenericSetAttr (o, attr_name, v);
}

typedef struct _PyMyQttEventData {
	int        id;
	PyObject * user_data;
	PyObject * user_data2;
	PyObject * handler;
	PyObject * ctx;
} PyMyQttEventData;

void py_myqtt_ctx_event_free (PyMyQttEventData * data)
{
	Py_DECREF (data->user_data);
	Py_DECREF (data->user_data2);
	Py_DECREF (data->handler);
	Py_DECREF (data->ctx);
	axl_free (data);
	return;
}

axl_bool py_myqtt_ctx_bridge_event (MyQttCtx * ctx, axlPointer user_data, axlPointer user_data2)
{
	/* reference to the python channel */
	PyObject           * args;
	PyGILState_STATE     state;
	PyObject           * result;
	axl_bool             _result;
	PyMyQttEventData  * data       = user_data;
	char               * str;

	/* check if myqtt engine is existing */
	if (myqtt_is_exiting (ctx)) {
		py_myqtt_log (PY_MYQTT_DEBUG, "disabled bridged event into python code, myqtt exiting=%d..",
			       myqtt_is_exiting (ctx));
		return axl_true;
	}

	/* check to skip events */
	if (PTR_TO_INT (myqtt_ctx_get_data (ctx, "py:vo:ctx:de"))) {
		py_myqtt_log (PY_MYQTT_DEBUG, "disabled bridged event into python code, myqtt exiting=%d due to key..",
			       myqtt_is_exiting (ctx));
		return axl_true;
	}

	/* acquire the GIL */
	/* py_myqtt_log2 (PY_MYQTT_DEBUG, "bridging event id=%d into python code (getting GIL) myqtt exiting=%d..", 
	   data->id, myqtt_is_exiting (ctx));   */
	state = PyGILState_Ensure();

	/* create a tuple to contain arguments */
	args = PyTuple_New (3);

	/* increase reference counting */
	Py_INCREF (data->ctx);
	PyTuple_SetItem (args, 0, data->ctx);

	Py_INCREF (data->user_data);
	PyTuple_SetItem (args, 1, data->user_data);

	Py_INCREF (data->user_data2);
	PyTuple_SetItem (args, 2, data->user_data2);

	/* record handler */
	START_HANDLER (data->handler);

	/* now invoke */
	result = PyObject_Call (data->handler, args, NULL);

	/* unrecord handler */
	CLOSE_HANDLER (data->handler);

	/* py_myqtt_log2 (PY_MYQTT_DEBUG, "event notification finished, checking for exceptions and result.."); */
	if (py_myqtt_handle_and_clear_exception (NULL)) {
		/* exception found */
		py_myqtt_log (PY_MYQTT_CRITICAL, "removing bridged event %d because an exception was found during its handling",
			       data->id);
		goto remove_event;
	}

	/* now get result value */
	_result = axl_false;
	if (! PyArg_Parse (result, "i", &_result)) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "failed to parse result from python event handler, requesting to remove handler");
	} /* end if */

	/* release tuple and result returned (which may be null) */
	Py_DECREF (args);
	Py_XDECREF (result);

	/* in the case the python code signaled to finish the event,
	 * terminate content inside ctx */
	if (_result || myqtt_is_exiting (ctx)) {

		py_myqtt_log (PY_MYQTT_DEBUG, "removing bridged event %d because myqtt exiting=%d or event_result=%d",
			       data->id, myqtt_is_exiting (ctx), _result);

	remove_event:

		/* call to remove event before returning */
		myqtt_thread_pool_remove_event (ctx, data->id);

		/* we have to remove the event, finish all data */
		str = axl_strdup_printf ("py:vo:event:%d", data->id);
		myqtt_ctx_set_data (ctx, str, NULL);
		axl_free (str);
	} /* end if */

	/* release the GIL */
	PyGILState_Release(state);

	/* return value from python handler */
	return _result;
}

static PyObject * py_myqtt_ctx_storage_set_path (PyObject * self, PyObject * args, PyObject * kwds)
{
	const char         * storage_path = NULL;
	int                  hash_size    = 4096;
	MyQttCtx           * ctx;

	/* now parse arguments */
	static char *kwlist[] = {"storage_path", "hash_size", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "si", kwlist, 
					   &storage_path,
					   &hash_size))
		return NULL;

	/* configure path */
	ctx = py_myqtt_ctx_get (self);
	myqtt_storage_set_path (ctx, storage_path, hash_size);

	/* return None */
	Py_INCREF (Py_None);
	return Py_None;
}

typedef struct _PyMyQttCtxSetOnPublishData {

	PyObject           * on_publish;
	PyObject           * on_publish_data;

} PyMyQttCtxSetOnPublishData;

MyQttPublishCodes py_myqtt_ctx_set_on_publish_handler (MyQttCtx     * ctx,
						       MyQttConn    * conn, 
						       MyQttMsg     * msg,
						       axlPointer     _on_publish_obj)
{
	PyMyQttCtxSetOnPublishData   * on_publish_obj = _on_publish_obj;
	PyGILState_STATE               state;
	PyObject                     * args;
	PyObject                     * result;
	PyObject                     * py_conn;
	MyQttPublishCodes              codes = MYQTT_PUBLISH_OK;

	/* notify on close notification received */
	py_myqtt_log (PY_MYQTT_DEBUG, "received on publish notification for conn id=%d, (internal: %p)", 
		      myqtt_conn_get_id (conn), _on_publish_obj);
	
	/*** bridge into python ***/
	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* create a tuple to contain arguments */
	args = PyTuple_New (4);

	/* param 0: ctx */
	PyTuple_SetItem (args, 0, py_myqtt_ctx_create (ctx));

	/* param 1: conn */
	py_conn = py_myqtt_conn_create (conn, axl_true, axl_false);
	PyTuple_SetItem (args, 1, py_conn);

	/* param 2: msg */
	PyTuple_SetItem (args, 2, py_myqtt_msg_create (msg, axl_true));

	/* param 3: on msg data */
	Py_INCREF (on_publish_obj->on_publish_data);
	PyTuple_SetItem (args, 3, on_publish_obj->on_publish_data);

	/* record handler */
	START_HANDLER (on_publish_obj->on_publish);

	/* now invoke */
	result = PyObject_Call (on_publish_obj->on_publish, args, NULL);

	/* unrecord handler */
	CLOSE_HANDLER (on_publish_obj->on_publish);

	py_myqtt_log (PY_MYQTT_DEBUG, "conn on publish notification finished, checking for exceptions..");
	if (! py_myqtt_handle_and_clear_exception (py_conn)) {
		/* no error was found */
		if (PyInt_Check (result)) {
			codes = PyInt_AsLong (result);
		} else {
			py_myqtt_log (PY_MYQTT_DEBUG, "on_publish handler is reporting something that is not an integer value (myqtt.PUBLISH_OK, myqtt.PUBLISH_DISCARD, myqtt.PUBLISH_CONN_CLOSE)");
		}
	} /* end if */

	Py_XDECREF (result);
	Py_DECREF (args);

	/* now release the rest of data */
	/* Py_DECREF (on_publish_obj->py_conn); */
	/* Py_DECREF (on_publish_obj->on_close);*/
	/* Py_DECREF (on_publish_obj->on_publish_data); */

	/* release the GIL */
	PyGILState_Release(state);

	return codes;
}


PyObject * py_myqtt_ctx_set_on_publish (PyObject * self, PyObject * args, PyObject * kwds)
{
	PyObject                     * on_publish        = NULL;
	PyObject                     * on_publish_data   = Py_None;
	PyMyQttCtxSetOnPublishData   * on_publish_obj;
	
	/* now parse arguments */
	static char *kwlist[] = {"on_publish", "on_publish_data", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &on_publish, &on_publish_data))
		return NULL;

	/* check handler received */
	if (on_publish == NULL || ! PyCallable_Check (on_publish)) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "received on_publish handler which is not a callable object");
		return NULL;
	} /* end if */

	/* configure an on msg handler to bridge into python. In this
	 * case we are reusing PyMyQttConnSetOnMsgData reference.  */
	on_publish_obj = axl_new (PyMyQttCtxSetOnPublishData, 1);
	if (on_publish_obj == NULL) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "received on_publish handler but unable to acquire memory required to store handlers during operations");
		return NULL;
	} /* end if */
		
	/* set reference to release it when the connection is
	 * closed */
	myqtt_ctx_set_data_full (py_myqtt_ctx_get (self),
				 axl_strdup_printf ("%p", on_publish_obj),
				 on_publish_obj,
				 axl_free, axl_free);

	/* configure on_close handler */
	on_publish_obj->on_publish = on_publish;
	Py_INCREF (on_publish);

	/* configure on_close_data handler data */
	if (on_publish_data == NULL)
		on_publish_data = Py_None;
	on_publish_obj->on_publish_data = on_publish_data;
	Py_INCREF (on_publish_data);

	/* now acquire a reference to the handler and the data to make
	 * them permanent during the execution of the script *and* to
	 * release them when finishing the connection */
	myqtt_ctx_set_data_full (py_myqtt_ctx_get (self),
				 axl_strdup_printf ("%p", on_publish),
				 on_publish,
				 axl_free,
				 (axlDestroyFunc) py_myqtt_decref);
	myqtt_ctx_set_data_full (py_myqtt_ctx_get (self),
				 axl_strdup_printf ("%p", on_publish_data),
				 on_publish_data,
				 axl_free,
				 (axlDestroyFunc) py_myqtt_decref);

	/* configure on publish handler */
	myqtt_ctx_set_on_publish (
		/* the ctx */
		py_myqtt_ctx_get (self),
		/* the handler */
		py_myqtt_ctx_set_on_publish_handler, 
		/* the object with all references */
		on_publish_obj);

	/* create a handle that allows to remove this particular
	   handler. This handler can be used to remove the on close
	   handler */
	return py_myqtt_handle_create (on_publish_obj, NULL);
}

typedef struct _PyMyQttCtxSetOnConnectData {

	PyObject           * on_connect;
	PyObject           * on_connect_data;

} PyMyQttCtxSetOnConnectData;

MyQttConnAckTypes py_myqtt_ctx_set_on_connect_handler (MyQttCtx     * ctx,
						       MyQttConn    * conn, 
						       axlPointer     _on_connect_obj)
{
	PyMyQttCtxSetOnConnectData   * on_connect_obj = _on_connect_obj;
	PyGILState_STATE               state;
	PyObject                     * args;
	PyObject                     * result;
	PyObject                     * py_conn;
	MyQttConnAckTypes              codes = MYQTT_CONNACK_ACCEPTED;

	/* notify on close notification received */
	py_myqtt_log (PY_MYQTT_DEBUG, "received on connect notification for conn id=%d, (internal: %p)", 
		      myqtt_conn_get_id (conn), _on_connect_obj);
	
	/*** bridge into python ***/
	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* create a tuple to contain arguments */
	args = PyTuple_New (3);

	/* param 0: ctx */
	PyTuple_SetItem (args, 0, py_myqtt_ctx_create (ctx));

	/* param 1: conn */
	py_conn = py_myqtt_conn_create (conn, axl_true, axl_false);
	PyTuple_SetItem (args, 1, py_conn);

	/* param 3: on msg data */
	Py_INCREF (on_connect_obj->on_connect_data);
	PyTuple_SetItem (args, 2, on_connect_obj->on_connect_data);

	/* record handler */
	START_HANDLER (on_connect_obj->on_connect);

	/* now invoke */
	result = PyObject_Call (on_connect_obj->on_connect, args, NULL);

	/* unrecord handler */
	CLOSE_HANDLER (on_connect_obj->on_connect);

	py_myqtt_log (PY_MYQTT_DEBUG, "conn on connect notification finished, checking for exceptions..");
	if (! py_myqtt_handle_and_clear_exception (py_conn)) {
		/* no error was found */

		if (PyInt_Check (result)) {
			codes = PyInt_AsLong (result);
		} else {
			py_myqtt_log (PY_MYQTT_DEBUG, "on_connect handler is reporting something that is not an integer value (myqtt.CONNECT_OK, myqtt.CONNECT_DISCARD, myqtt.CONNECT_CONN_CLOSE)");
		} /* end if */
	}

	Py_XDECREF (result);
	Py_DECREF (args);

	/* now release the rest of data */
	/* Py_DECREF (on_connect_obj->py_conn); */
	/* Py_DECREF (on_connect_obj->on_close);*/
	/* Py_DECREF (on_connect_obj->on_connect_data); */

	/* release the GIL */
	PyGILState_Release(state);

	return codes;
}

PyObject * py_myqtt_ctx_set_on_connect (PyObject * self, PyObject * args, PyObject * kwds)
{
	PyObject                     * on_connect        = NULL;
	PyObject                     * on_connect_data   = Py_None;
	PyMyQttCtxSetOnConnectData   * on_connect_obj;
	
	/* now parse arguments */
	static char *kwlist[] = {"on_connect", "on_connect_data", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &on_connect, &on_connect_data))
		return NULL;

	/* check handler received */
	if (on_connect == NULL || ! PyCallable_Check (on_connect)) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "received on_connect handler which is not a callable object");
		return NULL;
	} /* end if */

	/* configure an on msg handler to bridge into python. In this
	 * case we are reusing PyMyQttConnSetOnMsgData reference.  */
	on_connect_obj = axl_new (PyMyQttCtxSetOnConnectData, 1);
	if (on_connect_obj == NULL) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "received on_connect handler but unable to acquire memory required to store handlers during operations");
		return NULL;
	} /* end if */
		
	/* set reference to release it when the connection is
	 * closed */
	myqtt_ctx_set_data_full (py_myqtt_ctx_get (self),
				 axl_strdup_printf ("%p", on_connect_obj),
				 on_connect_obj,
				 axl_free, axl_free);

	/* configure on_close handler */
	on_connect_obj->on_connect = on_connect;
	Py_INCREF (on_connect);

	/* configure on_close_data handler data */
	if (on_connect_data == NULL)
		on_connect_data = Py_None;
	on_connect_obj->on_connect_data = on_connect_data;
	Py_INCREF (on_connect_data);

	/* now acquire a reference to the handler and the data to make
	 * them permanent during the execution of the script *and* to
	 * release them when finishing the connection */
	myqtt_ctx_set_data_full (py_myqtt_ctx_get (self),
				 axl_strdup_printf ("%p", on_connect),
				 on_connect,
				 axl_free,
				 (axlDestroyFunc) py_myqtt_decref);
	myqtt_ctx_set_data_full (py_myqtt_ctx_get (self),
				 axl_strdup_printf ("%p", on_connect_data),
				 on_connect_data,
				 axl_free,
				 (axlDestroyFunc) py_myqtt_decref);

	/* configure on connect handler */
	myqtt_ctx_set_on_connect (
		/* the ctx */
		py_myqtt_ctx_get (self),
		/* the handler */
		py_myqtt_ctx_set_on_connect_handler, 
		/* the object with all references */
		on_connect_obj);

	/* create a handle that allows to remove this particular
	   handler. This handler can be used to remove the on close
	   handler */
	return py_myqtt_handle_create (on_connect_obj, NULL);
}

/** 
 * @brief Allows to register a new callable event.
 */
static PyObject * py_myqtt_ctx_new_event (PyObject * self, PyObject * args, PyObject * kwds)
{
	PyObject           * handler      = NULL;
	PyObject           * user_data    = NULL;
	PyObject           * user_data2   = NULL;
	long                 microseconds = 0;         
	PyMyQttEventData  * data;

	/* now parse arguments */
	static char *kwlist[] = {"microseconds", "handler", "user_data", "user_data2", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "lO|OO", kwlist, 
					   &microseconds,
					   &handler, 
					   &user_data,
					   &user_data2))
		return NULL;

	py_myqtt_log (PY_MYQTT_DEBUG, "received request to register new event");

	/* check handlers defined */
	if (handler != NULL && ! PyCallable_Check (handler)) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "called to define a new event but provided a handler which is not callable");
		return NULL;
	} /* end if */

	if (microseconds <= 0) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "called to define a new event but provided a microseconds value which is less or equal to 0");
		return NULL;
	} /* end if */

	/* create data to hold all objects */
	data = axl_new (PyMyQttEventData, 1);
	if (data == NULL) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "failed to allocate memory for myqtt event..");
		return NULL;
	} /* end if */

	/* set references */
	PY_MYQTT_SET_REF (data->user_data,  user_data);
	PY_MYQTT_SET_REF (data->user_data2, user_data2);
	PY_MYQTT_SET_REF (data->handler,    handler);
	PY_MYQTT_SET_REF (data->ctx,        self);

	/* register the handler and get the id */
	data->id = myqtt_thread_pool_new_event (py_myqtt_ctx_get (self),
						 microseconds,
						 py_myqtt_ctx_bridge_event,
						 data, INT_TO_PTR(data->id));
	myqtt_ctx_set_data_full (py_myqtt_ctx_get (self), axl_strdup_printf ("py:vo:event:%d", data->id), data, axl_free, (axlDestroyFunc) py_myqtt_ctx_event_free);

	/* reply work done */
	py_myqtt_log (PY_MYQTT_DEBUG, "event registered with id %d", data->id);
	return Py_BuildValue ("i", data->id);
}

/** 
 * @brief Allows to register a new callable event.
 */
static PyObject * py_myqtt_ctx_remove_event (PyObject * self, PyObject * args, PyObject * kwds)
{
	int handle_id;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "i", &handle_id))
		return NULL;	

	/* call to remove handle */
	return Py_BuildValue ("i", myqtt_thread_pool_remove_event (py_myqtt_ctx_get (self), handle_id));
}

/** 
 * @brief Too long notification to file
 */
static PyObject * py_myqtt_ctx_enable_too_long_notify_to_file (PyObject * _self, PyObject * args, PyObject * kwds)
{
	int            watching_period = 0;
	const char   * file;
	PyMyQttCtx  * self = (PyMyQttCtx *) _self;
	/* now parse arguments */
	static char *kwlist[] = {"watching_period", "file", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "is", kwlist, 
					   &watching_period, &file))
		return NULL;

	py_myqtt_log (PY_MYQTT_DEBUG, "Enabling sending too long notifications to file %s with watching period %d", file, watching_period);

	/* call to enable the handle and return the result */
	return Py_BuildValue ("i", py_myqtt_ctx_log_too_long_notifications (self->ctx, watching_period, file));
}

static PyMethodDef py_myqtt_ctx_methods[] = { 
	/* init */
	{"init", (PyCFunction) py_myqtt_ctx_init, METH_NOARGS,
	 "Inits the MyQtt context starting all myqtt functions associated. This API call is required before using the rest of the MyQtt API."},
	/* exit */
	{"exit", (PyCFunction) py_myqtt_ctx_exit, METH_NOARGS,
	 "Finish the MyQtt context. This call must be the last one MyQtt API usage (for this context)."},
	/* storage_set_path */
	{"storage_set_path", (PyCFunction) py_myqtt_ctx_storage_set_path, METH_VARARGS | METH_KEYWORDS,
	 "Allows to configure storage path used by the provided content to hold messages in transit. This method implements support for myqtt_storage_set_path C API."},
	/* set_on_publish */
	{"set_on_publish", (PyCFunction) py_myqtt_ctx_set_on_publish, METH_VARARGS | METH_KEYWORDS,
	 "API wrapper for myqtt_ctx_set_on_publish. This method allows to configure a handler which will be called in case a message is received on the provided connection."},
	/* set_on_connect */
	{"set_on_connect", (PyCFunction) py_myqtt_ctx_set_on_connect, METH_VARARGS | METH_KEYWORDS,
	 "API wrapper for myqtt_ctx_set_on_connect. This method allows to configure a handler which will be called when a new connection is received (CONNECT packet received)."},
	/* new_event */
	{"new_event", (PyCFunction) py_myqtt_ctx_new_event, METH_VARARGS | METH_KEYWORDS,
	 "Function that allows to configure an asynchronous event calling to the handler defined, between the intervals defined. This function is the interface to myqtt_thread_pool_event_new."},
	/* remove_event */
	{"remove_event", (PyCFunction) py_myqtt_ctx_remove_event, METH_VARARGS | METH_KEYWORDS,
	 "Allows to remove an event installed with new_event() method using the handle id returned from that function. This function is the interface to myqtt_thread_pool_event_remove."},
	/* enable_too_long_notify_to_file */
	{"enable_too_long_notify_to_file", (PyCFunction) py_myqtt_ctx_enable_too_long_notify_to_file, METH_VARARGS | METH_KEYWORDS,
	 "Allows to activate a too long notification handler that will long into the provided file a notification every time is found a handler that is taking too long to finish."},
 	{NULL}  
}; 

static PyTypeObject PyMyQttCtxType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "myqtt.Ctx",              /* tp_name*/
    sizeof(PyMyQttCtx),       /* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_myqtt_ctx_dealloc, /* tp_dealloc*/
    0,                         /* tp_print*/
    0,                         /* tp_getattr*/
    0,                         /* tp_setattr*/
    0,                         /* tp_compare*/
    0,                         /* tp_repr*/
    0,                         /* tp_as_number*/
    0,                         /* tp_as_sequence*/
    0,                         /* tp_as_mapping*/
    0,                         /* tp_hash */
    0,                         /* tp_call*/
    0,                         /* tp_str*/
    py_myqtt_ctx_get_attr,    /* tp_getattro*/
    py_myqtt_ctx_set_attr,    /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "MyQtt context object required to function with MyQtt API",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_myqtt_ctx_methods,     /* tp_methods */
    0, /* py_myqtt_ctx_members, */     /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_myqtt_ctx_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_myqtt_ctx_new,         /* tp_new */

};

/** 
 * @brief Allows to create a PyMyQttCtx wrapper for the MyQttCtx
 * reference received.
 *
 * @param ctx The MyQttCtx reference to wrap.
 *
 * @return A newly created PyMyQttCtx reference.
 */
PyObject * py_myqtt_ctx_create (MyQttCtx * ctx)
{
	/* return a new instance */
	PyMyQttCtx * obj = (PyMyQttCtx *) PyObject_CallObject ((PyObject *) &PyMyQttCtxType, NULL);

	if (obj) {
		/* acquire a reference to the MyQttCtx if defined */
		if (ctx) {
			py_myqtt_log (PY_MYQTT_DEBUG, "found ctx reference defined, creating PyMyQttCtx reusing reference received (deallocating previous one)");
			/* free previous ctx */
			myqtt_ctx_free (obj->ctx);

			obj->ctx = ctx;
			myqtt_ctx_ref (ctx);
		} 

		/* flag to not exit once the ctx is deallocated */
		obj->exit_pending = axl_false;
		
		py_myqtt_log (PY_MYQTT_DEBUG, "created myqtt.Ctx (self: %p, self->ctx: %p)", 
			      obj, obj->ctx);
		
		return __PY_OBJECT (obj);
	} /* end if */

	/* failed to create object */
	return NULL;
}

/** 
 * @brief Allows to store a python object into the provided myqtt.Ctx
 * object, incrementing the reference count. The object is
 * automatically removed when the myqtt.Ctx reference is collected.
 */
void        py_myqtt_ctx_register (MyQttCtx  * ctx,
				    PyObject   * data,
				    const char * key,
				    ...)
{
	va_list    args;
	char     * full_key;

	/* check data received */
	if (key == NULL || ctx == NULL)
		return;

	va_start (args, key);
	full_key = axl_strdup_printfv (key, args);
	va_end   (args);

	/* check to remove */
	if (data == NULL) {
		myqtt_ctx_set_data (ctx, full_key, NULL);
		axl_free (full_key);
		return;
	} /* end if */
	
	/* now register the data received into the key created */
	py_myqtt_log (PY_MYQTT_DEBUG, "registering key %s = %p on myqtt.Ctx %p",
		       full_key, data, ctx);
	Py_INCREF (data);
	myqtt_ctx_set_data_full (ctx, full_key, data, axl_free, (axlDestroyFunc) py_myqtt_decref);
	return;
}


/** 
 * @brief Allows to get the object associated to the key provided. The
 * reference returned is still owned by the internal hash. Use
 * Py_INCREF in the case a new reference must owned by the caller.
 */
PyObject  * py_myqtt_ctx_register_get (MyQttCtx  * ctx,
				       const char * key,
				       ...)
{
	va_list    args;
	char     * full_key;
	PyObject * data;

	/* check data received */
	if (key == NULL || ctx == NULL) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "Failed to register data, key %p or myqtt.Ctx %p reference is null",
			       key, ctx);
		return NULL;
	} /* end if */

	va_start (args, key);
	full_key = axl_strdup_printfv (key, args);
	va_end   (args);
	
	/* now register the data received into the key created */
	data = __PY_OBJECT (myqtt_ctx_get_data (ctx, full_key));
	py_myqtt_log (PY_MYQTT_DEBUG, "returning key %s = %p on myqtt.Ctx %p",
		       full_key, data, ctx);
	axl_free (full_key);
	return data;
}

/** 
 * @brief Allows to check if the PyObject received represents a
 * PyMyQttCtx reference.
 */
axl_bool             py_myqtt_ctx_check    (PyObject          * obj)
{
	/* check null references */
	if (obj == NULL)
		return axl_false;

	/* return check result */
	return PyObject_TypeCheck (obj, &PyMyQttCtxType);
}

/** 
 * @brief Inits the myqtt ctx module. It is implemented as a type.
 */
void init_myqtt_ctx (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyMyQttCtxType) < 0)
		return;
	
	Py_INCREF (&PyMyQttCtxType);
	PyModule_AddObject(module, "Ctx", (PyObject *)&PyMyQttCtxType);

	return;
}

void __py_myqtt_ctx_set_to_release (MyQttCtx * ctx, PyObject * data)
{
	/* configure a reference to be released */
	myqtt_ctx_set_data_full (ctx,
				 axl_strdup_printf ("%p", data),
				 data,
				 axl_free,
				 (axlDestroyFunc) py_myqtt_decref);
	return;
}

