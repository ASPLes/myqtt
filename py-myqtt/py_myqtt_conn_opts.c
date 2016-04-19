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

#include <py_myqtt_conn_opts.h>

struct _PyMyQttConnOpts {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the MyQttConnOpts object */
	MyQttConnOpts * conn_opts;
};

static int py_myqtt_conn_opts_init_type (PyMyQttConnOpts *self, PyObject *args, PyObject *kwds)
{
    return 0;
}


/** 
 * @brief Function used to allocate memory required by the object myqtt.ConnOpts
 */
static PyObject * py_myqtt_conn_opts_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyMyQttConnOpts    * self;

	/* create the object */
	self = (PyMyQttConnOpts *)type->tp_alloc(type, 0);

	/* create empty options */
	self->conn_opts = myqtt_conn_opts_new ();

	/* and do nothing more */
	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object myqtt.ConnOpts
 */
static void py_myqtt_conn_opts_dealloc (PyMyQttConnOpts* self)
{
	/* unref the conn_opts */
	/* myqtt_conn_opts_free (self->conn_opts); */
	self->conn_opts = NULL;

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}


/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_myqtt_conn_opts_get_attr (PyObject *o, PyObject *attr_name) {
	const char         * attr = NULL;
	PyObject           * result;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;


	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

PyObject * py_myqtt_conn_opts_set_auth (PyObject * self, PyObject * args, PyObject * kwds)
{
	const char     * username;
	const char     * password;
	MyQttConnOpts  * conn_opts;
		
	/* now parse arguments */
	static char *kwlist[] = {"username", "password", NULL};

	/* get connection options */
	conn_opts = py_myqtt_conn_opts_get (self);
	if (! conn_opts) {
		PyErr_SetString (PyExc_ValueError, "Expected to receive a myqtt.ConnOpts not used or with reuse option enabled but found internal reference empty");
		return NULL;
	} /* end if */

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "ss", kwlist, &username, &password))
		return NULL;
	
	/* configure options */
	myqtt_conn_opts_set_auth (conn_opts, username, password);

	/* done, return ok */
	Py_INCREF (Py_None);
	return Py_None;
}

PyObject * py_myqtt_conn_opts_set_will (PyObject * self, PyObject * args, PyObject * kwds)
{
	MyQttQos         will_qos;
	const char     * will_topic;
	const char     * will_message;
	axl_bool         will_retain;
	MyQttConnOpts  * conn_opts;
		
	/* now parse arguments */
	static char *kwlist[] = {"will_qos", "will_topic", "will_message", "will_retain", NULL};

	/* get connection options */
	conn_opts = py_myqtt_conn_opts_get (self);
	if (! conn_opts) {
		PyErr_SetString (PyExc_ValueError, "Expected to receive a myqtt.ConnOpts not used or with reuse option enabled but found internal reference empty");
		return NULL;
	} /* end if */

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "iszi", kwlist, &will_qos, &will_topic, &will_message, &will_retain))
		return NULL;
	
	/* configure options */
	myqtt_conn_opts_set_will (conn_opts, will_qos, will_topic, will_message, will_retain);

	/* done, return ok */
	Py_INCREF (Py_None);
	return Py_None;
}

PyObject * py_myqtt_conn_opts_set_reconnect (PyObject * self, PyObject * args, PyObject * kwds)
{
	axl_bool         reconnect = axl_true;
	MyQttConnOpts  * conn_opts;
		
	/* now parse arguments */
	static char *kwlist[] = {"reconnect", NULL};

	/* get connection options */
	conn_opts = py_myqtt_conn_opts_get (self);
	if (! conn_opts) {
		PyErr_SetString (PyExc_ValueError, "Expected to receive a myqtt.ConnOpts not used or with reuse option enabled but found internal reference empty");
		return NULL;
	} /* end if */

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &reconnect))
		return NULL;
	
	/* configure options */
	myqtt_conn_opts_set_reconnect (conn_opts, reconnect);

	/* done, return ok */
	Py_INCREF (Py_None);
	return Py_None;
}


