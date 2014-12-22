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
#include <myqtt.h>

/* local/private includes */
#include <myqtt-ctx-private.h>
#include <myqtt-conn-private.h>
#include <myqtt-msg-private.h>

#define LOG_DOMAIN "myqtt-reader"

/**
 * \defgroup myqtt_reader MyQtt Reader: The module that reads your msgs. 
 */

/**
 * \addtogroup myqtt_reader
 * @{
 */

typedef enum {CONNECTION, 
	      LISTENER, 
	      TERMINATE, 
	      IO_WAIT_CHANGED,
	      IO_WAIT_READY,
	      FOREACH
} WatchType;

typedef struct _MyQttReaderData {
	WatchType            type;
	MyQttConn          * connection;
	MyQttForeachFunc     func;
	axlPointer           user_data;
	/* queue used to notify that the foreach operation was
	 * finished: currently only used for type == FOREACH */
	MyQttAsyncQueue    * notify;
} MyQttReaderData;

/**  
 * @internal handler definition for all myqtt reader handlers that
 * manages incoming MQTT packets.
 */
typedef void (*MyQttReaderHandler) (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data);

/** 
 * @internal Internal function that allows to get the utf-8 string
 * found at the provided payload.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param payload The payload where it is expected to find a MQTT
 * utf-8 encoded string (2 bytes indicating the length, followed by
 * the string itself).
 */
char * __myqtt_reader_get_utf8_string (MyQttCtx * ctx, const unsigned char * payload, int limit)
{
	int    length;
	char * result;
	
	if (payload == NULL)
		return NULL;

	/* get the length to read */
	length = myqtt_get_16bit (payload);

	if (length > limit) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Requested to read an utf-8 string of length %d but limit is %d", length, limit);
		return NULL;
	} /* end if */

	if (length > 65535) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Requested to read an utf-8 string of length %d but that is not accepted", length);
		return NULL;
	} /* end if */
	
	/* allocate memory to hold the string */
	result = axl_new (char, length + 1);
	if (result == NULL)
		return NULL;

	/* copy content into result */
	memcpy (result, payload + 2, length);

	return result;
}

typedef struct _MyQttReaderAsyncData {

	MyQttMsg            * msg;
	MyQttConn           * conn;
	MyQttReaderHandler    func;

} MyQttReaderAsyncData;

axlPointer __myqtt_reader_async_run_proxy (axlPointer _data)
{
	MyQttReaderAsyncData * data = _data;
	MyQttCtx             * ctx  = data->conn->ctx;
	
	/* call function */
	data->func (ctx, data->conn, data->msg, NULL);

	/* release references during the operation */
	myqtt_msg_unref (data->msg);
	myqtt_conn_unref (data->conn, "async-run-proxy");
	myqtt_ctx_unref (&ctx);

	/* release data */
	axl_free (data);

	/* return value expected by threaded handler */
	return NULL;
}

/** 
 * @internal Function used to create MyQttReaderData that is to convey
 * (conn, msg) into the threaded function and then call if everything
 * went ok during memory allocation.
 */
void __myqtt_reader_async_run (MyQttConn * conn, MyQttMsg * msg, MyQttReaderHandler func) 
{
	MyQttReaderAsyncData * data;
	MyQttCtx             * ctx = conn->ctx;

	if (! myqtt_ctx_ref (ctx)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to grab reference to the context for async handler activation, unable to handle incoming request");
		return;
	} /* end if */

	/* acquire a reference to the message */
	if (! myqtt_msg_ref (msg)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to create reader data to handle incoming request");

		myqtt_ctx_unref (&ctx);
		return;
	} /* end if */

	if (! myqtt_conn_ref (conn, "async-run-proxy")) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to handle incoming request, failed to acquire connection refere");

		myqtt_msg_unref (msg);
		myqtt_ctx_unref (&ctx);
		return;
	} /* end if */

	/* create data to pass it into the thread pool */
	data = axl_new (MyQttReaderAsyncData, 1);
	if (data == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to create reader data to handle incoming request");

		/* reduce message reference previously acquired */
		myqtt_msg_unref (msg);
		myqtt_ctx_unref (&ctx);
		myqtt_conn_unref (conn, "async-run-proxy");

		return;
	} /* end if */

	/* pack all data and call function */
	data->conn = conn;
	data->msg  = msg;
	data->func = func;

	/* run task */
	myqtt_thread_pool_new_task (ctx, __myqtt_reader_async_run_proxy, data);

	return;
}

axl_bool __myqtt_reader_check_client_id (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, MyQttConnAckTypes * response)
{
	struct timeval stamp;

	/* get clean session status */
	conn->clean_session = myqtt_get_bit (msg->payload[7], 1);

	if (strlen (conn->client_identifier) == 0) {
		/* check MQTT-3.1.3-7 */
		if (conn->clean_session == 0) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Received CONNECT request with empty client id and clean session == 0 (MQTT-3.1.3-7), denying connect");
			(*response) = MYQTT_CONNACK_IDENTIFIER_REJECTED;
			return axl_false;
		} /* end if */

		/* generate a random client id */
		gettimeofday (&stamp, NULL);
		axl_free (conn->client_identifier);
		conn->client_identifier = axl_strdup_printf ("%ld-%ld-%ld", conn, stamp.tv_sec, stamp.tv_usec);
		myqtt_log (MYQTT_LEVEL_WARNING, "Received CONNECT request with empty client id, created new client id: %s", conn->client_identifier);
	} /* end if */


	/* check to initialise client ids hash */
	if (ctx->client_ids == NULL) {
		/* initialize client ids hash */
		myqtt_mutex_lock (&ctx->client_ids_m);
		if (ctx->client_ids == NULL) 
			ctx->client_ids = axl_hash_new (axl_hash_string, axl_hash_equal_string);
		myqtt_mutex_unlock (&ctx->client_ids_m);
	} /* end if */

	/* check if client id is already registered */
	myqtt_mutex_lock (&ctx->client_ids_m);
	if (axl_hash_get (ctx->client_ids, conn->client_identifier) || ! myqtt_support_is_utf8 (conn->client_identifier, strlen (conn->client_identifier))) {
		/* client id found, reject it */
		(*response) = MYQTT_CONNACK_IDENTIFIER_REJECTED;

		myqtt_mutex_unlock (&ctx->client_ids_m);
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Rejected CONNECT request because client id %s is already in use, denying connect", conn->client_identifier);
		return axl_false; /* reject CONNECT */
	} /* end if */

	/* init storage if it has session */
	if (! conn->clean_session) {
		if (! myqtt_storage_init (ctx, conn, MYQTT_STORAGE_ALL)) {
			/* client id found, reject it */
			(*response) = MYQTT_CONNACK_SERVER_UNAVAILABLE;

			myqtt_mutex_unlock (&ctx->client_ids_m);
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to init storage service for provided client identifier '%s', unable to accept connection", conn->client_identifier);
			return axl_false; /* reject CONNECT */
		} /* end if */
	} /* end if */

	/* register client identifier */
	axl_hash_insert (ctx->client_ids, conn->client_identifier, conn);

	/* not found, everything ok */
	myqtt_mutex_unlock (&ctx->client_ids_m);

	return axl_true; /* no problem found */
}

void __myqtt_reader_move_offline_to_online_aux (MyQttCtx * ctx, MyQttConn * conn, axlHash * hash, axlHash * offline_hash)
{
	axlHashCursor * cursor;
	const char    * topic_filter;
	axlHash       * sub_hash;

	/* CONTEXT: lock subscribtions to remove connection subscription */
	myqtt_mutex_lock (&ctx->subs_m);

	/* check no publish operation is taking place now */
	while (ctx->publish_ops > 0)
		myqtt_cond_timedwait (&ctx->subs_c, &ctx->subs_m, 10000);

	/* move online subscriptions to offline subs */
	cursor = axl_hash_cursor_new (hash);
	while (axl_hash_cursor_has_item (cursor)) {
		/* get values to go offline */
		topic_filter = (const char *) axl_hash_cursor_get_key (cursor);

		/* now get subhash */
		sub_hash     = axl_hash_get (offline_hash, (axlPointer) topic_filter);
		if (sub_hash) {
			/* and remove client id from here */
			axl_hash_remove (sub_hash, conn->client_identifier);
		} /* end if */

		/* go for the next registry */
		axl_hash_cursor_next (cursor);
	} /* end if */

	/* release lock */
	myqtt_mutex_unlock (&ctx->subs_m);

	axl_hash_cursor_free (cursor);

	return;
}

void __myqtt_reader_move_offline_to_online (MyQttCtx * ctx, MyQttConn * conn)
{
	/* nothing to unsubscribe from offline because there is
	 * nothing online */
	if (axl_hash_items (conn->subs) == 0 && axl_hash_items (conn->wild_subs) == 0)
		return;

	/* move offline subscriptions to online subs */
	__myqtt_reader_move_offline_to_online_aux (ctx, conn, conn->subs, ctx->offline_subs);

	/* move offline subscriptions to online subs */
	__myqtt_reader_move_offline_to_online_aux (ctx, conn, conn->wild_subs, ctx->offline_wild_subs);
	return;
}

void __myqtt_reader_handle_connect (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data) 
{
	/** 
	 * By default all connections are accepted. User application
	 * can set their own handler to control this.
	 */
	MyQttConnAckTypes   response = MYQTT_CONNACK_ACCEPTED;
	int                 desp;
	unsigned char     * reply;
	int                 size;

	/* const char * username = NULL;
	   const char * password = NULL; */

	/* check if the connection was already accepted */
	if (conn->connect_received) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received second CONNECT request, protocol violation, closing conn-id=%d from %s:%s", conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* flag that CONNECT message was received over this connection */
	conn->connect_received = axl_true;

	/* check protocol declaration */
	if (! axl_memcmp ((const char * ) msg->payload + 2, "MQTT", 4)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Expected to receive MQTT indication, but found something different, closing conn-id=%d from %s:%s", conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* check protocol level */
	if (msg->payload[6] != 4) {
		myqtt_log (MYQTT_LEVEL_WARNING, "Expected to receive MQTT protocol level 4 but found %d, conn-id=%d from %s:%s", 
			   msg->payload[6], conn->id, conn->host, conn->port);
		
	} /* end if */

	/* get keep alive configuration */
	conn->keep_alive = myqtt_get_16bit (msg->payload + 8);

	/* get client identifier */
	desp = 10;
	conn->client_identifier = __myqtt_reader_get_utf8_string (ctx, msg->payload + desp, msg->size - desp);
	if (! conn->client_identifier) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Undefined client identifier, closing conn-id=%d from %s:%s", conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* only update desp if client_identifier is defined */
	if (conn->client_identifier) {
		/* update desp */
		desp += (strlen (conn->client_identifier) + 2);
	} /* end if */

	/* check client identifier */
	if (! __myqtt_reader_check_client_id (ctx, conn, msg, &response)) 
		goto connect_send_reply;

	/* now get the will topic if indicated */
	if (myqtt_get_bit (msg->payload[7], 2)) {
		/* will flag is on, find will topic and will message */
		conn->will_topic = __myqtt_reader_get_utf8_string (ctx, msg->payload + desp, msg->size - desp);
		if (! conn->will_topic) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Undefined will topic, closing conn-id=%d from %s:%s", conn->id, conn->host, conn->port);
			myqtt_conn_shutdown (conn);
			return;
		} /* end if */

		/* update desp */
		desp += (strlen (conn->will_topic) + 2);

		/* will flag is on, find will topic and will message */
		conn->will_msg = __myqtt_reader_get_utf8_string (ctx, msg->payload + desp, msg->size - desp);
		if (! conn->will_msg) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Undefined will msg, closing conn-id=%d from %s:%s", conn->id, conn->host, conn->port);
			myqtt_conn_shutdown (conn);
			return;
		} /* end if */

		/* get will qos */
		if (myqtt_get_bit (msg->payload[7], 4))
			conn->will_qos = MYQTT_QOS_2;
		else if (myqtt_get_bit (msg->payload[7], 3))
			conn->will_qos = MYQTT_QOS_1;
		else
			conn->will_qos = MYQTT_QOS_0; /* this statement is not neccessary but helps understanding the code */

		/* update desp */
		desp += (strlen (conn->will_msg) + 2);
	} /* end if */

	/* now get user and password */
	if (myqtt_get_bit (msg->payload[7], 7)) {
		/* username flag on, get username */
		conn->username = __myqtt_reader_get_utf8_string (ctx, msg->payload + desp, msg->size - desp);
		if (! conn->username) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Undefined username, closing conn-id=%d from %s:%s", conn->id, conn->host, conn->port);
			myqtt_conn_shutdown (conn);
			return;
		} /* end if */

		/* update desp */
		desp += (strlen (conn->username) + 2);
	} /* end if */

	/* now get password */
	if (myqtt_get_bit (msg->payload[7], 6)) {
		/* username flag on, get username */
		conn->password = __myqtt_reader_get_utf8_string (ctx, msg->payload + desp, msg->size - desp);
		if (! conn->password) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Undefined username, closing conn-id=%d from %s:%s", conn->id, conn->host, conn->port);
			myqtt_conn_shutdown (conn);
			return;
		} /* end if */

		/* update desp */
		desp += (strlen (conn->password) + 2);
	} /* end if */

	/* report that we've received a connection */
	myqtt_log (MYQTT_LEVEL_DEBUG, "Received CONNECT request conn-id=%d from %s:%s, client-identifier=%s, keep alive=%d, username=%s, password=%s", 
		   conn->id, conn->host, conn->port, 
		   conn->client_identifier ? conn->client_identifier : "",
		   conn->keep_alive,
		   conn->username ? conn->username : "",
		   conn->password ? "*****" : "");

	/* call here to autorize connect request received */
	if (ctx->on_connect) {
		/* call to user level application to decide about this
		 * new request */
		response = ctx->on_connect (ctx, conn, ctx->on_connect_data);
	} /* end if */

	/* after on_connect has finished, clear password */
	if (conn->password) {
		axl_free (conn->password);
		conn->password = NULL;
	} /* end if */

	/* handle deferred responses */
	if (response == MYQTT_CONNACK_DEFERRED) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "CONNECT decision deferred");
		return;
	} /* end if */

