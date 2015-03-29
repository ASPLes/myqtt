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
#ifndef __MYQTT_HANDLERS_H__
#define __MYQTT_HANDLERS_H__

#include <myqtt.h>

/** 
 * \defgroup myqtt_handlers MyQtt Handlers: Handlers used across MyQtt Library for async notifications.
 */

/** 
 * \addtogroup myqtt_handlers
 * @{
 */


/** 
 * @brief Async notification for listener creation.
 *
 * Functions using this handler:
 * - \ref myqtt_listener_new
 *
 * Optional handler defined to report which host and port have
 * actually allocated a listener peer. If host and port is null means
 * listener have failed to run.
 *
 * You should not free any parameter received, myqtt system will do
 * this for you.  If you want to actually keep a copy you should use
 * axl_strdup.
 * 
 * @param host the final host binded
 * @param port the final port binded
 * @param status the listener creation status.
 * @param message the message reporting the listener status creation.
 * @param user_data user data passed in to this async notifier.
 */
typedef void (*MyQttListenerReady)           (const char   * host, 
					      int            port, 
					      MyQttStatus    status, 
					      char         * message, 
					      MyQttConn    * conn,
					      axlPointer     user_data);

/** 
 * @brief Async notification handler called to notify a new message
 * received (\ref MyQttMsg) due to a PUBLISH packet.
 *
 * @param ctx The context where the operation is taking place.
 *
 * @param conn The connection where the application message was received.
 *
 * @param msg The message received. This reference is stable during
 * the handler execution. If you need to have a permanent reference
 * that survives to the handler execution grab a reference to it using
 * \ref myqtt_msg_ref (and of course, release that reference with \ref
 * myqtt_msg_unref when no longer needed anymore).
 *
 * @param user_data User pointer passed in into the function.
 */
typedef void (*MyQttOnMsgReceived) (MyQttCtx  * ctx,
				    MyQttConn * conn,
				    MyQttMsg  * msg, 
				    axlPointer  user_data);

/** 
 * @brief Async notification handler called when the message header is
 * received (and before receiving the rest of the message).
 *
 * @param ctx The context where the operation takes place.
 *
 * @param conn The connection where the operation is taking place.
 *
 * @param msg The message that contains hearder information. Keep in
 * mind that this reference is still not well defined (for example, it
 * doesn't have the entire message yet, but it contains all the header
 * indications like type, qos, dup, retain and size.
 *
 * @param user_data User data pointer defined at the time this handler was configured.
 *
 * @return axl_true to allow accepting the message. Return axl_false
 * to cause the message to be not accepted and to close the connection
 * that sent the message (there is no way to communicate sending party
 * that the message is not going to be accepted).
 */
typedef axl_bool (*MyQttOnHeaderReceived) (MyQttCtx * ctx,
					   MyQttConn * conn,
					   MyQttMsg  * msg, 
					   axlPointer  user_data);

/** 
 * @brief Async notification for connection creation process.
 *
 * Functions using this handler:
 *  - \ref myqtt_conn_new 
 * 
 * Optional handler defined to report the final status for a \ref
 * myqtt_conn_new process. This handler allows to create a new
 * connection to a myqtt server in a non-blocking way.
 * 
 * Once the connection creation process have finished, no matter which
 * is the final result, the handler is invoked.
 *
 * @param connection The connection created (or not, check \ref myqtt_conn_is_ok).
 * @param user_data user defined data passed in to this async notifier.
 */
typedef void (*MyQttConnNew) (MyQttConn * connection, axlPointer user_data);


/** 
 * @brief Defines the writers handlers used to actually send data
 * through the underlaying socket descriptor.
 * 
 * This handler is used by: 
 *  - \ref myqtt_conn_set_send_handler
 * 
 * @param connection MyQtt Connection where the data will be sent.
 * @param buffer     The buffer holding data to be sent
 * @param buffer_len The buffer len.
 * 
 * @return     How many data was actually sent.
 */
typedef int      (*MyQttSend)                (MyQttConn           * connection,
					      const unsigned char * buffer,
					      int                   buffer_len);

/** 
 * @brief Defines the readers handlers used to actually received data
 * from the underlying socket descriptor.
 *  
 * This handler is used by: 
 *  - \ref myqtt_conn_set_receive_handler
 * 
 * @param connection The MyQtt connection where the data will be received.
 * @param buffer     The buffer reference used as container for data received.
 * @param buffer_len The buffer len use to know the limits of the buffer.
 * 
 * @return How many data was actually received.
 */
