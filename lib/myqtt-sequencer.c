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


	return axl_true;
}

axl_bool myqtt_sequencer_remove_message_sent (MyQttCtx * ctx)
{


	return axl_false;
}

int myqtt_sequencer_build_packet_to_send (MyQttCtx           * ctx, 
					  MyQttConn          * conn, 
					  MyQttSequencerData * data, 
					  MyQttWriterData    * packet)
{
 	int          size_to_copy        = 0;

	/* clear packet */
	memset (packet, 0, sizeof (MyQttWriterData));

	/* check particular case where an empty message is to be sent
	 * and the message is NUL */
	if (data->message_size == 0)
		goto build_frame;
  
	/* check that the next_frame_size do not report wrong values */
	if (size_to_copy > data->message_size || size_to_copy <= 0) 
		return 0;

	/* we have the payload on buffer */
build_frame:


	
	/* return size used from the entire message */
	return size_to_copy;
}

/** 
 * @internal Function that does a send round for a channel. The
 * function assumes the channel is not stalled (but can end stalled
 * after the function finished).
 *
 */ 
void __myqtt_sequencer_do_send_round (MyQttCtx * ctx, MyQttConn * conn, axl_bool * paused, axl_bool * complete)
{

	/* that's all myqtt sequencer process can do */
	return;
}

axlPointer __myqtt_sequencer_run (axlPointer _data)
{

	/* never reached */
	return NULL;
}

/** 
 * @internal
 * 
 * Starts the myqtt sequencer process. This process with the myqtt
 * writer process conforms the subsystem which actually send a message
 * inside myqtt. While myqtt reader process threats all incoming
 * message and dispatch them to appropriate destination, the myqtt
 * sequencer waits for messages to be send over channels.
 *
 * Once the myqtt sequencer receive a petition to send a message, it
 * checks if actual channel window size is appropriate to actual
 * message being sent. If not, the myqtt sequencer splits the message
 * into many pieces until all pieces can be sent using the actual
 * channel window size.
 *
 * Once the message is segmented, the myqtt sequencer build up the
 * frames and enqueue the messages inside the channel waiting
 * queue. Once the sequencer have queue the first frame to be sent, it
 * signal the myqtt writer to starting to send messages.
 *
 *
 * @return axl_true if the sequencer was init, otherwise axl_false is
 * returned.
 **/
axl_bool  myqtt_sequencer_run (MyQttCtx * ctx)
{

	v_return_val_if_fail (ctx, axl_false);


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