connect_send_reply:

	/* session recovery: connection accepted, if it has session recover */
	if (! conn->clean_session && response == MYQTT_CONNACK_ACCEPTED) {
		/* move subscriptions from offline to online */
		if (! myqtt_storage_session_recover (ctx, conn)) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to recover session for the provided connection, unable to accept connection");
			response = MYQTT_CONNACK_SERVER_UNAVAILABLE;
		} else {
			
			/* session recovered, now remove offline subscriptions */
			__myqtt_reader_move_offline_to_online (ctx, conn);

		} /* end if */


	} /* end if */

	/* rest of cases, reply with the response */
	reply = myqtt_msg_build (ctx, MYQTT_CONNACK, axl_false, 0, axl_false, &size, 
				 /* variable header and payload */
				 MYQTT_PARAM_16BIT_INT, response, 
				 MYQTT_PARAM_END);

	/* send message */
	if (! myqtt_msg_send_raw (conn, reply, size))
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send CONNACK message, errno=%d", errno);

	/* free reply */
	myqtt_msg_free_build (ctx, reply, size);

	/* close connection in the case it is not an accepted */
	if (response != MYQTT_CONNACK_ACCEPTED) {
		myqtt_conn_shutdown (conn);
		myqtt_log (MYQTT_LEVEL_WARNING, "Connection conn-id=%d denied from %s:%s", conn->id, conn->host, conn->port);
	} else {
		myqtt_log (MYQTT_LEVEL_DEBUG, "Connection conn-id=%d accepted from %s:%s", conn->id, conn->host, conn->port);

		/* resend messages queued */
		if (myqtt_storage_queued_messages (ctx, conn) > 0) {
			/* we have pending messages, order to deliver them */
			myqtt_storage_queued_flush (ctx, conn);
		} /* end if */

	} /* end if */


	return;
}

void __myqtt_reader_recover_retained_message (MyQttCtx * ctx, MyQttConn * conn, const char * topic_filter)
{
	unsigned char * app_msg;
	int             app_msg_size;
	MyQttQos        qos;

	/* recover message with direct topic_filter */
	/* basic case: topic filter that aren't wild cards */
	
	if (! myqtt_storage_retain_msg_recover (ctx, topic_filter, &qos, &app_msg, &app_msg_size))
		return; /* nothing found */

	/* found message, send it to the client */
	if (! myqtt_conn_pub (conn, topic_filter, app_msg, app_msg_size, qos, axl_true /* report this is retained */, 0))
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to send message due to retained subscription, myqtt_conn_pub() failed");
	
	/* now release content */
	axl_free (app_msg);
	return;
}

/** 
 * @internal Function used to subcribe the provided topic filter / qos
 * for the provided connection into the in-memory maps used by MyQtt
 * to route all traffic.
 *
 * @param ctx The context where the operation will take place
 *
 * @param conn The connection where the operaion will take place.
 *
 * @param topic_filter The topic filter that is going to be added to
 * the system. IMPORTANT: this parameter must be allocated by the
 * caller and yield to the function for its deallocation using
 * axl_free.
 *
 * @param qos The qos that is going to configured.
 *
 */
void __myqtt_reader_subscribe (MyQttCtx * ctx, const char * client_identifier, MyQttConn * conn,  char * topic_filter, MyQttQos qos, axl_bool __is_offline)
{
	axlHash   * hash;
	axlHash   * sub_hash;
	axl_bool    should_release = axl_true;

	if (ctx == NULL || topic_filter == NULL)
		return;

	/* if it is not an offline operation, check conn reference */
	if (! __is_offline && conn == NULL)
		return;

	if (! __is_offline) {
		/** CONNECTION REGISTRY **/
		/* reached this point, subscription is accepted */
		myqtt_mutex_lock (&conn->op_mutex);
		if ((strstr (topic_filter, "#") != NULL) || (strstr (topic_filter, "+") != NULL))
			axl_hash_insert_full (conn->wild_subs, (axlPointer) topic_filter, axl_free, INT_TO_PTR (qos), NULL);
		else
			axl_hash_insert_full (conn->subs, (axlPointer) topic_filter, axl_free, INT_TO_PTR (qos), NULL);
		myqtt_mutex_unlock (&conn->op_mutex);
	} /* end if */

	/** CONTEXT REGISTRY **/
	/* now, register this connection into the hash of subscribed
	 * connections, if required */
	myqtt_mutex_lock (&ctx->subs_m);

	/* check no publish operation is taking place now. 
	 * See __myqtt_reader_do_publish for more information */
	while (ctx->publish_ops > 0)
		myqtt_cond_timedwait (&ctx->subs_c, &ctx->subs_m, 10000);
	
	/* check if topic filter is already registred, if not create base structure */
	if ((strstr (topic_filter, "#") != NULL) || (strstr (topic_filter, "+") != NULL)) 
		hash = __is_offline ? ctx->offline_wild_subs : ctx->wild_subs;
	else
		hash = __is_offline ? ctx->offline_subs : ctx->subs;
	
	/* check if the hash hold connections interested in
	 * this topic filter is already created, if not,
	 * proceed */
	sub_hash = axl_hash_get (hash, (axlPointer) topic_filter);
	myqtt_log (MYQTT_LEVEL_DEBUG, "Sub hash for hash %p (items: %d) holding topic_filter='%s' is %p (offline=%d)", hash, axl_hash_items (hash), topic_filter, sub_hash, __is_offline);
	if (! sub_hash) {
		/* create empty hash: when offline the hash contains
		 * client ids, when on-line, it contains reference to
		 * connections to do the delivery directly */
		if (__is_offline) { /* offline */
			sub_hash       = axl_hash_new (axl_hash_string, axl_hash_equal_string);
			should_release = axl_false;
		} else {  /* online*/
			sub_hash       = axl_hash_new (axl_hash_int, axl_hash_equal_int);
		}
		axl_hash_insert_full (hash, 
				      /* key and destroy */
				      __is_offline ? topic_filter : axl_strdup (topic_filter), axl_free, 
				      /* value and destroy */
				      sub_hash, (axlDestroyFunc) axl_hash_free);
		myqtt_log (MYQTT_LEVEL_DEBUG, "  ..created hash for topic_filter='%s' is %p", topic_filter, sub_hash);
	} /* end if */
	
	/* record this connection and requested qos */
	if (__is_offline) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "   ..storing client id %s with qos %d client id hash %p for topic_filter='%s'", client_identifier, qos, sub_hash, topic_filter);
		axl_hash_insert_full (sub_hash, strdup (client_identifier), axl_free, INT_TO_PTR (qos), NULL);
	} else {
		myqtt_log (MYQTT_LEVEL_DEBUG, "   ..storing connection %p with qos %d on conn hash %p for topic_filter='%s'", conn, qos, sub_hash, topic_filter);
		axl_hash_insert (sub_hash, conn, INT_TO_PTR (qos));
	} /* end if */

	if (__is_offline && should_release) {
		/* if offline and the hash was created, this reference then must be released */
		axl_free (topic_filter);
	} /* end if */
	
	/* release lock */
	myqtt_mutex_unlock (&ctx->subs_m);

	/* now recover retained message if any and send it to this
	   client */
	__myqtt_reader_recover_retained_message (ctx, conn, topic_filter);

	return;
}

