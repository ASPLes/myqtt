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
#ifndef __MYQTTD_CTX_PRIVATE_H__
#define __MYQTTD_CTX_PRIVATE_H__


struct _MyQttdCtx {
	/* Reference to the myqttd myqtt context associated.
	 */
	MyQttCtx          * myqtt_ctx;

	/* default signal handlers */
	MyQttdSignalHandler signal_handler;

	/* track when myqttd was started (at least this context) */
	long                    running_stamp;

	/* Controls if messages must be send to the console log.
	 */
	int                  console_enabled;
	int                  console_debug;
	int                  console_debug2;
	int                  console_debug3;
	int                  console_color_debug;

	/* wait queue: this queue is used by myqttd_ctx_wait to
	 * implement wait blocking wait without allocatting every time
	 * a custom wait is required */
	MyQttAsyncQueue   * wait_queue;

	/* MyQttd current pid (process identifier) */
	int                  pid;
	/* if the following is NULL this is the main process,
	 * otherwise it points to the child process */
	MyQttdChild    * child;
	
	/* some variables used to terminate myqttd. */
	axl_bool             is_exiting;
	MyQttMutex          exit_mutex;
	
	/* Mutex to protect the list of db list opened. */
	MyQttMutex          db_list_mutex;
	
	/* List of already opened db list, used to implement automatic
	 * features such automatic closing on myqttd exit,
	 * automatic reloading.. */
	axlList            * db_list_opened;
	axlDtd             * db_list_dtd;

	/*** myqttd log module ***/
	int                  general_log;
	int                  error_log;
	int                  myqtt_log;
	int                  access_log;
	MyQttdLoop     * log_manager;
	axl_bool             use_syslog;

	/*** myqttd config module ***/
	axlDoc             * config;
	char               * config_path;

	/* myqttd loading modules module */
	axlList            * registered_modules;
	MyQttMutex          registered_modules_mutex;

	/* myqttd connection manager module */
	MyQttMutex          conn_mgr_mutex;
	axlHash            * conn_mgr_hash; 

	/* myqttd stored data */
	axlHash            * data;
	MyQttMutex          data_mutex;

	/* used to signal if the server was started */
	axl_bool             started;

	/* DTd used by the myqttd-run module to validate module
	 * pointers */
	axlDtd             * module_dtd;

	/* several limits */
	int                  global_child_limit;
	int                  max_complete_flag_limit;

	/*** myqttd process module ***/
	axlHash                 * child_process;
	MyQttMutex               child_process_mutex;

	/*** myqttd mediator module ***/
	axlHash            * mediator_hash;
	MyQttMutex          mediator_hash_mutex;
	
	/*** support for proxy on parent ***/
	MyQttdLoop     * proxy_loop;
};

/** 
 * @brief Private definition that represents a child process created.
 */
struct _MyQttdChild {
	int                  pid;
	MyQttdCtx          * ctx;

	/** 
	 * @brief This is a reference to the serverName configuration
	 * that is found in the profile path that started this child.
	 */
	char               * serverName;

	int                  child_connection;
	MyQttdLoop         * child_conn_loop;
#if defined(AXL_OS_UNIX)
	char               * socket_control_path;
	char              ** init_string_items;
#endif

	/* connection management */
	MyQttConn          * conn_mgr;

	/* ref counting and mutex */
	int                  ref_count;
	MyQttMutex           mutex;
};

/** 
 * @internal Connection management done by conn-mgr module.
 */
typedef struct _MyQttdConnMgrState {
	MyQttConn    * conn;
	MyQttdCtx    * ctx;

	/* a hash that contains the set of profiles running on this
	 * connection and how many times */
	axlHash          * profiles_running;

	/* reference to handler ids to be removed */
	axlPointer         added_channel_id;
	axlPointer         removed_channel_id;
} MyQttdConnMgrState;

#endif
