/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation version 2.0 of the
 *  License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is GPL software 
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
	MyQttAsyncQueue    * wait_queue;

	/* MyQttd current pid (process identifier) */
	int                  pid;
	/* if the following is NULL this is the main process,
	 * otherwise it points to the child process */
	MyQttdChild        * child;
	
	/* some variables used to terminate myqttd. */
	axl_bool             is_exiting;
	MyQttMutex           exit_mutex;
	
	/*** myqttd log module ***/
	int                  general_log;
	int                  error_log;
	int                  myqtt_log;
	int                  access_log;
	MyQttdLoop         * log_manager;
	axl_bool             use_syslog;

	/*** myqttd config module ***/
	axlDoc             * config;
	char               * config_path;

	/* myqttd loading modules module */
	axlList            * registered_modules;
	MyQttMutex           registered_modules_mutex;

	/* myqttd connection manager module */
	MyQttMutex           conn_mgr_mutex;
	axlHash            * conn_mgr_hash; 

	/* myqttd stored data */
	axlHash            * data;
	MyQttMutex           data_mutex;

	/* used to signal if the server was started */
	axl_bool             started;

	/* several limits */
	int                  global_child_limit;
	int                  max_complete_flag_limit;

	/*** myqttd process module ***/
	axlHash            * child_process;
	MyQttMutex           child_process_mutex;

	/*** support for proxy on parent ***/
	MyQttdLoop         * proxy_loop;

	/* reference to all domains currently supported */
	MyQttHash          * domains;

	/* reference to authentication backends registered */
	MyQttHash          * auth_backends;

	/*** on publish handlers ***/
	/* protected by data_mutex */
	axlList            * on_publish_handlers;

	/*** on listener activators ***/
	MyQttHash          * listener_activators;

	/* store for domain settings */
	MyQttHash            * domain_settings;
	MyQttdDomainSetting  * default_setting;

	/** 
	 * @brief List of on day change registered handlers 
	 */
	axlList             * on_day_change_handlers;

	/** 
	 * @brief List of on day change registered handlers 
	 */
	axlList             * on_month_change_handlers;
	
	/** 
	 * @brief Event id reference to stop it once started.
	 */
	long                  time_tracking_event_id;                   
};

/** 
 * @brief Private definition that represents a child process created.
 */
struct _MyQttdChild {
	int                  pid;
	MyQttdCtx          * ctx;

	/** 
	 * @brief This is a reference to the serverName configuration
	 * that is found in the domain that started this child.
	 */
	char               * serverName;

	int                  child_connection;
	MyQttdLoop         * child_conn_loop;

#if defined(AXL_OS_UNIX)
	char               * socket_control_path;
	char              ** init_string_items;
#endif

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
} MyQttdConnMgrState;

/** 
 * @internal Domain definition.
 */
struct _MyQttdDomain {
	MyQttdCtx    * ctx;
	char         * name;
	char         * storage_path;
	char         * users_db;
	MyQttMutex     mutex;
	axl_bool       is_active;

	/* reference to the myqtt context for this domain */
	axl_bool       initialized;
	MyQttCtx     * myqtt_ctx;

	/* reference to a users database used by this domain */
	MyQttdUsers  * users;

	/* reference to settings */
	char                * use_settings;
	MyQttdDomainSetting * settings;
};

typedef struct _MyQttdUsersBackend MyQttdUsersBackend;
struct _MyQttdUsersBackend {
	char                 * backend_type;
	MyQttdUsersLoadDb      load;
	MyQttdUsersUnloadDb    unload;
	MyQttdUsersExists      exists;
	MyQttdUsersAuthUser    auth;
};

struct _MyQttdUsers {
	/* reference to the context used */
	MyQttdCtx  * ctx;

	/* label to identify backend engine */
	const char * backend_type;

	/* pointer to the backend reference used */
	axlPointer   backend_reference;

	/* pointer to the backend configuration and handlers */
	MyQttdUsersBackend * backend;

	/* reference to the unload backend */
	MyQttdUsersUnloadDb    unload;
};

/** 
 * @internal Type that represents an on publish operation.
 */
typedef struct _MyQttdOnPublishData MyQttdOnPublishData;
struct _MyQttdOnPublishData {
	MyQttdOnPublish  on_publish;
	axlPointer       user_data;
};

/** 
 * @internal Type structure.
 */       
typedef struct _MyQttdListenerActivatorData MyQttdListenerActivatorData;
struct _MyQttdListenerActivatorData {
	MyQttdListenerActivator  listener_activator;
	axlPointer               user_data;
};


struct _MyQttdDomainSetting {

	/* operational limits or configurations */
	axl_bool    require_auth;
	axl_bool    restrict_ids;
	axl_bool    drop_conn_same_client_id;

	/* connections and messages */
	int         conn_limit;
	int         message_size_limit;

	/* storage ilmits */
	int         storage_messages_limit;

	/* storage quota per user */
	int         storage_quota_limit;

	/* reference to disable_wild_card_support */
	axl_bool    disable_wildcard_support;

	/* quota for number of messages per day and montly */
	int         month_message_quota;
	int         day_message_quota;
	
};

typedef struct _MyQttdHandlePtr {
	axlPointer handler;
	axlPointer ptr;
} MyQttdHandlerPtr;

#endif