typedef int      (*MyQttReceive)                (MyQttConn          * connection,
						 unsigned char      * buffer,
						 int                  buffer_len);

/** 
 * @brief Allows to set a handler that will be called when a
 * connection is about being closed.
 * 
 * This method could be used to establish some especial actions to be
 * taken before the connection is completely unrefered. This function
 * could be used to detect connection broken.
 * 
 * This handler is used by:
 *  - \ref myqtt_conn_set_on_close
 * 
 * <i><b>NOTE:</b> You must not free the connection received at the
 * handler. This is actually done by the library. </i>
 * 
 * @param connection The connection that is about to be unrefered.
 *
 * @param data User defined pointer passed to the handler and configured at \ref myqtt_conn_set_on_close
 * 
 */
typedef void     (*MyQttConnOnClose)      (MyQttConn * connection, axlPointer data);

/** 
 * @brief Async handler definition to get a notification for
 * connections created at the server side (peer that is able to accept
 * incoming connections).
 *
 * This handler is executed once the connection is accepted and
 * registered in the myqtt engine. If the function return axl_false, the
 * connection will be dropped, without reporting any error to the
 * remote peer.
 *
 * This function could be used as a initial configuration for every
 * connection. So, if it is only required to make a connection
 * initialization, the handler provided must always return axl_true to
 * avoid dropping the connection.
 *
 * This handler is also used by the TUNNEL profile implementation to
 * notify the application layer if the incoming TUNNEL profile should
 * be accepted. 
 *
 * Note this handler is called twice on TLS activation: one for the
 * first connection and one for the connection creation after TLS
 * activation. This is because both connections are diferent objects
 * with different states. This also allows to setup or run different
 * configurations for non TLS or and for TLS enabled clients.
 *
 * This handler is used by:
 * 
 *  - \ref myqtt_listener_set_on_connection_accepted 
 *
 * @param connection The connection that has been accepted to be
 * managed by the listener.
 *
 * @param data The connection data to be passed in to the handler. 
 * 
 * @return axl_true if the connection is finally accepted. axl_false to drop
 * the connection, rejecting the incoming connection.
 */
typedef axl_bool      (*MyQttOnAcceptedConnection)   (MyQttConn * connection, axlPointer data);

/** 
 * @brief IO handler definition to allow defining the method to be
 * invoked while createing a new fd set.
 *
 * @param ctx The context where the IO set will be created.
 *
 * @param wait_to Allows to configure the file set to be prepared to
 * be used for the set of operations provided. 
 * 
 * @return A newly created fd set pointer, opaque to MyQtt, to a
 * structure representing the fd set, that will be used to perform IO
 * waiting operation at the \ref myqtt_io "MyQtt IO module".
 * 
 */
typedef axlPointer   (* MyQttIoCreateFdGroup)        (MyQttCtx * ctx, MyQttIoWaitingFor wait_to);

/** 
 * @brief IO handler definition to allow defining the method to be
 * invoked while destroying a fd set. 
 *
 * The reference that the handler will receive is the one created by
 * the \ref MyQttIoCreateFdGroup handler.
 * 
 * @param MyQttIoDestroyFdGroup The fd_set, opaque to myqtt, pointer
 * to a structure representing the fd set to be destroy.
 * 
 */
typedef void     (* MyQttIoDestroyFdGroup)        (axlPointer             fd_set);

/** 
 * @brief IO handler definition to allow defining the method to be
 * invoked while clearing a fd set.
 * 
 * @param MyQttIoClearFdGroup The fd_set, opaque to myqtt, pointer
 * to a structure representing the fd set to be clear.
 * 
 */
typedef void     (* MyQttIoClearFdGroup)        (axlPointer             fd_set);



/** 
 * @brief IO handler definition to allow defining the method to be
 * used while performing a IO blocking wait, by default implemented by
 * the IO "select" call.
 *
 * @param MyQttIoWaitOnFdGroup The handler to set.
 *
 * @param The maximum value for the socket descriptor being watched.
 *
 * @param The requested operation to perform.
 * 
 * @return An error code according to the description found on this
 * function: \ref myqtt_io_waiting_set_wait_on_fd_group.
 */
typedef int      (* MyQttIoWaitOnFdGroup)       (axlPointer             fd_group,
						  int                    max_fds,
						  MyQttIoWaitingFor     wait_to);

/** 
 * @brief IO handler definition to perform the "add to" the fd set
 * operation.
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_group The socket descriptor group to be used as
 * destination for the socket.
 * 
 * @return returns axl_true if the socket descriptor was added, otherwise,
 * axl_false is returned.
 */
