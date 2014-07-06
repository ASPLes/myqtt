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
#include <stdarg.h>

/* local include */
#include <myqtt-ctx-private.h>
#include <myqtt-conn-private.h>
#include <myqtt-msg-private.h>

#define LOG_DOMAIN "myqtt-msg"

/** 
 * @internal
 *
 * @brief Support function for msg identificators.  
 *
 * This is used to generate and return the next msg identifier, an
 * unique integer value to track msgs created.
 *
 * @return Next msg identifier available.
 */
int  __myqtt_msg_get_next_id (MyQttCtx * ctx, char  * from)
{
	/* get current context */
	int         result;

	myqtt_mutex_lock (&ctx->msg_id_mutex);
	
	result = ctx->msg_id;
	ctx->msg_id++;

	myqtt_log (MYQTT_LEVEL_DEBUG, "Created msg id=%d", result);

	myqtt_mutex_unlock (&ctx->msg_id_mutex);

	return result;
}

/**
 * \defgroup myqtt_msg MyQtt Msg: Functions to manage messages received 
 */

/**
 * \addtogroup myqtt_msg
 * @{
 */


/** 
 * @internal
 * @brief reads n bytes from the connection.
 * 
 * @param connection the connection to read data.
 * @param buffer buffer to hold data.
 * @param maxlen 
 * 
 * @return how many bytes were read.
 */
int         myqtt_msg_receive_raw  (MyQttConn * connection, unsigned char  * buffer, int  maxlen)
{
	int         nread;
#if defined(ENABLE_MYQTT_LOG)
	char      * error_msg;
#endif
#if defined(ENABLE_MYQTT_LOG) && ! defined(SHOW_FORMAT_BUGS)
	MyQttCtx * ctx = myqtt_conn_get_ctx (connection);
#endif

	/* avoid calling to read when no good socket is defined */
	if (connection->session == -1)
		return -1;

 __myqtt_msg_readn_keep_reading:
	/* clear buffer */
	/* memset (buffer, 0, maxlen * sizeof (char )); */
	if ((nread = myqtt_conn_invoke_receive (connection, buffer, maxlen)) == MYQTT_SOCKET_ERROR) {
		if (errno == MYQTT_EAGAIN) {
			return 0;
		}
		if (errno == MYQTT_EWOULDBLOCK) {
			return 0;
		}
		if (errno == MYQTT_EINTR)
			goto __myqtt_msg_readn_keep_reading;
		
#if defined(ENABLE_MYQTT_LOG)
		if (errno != 0) {
			error_msg = myqtt_errno_get_last_error ();
			myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to readn=%d, error was: '%s', errno=%d (conn-id=%d, session=%d)",
				    maxlen, error_msg ? error_msg : "", errno, connection->id, connection->session);
		} /* end if */
#endif
	}

	if (nread > 0) {
		/* notify here msg received (content received) */
		myqtt_conn_set_receive_stamp (connection, (long) nread, 0);
	}

	/* ensure we don't access outside the array */
	if (nread < 0) 
		nread = 0;

	buffer[nread] = 0;
	return nread;
}

/** 
 * @internal Function used to encode remaining length on the provided
 * buffer using representation provided by the MQTT standard.
 */
axl_bool myqtt_msg_encode_remaining_length (MyQttCtx * ctx, unsigned char * input, int value, int * out_position)
{
	int  value_to_encode = value;
	int  iterator        = 0;

	int encoded_byte;
	do {
		/* check values before continue */
		if (iterator == 4) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Found internal engine encoding error, reached a position that shouldn't be reached (iterator=%d)",
				   iterator);
			return axl_false;
		} /* end if */

		/* encode byte */
		encoded_byte = value_to_encode % 128;

		/* update value */
		value_to_encode = value_to_encode / 128;

		/* signal more to come in the upper part */
		myqtt_log (MYQTT_LEVEL_DEBUG, "Encoded value %d): encoded_byte=0x%x (value_to_encode=%d)", iterator, encoded_byte, value_to_encode);
		if (value_to_encode > 0)
			encoded_byte = (encoded_byte | 0x80);
		myqtt_log (MYQTT_LEVEL_DEBUG, "Encoded value (after adding continuator): 0x%x", encoded_byte);

		/* save value at the right position and move next */
		input[iterator] = 0;
		input[iterator] = encoded_byte;
		iterator++;
		
	} while (value_to_encode > 0);

	/* report caller where he should be writing next byte */
	if (out_position)
		(*out_position) = iterator;

	return axl_true;
}


