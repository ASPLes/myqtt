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

#include <py_myqtt_msg.h>

struct _PyMyQttMsg {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the MyQttMsg object */
	MyQttMsg * msg;
};

static int py_myqtt_msg_init_type (PyMyQttMsg *self, PyObject *args, PyObject *kwds)
{
    return 0;
}


/** 
 * @brief Function used to allocate memory required by the object myqtt.Msg
 */
static PyObject * py_myqtt_msg_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyMyQttMsg    * self;

	/* create the object */
	self = (PyMyQttMsg *)type->tp_alloc(type, 0);

	/* and do nothing more */
	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object myqtt.Msg
 */
static void py_myqtt_msg_dealloc (PyMyQttMsg* self)
{
	/* unref the msg */
	myqtt_msg_unref (self->msg);
	self->msg = NULL;

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}


/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_myqtt_msg_get_attr (PyObject *o, PyObject *attr_name) {
	const char         * attr = NULL;
	PyObject           * result;
	PyMyQttMsg * self = (PyMyQttMsg *) o;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	/* printf ("received request to return attribute value of '%s'..\n", attr); */

	if (axl_cmp (attr, "id")) {
		/* get id attribute */
		return Py_BuildValue ("i", myqtt_msg_get_id (self->msg));
	} else if (axl_cmp (attr, "type")) {
		/* return string type */
		return Py_BuildValue ("s", myqtt_msg_get_type_str (self->msg));
	} else if (axl_cmp (attr, "size")) {
		/* get msg_size attribute */
		return Py_BuildValue ("i", myqtt_msg_get_app_msg_size (self->msg));
	} else if (axl_cmp (attr, "content")) {
		/* get payload attribute */
		return Py_BuildValue ("z#", myqtt_msg_get_app_msg (self->msg), myqtt_msg_get_app_msg_size (self->msg));
	} else if (axl_cmp (attr, "payload_size")) {
		/* get payload_size attribute */
		return Py_BuildValue ("i", myqtt_msg_get_payload_size (self->msg));
	} else if (axl_cmp (attr, "payload")) {
		/* get payload attribute */
		return Py_BuildValue ("z#", myqtt_msg_get_payload (self->msg), myqtt_msg_get_payload_size (self->msg));
	} else if (axl_cmp (attr, "topic")) {
		/* get message topic */
		return Py_BuildValue ("z", myqtt_msg_get_topic (self->msg));
	} else if (axl_cmp (attr, "qos")) {
		/* get message qos */
		return Py_BuildValue ("i", myqtt_msg_get_qos (self->msg));
	} /* end if */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

/* no methods */
/* static PyMethodDef py_myqtt_msg_methods[] = { 
 	{NULL}  
}; */


static PyTypeObject PyMyQttMsgType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "myqtt.Msg",       /* tp_name*/
    sizeof(PyMyQttMsg),/* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_myqtt_msg_dealloc, /* tp_dealloc*/
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
    py_myqtt_msg_get_attr, /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "myqtt.Msg, the object used to represent a MQTT msg.",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0, /*py_myqtt_msg_methods,*/     /* tp_methods */
    0, /* py_myqtt_msg_members, */ /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_myqtt_msg_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_myqtt_msg_new,  /* tp_new */
};

/** 
 * @brief Creates new empty msg instance. The function acquire a new
 * reference to the msg (myqtt_msg_unref) because it us assumed
 * the caller is inside a msg received handler or a similar handler
 * where it is assumed the msg will be no longer available after
 * handler has finished.
 *
 * In the case you are wrapping a msg and you already own a
 * reference to them, you can set acquire_ref to axl_false.
 *
 * @param msg The msg to wrap creating a PyMyQttMsg reference.
 *
 * @param acquire_ref Signal the function to acquire a reference to
 * the myqtt_msg_ref making the PyMyQttMsg returned to "own" a
 * reference to the msg. In the case acquire_ref is axl_false, the
 * caller is telling the function to "steal" a reference from the
 * msg.
 *
 * @return A newly created PyMyQttMsg reference, casted to PyObject.
 */
PyObject * py_myqtt_msg_create (MyQttMsg * msg, axl_bool acquire_ref)
{
	/* return a new instance */
	PyMyQttMsg * obj = (PyMyQttMsg *) PyObject_CallObject ((PyObject *) &PyMyQttMsgType, NULL);

	/* set msg reference received */
	if (obj && msg) {
		if (acquire_ref) {
			/* increase reference counting */
			myqtt_msg_ref (msg);
		} /* end if */

		/* acquire reference */
		obj->msg = msg;
	} /* end if */

	/* return object */
	return (PyObject *) obj;
}

/** 
 * @brief Inits the myqtt msg module. It is implemented as a type.
 */
void init_myqtt_msg (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyMyQttMsgType) < 0)
		return;
	
	Py_INCREF (&PyMyQttMsgType);
	PyModule_AddObject(module, "Msg", (PyObject *)&PyMyQttMsgType);

	return;
}

/** 
 * @brief Allows to get the MyQttMsg reference that is stored
 * inside the python msg reference received.
 *
 * @param msg The python wrapper that contains the msg to be returned.
 *
 * @return A reference to the msg inside the python msg.
 */
MyQttMsg * py_myqtt_msg_get    (PyMyQttMsg * msg)
{
	/* check null values */
	if (msg == NULL)
		return NULL;

	/* return the msg reference inside */
	return msg->msg;
}