void __myqtt_reader_handle_subscribe (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer _data) {

	/* local parameters */
	int                      packet_id;
	char                   * topic_filter;
	MyQttQos                 qos;
	int                      desp;
	int                    * replies_mem = NULL;
	int                      replies = 0;
	unsigned char          * reply;
	int                      iterator;

	/* check if this is a listener */
	if (conn->role != MyQttRoleListener) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received SUBSCRIBE request over a connection that is not a listener from conn-id=%d from %s:%s, closing connection..", 
			   conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);

		return;
	} /* end if */

	/* get packet id */
	packet_id = myqtt_get_16bit (msg->payload);
	
	/* get subscriptions */
	desp = 2;
	while (msg->size - desp > 0) {
		/* get subscription */
		topic_filter = __myqtt_reader_get_utf8_string (ctx, msg->payload + desp, msg->size - desp);
		if (topic_filter == NULL) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Received NULL topic filter on SUBSCRIBE packet, closing connection conn-id=%d from %s:%s, closing connection..", 
				   conn->id, conn->host, conn->port);
			myqtt_conn_shutdown (conn);
			axl_free (replies_mem);

			return;
		} /* end if */

		if (! myqtt_support_is_utf8 (topic_filter, strlen (topic_filter))) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Received SUBSCRIBE message wrong UTF-8 encoding on topic filter closing conn-id=%d from %s:%s", conn->id, conn->host, conn->port);
			myqtt_conn_shutdown (conn);

			/* release memory */
			axl_free (replies_mem);
			axl_free (topic_filter);
			return;
		} /* end if */

		/* increase desp */
		desp += (strlen (topic_filter) + 2);

		if (desp >= msg->size) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Received missing QoS for topic filter %s on SUBSCRIBE packet, closing connection conn-id=%d from %s:%s, closing connection..", 
				   topic_filter, conn->id, conn->host, conn->port);
			myqtt_conn_shutdown (conn);
			axl_free (replies_mem);

			/* release topic filter */
			axl_free (topic_filter);

			return;
		}

		/* get qos */
		qos   = myqtt_get_8bit (msg->payload + desp);

		/* increase desp */
		desp += 1;
		
		myqtt_log (MYQTT_LEVEL_DEBUG, "Received SUBSCRIBE pkt-id=%d request with topic_filter '%s' and QoS=%d on conn-id=%d from %s:%s", 
			   packet_id, topic_filter, qos, conn->id, conn->host, conn->port);

		/* call to check if user level accept this subscription */
		if (ctx->on_subscribe) 
			qos = ctx->on_subscribe (ctx, conn, topic_filter, qos, ctx->on_subscribe_data);

		if (replies_mem == NULL) 
			replies_mem = axl_new (int, 1);
		else
			replies_mem = realloc (replies_mem, sizeof (int) * (replies + 1));

		/* increase replies */
		replies ++;

		/* call to save on storage if there is session */
		if (! conn->clean_session) {
			if (! myqtt_storage_sub (ctx, conn, topic_filter, qos)) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send subscription received to storage, denying subscription"); 
				qos = MYQTT_QOS_DENIED;
			} /* end if */

		} /* end if */

		/* store value */
		replies_mem [replies - 1] = qos;

		if (qos == MYQTT_QOS_DENIED) {
			/* release topic filter */
			axl_free (topic_filter);
			continue; 
		} /* end if */

		/** CONNECTION REGISTRY **/
		__myqtt_reader_subscribe (ctx, conn->client_identifier, conn, topic_filter, qos, /* offline */ axl_false);
		
	} /* end while */

	/* build reply SUBACK */
	reply = axl_new (unsigned char, replies + 4);
	if (reply == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to allocate memory to handle SUBACK reply from conn-id=%d from %s:%s", 
			   conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);
		axl_free (replies_mem);

		return;

	} /* end if */
	
	/* configure header */
	reply[0] = (( 0x00000f & MYQTT_SUBACK) << 4);

	/* configure remaining length */
	desp     = 0;
	myqtt_msg_encode_remaining_length (ctx, reply + 1, replies + 2, &desp);

	/* set packet id in reply */
	/* myqtt_log (MYQTT_LEVEL_DEBUG, "Saving packet id %d on desp=%d", packet_id, desp + 1); */
	myqtt_set_16bit (packet_id, reply + desp + 1);
	desp   += 2;
	
	/* configure all subscription replies */
	iterator = 0;
	while (iterator < replies) {
		reply[desp + iterator + 1] = replies_mem[iterator];
		/* myqtt_log (MYQTT_LEVEL_DEBUG, "Saving sub reply %d on byte %d", replies_mem[iterator], desp+ iterator); */
		iterator++;
	} /* end if */

	/* myqtt_show_byte (ctx, reply[0], "header");
	   myqtt_show_byte (ctx, reply[1], "re.len");
	   myqtt_show_byte (ctx, reply[2], "msb pk");
	   myqtt_show_byte (ctx, reply[3], "lsb pk");
	   myqtt_show_byte (ctx, reply[4], "r code"); */

	/* release memory */
	axl_free (replies_mem);

	/* send message */
	if (! myqtt_sequencer_send (conn, MYQTT_SUBACK, reply, replies + 4))
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send SUBACK message, errno=%d", errno);

	myqtt_log (MYQTT_LEVEL_DEBUG, "Sent SUBACK reply to conn-id=%d at %s:%s..", conn->id, conn->host, conn->port);

	return;
}

void __myqtt_reader_move_online_to_offline_aux (MyQttCtx * ctx, MyQttConn * conn, axlHash * hash)
{
	axlHashCursor * cursor;
	const char    * topic_filter;
	MyQttQos        qos;

	/* move online subscriptions to offline subs */
	cursor = axl_hash_cursor_new (hash);
	while (axl_hash_cursor_has_item (cursor)) {
		/* get values to go offline */
		topic_filter = (const char *) axl_hash_cursor_get_key (cursor);
		qos          = PTR_TO_INT (axl_hash_cursor_get_value (cursor));

		/* register and create a new reference to topic_filter */
		__myqtt_reader_subscribe (ctx, conn->client_identifier, NULL, axl_strdup (topic_filter), qos, axl_true /* offline */);

		/* go for the next registry */
		axl_hash_cursor_next (cursor);
	} /* end if */

	axl_hash_cursor_free (cursor);
	return;
}

void __myqtt_reader_move_online_to_offline (MyQttCtx * ctx, MyQttConn * conn)
{
	/* skip step if no subscription is found */
	if (axl_hash_items (conn->subs) == 0 && axl_hash_items (conn->wild_subs) == 0)
		return;

	/* move online subscriptions to offline subs */
	__myqtt_reader_move_online_to_offline_aux (ctx, conn, conn->subs);

	/* move online subscriptions to offline subs */
	__myqtt_reader_move_online_to_offline_aux (ctx, conn, conn->wild_subs);

	return;
}

void __myqtt_reader_handle_disconnect (MyQttCtx * ctx, MyQttMsg * msg, MyQttConn * conn)
{
	myqtt_log (MYQTT_LEVEL_DEBUG, "Received DISCONNECT notification for conn-id=%d from %s:%s, closing connection..", conn->id, conn->host, conn->port);
	myqtt_conn_shutdown (conn);

	/* skip will publication */
	if (conn->will_topic) {
		axl_free (conn->will_topic);
		conn->will_topic = NULL;
	} /* end if */
	if (conn->will_msg) {
		axl_free (conn->will_msg);
		conn->will_msg = NULL;
	} /* end if */

	if (! conn->clean_session) {
		/* move current sessions to offline sessions */
		__myqtt_reader_move_online_to_offline (ctx, conn);
	} /* end if */

	return;
}

void __myqtt_reader_handle_wait_reply (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer _data)
{
	MyQttAsyncQueue        * queue;

 	/* check if this is a listener */
	if (conn->role != MyQttRoleInitiator) {
		if (msg->type == MYQTT_SUBACK)
			myqtt_conn_report_and_close (conn, "Received SUBACK request over a connection that is not an initiator");
		else if (msg->type == MYQTT_UNSUBACK)
			myqtt_conn_report_and_close (conn, "Received UNSUBACK request over a connection that is not an initiator");
		else if (msg->type == MYQTT_PINGRESP)
			myqtt_conn_report_and_close (conn, "Received PINGRESP request over a connection that is not an initiator");
		else if (msg->type == MYQTT_PUBREC)
			myqtt_conn_report_and_close (conn, "Received PUBREC request over a connection that is not an initiator");
		else if (msg->type == MYQTT_PUBCOMP)
			myqtt_conn_report_and_close (conn, "Received PUBCOMP request over a connection that is not an initiator");

		return;
	} /* end if */

	if ((msg->type == MYQTT_PUBACK   && msg->size != 2) || 
	    (msg->type == MYQTT_PUBREC   && msg->size != 2) || 
	    (msg->type == MYQTT_PUBCOMP  && msg->size != 2) || 
	    (msg->type == MYQTT_SUBACK   && msg->size < 3) || 
	    (msg->type == MYQTT_UNSUBACK && msg->size < 2) || 
	    (msg->type == MYQTT_PINGRESP && msg->size != 0) ) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "%s message size=%d without header and remaining length is insufficient", myqtt_msg_get_type_str (msg), msg->size);

		if (msg->type == MYQTT_SUBACK)
			myqtt_conn_report_and_close (conn, "Received poorly formed SUBACK request that do not contains bare minimum bytes");
		else if (msg->type == MYQTT_UNSUBACK)
			myqtt_conn_report_and_close (conn, "Received poorly formed UNSUBACK request that do not contains bare minimum bytes");
		else if (msg->type == MYQTT_PINGRESP)
			myqtt_conn_report_and_close (conn, "Received poorly formed PINGRESP request that contains unexpected content");
		else if (msg->type == MYQTT_PUBACK)
			myqtt_conn_report_and_close (conn, "Received poorly formed PUBACK request that contains unexpected content");
		else if (msg->type == MYQTT_PUBREC)
			myqtt_conn_report_and_close (conn, "Received poorly formed PUBREC request that contains unexpected content");
		else if (msg->type == MYQTT_PUBCOMP)
			myqtt_conn_report_and_close (conn, "Received poorly formed PUBCOMP request that contains unexpected content");

		return;
	} /* end if */

	/* get packet id */
	if (msg->type == MYQTT_PINGRESP)
		msg->packet_id = -1;
	else
		msg->packet_id = myqtt_get_16bit (msg->payload);
	myqtt_log (MYQTT_LEVEL_DEBUG, "Pushing %s msg=%d, for packet-id=%d", myqtt_msg_get_type_str (msg), msg->id, msg->packet_id);

	/* now call to wait queue if defined */
	myqtt_mutex_lock (&conn->op_mutex);

	/* get queue and remove it from the set of wait replies */
	if (msg->type == MYQTT_PINGRESP)
		queue = conn->ping_resp_queue;
	else
		queue = axl_hash_get (conn->wait_replies, INT_TO_PTR (msg->packet_id));

	/* release lock */
	myqtt_mutex_unlock (&conn->op_mutex);

	if (queue == NULL) {
		if (msg->packet_id != -1 && msg->type != MYQTT_PINGRESP) {
			/* too late, unable to deliver message to wait reply */
			myqtt_log (MYQTT_LEVEL_WARNING, 
				   "Received a %s message but queue to handle reply wasn't there...it seems we arrived too late or packet id=%d do not match anything on our side",
				   myqtt_msg_get_type_str (msg), msg->packet_id);
		} /* end if */

		return;
	} /* end if */

	/* acquire reference and push content */
	myqtt_msg_ref (msg);
	myqtt_async_queue_push (queue, msg);

	/* NOTE: do not call to release myqtt_msg_unref because we are "delegating" this reference to the queue waiter */

	return;
}

void __myqtt_reader_handle_unsubscribe (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer _data)
{
	/* local parameters */
	int                      packet_id;
	char                   * topic_filter;
	int                      desp = 0;
	axlHash                * sub_hash;
	unsigned char          * reply;
	int                      size;

	/* check if this is a listener */
	if (conn->role != MyQttRoleListener) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received UNSUBSCRIBE request over a connection that is not a listener from conn-id=%d from %s:%s, closing connection..", 
			   conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);

		return;
	} /* end if */

	if (msg->size < 3) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received UNSUBSCRIBE request without enough data to continue with security from conn-id=%d from %s:%s, closing connection..", 
			   conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);

		return;
	} /* end if */

	/* get packet id */
	packet_id   = myqtt_get_16bit (msg->payload);
	desp       += 2;

	/* get topic filter */
	while (desp < msg->size) {
		topic_filter = __myqtt_reader_get_utf8_string (ctx, msg->payload + desp, msg->size - desp);
		if (topic_filter == NULL) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to process topic filter from received UNSUBSCRIBE request from conn-id=%d from %s:%s, closing connection..", 
				   conn->id, conn->host, conn->port);
			myqtt_conn_shutdown (conn);
			
			return;
		} /* end if */

		/* increase desp */
		desp += (strlen (topic_filter) + 2);

		/* call to save on storage if there is session */
		if (! conn->clean_session) {
			if (! myqtt_storage_unsub (ctx, conn, topic_filter)) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send to storage unsubscribe received"); 
			} /* end if */
		} /* end if */

		/* CONNECTION CONTEXT: remove subscription from the connection */
		myqtt_mutex_lock (&conn->op_mutex);
		if ((strstr (topic_filter, "#") != NULL) || (strstr (topic_filter, "+") != NULL))
			axl_hash_remove (conn->wild_subs, topic_filter);
		else
			axl_hash_remove (conn->subs, topic_filter);
		myqtt_mutex_unlock (&conn->op_mutex);

		/* CONTEXT: lock subscribtions to remove connection subscription */
		myqtt_mutex_lock (&ctx->subs_m);

		/* check no publish operation is taking place now */
		while (ctx->publish_ops > 0)
			myqtt_cond_timedwait (&ctx->subs_c, &ctx->subs_m, 10000);
		
		/* now get connection hash handling that topic */
		sub_hash = axl_hash_get (ctx->subs, (axlPointer) topic_filter);
		if (! sub_hash) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Expected to find connection hash under the topic %s but found NULL reference [internal engine error]..", 
				   topic_filter);
			/* release lock */
			myqtt_mutex_unlock (&ctx->subs_m);
			continue;
		} /* end if */

		/* remove the connection from that connection hash */
		axl_hash_remove (sub_hash, conn);

		/* release lock */
		myqtt_mutex_unlock (&ctx->subs_m);

		/* release topic filter */
		axl_free (topic_filter);

	} /* end if */

	/* send reply */
	reply = myqtt_msg_build (ctx, MYQTT_UNSUBACK, axl_false, 0, axl_false, &size, /* 2 bytes */
				 MYQTT_PARAM_16BIT_INT, packet_id,
				 MYQTT_PARAM_END);
	if (reply == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to allocate memory required for UNSUBSCRIBE reply for conn-id=%d from %s:%s, closing connection..", 
			   conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);
			
		return;
	} /* end if */

	/* send message */
	if (! myqtt_sequencer_send (conn, MYQTT_UNSUBACK, reply, size))
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send UNSUBACK message, errno=%d", errno);

	myqtt_log (MYQTT_LEVEL_DEBUG, "Sent UNSUBACK reply to conn-id=%d at %s:%s..", conn->id, conn->host, conn->port);

	/* free reply */
	/* myqtt_msg_free_build (ctx, reply, size); */

	return;
}

