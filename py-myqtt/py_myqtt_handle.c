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


/* include header */
#include <py_myqtt_handle.h>

struct _PyMyQttHandle {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* reference to the pointer that this PyMyQttHandle wraps */
	axlPointer      data;

	/* reference to the destroy function that this PyMyQttHandle
	   optionally may use to cleanup associated pointer */
	axlDestroyFunc  data_destroy;
};

/** 
 * @brief Function used to finish and dealloc memory used by the object myqtt.Handle
 */
static void py_myqtt_handle_dealloc (PyMyQttHandle* self)
{
	py_myqtt_log (PY_MYQTT_DEBUG, "finishing PyMyQttHandle id: %p", self);
	
	/* call to destroy */
	if (self->data_destroy) {
		/* allow threads */
		Py_BEGIN_ALLOW_THREADS

		/* call user code */
		self->data_destroy (self->data);

		/* end threads */
		Py_END_ALLOW_THREADS
	}
	self->data         = NULL;
	self->data_destroy = NULL;

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	py_myqtt_log (PY_MYQTT_DEBUG, "terminated PyMyQttHandle dealloc with id: %p)", self);

	return;
}

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_myqtt_handle_get_attr (PyObject *o, PyObject *attr_name) {
	const char         * attr = NULL;
	PyObject           * result;
	/* PyMyQttHandle * self = (PyMyQttHandle *) o; */

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

static int py_myqtt_handle_init_type (PyMyQttHandle *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static PyMethodDef py_myqtt_handle_methods[] = { 
 	{NULL}  
}; 

/** 
 * @brief Function used to allocate memory required by the object myqtt.Handle
 */
static PyObject * py_myqtt_handle_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	/* create the object */
	return type->tp_alloc(type, 0);
}

static PyTypeObject PyMyQttHandleType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "myqtt.Handle",       /* tp_name*/
    sizeof(PyMyQttHandle),/* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_myqtt_handle_dealloc, /* tp_dealloc*/
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
    py_myqtt_handle_get_attr, /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "myqtt.Handle, the object used to represent a generic pointer used by Myqtt API.",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_myqtt_handle_methods,     /* tp_methods */
    0, /* py_myqtt_handle_members, */ /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_myqtt_handle_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_myqtt_handle_new,      /* tp_new */

};

void                 init_myqtt_handle        (PyObject           * module)
{
	/* register type */
	if (PyType_Ready(&PyMyQttHandleType) < 0)
		return;
	
	Py_INCREF (&PyMyQttHandleType);
	PyModule_AddObject(module, "Handle", (PyObject *)&PyMyQttHandleType);

	return;
}

/** 
 * @brief Allows to get the reference stored inside the provided myqtt.Handle.
 *
 * @param obj The myqtt.Handle that contains the pointer to return.
 *
 * @return The pointer stored (including NULL).
 */
axlPointer         py_myqtt_handle_get      (PyObject           * obj)
{
	PyMyQttHandle * handle = (PyMyQttHandle *) obj;
	if (handle == NULL)
		return NULL;
	/* return stored pointer */
	return handle->data;
}

/** 
 * @brief Cleanup the provided PyMyQttHandle to erase its internal
 * pointer and internal destroy function.
 */
void                 py_myqtt_handle_nullify  (PyObject           * obj)
{
	PyMyQttHandle * handle = (PyMyQttHandle *) obj;
	if (handle == NULL)
		return;
	py_myqtt_log (PY_MYQTT_DEBUG, "nullifying myqtt.Handle (%p)", obj);
	/* clear handles */
	handle->data         = NULL;
	handle->data_destroy = NULL;

	return;
}

/** 
 * @brief Allows to check if the PyObject received represents a
 * PyMyQttHandle reference.
 */
axl_bool             py_myqtt_handle_check    (PyObject           * obj)
{
	/* check null references */
	if (obj == NULL)
		return axl_false;

	/* return check result */
	return PyObject_TypeCheck (obj, &PyMyQttHandleType);
}

/** 
 * @brief Creates a new empty PyMyQttHandle holding the provided
 * pointer and destroy handler.
 *
 * @param data A user defined pointer to the data that will be hold by this myqtt.Handle
 *
 * @param data_destroy An optional destroy function used to terminate
 * the handle.
 */
PyObject         * py_myqtt_handle_create   (axlPointer           data,
					      axlDestroyFunc       data_destroy)
{
	/* return a new instance */
	PyMyQttHandle * obj = (PyMyQttHandle *) PyObject_CallObject ((PyObject *) &PyMyQttHandleType, NULL); 

	/* set references */
	obj->data         = data;
	obj->data_destroy = data_destroy;

	py_myqtt_log (PY_MYQTT_DEBUG, "creating myqtt.Handle (%p), storing %p", obj, data);

	return __PY_OBJECT (obj);
}


