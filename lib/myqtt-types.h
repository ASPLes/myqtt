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
#ifndef __MYQTT_TYPES_H__
#define __MYQTT_TYPES_H__

/**
 * \defgroup myqtt_types MyQtt Types: Types definitions used across MyQtt Library.
 */
 
/** 
 * \addtogroup myqtt_types
 * @{
 */

/*
 * @brief Debug levels to be used with \ref _myqtt_log, which is used
 * through myqtt_log macro.
 *
 * The set of functions allowing to activate the debug at run time and
 * its variants are:
 * 
 * - \ref myqtt_log_is_enabled
 * - \ref myqtt_log2_is_enabled
 * - \ref myqtt_log_enable
 * - \ref myqtt_log2_enable
 *
 * Activate console color log (using ansi characters):
 * 
 * - \ref myqtt_color_log_is_enabled
 * - \ref myqtt_color_log_enable
 *
 * To lock the log during its emision to avoid several threads doing
 * log at the same time:
 * 
 * - \ref myqtt_log_is_enabled_acquire_mutex
 * - \ref myqtt_log_acquire_mutex
 *
 * Finally, to make the application level to configure a handler:
 * 
 * - \ref myqtt_log_set_handler
 * - \ref myqtt_log_get_handler
 * 
 * @param domain Domain that is registering a log.
 *
 * @param level Log level that is being registered.
 *
 * @param message Message that is being registered.
 */
typedef enum {
	/** 
	 * @internal Log a message as a debug message.
	 */
	MYQTT_LEVEL_DEBUG    = 1 << 0,
	/** 
	 * @internal Log a warning message.
	 */
	MYQTT_LEVEL_WARNING  = 1 << 1,
	/** 
	 * @internal Log a critical message.
	 */
	MYQTT_LEVEL_CRITICAL = 1 << 2,
} MyQttDebugLevel;

/** 
 * @brief Allows to specify which type of operation should be
 * implemented while calling to MyQtt Library internal IO blocking
 * abstraction.
 */
typedef enum {
	/** 
	 * @brief A read watching operation is requested. If this
	 * value is received, the fd set containins a set of socket
	 * descriptors which should be watched for incoming data to be
	 * received.
	 */
	READ_OPERATIONS  = 1 << 0, 
	/** 
	 * @brief A write watching operation is requested. If this
	 * value is received, the fd set contains a set of socket that
	 * is being requested for its availability to perform a write
	 * operation on them.
	 */
	WRITE_OPERATIONS = 1 << 1
} MyQttIoWaitingFor;

/**
 * @brief MyQtt library context. This object allows to store the
 * library status into a single reference, representing an independent
 * execution context.
 *
 * Its normal usage is to create a context variable using \ref
 * myqtt_ctx_new and then start an execution context by calling to
 * \ref myqtt_init_ctx. This makes the library to start its
 * operation.
 *
 * Once finished, a call to stop the context is required: \ref
 * myqtt_exit_ctx followed by a call to \ref myqtt_ctx_free.
 *
 */
typedef struct _MyQttCtx MyQttCtx;

/**
 * @brief A MyQtt Connection object.
 *
 * This object represent a connection inside the MyQtt Library. It
 * can be created using \ref myqtt_connection_new and then checked
 * with \ref myqtt_connection_is_ok.
 *
 * Internal \ref MyQttConn representation is not exposed to
 * user code space to ensure the minimal impact while improving or
 * changing MyQtt Library internals. 
 * 
 * To operate with a \ref MyQttConn object \ref myqtt_connection "check out its API documentation".
 * 
 */
typedef struct _MyQttConn  MyQttConn;

/** 
 * @brief A single MQTT received message.
 */
typedef struct _MyQttMsg  MyQttMsg;

/** 
 * @brief Thread safe hash used by MyQtt
 */
typedef struct _MyQttHash MyQttHash;

/** 
 * @brief Connection options. 
 */
typedef struct _MyQttConnOpts MyQttConnOpts;

/** 
 * @brief MyQtt Operation Status.
 * 
 * This enum is used to represent different MyQtt Library status,
 * especially while operating with \ref MyQttConn
 * references. Values described by this enumeration are returned by
 * \ref myqtt_connection_get_status.
 */
