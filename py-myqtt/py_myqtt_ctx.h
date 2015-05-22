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

#ifndef __PY_MYQTT_CTX_H__
#define __PY_MYQTT_CTX_H__

/** 
 * @brief Object definition that represents the Python encapsulation
 * for MyQttCtx API type.
 */
typedef struct _PyMyQttCtx PyMyQttCtx;

/* include base header */
#include <py_myqtt.h>

MyQttCtx * py_myqtt_ctx_get    (PyObject * py_myqtt_ctx);

#define START_HANDLER(handler) py_myqtt_ctx_record_start_handler (ctx, handler)

#define CLOSE_HANDLER(handler) py_myqtt_ctx_record_close_handler (ctx, handler)

typedef void (*PyMyQttTooLongNotifier) (const char * msg, axlPointer user_data);

void        py_myqtt_ctx_record_start_handler (MyQttCtx * ctx, PyObject * handler);

void        py_myqtt_ctx_record_close_handler (MyQttCtx * ctx, PyObject * handler);

void        py_myqtt_ctx_start_handler_watcher (MyQttCtx * ctx, int watching_period,
						 PyMyQttTooLongNotifier notifier, axlPointer notifier_data);

axl_bool    py_myqtt_ctx_log_too_long_notifications (MyQttCtx * ctx, int watching_period,
						      const char * file);

PyObject  * py_myqtt_ctx_create (MyQttCtx * ctx);

void        py_myqtt_ctx_register (MyQttCtx  * ctx,
				    PyObject   * data,
				    const char * key,
				    ...);

PyObject  * py_myqtt_ctx_register_get (MyQttCtx  * ctx,
					const char * key,
					...);

axl_bool    py_myqtt_ctx_check  (PyObject  * obj);

void        init_myqtt_ctx      (PyObject * module);

/** internal declaration **/
PyObject  * py_myqtt_ctx_exit   (PyMyQttCtx* self);

#endif