/** 
 * @internal Function that recovers remaining length from the provided
 * buffer and reports the value. The function also reports the next
 * position that should be read from the package.
 */
int      myqtt_msg_decode_remaining_length (MyQttCtx * ctx, unsigned char * input, int * out_position)
{
	int multiplier = 1;
	int value = 0;
	int iterator = 0;
	unsigned char encoded_byte;

	do {
		myqtt_log (MYQTT_LEVEL_DEBUG, "%d) current value is %d", iterator, value);
		encoded_byte = input[iterator];
		value += (encoded_byte & 0x7f) * multiplier;
		multiplier = multiplier * 128;
		if (iterator > 3)
			return -1;

		iterator++;

	} while ((encoded_byte & 128) != 0);

	/* report caller iterator */
	(*out_position) = iterator;

	return value;
}

/** 
 * @internal Allows to build a raw MQTT message to be sent over the
 * network.
 *
 * The function creates a network representation with the provided
 * arguments which is suitable for sending through the network.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param type The control packet type.
 *
 * @param dup If dup bit should be enabled.
 *
 * @param qos What type of QoS should be enabled.
 *
 * @param retain If retain bit should be enabled.
 *
 * @param size A reference to report the size of the chunk.
 *
 * The rest of parameters is a list of if parameters that configures
 * the payload to be sent. Here are the allowed combination:
 * 
 * - (MYQTT_PARAM_UTF8_STRING, length in bytes, utf-8 string)
 * - (MYQTT_PARAM_BINARY_PAYLOAD, length in bytes, binary data)
 * - (MYQTT_PARAM_16BIT_INT, value)
 * - (MYQTT_PARAM_8BIT_INT, value)
 *
 * a reference to a memory chunk plus the size of that chunk ended by
 * a NULL parameter. You must release that chunk of memory usign \ref myqtt_msg_free_build
 */