static PyMethodDef py_myqtt_conn_opts_methods[] = { 
	/* set_auth */
	{"set_auth", (PyCFunction) py_myqtt_conn_opts_set_auth, METH_VARARGS | METH_KEYWORDS,
	 "API wrapper for myqtt_conn_opts_set_auth. This method allows to configure auth options on the provided connection options."},
	/* set_will */
	{"set_will", (PyCFunction) py_myqtt_conn_opts_set_will, METH_VARARGS | METH_KEYWORDS,
	 "API wrapper for myqtt_conn_opts_set_will. This method allows to configure will options for the provided connection."},
	/* set_reconnect */
	{"set_reconnect", (PyCFunction) py_myqtt_conn_opts_set_reconnect, METH_VARARGS | METH_KEYWORDS,
	 "API wrapper for myqtt_conn_opts_set_reconnect. This method allows to configure automatic reconnect option for the provided connection."},
 	{NULL} 
}; 


static PyTypeObject PyMyQttConnOptsType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "myqtt.ConnOpts",       /* tp_name*/
    sizeof(PyMyQttConnOpts),/* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_myqtt_conn_opts_dealloc, /* tp_dealloc*/
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
    py_myqtt_conn_opts_get_attr, /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "myqtt.ConnOpts, the object used to represent a MQTT conn_opts.",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_myqtt_conn_opts_methods,     /* tp_methods */
    0, /* py_myqtt_conn_opts_members, */ /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_myqtt_conn_opts_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_myqtt_conn_opts_new,  /* tp_new */
};

/** 
 * @brief Creates new empty conn_opts instance. The function acquire a new
 * reference to the conn_opts (myqtt_conn_opts_unref) because it us assumed
 * the caller is inside a conn_opts received handler or a similar handler
 * where it is assumed the conn_opts will be no longer available after
 * handler has finished.
 *
 * In the case you are wrapping a conn_opts and you already own a
 * reference to them, you can set acquire_ref to axl_false.
 *
 * @param conn_opts The conn_opts to wrap creating a PyMyQttConnOpts reference.
 *
 * @param acquire_ref Signal the function to acquire a reference to
 * the myqtt_conn_opts_ref making the PyMyQttConnOpts returned to "own" a
 * reference to the conn_opts. In the case acquire_ref is axl_false, the
 * caller is telling the function to "steal" a reference from the
 * conn_opts.
 *
 * @return A newly created PyMyQttConnOpts reference, casted to PyObject.
 */
PyObject * py_myqtt_conn_opts_create (MyQttConnOpts * conn_opts)
{
	/* return a new instance */
	PyMyQttConnOpts * obj = (PyMyQttConnOpts *) PyObject_CallObject ((PyObject *) &PyMyQttConnOptsType, NULL);

	/* set conn_opts reference received */
	if (obj && conn_opts) {
			/* acquire reference */
		obj->conn_opts = conn_opts;
	} /* end if */

	/* return object */
	return (PyObject *) obj;
}

/** 
 * @brief Allows to check if the PyObject received represents a
 * PyMyQttConnOpts reference.
 */
axl_bool             py_myqtt_conn_opts_check    (PyObject          * obj)
{
	/* check null references */
	if (obj == NULL)
		return axl_false;

	/* return check result */
	return PyObject_TypeCheck (obj, &PyMyQttConnOptsType);
}

/** 
 * @brief Inits the myqtt conn_opts module. It is implemented as a type.
 */
void init_myqtt_conn_opts (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyMyQttConnOptsType) < 0)
		return;
	
	Py_INCREF (&PyMyQttConnOptsType);
	PyModule_AddObject(module, "ConnOpts", (PyObject *)&PyMyQttConnOptsType);

	return;
}

/** 
 * @brief Allows to get the MyQttConnOpts reference that is stored
 * inside the python conn_opts reference received.
 *
 * @param conn_opts The python wrapper that contains the conn_opts to be returned.
 *
 * @return A reference to the conn_opts inside the python conn_opts.
 */
MyQttConnOpts * py_myqtt_conn_opts_get    (PyObject * conn_opts)
{
	PyMyQttConnOpts * _conn_opts = (PyMyQttConnOpts *) conn_opts;

	/* check null values */
	if (conn_opts == NULL)
		return NULL;

	/* return the conn_opts reference inside */
	return _conn_opts->conn_opts;
}