typedef enum {
	/** 
	 * @brief Represents an Error while MyQtt Library was operating.
	 *
	 * The operation asked to be done by MyQtt Library could be
	 * completed.
	 */
	MyQttError                  = 1,
	/** 
	 * @brief Represents the operation have been successfully completed.
	 *
	 * The operation asked to be done by MyQtt Library have been
	 * completed.
	 */
	MyQttOk                     = 2,

	/** 
	 * @brief The operation wasn't completed because an error to
	 * tcp bind call. This usually means the listener can be
	 * started because the port is already in use.
	 */
	MyQttBindError              = 3,

	/** 
	 * @brief The operation can't be completed because a wrong
	 * reference (memory address) was received. This also include
	 * NULL references where this is not expected.
	 */
	MyQttWrongReference         = 4,

	/** 
	 * @brief The operation can't be completed because a failure
	 * resolving a name was found (usually a failure in the
	 * gethostbyname function). 
	 */ 
	MyQttNameResolvFailure      = 5,

	/** 
	 * @brief A failure was found while creating a socket.
	 */
	MyQttSocketCreationError    = 6,

	/** 
	 * @brief Found socket created to be using reserved system
	 * socket descriptors. This will cause problems.
	 */
	MyQttSocketSanityError      = 7,

	/** 
	 * @brief Connection error. Unable to connect to remote
	 * host. Remote hosting is refusing the connection.
	 */
	MyQttConnectionError        = 8,

	/** 
	 * @brief Connection error after timeout. Unable to connect to
	 * remote host after after timeout expired.
	 */
	MyQttConnectionTimeoutError = 9,

	/** 
	 * @brief Greetings error found. Initial MQTT senquence not
	 * completed, failed to connect.
	 */
	MyQttGreetingsFailure       = 10,
	/** 
	 * @brief Failed to complete operation due to an xml
	 * validation error.
	 */
	MyQttXmlValidationError     = 11,
	/** 
	 * @brief Connection is in transit to be closed. This is not
	 * an error just an indication that the connection is being
	 * closed at the time the call to \ref
	 * myqtt_connection_get_status was done.
	 */
	MyQttConnectionCloseCalled  = 12,
	/** 
	 * @brief The connection was terminated due to a call to \ref
	 * myqtt_connection_shutdown or an internal implementation
	 * that closes the connection without taking place the MQTT disconnect.
	 */
	MyQttConnectionForcedClose  = 13,
	/** 
	 * @brief Found a protocol error while operating.
	 */
	MyQttProtocolError          = 14,
	/** 
	 * @brief  The connection was closed or not accepted due to a filter installed. 
	 */
	MyQttConnectionFiltered     = 15,
	/** 
	 * @brief Memory allocation failed.
	 */
	MyQttMemoryFail             = 16,
	/** 
	 * @brief When a connection is closed by the remote side but
	 * without going through the MQTT disconnect.
	 */
	MyQttUnnotifiedConnectionClose = 17
} MyQttStatus;


/** 
 * @brief Provides info about the role of the \ref MyQttConn.
 *
 *
 * You can get current role for a given connection using \ref myqtt_conn_get_role.
 * 
 */
typedef enum {
	/** 
	 * @brief This value is used to represent an unknown role state.
	 */
	MyQttRoleUnknown,
	
	/** 
	 * @brief The connection is acting as an Initiator one.
	 */
	MyQttRoleInitiator,

	/** 
	 * @brief The connection is acting as a Listener one.
	 */
	MyQttRoleListener,
	
	/** 
	 * @brief The connection is acting as a master listener to
	 * accept new incoming connections.
	 *
	 * Connections reference that were received with the following
	 * functions are the only ones that can have this value:
	 *
	 * - \ref myqtt_listener_new
	 * - \ref myqtt_listener_new2
	 * - \ref myqtt_listener_new_full
	 */
	MyQttRoleMasterListener
	
} MyQttPeerRole;