unsigned char        * myqtt_msg_build        (MyQttCtx     * ctx,
					       MyQttMsgType   type,
					       axl_bool       dup,
					       MyQttQos       qos,
					       axl_bool       retain,
					       int          * size,
					       ...)
{
	const char      * ref;
	int               ref_size;
	int               iterator = 0;
	int               total_size;
	int               total_header;
	unsigned char   * result;
	va_list           args;
	MyQttParamType    param_type;
	int               int_value;

	/* open stdargs */
	va_start (args, size);

	total_size   = 0;
	total_header = 0;
	
	do {
		/* get paramter type to process */
		param_type = va_arg (args, MyQttParamType);
		if (param_type == MYQTT_PARAM_END)
			break;

		/* reset size */
		ref_size = 0;

		switch (param_type) {

		case MYQTT_PARAM_UTF8_STRING:
			/* get size */
			ref_size = va_arg (args, int);

			/* check parameter */
			ref      = va_arg (args, const char *);
			if (ref == NULL)  {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Null parameter found while expecting UTF-8 string");
				/* call to close to avoid problems with amd64 platforms */
				va_end (args);
				return NULL;
			} /* end if */

			/* add the additional vlue */
			ref_size += 2;

			break;
		case MYQTT_PARAM_8BIT_INT:
			/* report size */
			ref_size = 1;
			/* get the value from parameters */
			int_value = va_arg (args, int);

			if (int_value > 255) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Received a bigger value than 255 for a 8bit parameter");
				/* call to close to avoid problems with amd64 platforms */
				va_end (args);
				return NULL;
			} /* end if */

			break;
		case MYQTT_PARAM_16BIT_INT:
			/* report size */
			ref_size = 2;
			/* value reported */
			int_value = va_arg (args, int);

			if (int_value > 65535) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Received a bigger value than 255 for a 8bit parameter");
				/* call to close to avoid problems with amd64 platforms */
				va_end (args);
				return NULL;
			} /* end if */

			break;
		case MYQTT_PARAM_BINARY_PAYLOAD:
			/* get size */
			ref_size = va_arg (args, int);

			/* check parameter */
			ref      = va_arg (args, const char *);
			if (ref == NULL)  {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Null parameter found while expecting binary payload");
				/* call to close to avoid problems with amd64 platforms */
				va_end (args);
				return NULL;
			} /* end if */

			break;
		default:
			break;
		}

		/* accumulate */
		total_size += ref_size;

		/* limit, never go more than 20 parameters */
		iterator += 1;

		if (iterator > 20) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Parameter list exceeded %d", iterator);
			/* call to close to avoid problems with amd64 platforms */
			va_end (args);
			return NULL;
		}

	} while ( axl_true );

	/* call to close to avoid problems with amd64 platforms */
	va_end (args);

	/* according to the total size to send */
	if (total_size <= 127) {
		/* 1 bytes from the header plus 1 for the remaining length header */
		total_size  += 2;
		total_header = 2;
	} else if (total_size <= 16383) {
		/* 1 bytes from the header plus 2 for the remaining length header */
		total_size  += 3;
		total_header = 3;
	} else if (total_size <= 2097151) {
		/* 1 bytes from the header plus 3 for the remaining length header */
		total_size  += 4;
		total_header = 4;
	} else if (total_size <= 268435455) {
		/* 1 bytes from the header plus 4 for the remaining length header */
		total_size  += 5;
		total_header = 5;
	} else {
		/* ERROR: size not suppoted */
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Requested to size an unsuppored size which is bigger than 268435455 bytes");
		return NULL;
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "Output message is %d bytes long", total_size);
	result = axl_new (unsigned char, total_size + 1);
	if (result == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to allocate memory for msg, errno=%d", errno);
		return NULL;
	} /* end if */

	/* dump MQTT control packet type */
	result[0] = (( 0x00000f & type) << 4);
	if (dup)
		myqtt_set_bit (result, 3);
	if (retain)
		myqtt_set_bit (result, 0);
	switch (qos) {
	case MYQTT_QOS_AT_MOST_ONCE:
		/* nothing requried */
		break;
	case MYQTT_QOS_AT_LEAST_ONCE_DELIVERY:
		/* set the bit required */
		myqtt_set_bit (result, 1);
		break;
	case MYQTT_QOS_EXACTLY_ONCE_DELIVERY:
		/* set the bit required */
		myqtt_set_bit (result, 2);
		break;
	} /* end if */

	/* now save remaining bytes */
	iterator = 0;
	if (! myqtt_msg_encode_remaining_length (ctx, result + 1, total_size - total_header, &iterator)) {
		axl_free (result);
		return NULL;
	} /* end if */

	/* now encode each additional part */
	va_start (args, size);

	do {
		/* get paramter type to process */
		param_type = va_arg (args, MyQttParamType);
		if (param_type == MYQTT_PARAM_END)
			break;

		/* reset size */
		ref_size = 0;

		switch (param_type) {
		case MYQTT_PARAM_UTF8_STRING:
			/* get size */
			ref_size = va_arg (args, int);

			/* store value at iterator */
			myqtt_set_16bit (ref_size, result + 1 + iterator);
			iterator += 2;

			/* check parameter */
			ref      = va_arg (args, const char *);
			
			/* now store the string itself */
			memcpy (result + 1 + iterator, ref, ref_size);
			iterator += ref_size;

			break;
		case MYQTT_PARAM_8BIT_INT:
			/* get the value from parameters */
			int_value = va_arg (args, int);

			result[iterator + 1] = int_value;
			iterator += 1;

			break;
		case MYQTT_PARAM_16BIT_INT:
			/* get the value from parameters */
			int_value = va_arg (args, int);

			myqtt_set_16bit (int_value, result + 1 + iterator);
			iterator += 2;

			break;
		case MYQTT_PARAM_BINARY_PAYLOAD:
			/* get size */
			ref_size = va_arg (args, int);

			/* check parameter */
			ref      = va_arg (args, const char *);
			
			/* now store the string itself */
			memcpy (result + 1 + iterator, ref, ref_size);
			iterator += ref_size;

			break;
		default:
			break;
		}


	} while ( axl_true );

	/* call to close to avoid problems with amd64 platforms */
	va_end (args);

	/* report size to the caller */
	if (size)
		(*size) = total_size;
	
	myqtt_log (MYQTT_LEVEL_DEBUG, "Created MQTT message string of %d bytes (iterator=%d)", total_size, iterator);
	return result;
}

/** 
 * @internal Releases a memory chunk created by \ref myqtt_msg_build.
 */
void myqtt_msg_free_build (MyQttCtx * ctx, unsigned char * msg_build, int size)
{
	/* call to release message */
	axl_free (msg_build);
	return;
}