void __myqtt_reader_queue_offline (MyQttCtx * ctx, MyQttMsg * msg, axlHash * sub_hash)
{
	axlHashCursor * cursor;
	const char    * client_identifier;
	MyQttQos        qos;

	if (! sub_hash)
		return;

	/* found topic registered, now iterate over all
	 * registered connections to send the message */
	cursor = axl_hash_cursor_new (sub_hash);
	axl_hash_cursor_first (cursor);
	while (axl_hash_cursor_has_item (cursor)) {

		/* get client identifier */
		client_identifier = axl_hash_cursor_get_key (cursor);
		
		/* get qos to publish */
		qos  = msg->qos;
		
		/* check to downgrade publication qos to the
		 * value of the subscription */
		if (qos > PTR_TO_INT (axl_hash_cursor_get_value (cursor)))
			qos = PTR_TO_INT (axl_hash_cursor_get_value (cursor));
		
		/* publish message */
		myqtt_log (MYQTT_LEVEL_DEBUG, "Publishing offline topic name '%s', qos: %d (app msg size: %d) on client id session %p", 
			   msg->topic_name, qos, msg->app_message_size, client_identifier);
		if (! myqtt_conn_offline_pub (ctx, client_identifier, msg->topic_name, (axlPointer) msg->app_message, msg->app_message_size, qos, axl_false))
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send PUBLISH message, errno=%d", errno); 
		
		/* next item */
		axl_hash_cursor_next (cursor);
	} /* end if */

	/* release cursor */
	axl_hash_cursor_free (cursor);
	return;
}

/** 
 * @internal Handle retained message (see if we have to store it,
 * release,..)
 */
void __myqtt_reader_handle_retained_msg (MyQttCtx * ctx, MyQttMsg * msg)
{
	/* do nothing if retain flag is not set */
	if (! msg->retain)
		return;

	if (msg->qos == MYQTT_QOS_0 || msg->app_message_size == 0) {
		/* remove retained message if any */
		myqtt_storage_retain_msg_release (ctx, msg->topic_name);
		return;
	} /* end if */

	/* save message for later publication */
	myqtt_storage_retain_msg_set (ctx, msg->topic_name, msg->qos, msg->app_message, msg->app_message_size);

	return;
} /* end if */
      

/** 
 * @internal Fucntion to implement global publishing. ctx, conn and
 * msg must be defined.
 */
void __myqtt_reader_do_publish (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg)
{
	MyQttPublishCodes        pub_codes;
	axlHash                * sub_hash;
	axlHashCursor          * cursor;
	MyQttQos                 qos;

	if (ctx->on_publish) {
		/* call to on publish */
		pub_codes = ctx->on_publish (ctx, conn, msg, ctx->on_publish_data);
		switch (pub_codes) {
		case MYQTT_PUBLISH_OK:
			/* just break, do nothing */
			break;
		case MYQTT_PUBLISH_DISCARD:
			/* releaes message and return */
			myqtt_log (MYQTT_LEVEL_WARNING, "On publish handler reported to discard msg-id=%d from conn-id=%d from %s:%s", 
				   msg->id, conn->id, conn->host, conn->port);
			return;
		case MYQTT_PUBLISH_CONN_CLOSE:
			/* connection close */
			myqtt_log (MYQTT_LEVEL_WARNING, "On publish handler reported to close connection for msg-id=%d from conn-id=%d from %s:%s, closing connection..", 
				   msg->id, conn->id, conn->host, conn->port);
			myqtt_conn_shutdown (conn);
			return;
		} /* end if */
	} /* end if */

	/**** SERVER HANDLING ****
	 *
	 * NOTES: the following try to "signal" how many operations are taking place right now. 
	 *
	 * Then this value is sed by subscription code (that modifies these hashes) to ensure
	 * nobody is touching the hashes... this we this code can work in thread safe mode without 
	 * having to lock the mutex during the publish operation. 
	 *
	 * In short, the code ensures these hashes are static structures.
	 */

	/* check for packages with retain flag */
	if (msg->retain) {
		/* handle retained messages */
		__myqtt_reader_handle_retained_msg (ctx, msg);
	} /* end if */
	
	/* notify we are publishing */
	myqtt_mutex_lock (&ctx->subs_m);
	ctx->publish_ops++;
	myqtt_mutex_unlock (&ctx->subs_m);

	/* get the hash */
	sub_hash = axl_hash_get (ctx->subs, (axlPointer) msg->topic_name);
	if (sub_hash) {
		/* found topic registered, now iterate over all
		 * registered connections to send the message */
		cursor = axl_hash_cursor_new (sub_hash);
		axl_hash_cursor_first (cursor);
		while (axl_hash_cursor_has_item (cursor)) {

			/* get connection and qos */
			conn = axl_hash_cursor_get_key (cursor);

			/* skip connection because it is not ok */
			if (! myqtt_conn_is_ok (conn, axl_false)) {
				/* next item */
				axl_hash_cursor_next (cursor);
				continue;
			} /* end if */

			/* get qos to publish */
			qos  = msg->qos;

			/* check to downgrade publication qos to the
			 * value of the subscription */
			if (qos > PTR_TO_INT (axl_hash_cursor_get_value (cursor)))
				qos = PTR_TO_INT (axl_hash_cursor_get_value (cursor));

			/* skip storage for qos1 because it is already stored on the sender */
			if (qos == MYQTT_QOS_1)
				qos |= MYQTT_QOS_SKIP_STORAGE;

			/* publish message */
			myqtt_log (MYQTT_LEVEL_DEBUG, "Publishing topic name '%s', qos: %d (app msg size: %d) on conn %p", 
				   msg->topic_name, qos, msg->app_message_size, conn);
			/* retain = axl_false always : MQTT-2.1.2-11 */
			if (! myqtt_conn_pub (conn, msg->topic_name, (axlPointer) msg->app_message, msg->app_message_size, qos, axl_false, 0))
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to publish message message, errno=%d", errno); 
			
			/* next item */
			axl_hash_cursor_next (cursor);
		} /* end if */

		/* release cursor */
		axl_hash_cursor_free (cursor);
	} else {
		/* no one interested in this, no one subscribed to received this */
		myqtt_log (MYQTT_LEVEL_DEBUG, "Published topic name '%s' but no one was subscribed to it", msg->topic_name);
	}

	/* publish on wild subs */

	/* publish on offline subs */
	sub_hash = axl_hash_get (ctx->offline_subs, (axlPointer) msg->topic_name);
	__myqtt_reader_queue_offline (ctx, msg, sub_hash);

	/* publish on offline wild subs */

	/* notify we have finished publishing */
	myqtt_mutex_lock (&ctx->subs_m);
	ctx->publish_ops--;
	myqtt_mutex_unlock (&ctx->subs_m);

	/* finish don */
	return;
}


/** 
 * @internal PUBLISH handling..
 */
void __myqtt_reader_handle_publish (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer _data)
{
	/* local variables */
	int                      desp = 0;
	unsigned char          * reply;

	/* parse content received inside message */
	msg->topic_name = __myqtt_reader_get_utf8_string (ctx, msg->payload, msg->size);
	if (! msg->topic_name ) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received PUBLISH message without topic name closing conn-id=%d from %s:%s", conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);

		return;
	} /* end if */

	if (! myqtt_support_is_utf8 (msg->topic_name, strlen (msg->topic_name))) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received PUBLISH message wrong UTF-8 encoding on topic name closing conn-id=%d from %s:%s", conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* increase desp */
	desp       += (strlen (msg->topic_name) + 2);

	/* now, according to the qos, we have also a packet id */
	if (msg->qos > MYQTT_QOS_0) {
		/* get packet id */
		msg->packet_id   = myqtt_get_16bit (msg->payload + desp); 
		desp            += 2;
	} /* end if */

	/* get payload reference */
	msg->app_message         = msg->payload + desp;
	msg->app_message_size    = msg->size - desp;

	myqtt_log (MYQTT_LEVEL_DEBUG, "PUBLISH: received request to publish (qos: %d, topic name: %s, packet id: %d, app msg size: %d, msg size: %d)",
		   msg->qos, msg->topic_name, msg->packet_id, msg->app_message_size, msg->size);

	/**** CLIENT HANDLING **** 
	 * we have received a PUBLISH packet as client, we have to
	 * notify it to the application level */
	if (conn->role == MyQttRoleInitiator) {
		/* call to notify message */
		if (conn->on_msg)
			conn->on_msg (conn, msg, conn->on_msg_data);
		else if (ctx->on_msg)
			ctx->on_msg (conn, msg, ctx->on_msg_data);
		return;
	} /* end if */

	/* call to do publish */
	__myqtt_reader_do_publish (ctx, conn, msg);

	/* now, for QoS1 we have to reply with a puback */
	if (msg->qos == MYQTT_QOS_1 || msg->qos == MYQTT_QOS_2) {
		/* configure header */
		reply    = axl_new (unsigned char, 4);
		if (reply == NULL)
			return;

		if (msg->qos == MYQTT_QOS_1)
			reply[0] = (( 0x00000f & MYQTT_PUBACK) << 4);
		else
			reply[0] = (( 0x00000f & MYQTT_PUBREC) << 4);

		/* configure remaining length */
		reply[1] = 2;

		/* set packet id in reply */
		/* myqtt_log (MYQTT_LEVEL_DEBUG, "Saving packet id %d on desp=%d", packet_id, desp + 1); */
		myqtt_set_16bit (msg->packet_id, reply + 2);

		/* send message */
		if (! myqtt_sequencer_send (conn, (msg->qos == MYQTT_QOS_1) ? MYQTT_PUBACK : MYQTT_PUBREC, reply, 4))
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send %s message, errno=%d", (msg->qos == MYQTT_QOS_1) ? "PUBACK" : "PUBREC", errno);
	} /* end if */

	return;
}

/** 
 * @internal PUBREL handling..
 */
void __myqtt_reader_handle_pubrel (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer _data)
{
	/* local variables */
	int                      desp = 0;
	unsigned char          * reply;

	/* request to release message and acknoledge message
	 * publication. Currently we have no mean to notify if we
	 * finished but that's is a minor problem because we have
	 * stored all messages to be sent so they will be delivered
	 * because devices are connected or because the message is
	 * queued to be sent on next connection */
		
	/**** CLIENT HANDLING ****/
	if (conn->role == MyQttRoleInitiator) {
		/* do nothing */

		return;
	} /* end if */

	/* get packet id */
	msg->packet_id   = myqtt_get_16bit (msg->payload + desp); 

	/* configure header */
	reply    = axl_new (unsigned char, 4);
	if (reply == NULL)
		return;

	reply[0] = (( 0x00000f & MYQTT_PUBCOMP) << 4);

	/* configure remaining length */
	reply[1] = 2;

	/* set packet id in reply */
	/* myqtt_log (MYQTT_LEVEL_DEBUG, "Saving packet id %d on desp=%d", packet_id, desp + 1); */
	myqtt_set_16bit (msg->packet_id, reply + 2);

	/* send message */
	if (! myqtt_sequencer_send (conn, MYQTT_PUBCOMP, reply, 4))
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send PUBCOMP message, errno=%d", errno);

	return;
}