/** 
 * @brief Helper macro which allows to push data into a particular
 * queue, checking some conditions, which are logged at the particular
 * position if they fail.
 *
 * @param queue The queue to be used to push the new data. This reference can't be null.
 *
 * @param data The data to be pushed. This data can't be null.
 */
#define QUEUE_PUSH(queue, data)\
do {\
    if (queue == NULL) { \
       myqtt_log (MYQTT_LEVEL_CRITICAL, "trying to push data in a null reference queue at: %s:%d", __AXL_FILE__, __AXL_LINE__); \
    } else if (data == NULL) {\
       myqtt_log (MYQTT_LEVEL_CRITICAL, "trying to push null data in a queue at: %s:%d", __AXL_FILE__, __AXL_LINE__); \
    } else { \
       myqtt_async_queue_push (queue,data);\
    }\
}while(0)

/** 
 * @brief Helper macro which allows to PRIORITY push data into a
 * particular queue, checking some conditions, which are logged at the
 * particular position if they fail. 
 *
 * @param queue The queue to be used to push the new data. This reference can't be null.
 *
 * @param data The data to be pushed. This data can't be null.
 */
#define QUEUE_PRIORITY_PUSH(queue, data)\
do {\
    if (queue == NULL) { \
       myqtt_log (MYQTT_LEVEL_CRITICAL, "trying to push priority data in a null reference queue at: %s:%d", __AXL_FILE__, __AXL_LINE__); \
    } else if (data == NULL) {\
       myqtt_log (MYQTT_LEVEL_CRITICAL, "trying to push priority null data in a queue at: %s:%d", __AXL_FILE__, __AXL_LINE__); \
    } else { \
       myqtt_async_queue_priority_push (queue,data);\
    }\
}while(0)

/** 
 * @internal Definitions to accomodate the underlaying thread
 * interface to the MyQtt thread API.
 */
#if defined(AXL_OS_WIN32)

#define __OS_THREAD_TYPE__ win32_thread_t
#define __OS_MUTEX_TYPE__  HANDLE
#define __OS_COND_TYPE__   win32_cond_t

typedef struct _win32_thread_t {
	HANDLE    handle;
	void*     data;
	unsigned  id;	
} win32_thread_t;

/** 
 * @internal pthread_cond_t definition, fully based on the work done
 * by Dr. Schmidt. Take a look into his article (it is an excelent article): 
 * 
 *  - http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
 * 
 * Just a wonderful work. 
 *
 * Ok. The following is a custom implementation to solve windows API
 * flaw to support conditional variables for critical sections. The
 * solution provided its known to work under all windows platforms
 * starting from NT 4.0. 
 *
 * In the case you are experimenting problems for your particular
 * windows platform, please contact us through the mailing list.
 */
typedef struct _win32_cond_t {
	/* Number of waiting threads. */
	int waiters_count_;
	
	/* Serialize access to <waiters_count_>. */
	CRITICAL_SECTION waiters_count_lock_;

	/* Semaphore used to queue up threads waiting for the
	 * condition to become signaled. */
	HANDLE sema_;

	/* An auto-reset event used by the broadcast/signal thread to
	 * wait for all the waiting thread(s) to wake up and be
	 * released from the semaphore.  */
	HANDLE waiters_done_;

	/* Keeps track of whether we were broadcasting or signaling.
	 * This allows us to optimize the code if we're just
	 * signaling. */
	size_t was_broadcast_;
	
} win32_cond_t;

#elif defined(AXL_OS_UNIX)

#include <pthread.h>
#define __OS_THREAD_TYPE__ pthread_t
#define __OS_MUTEX_TYPE__  pthread_mutex_t
#define __OS_COND_TYPE__   pthread_cond_t

#endif

/** 
 * @brief Thread definition, which encapsulates the os thread API,
 * allowing to provide a unified type for all threading
 * interface. 
 */
typedef __OS_THREAD_TYPE__ MyQttThread;

/** 
 * @brief Mutex definition that encapsulates the underlaying mutex
 * API.
 */
typedef __OS_MUTEX_TYPE__  MyQttMutex;

/** 
 * @brief Conditional variable mutex, encapsulating the underlaying
 * operating system implementation for conditional variables inside
 * critical sections.
 */