/** 
 * @internal
 * 
 * Tries to get the next incomming msg inside the given connection. If
 * received msg is ok, it return a newly-allocated MyQttMsg with
 * its data. This function is only useful for myqtt library
 * internals. MyQtt library consumer should not use it.
 *
 * This function also close you connection if some error happens.
 * 
 * @param connection the connection where msg is going to be
 * received
 * 
 * @return a msg  or NULL if msg is wrong.
 **/
MyQttMsg * myqtt_msg_get_next     (MyQttConn * connection)
{
	int              bytes_read;
	int              remaining;
	MyQttMsg       * msg;
	unsigned char    header[6];
	unsigned char  * buffer = NULL;
	MyQttCtx       * ctx    = myqtt_conn_get_ctx (connection);
	int              iterator;

	/* check here port sharing for this connection before reading
	 * the content. The function returns axl_true if the
	 * connection is ready to be used to read content */
	if (! __myqtt_listener_check_port_sharing (ctx, connection)) 
		return NULL;

	/* before reading anything else, we have to check if previous
	 * read was complete if not, we are in a msg fragment case */
	buffer = connection->buffer;
	
	if (buffer) {
		myqtt_log (MYQTT_LEVEL_DEBUG, 
			    "received more data after a msg fragment, previous read isn't still complete");
		/* get previous msg */
		msg        = connection->last_msg;
		v_return_val_if_fail (msg, NULL);

		/* get previous remaining */
		remaining    = connection->remaining_bytes;
		myqtt_log (MYQTT_LEVEL_DEBUG, "remaining bytes to be read: %d", remaining);
		v_return_val_if_fail (remaining > 0, NULL);

		/* get previous bytes read */
		bytes_read   = connection->bytes_read;
		myqtt_log (MYQTT_LEVEL_DEBUG, "bytes already read: %d", bytes_read);

		bytes_read = myqtt_msg_receive_raw (connection, buffer + bytes_read, remaining);
		if (bytes_read == 0) {
			myqtt_msg_free (msg);
			axl_free (buffer);

			connection->buffer     = NULL;
			connection->last_msg = NULL;

			__myqtt_conn_shutdown_and_record_error (
				connection, MyQttProtocolError, 
				"remote peer have closed connection while reading the rest of the msg having received part of it");
			return NULL;
		}


		myqtt_log (MYQTT_LEVEL_DEBUG, "bytes ready this time: %d", bytes_read);

		/* check data received */
		if (bytes_read != remaining) {
			/* add bytes read to keep on reading */
			bytes_read += connection->bytes_read;
			myqtt_log (MYQTT_LEVEL_DEBUG, "the msg fragment isn't still complete, total read: %d", bytes_read);
			goto save_buffer;
		}
		
		/* We have a complete buffer for the msg, let's
		 * continue the process but, before doing that we have
		 * to restore expected state of bytes_read. */
		bytes_read = msg->size;

		connection->buffer     = NULL;
		connection->last_msg = NULL;

		myqtt_log (MYQTT_LEVEL_DEBUG, "this already complete (total size: %d", msg->size);
		goto process_buffer;
	}
	
	/* parse msg header, read the first line */
	bytes_read = myqtt_msg_receive_raw (connection, header, 2);
	if (bytes_read == -2) {
		/* count number of non-blocking operations on this
		 * connection to avoid iterating for ever */
		connection->no_data_opers++;
		if (connection->no_data_opers > 25) {
			__myqtt_conn_shutdown_and_record_error (
				connection, MyQttError, "too much no data available operations over this connection");
			return NULL;
		} /* end if */

                myqtt_log (MYQTT_LEVEL_WARNING,
			    "no data was waiting on this non-blocking connection id=%d (EWOULDBLOCK|EAGAIN errno=%d)",
			    myqtt_conn_get_id (connection), errno);
		return NULL;
	} /* end if */
	/* reset no data opers */
	connection->no_data_opers = 0;

	if (bytes_read == 0) {
		/* check if channel is expected to be closed */
		if (myqtt_conn_get_data (connection, "being_closed")) {
			__myqtt_conn_shutdown_and_record_error (
				connection, MyQttOk, 
				"connection properly closed");
			return NULL;
		}

		/* check for connection into initial connect state */
		if (connection->initial_accept) {
			/* found a connection broken in the middle of
			 * the negotiation (just before the initial
			 * step, but after the second step) */
			__myqtt_conn_shutdown_and_record_error (
				connection, MyQttProtocolError, "found connection closed before finishing negotiation, dropping..");
			return NULL;
		}
	
		/* check if we have a non-blocking connection */
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttUnnotifiedConnectionClose,
			"remote side has disconnected without closing properly this session id=%d",
			myqtt_conn_get_id (connection));
		return NULL;
	}
	if (bytes_read == -1) {
	        if (myqtt_conn_is_ok (connection, axl_false))
		        __myqtt_conn_shutdown_and_record_error (connection, MyQttProtocolError, "an error have ocurred while reading socket");
		return NULL;
	} /* end if */

	/* get remaining length values to get the final amount to read
	 * from the network */
	iterator = 0;
	while (iterator < 3) {
		/* check if the highest order bit indicates whether
		 * there is more bytes */
		if (myqtt_get_bit (header[1 + iterator], 7) == 0)
			break;

		/* next position */
		iterator++;

		/* get the value */
		bytes_read = myqtt_msg_receive_raw (connection, header + 1 + iterator, 1);
		if (bytes_read == 0) {
			/* incomplete header received */
			__myqtt_conn_shutdown_and_record_error (connection, MyQttProtocolError, "incomplete header received");
			return NULL;
		} /* end if */
	} /* end if */

	/* report content received */
	iterator  = 0;
	remaining = myqtt_msg_decode_remaining_length (ctx, header + 1, &iterator);
	myqtt_log (MYQTT_LEVEL_DEBUG, "New packet received, header size indication is: %d (iterator=%d)", remaining, iterator);
	
	/* create a msg */
	msg       = axl_new (MyQttMsg, 1);
	if (msg == NULL) {
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttMemoryFail, "Failed to allocate memory for msg");
		return NULL;
	} /* end if */

	/* report the message type */
	msg->type = (header[0] & 0xf0) >> 4;
	myqtt_log (MYQTT_LEVEL_DEBUG, "New packet received: %s, header size indication is: %d (iterator=%d)", myqtt_msg_get_type_str (msg), remaining, iterator);

	/* update message size */
	msg->size = remaining;

	/* acquire a reference to the context */
	myqtt_ctx_ref2 (ctx, "new msg");

	/* set initial ref count */
	msg->ref_count = 1;

	/* associate the next msg id available */
	msg-> id  = __myqtt_msg_get_next_id (ctx, "get-next");
	msg->ctx  = ctx;

	if (msg->size == 0) {
		/* report message with empty payload */
		return msg;
	} /* end if */

	/* allocate exactly msg->size + 1 bytes */
	buffer = malloc (sizeof (unsigned char) * msg->size + 1);
	MYQTT_CHECK_REF2 (buffer, NULL, msg, axl_free);
	
	/* read the next msg content */
	bytes_read = myqtt_msg_receive_raw (connection, buffer, msg->size);
 	if (bytes_read == 0 && errno != MYQTT_EAGAIN && errno != MYQTT_EWOULDBLOCK) {
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttProtocolError, "remote peer have closed connection while reading the rest of the msg");

		/* unref msg node allocated */
		myqtt_msg_free (msg);

		/* unref buffer allocated */
		axl_free (buffer);
		return NULL;
	}

	if (bytes_read != msg->size) {
		/* ok, we have received few bytes than expected but
		 * this is not wrong. Non-blocking sockets behave this
		 * way. What we have to do is to store the msg chunk
		 * we already read and the rest of bytes expected. 
		 *
		 * Later on, next msg_get_next calls on the same
		 * connection will return the rest of msg to be read. */

	save_buffer:
		/* save current msg */
		connection->last_msg = msg;
		
		/* save current buffer read */
		connection->buffer = buffer;
		
		/* save remaining bytes */
		connection->remaining_bytes = msg->size - bytes_read;

		/* save read bytes */
		connection->bytes_read      = bytes_read;

		myqtt_log (MYQTT_LEVEL_DEBUG, 
			   "received a msg fragment (expected: %d read: %d remaining: %d), storing into this connection id=%d",
			   msg->size, bytes_read, msg->size - bytes_read, myqtt_conn_get_id (connection));
		return NULL;
	}