void __myqtt_reader_handle_pingreq (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer _data)
{
	/* local variables */
	unsigned char          * reply;
	int                      size;

	/* check if this is a listener */
	if (conn->role != MyQttRoleListener) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received PINGREQ request over a connection that is not a listener from conn-id=%d from %s:%s, closing connection..", 
			   conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* myqtt_msg_free_build (ctx, reply, size); */
	reply = myqtt_msg_build (ctx, MYQTT_PINGRESP, axl_false, 0, axl_false, &size, /* 2 bytes */
				 MYQTT_PARAM_END);
	if (reply == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to allocate memory required for PINGRESP reply for conn-id=%d from %s:%s, closing connection..", 
			   conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);
		return;
	} /* end if */

	/* send message */
	if (! myqtt_sequencer_send (conn, MYQTT_PINGRESP, reply, size))
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send PINGRESP message, errno=%d", errno);

	myqtt_log (MYQTT_LEVEL_DEBUG, "Sent PINGRESP reply to conn-id=%d at %s:%s..", conn->id, conn->host, conn->port);

	/* free reply */
	/* myqtt_msg_free_build (ctx, reply, size); */

	return;
}


/** 
 * @internal
 **/
void __myqtt_reader_process_socket (MyQttCtx  * ctx, 
				    MyQttConn * conn)
{

	MyQttMsg         * msg;

	myqtt_log (MYQTT_LEVEL_DEBUG, "something to read conn-id=%d", myqtt_conn_get_id (conn));

	/* before doing anything, check if the conn is broken */
	if (! myqtt_conn_is_ok (conn, axl_false))
		return;

	/* check for unwatch requests */
	if (conn->reader_unwatch)
		return;

	/* check for preread handler */
	if (conn->preread_handler) {
		/* call pre read handler */
		conn->preread_handler (ctx, conn, conn->preread_user_data);
		return;
	} /* end if */

	/* read all msgs received from remote site */
	msg   = myqtt_msg_get_next (conn);
	if (msg == NULL) 
		return;

	/* according to message type, handle it */
	switch (msg->type) {
	case MYQTT_CONNECT:
		/* handle CONNECT packet */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_connect); 
		break;
	case MYQTT_DISCONNECT:
		/* handle DISCONNECT packet */
		__myqtt_reader_handle_disconnect (ctx, msg, conn);
		break;
	case MYQTT_SUBSCRIBE:
		/* handle SUBCRIBE packet */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_subscribe);
		break;
	case MYQTT_SUBACK:
		/* handle SUBACK packet */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_wait_reply);
		break;
	case MYQTT_UNSUBSCRIBE:
		/* handle UNSUBSCRIBE packet */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_unsubscribe);
		break;
	case MYQTT_UNSUBACK:
		/* handle UNSUBACK packet */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_wait_reply);
		break;
	case MYQTT_PUBLISH:
		/* handle PUBLISH packet */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_publish);
		break;
	case MYQTT_PUBACK:
		/* handle PUBACK packet */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_wait_reply);
		break;
	case MYQTT_PUBREC:
		/* handle PUBREC packet */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_wait_reply);
		break;
	case MYQTT_PUBREL:
		/* handle PUBREC packet */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_pubrel);
		break;
	case MYQTT_PUBCOMP:
		/* handle PUBCOMP packet */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_wait_reply);
		break;
	case MYQTT_PINGREQ:
		/* handle ping request */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_pingreq);
		break;
	case MYQTT_PINGRESP:
		/* handle ping request */
		__myqtt_reader_async_run (conn, msg, __myqtt_reader_handle_wait_reply);
		break;
	default:
		/* report unhandled packet type */
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Received unhandled message type (%d : %s) conn-id=%d from %s:%s, closing connection", 
			   msg->type, myqtt_msg_get_type_str (msg), conn->id, conn->host, conn->port);
		myqtt_conn_shutdown (conn);
		break;
	}

	/* unable to deliver the msg, free it */
	myqtt_msg_unref (msg);

	/* that's all I can do */
	return;
}

/** 
 * @internal 
 *
 * @brief Classify myqtt reader items to be managed, that is,
 * connections or listeners.
 * 
 * @param data The internal myqtt reader data to be managed.
 * 
 * @return axl_true if the item to be managed was clearly read or axl_false if
 * an error on registering the item was produced.
 */
axl_bool   myqtt_reader_register_watch (MyQttReaderData * data, axlList * con_list, axlList * srv_list)
{
	MyQttConn * connection;
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx        * ctx;
#endif

	/* get a reference to the connection (no matter if it is not
	 * defined) */
	connection = data->connection;
#if defined(ENABLE_MYQTT_LOG)
	ctx        = myqtt_conn_get_ctx (connection);
#endif

	switch (data->type) {
	case CONNECTION:
		/* check the connection */
		if (!myqtt_conn_is_ok (connection, axl_false)) {
			/* check if we can free this connection */
			myqtt_conn_unref (connection, "myqtt reader (watch)");
			myqtt_log (MYQTT_LEVEL_DEBUG, "received a non-valid connection, ignoring it");

			/* release data */
			axl_free (data);
			return axl_false;
		}
			
		/* now we have a first connection, we can start to wait */
		myqtt_log (MYQTT_LEVEL_DEBUG, "new connection (conn-id=%d) to be watched (%d)", 
			    myqtt_conn_get_id (connection), myqtt_conn_get_socket (connection));
		axl_list_append (con_list, connection);

		break;
	case LISTENER:
		myqtt_log (MYQTT_LEVEL_DEBUG, "new listener connection to be watched (socket: %d --> %s:%s, conn-id: %d)",
			    myqtt_conn_get_socket (connection), 
			    myqtt_conn_get_host (connection), 
			    myqtt_conn_get_port (connection),
			    myqtt_conn_get_id (connection));
		axl_list_append (srv_list, connection);
		break;
	case TERMINATE:
	case IO_WAIT_CHANGED:
	case IO_WAIT_READY:
	case FOREACH:
		/* just unref myqtt reader data */
		break;
	} /* end switch */
	
	axl_free (data);
	return axl_true;
}

/** 
 * @internal MyQtt function to implement myqtt reader I/O change.
 */
MyQttReaderData * __myqtt_reader_change_io_mech (MyQttCtx        * ctx,
						   axlPointer       * on_reading, 
						   axlList          * con_list, 
						   axlList          * srv_list, 
						   MyQttReaderData * data)
{
	/* get current context */
	MyQttReaderData * result;

	myqtt_log (MYQTT_LEVEL_DEBUG, "found I/O notification change");
	
	/* unref IO waiting object */
	myqtt_io_waiting_invoke_destroy_fd_group (ctx, *on_reading); 
	*on_reading = NULL;
	
	/* notify preparation done and lock until new
	 * I/O is installed */
	myqtt_log (MYQTT_LEVEL_DEBUG, "notify myqtt reader preparation done");
	myqtt_async_queue_push (ctx->reader_stopped, INT_TO_PTR(1));
	
	/* free data use the function that includes that knoledge */
	myqtt_reader_register_watch (data, con_list, srv_list);
	
	/* lock */
	myqtt_log (MYQTT_LEVEL_DEBUG, "lock until new API is installed");
	result = myqtt_async_queue_pop (ctx->reader_queue);

	/* initialize the read set */
	myqtt_log (MYQTT_LEVEL_DEBUG, "unlocked, creating new I/O mechanism used current API");
	*on_reading = myqtt_io_waiting_invoke_create_fd_group (ctx, READ_OPERATIONS);

	return result;
}


/* do a foreach operation */
void myqtt_reader_foreach_impl (MyQttCtx        * ctx, 
				 axlList          * con_list, 
				 axlList          * srv_list, 
				 MyQttReaderData * data)
{
	axlListCursor * cursor;

	myqtt_log (MYQTT_LEVEL_DEBUG, "doing myqtt reader foreach notification..");

	/* check for null function */
	if (data->func == NULL) 
		goto foreach_impl_notify;

	/* foreach the connection list */
	cursor = axl_list_cursor_new (con_list);
	while (axl_list_cursor_has_item (cursor)) {

		/* notify, if the connection is ok */
		if (myqtt_conn_is_ok (axl_list_cursor_get (cursor), axl_false)) {
			data->func (axl_list_cursor_get (cursor), data->user_data);
		} /* end if */

		/* next cursor */
		axl_list_cursor_next (cursor);
	} /* end while */
	
	/* free cursor */
	axl_list_cursor_free (cursor);

	/* foreach the connection list */
	cursor = axl_list_cursor_new (srv_list);
	while (axl_list_cursor_has_item (cursor)) {
		/* notify, if the connection is ok */
		if (myqtt_conn_is_ok (axl_list_cursor_get (cursor), axl_false)) {
			data->func (axl_list_cursor_get (cursor), data->user_data);
		} /* end if */

		/* next cursor */
		axl_list_cursor_next (cursor);
	} /* end while */

	/* free cursor */
	axl_list_cursor_free (cursor);

	/* notify that the foreach operation was completed */
 foreach_impl_notify:
	myqtt_async_queue_push (data->notify, INT_TO_PTR (1));

	return;
}

/** 
 * @internal Function used to prepare wait reply for the provided
 * connection.
 *
 * @param conn The connection where the wait reply will be prepared,
 * allowing to call then to __myqtt_reader_get_reply.
 *
 * @param packet_id The packet id we are going to wait.
 */
void        __myqtt_reader_prepare_wait_reply (MyQttConn * conn, int packet_id)
{
	MyQttCtx        * ctx;
	MyQttAsyncQueue * queue;

	if (conn == NULL)
		return;

	ctx = conn->ctx;

	/* enable wait reply */
	queue = myqtt_async_queue_new ();

	/* register wait reply method */
	myqtt_mutex_lock (&conn->op_mutex);

	/* check */
	if (axl_hash_get (conn->wait_replies, INT_TO_PTR (packet_id)))
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Enabling wait reply for a packet_id=%d that is already waiting for a reply...someone is going to miss a reply");

	/* add queue to implement the wait operation */
	axl_hash_insert (conn->wait_replies, INT_TO_PTR (packet_id), queue);

	myqtt_mutex_unlock (&conn->op_mutex);

	return;
}

/** 
 * @internal Function that allows to wait for a specific packet id
 * reply on the provided connection.
 *
 * @param conn The connection where to implement the reply wait.
 *
 * @param packet_id The packet id we are waiting for.
 *
 * @param timeout The timeout we are going to wait or 0 if it is
 * required to wait forever.
 */
MyQttMsg  * __myqtt_reader_get_reply          (MyQttConn * conn, int packet_id, int timeout)
{
	MyQttAsyncQueue * queue;
	MyQttMsg        * msg;
	MyQttCtx        * ctx;

	if (conn == NULL)
		return NULL;

	/* get wait to wait on */
	myqtt_mutex_lock (&conn->op_mutex);

	/* get queue and remove it from the set of wait replies */
	queue = axl_hash_get (conn->wait_replies, INT_TO_PTR (packet_id));

	/* release lock */
	myqtt_mutex_unlock (&conn->op_mutex);

	if (queue == NULL) {
		/* get context to log */
		ctx = conn->ctx;
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Expected to find a queue reference for packet_id=%d, conn-id=%d, but found NULL",
			   packet_id, conn->id);
		return NULL;
	} /* end if */

	/* call to wait for message */
	if (timeout > 0)
		msg = myqtt_async_queue_pop (queue);
	else
		msg = myqtt_async_queue_timedpop (queue, timeout * 1000);

	/* null reference received, try to remove the queue but first
	 * we have to acquire the reference */
	myqtt_mutex_lock (&conn->op_mutex);

	/* get the queue to release it */
	queue = axl_hash_get (conn->wait_replies, INT_TO_PTR (packet_id));
	axl_hash_delete (conn->wait_replies, INT_TO_PTR (packet_id));
	
	/* release lock */
	myqtt_mutex_unlock (&conn->op_mutex);

	/* release queue if defined */
	if (queue)
		myqtt_async_queue_unref (queue);

	return msg;
}