typedef axl_bool      (* MyQttIoAddToFdGroup)        (int                    fds,
						       MyQttConn     * connection,
						       axlPointer             fd_group);

/** 
 * @brief IO handler definition to perform the "is set" the fd set
 * operation.
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_group The socket descriptor group to be used as
 * destination for the socket.
 *
 * @param user_data User defined pointer provided to the function.
 *
 * @return axl_true if the socket descriptor is active in the given fd
 * group.
 *
 */
typedef axl_bool      (* MyQttIoIsSetFdGroup)        (int                    fds,
						       axlPointer             fd_group,
						       axlPointer             user_data);

/** 
 * @brief Handler definition to allow implementing the have dispatch
 * function at the myqtt io module.
 *
 * An I/O wait implementation must return axl_true to notify myqtt engine
 * it support automatic dispatch (which is a far better mechanism,
 * supporting better large set of descriptors), or axl_false, to notify
 * that the \ref myqtt_io_waiting_set_is_set_fd_group mechanism must
 * be used.
 *
 * In the case the automatic dispatch is implemented, it is also
 * required to implement the \ref MyQttIoDispatch handler.
 * 
 * @param fd_group A reference to the object created by the I/O waiting mechanism.
 * p
 * @return Returns axl_true if the I/O waiting mechanism support automatic
 * dispatch, otherwise axl_false is returned.
 */
typedef axl_bool      (* MyQttIoHaveDispatch)         (axlPointer             fd_group);

/** 
 * @brief User space handler to implement automatic dispatch for I/O
 * waiting mechanism implemented at myqtt io module.
 *
 * This handler definition is used by:
 * - \ref myqtt_io_waiting_invoke_dispatch
 *
 * Do not confuse this handler definition with \ref MyQttIoDispatch,
 * which is the handler definition for the actual implemenation for
 * the I/O mechanism to implement automatic dispatch.
 * 
 * @param fds The socket that is being notified and identified to be dispatched.
 * 
 * @param wait_to The purpose of the created I/O waiting mechanism.
 *
 * @param connection Connection where the dispatch operation takes
 * place.
 * 
 * @param user_data Reference to the user data provided to the dispatch function.
 */
typedef void     (* MyQttIoDispatchFunc)         (int                    fds,
						   MyQttIoWaitingFor     wait_to,
						   MyQttConn     * connection,
						   axlPointer             user_data);

/** 
 * @brief Handler definition for the automatic dispatch implementation
 * for the particular I/O mechanism selected.
 *
 * This handler is used by:
 *  - \ref myqtt_io_waiting_set_dispatch
 *  - \ref myqtt_io_waiting_invoke_dispatch (internally)
 *
 * If this handler is implemented, the \ref MyQttIoHaveDispatch must
 * also be implemented, making it to always return axl_true. If this two
 * handler are implemented, its is not required to implement the "is
 * set?" functionality provided by \ref MyQttIoIsSetFdGroup (\ref
 * myqtt_io_waiting_set_is_set_fd_group).
 * 
 * @param fd_group A reference to the object created by the I/O
 * waiting mechanism.
 * 
 * @param dispatch_func The dispatch user space function to be called.
 *
 * @param changed The number of descriptors that changed, so, once
 * inspected that number, it is not required to continue.
 *
 * @param user_data User defined data provided to the dispatch
 * function once called.
 */
typedef void     (* MyQttIoDispatch)             (axlPointer             fd_group,
						   MyQttIoDispatchFunc   dispatch_func,
						   int                    changed,
						   axlPointer             user_data);


/** 
 * @brief Handler definition that allows a client to print log
 * messages itself.
 *
 * This function is used by: 
 * 
 * - \ref myqtt_log_set_handler
 * - \ref myqtt_log_get_handler
 *
 * @param file The file that produced the log.
 *
 * @param line The line where the log was produced.
 *
 * @param log_level The level of the log
 *
 * @param message The message being reported.
 *
 * @param args Arguments for the message.
 */
typedef void (*MyQttLogHandler) (const char       * file,
				 int                line,
				 MyQttDebugLevel   log_level,
				 const char       * message,
				 va_list            args);

/** 
 * @brief Handler definition used by \ref myqtt_async_queue_foreach
 * to implement a foreach operation over all items inside the provided
 * queue, blocking its access during its process.
 *
 * @param queue The queue that will receive the foreach operation.
 *
 * @param item_stored The item stored on the provided queue.
 *
 * @param position Item position inside the queue. 0 position is the
 * next item to pop.
 *
 * @param user_data User defined optional data provided to the foreach
 * function.
 */
