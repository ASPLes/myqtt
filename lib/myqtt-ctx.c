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

/* global include */
#include <myqtt.h>

/* private include */
#include <myqtt-ctx-private.h>
#include <myqtt-conn-private.h>

/** 
 * \defgroup myqtt_ctx MyQtt context: functions to manage myqtt context, an object that represent a myqtt library state.
 */

/** 
 * \addtogroup myqtt_ctx
 * @{
 */

/** 
 * @brief Creates a new myqtt execution context. This is mainly used
 * by the main module (called from \ref myqtt_init_ctx) and finished from
 * \ref myqtt_exit_ctx. 
 *
 * A context is required to make myqtt library to work. This object
 * stores a single execution context. Several execution context can be
 * created inside the same process.
 *
 * After calling to this function, a new \ref MyQttCtx is created and
 * all configuration required previous to \ref myqtt_init_ctx can be
 * done. Once prepared, a call to \ref myqtt_init_ctx starts myqtt
 * library.
 *
 * Once you want to stop the library execution you must call to \ref
 * myqtt_exit_ctx.
 *
 * See http://lists.aspl.es/pipermail/myqtt/2008-January/000343.html
 * for more information.
 *
 * @return A newly allocated reference to the \ref MyQttCtx. You must
 * finish it with \ref myqtt_ctx_free. Reference returned must be
 * checked to be not NULL (in which case, memory allocation have
 * failed).
 */
MyQttCtx * myqtt_ctx_new (void)
{
	MyQttCtx * ctx;

	/* create a new context */
	ctx           = axl_new (MyQttCtx, 1);
	MYQTT_CHECK_REF (ctx, NULL);
	myqtt_log (MYQTT_LEVEL_DEBUG, "created MyQttCtx reference %p", ctx);

	/* create the hash to store data */
	ctx->data     = myqtt_hash_new (axl_hash_string, axl_hash_equal_string);
	MYQTT_CHECK_REF2 (ctx->data, NULL, ctx, axl_free);

	/**** myqtt-msg.c: init module ****/
	ctx->msg_id = 1;

	/* init mutex for the log */
	myqtt_mutex_create (&ctx->log_mutex);

	/**** myqtt_thread_pool.c: init ****/
	ctx->thread_pool_exclusive = axl_true;

	/* init reference counting */
	myqtt_mutex_create (&ctx->ref_mutex);
	ctx->ref_count = 1;

	/* init pending messages */
	ctx->pending_messages = axl_list_new (axl_list_always_return_1, NULL);
	myqtt_mutex_create (&ctx->pending_messages_m);
	myqtt_cond_create (&ctx->pending_messages_c);

	/* subscription list */
	myqtt_mutex_create (&ctx->subs_m);
	myqtt_cond_create (&ctx->subs_c);

	/* client ids */
	myqtt_mutex_create (&ctx->client_ids_m);

	/* set default connect timeout */
	ctx->connection_connect_std_timeout = 15;

	/* return context created */
	return ctx;
}


/** 
 * @brief Allows to install a \ref MyQttOnPublish handler on the
 * provided context. This is a server side handler (broker). Clients
 * publishing messages does not get this handler called. 
 *
 * See \ref myqtt_ctx_set_on_msg and \ref myqtt_conn_set_on_msg for
 * more information about getting notifications when acting as a
 * MQTT client.
 *
 * See \ref MyQttOnPublish for more information.
 *
 * You can only configure one handler at time. Calling to configure a
 * handler twice will replace previous one.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param on_publish The handler to be configured.
 *
 * @param user_data User defined pointer to be passed in into the
 * handler every time it gets called.
 * 
 * <b>EXAMPLE 1</b>: Discarding publishing a message to reply to it (request-reply pattern)
 *
 * The following example shows how to use this handler to block
 * received message so the engine will discard it, but, because you
 * are in control of the connection and the message received, you can
 * reply to it:
 *
 * \code
 * // Use some pattern to detect which message to reply; in this case we are matching the topic
 * // In this case, we are using this technique to report current client id if the connecting peer
 * // asks for it by publishing to the topic myqtt/admin/get-client-identifier
 * if (axl_cmp ("myqtt/admin/get-client-identifier", myqtt_msg_get_topic (msg))) {
 *
 *		// found administrative request, report this information and block publication 
 *		client_id = myqtt_conn_get_client_id (conn);
 *		if (! myqtt_conn_pub (conn, "myqtt/admin/get-client-identifier", 
 *                                   (axlPointer) client_id, strlen (client_id), MYQTT_QOS_0, axl_false, 0)) 
 *			printf ("ERROR: failed to publish get-client-identifier..\n");
 *            
 *              // report received PUBLISH should be discarded 
 *		return MYQTT_PUBLISH_DISCARD; 
 *
 * } // end if 
 * \endcode
 */
