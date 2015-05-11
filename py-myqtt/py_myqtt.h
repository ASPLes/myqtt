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


#ifndef __PY_MYQTT_H__
#define __PY_MYQTT_H__

/* include python devel headers */
#include <Python.h>

/* include myqtt headers */
#include <myqtt.h>

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

/** 
 * @brief PyMyQtt macro used to cast objects to PyObject.
 */
#define __PY_OBJECT(o) ((PyObject *)o)

/** 
 * @brief Enum used to configure debug level used by the binding.
 */
typedef enum {
	/** 
	 * @brief Debug and information messages.
	 */
	PY_MYQTT_DEBUG   = 1,
	/** 
	 * @brief Warning messages
	 */
	PY_MYQTT_WARNING = 2,
	/** 
	 * @brief Critical messages.
	 */
	PY_MYQTT_CRITICAL   = 3,
} PyMyQttLog;

/* console debug support:
 *
 * If enabled, the log reporting is activated as usual. 
 */
#if defined(ENABLE_PY_MYQTT_LOG)
# define py_myqtt_log(l, m, ...)    do{_py_myqtt_log  (__AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
# define py_myqtt_log2(l, m, ...)   do{_py_myqtt_log2  (__AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
#else
# if defined(AXL_OS_WIN32) && !( defined(__GNUC__) || _MSC_VER >= 1400)
/* default case where '...' is not supported but log is still
 * disabled */
#   define py_myqtt_log _py_myqtt_log
#   define py_myqtt_log2 _py_myqtt_log2
# else
#   define py_myqtt_log(l, m, ...) /* nothing */
#   define py_myqtt_log2(l, m, message, ...) /* nothing */
# endif
#endif

void _py_myqtt_log (const char          * file,
		     int                   line,
		     PyMyQttLog           log_level,
		     const char          * message,
		     ...);

void _py_myqtt_log2 (const char          * file,
		      int                   line,
		      PyMyQttLog           log_level,
		      const char          * message,
		      ...);

axl_bool py_myqtt_log_is_enabled       (void);

axl_bool py_myqtt_log2_is_enabled      (void);

axl_bool py_myqtt_color_log_is_enabled (void);

axl_bool py_myqtt_handle_and_clear_exception (PyObject * py_conn);

typedef void (*PyMyQttExceptionHandler) (const char * exception_msg);

void     py_myqtt_set_exception_handler (PyMyQttExceptionHandler handler);

void     py_myqtt_decref                (PyObject * obj);

PyMODINIT_FUNC  initlibpy_myqtt_10 (void);

/* include other modules */
#include <py_myqtt_handle.h>
#include <py_myqtt_ctx.h>
#include <py_myqtt_connection.h>
#include <py_myqtt_channel.h>
#include <py_myqtt_async_queue.h>
#include <py_myqtt_frame.h>
#include <py_myqtt_channel_pool.h>

/** 
 * @brief Macro that records a python object into the first reference,
 * checking if it is null, setting Py_None in the case null reference
 * is found.
 *
 * @param first_ref The reference that will receive the configuration.
 * @param value The value reference to configure.
 */
#define PY_MYQTT_SET_REF(first_ref, value) do{  \
	first_ref = value;                      \
        if (first_ref == NULL) {                \
	       first_ref = Py_None;             \
        }                                       \
        Py_INCREF (first_ref);                  \
}while(0)

/** 
 * @brief Macro that allows to check if the provided handler is
 * callable, producing an exception that notifies it is not, returning
 * the particular error message.
 *
 * @param handler The python object to be check if it is callable.
 * @param message The error message to raise for exception details.
 */
#define PY_MYQTT_IS_CALLABLE(handler, message) do{		     \
	if (handler && ! PyCallable_Check (handler)) {               \
		PyErr_Format (PyExc_ValueError, message);            \
		return NULL;                                         \
	}                                                            \
	}while(0);

#endif