process_buffer:

	/* point to the content received and nullify trailing BEEP msg */
	msg->payload   = buffer;
	((char*)msg->payload) [msg->size] = 0;

	/* get a reference to the buffer to dealloc it */
	msg->buffer    = buffer;

	/* nullify */

	return msg;

}

/** 
 * @brief Allows to get the MQTT message type from the provided message.
 *
 * @param msg The message to report type from.
 *
 * @return The message type. The function returns -1 if it fails (msg reference is NULL).
 */
MyQttMsgType  myqtt_msg_get_type              (MyQttMsg    * msg)
{
	if (msg == NULL)
		return -1;
	return msg->type;
}

/** 
 * @brief Allows to get the MQTT message type from the provided message in string format.
 *
 * This function is similar to \ref myqtt_msg_get_type but this one reports a string.
 *
 * @param msg The message to report type from.
 *
 * @return The message type in the form of a string.
 */
const char  * myqtt_msg_get_type_str          (MyQttMsg    * msg)
{
	if (msg == NULL)
		return "UNKNONW";
	switch (msg->type) {
	case MYQTT_CONNECT:
		return "CONNECT";
	case MYQTT_CONNACK:
		return "CONNACK";
	case MYQTT_PUBLISH:
		return "PUBLISH";
	case MYQTT_PUBACK:
		return "PUBACK";
	case MYQTT_PUBREC:
		return "PUBREC";
	case MYQTT_PUBREL:
		return "PUBREL";
	case MYQTT_PUBCOMP:
		return "PUBCOMP";
	case MYQTT_SUBSCRIBE:
		return "SUBSCRIBE";
	case MYQTT_SUBACK:
		return "SUBACK";
	case MYQTT_UNSUBSCRIBE:
		return "UNSUBSCRIBE";
	case MYQTT_UNSUBACK:
		return "UNSUBACK";
	case MYQTT_PINGREQ:
		return "PINGREQA";
	case MYQTT_PINGRESP:
		return "PINGRESP";
	case MYQTT_DISCONNECT:
		return "DISCONNECT";
	default:
		return "UNKNOWN";
	}
	return "UNKNOWN";
}