typedef __OS_COND_TYPE__   MyQttCond;

/** 
 * @brief Message queue implementation that allows to communicate
 * several threads in a safe manner. 
 */
typedef struct _MyQttAsyncQueue MyQttAsyncQueue;

/** 
 * @brief Handle definition for the family of function that is able to
 * accept the function \ref myqtt_thread_create.
 *
 * The function receive a user defined pointer passed to the \ref
 * myqtt_thread_create function, and returns an pointer reference
 * that must be used as integer value that could be retrieved if the
 * thread is joined.
 *
 * Keep in mind that there are differences between the windows and the
 * posix thread API, that are supported by this API, about the
 * returning value from the start function. 
 * 
 * While POSIX defines as returning value a pointer (which could be a
 * reference pointing to memory high above 32 bits under 64
 * architectures), the windows API defines an integer value, that
 * could be easily used to return pointers, but only safe on 32bits
 * machines.
 *
 * The moral of the story is that you must use another mechanism to
 * return data from this function to the thread that is expecting data
 * from this function. 
 * 
 * Obviously if you are going to return an status code, there is no
 * problem. This only applies to user defined data that is returned as
 * a reference to allocated data.
 */
typedef axlPointer (* MyQttThreadFunc) (axlPointer user_data);

/** 
 * @brief Thread configuration its to modify default behaviour
 * provided by the thread creation API.
 */
typedef enum  {
	/** 
	 * @brief Marker used to signal \ref myqtt_thread_create that
	 * the configuration list is finished.
	 * 
	 * The following is an example on how to create a new thread
	 * without providing any configuration, using defaults:
	 *
	 * \code
	 * MyQttThread thread;
	 * if (! myqtt_thread_created (&thread, 
	 *                              some_start_function, NULL,
	 *                              MYQTT_THREAD_CONF_END)) {
	 *      // failed to create the thread 
	 * }
	 * // thread created
	 * \endcode
	 */
	MYQTT_THREAD_CONF_END = 0,
	/** 
	 * @brief Allows to configure if the thread create can be
	 * joined and waited by other. 
	 *
	 * Default state for all thread created with \ref
	 * myqtt_thread_create is true, that is, the thread created
	 * is joinable.
	 *
	 * If configured this value, you must provide as the following
	 * value either axl_true or axl_false.
	 *
	 * \code
	 * MyQttThread thread;
	 * if (! myqtt_thread_create (&thread, some_start_function, NULL, 
	 *                             MYQTT_THREAD_CONF_JOINABLE, axl_false,
	 *                             MYQTT_THREAD_CONF_END)) {
	 *    // failed to create the thread
	 * }
	 * 
	 * // Nice! thread created
	 * \endcode
	 */
	MYQTT_THREAD_CONF_JOINABLE  = 1,
	/** 
	 * @brief Allows to configure that the thread is in detached
	 * state, so no thread can join and wait for it for its
	 * termination but it will also provide.
	 */
	MYQTT_THREAD_CONF_DETACHED = 2,
}MyQttThreadConf;

/** 
 * @brief Enumeration type that allows to use the waiting mechanism to
 * be used by the core library to perform wait on changes on sockets
 * handled.
 */

