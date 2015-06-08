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

/* include base library */
#include <py_myqtt_tls.h>

/* include myqtt tls support */
#include <myqtt-tls.h>

/** 
 * @brief Allows to init myqtt.tls module.
 */
static PyObject * py_myqtt_tls_init (PyObject * self, PyObject * args)
{
	PyObject * py_ctx = NULL;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "O", &py_ctx))
		return NULL;

	/* check connection object */
	if (! py_myqtt_ctx_check (py_ctx)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a myqtt.Ctx object but received something different");
		return NULL;
	} /* end if */
	
	/* call to return value */
	return Py_BuildValue ("i", myqtt_tls_init (py_myqtt_ctx_get (py_ctx)));
}

static PyObject * py_myqtt_tls_create_conn (PyObject * self, PyObject * args, PyObject * kwds)
{
	const char         * client_identifier = NULL;
	axl_bool             clean_session     = axl_false;
	int                  keep_alive        = 0;

	const char         * host       = NULL;
	const char         * port       = NULL;
	PyObject           * py_myqtt_ctx = NULL;
	PyObject           * conn_opts  = NULL;
	MyQttConn          * conn;
	PyObject           * py_conn;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", "host", "port", "client_identifier", "clean_session", "keep_alive", "conn_opts", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OssziiO", kwlist, 
					  &py_myqtt_ctx, &host, &port, &client_identifier, &clean_session, &keep_alive, &conn_opts)) 
		return NULL;

	/* allow other threads to enter into the python space */
	Py_BEGIN_ALLOW_THREADS 

	/* call to authenticate in a blocking manner */
	conn = myqtt_tls_conn_new (py_myqtt_ctx_get (py_myqtt_ctx), client_identifier, 
				   clean_session, keep_alive, host, port, 
				   conn_opts ? py_myqtt_conn_opts_get (conn_opts) : NULL,
				   NULL, NULL);

	/* restore thread state */
	Py_END_ALLOW_THREADS 

	py_conn = py_myqtt_conn_create (conn, 
					/* acquire reference */
					axl_false,
					/* close connection on deallocation */
					axl_true);

	return py_conn;
}

static PyObject * py_myqtt_tls_create_listener (PyObject * self, PyObject * args, PyObject * kwds)
{
	const char         * host       = NULL;
	const char         * port       = NULL;
	PyObject           * py_myqtt_ctx = NULL;
	PyObject           * conn_opts  = NULL;
	MyQttConn          * conn;
	PyObject           * py_conn;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", "host", "port", "conn_opts", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OssO", kwlist, 
					  &py_myqtt_ctx, &host, &port, &conn_opts)) 
		return NULL;

	/* allow other threads to enter into the python space */
	Py_BEGIN_ALLOW_THREADS 

	/* call to authenticate in a blocking manner */
	conn = myqtt_tls_listener_new (py_myqtt_ctx_get (py_myqtt_ctx), 
				       host, port, 
				       conn_opts ? py_myqtt_conn_opts_get (conn_opts) : NULL,
				       NULL, NULL);

	/* restore thread state */
	Py_END_ALLOW_THREADS 

	py_conn = py_myqtt_conn_create (conn, 
					/* acquire reference */
					axl_true,
					/* close connection on deallocation */
					axl_true);

	return py_conn;
}


static PyObject * py_myqtt_tls_set_certificate (PyObject * self, PyObject * args, PyObject * kwds)
{
	const char         * certificate  = NULL;
	const char         * private_key  = NULL;
	const char         * chain_file   = NULL;

	PyObject           * py_listener = NULL;

	/* now parse arguments */
	static char *kwlist[] = {"listener", "certificate", "private_key", "chain_file", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oss|s", kwlist, 
					  &py_listener, &certificate, &private_key, &chain_file)) 
		return NULL;

	/* allow other threads to enter into the python space */
	Py_BEGIN_ALLOW_THREADS 

	/* configure certificates */
	myqtt_tls_set_certificate (py_myqtt_conn_get (py_listener), 
				   certificate, private_key, chain_file);

	/* restore thread state */
	Py_END_ALLOW_THREADS 

	/* return none */
	Py_INCREF (Py_None);
	return Py_None;
}

typedef struct PyMyQttTlsCertificateHandlersData {
	PyObject           * certificate_handler;
	PyObject           * private_key_handler;
	PyObject           * chain_handler;
	PyObject           * data;
} PyMyQttTlsCertificateHandlersData;

__py_myqtt_tls_certificate_handler,