/** 
 * @internal
 * 
 * Sends data over the given connection
 * 
 * @param connection 
 * @param a_msg 
 * @param msg_size 
 * 
 * @return 
 */
axl_bool             myqtt_msg_send_raw     (MyQttConn * connection, const unsigned char  * a_msg, int  msg_size)
{

	MyQttCtx  * ctx    = myqtt_conn_get_ctx (connection);
	int          bytes  = 0;
 	int          total  = 0;
	char       * error_msg;
 	int          fds;
 	int          wait_result;
 	int          tries    = 3;
 	axlPointer   on_write = NULL;

	v_return_val_if_fail (connection, axl_false);
	v_return_val_if_fail (myqtt_conn_is_ok (connection, axl_false), axl_false);
	v_return_val_if_fail (a_msg, axl_false);

 again:
	if ((bytes = myqtt_conn_invoke_send (connection, a_msg + total, msg_size - total)) < 0) {
		if (errno == MYQTT_EINTR)
			goto again;
 		if ((errno == MYQTT_EWOULDBLOCK) || (errno == MYQTT_EAGAIN) || (bytes == -2)) {
 		implement_retry:
 			myqtt_log (MYQTT_LEVEL_WARNING, 
 				    "unable to write data to socket (requested %d but written %d), socket not is prepared to write, doing wait",
 				    msg_size, total);
 
 			/* create and configure waiting set */
 			if (on_write == NULL) {
 				on_write = myqtt_io_waiting_invoke_create_fd_group (ctx, WRITE_OPERATIONS);
 			} /* end if */
 
 			/* clear and add  fd */
 			myqtt_io_waiting_invoke_clear_fd_group (ctx, on_write);
 			fds = myqtt_conn_get_socket (connection);
 			if (! myqtt_io_waiting_invoke_add_to_fd_group (ctx, fds, connection, on_write)) {
				__myqtt_conn_shutdown_and_record_error (
					connection, MyQttProtocolError,
					"failed to add connection to waiting set for write operation, closing connection");
 				goto end;
 			} /* en dif */
 
 			/* perform a wait operation */
 			wait_result = myqtt_io_waiting_invoke_wait (ctx, on_write, fds + 1, WRITE_OPERATIONS);
 			switch (wait_result) {
 			case -3: /* unrecoberable error */
				__myqtt_conn_shutdown_and_record_error (
					connection, MyQttError, "unrecoberable error was found while waiting to perform write operation, closing connection");
 				goto end;
 			case -2: /* error received while waiting (soft error like signals) */
 			case -1: /* timeout received */
 			case 0:  /* nothing changed which is a kind of timeout */
 				myqtt_log (MYQTT_LEVEL_DEBUG, "found timeout while waiting to perform write operation (tries=%d)", tries);
 				tries --;
 				if (tries == 0) {
					__myqtt_conn_shutdown_and_record_error (
						connection, MyQttError,
						"found timeout while waiting to perform write operation and maximum tries were reached");
 					goto end;
 				} /* end if */
 				goto again;
 			default:
 				/* default case when it is found the socket is now available */
 				myqtt_log (MYQTT_LEVEL_DEBUG, "now the socket is able to perform a write operation, doing so (total written until now %d)..",
 					    total);
 				goto again;
 			} /* end switch */
			goto end;
		}
		
		/* check if socket have been disconnected (macro
		 * definition at myqtt.h) */
		if (myqtt_is_disconnected) {
			__myqtt_conn_shutdown_and_record_error (
				connection, MyQttProtocolError,
				"remote peer have closed connection");
			goto end;
		}
		error_msg = myqtt_errno_get_last_error ();
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttError, "unable to write data to socket: %s",
			error_msg ? error_msg : "");
		goto end;
	}

	myqtt_log (MYQTT_LEVEL_DEBUG, "bytes written: %d", bytes);

	if (bytes == 0) {
		__myqtt_conn_shutdown_and_record_error (
			connection, MyQttProtocolError,
			"remote peer have closed before sending proper close connection, closing");
		goto end;
	}

 	/* sum total amount of data */
 	if (bytes > 0) {
		/* notify content written (content received) */
		myqtt_conn_set_receive_stamp (connection, 0, bytes);

 		total += bytes;
	}

 	if (total != msg_size) {
 		myqtt_log (MYQTT_LEVEL_CRITICAL, "write request mismatch with write done (%d != %d), pending tries=%d",
 			    total, msg_size, tries);
 		tries--;
 		if (tries == 0) {
 			goto end;
		}
 
 		/* do retry operation */
 		goto implement_retry;
  	}
	myqtt_log (MYQTT_LEVEL_DEBUG, "write on socket request=%d written=%d", msg_size, bytes);
 end:

 	/* clear waiting set before returning */
 	if (on_write)
 		myqtt_io_waiting_invoke_destroy_fd_group (ctx, on_write);
	
 	return (total == msg_size);
}