void        myqtt_ctx_set_on_publish            (MyQttCtx                       * ctx,
						 MyQttOnPublish                   on_publish,
						 axlPointer                       user_data)
{
	if (ctx == NULL)
		return;

	/* configure handlers */
	myqtt_mutex_lock (&ctx->ref_mutex);
	ctx->on_publish_data = user_data;
	ctx->on_publish      = on_publish;
	myqtt_mutex_unlock (&ctx->ref_mutex);

	return;
}

/** 
 * @brief Allows to configure the on message handler at context level.
 *
 * The handler configured here affects to all client connections
 * receiving messages on this context that do not have a particular on
 * message \ref myqtt_conn_set_on_msg when this connection is acting
 * as a client (not a MQTT broker).
 *
 * Note that the handler configured at \ref myqtt_conn_set_on_msg is
 * called first, and if defined, makes the handler configured on this
 * function (\ref myqtt_ctx_set_on_msg) to be not called).
 *
 * If you want to set a handler to get a notification at the server
 * side (acting as a broker) for every message received see \ref myqtt_ctx_set_on_publish
 *
 * You can only configure one handler at time. Calling to configure a
 * handler twice will replace previous one.
 *
 * @param ctx The context where the handler will be configured.
 *
 * @param on_msg The handler to be configured.
 *
 * @param on_msg_data User defined pointer to be passed in into the
 * on_msg when called.
 */
void        myqtt_ctx_set_on_msg               (MyQttCtx             * ctx,
						MyQttOnMsgReceived     on_msg,
						axlPointer             on_msg_data)
{
	if (ctx == NULL)
		return;

	/* configure handlers */
	myqtt_mutex_lock (&ctx->ref_mutex);
	ctx->on_msg_data = on_msg_data;
	ctx->on_msg      = on_msg;
	myqtt_mutex_unlock (&ctx->ref_mutex);

	return;
}

/** 
 * @brief Allows to configure the on header handler at context level.
 *
 * The handler configured here affects to all client connections
 * receiving messages on this context.
 *
 * You can only configure one handler at time. Calling to configure a
 * handler twice will replace previous one.
 *
 * This handler is called when a message basic HEADER is received
 * (qos, type, size, dup and retain indications). See \ref
 * MyQttOnHeaderReceived for more information.
 *
 * @param ctx The context where the handler will be configured.
 *
 * @param on_header The handler to be configured.
 *
 * @param on_header_data User defined pointer to be passed in into the
 * on_msg when called.
 */
void        myqtt_ctx_set_on_header             (MyQttCtx               * ctx,
						 MyQttOnHeaderReceived    on_header,
						 axlPointer               on_header_data)
{
	if (ctx == NULL)
		return;

	/* configure handlers */
	myqtt_mutex_lock (&ctx->ref_mutex);
	ctx->on_header_data = on_header_data;
	ctx->on_header      = on_header;
	myqtt_mutex_unlock (&ctx->ref_mutex);

	return;
}

/** 
 * @brief Allows to configure the on store handler at context level.
 *
 * The handler configured here affects to all client connections
 * storing messages on this context.
 *
 * You can only configure one handler at time. Calling to configure a
 * handler twice will replace previous one.
 *
 * This handler is called when a message is requested to be stored. If
 * the function returns axl_false the message is rejected.
 *
 * @param ctx The context where the handler will be configured.
 *
 * @param on_store The handler to be configured.
 *
 * @param on_store_data User defined pointer to be passed in into the
 * on_msg when called.
 */
void        myqtt_ctx_set_on_store              (MyQttCtx               * ctx,
						 MyQttOnStoreMsg          on_store,
						 axlPointer               on_store_data)
{
	if (ctx == NULL)
		return;

	/* configure handlers */
	myqtt_mutex_lock (&ctx->ref_mutex);
	ctx->on_store_data = on_store_data;
	ctx->on_store      = on_store;
	myqtt_mutex_unlock (&ctx->ref_mutex);

	return;
}

