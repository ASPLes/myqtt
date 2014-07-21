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

/* local include */
#include <myqtt-ctx-private.h>

#define LOG_DOMAIN "myqtt-sequencer"

axl_bool myqtt_sequencer_queue_data (MyQttCtx * ctx, MyQttSequencerData * data)
{
	v_return_val_if_fail (data, axl_false);

	/* check state before handling this message with the sequencer */
	if (ctx->myqtt_exit) {
		axl_free (data->message);
		axl_free (data);
		return axl_false;
	}

	/* lock connection and queue message */
	myqtt_mutex_lock (&ctx->pending_messages_m);

	/* queue message */
	axl_list_append (ctx->pending_messages, data);

	myqtt_mutex_unlock (&ctx->pending_messages_m);

	/* signal sequencer to move on! */
	myqtt_cond_signal (&ctx->pending_messages_c);


	return axl_true;
}

axl_bool myqtt_sequencer_process_pending_messages (axlPointer _data, axlPointer _conn)
{
	MyQttConn          * conn = _conn;
	MyQtt              * ctx;
	MyQttSequencerData * data = _data;
	int                  size;

	/* get context */
	ctx  = conn->ctx;

	/* write step */
	if ((data->message_size - data->step) > 4096)
		size = 4096;
	else
		size = data->message_size - data-> step

	if (! myqtt_msg_send_raw (conn, data->message + data->step, size)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to send MQTT message (type: %d, size: %d (total: %d), step: %d) error was errno=%d", 
			   data->type, data->size, size, data->step, data->errno);
		
		return;
	} /* end if */

	/* increase step */
	data->step += size;

	return;
}


axlPointer __myqtt_sequencer_run (axlPointer _data)
{

	/* get current context */
	MyQttCtx             * ctx = _data;

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
			
			myqtt_log (MYQTT_LEVEL_DEBUG, "exiting myqtt sequencer thread ..");

			/* release reference acquired here */
			myqtt_ctx_unref (&ctx);

			return NULL;
		} /* end if */

		/* process all ready administrative channels (channel 0) */
		axl_list_lookup (ctx->pending_messages, myqtt_sequencer_process_pending_messages, ctx);
		
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

	return; 
}



axl_bool      myqtt_sequencer_direct_send (MyQttConn    * connection,
					   MyQttWriterData    * packet)
{
	/* reply number */
	axl_bool    result = axl_true;
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx * ctx    = myqtt_conn_get_ctx (connection);
#endif

	if (! myqtt_msg_send_raw (connection, packet->msg, packet->size)) {
		/* drop a log */
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to send frame over connection id=%d: errno=(%d): %s", 
			    myqtt_conn_get_id (connection),
			    errno, myqtt_errno_get_error (errno));
		
		/* set as non connected and flag the result */
		result = axl_false;
	}
	
	/* nothing more */
	return result;
}