/** 
 * @brief Increases the msg reference counting.
 *
 * The function isn't thread safe, which means you should call to this
 * function from several threads. In is safe to call this function on
 * every place you receive a msg (second and first level invocation
 * handler).
 *
 * Once you use this function, you can't call directly to \ref
 * myqtt_msg_free. Instead a call to \ref myqtt_msg_unref is
 * required. 
 *
 * Calling to \ref myqtt_msg_free when no reference count was
 * increases is the same as calling to \ref myqtt_msg_unref. This
 * means that code written against MyQtt Library 0.9.0 (or previous)
 * will keep working doing calls to \ref myqtt_msg_free.
 * 
 * @param msg The msg to increase its reference counting. 
 * 
 * @return axl_true if the msg reference counting was
 * increased. Otherwise, axl_false is returned and the reference counting
 * is left untouched.
 */
axl_bool           myqtt_msg_ref                   (MyQttMsg * msg)
{

	/* check reference received */
	v_return_val_if_fail (msg, axl_false);

	myqtt_mutex_lock (&msg->mutex);

	/* increase the msg counting */
	msg->ref_count++;

	myqtt_mutex_unlock (&msg->mutex);

	return axl_true;
}

/** 
 * @brief Allows to decrease the msg reference counting, making an
 * automatic call to myqtt_msg_free if the reference counting reach
 * 0.
 * 
 * @param msg The msg to be unreferenced (and deallocated if the
 * ref count reach 0).
 */
void          myqtt_msg_unref                 (MyQttMsg * msg)
{
	if (msg == NULL)
		return;

	myqtt_mutex_lock (&msg->mutex);
	
	/* decrease reference counting */
	msg->ref_count--;

	myqtt_mutex_unlock (&msg->mutex);

	/* check and dealloc */
	if (msg->ref_count == 0) {
		myqtt_msg_free (msg);
	}
	
	return;
}

/** 
 * @brief Returns current reference counting for the msg received.
 * 
 * @param msg The msg that is required to return the current
 * reference counting.
 * 
 * @return Reference counting or -1 if it fails. The function could
 * only fail if a null reference is received.
 */