/** 
 * @internal
 * @brief Read the next item on the myqtt reader to be processed
 * 
 * Once an item is read, it is check if something went wrong, in such
 * case the loop keeps on going.
 * 
 * The function also checks for terminating myqtt reader loop by
 * looking for TERMINATE value into the data->type. In such case axl_false
 * is returned meaning that no further loop should be done by the
 * myqtt reader.
 *
 * @return axl_true to keep myqtt reader working, axl_false if myqtt reader
 * should stop.
 */
axl_bool      myqtt_reader_read_queue (MyQttCtx  * ctx,
					axlList    * con_list, 
					axlList    * srv_list, 
					axlPointer * on_reading)
{
	/* get current context */
	MyQttReaderData * data;
	int               should_continue = axl_true;

	do {
		data            = myqtt_async_queue_pop (ctx->reader_queue);

		/* check if we have to continue working */
		should_continue = (data->type != TERMINATE);

		myqtt_log (MYQTT_LEVEL_DEBUG, "Reading from queue, should_continue=%d..", should_continue);

		/* check if the io/wait mech have changed */
		if (data->type == IO_WAIT_CHANGED) {
			/* change io mechanism */
			data = __myqtt_reader_change_io_mech (ctx,
							       on_reading, 
							       con_list, 
							       srv_list, 
							       data);
		} else if (data->type == FOREACH) {
			/* do a foreach operation */
			myqtt_reader_foreach_impl (ctx, con_list, srv_list, data);

		} /* end if */

		if (data->type == TERMINATE) {
			axl_free (data);
		}

	}while (should_continue && !myqtt_reader_register_watch (data, con_list, srv_list));

	return should_continue;
}

/** 
 * @internal Function used by the myqtt reader main loop to check for
 * more connections to watch, to check if it has to terminate or to
 * check at run time the I/O waiting mechanism used.
 * 
 * @param con_list The set of connections already watched.
 *
 * @param srv_list The set of listener connections already watched.
 *
 * @param on_reading A reference to the I/O waiting object, in the
 * case the I/O waiting mechanism is changed.
 * 
 * @return axl_true to flag the process to continue working to to stop.
 */
axl_bool      myqtt_reader_read_pending (MyQttCtx  * ctx,
					  axlList    * con_list, 
					  axlList    * srv_list, 
					  axlPointer * on_reading)
{
	/* get current context */
	MyQttReaderData * data;
	int                length;
	axl_bool           should_continue = axl_true;

	length = myqtt_async_queue_length (ctx->reader_queue);
	while (length > 0 && should_continue) {
		length--;
		data            = myqtt_async_queue_pop (ctx->reader_queue);

		/* check if we have to continue working */
		should_continue = (data->type != TERMINATE);

		myqtt_log (MYQTT_LEVEL_DEBUG, "read pending type=%d (should_continue=%d)",
			   data->type, should_continue);

		/* check if the io/wait mech have changed */
		if (data->type == IO_WAIT_CHANGED) {
			/* change io mechanism */
			data = __myqtt_reader_change_io_mech (ctx, on_reading, con_list, srv_list, data);

		} else if (data->type == FOREACH) {
			/* do a foreach operation */
			myqtt_reader_foreach_impl (ctx, con_list, srv_list, data);

		} /* end if */

		/* watch the request received, maybe a connection or a
		 * myqtt reader command to process  */
		myqtt_reader_register_watch (data, con_list, srv_list);
		
	} /* end while */

	return should_continue;
}

void       __myqtt_reader_remove_conn_from_hash (MyQttConn * conn, axlHashCursor * cursor)
{
	axlHash       * sub_hash;
	const char    * topic_filter;
	MyQttCtx      * ctx = conn->ctx;

	myqtt_mutex_lock (&conn->op_mutex);

	/* iterate all subscriptions this connection hash */
	while (axl_hash_cursor_has_item (cursor)) {

		/* get topic filter where the connection has to be removed */
		topic_filter = axl_hash_cursor_get_key (cursor);
		if (! topic_filter) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Expected to find topic filter value but found NULL [internal engine error]");
			continue;
		} /* end if */
		
		/* now get connection hash handling that topic */
		sub_hash    = axl_hash_get (ctx->subs, (axlPointer) topic_filter);
		if (! sub_hash) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Expected to find connection hash under the topic %s but found NULL reference [internal engine error]..", 
				   topic_filter);
			continue;
		} /* end if */

		/* remove the connection from that connection hash */
		axl_hash_remove (sub_hash, conn);

		/* get next */
		axl_hash_cursor_next (cursor);
	} /* end while */

	myqtt_mutex_unlock (&conn->op_mutex);

	return;
}

/* publish will if defined */
void __myqtt_reader_check_and_trigger_will (MyQttCtx * ctx, MyQttConn * conn)
{
	MyQttMsg * msg;

	/* do not trigger will if it is not defined if it is a
	 * initiator */
	if (ctx->myqtt_exit)
		return;
	if (conn->role == MyQttRoleInitiator)
		return;
	if (conn->will_topic == NULL)
		return;

	/* prepare message */
	msg = axl_new (MyQttMsg, 1);
	if (msg == NULL)
		return;
	
	/* configure message */
	msg->type      = MYQTT_PUBLISH;
	msg->qos       = conn->will_qos;
	msg->ref_count = 1;
	msg->id        = __myqtt_msg_get_next_id (ctx, "get-next");
	msg->ctx       = ctx;

	/* acquire a reference to the context */
	myqtt_ctx_ref2 (ctx, "new msg");

	/* setup app messages and topic */
	msg->app_message      = (unsigned char *) axl_strdup (conn->will_msg);
	msg->app_message_size = conn->will_msg ? strlen (conn->will_msg) : 0;
	msg->size             = msg->app_message_size;
	msg->payload          = msg->app_message;

	/* configure topic */
	msg->topic_name       = axl_strdup (conn->will_topic);

	/* call to publish */
	__myqtt_reader_do_publish (ctx, conn, msg);

	/* release message */
	myqtt_msg_unref (msg);	

	return;
}

axlPointer __myqtt_reader_remove_conn_refs_aux (axlPointer _conn) 
{
	MyQttConn     * conn = _conn;
	MyQttCtx      * ctx;
	axlHashCursor * cursor = NULL;

	if (_conn == NULL)
		return NULL;

	/* get context reference */
	ctx = conn->ctx;

	if (axl_hash_items (conn->subs) > 0) {
		/* create cursor */
		cursor = axl_hash_cursor_new (conn->subs);
		axl_hash_cursor_first (cursor);

		/* lock subscribtions to remove all connection references */
		myqtt_mutex_lock (&ctx->subs_m);

		/* check no publish operation is taking place now */
		while (ctx->publish_ops > 0)
			myqtt_cond_timedwait (&ctx->subs_c, &ctx->subs_m, 10000);

		/* call to remove connection from this hash */
		__myqtt_reader_remove_conn_from_hash (conn, cursor);

		/* release lock */
		myqtt_mutex_unlock (&ctx->subs_m);
		axl_hash_cursor_free (cursor);
	} /* end if */

	if (axl_hash_items (conn->wild_subs) > 0) {
		cursor = axl_hash_cursor_new (conn->wild_subs);
		axl_hash_cursor_first (cursor);

		/* lock subscribtions to remove all connection references */
		myqtt_mutex_lock (&ctx->subs_m);

		/* check no publish operation is taking place now */
		while (ctx->publish_ops > 0)
			myqtt_cond_timedwait (&ctx->subs_c, &ctx->subs_m, 10000);

		/* call to remove connection from this hash */
		__myqtt_reader_remove_conn_from_hash (conn, cursor);

		/* release lock */
		myqtt_mutex_unlock (&ctx->subs_m);
		axl_hash_cursor_free (cursor);
	} /* end if */

	/* call to publish will if it applies */
	__myqtt_reader_check_and_trigger_will (ctx, conn);

	/* connection isn't ok, unref it */
	myqtt_conn_unref (conn, "myqtt reader (build set)");

	return NULL;
}

/* call to remove all connection references */
void __myqtt_reader_remove_conn_refs (MyQttConn * conn)
{
	MyQttCtx * ctx = conn->ctx;
	
	/* remove client id from global table */
	myqtt_mutex_lock (&ctx->client_ids_m);
	axl_hash_delete (ctx->client_ids, conn->client_identifier); 
	myqtt_mutex_unlock (&ctx->client_ids_m);

	/* skip any operation if we are about to finish */
	if (ctx->myqtt_exit) {
		/* connection isn't ok, unref it */
		myqtt_conn_unref (conn, "myqtt reader (build set)");
		return;
	} /* end if */

	/* call to run next task */
	/* if (! myqtt_thread_pool_new_task (conn->ctx, __myqtt_reader_remove_conn_refs_aux, conn)) { */
		/* unable to queue message into the thread pool, ok, run it directly */ 
	__myqtt_reader_remove_conn_refs_aux (conn);
	/* } */

	return;
}


/** 
 * @internal Auxiliar function that populates the reading set of file
 * descriptors (on_reading), returning the max fds.
 */
MYQTT_SOCKET __myqtt_reader_build_set_to_watch_aux (MyQttCtx     * ctx,
						      axlPointer      on_reading, 
						      axlListCursor * cursor, 
						      MYQTT_SOCKET   current_max)
{
	MYQTT_SOCKET      max_fds     = current_max;
	MYQTT_SOCKET      fds         = 0;
	MyQttConn * connection;
	long               time_stamp  = 0;

	/* get current time stamp if idle handler is defined */
	if (ctx->global_idle_handler)
		time_stamp = (long) time (NULL);
	
	axl_list_cursor_first (cursor);
	while (axl_list_cursor_has_item (cursor)) {

		/* get current connection */
		connection = axl_list_cursor_get (cursor);

		/* check for idle status */
		if (ctx->global_idle_handler)
			myqtt_conn_check_idle_status (connection, ctx, time_stamp);

		/* check ok status */
		if (! myqtt_conn_is_ok (connection, axl_false)) {

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* call to remove all connection references */
			__myqtt_reader_remove_conn_refs (connection);

			continue;
		} /* end if */

		/* check if the connection must be unwatched */
		if (connection->reader_unwatch) {
			/* remove the unwatch flag from the connection */
			connection->reader_unwatch = axl_false;

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* call to remove all connection references */
			__myqtt_reader_remove_conn_refs (connection);

			continue;
		} /* end if */

		/* check if the connection is blocked (no I/O read to
		 * perform on it) */
		if (myqtt_conn_is_blocked (connection)) {
			/* myqtt_log (MYQTT_LEVEL_DEBUG, "connection id=%d has I/O read blocked (myqtt_conn_block)", 
			   myqtt_conn_get_id (connection)); */
			/* get the next */
			axl_list_cursor_next (cursor);
			continue;
		} /* end if */

		/* get the socket to ge added and get its maximum
		 * value */
		fds        = myqtt_conn_get_socket (connection);
		max_fds    = fds > max_fds ? fds: max_fds;

		/* add the socket descriptor into the given on reading
		 * group */
		if (! myqtt_io_waiting_invoke_add_to_fd_group (ctx, fds, connection, on_reading)) {
			
			myqtt_log (MYQTT_LEVEL_WARNING, 
				    "unable to add the connection to the myqtt reader watching set. This could mean you did reach the I/O waiting mechanism limit.");

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* set it as not connected */
			if (myqtt_conn_is_ok (connection, axl_false))
				__myqtt_conn_shutdown_and_record_error (connection, MyQttError, "myqtt reader (add fail)");

			/* call to remove all connection references */
			__myqtt_reader_remove_conn_refs (connection);

			continue;
		} /* end if */

		/* get the next */
		axl_list_cursor_next (cursor);

	} /* end while */

	/* return maximum number for file descriptors */
	return max_fds;
	
} /* end __myqtt_reader_build_set_to_watch_aux */

MYQTT_SOCKET   __myqtt_reader_build_set_to_watch (MyQttCtx     * ctx,
						    axlPointer      on_reading, 
						    axlListCursor * conn_cursor, 
						    axlListCursor * srv_cursor)
{

	MYQTT_SOCKET       max_fds     = 0;

	/* read server connections */
	max_fds = __myqtt_reader_build_set_to_watch_aux (ctx, on_reading, srv_cursor, max_fds);

	/* read client connection list */
	max_fds = __myqtt_reader_build_set_to_watch_aux (ctx, on_reading, conn_cursor, max_fds);

	/* return maximum number for file descriptors */
	return max_fds;
	
}

