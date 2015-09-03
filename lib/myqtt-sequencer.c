/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2015 Advanced Software Production Line, S.L.
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

/* local include */
#include <myqtt-ctx-private.h>
#include <myqtt-conn-private.h>

#define LOG_DOMAIN "myqtt-sequencer"

axl_bool myqtt_sequencer_queue_data (MyQttCtx * ctx, MyQttSequencerData * data)
{
	v_return_val_if_fail (data, axl_false);

	/* check state before handling this message with the sequencer */
	if (ctx->myqtt_exit) {
		/* axl_free (data->message); */
		myqtt_msg_free_build (ctx, data->message, data->message_size);
		axl_free (data);
		myqtt_log (MYQTT_LEVEL_WARNING, "Not queueing data because this myqtt instance is finishing..");
		return axl_false;
	}

	/* acquire reference to the connection */
	if (! myqtt_conn_ref (data->conn, "sequencer")) {
		myqtt_log (MYQTT_LEVEL_WARNING, "Not queueing data because connection reference conn-id=%d (%p) isn't working, myqtt_conn_ref() failed..",
			   data->conn->id, data->conn);
		/* axl_free (data->message); */
		myqtt_msg_free_build (ctx, data->message, data->message_size);
		axl_free (data);
		return axl_false;
	} /* end if */

	/* increase pending messages to be sent : this is to help and
	 * ensure myqtt_conn_close flushes all messages before closing
	 * the connection */
	myqtt_mutex_lock (&data->conn->op_mutex);
	data->conn->sequencer_messages++;
	myqtt_mutex_unlock (&data->conn->op_mutex);

	/* lock connection and queue message */
	myqtt_mutex_lock (&ctx->pending_messages_m);

	/* queue message */
	axl_list_append (ctx->pending_messages, data);

	myqtt_mutex_unlock (&ctx->pending_messages_m);

	/* signal sequencer to move on! */
	myqtt_cond_signal (&ctx->pending_messages_c);


	return axl_true;
}

/** 
 * @internal Function to send content in an async manner, handled by
 * the MyQtt Sequencer. Provided reference (msg) is now owned by
 * myqtt_sequencer_send: DO NOT RELEASE IT EVEN WITH A FAILURE OF THIS
 * FUNCTION. This is already handled by this function (releasing msg
 * at the right point).
 */
axl_bool myqtt_sequencer_send                     (MyQttConn            * conn, 
						   MyQttMsgType           type,
						   unsigned char        * msg, 
						   int                    msg_size)
{
	MyQttSequencerData * data;
	MyQttCtx           * ctx;

	if (conn == NULL || msg == NULL || msg_size <= 0) {
		/* free build */
		if (conn)
			myqtt_msg_free_build (conn->ctx, msg, msg_size);
		else
			axl_free (msg);

		return axl_false;
	} /* end if */

	/* acquire reference to the context */
	ctx = conn->ctx;

	/* queue package to be sent */
	data = axl_new (MyQttSequencerData, 1);
	if (data == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to acquire memory to send message");
		myqtt_msg_free_build (ctx, msg, msg_size);
		return axl_false;
	} /* end if */

	/* configure package to send */
	data->conn         = conn;
	data->message      = msg;
	data->message_size = msg_size;
	data->type         = type;

	if (! myqtt_sequencer_queue_data (ctx, data)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to queue data for delivery, failed to send message, myqtt_sequencer_queue_data() failed");
		/* IMPORTANT NOTE: do not release msg here because
		   this is already handled by
		   myqtt_sequencer_queue_data */
		return axl_false;
	} /* end if */

	/* data queued without problems */
	return axl_true;
}

