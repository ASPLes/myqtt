/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2015 Advanced Software Production Line, S.L.
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


#ifndef __PY_MYQTT_CONN_H__
#define __PY_MYQTT_CONN_H__

/** 
 * @brief Type representation for the python object that holds a
 * MyQttConn inside it.
 */
typedef struct _PyMyQttConn PyMyQttConn;

/* include base header */
#include <py_myqtt.h>

/** 
 * @brief Cast a PyObject reference into a PyMyQttConn.
 */
#define PY_MYQTT_CONN(c) ((PyMyQttConn *)c)

void                 init_myqtt_conn        (PyObject           * module);

MyQttConn          * py_myqtt_conn_get      (PyObject           * py_conn);

PyObject           * py_myqtt_conn_create   (MyQttConn   * conn, 
					     axl_bool             acquire_ref,
					     axl_bool             close_ref);

PyObject           * py_myqtt_conn_shutdown (PyMyQttConn * self);

void                 py_myqtt_conn_nullify  (PyObject           * py_conn);

axl_bool             py_myqtt_conn_check    (PyObject           * obj);

void                 py_myqtt_conn_register (PyObject   * py_conn, 
						    PyObject   * data,
						    const char * key,
						    ...);

PyObject           * py_myqtt_conn_register_get (PyObject * py_conn,
							const char * key,
							...);

PyObject           * py_myqtt_conn_find_reference (MyQttConn * conn);

#define PY_CONN_GET(py_obj) (((PyMyQttConn*)self)->conn)

#endif