/** 
 * @brief Allows to configure the on release handler at context level.
 *
 * The handler configured here affects to all client connections
 * storing messages on this context.
 *
 * You can only configure one handler at time. Calling to configure a
 * handler twice will replace previous one.
 *
 * This handler is called when a message is released and removed from
 * the storage associated . This is a notification.
 *
 * @param ctx The context where the handler will be configured.
 *
 * @param on_release The handler to be configured.
 *
 * @param on_release_data User defined pointer to be passed in into the
 * on_msg when called.
 */
void        myqtt_ctx_set_on_release            (MyQttCtx               * ctx,
						 MyQttOnReleaseMsg        on_release,
						 axlPointer               on_release_data)
{
	if (ctx == NULL)
		return;

	/* configure handlers */
	myqtt_mutex_lock (&ctx->ref_mutex);
	ctx->on_release_data = on_release_data;
	ctx->on_release      = on_release;
	myqtt_mutex_unlock (&ctx->ref_mutex);

	return;
}

/** 
 * @brief Allows to configure a finish handler which is called once
 * the process (myqtt reader) detects no more pending connections are
 * available.
 * 
 * @param ctx The context where the finish handler will be installed.
 *
 * @param finish_handler Finish handler to be called as described by
 * \ref MyQttOnFinishHandler. If the value passed is NULL, then the
 * handler will be unconfigured. Calling again with a handler will
 * unconfigure previous one and set the value passed.
 *
 * @param user_data User defined data to be passed to the handler.
 */
void        myqtt_ctx_set_on_finish        (MyQttCtx              * ctx,
					    MyQttOnFinishHandler    finish_handler,
					    axlPointer               user_data)
{
	if (ctx == NULL)
		return;

	/* removes previous configured handler */
	if (finish_handler == NULL) {
		ctx->finish_handler = NULL;
		ctx->finish_handler_data = NULL;
		return;
	} /* end if */

	/* set handler */
	ctx->finish_handler      = finish_handler;
	ctx->finish_handler_data = user_data;
	return;
}

/** 
 * @internal Function used by myqtt reader module to check and call
 * the finish handler defined.
 */
void        myqtt_ctx_check_on_finish      (MyQttCtx * ctx)
{
	/* check */
	if (ctx == NULL || ctx->finish_handler == NULL)
		return;
	/* call handler */
	ctx->finish_handler (ctx, ctx->finish_handler_data);
	return;
}


/** 
 * @brief Allows to store arbitrary data associated to the provided
 * context, which can later retrieved using a particular key. 
 * 
 * @param ctx The ctx where the data will be stored.
 *
 * @param key The key to index the value stored. The key must be a
 * string.
 *
 * @param value The value to be stored. 
 */
void        myqtt_ctx_set_data (MyQttCtx       * ctx, 
				const char      * key, 
				axlPointer        value)
{
	v_return_if_fail (ctx && key);

	/* call to configure using full version */
	myqtt_ctx_set_data_full (ctx, key, value, NULL, NULL);
	return;
}


/** 
 * @brief Allows to store arbitrary data associated to the provided
 * context, which can later retrieved using a particular key. It is
 * also possible to configure a destroy handler for the key and the
 * value stored, ensuring the memory used will be deallocated once the
 * context is terminated (\ref myqtt_ctx_free) or the value is
 * replaced by a new one.
 * 
 * @param ctx The ctx where the data will be stored.
 * @param key The key to index the value stored. The key must be a string.
 * @param value The value to be stored. If the value to be stored is NULL, the function calls to remove previous content stored on the same key.
 * @param key_destroy Optional key destroy function (use NULL to set no destroy function).
 * @param value_destroy Optional value destroy function (use NULL to set no destroy function).
 */
void        myqtt_ctx_set_data_full (MyQttCtx       * ctx, 
				     const char      * key, 
				     axlPointer        value,
				     axlDestroyFunc    key_destroy,
				     axlDestroyFunc    value_destroy)
{
	v_return_if_fail (ctx && key);

	/* check if the value is not null. It it is null, remove the
	 * value. */
	if (value == NULL) {
		myqtt_hash_remove (ctx->data, (axlPointer) key);
		return;
	} /* end if */

	/* store the data */
	myqtt_hash_replace_full (ctx->data, 
				 /* key and function */
				 (axlPointer) key, key_destroy,
				 /* value and function */
				 value, value_destroy);
	return;
}


/** 
 * @brief Allows to retreive data stored on the given context (\ref
 * myqtt_ctx_set_data) using the provided index key.
 * 
 * @param ctx The context where to lookup the data.
 * @param key The key to use as index for the lookup.
 * 
 * @return A reference to the pointer stored or NULL if it fails.
 */
