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
#include <myqtt_ctx_private.h>

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

	/**** myqtt_frame_factory.c: init module ****/
	ctx->frame_id = 1;

	/* init mutex for the log */
	myqtt_mutex_create (&ctx->log_mutex);

	/**** myqtt_thread_pool.c: init ****/
	ctx->thread_pool_exclusive = axl_true;

	/* init reference counting */
	myqtt_mutex_create (&ctx->ref_mutex);
	ctx->ref_count = 1;

	/* set default serverName acquire value */
	ctx->serverName_acquire = axl_true;

	/* return context created */
	return ctx;
}

/** 
 * @internal Function used to reinit the MyQttCtx. This function is
 * highly unix dependant.
 */
void      myqtt_ctx_reinit (MyQttCtx * ctx)
{
	myqtt_mutex_create (&ctx->log_mutex);
	myqtt_mutex_create (&ctx->ref_mutex);

	/* the rest of mutexes are initialized by myqtt_init_ctx. */
	ctx->ref_count = 1;

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
 * @brief Allows to configure a global frame received handler where
 * all frames are delivered, overriding first and second level
 * handlers. The frame handler is executed using the thread created
 * for the myqtt reader process, that is, without activing a new
 * thread from the pool. This means that the function must not block
 * the caller because no frame will be received until the handler
 * configured on this function finish.
 *
 * @param ctx The context to configure.
 *
 * @param received The handler to configure.
 *
 * @param received_user_data User defined data to be configured
 * associated to the handler. This data will be provided to the frame
 * received handler each time it is activated.
 */
void      myqtt_ctx_set_frame_received          (MyQttCtx             * ctx,
						  MyQttOnFrameReceived   received,
						  axlPointer              received_user_data)
{
	v_return_if_fail (ctx);
	
	/* configure handler and data even if they are null */
	ctx->global_frame_received      = received;
	ctx->global_frame_received_data = received_user_data;

	return;
}

/** 
 * @brief Allows to configure a global close notify handler on the
 * provided on the provided context. 
 * 
 * See \ref MyQttOnNotifyCloseChannel and \ref
 * myqtt_channel_notify_close to know more about this function. The
 * handler configured on this function will affect to all channel
 * close notifications received on the provided context.
 * 
 * @param ctx The context to configure with a global close channel notify.
 *
 * @param close_notify The close notify handler to execute.
 *
 * @param user_data User defined data to be passed to the close notify
 * handler.
 */
void               myqtt_ctx_set_close_notify_handler     (MyQttCtx                  * ctx,
							    MyQttOnNotifyCloseChannel   close_notify,
							    axlPointer                   user_data)
{
	/* check context received */
	if (ctx == NULL)
		return;

	/* configure handlers */
	ctx->global_notify_close      = close_notify;
	ctx->global_notify_close_data = user_data;

	/* nothing more to do over here */
	return;
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

axlPointer __myqtt_ctx_notify_idle (MyQttConnection * conn)
{
	MyQttCtx          * ctx     = CONN_CTX (conn);
	MyQttIdleHandler    handler = ctx->global_idle_handler;

	/* check null handler */
	if (handler == NULL)
		return NULL;

	/* call to notify idle */
	myqtt_log (MYQTT_LEVEL_DEBUG, "notifying idle connection on id=%d because %ld was exceeded", 
		    myqtt_connection_get_id (conn), ctx->max_idle_period);
	handler (ctx, conn, ctx->global_idle_handler_data, ctx->global_idle_handler_data2);
	
	/* reduce reference acquired */
	myqtt_log (MYQTT_LEVEL_DEBUG, "Calling to reduce reference counting now finished idle handler for id=%d",
		    myqtt_connection_get_id (conn));

	/* reset idle state to current time and notify idle notification finished */
	myqtt_connection_set_receive_stamp (conn, 0, 0);
	myqtt_connection_set_data (conn, "vo:co:idle", NULL);

	myqtt_connection_unref (conn, "notify-idle");

	return NULL;
}

/** 
 * @internal Function used to implement idle notify on a connection.
 */
void        myqtt_ctx_notify_idle               (MyQttCtx                       * ctx,
						  MyQttConnection                * conn)
{
	/* do nothing if handler or context aren't defined */
	if (ctx == NULL || ctx->global_idle_handler == NULL)
		return;

	/* check if the connection was already notified */
	if (PTR_TO_INT (myqtt_connection_get_data (conn, "vo:co:idle")))
		return;
	/* notify idle notification is in progress */
	myqtt_connection_set_data (conn, "vo:co:idle", INT_TO_PTR (axl_true));

	/* check unchecked reference (always acquire reference) */
	myqtt_connection_ref_internal (conn, "notify-idle", axl_false);

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
 */
void        myqtt_ctx_ref                       (MyQttCtx  * ctx)
{
	myqtt_ctx_ref2 (ctx, "begin ref");
	return;
}

/** 
 * @brief Allows to increase reference count to the MyQttCtx
 * instance.
 *
 * @param ctx The reference to update its reference count.
 *
 * @param who An string that identifies this ref. Useful for debuging.
 */
void        myqtt_ctx_ref2                       (MyQttCtx  * ctx, const char * who)
{
	/* do nothing */
	if (ctx == NULL)
		return;

	/* acquire the mutex */
	myqtt_mutex_lock (&ctx->ref_mutex);
	ctx->ref_count++;

	myqtt_log (MYQTT_LEVEL_DEBUG, "%s: increased references to MyQttCtx %p (refs: %d)", who, ctx, ctx->ref_count);

	myqtt_mutex_unlock (&ctx->ref_mutex);

	return;
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

	myqtt_log (MYQTT_LEVEL_DEBUG, "about.to.free MyQttCtx %p", ctx);

	/* free the context */
	axl_free (ctx);
	
	return;
}

/** 
 * @internal Function that allows to configure MyQttClientConnCreated
 * handler.
 */
void        myqtt_ctx_set_client_conn_created (MyQttCtx * ctx, 
						MyQttClientConnCreated conn_created,
						axlPointer              user_data)
{
	if (ctx == NULL)
		return;
	ctx->conn_created = conn_created;
	ctx->conn_created_data = user_data;
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