typedef void (*MyQttAsyncQueueForeach) (MyQttAsyncQueue * queue,
					axlPointer         item_stored,
					int                position,
					axlPointer         user_data);

/** 
 * @brief Handler used by MyQtt library to create a new thread. A custom handler
 * can be specified using \ref myqtt_thread_set_create
 *
 * @param thread_def A reference to the thread identifier created by
 * the function. This parameter is not optional.
 *
 * @param func The function to execute.
 *
 * @param user_data User defined data to be passed to the function to
 * be executed by the newly created thread.
 *
 * @return The function returns axl_true if the thread was created
 * properly and the variable thread_def is defined with the particular
 * thread reference created.
 *
 * @see myqtt_thread_create
 */
typedef axl_bool (* MyQttThreadCreateFunc) (MyQttThread      * thread_def,
					    MyQttThreadFunc    func,
					    axlPointer          user_data,
					    va_list             args);

/** 
 * @brief Handler used by MyQtt Library to release a thread's resources.
 * A custom handler can be specified using \ref myqtt_thread_set_destroy
 *
 * @param thread_def A reference to the thread that must be destroyed.
 *
 * @param free_data Boolean that set whether the thread pointer should
 * be released or not.
 *
 * @return axl_true if the destroy operation was ok, otherwise axl_false is
 * returned.
 *
 * @see myqtt_thread_destroy
 */
typedef axl_bool (* MyQttThreadDestroyFunc) (MyQttThread      * thread_def,
					     axl_bool            free_data);

/** 
 * @brief Handler used by \ref myqtt_ctx_set_on_finish which is
 * called when the myqtt reader process detects no more pending
 * connections are available to be watched which is a signal that no
 * more pending work is available. This handler can be used to detect
 * and finish a process when no more work is available. For example,
 * this handler is used by turbulence to finish processes that where
 * created specifically to manage an incoming connection.
 *
 * @param ctx The context where the finish status as described is
 * signaled. The function executes in the context of the myqtt
 * reader.
 *
 * @param user_data User defined pointer configured at \ref myqtt_ctx_set_on_finish
 */
typedef void     (* MyQttOnFinishHandler)   (MyQttCtx * ctx, axlPointer user_data);

/** 
 * @brief Handler used by async event handlers activated via \ref
 * myqtt_thread_pool_new_event, which causes the handler definition
 * to be called at the provided milliseconds period.
 *
 * @param ctx The myqtt context where the async event will be fired.
 * @param user_data User defined pointer that was defined at \ref myqtt_thread_pool_new_event function.
 * @param user_data2 Second User defined pointer that was defined at \ref myqtt_thread_pool_new_event function.
 *
 * @return The function returns axl_true to signal the system to
 * remove the handler. Otherwise, axl_false must be returned to cause
 * the event to be fired again in the future at the provided period.
 */
typedef axl_bool (* MyQttThreadAsyncEvent)        (MyQttCtx  * ctx, 
						   axlPointer   user_data,
						   axlPointer   user_data2);

/** 
 * @brief Handler called to notify idle state reached for a particular
 * connection. The handler is configured a myqtt context level,
 * applying to all connections created under this context. This
 * handler is configured by:
 *
 * - \ref myqtt_ctx_set_idle_handler
 */
typedef void (* MyQttIdleHandler) (MyQttCtx     * ctx, 
				   MyQttConn    * conn,
				   axlPointer     user_data,
				   axlPointer     user_data2);

/** 
 * @brief Port sharing handler definition used by those functions that
 * tries to detect alternative transports that must be activated
 * before continue with normal MQTT course.
 *
 * The handler must return the following codes to report what they
 * found:
 *  - 1 : no error, nothing found for this handler (don't call me again).
 *  - 2 : found transport, stop calling the next.
 *  - (-1) : transport detected but failure found while activating it.
 *
 * @param ctx The myqtt context where the operation takes place.
 *
 * @param listener The listener where this connection was received.
 *
 * @param conn The connection where port sharing is to be
 * detected. The function must use PEEK operation over the socket
 * (_session) to detect transports until the function is sure about
 * what is going to do. If the function do complete reads during the
 * testing part (removing data from the socket queue) the rest of the
 * MQTT session will fail.
 *
 * @param _session The socket associated to the provided \ref MyQttConn.
 *
 * @param bytes Though the function can do any PEEK read required, the
 * engine reads 4 bytes from the socket (or tries) and pass them into
 * this reference. It should be enough for most cases.
 *
 * @param user_data User defined pointer pass in to the handler (as defined by \ref myqtt_listener_set_port_sharing_handling 's last parameter).
 */