axlPointer __myqtt_sequencer_run (axlPointer _data)
{

	/* get current context */
	MyQttCtx             * ctx = _data;
	axlListCursor        * cursor;

	/* references to local state */
	MyQttSequencerData   * data;
	MyQttConn            * conn;
	int                    size;

	/* get a cursor */
	cursor = axl_list_cursor_new (ctx->pending_messages);

	/* lock mutex to handle pending messages */
	myqtt_mutex_lock (&ctx->pending_messages_m);

	while (axl_true) {
		/* block until receive a new message to be sent (but
		 * only if there are no ready events) */
		myqtt_log (MYQTT_LEVEL_DEBUG, "sequencer locking (pending messages: %d, exit: %d)",
			   axl_list_length (ctx->pending_messages), ctx->myqtt_exit);
		while ((axl_list_length (ctx->pending_messages) == 0) && (! ctx->myqtt_exit )) {
			myqtt_cond_timedwait (&ctx->pending_messages_c, &ctx->pending_messages_m, 10000);
		} /* end if */

		myqtt_log (MYQTT_LEVEL_DEBUG, "sequencer unlocked (pending messages: %d, exit: %d)",
			   axl_list_length (ctx->pending_messages), ctx->myqtt_exit);

		/* check if it was requested to stop the myqtt
		 * sequencer operation */
		if (ctx->myqtt_exit) {

			/* release unlock now we are finishing */
			myqtt_mutex_unlock (&ctx->pending_messages_m);

			/* release cursor */
			axl_list_cursor_free (cursor);
			
			myqtt_log (MYQTT_LEVEL_DEBUG, "exiting myqtt sequencer thread ..");

			/* release reference acquired here */
			myqtt_ctx_unref (&ctx);

			return NULL;
		} /* end if */

		/* process all ready connections */
		axl_list_cursor_first (cursor);
		while (axl_list_cursor_has_item (cursor)) {

			/* get data */
			data    = axl_list_cursor_get (cursor);
			conn    = data->conn;

			/* check connection is working */
			if (! myqtt_conn_is_ok (conn, axl_false)) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send MQTT %s message, connection is not working, closing (conn=%p, conn-id=%d, size=%d)",
					   myqtt_msg_get_type_str2 (data->type), conn, conn->id, data->message_size);
				goto release_message;
			} /* end if */

			/* write step */
			if ((data->message_size - data->step) > 4096)
				size = 4096;
			else
				size = data->message_size - data-> step;

			if (! myqtt_msg_send_raw (conn, data->message + data->step, size)) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send MQTT message (type: %d, size: %d (total: %d), step: %d) error was errno=%d",  
					   data->type, size, data->message_size, data->step, errno); 
				goto release_message;
			} /* end if */
			
			/* increase step */
			data->step += size;

			/* check if we have finished with this data to be sent */
			if (data->step == data->message_size) {

			release_message:
				/* notify message sent */
				if (conn->on_msg_sent)
					conn->on_msg_sent (ctx, conn, data->message, data->message_size, data->type, conn->on_msg_sent_data);

				/* decrease pending messages to be sent */
				myqtt_mutex_lock (&conn->op_mutex);
				conn->sequencer_messages--;
				myqtt_mutex_unlock (&conn->op_mutex);

				/* release connection */
				myqtt_conn_unref (conn, "sequencer");

				/* release message */
				axl_free (data->message);
				data->message = NULL;

				/* release common container */
				axl_free (data);

				axl_list_cursor_remove (cursor);
				continue;
			} /* end if */

			/* call to get next */
			axl_list_cursor_next (cursor);
		} /* end while */
		
	} /* end while */

	/* never reached */
	return NULL;
}

/** 
 * @internal
 *
 * @return axl_true if the sequencer was init, otherwise axl_false is
 * returned.
 **/
axl_bool  myqtt_sequencer_run (MyQttCtx * ctx)
{

	v_return_val_if_fail (ctx, axl_false);

	/* acquire a reference to the context to avoid loosing it
	 * during a log running sequencer not stopped */
	myqtt_ctx_ref2 (ctx, "sequencer");

	/* starts the myqtt sequencer */
	if (! myqtt_thread_create (&ctx->sequencer_thread,
				   (MyQttThreadFunc) __myqtt_sequencer_run,
				   ctx,
				   MYQTT_THREAD_CONF_END)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to initialize the sequencer thread");
		return axl_false;
	} /* end if */

	/* ok, sequencer initialized */
	return axl_true;
}


/** 
 * @internal
 * @brief Stop myqtt sequencer process.
 */
void myqtt_sequencer_stop (MyQttCtx * ctx)
{

	/* terminate sequencer thread */
	myqtt_thread_destroy (&ctx->sequencer_thread, axl_false);

	return; 
}


