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
#ifndef __MYQTT_THREAD_POOL_H__
#define __MYQTT_THREAD_POOL_H__

#include <myqtt.h>

BEGIN_C_DECLS

typedef struct _MyQttThreadPool MyQttThreadPool;

void myqtt_thread_pool_init                (MyQttCtx * ctx, int  max_threads);

void myqtt_thread_pool_add                 (MyQttCtx * ctx, int threads);

void myqtt_thread_pool_setup               (MyQttCtx * ctx, 
					     int         thread_max_limit, 
					     int         thread_add_step,
					     int         thread_add_period, 
					     axl_bool    auto_remove);

void myqtt_thread_pool_setup2              (MyQttCtx * ctx, 
					     int         thread_max_limit, 
					     int         thread_add_step,
					     int         thread_add_period, 
					     int         thread_remove_step,
					     int         thread_remove_period, 
					     axl_bool    auto_remove,
					     axl_bool    preemtive); 

void myqtt_thread_pool_remove              (MyQttCtx * ctx, int threads);

void myqtt_thread_pool_exit                (MyQttCtx * ctx);

void myqtt_thread_pool_being_closed        (MyQttCtx * ctx);

axl_bool myqtt_thread_pool_new_task            (MyQttCtx        * ctx,
						MyQttThreadFunc   func, 
						axlPointer         data);

int  myqtt_thread_pool_new_event           (MyQttCtx              * ctx,
					    long                     microseconds,
					    MyQttThreadAsyncEvent   event_handler,
					    axlPointer               user_data,
					    axlPointer               user_data2);

axl_bool myqtt_thread_pool_remove_event        (MyQttCtx              * ctx,
						 int                      event_id);

void myqtt_thread_pool_stats               (MyQttCtx        * ctx,
					     int              * running_threads,
					     int              * waiting_threads,
					     int              * pending_tasks);

void myqtt_thread_pool_event_stats         (MyQttCtx        * ctx,
					     int              * events_installed);

int  myqtt_thread_pool_get_running_threads (MyQttCtx        * ctx);

void myqtt_thread_pool_set_num             (int  number);

int  myqtt_thread_pool_get_num             (void);

void myqtt_thread_pool_set_exclusive_pool  (MyQttCtx        * ctx,
					     axl_bool           value);

/* internal API */
void myqtt_thread_pool_add_internal        (MyQttCtx        * ctx, 
					     int                threads);
void myqtt_thread_pool_remove_internal     (MyQttCtx        * ctx, 
					     int                threads);

void __myqtt_thread_pool_automatic_resize  (MyQttCtx * ctx);

END_C_DECLS

#endif