void __myqtt_reader_check_connection_list (MyQttCtx     * ctx,
					    axlPointer      on_reading, 
					    axlListCursor * conn_cursor, 
					    int             changed)
{

	MYQTT_SOCKET       fds        = 0;
	MyQttConn  * connection = NULL;
	int                 checked    = 0;

	/* check all connections */
	axl_list_cursor_first (conn_cursor);
	while (axl_list_cursor_has_item (conn_cursor)) {

		/* check changed */
		if (changed == checked)
			return;

		/* check if we have to keep on listening on this
		 * connection */
		connection = axl_list_cursor_get (conn_cursor);
		if (!myqtt_conn_is_ok (connection, axl_false)) {
			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (conn_cursor);

			/* call to remove all connection references */
			__myqtt_reader_remove_conn_refs (connection);
			continue;
		}
		
		/* get the connection and socket. */
	        fds = myqtt_conn_get_socket (connection);
		
		/* ask if this socket have changed */
		if (myqtt_io_waiting_invoke_is_set_fd_group (ctx, fds, on_reading, ctx)) {

			/* call to process incoming data, activating
			 * all invocation code (first and second level
			 * handler) */
			__myqtt_reader_process_socket (ctx, connection);

			/* update number of sockets checked */
			checked++;
		}

		/* get the next */
		axl_list_cursor_next (conn_cursor);

	} /* end for */

	return;
}

int  __myqtt_reader_check_listener_list (MyQttCtx     * ctx, 
					  axlPointer      on_reading, 
					  axlListCursor * srv_cursor, 
					  int             changed)
{

	int                fds      = 0;
	int                checked  = 0;
	MyQttConn * connection;

	/* check all listeners */
	axl_list_cursor_first (srv_cursor);
	while (axl_list_cursor_has_item (srv_cursor)) {

		/* get the connection */
		connection = axl_list_cursor_get (srv_cursor);

		if (!myqtt_conn_is_ok (connection, axl_false)) {
			myqtt_log (MYQTT_LEVEL_DEBUG, "myqtt reader found listener id=%d not operational, unreference",
				    myqtt_conn_get_id (connection));

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (srv_cursor);

			/* call to remove all connection references */
			__myqtt_reader_remove_conn_refs (connection);

			/* update checked connections */
			checked++;

			continue;
		} /* end if */
		
		/* get the connection and socket. */
		fds  = myqtt_conn_get_socket (connection);

		/* check if the socket is activated */
		if (myqtt_io_waiting_invoke_is_set_fd_group (ctx, fds, on_reading, ctx)) {
			/* init the listener incoming connection phase */
			myqtt_log (MYQTT_LEVEL_DEBUG, "listener (%d) have requests, processing..", fds);
			myqtt_listener_accept_connections (ctx, fds, connection);

			/* update checked connections */
			checked++;
		} /* end if */

		/* check to stop listener */
		if (checked == changed)
			return 0;

		/* get the next */
		axl_list_cursor_next (srv_cursor);
	}
	
	/* return remaining sockets active */
	return changed - checked;
}

/** 
 * @internal
 *
 * @brief Internal function called to stop myqtt reader and cleanup
 * memory used.
 * 
 */
void __myqtt_reader_stop_process (MyQttCtx     * ctx,
				   axlPointer      on_reading, 
				   axlListCursor * conn_cursor, 
				   axlListCursor * srv_cursor)

{
	/* stop myqtt reader process unreferring already managed
	 * connections */
	myqtt_log (MYQTT_LEVEL_DEBUG, "Stopping reading..");
	
	myqtt_async_queue_unref (ctx->reader_queue);

	/* unref listener connections */
	myqtt_log (MYQTT_LEVEL_DEBUG, "cleaning pending %d listener connections..", axl_list_length (ctx->srv_list));
	ctx->srv_list = NULL;
	axl_list_free (axl_list_cursor_list (srv_cursor));
	axl_list_cursor_free (srv_cursor);

	/* unref initiators connections */
	myqtt_log (MYQTT_LEVEL_DEBUG, "cleaning pending %d peer connections..", axl_list_length (ctx->conn_list));
	ctx->conn_list = NULL;
	axl_list_free (axl_list_cursor_list (conn_cursor));
	axl_list_cursor_free (conn_cursor);

	/* unref IO waiting object */
	myqtt_io_waiting_invoke_destroy_fd_group (ctx, on_reading); 

	/* signal that the myqtt reader process is stopped */
	QUEUE_PUSH (ctx->reader_stopped, INT_TO_PTR (1));

	return;
}

void __myqtt_reader_close_connection (axlPointer pointer)
{
	MyQttConn * conn = pointer;

	/* unref the connection */
	myqtt_conn_shutdown (conn);
	myqtt_conn_unref (conn, "myqtt reader");

	return;
}

/** 
 * @internal Dispatch function used to process all sockets that have
 * changed.
 * 
 * @param fds The socket that have changed.
 * @param wait_to The purpose that was configured for the file set.
 * @param connection The connection that is notified for changes.
 */
void __myqtt_reader_dispatch_connection (int                  fds,
					  MyQttIoWaitingFor   wait_to,
					  MyQttConn   * connection,
					  axlPointer           user_data)
{
	/* cast the reference */
	MyQttCtx * ctx = user_data;

	switch (myqtt_conn_get_role (connection)) {
	case MyQttRoleMasterListener:
		/* listener connections */
		myqtt_listener_accept_connections (ctx, fds, connection);
		break;
	default:
		/* call to process incoming data, activating all
		 * invocation code (first and second level handler) */
		__myqtt_reader_process_socket (ctx, connection);
		break;
	} /* end if */
	return;
}

axl_bool __myqtt_reader_detect_and_cleanup_connection (axlListCursor * cursor) 
{
	MyQttConn * conn;
	char               bytes[3];
	int                result;
	int                fds;
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx        * ctx;
#endif
	
	/* get connection from cursor */
	conn = axl_list_cursor_get (cursor);
	if (myqtt_conn_is_ok (conn, axl_false)) {

		/* get the connection and socket. */
		fds    = myqtt_conn_get_socket (conn);
#if defined(AXL_OS_UNIX)
		errno  = 0;
#endif
		result = recv (fds, bytes, 1, MSG_PEEK);
		if (result == -1 && errno == EBADF) {
			  
			/* get context */
#if defined(ENABLE_MYQTT_LOG)
			ctx = CONN_CTX (conn);
#endif
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Found connection-id=%d, with session=%d not working (errno=%d), shutting down",
				    myqtt_conn_get_id (conn), fds, errno);
			/* close connection, but remove the socket reference to avoid closing some's socket */
			conn->session = -1;
			myqtt_conn_shutdown (conn);
			
			/* connection isn't ok, unref it */
			myqtt_conn_unref (conn, "myqtt reader (process), wrong socket");
			axl_list_cursor_unlink (cursor);
			return axl_false;
		} /* end if */
	} /* end if */
	
	return axl_true;
}

void __myqtt_reader_detect_and_cleanup_connections (MyQttCtx * ctx)
{
	/* check all listeners */
	axl_list_cursor_first (ctx->conn_cursor);
	while (axl_list_cursor_has_item (ctx->conn_cursor)) {

		/* get the connection */
		if (! __myqtt_reader_detect_and_cleanup_connection (ctx->conn_cursor))
			continue;

		/* get the next */
		axl_list_cursor_next (ctx->conn_cursor);
	} /* end while */

	/* check all listeners */
	axl_list_cursor_first (ctx->srv_cursor);
	while (axl_list_cursor_has_item (ctx->srv_cursor)) {

	  /* get the connection */
	  if (! __myqtt_reader_detect_and_cleanup_connection (ctx->srv_cursor))
		   continue; 

	    /* get the next */
	    axl_list_cursor_next (ctx->srv_cursor); 
	} /* end while */

	/* clear errno after cleaning descriptors */
#if defined(AXL_OS_UNIX)
	errno = 0;
#endif

	return; 
}

axlPointer __myqtt_reader_run (MyQttCtx * ctx)
{
	MYQTT_SOCKET      max_fds     = 0;
	MYQTT_SOCKET      result;
	int                error_tries = 0;

	/* initialize the read set */
	if (ctx->on_reading != NULL)
		myqtt_io_waiting_invoke_destroy_fd_group (ctx, ctx->on_reading);
	ctx->on_reading  = myqtt_io_waiting_invoke_create_fd_group (ctx, READ_OPERATIONS);

	/* create lists */
	ctx->conn_list = axl_list_new (axl_list_always_return_1, __myqtt_reader_close_connection);
	ctx->srv_list = axl_list_new (axl_list_always_return_1, __myqtt_reader_close_connection);

	/* create cursors */
	ctx->conn_cursor = axl_list_cursor_new (ctx->conn_list);
	ctx->srv_cursor = axl_list_cursor_new (ctx->srv_list);

	/* first step. Waiting blocked for our first connection to
	 * listen */
 __myqtt_reader_run_first_connection:
	if (!myqtt_reader_read_queue (ctx, ctx->conn_list, ctx->srv_list, &(ctx->on_reading))) {
		/* seems that the myqtt reader main loop should
		 * stop */
		__myqtt_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
		return NULL;
	}

	while (axl_true) {
		/* reset descriptor set */
		myqtt_io_waiting_invoke_clear_fd_group (ctx, ctx->on_reading);

		if ((axl_list_length (ctx->conn_list) == 0) && (axl_list_length (ctx->srv_list) == 0)) {
			/* check if we have to terminate the process
			 * in the case no more connections are
			 * available: useful when the current instance
			 * is running in the context of turbulence */
			myqtt_ctx_check_on_finish (ctx);

			myqtt_log (MYQTT_LEVEL_DEBUG, "no more connection to watch for, putting thread to sleep");
			goto __myqtt_reader_run_first_connection;
		}

		/* build socket descriptor to be read */
		max_fds = __myqtt_reader_build_set_to_watch (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
		if (errno == EBADF) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Found wrong file descriptor error...(max_fds=%d, errno=%d), cleaning", max_fds, errno);
			/* detect and cleanup wrong connections */
			__myqtt_reader_detect_and_cleanup_connections (ctx);
			continue;
		} /* end if */
		
		/* perform IO blocking wait for read operation */
		result = myqtt_io_waiting_invoke_wait (ctx, ctx->on_reading, max_fds, READ_OPERATIONS);

		/* do automatic thread pool resize here */
		__myqtt_thread_pool_automatic_resize (ctx);  

		/* check for timeout error */
		if (result == -1 || result == -2)
			goto process_pending;

		/* check errors */
		if ((result < 0) && (errno != 0)) {

			error_tries++;
			if (error_tries == 2) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, 
					    "tries have been reached on reader, error was=(errno=%d): %s exiting..",
					    errno, myqtt_errno_get_last_error ());
				return NULL;
			} /* end if */
			continue;
		} /* end if */

		/* check for fatal error */
		if (result == -3) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "fatal error received from io-wait function, exiting from myqtt reader process..");
			__myqtt_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
			return NULL;
		}


		/* check for each listener */
		if (result > 0) {
			/* check if the mechanism have automatic
			 * dispatch */
			if (myqtt_io_waiting_invoke_have_dispatch (ctx, ctx->on_reading)) {
				/* perform automatic dispatch,
				 * providing the dispatch function and
				 * the number of sockets changed */
				myqtt_io_waiting_invoke_dispatch (ctx, ctx->on_reading, __myqtt_reader_dispatch_connection, result, ctx);

			} else {
				/* call to check listener connections */
				result = __myqtt_reader_check_listener_list (ctx, ctx->on_reading, ctx->srv_cursor, result);
			
				/* check for each connection to be watch is it have check */
				__myqtt_reader_check_connection_list (ctx, ctx->on_reading, ctx->conn_cursor, result);
			} /* end if */
		}

		/* we have finished the connection dispatching, so
		 * read the pending queue elements to be watched */
		
		/* reset error tries */
	process_pending:
		error_tries = 0;

		/* read new connections to be managed */
		if (! myqtt_reader_read_pending (ctx, ctx->conn_list, ctx->srv_list, &(ctx->on_reading))) {
			myqtt_log (MYQTT_LEVEL_DEBUG, "Calling to stop process..");
			__myqtt_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
			return NULL;
		}
	}
	return NULL;
}