int           myqtt_msg_ref_count             (MyQttMsg * msg)
{
	int value;

	v_return_val_if_fail (msg, -1);
	myqtt_mutex_lock (&msg->mutex);
	value = msg->ref_count;
	myqtt_mutex_unlock (&msg->mutex);

	return value;
}


/** 
 * @brief Deallocate the msg. You shouldn't call this directly,
 * instead use \ref myqtt_msg_unref.
 *
 * Frees a allocated \ref MyQttMsg. Keep in mind that, unless
 * stated, \ref MyQttMsg object received from the MyQtt Library
 * API are not required to be unrefered. 
 *
 * In particular, first and second level invocation automatically
 * handle msg disposing. See \ref myqtt_manual_dispatch_schema
 * "MyQtt Msg Dispatch schema".
 *
 * @param msg The msg to free.
 **/
void          myqtt_msg_free (MyQttMsg * msg)
{
#if defined(ENABLE_MYQTT_LOG)
	MyQttCtx * ctx;
#endif
	if (msg == NULL)
		return;

	/* log a msg deallocated message */
#if defined(ENABLE_MYQTT_LOG)
	ctx = msg->ctx;
	myqtt_log (MYQTT_LEVEL_DEBUG, "deallocating msg id=%d", msg->id);
#endif

	/* free msg payload (first checking for content, and, if not
	 * defined, then payload) */
	if (msg->buffer != NULL)
		axl_free (msg->buffer);
	else if (msg->content != NULL)
		axl_free (msg->content);
	else if (msg->payload != NULL)
		axl_free ((char *) msg->payload);

	/* release reference to the context */
	myqtt_ctx_unref2 (&msg->ctx, "end msg");

	/* free the msg node itself */
	axl_free (msg);
	return;
}

/** 
 * @brief Allows to get the unique msg identifier for the given
 * msg.
 *
 * Inside MyQtt Library, every msg created is flagged with an
 * unique msg identifier which is not part of the BEEP protocol,
 * transmitted or received from the remote side.
 *
 * This unique identifiers was created for the internal MyQtt Library
 * support but, it has become a useful mechanism to have msgs
 * identified. 
 *
 * Application level could use this value to implement fast msg
 * recognition rather than using \ref myqtt_msg_are_equal, which is
 * more expensive.
 *
 * MyQtt Library will ensure, as long as the process is in memory
 * that every unique identifier returned/generated for a msg will be
 * unique.
 * 
 * @param msg The msg where the unique identifier value will be
 * get.
 * 
 * @return Returns the unique msg identifier or -1 if it fails. The
 * only way for this function to fail is to receive a NULL value.
 */
int           myqtt_msg_get_id                (MyQttMsg * msg)
{
	v_return_val_if_fail (msg, -1);
	return msg->id;
}

/**
 * @brief Returns the payload associated to the given msg. 
 * 
 * Return actual msg payload. You must not free returned reference.
 *
 * @param msg The msg where the payload is being requested.
 * 
 * @return the actual msg's payload or NULL if fail.
 **/
const void *  myqtt_msg_get_payload  (MyQttMsg * msg)
{
	v_return_val_if_fail (msg, NULL);

	return msg->payload;
}

/**
 * @brief Returns the current payload size the given msg have
 * without taking into account mime headers.
 *
 * This function will return current payload size stored on the given
 * msg, skipping mime header size. This is because MyQtt Library
 * manages mime headers for every msg received, making a separation
 * between the mime header part and the mime content.
 *
 * However, if you don't use mime headers, this function will return
 * the entire payload size.
 *
 * If you need to get current msg content size, including payload
 * size and mime content size, use \ref myqtt_msg_get_content_size.
 *
 * 
 * @param msg The msg where the payload size will be reported.
 *
 * @return the actual payload size or -1 if fail
 **/
int    myqtt_msg_get_payload_size (MyQttMsg * msg)
{
	v_return_val_if_fail (msg, -1);

	return msg->size;

}

/** 
 * @brief Allows to get the context reference under which the msg
 * was created.
 * 
 * @param msg The msg that is required to return its context.
 * 
 * @return A reference to the context (\ref MyQttCtx) or NULL if it
 * fails.
 */
MyQttCtx   * myqtt_msg_get_ctx               (MyQttMsg * msg)
{
	if (msg == NULL)
		return NULL;

	/* return context configured */
	return msg->ctx;
}

/* @} */