typedef enum {
	/**
	 * @brief Allows to configure the select(2) system call based
	 * mechanism. It is known to be available on any platform,
	 * however it has some limitations while handling big set of
	 * sockets, and it is limited to a maximum number of sockets,
	 * which is configured at the compilation process.
	 *
         * Its main disadvantage it that it can't handle
	 * more connections than the number provided at the
	 * compilation process. See <myqtt.h> file, variable
	 * FD_SETSIZE and MYQTT_FD_SETSIZE.
	 */
	MYQTT_IO_WAIT_SELECT = 1,
	/**
	 * @brief Allows to configure the poll(2) system call based
	 * mechanism. 
	 * 
	 * It is also a widely available mechanism on POSIX
	 * envirionments, but not on Microsoft Windows. It doesn't
	 * have some limitations found on select(2) call, but it is
	 * known to not scale very well handling big socket sets as
	 * happens with select(2) (\ref MYQTT_IO_WAIT_SELECT).
	 *
	 * This mechanism solves the runtime limitation that provides
	 * select(2), making it possible to handle any number of
	 * connections without providing any previous knowledge during
	 * the compilation process. 
	 * 
	 * Several third party tests shows it performs badly while
	 * handling many connections compared to (\ref MYQTT_IO_WAIT_EPOLL) epoll(2).
	 *
	 * However, reports showing that results, handles over 50.000
	 * connections at the same time (up to 400.000!). In many
	 * cases this is not going your production environment.
	 *
	 * At the same time, many reports (and our test results) shows
	 * that select(2), poll(2) and epoll(2) performs the same
	 * while handling up to 10.000 connections at the same time.
	 */
	MYQTT_IO_WAIT_POLL   = 2,
	/**
	 * @brief Allows to configure the epoll(2) system call based
	 * mechanism.
	 * 
	 * It is a mechanism available on GNU/Linux starting from
	 * kernel 2.6. It is supposed to be a better implementation
	 * than poll(2) and select(2) due the way notifications are
	 * done.
	 *
	 * It is currently selected by default if your kernel support
	 * it. It has the advantage that performs very well with
	 * little set of connections (0-10.000) like
	 * (\ref MYQTT_IO_WAIT_POLL) poll(2) and (\ref MYQTT_IO_WAIT_SELECT)
	 * select(2), but scaling much better when going to up heavy
	 * set of connections (50.000-400.000).
	 *
	 * It has also the advantage to not require defining a maximum
	 * socket number to be handled at the compilation process.
	 */
	MYQTT_IO_WAIT_EPOLL  = 3,
} MyQttIoWaitingType;



/** 
 * @brief Definition for supported network transports.
 */ 
typedef enum { 
	/** 
	 * @brief IPv4 transport.
	 */
	MYQTT_IPv4 = 1,
	/** 
	 * @brief IPv6 transport.
	 */
	MYQTT_IPv6 = 2,
} MyQttNetTransport;

/** 
 * @brief Quality of service for message delivery. \ref
 * MYQTT_QOS_AT_MOST_ONCE is the lowest quality of service because it
 * does not track missing messages and replication while \ref
 * MYQTT_QOS_EXACTLY_ONCE_DELIVERY is the highest quality of service
 * possible.
 */
typedef enum {
	/** 
	 * @brief Quality of service denied. API code used to report
	 * to the engine, for example, that a particular subscription
	 * is not accepted (\ref MyQttOnSubscribeHandler).
	 */
	MYQTT_QOS_DENIED = 128,

	/** 
	 * @brief Combinable flag for \ref myqtt_conn_pub to control
	 * storage for publications that may require them by default
	 * (for example \ref MQTT_QOS_1 and \ref MYQTT_QOS_2) but
	 * still requires that quality of service to be present in the
	 * header. Combine them by doing \ref MYQTT_QOS_1 | \ref MYQTT_QOS_SKIP_STORAGE.
	 */
	MYQTT_QOS_SKIP_STORAGE = 127,

	/** 
	 * @brief Combinable flag for myqtt_storage_store_msg_offline
	 * to avoid having that function notifying on_store
	 * handler. This is very rare unless you know what you are
	 * doing. 
	 */
	MYQTT_QOS_SKIP_STOREAGE_NOTIFY = 126,
	

	/** 
	 * @brief This is Qos 0. The message is delivered according to
	 * the capabilities of the underlying network. No response is
	 * sent by the receiver and no retry is performed by the
	 * sender.
	 */
	MYQTT_QOS_AT_MOST_ONCE = 0,
	/** 
	 * @brief Alias to \ref MYQTT_QOS_AT_MOST_ONCE.
	 */
	MYQTT_QOS_0 = 0,
	/** 
	 * @brief This is Qos 1. The receiver of a QoS 1 PUBLISH
	 * Packet acknowledges receipt with a PUBACK Packet.
	 */
	MYQTT_QOS_AT_LEAST_ONCE_DELIVERY = 1,
	/** 
	 * @brief Alias to \ref MYQTT_QOS_AT_LEAST_ONCE_DELIVERY.
	 */
	MYQTT_QOS_1 = 1,
	/** 
	 * @brief This is Qos 2. This is the highest quality of
	 * service, for use when neither loss nor duplication of
	 * messages are acceptable. There is an increased overhead
	 * associated with this quality of service.
	 */
	MYQTT_QOS_EXACTLY_ONCE_DELIVERY = 2,
	/** 
	 * @brief Alias to \ref MYQTT_QOS_EXACTLY_ONCE_DELIVERY.
	 */
	MYQTT_QOS_2 = 2
} MyQttQos;