axlPointer  myqtt_ctx_get_data (MyQttCtx       * ctx,
				 const char      * key)
{
	v_return_val_if_fail (ctx && key, NULL);

	/* lookup */
	return myqtt_hash_lookup (ctx->data, (axlPointer) key);
}

/** 
 * @brief Allows to configure idle handler at context level. This
 * handler will be called when no traffic was produced/received on a
 * particular connection during the max idle period. This is
 * configured when the handler is passed. 
 *
 * @param ctx The context where the idle period and the handler will be configured.
 *
 * @param idle_handler The handler to be called in case a connection
 * have no activity within max_idle_period. The handler will be called
 * each max_idle_period.
 *
 * @param max_idle_period Amount of seconds to wait until considering
 * a connection was idle because no activity was registered within
 * that period.
 *
 * @param user_data Optional user defined pointer to be passed to the
 * idle_handler when activated.
 *
 * @param user_data2 Second optional user defined pointer to be passed
 * to the idle_handler when activated.
 */
void        myqtt_ctx_set_idle_handler          (MyQttCtx                       * ctx,
						  MyQttIdleHandler                 idle_handler,
						  long                              max_idle_period,
						  axlPointer                        user_data,
						  axlPointer                        user_data2)
{
	v_return_if_fail (ctx);

	/* set handlers */
	ctx->global_idle_handler       = idle_handler;
	ctx->max_idle_period           = max_idle_period;
	ctx->global_idle_handler_data  = user_data;
	ctx->global_idle_handler_data2 = user_data2;

	/* do nothing more for now */
	return;
}

/** 
 * @brief Allows to configure the handler that will be called to
 * delegate decision about CONNECT requests received.
 *
 * @param ctx The context that will be configured.
 *
 * @param on_connect The handler that will be configured.
 *
 * This function differs from \ref myqtt_listener_set_on_connection_accepted in the sense 
 * that the handler configured in this function is called when CONNECT MQTT package has 
 * been received and fully parsed while the handler configured at \ref myqtt_listener_set_on_connection_accepted is 
 * called when the connection has been just received (but no protocol negotiation has taken place).
 *
 * @param user_data User defined pointer that will be passed in into
 * the handler when called.
 */
void                myqtt_ctx_set_on_connect (MyQttCtx               * ctx, 
					      MyQttOnConnectHandler    on_connect, 
					      axlPointer               user_data)
{
	if (ctx == NULL)
		return;

	/* set data in a consistent manner */
	myqtt_mutex_lock (&ctx->ref_mutex);
	ctx->on_connect_data = user_data;
	ctx->on_connect      = on_connect;
	myqtt_mutex_unlock (&ctx->ref_mutex);

	return;
}

axlPointer __myqtt_ctx_notify_idle (MyQttConn * conn)
{
	MyQttCtx          * ctx     = CONN_CTX (conn);
	MyQttIdleHandler    handler = ctx->global_idle_handler;

	/* check null handler */
	if (handler == NULL)
		return NULL;

	/* call to notify idle */
	myqtt_log (MYQTT_LEVEL_DEBUG, "notifying idle connection on id=%d because %ld was exceeded", 
		    myqtt_conn_get_id (conn), ctx->max_idle_period);
	handler (ctx, conn, ctx->global_idle_handler_data, ctx->global_idle_handler_data2);
	
	/* reduce reference acquired */
	myqtt_log (MYQTT_LEVEL_DEBUG, "Calling to reduce reference counting now finished idle handler for id=%d",
		    myqtt_conn_get_id (conn));

	/* reset idle state to current time and notify idle notification finished */
	myqtt_conn_set_receive_stamp (conn, 0, 0);
	myqtt_conn_set_data (conn, "vo:co:idle", NULL);

	myqtt_conn_unref (conn, "notify-idle");

	return NULL;
}

/** 
 * @internal Function used to implement idle notify on a connection.
 */
void        myqtt_ctx_notify_idle               (MyQttCtx              * ctx,
						 MyQttConn             * conn)
{
	/* do nothing if handler or context aren't defined */
	if (ctx == NULL || ctx->global_idle_handler == NULL)
		return;

	/* check if the connection was already notified */
	if (PTR_TO_INT (myqtt_conn_get_data (conn, "vo:co:idle")))
		return;
	/* notify idle notification is in progress */
	myqtt_conn_set_data (conn, "vo:co:idle", INT_TO_PTR (axl_true));

	/* check unchecked reference (always acquire reference) */
	myqtt_conn_ref_internal (conn, "notify-idle", axl_false);

	/* call to notify */
	myqtt_thread_pool_new_task (ctx, (MyQttThreadFunc) __myqtt_ctx_notify_idle, conn);

	return;
}

