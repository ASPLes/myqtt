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
#ifndef __MYQTT_CTX_PRIVATE_H__
#define __MYQTT_CTX_PRIVATE_H__

/* global included */
#include <axl.h>
#include <myqtt.h>

struct _MyQttCtx {

	MyQttMutex           ref_mutex;
	int                  ref_count;

	/* global hash to store arbitrary data */
	MyQttHash         * data;

	/* @internal Allows to check if the myqtt library is in exit
	 * transit.
	 */
	axl_bool             myqtt_exit;
	/* @internal Allows to check if the provided myqtt context is initialized
	 */
	axl_bool             myqtt_initialized;

	/* global mutex */
	MyQttMutex           msg_id_mutex;
	MyQttMutex           connection_id_mutex;
	MyQttMutex           search_path_mutex;
	MyQttMutex           exit_mutex;
	MyQttMutex           listener_mutex;
	MyQttMutex           listener_unlock;
	MyQttMutex           inet_ntoa_mutex;
	MyQttMutex           log_mutex;
	axl_bool             use_log_mutex;
	axl_bool             prepare_log_string;

	/* external cleanup functions */
	axlList            * cleanups;

	/* default configurations */
	int                  backlog;
	int                  automatic_mime;

	/* allows to control if we should wait to finish threads
	 * inside the pool */
	axl_bool             skip_thread_pool_wait;
	
	/* local log variables */
	axl_bool             debug_checked;
	axl_bool             debug;
	
	axl_bool             debug2_checked;
	axl_bool             debug2;
	
	axl_bool             debug_color_checked;
	axl_bool             debug_color;

	MyQttLogHandler      debug_handler;

	int                  debug_filter;
	axl_bool             debug_filter_checked;
	axl_bool             debug_filter_is_enabled;

	/*** global handlers */
	/* @internal Finish handler */
	MyQttOnFinishHandler             finish_handler;
	axlPointer                       finish_handler_data;

	/* @internal Handler used to implement global idle
	   notification */
	MyQttIdleHandler                 global_idle_handler;
	long                             max_idle_period;
	axlPointer                       global_idle_handler_data;
	axlPointer                       global_idle_handler_data2;

#if defined(AXL_OS_WIN32)
	/**
	 * Temporal hack to support sock limits under windows until we
	 * find a viable alternative to getrlimit and setrlimit.
	 */
	int                  __myqtt_conf_hard_sock_limit;
	int                  __myqtt_conf_soft_sock_limit;
#endif

	/**** myqtt connection module state *****/
	/** 
	 * @internal
	 * @brief A connection identifier (for internal myqtt use).
	 */
	long                 connection_id;
	axl_bool             connection_enable_sanity_check;

	/**
	 * @internal xml caching functions:
	 */ 
	axlHash            * connection_hostname;
	MyQttMutex           connection_hostname_mutex;
	
	/** 
	 * @internal Default timeout used by myqtt connection operations.
	 */
	long                 connection_std_timeout;
	axl_bool             connection_timeout_checked;
	char              *  connection_timeout_str;
	long                 connection_connect_std_timeout;
	axl_bool             connection_connect_timeout_checked;
	char              *  connection_connect_timeout_str;

	/**** myqtt msg module state ****/
	/** 
	 * @internal
	 *
	 * Internal variables (msg_id and msg_id_mutex) to make msg
	 * identification for the on going process. This allows to check if
	 * two msgs are equal or to track which msgs are not properly
	 * released by the MyQtt Library.
	 *
	 * Msgs are generated calling to __myqtt_msg_get_next_id. Thus,
	 * every msg created while the running process is alive have a
	 * different msg unique identifier. 
	 */
	long                msg_id;

	/**** myqtt io waiting module state ****/
	MyQttIoCreateFdGroup  waiting_create;
	MyQttIoDestroyFdGroup waiting_destroy;
	MyQttIoClearFdGroup   waiting_clear;
	MyQttIoWaitOnFdGroup  waiting_wait_on;
	MyQttIoAddToFdGroup   waiting_add_to;
	MyQttIoIsSetFdGroup   waiting_is_set;
	MyQttIoHaveDispatch   waiting_have_dispatch;
	MyQttIoDispatch       waiting_dispatch;
	MyQttIoWaitingType    waiting_type;

	/**** myqtt reader module state ****/
	MyQttAsyncQueue        * reader_queue;
	MyQttAsyncQueue        * reader_stopped;
	axlPointer                on_reading;
	axlList                 * conn_list;
	axlList                 * srv_list;
	axlListCursor           * conn_cursor;
	axlListCursor           * srv_cursor;
	/* the following flag is used to detecte myqtt
	   reinitialization escenarios where it is required to release
	   memory but without perform all release operatios like mutex
	   locks */
	axl_bool                  reader_cleanup;
	
	/** 
	 * @internal Reference to the thread created for the reader loop.
	 */
	MyQttThread              reader_thread;

	/**** myqtt support module state ****/
	axlList                 * support_search_path;

	/**** myqtt thread pool module state ****/
	/** 
	 * @internal Reference to the thread pool.
	 */
	axl_bool                     thread_pool_exclusive;
	MyQttThreadPool *            thread_pool;
	axl_bool                     thread_pool_being_stopped;

	/**** myqtt listener module state ****/
	MyQttAsyncQueue            * listener_wait_lock;

	/* configuration handler for OnAccepted signal */
	axlList                    * listener_on_accept_handlers;

	/* default realm configuration for all connections at the
	 * server side */
	char                       * listener_default_realm;

	axlList                    * port_share_handlers;
	MyQttMutex                   port_share_mutex;

	/** references to on connect handlers **/
	MyQttOnConnectHandler        on_connect;
	axlPointer                   on_connect_data;

	/** 
	 * @internal References to pending messages to be sent by
	 * sequencer.
	 */
	MyQttMutex                  pending_messages_m;
	MyQttCond                   pending_messages_c;
	axlList                   * pending_messages;
	MyQttThread                 sequencer_thread;

	/** references to the on subscribe handler */
	MyQttOnSubscribeHandler     on_subscribe;
	axlPointer                  on_subscribe_data;

	/** on publish message */
	MyQttOnPublish              on_publish;
	axlPointer                  on_publish_data;

	/** on msg */
	MyQttOnMsgReceived          on_msg;
	axlPointer                  on_msg_data;

	/*** hash of subscriptions ***/
	/* subs is a hash where key is the topic filter and it points
	 * to a hash that contains the connection = > qos:
	 * a/b => { conn1 => 1, conn2 => 0, conn3 => 0 }
	 */
	axlHash                   * subs;

	/* wild_subs works the same as subs but with wildcard topic
	 * filters.
	 */
	axlHash                   * wild_subs;

	axlHash                   * offline_subs;
	axlHash                   * offline_wild_subs;

	MyQttMutex                  subs_m;
	MyQttCond                   subs_c;
	int                         publish_ops;

	axlHash                   * client_ids;
	MyQttMutex                  client_ids_m;

	/** 
	 * @internal Storage path as defined by the user.
	 */
	char                      * storage_path;
	int                         storage_path_hash_size;
	axl_bool                    local_storage;

	/* ssl/tls support */
	axlPointer              context_creator;
	axlPointer              context_creator_data;

	axlPointer              post_ssl_check;
	axlPointer              post_ssl_check_data;
};

#endif /* __MYQTT_CTX_PRIVATE_H__ */