/** 
 * @brief Message type (Control packet types).
 */
typedef enum {
	/** 
	 * @brief Client request to connect to Server.
	 */
	MYQTT_CONNECT      = 1,
	/** 
	 * @brief Connect acknowledgment.
	 */
	MYQTT_CONNACK      = 2,
	/** 
	 * @brief Publish message.
	 */
	MYQTT_PUBLISH      = 3,
	/** 
	 * @brief Publish acknowledgment.
	 */
	MYQTT_PUBACK       = 4,
	/** 
	 * @brief Publish received (assured delivery part 1).
	 */
	MYQTT_PUBREC       = 5,
	/** 
	 * @brief Publish release (assured delivery part 2).
	 */
	MYQTT_PUBREL       = 6,
	/** 
	 * @brief Publish complete (assured delivery part 3).
	 */
	MYQTT_PUBCOMP      = 7,
	/** 
	 * @brief Client subscribe request.
	 */
	MYQTT_SUBSCRIBE    = 8,
	/** 
	 * @brief Subscribe acknowledgment.
	 */
	MYQTT_SUBACK       = 9,
	/** 
	 * @brief Unsubscribe request.
	 */
	MYQTT_UNSUBSCRIBE  = 10,
	/** 
	 * @brief Unsubscribe acknowledgment.
	 */
	MYQTT_UNSUBACK     = 11,
	/** 
	 * @brief PING request from client.
	 */
	MYQTT_PINGREQ      = 12,
	/** 
	 * @brief PING response from server.
	 */
	MYQTT_PINGRESP     = 13,
	/** 
	 * @brief Client is disconnecting.
	 */
	MYQTT_DISCONNECT   = 14,
	
} MyQttMsgType;

/** 
 * @brief Set of return codes that are used by the library to reply to
 * CONNECT requests. 
 */
typedef enum {
	/** 
	 * @brief Local error code reported when unknown error happened.
	 */
	MYQTT_CONNACK_UNKNOWN_ERR              = -4,
	/** 
	 * @brief Local error code reported when connection fails with a timeout.
	 */
	MYQTT_CONNACK_CONNECT_TIMEOUT          = -3,
	/** 
	 * @brief Local error code reported when connection was not possible (connection refused).
	 */
	MYQTT_CONNACK_UNABLE_TO_CONNECT        = -2,
	/** 
	 * @brief Report code used to indicate the library to skip
	 * replying the particular connect method. Then, the
	 * application library can reply to that connect request later
	 * or maybe close the connection....or introduce certain delay
	 * according to some security scheme. Because the reply is
	 * deferred, the library won't be locked waiting the handler
	 * to reply the CONNACK code.
	 */
	MYQTT_CONNACK_DEFERRED                 = -1,
	/** 
	 * @brief Return code for connection accepted.
	 */
	MYQTT_CONNACK_ACCEPTED                 = 0,
	/** 
	 * @brief Return code for Connection refused, unacceptable
	 * protocol version. This return code will report that the
	 * server does not support the level of the MQTT protocol
	 * requested by the client.
	 */
	MYQTT_CONNACK_REFUSED                  = 1,
	/** 
	 * @brief Return code for connection refused, identifier
	 * rejected.
	 */
	MYQTT_CONNACK_IDENTIFIER_REJECTED      = 2,
	/** 
	 * @brief Return code for connection refused, server
	 * unavailable. This return code will report that the network
	 * connection has been made but the MQTT service is
	 * unavailable.
	 */
	MYQTT_CONNACK_SERVER_UNAVAILABLE       = 3,
	/** 
	 * @brief Return code for connection refused, bad user name or password. 
	 */
	MYQTT_CONNACK_BAD_USERNAME_OR_PASSWORD = 4,
	/** 
	 * @brief Return code for connection refused, not authorized.
	 */
	MYQTT_CONNACK_NOT_AUTHORIZED           = 5
} MyQttConnAckTypes;