static PyObject * py_myqtt_tls_set_certificate_handlers (PyObject * self, PyObject * args, PyObject * kwds)
{
	PyObject           * certificate_handler;
	PyObject           * private_key_handler;
	PyObject           * chain_handler;
	PyObject           * data;
	PyObject           * py_ctx = NULL;

	PyMyQttTlsCertificateHandlersData * on_tls_handlers;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", "certificate_handler", "private_key_handler", "chain_handler", "data", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO", kwlist, 
					  &py_listener, &certificate, &private_key, &chain_file)) 
		return NULL;

	/* get some memory to hold data received */
	on_tls_handlers = axl_new (PyMyQttTlsCertificateHandlersData, 1);
	if (on_tls_handlers == NULL) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "received set_certificate_handlers call but unable to acquire memory required to store handlers during operations");
		return NULL;
	} /* end if */

	/* set reference to release it when the connection is
	 * closed */
	myqtt_ctx_set_data_full (py_myqtt_ctx_get (self),
				 axl_strdup_printf ("%p", on_tls_handlers),
				 on_tls_handlers,
				 axl_free, axl_free);

	/* configure  certificate_handler */
	on_tls_handlers->certificate_handler = certificate_handler;
	Py_INCREF (certificate_handler);

	/* configure  private_key_handler */
	on_tls_handlers->private_key_handler = private_key_handler;
	Py_INCREF (private_key_handler);

	/* configure  chain_handler */
	if (chain_handler == NULL)
		chain_handler = Py_None;
	on_tls_handlers->chain_handler = chain_handler;
	Py_INCREF (chain_handler);

	/* configure on_close_data handler data */
	if (data == NULL)
		data = Py_None;
	on_tls_handlers->data = data
	Py_INCREF (data);

	/* configure code to release these references when finished */
	__py_myqtt_ctx_set_to_release (py_myqtt_ctx_get (ctx), certificate_handler);
	__py_myqtt_ctx_set_to_release (py_myqtt_ctx_get (ctx), private_key_handler);
	__py_myqtt_ctx_set_to_release (py_myqtt_ctx_get (ctx), chain_handler);
	__py_myqtt_ctx_set_to_release (py_myqtt_ctx_get (ctx), data);

	/* now configure these handlers */
	myqtt_tls_listener_set_certificate_handlers (py_myqtt_ctx_get (ctx),
						     __py_myqtt_tls_certificate_handler,
						     __py_myqtt_tls_private_key_handler,
						     __py_myqtt_tls_chain_handler,
						     on_tls_handlers);

	/* return none */
	Py_INCREF (Py_None);
	return Py_None;
}



typedef struct _PyMyQttTlsAcceptData {
	PyObject     * ctx;
	PyObject     * accept_handler;
	PyObject     * accept_handler_data;
	PyObject     * cert_handler;
	PyObject     * cert_handler_data;
	PyObject     * key_handler;
	PyObject     * key_handler_data;
} PyMyQttTlsAcceptData;

void py_myqtt_tls_data_free (PyMyQttTlsAcceptData * data)
{
	if (data == NULL)
		return;

	/* unref handlers (if defined) */
	Py_XDECREF (data->accept_handler);
	Py_XDECREF (data->accept_handler_data);

	Py_XDECREF (data->cert_handler);
	Py_XDECREF (data->cert_handler_data);

	Py_XDECREF (data->key_handler);
	Py_XDECREF (data->key_handler_data);

	/* free the node itself */
	axl_free (data);
	return;
}

static PyObject * py_myqtt_tls_is_on (PyObject * self, PyObject * args)
{
	PyObject * py_conn = NULL;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "O", &py_conn))
		return NULL;

	/* check connection object */
	if (! py_myqtt_conn_check (py_conn)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a myqtt.Connection object but received something different");
		return NULL;
	} /* end if */
	
	/* call to return value */
	return Py_BuildValue ("i", myqtt_tls_is_on (py_myqtt_conn_get (py_conn)));
}

