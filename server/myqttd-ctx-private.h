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
	MyqttCtx          * myqtt_ctx;

	/* default signal handlers */
	MyqttdSignalHandler signal_handler;

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
	MyqttAsyncQueue   * wait_queue;

	/* Myqttd current pid (process identifier) */
	int                  pid;
	/* if the following is NULL this is the main process,
	 * otherwise it points to the child process */
	MyqttdChild    * child;
	
	/* some variables used to terminate myqttd. */
	axl_bool             is_exiting;
	MyqttMutex          exit_mutex;
	
	/* Mutex to protect the list of db list opened. */
	MyqttMutex          db_list_mutex;
	
	/* List of already opened db list, used to implement automatic
	 * features such automatic closing on myqttd exit,
	 * automatic reloading.. */
	axlList            * db_list_opened;
	axlDtd             * db_list_dtd;

	/*** myqttd ppath module ***/
	int                  ppath_next_id;
	MyqttdPPath    * paths;
	axl_bool             all_rules_address_based;
	/* profile_attr_alias: this hash allows to establish a set of
	 * alias that is used by profile path module to check for a
	 * particular attribute found on the connection instead of the
	 * particular profile requested. This is useful for TLS case
	 * (and other tuning profiles) that reset the connection,
	 * causing the channel that represents it to disapear (RFC
	 * 3080, page 30, all channels closed) making the profile path
	 * configuration to not detect the channel after tuning have
	 * been completed. */
	axlHash            * profile_attr_alias;

	/*** myqttd log module ***/
	int                  general_log;
	int                  error_log;
	int                  myqtt_log;
	int                  access_log;
	MyqttdLoop     * log_manager;
	axl_bool             use_syslog;

	/*** myqttd config module ***/
	axlDoc             * config;
	char               * config_path;

	/* myqttd loading modules module */
	axlList            * registered_modules;
	MyqttMutex          registered_modules_mutex;

	/* myqttd connection manager module */
	MyqttMutex          conn_mgr_mutex;
	axlHash            * conn_mgr_hash; 

	/* myqttd stored data */
	axlHash            * data;
	MyqttMutex          data_mutex;

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
	MyqttMutex               child_process_mutex;

	/*** myqttd mediator module ***/
	axlHash            * mediator_hash;
	MyqttMutex          mediator_hash_mutex;
	
	/*** support for proxy on parent ***/
	MyqttdLoop     * proxy_loop;
};

/** 
 * @brief Private definition that represents a child process created.
 */
struct _MyqttdChild {
	int                  pid;
	MyQttdCtx      * ctx;
	MyqttdPPathDef * ppath;

	/** 
	 * @brief This is a reference to the serverName configuration
	 * that is found in the profile path that started this child.
	 */
	char               * serverName;

	int                  child_connection;
	MyqttdLoop     * child_conn_loop;
#if defined(AXL_OS_UNIX)
	char               * socket_control_path;
	char              ** init_string_items;
#endif

	/* connection management */
	MyqttConnection   * conn_mgr;

	/* ref counting and mutex */
	int                  ref_count;
	MyqttMutex          mutex;
};

typedef enum {
	PROFILE_ALLOW, 
	PROFILE_IF
} MyqttdPPathItemType;

struct _MyqttdPPathItem {
	/* The type of the profile item path  */
	MyqttdPPathItemType type;
	
	/* support for the profile to be matched by this profile item
	 * path */
	MyqttdExpr * profile;

	/* optional expression to match a mark that must have a
	 * connection holding the profile */
	char * connmark;
	
	/* optional configuration that allows to configure the number
	   number of channels opened with a particular profile */
	int    max_per_con;

	/* optional expression to match a pre-mark that must have the
	 * connection before accepting the profile. */
	char * preconnmark;

	/* Another list for all profile path item found inside this
	 * profile path item. This is only used by PROFILE_IF items */
	MyqttdPPathItem ** ppath_items;
	
};

struct _MyqttdPPathDef {
	int    id;

	/* the name of the profile path group (optional value) */
	char                 * path_name;

	/* the server name pattern to be used to match the profile
	 * path. If myqttd wasn't built with pcre support, it will
	 * compiled as an string. */
	MyqttdExpr       * serverName;

	/* source filter pattern. Again, if the library doesn't
	 * support regular expression, the source is taken as an
	 * string */
	MyqttdExpr       * src;

	/* destination filter pattern. Again, if the library doesn't
	 * support regular expression, the source is taken as an
	 * string */
	MyqttdExpr       * dst;

	/* a reference to the list of profile path supported (first
	 * xml level of <allow> and <if-sucess> nodes). */
	MyqttdPPathItem ** ppath_items;

#if defined(AXL_OS_UNIX)
	/* user id to that must be used to run the process */
	int  user_id;

	/* group id that must be used to run the process */
	int  group_id;
#endif

	/* allows to control if myqttd must run the connection
	 * received in the context of a profile path in a separate
	 * process */
	axl_bool separate;

	/* allows to control if childs created for each profile path
	   are reused. */
	axl_bool reuse;

	/* allows to change working directory to the provided value */
	const char * chroot;	

	/* allows to configure a working directory associated to the
	 * profile path. */
	const char * work_dir;
	
	/** 
	 * allows to control profile path child limit. By default it
	 * is set to -1 which means no limit is set, so global child
	 * limit is applied (</myqttd/global-settings/global-child-limit>)
	 */
	int child_limit;

	/** 
	 * allows to track current number of child processes that are
	 * running with this profile path. On child process it has no
	 * value.
	 */
	int childs_running;

	/** 
	 * reference to the <ppath-def> that where this profile path was loaded.
	 */
	axlNode * node;

	/** 
	 * In the case profile <path-def> has search nodes configured,
	 * this flag signal if they were loaded previously (to avoid
	 * loading them twice).
	 */
	axl_bool  search_nodes_loaded;
};

/** 
 * @internal Connection management done by conn-mgr module.
 */
typedef struct _MyQttdConnMgrState {
	MyqttConnection * conn;
	MyQttdCtx        * ctx;

	/* a hash that contains the set of profiles running on this
	 * connection and how many times */
	axlHash          * profiles_running;

	/* reference to handler ids to be removed */
	axlPointer         added_channel_id;
	axlPointer         removed_channel_id;
} MyQttdConnMgrState;

#endif