typedef enum {
	/** 
	 * @internal Indication for no more parameters 
	 */
	MYQTT_PARAM_END            = 0,
	/** 
	 * @internal Variable header or payload parameter that that
	 * representings an utf-8 string.
	 * 2 bytes + the string as indicated by MQTT standard
	 * MSB+LSB + string.
	 */
	MYQTT_PARAM_UTF8_STRING    = 1,
	/**
	 * @internal Binary payload without checking its content.
	 */
	MYQTT_PARAM_BINARY_PAYLOAD = 2,
	/** 
	 * @internal Integer value codified with 2 bytes. Only allowed
	 * values up to 65535
	 */
	MYQTT_PARAM_16BIT_INT      = 3,
	/** 
	 * @internal Integer value codified with 1 byte. Only allowed
	 * values up to 255.
	 */
	MYQTT_PARAM_8BIT_INT       = 4,
	/** 
	 * @internal Special parameter to instruct myqtt_msg_build to
	 * skip it if received.
	 */
	MYQTT_PARAM_SKIP           = 5
} MyQttParamType;

/** 
 * @brief Return codes used for \ref MyQttOnPublish handler. 
 */
typedef enum {
	/** 
	 * @brief Reports to the engine that the publish operation can continue.
	 */
	MYQTT_PUBLISH_OK = 1,
	/** 
	 * @brief Reports to the engine that the message must be
	 * discarded and to skip any publish/relay. Note MQTT protocol
	 * does not have any support to notify a PUBLISH denial. In
	 * this case, the engine will just unref the message without
	 * taking any further action (including publishing it to
	 * subscribers).
	 */
	MYQTT_PUBLISH_DISCARD = 2,
	/** 
	 * @brief Close publisher's connection.
	 */
	MYQTT_PUBLISH_CONN_CLOSE = 3,
} MyQttPublishCodes;

/** 
 * @internal While using \ref myqtt_storage "MyQttStorage module" it is
 * possible to tell the module to initialize the entire storage or
 * part of it. This is used in conjuntion with \ref myqtt_storage_init.
 */
typedef enum {
	/** 
	 * @internal Allows to only initialize the storage related to
	 * packet ids.
	 */
	MYQTT_STORAGE_PKGIDS = 1 << 0,

	/** 
	 * @internal Allows to only initialize the storage related to
	 * packet ids.
	 */
	MYQTT_STORAGE_MSGS   = 1 << 1,

	/** 
	 * @internal Allows to initialize the entire storage for the provided
	 * connection. This is mostly used by the server. 
	 */
	MYQTT_STORAGE_ALL    = 7,
	
} MyQttStorage;

/***** INTERNAL TYPES: don't use them because they may change at any time without change API notification ****/

/** 
 * @internal
 */
typedef struct _MyQttSequencerData {
	/**
	 * @brief Reference to the connection.
	 */
	MyQttConn          * conn;

	/** 
	 * @brief The content to be sequenced into msgs.
	 */
	unsigned char      * message;

	/** 
	 * @brief The message size content.
	 */
        int                  message_size;

	/** 
	 * @brief Used to allow myqtt sequencer to keep track about
	 * byte stream to be used while sending remaining data.
	 *
	 * In this context the myqtt sequencer queue the message to
	 * be pending and flags on steps how many bytes remains to be
	 * sent for the given message.
	 */
	unsigned int         step;

	/** 
	 * @brief Message type been sent.
	 */
	MyQttMsgType         type;

} MyQttSequencerData;

/**
 * @internal
 */
typedef struct _MyQttWriterData {
	unsigned char   * msg;
	int               size;
} MyQttWriterData;

#endif

/* @} */