static PyObject * py_myqtt_tls_ssl_peer_verify (PyObject * self, PyObject * args)
{
	PyObject * py_conn_opts = NULL;
	axl_bool   disable      = axl_false;
	

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "O|i", &py_conn_opts, &disable))
		return NULL;

	/* check connection object */
	if (! py_myqtt_conn_opts_check (py_conn_opts)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a myqtt.ConnOpts object but received something different");
		return NULL;
	} /* end if */

	/* configure ssl peer verification */
	myqtt_tls_opts_ssl_peer_verify (py_myqtt_conn_opts_get (py_conn_opts), disable);
	
	/* return none */
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject * py_myqtt_tls_set_ssl_certs (PyObject * self, PyObject * args)
{
	PyObject * py_conn_opts = NULL;
	const char     * certificate = NULL;
	const char     * private_key = NULL;
	const char     * chain_certificate = NULL;
	const char     * ca_certificate = NULL;
	
	/* parse and check result */
	if (! PyArg_ParseTuple (args, "O|sszz", &py_conn_opts, &certificate, &private_key, &chain_certificate, &ca_certificate))
		return NULL;

	/* check connection object */
	if (! py_myqtt_conn_opts_check (py_conn_opts)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a myqtt.ConnOpts object but received something different");
		return NULL;
	} /* end if */

	/* configure ssl peer verification */
	myqtt_tls_opts_set_ssl_certs (py_myqtt_conn_opts_get (py_conn_opts), 
				      certificate, private_key, 
				      chain_certificate, ca_certificate);
	/* return none */
	Py_INCREF (Py_None);
	return Py_None;
}


static PyObject * py_myqtt_tls_verify_cert (PyObject * self, PyObject * args)
{
	PyObject * py_conn = NULL;
	axl_bool   enable  = axl_true;
       

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "O", &py_conn, &enable))
		return NULL;

	/* check connection object */
	if (! py_myqtt_conn_check (py_conn)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a myqtt.Connection object but received something different");
		return NULL;
	} /* end if */
	
	/* call to return value */
	return Py_BuildValue ("i", myqtt_tls_verify_cert (py_myqtt_conn_get (py_conn)));
}


static PyMethodDef py_myqtt_tls_methods[] = { 
	/* init */
	{"init", (PyCFunction) py_myqtt_tls_init, METH_VARARGS,
	 "Inits myqtt.tls module (a wrapper to myqtt_tls_init function)."},
	/* create_conn */
	{"create_conn", (PyCFunction) py_myqtt_tls_create_conn, METH_VARARGS | METH_KEYWORDS,
	 "Create MQTT TLS connection (myqtt_tls_conn_new wrapper)."},
	/* create_listener */
	{"create_listener", (PyCFunction) py_myqtt_tls_create_listener, METH_VARARGS | METH_KEYWORDS,
	 "Create MQTT TLS listener (myqtt_tls_listener_new wrapper)."},
	/* set_certificate */
	{"set_certificate", (PyCFunction) py_myqtt_tls_set_certificate, METH_VARARGS | METH_KEYWORDS,
	 "Allows to configure listener TLS certificates (myqtt_tls_set_certificate wrapper)."},
	/* set_certificate_handlers */
	{"set_certificate_handlers", (PyCFunction) py_myqtt_tls_set_certificate_handlers, METH_VARARGS | METH_KEYWORDS,
	 "Allows to configure a set of handlers that will be called to find the location of the certificates (myqtt_tls_listener_set_certificate_handlers wrapper)."},
	/* in_on */
	{"is_on", (PyCFunction) py_myqtt_tls_is_on, METH_VARARGS | METH_KEYWORDS,
	 "Allows to check if the provided connection running TLS protocol (see myqtt_conn_tls_is_on)."},
	/* ssl_peer_verify */
	{"ssl_peer_verify", (PyCFunction) py_myqtt_tls_ssl_peer_verify, METH_VARARGS | METH_KEYWORDS,
	 "Allows to disable/enable ssl peer verification on the provided myqtt.ConnOpts."},
	/* set_ssl_certs */
	{"set_ssl_certs", (PyCFunction) py_myqtt_tls_set_ssl_certs, METH_VARARGS | METH_KEYWORDS,
	 "Allows to configure set of certificates on the provided myqtt.ConnOpts."},
	{"verify_cert", (PyCFunction) py_myqtt_tls_verify_cert, METH_VARARGS | METH_KEYWORDS,
	 "Allows to check peer certificate verify status  (see myqtt_tls_verify_cert)."},
	/* is_authenticated */
	{NULL, NULL, 0, NULL}   /* sentinel */
}; 

PyMODINIT_FUNC initlibpy_myqtt_tls_10 (void)
{
	PyObject * module;

	/* call to initilize threading API and to acquire the lock */
	PyEval_InitThreads();

	/* register myqtt module */
	module = Py_InitModule3 ("libpy_myqtt_tls_10", py_myqtt_tls_methods, 
				 "TLS binding support for myqtt library TLS profile");
	if (module == NULL) {
		py_myqtt_log (PY_MYQTT_CRITICAL, "failed to create tls module");
		return;
	} /* end if */

	py_myqtt_log (PY_MYQTT_DEBUG, "Tls module started");

	return;
}