/** 
 * @brief Allows to install a cleanup function which will be called
 * just before the \ref MyQttCtx is finished (by a call to \ref
 * myqtt_exit_ctx or a manual call to \ref myqtt_ctx_free).
 *
 * This function is provided to allow MyQtt Library extensions to
 * install its module deallocation or termination functions.
 *
 * @param ctx The context to configure with the provided cleanup
 * function.  @param cleanup The cleanup function to configure. This
 * function will receive a reference to the \ref MyQttCtx.
 */
void        myqtt_ctx_install_cleanup (MyQttCtx * ctx,
					axlDestroyFunc cleanup)
{
	v_return_if_fail (ctx);
	v_return_if_fail (cleanup);

	myqtt_mutex_lock (&ctx->ref_mutex);
	
	/* init the list in the case it isn't */
	if (ctx->cleanups == NULL) 
		ctx->cleanups = axl_list_new (axl_list_always_return_1, NULL);
	
	/* add the cleanup function */
	axl_list_append (ctx->cleanups, cleanup);

	myqtt_mutex_unlock (&ctx->ref_mutex);

	return;
}

/** 
 * @brief Allows to remove a cleanup function installed previously
 * with myqtt_ctx_install_cleanup.
 *
 * @param ctx The context where the cleanup function will be
 * uninstalled.
 *
 * @param cleanup The cleanup function to be uninstalled.
 */
void        myqtt_ctx_remove_cleanup            (MyQttCtx * ctx,
						  axlDestroyFunc cleanup)
{
	v_return_if_fail (ctx);
	v_return_if_fail (cleanup);

	if (ctx->cleanups == NULL) 
		return;

	myqtt_mutex_lock (&ctx->ref_mutex);
	
	/* add the cleanup function */
	axl_list_remove_ptr (ctx->cleanups, cleanup);

	myqtt_mutex_unlock (&ctx->ref_mutex);

	return;
}

/** 
 * @brief Allows to increase reference count to the MyQttCtx
 * instance.
 *
 * @param ctx The reference to update its reference count.
 *
 * @return axl_true in the case a reference to the context was
 * acquired, otherwise axl_false is returned.
 */
axl_bool        myqtt_ctx_ref                       (MyQttCtx  * ctx)
{
	return myqtt_ctx_ref2 (ctx, "begin ref");
}

/** 
 * @brief Allows to increase reference count to the MyQttCtx instance.
 *
 * @param ctx The reference to update its reference count.
 *
 * @param who An string that identifies this ref. Useful for debuging.
 *
 * @return axl_true if unref operation finished without errors, otherwise axl_false is returned.
 */
axl_bool        myqtt_ctx_ref2                       (MyQttCtx  * ctx, const char * who)
{
	int refs;

	/* do nothing */
	if (ctx == NULL)
		return axl_false;

	/* acquire the mutex */
	myqtt_mutex_lock (&ctx->ref_mutex);
	if (ctx->ref_count <= 0) {
		/* estrange case */
		myqtt_mutex_unlock (&ctx->ref_mutex);
		return axl_false;
	} /* end if */

	ctx->ref_count++;
	refs = ctx->ref_count;
	myqtt_mutex_unlock (&ctx->ref_mutex);

	myqtt_log (MYQTT_LEVEL_DEBUG, "%s: increased references to MyQttCtx %p (refs: %d)", who, ctx, refs);

	return axl_true;
}

/** 
 * @brief Allows to get current reference counting state from provided
 * myqtt context.
 *
 * @param ctx The myqtt context to get reference counting
 *
 * @return Reference counting or -1 if it fails.
 */
int         myqtt_ctx_ref_count                 (MyQttCtx  * ctx)
{
	int result;
	if (ctx == NULL)
		return -1;
	
	/* acquire the mutex */
	myqtt_mutex_lock (&ctx->ref_mutex); 
	result = ctx->ref_count;
	myqtt_mutex_unlock (&ctx->ref_mutex); 

	return result;
}

/** 
 * @brief Decrease reference count and nullify caller's pointer in the
 * case the count reaches 0.
 *
 * @param ctx The context to decrement reference count. In the case 0
 * is reached the MyQttCtx instance is deallocated and the callers
 * reference is nullified.
 */