typedef int (*MyQttPortShareHandler) (MyQttCtx * ctx, MyQttConn * listener, MyQttConn * conn, MYQTT_SOCKET _session, const char * bytes, axlPointer user_data);


/** 
 * @brief Set of handlers used by the library to check with user level
 * if the provided connection should be accepted. The function must
 * return of the codes available at \ref MyQttConnAckTypes to report
 * to the library what to do with the connection.
 *
 * This handler is used by the following functions:
 *
 * \ref myqtt_ctx_set_connect_handler
 *
 * You can use the following function to get various elements
 * associated to the CONNECT method. 
 *
 * - \ref myqtt_conn_get_username
 * - \ref myqtt_conn_get_password
 * - \ref myqtt_conn_get_client_id 
 *
 * @param ctx The context where the operation takes place.
 *
 * @param conn The connection where the CONNECT packet was received.
 *
 * @param user_data User defined pointer passed in into the handler.
 */
typedef MyQttConnAckTypes (*MyQttOnConnectHandler) (MyQttCtx * ctx, MyQttConn * conn, axlPointer user_data);

/** 
 * @brief Async notification handler that allows application level to
 * control whether a subscription request should be accepted or not.
 *
 * It can also be used as a way to track subscription taking place.
 *
 * @param ctx The context wherethe operation takes place.
 *
 * @param conn The connection where the SUBSCRIBE request was received.
 *
 * @param topic_filter The topic filter requested on the SUBSCRIBE packet.
 *
 * @param qos The QOS value requested for this subscription.
 *
 * @param user_data User defined pointer received on the function and
 * provided at the configuration handler.
 * 
 */
typedef MyQttQos          (*MyQttOnSubscribeHandler) (MyQttCtx * ctx, MyQttConn * conn, const char * topic_filter, MyQttQos qos, axlPointer user_data);

/** 
 * @brief Async notification handler that gets called once a PUBLISH
 * message is received on a listener connection. This handler allows
 * to control message publication to currently registered connection
 * with general options and/or to implement especific actions upon
 * message reception.
 *
 * It's important to note that this handler is called before any
 * publish/relay operation takes place. 
 *
 * This handler is used by:
 *
 * - \ref myqtt_ctx_set_on_publish
 *
 * @param ctx The context where the operation takes place.
 *
 * @param conn The connection where the PUBLISH message was received.
 *
 * @param msg The message received. The handler must not release message received. It can acquire references to it. 
 *
 * @param user_data User defined pointer passed in into the function once it gets called.
 * 
 * @return The function must report the publish code to the engine so it can act upon it.
 *
 */
typedef MyQttPublishCodes (*MyQttOnPublish) (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data);

/** 
 * @brief Optional Pre-read handler definition that represents the set
 * of functions that are called before any read operation happens on
 * the provided connection.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param listerner The listener that accepted the provided connection.
 *
 * @param conn The connection where the operation takes place.
 *
 * @param opts The connection options reference used for this
 * connection.
 *
 * @param user_data User defined pointer received on the handler and
 * defined at the time the pre-read handler was defined.
 */
typedef void (*MyQttPreRead) (MyQttCtx * ctx, MyQttConn * listener, MyQttConn * conn, MyQttConnOpts * opts, axlPointer user_data);

/** 
 * @internal A handler that is called to establish the session that is
 * going to be used by the provided connetion.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param conn The connection for which it is being requested a session.
 *
 * @param opts Optional connection options.
 *
 * @param user_data Optionl user pointer defined by the user when
 * configured this handler.
 */
typedef axl_bool (*MyQttSessionSetup) (MyQttCtx * ctx, MyQttConn * conn, MyQttConnOpts * opts, axlPointer user_data);

/** 
 * @internal Optional function that can be called when unwatching a
 * connection.
 *
 * @param ctx The context where the operation is taking place.
 *
 * @param conn The connection that is being unwatched. This reference
 * is valid during the handler execution. If you need a permanent
 * reference, grab one reference by using \ref myqtt_conn_ref
 *
 * @param user_data The user defined pointer configured at the unwatch
 * operation time.
 */
typedef void (*MyQttConnUnwatch) (MyQttCtx * ctx, MyQttConn * conn, axlPointer user_data);
				      
#endif

/* @} */