/** 
 * @brief Function that returns the number of connections that are
 * currently watched by the reader.
 * @param ctx The context where the reader loop is located.
 * @return Number of connections watched. 
 */
int  myqtt_reader_connections_watched         (MyQttCtx        * ctx)
{
	if (ctx == NULL || ctx->conn_list == NULL || ctx->srv_list == NULL)
		return 0;
	
	/* return list */
	return axl_list_length (ctx->conn_list) + axl_list_length (ctx->srv_list);
}

/** 
 * @internal Function used to unwatch the provided connection from the
 * myqtt reader loop.
 *
 * @param ctx The context where the unread operation will take place.
 * @param connection The connection where to be unwatched from the reader...
 */
void myqtt_reader_unwatch_connection          (MyQttCtx        * ctx,
						MyQttConn * connection)
{
	v_return_if_fail (ctx && connection);
	/* flag connection myqtt reader unwatch */
	connection->reader_unwatch = axl_true;
	return;
}

/** 
 * @internal
 * 
 * Adds a new connection to be watched on myqtt reader process. This
 * function is for internal myqtt library use.
 **/
void myqtt_reader_watch_connection (MyQttCtx        * ctx,
				     MyQttConn * connection)
{
	/* get current context */
	MyQttReaderData * data;

	v_return_if_fail (myqtt_conn_is_ok (connection, axl_false));
	v_return_if_fail (ctx->reader_queue);

	if (!myqtt_conn_set_nonblocking_socket (connection)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to set non-blocking I/O operation, at connection registration, closing session");
 		return;
	}

	/* increase reference counting */
	if (! myqtt_conn_ref (connection, "myqtt reader (watch)")) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to increase connection reference count, dropping connection");
		return;
	}

	myqtt_log (MYQTT_LEVEL_DEBUG, "Accepting conn-id=%d into reader queue %p, library status: %d", 
		    myqtt_conn_get_id (connection),
		    ctx->reader_queue,
		    myqtt_is_exiting (ctx));

	/* prepare data to be queued */
	data             = axl_new (MyQttReaderData, 1);
	data->type       = CONNECTION;
	data->connection = connection;

	/* push data */
	QUEUE_PUSH (ctx->reader_queue, data);

	return;
}

/** 
 * @internal
 *
 * Install a new listener to watch for new incoming connections.
 **/
void myqtt_reader_watch_listener   (MyQttCtx        * ctx,
				     MyQttConn * listener)
{
	/* get current context */
	MyQttReaderData * data;
	v_return_if_fail (listener > 0);
	
	/* prepare data to be queued */
	data             = axl_new (MyQttReaderData, 1);
	data->type       = LISTENER;
	data->connection = listener;

	/* push data */
	QUEUE_PUSH (ctx->reader_queue, data);

	return;
}

/** 
 * @internal Function used to configure the connection so the next
 * call to terminate the list of connections will not close the
 * connection socket.
 */
axl_bool __myqtt_reader_configure_conn (axlPointer ptr, axlPointer data)
{
	/* set the connection socket to be not closed */
	myqtt_conn_set_close_socket ((MyQttConn *) ptr, axl_false);
	return axl_false; /* not found so all items are iterated */
}

/** 
 * @internal
 * 
 * @return The function returns axl_true if the myqtt reader was started
 * properly, otherwise axl_false is returned.
 **/
axl_bool  myqtt_reader_run (MyQttCtx * ctx) 
{
	v_return_val_if_fail (ctx, axl_false);

	/* check connection list to be previously created to terminate
	   it without closing sockets associated to each connection */
	if (ctx->conn_list != NULL) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "releasing previous client connections, installed: %d",
			    axl_list_length (ctx->conn_list));
		ctx->reader_cleanup = axl_true;
		axl_list_lookup (ctx->conn_list, __myqtt_reader_configure_conn, NULL);
		axl_list_cursor_free (ctx->conn_cursor);
		axl_list_free (ctx->conn_list);
		ctx->conn_list   = NULL;
		ctx->conn_cursor = NULL;
	} /* end if */
	if (ctx->srv_list != NULL) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "releasing previous listener connections, installed: %d",
			    axl_list_length (ctx->srv_list));
		ctx->reader_cleanup = axl_true;
		axl_list_lookup (ctx->srv_list, __myqtt_reader_configure_conn, NULL);
		axl_list_cursor_free (ctx->srv_cursor);
		axl_list_free (ctx->srv_list);
		ctx->srv_list   = NULL;
		ctx->srv_cursor = NULL;
	} /* end if */

	/* clear reader cleanup flag */
	ctx->reader_cleanup = axl_false;

	/* reader_queue */
	if (ctx->reader_queue != NULL)
		myqtt_async_queue_release (ctx->reader_queue);
	ctx->reader_queue   = myqtt_async_queue_new ();

	/* reader stopped */
	if (ctx->reader_stopped != NULL) 
		myqtt_async_queue_release (ctx->reader_stopped);
	ctx->reader_stopped = myqtt_async_queue_new ();

	/* create the myqtt reader main thread */
	if (! myqtt_thread_create (&ctx->reader_thread, 
				    (MyQttThreadFunc) __myqtt_reader_run,
				    ctx,
				    MYQTT_THREAD_CONF_END)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to start myqtt reader loop");
		return axl_false;
	} /* end if */
	
	return axl_true;
}

/** 
 * @internal
 * @brief Cleanup myqtt reader process.
 */
void myqtt_reader_stop (MyQttCtx * ctx)
{
	/* get current context */
	MyQttReaderData * data;

	myqtt_log (MYQTT_LEVEL_DEBUG, "stopping myqtt reader ..");

	/* create a bacon to signal myqtt reader that it should stop
	 * and unref resources */
	data       = axl_new (MyQttReaderData, 1);
	data->type = TERMINATE;

	/* push data */
	myqtt_log (MYQTT_LEVEL_DEBUG, "pushing data stop signal..");
	QUEUE_PUSH (ctx->reader_queue, data);
	myqtt_log (MYQTT_LEVEL_DEBUG, "signal sent reader ..");

	/* waiting until the reader is stoped */
	myqtt_log (MYQTT_LEVEL_DEBUG, "waiting myqtt reader 60 seconds to stop");
	if (PTR_TO_INT (myqtt_async_queue_timedpop (ctx->reader_stopped, 60000000))) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "myqtt reader properly stopped, cleaning thread..");
		/* terminate thread */
		myqtt_thread_destroy (&ctx->reader_thread, axl_false);

		/* clear queue */
		myqtt_async_queue_unref (ctx->reader_stopped);
	} else {
		myqtt_log (MYQTT_LEVEL_WARNING, "timeout while waiting myqtt reader thread to stop..");
	}

	return;
}

/** 
 * @internal Allows to check notify myqtt reader to stop its
 * processing and to change its I/O processing model. 
 * 
 * @return The function returns axl_true to notfy that the reader was
 * notified and axl_false if not. In the later case it means that the
 * reader is not running.
 */
axl_bool  myqtt_reader_notify_change_io_api               (MyQttCtx * ctx)
{
	MyQttReaderData * data;

	/* check if the myqtt reader is running */
	if (ctx == NULL || ctx->reader_queue == NULL)
		return axl_false;

	myqtt_log (MYQTT_LEVEL_DEBUG, "stopping myqtt reader due to a request for a I/O notify change...");

	/* create a bacon to signal myqtt reader that it should stop
	 * and unref resources */
	data       = axl_new (MyQttReaderData, 1);
	data->type = IO_WAIT_CHANGED;

	/* push data */
	myqtt_log (MYQTT_LEVEL_DEBUG, "pushing signal to notify I/O change..");
	QUEUE_PUSH (ctx->reader_queue, data);

	/* waiting until the reader is stoped */
	myqtt_async_queue_pop (ctx->reader_stopped);

	myqtt_log (MYQTT_LEVEL_DEBUG, "done, now myqtt reader will wait until the new API is installed..");

	return axl_true;
}

/** 
 * @internal Allows to notify myqtt reader to continue with its
 * normal processing because the new I/O api have been installed.
 */
void myqtt_reader_notify_change_done_io_api   (MyQttCtx * ctx)
{
	MyQttReaderData * data;

	/* create a bacon to signal myqtt reader that it should stop
	 * and unref resources */
	data       = axl_new (MyQttReaderData, 1);
	data->type = IO_WAIT_READY;

	/* push data */
	myqtt_log (MYQTT_LEVEL_DEBUG, "pushing signal to notify I/O is ready..");
	QUEUE_PUSH (ctx->reader_queue, data);

	myqtt_log (MYQTT_LEVEL_DEBUG, "notification done..");

	return;
}

/** 
 * @internal Function that allows to preform a foreach operation over
 * all connections handled by the myqtt reader.
 * 
 * @param ctx The context where the operation will be implemented.
 *
 * @param func The function to execute on each connection. If the
 * function provided is NULL the call will produce to lock until the
 * reader tend the foreach, restarting the reader loop.
 *
 * @param user_data User data to be provided to the function.
 *
 * @return The function returns a reference to the queue that will be
 * used to notify the foreach operation finished.
 */
MyQttAsyncQueue * myqtt_reader_foreach                     (MyQttCtx            * ctx,
							      MyQttForeachFunc      func,
							      axlPointer             user_data)
{
	MyQttReaderData * data;
	MyQttAsyncQueue * queue;

	v_return_val_if_fail (ctx, NULL);

	/* queue an operation */
	data            = axl_new (MyQttReaderData, 1);
	data->type      = FOREACH;
	data->func      = func;
	data->user_data = user_data;
	queue           = myqtt_async_queue_new ();
	data->notify    = queue;
	
	/* queue the operation */
	myqtt_log (MYQTT_LEVEL_DEBUG, "notify foreach reader operation..");
	QUEUE_PUSH (ctx->reader_queue, data);

	/* notification done */
	myqtt_log (MYQTT_LEVEL_DEBUG, "finished foreach reader operation..");

	/* return a reference */
	return queue;
}

/** 
 * @internal Iterate over all connections currently stored on the
 * provided context associated to the myqtt reader. This function is
 * only usable when the context is stopped.
 */
void               myqtt_reader_foreach_offline (MyQttCtx           * ctx,
						  MyQttForeachFunc3    func,
						  axlPointer            user_data,
						  axlPointer            user_data2,
						  axlPointer            user_data3)
{
	/* first iterate over all client connextions */
	axl_list_cursor_first (ctx->conn_cursor);
	while (axl_list_cursor_has_item (ctx->conn_cursor)) {

		/* notify connection */
		func (axl_list_cursor_get (ctx->conn_cursor), user_data, user_data2, user_data3);

		/* next item */
		axl_list_cursor_next (ctx->conn_cursor);
	} /* end while */

	/* now iterate over all server connections */
	axl_list_cursor_first (ctx->srv_cursor);
	while (axl_list_cursor_has_item (ctx->srv_cursor)) {

		/* notify connection */
		func (axl_list_cursor_get (ctx->srv_cursor), user_data, user_data2, user_data3);

		/* next item */
		axl_list_cursor_next (ctx->srv_cursor);
	} /* end while */

	return;
}




/** 
 * @internal Allows to restart the myqtt reader module, locking the
 * caller until the reader restart its loop.
 */
void myqtt_reader_restart (MyQttCtx * ctx)
{
	MyQttAsyncQueue * queue;

	/* call to restart */
	queue = myqtt_reader_foreach (ctx, NULL, NULL);
	myqtt_async_queue_pop (queue);
	myqtt_async_queue_unref (queue);
	return;
}

/* @} */