void        myqtt_ctx_unref                     (MyQttCtx ** ctx)
{

	myqtt_ctx_unref2 (ctx, "unref");
	return;
}

/** 
 * @brief Decrease reference count and nullify caller's pointer in the
 * case the count reaches 0.
 *
 * @param ctx The context to decrement reference count. In the case 0
 * is reached the MyQttCtx instance is deallocated and the callers
 * reference is nullified.
 *
 * @param who An string that identifies this ref. Useful for debuging.
 */
void        myqtt_ctx_unref2                     (MyQttCtx ** ctx, const char * who)
{
	MyQttCtx * _ctx;
	axl_bool   nullify;

	/* do nothing with a null reference */
	if (ctx == NULL || (*ctx) == NULL)
		return;

	/* get local reference */
	_ctx = (*ctx);

	/* check if we have to nullify after unref */
	myqtt_mutex_lock (&_ctx->ref_mutex);

	/* do sanity check */
	if (_ctx->ref_count <= 0) {
		myqtt_mutex_unlock (&_ctx->ref_mutex);

		_myqtt_log (NULL, __AXL_FILE__, __AXL_LINE__, MYQTT_LEVEL_CRITICAL, "attempting to unref MyQttCtx %p object more times than references supported", _ctx);
		/* nullify */
		(*ctx) = NULL;
		return;
	}

	nullify =  (_ctx->ref_count == 1);
	myqtt_mutex_unlock (&_ctx->ref_mutex);

	/* call to unref */
	myqtt_ctx_free2 (*ctx, who);
	
	/* check to nullify */
	if (nullify)
		(*ctx) = NULL;
	return;
}

/** 
 * @brief Releases the memory allocated by the provided \ref
 * MyQttCtx.
 * 
 * @param ctx A reference to the context to deallocate.
 */
void        myqtt_ctx_free (MyQttCtx * ctx)
{
	myqtt_ctx_free2 (ctx, "end ref");
	return;
}

/** 
 * @brief Releases the memory allocated by the provided \ref
 * MyQttCtx.
 * 
 * @param ctx A reference to the context to deallocate.
 *
 * @param who An string that identifies this ref. Useful for debuging.
 */
void        myqtt_ctx_free2 (MyQttCtx * ctx, const char * who)
{
	/* do nothing */
	if (ctx == NULL)
		return;

	/* acquire the mutex */
	myqtt_mutex_lock (&ctx->ref_mutex);
	ctx->ref_count--;

	if (ctx->ref_count != 0) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "%s: decreased references to MyQttCtx %p (refs: %d)", who, ctx, ctx->ref_count);

		/* release mutex */
		myqtt_mutex_unlock (&ctx->ref_mutex);
		return;
	} /* end if */

	/* clear the hash */
	myqtt_hash_destroy (ctx->data);
	ctx->data = NULL;

	myqtt_log (MYQTT_LEVEL_DEBUG, "finishing MyQttCtx %p", ctx);

	/* release log mutex */
	myqtt_mutex_destroy (&ctx->log_mutex);
	
	/* release and clean mutex */
	myqtt_mutex_unlock (&ctx->ref_mutex);
	myqtt_mutex_destroy (&ctx->ref_mutex);

	/* init pending messages */
	axl_list_free (ctx->pending_messages);
	myqtt_mutex_destroy (&ctx->pending_messages_m);
	myqtt_cond_destroy (&ctx->pending_messages_c);

	/* release connections subscribed */
	axl_hash_free (ctx->subs);
	axl_hash_free (ctx->wild_subs);

	axl_hash_free (ctx->offline_subs);
	axl_hash_free (ctx->offline_wild_subs);

	myqtt_mutex_destroy (&ctx->subs_m);
	myqtt_cond_destroy (&ctx->subs_c);

	/* release client ids hash */
	myqtt_mutex_destroy (&ctx->client_ids_m);
	axl_hash_free (ctx->client_ids);

	/* release path */
	axl_free (ctx->storage_path);

	myqtt_log (MYQTT_LEVEL_DEBUG, "about.to.free MyQttCtx %p", ctx);

	/* free the context */
	axl_free (ctx);
	
	return;
}

void        __myqtt_ctx_set_cleanup (MyQttCtx * ctx)
{
	if (ctx == NULL)
		return;
	ctx->reader_cleanup = axl_true;
	return;
}

/** 
 * @}
 */
